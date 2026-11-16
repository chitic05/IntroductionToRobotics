#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ShiftRegister.hpp"
#include "esp_log.h"
#include "esp_timer.h"

#define BUTTON_1 GPIO_NUM_34
#define LED_1 GPIO_NUM_21

#define BUTTON_2 GPIO_NUM_39
#define LED_2 GPIO_NUM_22

#define BUTTON_3 GPIO_NUM_36
#define LED_3 GPIO_NUM_23

#define LED_ELEVATOR GPIO_NUM_17

static const char* TAG = "ELEVATOR";

enum ElevatorStates{
    IDLE,
    CLOSING_DOORS,
    MOVING,
    OPENING_DOORS
};

struct Elevator{
    ElevatorStates state = IDLE;
    uint8_t currentFloor = 0;
    uint8_t targetFloor = 0;
    int8_t direction = 1;
    // 1 = up, -1 = down
    bool ledState = false;
};

struct Button {
    gpio_num_t pin, led;
    uint8_t history = 0xFF;
    volatile bool pressed = false;
    volatile bool inQueue = false; // Urmărește dacă etajul este în coada liftului
};

struct InputTimerArgs {
    Button* buttons;
    SemaphoreHandle_t inputSemaphore;
};

void init_pins() {
    // Buttons (Requires external pull-ups on GPIOs 34, 36, 39)
    gpio_config_t buttons_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_1) |
                        (1ULL << BUTTON_2) |
                        (1ULL << BUTTON_3),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&buttons_cfg);

    // LEDs
    gpio_config_t leds_cfg = {
        .pin_bit_mask = (1ULL << LED_1) |
                        (1ULL << LED_2) |
                        (1ULL << LED_3) |
                        (1ULL << LED_ELEVATOR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&leds_cfg);
}

void findNextFloor(Elevator& e, Button* b){
    if(e.direction == 1){
        for(int i = e.currentFloor; i < 3; i++){
            if(b[i].inQueue){
                e.targetFloor = i;
                return;
            }
        }
        e.direction = -1;
    }
    if(e.direction == -1){
        for(int i = e.currentFloor; i >= 0; i--){
            if(b[i].inQueue){
                e.targetFloor = i;
                return;
            } 
        }
        e.direction = 1;
    }
    e.targetFloor = e.currentFloor;
}

void inputTimerCb(void* param) {
    auto* args = static_cast<InputTimerArgs*>(param);

    if (xSemaphoreTake(args->inputSemaphore, 0) == pdPASS) {
        for (int i = 0; i < 3; ++i) {
            args->buttons[i].history = (args->buttons[i].history << 1) | gpio_get_level(args->buttons[i].pin);
            
            if (args->buttons[i].history == 0x00 && !args->buttons[i].pressed) {
                args->buttons[i].pressed = true;
                if (!args->buttons[i].inQueue) {
                    args->buttons[i].inQueue = true;
                    gpio_set_level(args->buttons[i].led, 1);
                }
            } 
            else if (args->buttons[i].history == 0xFF && args->buttons[i].pressed) {
                args->buttons[i].pressed = false;
            }
        }
        xSemaphoreGive(args->inputSemaphore);
    }
}
void elevatorLedBlinkCb(void* param){
    bool* ledState = static_cast<bool*>(param);
    gpio_set_level(LED_ELEVATOR, *ledState);
    *ledState = !(*ledState);
}

extern "C" void app_main(void) {
    init_pins();

    static ShiftRegister screen_sh(GPIO_NUM_19, GPIO_NUM_5, GPIO_NUM_18);

    // Initialize buttons with their specific pins
    static Button buttons[3] = {
        { .pin = BUTTON_1, .led = LED_1 },
        { .pin = BUTTON_2, .led = LED_2 },
        { .pin = BUTTON_3, .led = LED_3 }
    };

    SemaphoreHandle_t inputSemaphore = xSemaphoreCreateMutex();
    static InputTimerArgs args = {
        .buttons = buttons,
        .inputSemaphore = inputSemaphore
    };
    esp_timer_create_args_t inputTimerArgs = {
        .callback = inputTimerCb,
        .arg = &args,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "input",
        .skip_unhandled_events = true
    };
    esp_timer_handle_t inputTimerHandle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&inputTimerArgs, &inputTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(inputTimerHandle, 3000)); // 3ms debounce tick

    static Elevator elevator;

    esp_timer_create_args_t blinkTimerArgs = {
        .callback = elevatorLedBlinkCb,
        .arg = &elevator.ledState,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "blink",
        .skip_unhandled_events = true
    };
    esp_timer_handle_t blinKHandle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&blinkTimerArgs, &blinKHandle));

    screen_sh.writeValue(ShiftRegister::number[elevator.currentFloor]);
    ESP_LOGI(TAG, "Lift pornit. Etaj curent: %d (IDLE)", elevator.currentFloor);

    while (1) {
        switch(elevator.state){
            case IDLE: {
                if (xSemaphoreTake(inputSemaphore, pdMS_TO_TICKS(10)) == pdPASS) {
                    findNextFloor(elevator, buttons);
                    if(elevator.targetFloor != elevator.currentFloor){
                        ESP_LOGI(TAG, "Comanda primita -> Destinatie: etajul %d (Directie: %s)", 
                                 elevator.targetFloor, elevator.direction == 1 ? "SUS" : "JOS");
                        elevator.state = CLOSING_DOORS;
                    } else if (buttons[elevator.currentFloor].inQueue) {
                        ESP_LOGI(TAG, "Liftul este deja la etajul %d -> Se redeschid usile", elevator.currentFloor);
                        elevator.state = OPENING_DOORS;
                    }
                    xSemaphoreGive(inputSemaphore);
                }
                break;
            }
            case CLOSING_DOORS: {
                ESP_LOGI(TAG, "Se inchid usile... (3 secunde)");
                // play sound for closing
                vTaskDelay(pdMS_TO_TICKS(3000));
                // stop sound

                ESP_LOGI(TAG, "Usile s-au inchis. Liftul porneste...");
                elevator.state = MOVING;
                elevator.ledState = 1;
                ESP_ERROR_CHECK(esp_timer_start_periodic(blinKHandle, 200 * 1000)); // 200ms on/off
                break;
            }
            case MOVING: {
                bool shouldStop = false;
                if (xSemaphoreTake(inputSemaphore, pdMS_TO_TICKS(10)) == pdPASS){
                    shouldStop = buttons[elevator.currentFloor].inQueue;
                    xSemaphoreGive(inputSemaphore);
                }

                if (shouldStop){
                    ESP_LOGI(TAG, "Oprire la etajul %d", elevator.currentFloor);
                    elevator.state = OPENING_DOORS;
                    ESP_ERROR_CHECK(esp_timer_stop(blinKHandle));
                    elevator.ledState = 0;
                    ESP_ERROR_CHECK(gpio_set_level(LED_ELEVATOR, elevator.ledState));
                } else if(elevator.currentFloor != elevator.targetFloor){
                    ESP_LOGI(TAG, "In miscare (%s)... Trecere spre etajul %d", 
                             elevator.direction == 1 ? "SUS" : "JOS", 
                             elevator.currentFloor + elevator.direction);
                    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 secunda intre etaje
                    elevator.currentFloor += elevator.direction;
                    screen_sh.writeValue(ShiftRegister::number[elevator.currentFloor]);
                    ESP_LOGI(TAG, "Ajuns in dreptul etajului %d", elevator.currentFloor);
                }
                break;
            }
            case OPENING_DOORS: {
                if(xSemaphoreTake(inputSemaphore, pdMS_TO_TICKS(10)) == pdPASS){
                    ESP_ERROR_CHECK(gpio_set_level(buttons[elevator.currentFloor].led, 0));
                    buttons[elevator.currentFloor].inQueue = false;
                    xSemaphoreGive(inputSemaphore);

                    ESP_LOGI(TAG, "Se deschid usile la etajul %d... (3 secunde)", elevator.currentFloor);
                    // play sound for arriving
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    
                    ESP_LOGI(TAG, "Usi deschise complet. Liftul intra in IDLE");
                    elevator.state = IDLE;
                }
                break; 
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
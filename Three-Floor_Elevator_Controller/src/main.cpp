#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ShiftRegister.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include <atomic>

#define BUTTON_1 GPIO_NUM_34
#define LED_1 GPIO_NUM_21

#define BUTTON_2 GPIO_NUM_39
#define LED_2 GPIO_NUM_22

#define BUTTON_3 GPIO_NUM_36
#define LED_3 GPIO_NUM_23

#define LED_ELEVATOR GPIO_NUM_17
#define BUZZER GPIO_NUM_16

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
    int8_t direction = 1; // 1 = up, -1 = down
    std::atomic<bool> ledState = false;
    std::atomic<bool> isMoving = false;
};

struct Button {
    gpio_num_t pin, led;
    uint8_t history = 0xFF;
    bool pressed = false;
    std::atomic<bool> inQueue = false;
};

struct Note{
    int freq = 0;
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

    // Buzzer ledc
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t chan_cfg = {
        .gpio_num = BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&chan_cfg);
    ledc_fade_func_install(0);
}

void findNextFloor(Elevator& e, Button* b){
    if(e.direction == 1){
        for(int i = e.currentFloor; i < 3; i++){
            if(b[i].inQueue.load()){
                e.targetFloor = i;
                return;
            }
        }
        e.direction = -1;
    }
    if(e.direction == -1){
        for(int i = e.currentFloor; i >= 0; i--){
            if(b[i].inQueue.load()){
                e.targetFloor = i;
                return;
            } 
        }
        e.direction = 1;
    }
    e.targetFloor = e.currentFloor;
}

void inputTimerCb(void* param) {
    auto* buttons = static_cast<Button*>(param);

    for (int i = 0; i < 3; ++i) {
        buttons[i].history = (buttons[i].history << 1) | gpio_get_level(buttons[i].pin);
        
        if (buttons[i].history == 0x00 && !buttons[i].pressed) {
            buttons[i].pressed = true;
            if (!buttons[i].inQueue.load()) {
                buttons[i].inQueue.store(true);
                gpio_set_level(buttons[i].led, 1);
            }
        } 
        else if (buttons[i].history == 0xFF && buttons[i].pressed) {
            buttons[i].pressed = false;
        }
    }
}

void elevatorLedBlinkCb(void* param) {
    auto* e = static_cast<Elevator*>(param);
    if (e->isMoving.load()) {
        gpio_set_level(LED_ELEVATOR, e->ledState.load());
        e->ledState.store(!e->ledState.load());
    } else {
        e->ledState.store(false);
        gpio_set_level(LED_ELEVATOR, 0);
    }
}

extern "C" void app_main(void) {
    init_pins();

    static ShiftRegister screen_sh(GPIO_NUM_19, GPIO_NUM_5, GPIO_NUM_18);

    static Button buttons[3] = {
        { .pin = BUTTON_1, .led = LED_1 },
        { .pin = BUTTON_2, .led = LED_2 },
        { .pin = BUTTON_3, .led = LED_3 }
    };

    esp_timer_create_args_t inputTimerArgs = {
        .callback = inputTimerCb,
        .arg = buttons,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "input",
        .skip_unhandled_events = true
    };
    esp_timer_handle_t inputTimerHandle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&inputTimerArgs, &inputTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(inputTimerHandle, 3000)); // 3ms debounce tick

    static Elevator elevator;
    Note doorsClosing{392}, doorsOpening{1047}; // Sol4 (jos) -> Do6 (sus)
    
    esp_timer_create_args_t blinkTimerArgs = {
        .callback = elevatorLedBlinkCb,
        .arg = &elevator,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "blink",
        .skip_unhandled_events = true
    };
    esp_timer_handle_t blinkHandle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&blinkTimerArgs, &blinkHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(blinkHandle, 300 * 1000));

    screen_sh.writeValue(ShiftRegister::number[elevator.currentFloor]);
    ESP_LOGI(TAG, "Lift pornit. Etaj curent: %d (IDLE)", elevator.currentFloor);

    while (1) {
        switch(elevator.state){
            case IDLE: {
                findNextFloor(elevator, buttons);
                if(elevator.targetFloor != elevator.currentFloor){
                    ESP_LOGI(TAG, "Comanda primita -> Destinatie: etajul %d (Directie: %s)", 
                             elevator.targetFloor, elevator.direction == 1 ? "SUS" : "JOS");
                    elevator.state = CLOSING_DOORS;
                } else if (buttons[elevator.currentFloor].inQueue.load()) {
                    ESP_LOGI(TAG, "Liftul este deja la etajul %d -> Se redeschid usile", elevator.currentFloor);
                    elevator.state = OPENING_DOORS;
                }
                break;
            }
            case CLOSING_DOORS: {
                ESP_LOGI(TAG, "Se inchid usile... (2 secunde)");
                ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, doorsClosing.freq);
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128, 0);
                vTaskDelay(pdMS_TO_TICKS(2000));
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 0);

                // Verifică dacă s-a apăsat un etaj nou cât timp ușile s-au deschis/închis
                findNextFloor(elevator, buttons);
                
                if (elevator.currentFloor != elevator.targetFloor) {
                    ESP_LOGI(TAG, "Usile s-au inchis. Liftul porneste spre etajul %d...", elevator.targetFloor);
                    elevator.state = MOVING;
                    elevator.ledState.store(true);
                    elevator.isMoving.store(true);
                } else {
                    ESP_LOGI(TAG, "Usile s-au inchis. Liftul intra in IDLE");
                    elevator.state = IDLE;
                }
                break;
            }
            case MOVING: {
                if (buttons[elevator.currentFloor].inQueue.load()){
                    ESP_LOGI(TAG, "Oprire la etajul %d", elevator.currentFloor);
                    elevator.state = OPENING_DOORS;
                    elevator.isMoving.store(false);
                    elevator.ledState.store(false);
                    ESP_ERROR_CHECK(gpio_set_level(LED_ELEVATOR, 0));
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
                buttons[elevator.currentFloor].inQueue.store(false);
                ESP_ERROR_CHECK(gpio_set_level(buttons[elevator.currentFloor].led, 0));

                ESP_LOGI(TAG, "Se deschid usile la etajul %d... (2 secunde)", elevator.currentFloor);
                ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, doorsOpening.freq);
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512, 0);
                vTaskDelay(pdMS_TO_TICKS(2000));
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 0);

                ESP_LOGI(TAG, "Usi deschise complet. Liftul intra in CLOSING DOORS");
                elevator.state = CLOSING_DOORS;
                break; 
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ledc_fade_func_uninstall();
}
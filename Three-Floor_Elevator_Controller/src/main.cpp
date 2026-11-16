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

struct Button {
    gpio_num_t pin, led;
    uint8_t history = 0xFF;
    volatile bool inQueue = false;
};

struct InputTimerArgs {
    Button* buttons;
    SemaphoreHandle_t inputSemaphore;
};

struct Sound {
    int duration;
    int freq;
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
        .pin_bit_mask = (1ULL << LED_1) | // led_button 1
                        (1ULL << LED_2) | // led_button 2
                        (1ULL << LED_3),  // led_button 3
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&leds_cfg);
}

void inputTimerCb(void* param) {
    auto* args = static_cast<InputTimerArgs*>(param);

    // Use 0 ticks wait in a fast periodic timer so it never blocks the timer service task
    if (xSemaphoreTake(args->inputSemaphore, 0) == pdPASS) {
        for (int i = 0; i < 3; ++i) {
            args->buttons[i].history = (args->buttons[i].history << 1) | gpio_get_level(args->buttons[i].pin);
            
            if (args->buttons[i].history == 0x00 && !args->buttons[i].inQueue) {
                args->buttons[i].inQueue = true;
                gpio_set_level(args->buttons[i].led, 1);
                // Avoid ESP_LOGI here; handle state changes in app_main or via a queue
            }
        }
        xSemaphoreGive(args->inputSemaphore);
    }
}

void playSoundTask(void* param) {
    Sound s;
    QueueHandle_t queueHandle = static_cast<QueueHandle_t>(param);
    while (1) {
        if (xQueueReceive(queueHandle, &s, portMAX_DELAY) == pdPASS) {
            // TODO: Start PWM/LEDc tone with s.freq
            vTaskDelay(pdMS_TO_TICKS(s.duration));
            // TODO: Stop tone
        }
    }
}

extern "C" void app_main(void) {
    init_pins();

    static ShiftRegister screen_sh(GPIO_NUM_19, GPIO_NUM_5, GPIO_NUM_18);
    screen_sh.writeValue(ShiftRegister::number[8]);

    QueueHandle_t soundQueueHandle = xQueueCreate(5, sizeof(Sound));
    TaskHandle_t playSoundHandler = NULL;
    if (xTaskCreate(playSoundTask, "playSound", 2048, (void*)soundQueueHandle, 2, &playSoundHandler) != pdPASS) {
        ESP_LOGE("main", "Couldn't create the play sound task!");
    }

    // Initialize buttons with their specific pins
    static Button buttons[3] = {
        { .pin = BUTTON_1, .led = LED_1},
        { .pin = BUTTON_2, .led = LED_2},
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

    bool lastState[3] = {false, false, false};
    while (1) {
        if (xSemaphoreTake(inputSemaphore, pdMS_TO_TICKS(10)) == pdPASS) {
            for (int i = 0; i < 3; ++i) {
                if(buttons[i].inQueue == true && lastState[i] == false)
                    lastState[i] = true;
            }
            xSemaphoreGive(inputSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
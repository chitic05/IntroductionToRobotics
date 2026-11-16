#pragma once
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include <cmath>
struct JoystickData{
    float x, y;
    bool buttonPressed;
};

class Joystick{
    gpio_num_t SW_Pin;
    adc_channel_t Y_Pin, X_Pin;
    adc_oneshot_unit_handle_t adc1_handle;
    const int DEADZONE;
    const int inputDelay;
    const int CENTER_VAL = 2048;
    uint64_t lastGetData = 0;
    uint8_t buttonHistory = 0xFF;

    JoystickData data;

    float applyFiltration(int rawValue) {
        if (std::abs(rawValue - CENTER_VAL) < DEADZONE) {
            rawValue = 0;
        }else rawValue -= CENTER_VAL;
        float normalized = (float)rawValue / CENTER_VAL; // sa obtinem [-1,1]
        if(normalized < -1.0f) return -1.0f;
        else if(normalized > 1.0f) return 1.0f;
        return normalized;
    }
    public:
        explicit Joystick(gpio_num_t SW, adc_channel_t Y, adc_channel_t X, int inputDelay=1000, int dz = 100)
            : SW_Pin(SW), Y_Pin(Y), X_Pin(X), DEADZONE(dz), inputDelay(inputDelay), data{0,0,0}{
                gpio_config_t button_config{
                    .pin_bit_mask = 1ULL << SW_Pin,
                    .mode = GPIO_MODE_INPUT,
                    .pull_up_en = GPIO_PULLUP_ENABLE,
                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                    .intr_type = GPIO_INTR_DISABLE
                };
                ESP_ERROR_CHECK(gpio_config(&button_config));

                adc_oneshot_unit_init_cfg_t init_config = {
                    .unit_id = ADC_UNIT_1, 
                    .ulp_mode = ADC_ULP_MODE_DISABLE,
                };
                ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

                adc_oneshot_chan_cfg_t chan_cfg{
                    .atten = ADC_ATTEN_DB_12,
                    .bitwidth = ADC_BITWIDTH_DEFAULT
                };
                ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, Y_Pin, &chan_cfg));
                ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, X_Pin, &chan_cfg));
            }
        
        const JoystickData& getData(){
            uint64_t now = esp_timer_get_time();
            if(now - lastGetData > inputDelay){
                lastGetData = now;
                int rawX = 0;
                int rawY = 0;
                
                adc_oneshot_read(adc1_handle, X_Pin, &rawX);
                adc_oneshot_read(adc1_handle, Y_Pin, &rawY);

                data.x = applyFiltration(rawX);
                data.y = applyFiltration(rawY);
                
                buttonHistory = (buttonHistory << 1) | gpio_get_level(SW_Pin);
       
                data.buttonPressed = (buttonHistory == 0x00);
                

            }
            return data;

        }
        Joystick(const Joystick& other)=delete;
        Joystick& operator=(const Joystick& other) = delete;
};
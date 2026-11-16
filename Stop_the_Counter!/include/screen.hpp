#pragma once
#include "driver/gpio.h"
#include "ShiftRegister.hpp"
#include <atomic>

constexpr int puteri10[] = { 1, 10, 100, 1000 };

class Screen {
    private:
        ShiftRegister sr;
        gpio_num_t digitPins[4];
        uint8_t currentDigit = 0;
        std::atomic<int> currentVal; 
    public:
        explicit Screen(gpio_num_t DS, gpio_num_t SH, gpio_num_t ST, const gpio_num_t digits[4])
            : sr(DS, SH, ST, 1), currentVal(0) {

            uint64_t pinMask = 0;
            for (int i = 0; i < 4; i++) {
                digitPins[i] = digits[i];
                pinMask |= (1ULL << digitPins[i]);
            }

            gpio_config_t io_conf = {
                .pin_bit_mask = pinMask,
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            ESP_ERROR_CHECK(gpio_config(&io_conf));

            for (int i = 0; i < 4; i++) {
                gpio_set_level(digitPins[i], 1); 
            }
        }

        void printCurrentValue() {
            int prevDigit = (currentDigit == 0) ? 3 : currentDigit - 1;
            gpio_set_level(digitPins[prevDigit], 1); 

            int val = currentVal.load();
            int digitVal = (val / puteri10[currentDigit]) % 10;
            if(digitVal == 0 && val < puteri10[currentDigit] && currentDigit != 0) sr.writeValue(0x00);
            else sr.writeValue(ShiftRegister::number[digitVal]);
            
            gpio_set_level(digitPins[currentDigit], 0);

            currentDigit = (currentDigit + 1) % 4;
        }

        void changeValue(int val) {
            currentVal.store(val);
        }

        Screen(const Screen&) = delete;
        Screen& operator=(const Screen&) = delete;
};
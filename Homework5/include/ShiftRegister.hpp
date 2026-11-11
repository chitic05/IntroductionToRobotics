#pragma once
#include "driver/gpio.h"

class ShiftRegister {
    private:
        gpio_num_t dataPin;
        gpio_num_t shiftPin;
        gpio_num_t latchPin;
        uint8_t registerCount;

        void pulsePin(gpio_num_t pin) {
            gpio_set_level(pin, 1);
            gpio_set_level(pin, 0);
        }

    public:
        inline static uint8_t number[9] = {
            0x0,
        };
        explicit ShiftRegister(gpio_num_t DS, gpio_num_t SH, gpio_num_t ST, uint8_t count = 1)
            : dataPin(DS), shiftPin(SH), latchPin(ST), registerCount(count) {
            
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << dataPin) | (1ULL << shiftPin) | (1ULL << latchPin),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            gpio_config(&io_conf);

            gpio_set_level(dataPin, 0);
            gpio_set_level(shiftPin, 0);
            gpio_set_level(latchPin, 0);

            clear();
        }

        void clear() {
            gpio_set_level(dataPin, 0);
            
            uint16_t totalBits = registerCount * 8;
            for (uint16_t i = 0; i < totalBits; i++) {
                pulsePin(shiftPin);
            }

            pulsePin(latchPin);
        }

        ShiftRegister(const ShiftRegister& other) = delete;
        ShiftRegister& operator=(const ShiftRegister& other) = delete;
};

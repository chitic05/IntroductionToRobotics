#pragma once
#include "driver/gpio.h"
#include "rom/ets_sys.h"

class ShiftRegister {
    private:
        gpio_num_t dataPin;
        gpio_num_t shiftPin;
        gpio_num_t latchPin;
        uint8_t registerCount;

        void pulsePin(gpio_num_t pin) {
            gpio_set_level(pin, 1);
            ets_delay_us(1);
            gpio_set_level(pin, 0);
        }

    public:
        inline static uint8_t number[10] = {
            0b00111111, // 0
            0b00000110, // 1
            0b01011011, // 2
            0b01001111, // 3
            0b01100110, // 4
            0b01101101, // 5
            0b01111101, // 6
            0b00000111, // 7
            0b01111111, // 8
            0b01101111  // 9
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
            ESP_ERROR_CHECK(gpio_config(&io_conf));

            gpio_set_level(dataPin, 0);
            gpio_set_level(shiftPin, 0);
            gpio_set_level(latchPin, 0);

            writeValue(0x00);
        }

        void writeValue(uint8_t val) {
            for (int i = 7; i >= 0; i--) {
                gpio_set_level(dataPin, (val >> i) & 0x01);
                pulsePin(shiftPin);
            }
            pulsePin(latchPin);
        }

        ShiftRegister(const ShiftRegister& other) = delete;
        ShiftRegister& operator=(const ShiftRegister& other) = delete;
};
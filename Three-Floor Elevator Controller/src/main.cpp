#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ShiftRegister.hpp"

struct button{
    uint8_t history = 0xFF;
    uint8_t level = 0;
    bool pressed = false; 
};

void init_pins(){
    //Buttons
    gpio_config_t buttons_cfg ={
        .pin_bit_mask = (1ULL << GPIO_NUM_34) | //button 1
                        (1ULL << GPIO_NUM_39) | //button 2
                        (1ULL << GPIO_NUM_36),   //button 3
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&buttons_cfg);

    //LEDS

    gpio_config_t leds_cfg ={
        .pin_bit_mask = (1ULL << GPIO_NUM_1) | //led_button 1
                        (1ULL << GPIO_NUM_22) | //led_button 2
                        (1ULL << GPIO_NUM_23),   //led_button 3
                                                //led lift
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&leds_cfg);

}

extern "C" void app_main(void) {
    init_pins();
    static ShiftRegister screen_sh(GPIO_NUM_19,GPIO_NUM_5 ,GPIO_NUM_18);

    
    vTaskDelete(NULL);
}
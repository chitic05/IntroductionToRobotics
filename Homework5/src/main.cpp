#include "esp_log.h"
#include "ShiftRegister.hpp"

extern "C" void app_main(void){
    ShiftRegister(GPIO_NUM_23, GPIO_NUM_18, GPIO_NUM_22, 1);
    ESP_LOGI("main", "Is working");
}
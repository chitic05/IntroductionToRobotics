#include "esp_log.h"
#include "screen.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "joystick.hpp"

static void screen_timer_cb(void* arg) {
    Screen* screen_ptr = static_cast<Screen*>(arg);
    screen_ptr->printCurrentValue();
}

int getRandomNumber(const int min, const int max){
    uint32_t raw = esp_random();
    return min + raw % (max-min+1);
}

enum GameState{
    START,
    PLAYING,
    END
};


extern "C" void app_main(void) {
    static Screen screen(GPIO_NUM_23, GPIO_NUM_18, GPIO_NUM_22, (const gpio_num_t[]) {
        GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_33
    });

    Joystick js(GPIO_NUM_32, ADC_CHANNEL_7, ADC_CHANNEL_6, 2000, 200);

    const esp_timer_create_args_t timer_args = {
        .callback = &screen_timer_cb,
        .arg = &screen,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "display_timer",
        .skip_unhandled_events = true
    };

    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 3000));

    GameState state = START;

    int startValue = 9999; 
    int currentValue = startValue;
    int targetValue = 0;
    bool targetValueChosed = false;
    int error = 0;
    uint64_t lastUpdateTime = esp_timer_get_time();

    uint64_t lastPrint = esp_timer_get_time();
    
    while (1) {
        uint64_t now = esp_timer_get_time();
        const JoystickData& data = js.getData();
        switch(state){
            case START:
                if(!targetValueChosed){
                    targetValue = getRandomNumber(4000,8000);
                    screen.changeValue(targetValue);
                    targetValueChosed = true;
                    currentValue = 9999;
                }
                //if joystick up switch to play
                if(data.x > 0.5f){
                    state = PLAYING;
                    lastUpdateTime = now;
                    screen.changeValue(currentValue);
                }
                break;
            case PLAYING:{
                
                uint64_t elapsed_ms = (now - lastUpdateTime) / 1000;
                
               
                    currentValue --;
                    if(currentValue < 0) currentValue = 0; 
                    
                    screen.changeValue(currentValue);
                    
                   
                    lastUpdateTime += (elapsed_ms * 1000);
                
                
                if(data.buttonPressed || currentValue == 0){
                    state = END;
                    error = abs(targetValue-currentValue);
                    ESP_LOGI("Main", "Target: %d | Result: %d | Error: %d", targetValue, currentValue, error);
                }
                break;
            }  
            case END:
                if(data.x < -0.5f){
                    state = START;
                    targetValueChosed = false;
                }
                break;
            }


        vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelete(nullptr);
    
}
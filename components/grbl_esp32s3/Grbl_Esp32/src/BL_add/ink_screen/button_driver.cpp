#include "button_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "BUTTON_DRIVER";

// 按键状态记录（用于消抖）
static uint32_t last_button_time[3] = {0, 0, 0};
static bool last_button_state[3] = {false, false, false};

/**
 * @brief 初始化按键GPIO
 */
bool button_init(void) {
    ESP_LOGI(TAG, "初始化按键驱动...");
    ESP_LOGI(TAG, "  按键1 (向上):   GPIO%d", BUTTON_UP_PIN);
    ESP_LOGI(TAG, "  按键2 (确认):   GPIO%d", BUTTON_CONFIRM_PIN);
    ESP_LOGI(TAG, "  按键3 (向下):   GPIO%d", BUTTON_DOWN_PIN);
    
    // 配置GPIO为输入模式，启用上拉电阻
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;           // 禁用中断
    io_conf.mode = GPIO_MODE_INPUT;                  // 输入模式
    io_conf.pin_bit_mask = (1ULL << BUTTON_UP_PIN) | 
                           (1ULL << BUTTON_CONFIRM_PIN) | 
                           (1ULL << BUTTON_DOWN_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;    // 禁用下拉
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;         // 启用上拉（按键按下时为低电平）
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "按键GPIO配置失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "按键驱动初始化成功");
    return true;
}

/**
 * @brief 读取按键原始状态
 * @param button 按键编号（1、2、3）
 * @return true 按键按下（低电平）
 */
bool button_read_raw(uint8_t button) {
    gpio_num_t pin;
    
    switch (button) {
        case 1:
            pin = BUTTON_UP_PIN;
            break;
        case 2:
            pin = BUTTON_CONFIRM_PIN;
            break;
        case 3:
            pin = BUTTON_DOWN_PIN;
            break;
        default:
            return false;
    }
    
    // 按键按下时为低电平，返回true表示按下
    return (gpio_get_level(pin) == 0);
}

/**
 * @brief 扫描按键状态（带消抖）
 * @return ButtonState 返回按下的按键或BUTTON_NONE
 */
ButtonState button_scan(void) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 依次检查三个按键（优先级：按键1 > 按键2 > 按键3）
    for (int i = 0; i < 3; i++) {
        bool current_state = button_read_raw(i + 1);
        
        // 检测到按键按下（低电平）
        if (current_state && !last_button_state[i]) {
            // 消抖检查：与上次按键时间间隔必须大于BUTTON_DEBOUNCE_MS
            if (current_time - last_button_time[i] > BUTTON_DEBOUNCE_MS) {
                last_button_time[i] = current_time;
                last_button_state[i] = true;
                
                ESP_LOGI(TAG, "按键%d按下", i + 1);
                return (ButtonState)(i + 1);
            }
        }
        // 检测到按键释放
        else if (!current_state && last_button_state[i]) {
            last_button_state[i] = false;
        }
    }
    
    return BUTTON_NONE;
}

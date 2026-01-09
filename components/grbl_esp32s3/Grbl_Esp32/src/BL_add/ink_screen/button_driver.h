#pragma once

#include <stdint.h>
#include "driver/gpio.h"

// 按键引脚定义
#define BUTTON_UP_PIN       GPIO_NUM_40    // 按键1：向上
#define BUTTON_CONFIRM_PIN  GPIO_NUM_41    // 按键2：确认
#define BUTTON_DOWN_PIN     GPIO_NUM_42    // 按键3：向下

// 按键消抖时间（毫秒）
#define BUTTON_DEBOUNCE_MS  50

// 按键状态枚举
typedef enum {
    BUTTON_NONE = 0,      // 无按键按下
    BUTTON_UP = 1,        // 按键1（向上）
    BUTTON_CONFIRM = 2,   // 按键2（确认）
    BUTTON_DOWN = 3       // 按键3（向下）
} ButtonState;

/**
 * @brief 初始化按键GPIO
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool button_init(void);

/**
 * @brief 扫描按键状态（带消抖）
 * @return ButtonState 返回按下的按键（1、2、3）或0（无按键）
 */
ButtonState button_scan(void);

/**
 * @brief 读取按键原始状态（不带消抖）
 * @param button 按键编号（1、2、3）
 * @return true 按键按下
 * @return false 按键未按下
 */
bool button_read_raw(uint8_t button);

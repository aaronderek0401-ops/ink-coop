/**
 * @file chinese_text_display.cpp
 * @brief 中文文本显示助手 - 使用SD卡.bin字库在墨水屏上显示中文
 */

#include "chinese_text_display.h"
#include "sd_font_loader.h"
#include "esp_log.h"

static const char* TAG = "ChineseTextDisplay";

// 全局SD字库加载器实例
SDFontLoader g_font_loader;

bool initChineseFontFromSD(const char* bin_path, int font_size) {
    return g_font_loader.init(bin_path, font_size);
}


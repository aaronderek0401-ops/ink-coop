/**
 * @file chinese_text_display_impl.h
 * @brief 中文文本显示助手 - 模板函数实现
 * 
 * 本文件包含中文显示相关模板函数的实现
 * 支持在任何 GxEPD2 显示设备上绘制中文文本
 */

#ifndef CHINESE_TEXT_DISPLAY_IMPL_H
#define CHINESE_TEXT_DISPLAY_IMPL_H

#include "sd_font_loader.h"
#include "esp_log.h"
#include <string.h>

// 从 chinese_text_display.cpp 中声明的全局字库加载器
extern SDFontLoader g_font_loader;

static const char* TAG_IMPL = "ChineseTextImpl";

/**
 * @brief 在墨水屏上绘制单个中文字符
 * @param display GxEPD2显示对象引用
 * @param x X坐标
 * @param y Y坐标
 * @param unicode Unicode编码
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 */
template<typename T>
void drawChineseChar(T& display, int16_t x, int16_t y, uint16_t unicode, uint16_t color) {
    if (!g_font_loader.isInitialized()) {
        ESP_LOGE(TAG_IMPL, "字库未初始化,请先调用 initChineseFontFromSD()");
        return;
    }
    
    int font_size = g_font_loader.getFontSize();
    int glyph_size = g_font_loader.getGlyphSize();
    
    // 分配缓冲区用于存储字模数据
    uint8_t* glyph_buffer = (uint8_t*)malloc(glyph_size);
    if (!glyph_buffer) {
        ESP_LOGE(TAG_IMPL, "内存分配失败");
        return;
    }
    
    // 获取字模数据
    if (!g_font_loader.getCharGlyph(unicode, glyph_buffer)) {
        ESP_LOGW(TAG_IMPL, "无法获取字符 0x%04X 的字模", unicode);
        free(glyph_buffer);
        return;
    }
    
    // 逐像素绘制
    int bytes_per_row = (font_size + 7) / 8;
    for (int row = 0; row < font_size; row++) {
        for (int col = 0; col < font_size; col++) {
            int byte_index = row * bytes_per_row + col / 8;
            int bit_index = 7 - (col % 8);
            
            if (glyph_buffer[byte_index] & (1 << bit_index)) {
                display.drawPixel(x + col, y + row, color);
            }
        }
    }
    
    free(glyph_buffer);
}

/**
 * @brief 在墨水屏上绘制中文文本 (支持自动换行)
 * @param display GxEPD2显示对象引用
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param text UTF-8编码的文本
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 * @return 绘制结束后的Y坐标
 */
template<typename T>
int16_t drawChineseText(T& display, int16_t x, int16_t y, const char* text, uint16_t color) {
    if (!g_font_loader.isInitialized()) {
        ESP_LOGE(TAG_IMPL, "字库未初始化,请先调用 initChineseFontFromSD()");
        return y;
    }
    
    if (!text || text[0] == '\0') {
        return y;
    }
    
    int font_size = g_font_loader.getFontSize();
    int16_t cursor_x = x;
    int16_t cursor_y = y;
    int16_t display_width = display.width();
    
    const char* p = text;
    while (*p) {
        int bytes_consumed = 0;
        uint16_t unicode = g_font_loader.utf8ToUnicode(p, &bytes_consumed);
        
        if (bytes_consumed == 0) {
            // 无效字符,跳过
            p++;
            continue;
        }
        
        // 检查是否需要换行
        if (cursor_x + font_size > display_width) {
            cursor_x = x;
            cursor_y += font_size + 2; // 行间距2像素
        }
        
        // 绘制字符
        if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
            // 中文字符
            drawChineseChar(display, cursor_x, cursor_y, unicode, color);
            cursor_x += font_size;
        } else if (unicode == '\n') {
            // 换行符
            cursor_x = x;
            cursor_y += font_size + 2;
        } else if (unicode == ' ') {
            // 空格
            cursor_x += font_size / 2;
        } else if (unicode >= 32 && unicode <= 126) {
            // ASCII字符 (使用内置字体)
            display.setCursor(cursor_x, cursor_y + font_size - 2);
            display.print((char)unicode);
            cursor_x += font_size / 2; // ASCII字符占半个中文字符宽度
        } else {
            // 其他字符,尝试作为中文字符处理
            drawChineseChar(display, cursor_x, cursor_y, unicode, color);
            cursor_x += font_size;
        }
        
        p += bytes_consumed;
    }
    
    return cursor_y + font_size;
}

/**
 * @brief 在墨水屏上绘制居中的中文文本
 * @param display GxEPD2显示对象引用
 * @param y Y坐标
 * @param text UTF-8编码的文本
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 * @return 绘制结束后的Y坐标
 */
template<typename T>
int16_t drawChineseTextCentered(T& display, int16_t y, const char* text, uint16_t color) {
    if (!g_font_loader.isInitialized()) {
        ESP_LOGE(TAG_IMPL, "字库未初始化,请先调用 initChineseFontFromSD()");
        return y;
    }
    
    if (!text || text[0] == '\0') {
        return y;
    }
    
    int font_size = g_font_loader.getFontSize();
    
    // 计算文本宽度
    int text_width = 0;
    const char* p = text;
    while (*p) {
        int bytes_consumed = 0;
        uint16_t unicode = g_font_loader.utf8ToUnicode(p, &bytes_consumed);
        
        if (bytes_consumed == 0) {
            p++;
            continue;
        }
        
        if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
            // 中文字符
            text_width += font_size;
        } else if (unicode == ' ') {
            // 空格
            text_width += font_size / 2;
        } else if (unicode >= 32 && unicode <= 126) {
            // ASCII字符
            text_width += font_size / 2;
        } else {
            // 其他字符当作中文字符
            text_width += font_size;
        }
        
        p += bytes_consumed;
    }
    
    // 计算居中的X坐标
    int16_t display_width = display.width();
    int16_t x = (display_width - text_width) / 2;
    
    if (x < 0) {
        x = 0;
    }
    
    // 绘制文本
    return drawChineseText(display, x, y, text, color);
}

#endif // CHINESE_TEXT_DISPLAY_IMPL_H

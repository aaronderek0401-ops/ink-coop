/**
 * @file chinese_text_display_impl.h
 * @brief 中文文本显示助手 - 模板函数实现
 * 
 * 本文件包含中文显示相关模板函数的实现
 * 支持在任何 GxEPD2 显示设备上绘制中文文本
 */

#ifndef CHINESE_TEXT_DISPLAY_IMPL_H
#define CHINESE_TEXT_DISPLAY_IMPL_H

#include "chinese_font_cache.h"  // 包含字体缓存系统和 PSRAM 字体功能
#include "sd_font_loader.h"      // SDFontLoader 类定义
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
    
    bool got_from_source = false;
    
    // ========== 只使用 PSRAM 完整字库 ==========
    const FullFontData* psram_font = findPSRAMFontBySize(font_size);
    if (psram_font && psram_font->is_loaded) {
        got_from_source = getCharGlyphFromPSRAM(psram_font, unicode, glyph_buffer);
        if (got_from_source) {
            ESP_LOGD(TAG_IMPL, "✅ 从 PSRAM 获取字符 0x%04X", unicode);
        } else {
            ESP_LOGW(TAG_IMPL, "❌ PSRAM中未找到字符 0x%04X", unicode);
        }
    } else {
        ESP_LOGW(TAG_IMPL, "❌ 未找到字号 %d 的PSRAM字库", font_size);
    }
    
    // ========== 优先级2: 缓存系统 (已禁用) ==========
    /*
    if (!got_from_source) {
        if (font_size == 16) {
            ChineseFontCache& cache = getFontCache16();
            got_from_source = cache.getCharGlyph(unicode, FONT_16x16, glyph_buffer);
        } else if (font_size == 20) {
            ChineseFontCache& cache = getFontCache20();
            got_from_source = cache.getCharGlyph(unicode, FONT_20x20, glyph_buffer);
        } else if (font_size == 24) {
            ChineseFontCache& cache = getFontCache24();
            got_from_source = cache.getCharGlyph(unicode, FONT_24x24, glyph_buffer);
        }
        
        if (got_from_source) {
            ESP_LOGD(TAG_IMPL, "✅ 从缓存获取字符 0x%04X", unicode);
        }
    }
    */
    
    // ========== 优先级3: SD卡直接读取 (已禁用) ==========
    /*
    if (!got_from_source) {
        if (!g_font_loader.getCharGlyph(unicode, glyph_buffer)) {
            ESP_LOGW(TAG_IMPL, "❌ 无法获取字符 0x%04X 的字模", unicode);
            free(glyph_buffer);
            return;
        }
        ESP_LOGD(TAG_IMPL, "⚠️ 从 SD 卡获取字符 0x%04X (慢)", unicode);
    }
    */
    
    // 如果没有获取到字模,直接返回
    if (!got_from_source) {
        ESP_LOGE(TAG_IMPL, "❌ 无法获取字符 0x%04X 的字模", unicode);
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
    
    ESP_LOGI(TAG_IMPL, "开始绘制中文文本: \"%s\" at (%d,%d)", text, x, y);
    
    int font_size = g_font_loader.getFontSize();
    int16_t cursor_x = x;
    int16_t cursor_y = y;
    int16_t display_width = display.width();
    int char_count = 0;
    
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
            char_count++;
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
            char_count++;
        }
        
        p += bytes_consumed;
    }
    
    ESP_LOGI(TAG_IMPL, "✅ 完成绘制，共 %d 个中文字符", char_count);
    
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

/**
 * @brief 直接使用缓存系统绘制单个中文字符(不依赖g_font_loader)
 * @param display GxEPD2显示对象引用
 * @param x X坐标
 * @param y Y坐标
 * @param unicode Unicode编码
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 * @param font_size 字体大小 (16, 20, 24, 28等)
 */
template<typename T>
void drawChineseCharWithCache(T& display, int16_t x, int16_t y, uint16_t unicode, uint16_t color, int font_size) {
    // 根据字体大小计算字模数据大小
    int bytes_per_row = (font_size + 7) / 8;
    int glyph_size = bytes_per_row * font_size;
    
    // 分配缓冲区用于存储字模数据
    uint8_t* glyph_buffer = (uint8_t*)malloc(glyph_size);
    if (!glyph_buffer) {
        ESP_LOGE(TAG_IMPL, "内存分配失败");
        return;
    }
    
    bool got_glyph = false;
    
    // ========== 只使用 PSRAM 完整字库 ==========
    const FullFontData* psram_font = findPSRAMFontBySize(font_size);
    if (psram_font && psram_font->is_loaded) {
        got_glyph = getCharGlyphFromPSRAM(psram_font, unicode, glyph_buffer);
        if (got_glyph) {
            ESP_LOGI(TAG_IMPL, "✅ 从 PSRAM 获取字符 0x%04X (字号:%d)", unicode, font_size);
        }
    }
    
    // ========== 缓存系统 (已禁用) ==========
    /*
    // 对于中文字符,优先使用Fangsong缓存系统
    if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
        if (font_size == 16) {
            ChineseFontCache& cache = getFontCache16();
            got_glyph = cache.getCharGlyph(unicode, FONT_16x16, glyph_buffer);
        } else if (font_size == 20) {
            ChineseFontCache& cache = getFontCache20();
            got_glyph = cache.getCharGlyph(unicode, FONT_20x20, glyph_buffer);
        } else if (font_size == 24) {
            ChineseFontCache& cache = getFontCache24();
            got_glyph = cache.getCharGlyph(unicode, FONT_24x24, glyph_buffer);
        }
        
        if (got_glyph) {
            ESP_LOGI(TAG_IMPL, "从Fangsong缓存获取中文字符 0x%04X", unicode);
        }
    } else {
        // 对于非中文字符(如ASCII),尝试从PSRAM获取
        const FullFontData* psram_font = findPSRAMFontBySize(font_size);
        if (psram_font) {
            got_glyph = getCharGlyphFromPSRAM(psram_font, unicode, glyph_buffer);
            if (got_glyph) {
                ESP_LOGI(TAG_IMPL, "从Comic Sans PSRAM获取字符 0x%04X", unicode);
            }
        }
    }
    */
    
    if (!got_glyph) {
        ESP_LOGE(TAG_IMPL, "❌ 未找到字符 0x%04X (字号:%d)", unicode, font_size);
        free(glyph_buffer);
        return;
    }
    
    // 逐像素绘制
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
 * @brief 直接使用缓存系统绘制中文文本(不依赖g_font_loader)
 * @param display GxEPD2显示对象引用
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param text UTF-8编码的文本
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 * @return 绘制结束后的Y坐标
 */
template<typename T>
int16_t drawChineseTextWithCache(T& display, int16_t x, int16_t y, const char* text, uint16_t color) {
    if (!g_current_psram_font) {
        ESP_LOGE(TAG_IMPL, "❌ 未选择字体，请先调用 switchToPSRAMFont()");
        return y;
    }
    
    int font_size = getFontSize(g_current_psram_font);
    const char* p = text;
    int16_t current_x = x;
    int16_t current_y = y;
    
    int display_width = display.width();
    int char_spacing = 2; // 字符间距
    
    while (*p) {
        uint16_t unicode = 0;
        int bytes_consumed = 0;
        
        // UTF-8 解码
        if ((*p & 0x80) == 0) {
            // 1字节 ASCII
            unicode = *p;
            bytes_consumed = 1;
        } else if ((*p & 0xE0) == 0xC0) {
            // 2字节
            unicode = ((*p & 0x1F) << 6) | (*(p+1) & 0x3F);
            bytes_consumed = 2;
        } else if ((*p & 0xF0) == 0xE0) {
            // 3字节 (中文在这里)
            unicode = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
            bytes_consumed = 3;
        } else {
            // 不支持的编码
            p++;
            continue;
        }
        
        // 检查是否需要换行
        if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
            // 中文字符
            if (current_x + font_size > display_width) {
                current_x = x;
                current_y += font_size + 5; // 换行
            }
            
            drawChineseCharWithCache(display, current_x, current_y, unicode, color, font_size);
            current_x += font_size + char_spacing;
            
        } else if (unicode == '\n') {
            // 手动换行
            current_x = x;
            current_y += font_size + 5;
        } else if (unicode == ' ') {
            // 空格
            current_x += font_size / 2;
        } else if (unicode >= 32 && unicode <= 126) {
            // ASCII字符 - 使用display默认字体
            if (current_x + font_size/2 > display_width) {
                current_x = x;
                current_y += font_size + 5;
            }
            display.print((char)unicode);
            current_x += font_size / 2;
        }
        
        p += bytes_consumed;
    }
    
    return current_y + font_size;
}

/**
 * @brief 使用PSRAM字体绘制ASCII英文文本
 * @param display GxEPD2显示对象引用
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param text ASCII文本
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 * @param font_size 字体大小 (16, 20, 24, 28等)
 * @return 绘制结束后的Y坐标
 */
template<typename T>
int16_t drawEnglishTextFromPSRAM(T& display, int16_t x, int16_t y, const char* text, uint16_t color, int font_size) {
    // 查找对应尺寸的PSRAM字体
    const FullFontData* psram_font = findPSRAMFontBySize(font_size);
    if (!psram_font) {
        ESP_LOGE(TAG_IMPL, "未找到 %dx%d 的PSRAM字体", font_size, font_size);
        return y;
    }
    
    int bytes_per_row = (font_size + 7) / 8;
    int glyph_size = bytes_per_row * font_size;
    uint8_t* glyph_buffer = (uint8_t*)malloc(glyph_size);
    if (!glyph_buffer) {
        ESP_LOGE(TAG_IMPL, "内存分配失败");
        return y;
    }
    
    int16_t current_x = x;
    int16_t current_y = y;
    int display_width = display.width();
    int char_spacing = 2;
    
    for (const char* p = text; *p; p++) {
        char c = *p;
        
        // 检查换行
        if (c == '\n') {
            current_x = x;
            current_y += font_size + 5;
            continue;
        }
        
        // 检查空格
        if (c == ' ') {
            current_x += font_size / 2;
            continue;
        }
        
        // 只处理ASCII可打印字符
        if (c < 32 || c > 126) {
            continue;
        }
        
        // 检查是否需要换行
        if (current_x + font_size > display_width) {
            current_x = x;
            current_y += font_size + 5;
        }
        
        // 从PSRAM获取ASCII字符字模 (假设ASCII在文件开头)
        // 注意: 这里假设字体文件中ASCII字符的索引就是字符码
        uint16_t char_index = (uint16_t)c;
        if (getCharGlyphFromPSRAM(psram_font, char_index, glyph_buffer)) {
            // 逐像素绘制
            for (int row = 0; row < font_size; row++) {
                for (int col = 0; col < font_size; col++) {
                    int byte_index = row * bytes_per_row + col / 8;
                    int bit_index = 7 - (col % 8);
                    
                    if (glyph_buffer[byte_index] & (1 << bit_index)) {
                        display.drawPixel(current_x + col, current_y + row, color);
                    }
                }
            }
            current_x += font_size / 2 + char_spacing; // 英文字符间距
            ESP_LOGI(TAG_IMPL, "从PSRAM绘制ASCII字符 '%c' (0x%02X)", c, c);
        } else {
            ESP_LOGW(TAG_IMPL, "PSRAM中未找到字符 '%c' (0x%02X)", c, c);
        }
    }
    
    free(glyph_buffer);
    return current_y + font_size;
}

/**
 * @brief 按字体名称切换当前 PSRAM 字体
 * @param font_name 字体名称 (如 "comic_sans_ms_20x20", "fangsong_gb2312_28x28")
 * @return true 切换成功, false 切换失败
 * 
 * 使用示例:
 *   switchToPSRAMFont("comic_sans_ms_20x20");        // 切换到 20x20 Comic Sans
 *   drawEnglishText(display, 10, 50, "Hello");       // 使用当前字体绘制
 *   
 *   switchToPSRAMFont("comic_sans_ms_bold_28x28");   // 切换到 28x28 Comic Sans Bold
 *   drawEnglishText(display, 10, 100, "Big");        // 使用新字体绘制
 */
inline bool switchToPSRAMFont(const char* font_name) {
    if (!font_name) {
        ESP_LOGE(TAG_IMPL, "❌ 字体名称不能为空");
        return false;
    }
    
    // 直接按字体名称查找
    const FullFontData* font = findPSRAMFontByName(font_name);
    if (!font) {
        ESP_LOGE(TAG_IMPL, "❌ 字体 '%s' 未找到或未加载到PSRAM", font_name);
        ESP_LOGI(TAG_IMPL, "提示: 字体名称应该是去掉.bin扩展名的文件名");
        return false;
    }
    
    // 切换全局当前字体
    g_current_psram_font = font;
    
    const char* name = getFontName(font);
    int size = getFontSize(font);
    ESP_LOGI(TAG_IMPL, "✅ 已切换到字体 '%s' (大小:%dx%d)", 
             name ? name : "unknown", size, size);
    
    return true;
}

/**
 * @brief 按字体大小切换当前 PSRAM 字体
 * @param font_size 字体大小 (16, 20, 24, 28, 32)
 * @return true 切换成功, false 切换失败
 * 
 * 使用示例:
 *   switchToPSRAMFontBySize(20);  // 切换到 20x20 字体
 *   drawEnglishText(display, 10, 50, "Hello");
 */
inline bool switchToPSRAMFontBySize(int font_size) {
    const FullFontData* font = findPSRAMFontBySize(font_size);
    if (!font) {
        ESP_LOGE(TAG_IMPL, "❌ 字体大小 %d 未找到或未加载到PSRAM", font_size);
        return false;
    }
    
    g_current_psram_font = font;
    ESP_LOGI(TAG_IMPL, "✅ 已切换到字体大小 %dx%d", font_size, font_size);
    
    return true;
}

/**
 * @brief 使用当前字体绘制英文文本 (不指定字体大小)
 * @param display GxEPD2显示对象引用
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param text 英文文本 (支持ASCII + IPA音标)
 * @param color 颜色
 * @return 下一行的Y坐标
 * 
 * 使用示例:
 *   switchToPSRAMFontBySize(20);
 *   drawEnglishText(display, 10, 50, "Hello World!", GxEPD_BLACK);
 */
template<typename T>
int16_t drawEnglishText(T& display, int16_t x, int16_t y, const char* text, uint16_t color) {
    if (!g_current_psram_font) {
        ESP_LOGE(TAG_IMPL, "❌ 未选择字体，请先调用 switchToPSRAMFontBySize() 或 switchToPSRAMFont()");
        return y;
    }
    
    if (!text) {
        ESP_LOGW(TAG_IMPL, "文本为空");
        return y;
    }
    
    int font_size = getFontSize(g_current_psram_font);
    int glyph_size = getGlyphSize(g_current_psram_font);
    int bytes_per_row = (font_size + 7) / 8;
    const int char_spacing = 2; // 字符间距
    
    uint8_t* glyph_buffer = (uint8_t*)malloc(glyph_size);
    if (!glyph_buffer) {
        ESP_LOGE(TAG_IMPL, "内存分配失败");
        return y;
    }
    
    int16_t current_x = x;
    int16_t current_y = y;
    
    for (int i = 0; text[i] != '\0'; ) {
        uint16_t unicode = 0;
        int bytes_consumed = 0;
        
        unsigned char c = (unsigned char)text[i];
        
        // 换行符处理
        if (c == '\n') {
            current_x = x;
            current_y += font_size;
            i++;
            continue;
        }
        
        // UTF-8 解码
        if (c < 0x80) {
            // 单字节 ASCII (0x00-0x7F)
            unicode = c;
            bytes_consumed = 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            // 双字节 UTF-8 (110xxxxx 10xxxxxx)
            if (text[i+1] != '\0') {
                unicode = ((c & 0x1F) << 6) | ((unsigned char)text[i+1] & 0x3F);
                bytes_consumed = 2;
            } else {
                // 不完整的 UTF-8 序列
                ESP_LOGW(TAG_IMPL, "不完整的 UTF-8 序列");
                i++;
                continue;
            }
        }
        else if ((c & 0xF0) == 0xE0) {
            // 三字节 UTF-8 (1110xxxx 10xxxxxx 10xxxxxx)
            if (text[i+1] != '\0' && text[i+2] != '\0') {
                unicode = ((c & 0x0F) << 12) | 
                         (((unsigned char)text[i+1] & 0x3F) << 6) | 
                         ((unsigned char)text[i+2] & 0x3F);
                bytes_consumed = 3;
            } else {
                ESP_LOGW(TAG_IMPL, "不完整的 UTF-8 序列");
                i++;
                continue;
            }
        }
        else {
            // 四字节或无效的 UTF-8
            ESP_LOGW(TAG_IMPL, "不支持的 UTF-8 字符: 0x%02X", c);
            i++;
            continue;
        }
        
        // 获取字符字模
        if (getCharGlyphFromPSRAM(g_current_psram_font, unicode, glyph_buffer)) {
            // 逐像素绘制
            for (int row = 0; row < font_size; row++) {
                for (int col = 0; col < font_size; col++) {
                    int byte_index = row * bytes_per_row + col / 8;
                    int bit_index = 7 - (col % 8);
                    
                    if (glyph_buffer[byte_index] & (1 << bit_index)) {
                        display.drawPixel(current_x + col, current_y + row, color);
                    }
                }
            }
            current_x += font_size / 2 + char_spacing;
        } else {
            ESP_LOGW(TAG_IMPL, "字符 U+%04X 未找到", unicode);
        }
        
        i += bytes_consumed;
    }
    
    free(glyph_buffer);
    return current_y + font_size;
}

/**
 * @brief 使用当前字体绘制居中的英文文本
 * @param display GxEPD2显示对象引用
 * @param y Y坐标
 * @param text 英文文本
 * @param color 颜色
 * @return 下一行的Y坐标
 */
template<typename T>
int16_t drawEnglishTextCentered(T& display, int16_t y, const char* text, uint16_t color) {
    if (!g_current_psram_font) {
        ESP_LOGE(TAG_IMPL, "❌ 未选择字体");
        return y;
    }
    
    if (!text) return y;
    
    int font_size = getFontSize(g_current_psram_font);
    int text_len = strlen(text);
    int text_width = text_len * (font_size / 2 + 2);
    int16_t x = (display.width() - text_width) / 2;
    
    return drawEnglishText(display, x, y, text, color);
}

#endif // CHINESE_TEXT_DISPLAY_IMPL_H



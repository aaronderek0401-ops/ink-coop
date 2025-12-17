/**
 * @file multi_font_manager.cpp
 * @brief 多字体管理器实现 - 支持在不同场景使用不同大小的字体
 */

#include "multi_font_manager.h"
#include "sd_font_loader.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "MultiFontMgr";

// 全局字体加载器
static SDFontLoader g_word_font_loader;      // 24x24 英文单词字体
static SDFontLoader g_phonetic_font_loader;  // 16x16 英文音标字体
static bool g_initialized = false;

/**
 * @brief 初始化多字体管理器
 */
bool initMultiFontManager(const char* word_font_path, const char* phonetic_font_path) {
    if (g_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }
    
    if (!word_font_path || !phonetic_font_path) {
        ESP_LOGE(TAG, "Invalid font paths");
        return false;
    }
    
    // 初始化单词字体 (24x24)
    if (!g_word_font_loader.init(word_font_path, 24)) {
        ESP_LOGE(TAG, "Failed to initialize word font: %s", word_font_path);
        return false;
    }
    
    // 初始化音标字体 (16x16)
    if (!g_phonetic_font_loader.init(phonetic_font_path, 16)) {
        ESP_LOGE(TAG, "Failed to initialize phonetic font: %s", phonetic_font_path);
        return false;
    }
    
    g_initialized = true;
    ESP_LOGI(TAG, "✅ Multi-font manager initialized successfully");
    ESP_LOGI(TAG, "  Word font (24x24): %s", word_font_path);
    ESP_LOGI(TAG, "  Phonetic font (16x16): %s", phonetic_font_path);
    
    return true;
}

/**
 * @brief 获取单个英文字符的字模
 * ASCII码映射到0x0020-0x007E范围
 */
bool getEnglishCharGlyph(int font_type, uint8_t ascii_code, uint8_t* out_buffer) {
    if (!g_initialized || !out_buffer) {
        ESP_LOGE(TAG, "Not initialized or invalid buffer");
        return false;
    }
    
    // ASCII范围检查: 0x20-0x7E (95个字符)
    if (ascii_code < 0x20 || ascii_code > 0x7E) {
        ESP_LOGW(TAG, "ASCII code 0x%02X out of range", ascii_code);
        return false;
    }
    
    // 选择对应的字体加载器
    SDFontLoader* loader = (font_type == 0) ? &g_word_font_loader : &g_phonetic_font_loader;
    
    if (!loader->isInitialized()) {
        ESP_LOGE(TAG, "Font loader not initialized");
        return false;
    }
    
    // 使用ASCII码直接作为Unicode
    uint16_t unicode = (uint16_t)ascii_code;
    return loader->getCharGlyph(unicode, out_buffer);
}

/**
 * @brief 绘制英文音标 (使用16x16字体)
 */
/**
 * @brief 获取字体信息
 */
int getMultiFontSize(int font_type) {
    if (!g_initialized) {
        return -1;
    }
    
    if (font_type == 0) {
        return g_word_font_loader.getFontSize();
    } else {
        return g_phonetic_font_loader.getFontSize();
    }
}

/**
 * @brief 检查字体是否已初始化
 */
bool isMultiFontManagerInitialized(void) {
    return g_initialized;
}

// ===== 内部函数（需要全局display对象）=====
// 这些函数由 wordbook_interface.cpp 直接调用，不需要通过基类引用

/**
 * @brief 内部实现：绘制英文单词（假设全局display已定义）
 * 使用24x24字体
 */
void _drawEnglishWordInternal(int16_t x, int16_t y, const char* text, uint16_t color) {
    if (!g_initialized || !text) {
        ESP_LOGE(TAG, "_drawEnglishWordInternal: Not initialized or invalid text");
        return;
    }
    
    if (!g_word_font_loader.isInitialized()) {
        ESP_LOGE(TAG, "_drawEnglishWordInternal: Word font not initialized");
        return;
    }
    
    // 注意：这里不能直接访问 display 对象，因为它未包含在这个文件中
    // 所以这个函数需要被修改为只返回数据，让调用者处理
    // 为了避免这个问题，我们在 wordbook_interface.cpp 中直接调用原始函数
    // 但需要重新考虑设计...
    
    ESP_LOGW(TAG, "_drawEnglishWordInternal is not fully implemented");
}

/**
 * @brief 内部实现：绘制英文音标（假设全局display已定义）
 * 使用16x16字体
 */
void _drawEnglishPhoneticInternal(int16_t x, int16_t y, const char* text, uint16_t color) {
    if (!g_initialized || !text) {
        ESP_LOGE(TAG, "_drawEnglishPhoneticInternal: Not initialized or invalid text");
        return;
    }
    
    if (!g_phonetic_font_loader.isInitialized()) {
        ESP_LOGE(TAG, "_drawEnglishPhoneticInternal: Phonetic font not initialized");
        return;
    }
    
    ESP_LOGW(TAG, "_drawEnglishPhoneticInternal is not fully implemented");
}

/**
 * @file sd_font_loader.cpp
 * @brief SD卡字库加载器 - 从.bin文件读取中文字模
 */

#include "sd_font_loader.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "../../Grbl.h"
#include <FS.h>
static const char* TAG = "SDFontLoader";

SDFontLoader::SDFontLoader() 
    : font_file_(nullptr)
    , font_size_(16)
    , glyph_size_(32)
    , is_initialized_(false)
{
    memset(font_path_, 0, sizeof(font_path_));
}

SDFontLoader::~SDFontLoader() {
    if (font_file_) {
        fclose(font_file_);
        font_file_ = nullptr;
    }
}

bool SDFontLoader::init(const char* bin_path, int font_size) {
    if (is_initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }
    
    if (!bin_path) {
        ESP_LOGE(TAG, "Invalid bin path");
        return false;
    }
    
    strncpy(font_path_, bin_path, sizeof(font_path_) - 1);
    font_size_ = font_size;
    
    // 计算字模大小
    int bytes_per_row = (font_size + 7) / 8;
    glyph_size_ = bytes_per_row * font_size;
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        if (state == SDState::NotPresent) {
           ESP_LOGE(TAG,"No SD Card");
        } else {
            ESP_LOGE(TAG,"SD Card Busy");
        }
    }
    // 尝试打开文件
    font_file_ = fopen(font_path_, "rb");
    if (!font_file_) {
        ESP_LOGE(TAG, "Failed to open font file: %s", font_path_);
        return false;
    }
    
    // 检查文件大小
    fseek(font_file_, 0, SEEK_END);
    long file_size = ftell(font_file_);
    fseek(font_file_, 0, SEEK_SET);
    
    ESP_LOGI(TAG, "Font file opened successfully");
    ESP_LOGI(TAG, "  Path: %s", font_path_);
    ESP_LOGI(TAG, "  Font size: %dx%d", font_size, font_size);
    ESP_LOGI(TAG, "  Glyph size: %d bytes", glyph_size_);
    ESP_LOGI(TAG, "  File size: %ld bytes", file_size);
    
    is_initialized_ = true;
    return true;
}

bool SDFontLoader::getCharGlyph(uint16_t unicode, uint8_t* out_buffer) {
    if (!is_initialized_ || !out_buffer) {
        ESP_LOGE(TAG, "Not initialized or invalid buffer");
        return false;
    }
    
    uint32_t offset = 0;
    
    // 支持两种范围：ASCII (0x0020-0x007E) 和 GB2312 (0x4E00-0x9FA5)
    if (unicode >= 0x0020 && unicode <= 0x007E) {
        // ASCII字符：0x20=' ' 到 0x7E='~'，共95个字符
        // 偏移 = (ASCII码 - 0x20) * 字模大小
        offset = (unicode - 0x0020) * glyph_size_;
    } else if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
        // GB2312中文字符
        // 偏移 = (Unicode - 0x4E00) * 字模大小
        offset = (unicode - 0x4E00) * glyph_size_;
    } else {
        ESP_LOGW(TAG, "Unicode 0x%04X out of range (not ASCII or GB2312)", unicode);
        return false;
    }
    
    // 检查文件指针，如果为NULL则重新打开
    if (!font_file_) {
        ESP_LOGW(TAG, "Font file pointer is NULL for unicode 0x%04X, attempting to reopen: %s", unicode, font_path_);
        font_file_ = fopen(font_path_, "rb");
        if (!font_file_) {
            ESP_LOGE(TAG, "Failed to reopen font file: %s", font_path_);
            return false;
        }
        ESP_LOGI(TAG, "Font file reopened successfully for unicode 0x%04X", unicode);
    }
    
    // 定位文件
    if (fseek(font_file_, offset, SEEK_SET) != 0) {
        int err = ferror(font_file_);
        ESP_LOGE(TAG, "Failed to seek to offset %d for unicode 0x%04X, ferror=%d", offset, unicode, err);
        
        // 尝试重新打开文件后重试
        if (font_file_) {
            fclose(font_file_);
            font_file_ = nullptr;
        }
        font_file_ = fopen(font_path_, "rb");
        if (!font_file_) {
            ESP_LOGE(TAG, "Failed to reopen font file after seek error: %s", font_path_);
            return false;
        }
        
        ESP_LOGI(TAG, "Font file reopened, retrying seek to offset %d for unicode 0x%04X", offset, unicode);
        if (fseek(font_file_, offset, SEEK_SET) != 0) {
            int retry_err = ferror(font_file_);
            ESP_LOGE(TAG, "Seek still failed after reopening, offset=%d, unicode=0x%04X, ferror=%d", offset, unicode, retry_err);
            return false;
        }
    }
    
    // 读取字模数据
    size_t bytes_read = fread(out_buffer, 1, glyph_size_, font_file_);
    if (bytes_read != glyph_size_) {
        ESP_LOGE(TAG, "Failed to read glyph data for unicode 0x%04X, expected %d bytes, got %d", unicode, glyph_size_, bytes_read);
        
        // 如果读取失败，也尝试重新打开文件
        if (ferror(font_file_)) {
            ESP_LOGW(TAG, "File error detected, attempting to reopen");
            if (font_file_) {
                fclose(font_file_);
                font_file_ = nullptr;
            }
        }
        return false;
    }
    
    // 检查是否全为0 (字体可能不包含此字符)
    bool all_zero = true;
    for (int i = 0; i < glyph_size_; i++) {
        if (out_buffer[i] != 0) {
            all_zero = false;
            break;
        }
    }
    
    if (all_zero) {
        ESP_LOGD(TAG, "Warning: Glyph for unicode 0x%04X is all zeros", unicode);
    }
    
    return true;
}

uint16_t SDFontLoader::utf8ToUnicode(const char* utf8_str, int* bytes_consumed) {
    if (!utf8_str || !bytes_consumed) {
        return 0;
    }
    
    const uint8_t* p = (const uint8_t*)utf8_str;
    uint16_t unicode = 0;
    
    if ((*p & 0x80) == 0) {
        // 1字节 ASCII
        unicode = *p;
        *bytes_consumed = 1;
    } else if ((*p & 0xE0) == 0xC0) {
        // 2字节
        unicode = ((*p & 0x1F) << 6) | (*(p+1) & 0x3F);
        *bytes_consumed = 2;
    } else if ((*p & 0xF0) == 0xE0) {
        // 3字节 (中文通常在这里)
        unicode = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
        *bytes_consumed = 3;
    } else if ((*p & 0xF8) == 0xF0) {
        // 4字节
        unicode = ((*p & 0x07) << 18) | ((*(p+1) & 0x3F) << 12) | 
                 ((*(p+2) & 0x3F) << 6) | (*(p+3) & 0x3F);
        *bytes_consumed = 4;
    } else {
        // 无效UTF-8
        *bytes_consumed = 1;
    }
    
    return unicode;
}

int SDFontLoader::getTextGlyphs(const char* text, GlyphInfo* glyphs, int max_count) {
    if (!text || !glyphs || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    const char* p = text;
    
    while (*p && count < max_count) {
        int bytes_consumed = 0;
        uint16_t unicode = utf8ToUnicode(p, &bytes_consumed);
        
        if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
            // 是中文字符
            glyphs[count].unicode = unicode;
            glyphs[count].is_chinese = true;
            
            if (getCharGlyph(unicode, glyphs[count].glyph_data)) {
                count++;
            }
        }
        
        p += bytes_consumed;
    }
    
    return count;
}

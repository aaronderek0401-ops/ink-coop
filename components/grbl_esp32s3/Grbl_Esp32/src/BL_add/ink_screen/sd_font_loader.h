/**
 * @file sd_font_loader.h
 * @brief SD卡字库加载器 - 从.bin文件读取中文字模
 */

#ifndef SD_FONT_LOADER_H
#define SD_FONT_LOADER_H

#include <stdint.h>
#include <stdio.h>

// 字形信息结构
struct GlyphInfo {
    uint16_t unicode;
    bool is_chinese;
    uint8_t glyph_data[128]; // 最大支持32x32
};

class SDFontLoader {
public:
    SDFontLoader();
    ~SDFontLoader();
    
    /**
     * @brief 初始化字库加载器
     * @param bin_path .bin字库文件路径 (例如: "/sd/fangsong_gb2312_16x16.bin")
     * @param font_size 字体大小 (16, 24 或 32)
     * @return 初始化成功返回true
     */
    bool init(const char* bin_path, int font_size);
    
    /**
     * @brief 获取单个字符的字模数据
     * @param unicode Unicode编码
     * @param out_buffer 输出缓冲区 (至少需要 glyph_size_ 字节)
     * @return 成功返回true
     */
    bool getCharGlyph(uint16_t unicode, uint8_t* out_buffer);
    
    /**
     * @brief UTF-8转Unicode
     * @param utf8_str UTF-8字符起始位置
     * @param bytes_consumed 输出: 读取的字节数
     * @return Unicode编码
     */
    uint16_t utf8ToUnicode(const char* utf8_str, int* bytes_consumed);
    
    /**
     * @brief 批量获取文本的字模数据
     * @param text UTF-8编码的文本
     * @param glyphs 输出字形数组
     * @param max_count 最大字符数
     * @return 实际获取的字符数量
     */
    int getTextGlyphs(const char* text, GlyphInfo* glyphs, int max_count);
    
    int getFontSize() const { return font_size_; }
    int getGlyphSize() const { return glyph_size_; }
    bool isInitialized() const { return is_initialized_; }
    
private:
    FILE* font_file_;
    char font_path_[256];
    int font_size_;
    int glyph_size_;
    bool is_initialized_;
};

// ===== PSRAM 字体功能已移至 chinese_font_cache.h =====
// FullFontData 结构体和所有 PSRAM 字体函数已移至 chinese_font_cache.h/cpp

#endif // SD_FONT_LOADER_H
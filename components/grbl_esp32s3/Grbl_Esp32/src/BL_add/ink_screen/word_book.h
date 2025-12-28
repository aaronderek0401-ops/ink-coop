#pragma once

#include <Arduino.h>
#include <SD.h>
#include <string>

// 预加载缓存配置
#define WORDBOOK_PRELOAD_SIZE 20  // 预加载单词数量（可调整）

// 单词条目结构
struct WordEntry {
    String word;          // 单词
    String phonetic;      // 音标
    String definition;    // 英文释义
    String translation;   // 中文翻译
    String pos;           // 词性
};

// 单词本缓存管理器
class WordBookCache {
private:
    WordEntry* cache_;              // PSRAM缓存数组
    int cache_size_;                // 缓存大小
    int current_line_;              // 当前读取到的行号（SD卡中的位置）
    int current_index_;             // 当前显示的缓存索引
    int total_lines_;               // 总行数
    bool is_initialized_;           // 是否已初始化
    String csv_file_path_;          // CSV文件路径
    
    // 私有工具函数
    int countLines(File &file);
    void parseCSVLine(String line, WordEntry &entry);
    void assignField(int fieldCount, String &field, WordEntry &entry);
    String extractFirstNMeanings(const String& translation, int count);

public:
    WordBookCache();
    ~WordBookCache();
    
    // 初始化缓存系统
    bool init(const char* csv_path, int cache_size = WORDBOOK_PRELOAD_SIZE);
    
    // 从SD卡预加载单词到PSRAM
    bool preloadNextBatch();
    
    // 获取当前单词
    WordEntry* getCurrentWord();
    
    // 移动到下一个单词
    bool moveNext();
    
    // 重置到第一行
    void reset();
    
    // 获取缓存状态
    int getCurrentLine() const { return current_line_; }
    int getTotalLines() const { return total_lines_; }
    int getCacheSize() const { return cache_size_; }
    bool isInitialized() const { return is_initialized_; }
};

// 全局变量声明
extern WordEntry entry;
extern WordEntry sleep_mode_entry;
extern bool has_sleep_data;
extern WordBookCache g_wordbook_cache;  // 全局单词本缓存

// 便捷函数声明
bool initWordBookCache(const char* csv_path);  // 初始化单词本缓存
WordEntry* getNextWord();                       // 获取下一个单词（自动预加载）
void printWordsFromCache(int count);            // 从缓存读取并打印单词到串口

// 测试函数声明
template<typename T>
void testDisplayWordsOnScreen(T& display, int word_count);  // 在墨水屏上显示单词

// ============ 模板函数实现（必须在头文件中） ============
// 注意：需要先包含 GxEPD2.h 以获取颜色定义
#ifndef GxEPD_BLACK
    #define GxEPD_BLACK 0x0000
    #define GxEPD_WHITE 0xFFFF
#endif

// 函数声明（避免循环依赖）
template<typename T>
int16_t drawChineseTextWithCache(T& display, int16_t x, int16_t y, const char* text, uint16_t color);
template<typename T>
int16_t drawEnglishText(T& display, int16_t x, int16_t y, const char* text, uint16_t color);
bool switchToPSRAMFont(const char* font_name);

template<typename T>
void testDisplayWordsOnScreen(T& display, int word_count) {
    const char* TAG_TEST = "WordBookTest";
    ESP_LOGI(TAG_TEST, "========== 墨水屏单词显示测试 ==========");
    
    if (!g_wordbook_cache.isInitialized()) {
        ESP_LOGE(TAG_TEST, "❌ 单词本缓存未初始化");
        display.setFullWindow();
        display.firstPage();
        do {
            // 切换到中文字体显示错误信息
            if (switchToPSRAMFont("chinese_translate_font")) {
                drawChineseTextWithCache(display, 10, 10, "错误：单词本未初始化", (uint16_t)0x0000);
            }
        } while (display.nextPage());
        return;
    }
    
    ESP_LOGI(TAG_TEST, "开始显示 %d 个单词", word_count);
    
    // 清屏并开始显示
    display.setFullWindow();
    display.firstPage();
    
    do {
        display.fillScreen((uint16_t)0xFFFF);  // 白色背景
        
        int16_t y_position = 5;           // 起始Y坐标
        const int16_t x_left = 5;         // 左边距
        const int16_t word_line_height = 22;   // 单词行高
        const int16_t phonetic_line_height = 20; // 音标行高
        const int16_t trans_line_height = 22;   // 翻译行高
        const int16_t spacing = 3;        // 单词间距
        int displayed = 0;
        
        // 显示标题
        if (switchToPSRAMFont("english_word_font")) {
            drawEnglishText(display, x_left, y_position, "Word Book", (uint16_t)0x0000);
        }
        y_position += 35;
        
        // 显示单词 (采用竖向排列，每个单词占3-4行)
        for (int i = 0; i < word_count && y_position < 235; i++) {
            WordEntry* word = getNextWord();
            
            if (!word || word->word.length() == 0) {
                ESP_LOGW(TAG_TEST, "第 %d 个单词为空，跳过", i + 1);
                continue;
            }
            
            int16_t current_y = y_position;
            
            // 第1行：序号 + 单词 + 词性 (英文)
            if (switchToPSRAMFont("english_sentence_font")) {
                String word_line = String(i + 1) + ". " + word->word;
                
                // 添加词性 (如果有)
                if (word->pos.length() > 0) {
                    word_line += " [" + word->pos + "]";
                }
                
                drawEnglishText(display, x_left, current_y, word_line.c_str(), (uint16_t)0x0000);
            }
            current_y += word_line_height;
            
            // 第2行：音标 (英文，如果有)
            if (word->phonetic.length() > 0) {
                if (switchToPSRAMFont("english_phonetic_font")) {
                    ESP_LOGI(TAG_TEST, "显示音标: /%s/", word->phonetic.c_str());
                    String phonetic_line = "/" + word->phonetic + "/";
                    drawEnglishText(display, x_left + 15, current_y, phonetic_line.c_str(), (uint16_t)0x0000);
                }
                current_y += phonetic_line_height;
            }
            
            // 第3行：翻译 (中英混合，保留所有字符)
            if (word->translation.length() > 0) {
                // 获取第一行翻译（到第一个\n或到结尾）
                String trans = word->translation;
                int newline_pos = trans.indexOf('\n');
                if (newline_pos > 0) {
                    trans = trans.substring(0, newline_pos);
                }
                
                // 检查翻译中是否包含ASCII字符（需要用英文字体）
                bool has_ascii = false;
                bool has_chinese = false;
                for (int j = 0; j < trans.length(); j++) {
                    if ((uint8_t)trans[j] < 128) {
                        has_ascii = true;
                    } else {
                        has_chinese = true;
                    }
                }
                
                // 根据内容选择合适的显示方式
                int16_t x_offset = x_left + 15;
                
                if (has_chinese) {
                    // 包含中文，需要分段处理
                    String current_segment = "";
                    bool in_ascii = false;
                    
                    for (int j = 0; j < trans.length() && x_offset < 410; j++) {
                        char c = trans[j];
                        bool is_ascii = ((uint8_t)c < 128);
                        
                        if (is_ascii != in_ascii && current_segment.length() > 0) {
                            // 切换字符类型，输出当前段
                            if (in_ascii) {
                                if (switchToPSRAMFont("english_sentence_font")) {
                                    x_offset = drawEnglishText(display, x_offset, current_y, current_segment.c_str(), (uint16_t)0x0000);
                                }
                            } else {
                                if (switchToPSRAMFont("chinese_translate_font")) {
                                    x_offset = drawChineseTextWithCache(display, x_offset, current_y, current_segment.c_str(), (uint16_t)0x0000);
                                }
                            }
                            current_segment = "";
                        }
                        
                        current_segment += c;
                        in_ascii = is_ascii;
                    }
                    
                    // 输出最后一段
                    if (current_segment.length() > 0 && x_offset < 410) {
                        if (in_ascii) {
                            if (switchToPSRAMFont("english_sentence_font")) {
                                drawEnglishText(display, x_offset, current_y, current_segment.c_str(), (uint16_t)0x0000);
                            }
                        } else {
                            if (switchToPSRAMFont("chinese_translate_font")) {
                                drawChineseTextWithCache(display, x_offset, current_y, current_segment.c_str(), (uint16_t)0x0000);
                            }
                        }
                    }
                } else if (has_ascii) {
                    // 仅包含ASCII，使用英文字体
                    if (switchToPSRAMFont("english_sentence_font")) {
                        drawEnglishText(display, x_offset, current_y, trans.c_str(), (uint16_t)0x0000);
                    }
                }
                
                current_y += trans_line_height;
            }
            
            // 添加单词间距
            y_position = current_y + spacing;
            displayed++;
            
            ESP_LOGD(TAG_TEST, "显示单词 %d: %s [%s] /%s/ - %s", 
                     i + 1, word->word.c_str(), word->pos.c_str(), 
                     word->phonetic.c_str(), word->translation.c_str());
        }
        
        ESP_LOGI(TAG_TEST, "✅ 成功显示 %d 个单词", displayed);
        
    } while (display.nextPage());
    
    ESP_LOGI(TAG_TEST, "========== 显示完成 ==========");
}



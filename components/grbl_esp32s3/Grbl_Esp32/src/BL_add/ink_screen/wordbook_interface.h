// #pragma once
// #include "ink_screen.h"
// void enterVocabularyScreen();
// void displayVocabularyScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
// String convertChinesePunctuationsInString(const String& input);
// uint16_t calculateTextWidth(const char* text, uint8_t font_size);
// uint16_t display_centered_line(uint16_t start_x, uint16_t y, const char* text, 
//                               uint8_t fontSize, uint16_t color, 
//                               uint16_t max_width, uint16_t line_height);
// uint16_t displayWrappedText(uint16_t x, uint16_t y, const char* text, uint8_t fontSize, 
//                            uint16_t color, uint16_t max_width, uint16_t line_height);
// String formatPhonetic(const String& phonetic);
// void updateDisplayWithString(int16_t x, int16_t y, uint8_t *text, uint8_t size, uint16_t color);
// void showPromptInfor(uint8_t *tempPrompt, bool isAllRefresh);

// // ===== 英文字体管理函数 (支持单词+音标) =====
// /**
//  * @brief 初始化英文字体系统
//  * @details 加载单词字体(24x24)和音标字体(16x16)
//  * @return true 初始化成功, false 失败
//  */
// bool initEnglishFontSystem();

// /**
//  * @brief 显示单个英文单词条目 (单词 + 音标)
//  * @param word 英文单词
//  * @param phonetic 音标 (可选，为NULL时只显示单词)
//  * @param x X坐标
//  * @param y Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  * @return 下一行的Y坐标
//  */
// int16_t displayEnglishWord(const char* word, const char* phonetic, 
//                            int16_t x, int16_t y, uint16_t color);

// /**
//  * @brief 显示多个英文单词 (词库列表)
//  * @param words 单词数组
//  * @param phonetics 音标数组 (可选)
//  * @param word_count 单词数量
//  * @param start_x 起始X坐标
//  * @param start_y 起始Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  */
// void displayEnglishWordList(const char** words, const char** phonetics, int word_count,
//                             int16_t start_x, int16_t start_y, uint16_t color);

// /**
//  * @brief 显示单个英文单词 (仅单词，无音标)
//  * @param word 英文单词
//  * @param x X坐标
//  * @param y Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  * @return 成功返回true
//  */
// bool displaySimpleWord(const char* word, int16_t x, int16_t y, uint16_t color);

// /**
//  * @brief 显示英文音标 (仅音标)
//  * @param phonetic 音标
//  * @param x X坐标
//  * @param y Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  * @return 成功返回true
//  */
// bool displaySimplePhonetic(const char* phonetic, int16_t x, int16_t y, uint16_t color);
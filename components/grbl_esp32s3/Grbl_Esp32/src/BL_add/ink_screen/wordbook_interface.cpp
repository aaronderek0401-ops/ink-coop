// #include "wordbook_interface.h"
// #include "word_book.h"
// #include "ink_screen.h"
// #include <GxEPD2_BW.h>
// #include "./Pic.h"
// #include "multi_font_manager.h"    // 多字体管理器 (英文单词+音标)
// #include "sd_font_loader.h"        // 直接使用字体加载器

// // 前向声明 ink_screen.cpp 中的全局变量
// extern uint8_t interfaceIndex;
// extern GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display;

// // ===== 本地包装函数：直接绘制到全局display对象 =====
// /**
//  * @brief 本地函数：绘制英文单词到全局display
//  */
// static void _drawWordToDisplay(int16_t x, int16_t y, const char* text, uint16_t color) {
//     if (!isMultiFontManagerInitialized() || !text) {
//         return;
//     }
    
//     int font_size = 24;  // 单词字体大小
//     int char_width = 14;  // 24x24字体的有效字符宽度约14-16像素，这里使用14来紧凑显示
    
//     // 获取字体数据（注意：这需要访问multi_font_manager的内部数据）
//     // 目前的实现方式是调用getEnglishCharGlyph获取字模
//     uint8_t glyph_buffer[128];
//     int16_t curr_x = x;
    
//     for (const char* p = text; *p; ++p) {
//         uint8_t ascii_code = (uint8_t)*p;
        
//         // 获取字模数据
//         if (!getEnglishCharGlyph(0, ascii_code, glyph_buffer)) {  // 0 = word font
//             continue;
//         }
        
//         // 逐像素绘制字模
//         int glyph_size = (font_size + 7) / 8 * font_size;
//         for (int row = 0; row < font_size; ++row) {
//             for (int col = 0; col < font_size; ++col) {
//                 int byte_idx = row * ((font_size + 7) / 8) + col / 8;
//                 int bit_idx = 7 - (col % 8);
                
//                 if (byte_idx < glyph_size && byte_idx < sizeof(glyph_buffer) && 
//                     (glyph_buffer[byte_idx] >> bit_idx) & 1) {
//                     display.drawPixel(curr_x + col, y + row, color);
//                 }
//             }
//         }
        
//         curr_x += char_width;  // 移动到下一个字符位置（使用更紧凑的间距）
//     }
// }

// /**
//  * @brief 本地函数：绘制英文音标到全局display
//  */
// static void _drawPhoneticToDisplay(int16_t x, int16_t y, const char* text, uint16_t color) {
//     if (!isMultiFontManagerInitialized() || !text) {
//         return;
//     }
    
//     int font_size = 16;  // 音标字体大小
//     int char_width = 10;  // 16x16字体的有效字符宽度约10-12像素，这里使用10来紧凑显示
    
//     uint8_t glyph_buffer[128];
//     int16_t curr_x = x;
    
//     for (const char* p = text; *p; ++p) {
//         uint8_t ascii_code = (uint8_t)*p;
        
//         // 获取字模数据
//         if (!getEnglishCharGlyph(1, ascii_code, glyph_buffer)) {  // 1 = phonetic font
//             continue;
//         }
        
//         // 逐像素绘制字模
//         int glyph_size = (font_size + 7) / 8 * font_size;
//         for (int row = 0; row < font_size; ++row) {
//             for (int col = 0; col < font_size; ++col) {
//                 int byte_idx = row * ((font_size + 7) / 8) + col / 8;
//                 int bit_idx = 7 - (col % 8);
                
//                 if (byte_idx < glyph_size && byte_idx < sizeof(glyph_buffer) && 
//                     (glyph_buffer[byte_idx] >> bit_idx) & 1) {
//                     display.drawPixel(curr_x + col, y + row, color);
//                 }
//             }
//         }
        
//         curr_x += char_width;  // 移动到下一个字符位置（使用更紧凑的间距）
//     }
// }

// static const char* TAG = "WordbookInterface";
// // 检查是否是中文标点符号
// bool isChinesePunctuation(const char* str) {
//     if (strlen(str) < 3) return false;
    
//     // 常见中文标点符号的UTF-8编码
//     if (str[0] == 0xEF && str[1] == 0xBC) {
//         // EF BC 开头的标点
//         if (str[2] == 0x8C ||  // ，
//             str[2] == 0x9B ||  // ；
//             str[2] == 0x9A ||  // ：
//             str[2] == 0x9F ||  // ？
//             str[2] == 0x81 ||  // ！
//             str[2] == 0x88 ||  // （
//             str[2] == 0x89) {  // ）
//             return true;
//         }
//     } else if (str[0] == 0xE3 && str[1] == 0x80) {
//         // E3 80 开头的标点
//         if (str[2] == 0x82 ||  // 、
//             str[2] == 0x81) {  // 。
//             return true;
//         }
//     }
//     return false;
// }

// // 将中文标点转换为英文标点
// char convertChinesePunctuation(const char* chinese_punct) {
//     if (strlen(chinese_punct) < 3) return '\0';
    
//     if (chinese_punct[0] == 0xEF && chinese_punct[1] == 0xBC) {
//         switch (chinese_punct[2]) {
//             case 0x8C: return ',';  // ， -> ,
//             case 0x9B: return ';';  // ； -> ;
//             case 0x9A: return ':';  // ： -> :
//             case 0x9F: return '?';  // ？ -> ?
//             case 0x81: return '!';  // ！ -> !
//             case 0x88: return '(';  // （ -> (
//             case 0x89: return ')';  // ） -> )
//         }
//     } else if (chinese_punct[0] == 0xE3 && chinese_punct[1] == 0x80) {
//         switch (chinese_punct[2]) {
//             case 0x82: return '/';  // 、 -> /
//             case 0x81: return '.';  // 。 -> .
//         }
//     }
    
//     return '\0';
// }

// // 辅助函数：估算文本显示宽度
// uint16_t calculateTextWidth(const char* text, uint8_t font_size) {
//     if (text == NULL) return 0;
    
//     uint16_t width = 0;
//     for (int i = 0; text[i] != '\0'; ) {
//         if ((text[i] & 0xE0) == 0xE0) {
//             // 中文字符
//             width += font_size;
//             i += 3;
//         } else if (isprint(text[i])) {
//             // 英文字符
//             width += font_size / 2;
//             i++;
//         } else {
//             i++;
//         }
//     }
//     return width;
// }

// String formatPhonetic(const String& phonetic) {
//     if (phonetic.length() == 0) {
//         return phonetic;
//     }
    
//     String result = phonetic;
    
//     // 移除可能已经存在的方括号
//     if (result.startsWith("[")) {
//         result = result.substring(1);
//     }
//     if (result.endsWith("]")) {
//         result = result.substring(0, result.length() - 1);
//     }
    
//     // 添加新的方括号
//     return "[" + result + "]";
// }

// /**
//  * @brief 初始化多字体管理器
//  * @details 加载英文单词字体 (24x24) 和音标字体 (16x16)
//  * @return true 初始化成功, false 失败
//  */
// bool initEnglishFontSystem() {
//     ESP_LOGI(TAG, "正在初始化英文字体系统...");
    
//     // 初始化多字体管理器
//     bool success = initMultiFontManager(
//         "/sd/comic_sans_ms_bold_24x24.bin",  // 单词字体 (24x24)
//         "/sd/comic_sans_ms_bold_16x16.bin"   // 音标字体 (16x16)
//     );
    
//     if (success) {
//         ESP_LOGI(TAG, "✅ 英文字体系统初始化成功");
//         return true;
//     } else {
//         ESP_LOGE(TAG, "❌ 英文字体系统初始化失败，请检查SD卡文件");
//         return false;
//     }
// }

// /**
//  * @brief 显示单个英文单词条目 (单词 + 音标)
//  * @param word 英文单词
//  * @param phonetic 音标 (可选，为NULL时只显示单词)
//  * @param x X坐标
//  * @param y Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  * @return 下一行的Y坐标
//  * 
//  * 使用示例:
//  *   int16_t next_y = displayEnglishWord("Hello", "/həˈloʊ/", 10, 50, 0);
//  */
// int16_t displayEnglishWord(const char* word, const char* phonetic, 
//                            int16_t x, int16_t y, uint16_t color) {
//     if (!isMultiFontManagerInitialized()) {
//         ESP_LOGE(TAG, "英文字体未初始化，请先调用 initEnglishFontSystem()");
//         return y;
//     }
    
//     if (!word) {
//         ESP_LOGE(TAG, "单词为空");
//         return y;
//     }
    
//     ESP_LOGI(TAG, "显示单词: %s (音标: %s)", word, phonetic ? phonetic : "无");
    
//     // 检查是否初始化
//     if (!isMultiFontManagerInitialized()) {
//         ESP_LOGE(TAG, "英文字体未初始化");
//         return y;
//     }
    
//     // 绘制单词 (24x24)
//     _drawWordToDisplay(x, y, word, color);
    
//     // 绘制音标 (16x16, 如果提供)
//     int16_t phonetic_y = y + 28;  // 单词下方28像素处
//     if (phonetic) {
//         _drawPhoneticToDisplay(x, phonetic_y, phonetic, color);
//         phonetic_y += 20;
//     }
    
//     // 更新屏幕显示
//     display.display(true);  // partial update
    
//     return phonetic_y;
// }

// /**
//  * @brief 显示多个英文单词 (词库列表)
//  * @param words 单词数组
//  * @param phonetics 音标数组 (可选)
//  * @param word_count 单词数量
//  * @param start_x 起始X坐标
//  * @param start_y 起始Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  * 
//  * 使用示例:
//  *   const char* words[] = {"Hello", "World", "Apple"};
//  *   const char* phonetics[] = {"/həˈloʊ/", "/wɜːrld/", "/ˈæpl/"};
//  *   displayEnglishWordList(words, phonetics, 3, 10, 50, 0);
//  */
// void displayEnglishWordList(const char** words, const char** phonetics, int word_count,
//                             int16_t start_x, int16_t start_y, uint16_t color) {
//     if (!isMultiFontManagerInitialized()) {
//         ESP_LOGE(TAG, "英文字体未初始化，请先调用 initEnglishFontSystem()");
//         return;
//     }
    
//     if (!words || word_count <= 0) {
//         ESP_LOGE(TAG, "单词数组为空或数量无效");
//         return;
//     }
    
//     ESP_LOGI(TAG, "显示单词列表，共%d个单词", word_count);
    
//     // 清屏
//     display.firstPage();
//     do {
//         display.fillScreen(GxEPD_WHITE);
        
//         int16_t current_y = start_y;
        
//         // 显示每个单词
//         for (int i = 0; i < word_count; i++) {
//             if (current_y > display.height() - 50) {
//                 ESP_LOGW(TAG, "屏幕空间不足，无法显示所有单词");
//                 break;
//             }
            
//             const char* phonetic = (phonetics && phonetics[i]) ? phonetics[i] : NULL;
            
//             // 绘制单词
//             _drawWordToDisplay(start_x, current_y, words[i], color);
//             current_y += 28;
            
//             // 绘制音标
//             if (phonetic) {
//                 _drawPhoneticToDisplay(start_x, current_y, phonetic, color);
//                 current_y += 20;
//             }
            
//             current_y += 10;  // 行间距
//         }
        
//     } while (display.nextPage());
    
//     ESP_LOGI(TAG, "✅ 单词列表显示完成");
// }

// /**
//  * @brief 显示单个英文单词 (仅单词，无音标)
//  * @param word 英文单词
//  * @param x X坐标
//  * @param y Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  * @return 成功返回true
//  * 
//  * 使用示例:
//  *   displaySimpleWord("Hello", 10, 50, 0);
//  */
// bool displaySimpleWord(const char* word, int16_t x, int16_t y, uint16_t color) {
//     if (!isMultiFontManagerInitialized()) {
//         ESP_LOGE(TAG, "英文字体未初始化，请先调用 initEnglishFontSystem()");
//         return false;
//     }
    
//     if (!word) {
//         ESP_LOGE(TAG, "单词为空");
//         return false;
//     }
    
//     ESP_LOGI(TAG, "显示单词: %s (24x24字体)", word);
    
//     _drawWordToDisplay(x, y, word, color);
    
//     bool success = true;
    
//     if (success) {
//         display.display(true);  // partial update
//     }
    
//     return success;
// }

// /**
//  * @brief 显示英文音标 (仅音标)
//  * @param phonetic 音标
//  * @param x X坐标
//  * @param y Y坐标
//  * @param color 颜色 (0=黑, 1=白)
//  * @return 成功返回true
//  * 
//  * 使用示例:
//  *   displaySimplePhonetic("/həˈloʊ/", 10, 80, 0);
//  */
// bool displaySimplePhonetic(const char* phonetic, int16_t x, int16_t y, uint16_t color) {
//     if (!isMultiFontManagerInitialized()) {
//         ESP_LOGE(TAG, "英文字体未初始化，请先调用 initEnglishFontSystem()");
//         return false;
//     }
    
//     if (!phonetic) {
//         ESP_LOGE(TAG, "音标为空");
//         return false;
//     }
    
//     ESP_LOGI(TAG, "显示音标: %s (16x16字体)", phonetic);
    
//     _drawPhoneticToDisplay(x, y, phonetic, color);
    
//     bool success = true;
    
//     if (success) {
//         display.display(true);  // partial update
//     }
    
//     return success;
// }

// // 预处理函数：将字符串中的中文标点全部转换为英文标点
// String convertChinesePunctuationsInString(const String& input) {
//     String result = "";
    
//     for (int i = 0; i < input.length(); ) {
//         if (isChinesePunctuation(input.c_str() + i)) {
//             char english_punct = convertChinesePunctuation(input.c_str() + i);
//             if (english_punct != '\0') {
//                 result += english_punct;
//             }
//             i += 3; // 跳过3字节UTF-8编码
//         } else {
//             result += input[i];
//             i++;
//         }
//     }
    
//     return result;
// }

// // 居中显示单行（自动换行）
// uint16_t display_centered_line(uint16_t start_x, uint16_t y, const char* text, 
//                               uint8_t fontSize, uint16_t color, 
//                               uint16_t max_width, uint16_t line_height) {
//     uint16_t current_x = start_x;
//     uint16_t current_y = y;
//     uint16_t english_width = fontSize / 2;
//     uint16_t chinese_width = fontSize;
    
//     for (int i = 0; text[i] != '\0'; ) {
//         if ((text[i] & 0x80) == 0) {
//             // ASCII字符
//             if (current_x + english_width > start_x + max_width) {
//                 current_x = start_x;
//                 current_y += line_height;
//             }
            
//             // 使用 GXEPD2 绘制字符 - 这里简化为使用 drawPixel 逐像素绘制或跳过
//             // display.drawPixel(current_x, current_y, color);
//             current_x += english_width;
//             i++;
            
//         } else if ((text[i] & 0xE0) == 0xE0) {
//             // 中文字符
//             if (current_x + chinese_width > start_x + max_width) {
//                 current_x = start_x;
//                 current_y += line_height;
//             }
            
//             if (text[i+1] != '\0' && text[i+2] != '\0') {
//                 uint8_t chinese_char[4] = {text[i], text[i+1], text[i+2], '\0'};
//                 // display.drawBitmap(中文显示);
//                 current_x += chinese_width;
//                 i += 3;
//             } else {
//                 i++;
//             }
//         } else {
//             i++;
//         }
//     }
    
//     return current_y + line_height;
// }
// void updateDisplayWithString(int16_t x, int16_t y, uint8_t *text, uint8_t size, uint16_t color) {
//     if (text == NULL || strlen((char*)text) == 0) {
//         ESP_LOGE(TAG, "显示文本为空");
//         return;
//     }
    
//     // 边界检查：确保坐标在屏幕范围内
//     if (x < 0 || y < 0 || x > 416 || y > 416) {
//         ESP_LOGW(TAG, "坐标超出范围: (%d, %d), 修正处理", x, y);
//         if (x < 0) x = 0;
//         if (y < 0) y = 0;
//         if (x > 416) x = 416;
//         if (y > 416) y = 416;
//     }
    
//     // 判断文本类型并调用相应的显示函数
//     if (isChineseText(text)) {
//         // 中文文本使用 showChineseString
//         ESP_LOGI(TAG, "显示中文文本: %s (字体大小: %d)", text, size);
//         showChineseString(x, y, text, size, color);
//         display.display(true);  // 部分刷新
//     } else {
//         ESP_LOGI(TAG, "显示英文文本: %s (位置: %d,%d, 字体大小: %d)", text, x, y, size);
        
//         // 检查字体是否初始化
//         if (!isMultiFontManagerInitialized()) {
//             // 如果多字体管理器未初始化，使用 GXEPD2 内置字体
//             ESP_LOGW(TAG, "多字体管理器未初始化，使用GXEPD2内置字体");
//             display.setCursor(x, y);
//             display.setTextColor(color);
            
//             // 计算字体缩放因子（GXEPD2 的 setTextSize 使用倍数）
//             uint8_t text_scale = size / 8;
//             if (text_scale < 1) text_scale = 1;
//             if (text_scale > 4) text_scale = 4;
            
//             display.setTextSize(text_scale);
//             display.print((char*)text);
//             display.display(true);  // 部分刷新
//         } else {
//             // 使用多字体管理器显示英文单词
//             displaySimpleWord((char*)text, x, y, color);
//         }
//     }
    
//     // 记录最后显示的文本信息
//     size_t textLen = strlen((char*)text);
//     size_t copyLen = (textLen < sizeof(lastDisplayedText)) ? textLen : (sizeof(lastDisplayedText) - 1);
//     memcpy(lastDisplayedText, text, copyLen);
//     lastDisplayedText[copyLen] = '\0';
//     lastTextLength = copyLen;
    
//     lastX = x;
//     lastY = y;
//     lastSize = size;
    
//     ESP_LOGI(TAG, "✅ 文本已显示: 长度=%d, 内容=%s", lastTextLength, lastDisplayedText);
// }

// uint16_t displayEnglishWrapped(uint16_t x, uint16_t y, const char* text, uint8_t fontSize, 
//                               uint16_t color, uint16_t max_width, uint16_t line_height) {
//     if (text == nullptr || strlen(text) == 0) {
//         return y;
//     }
    
//     uint16_t current_x = x;
//     uint16_t current_y = y;
//     uint16_t char_width = fontSize / 2;
    
//     ESP_LOGI("WRAP", "开始换行显示英语句子，最大宽度: %d", max_width);
    
//     for (int i = 0; text[i] != '\0'; i++) {
//         // 检查换行符
//         if (text[i] == '\\' && text[i+1] == 'n') {
//             // 遇到 \n，强制换行
//             current_x = x;
//             current_y += line_height;
//             i++; // 跳过 n，下一次循环会跳过 
//             ESP_LOGD("WRAP", "英语句子 \\n 强制换行到 Y: %d", current_y);
//             continue;
//         }
//         else if (text[i] == '\n') {
//             // 遇到真正的换行符
//             current_x = x;
//             current_y += line_height;
//             ESP_LOGD("WRAP", "英语句子换行符强制换行到 Y: %d", current_y);
//             continue;
//         }
//         else if (text[i] == '\r') {
//             // 跳过回车符
//             continue;
//         }
        
//         // 检查当前字符是否会超出边界
//         if (current_x + char_width > x + max_width) {
//             // 自动换行
//             current_x = x;
//             current_y += line_height;
//             ESP_LOGD("WRAP", "英语句子自动换行到 Y: %d", current_y);
//         }
        
//         // 显示当前字符 - 使用 GXEPD2
//         display.drawChar(current_x, current_y, text[i], color, WHITE, 1);
//         current_x += char_width;
        
//         ESP_LOGD("WRAP", "显示字符 '%c' 在 (%d,%d)", text[i], current_x - char_width, current_y);
//     }
    
//     // 返回下一行的起始Y坐标
//     return current_y + line_height;
// }

// /**
//  * @brief 根据矩形配置渲染单词相关内容
//  * @param rect 矩形配置信息
//  * @param entry 单词数据
//  * @param global_scale 全局缩放比例
//  */
// void renderRectContent(RectInfo* rect, WordEntry& entry, float global_scale) {
//     if (!rect) {
//         ESP_LOGE("RENDER", "错误：rect参数为空指针！");
//         return;
//     }
    
//     // 计算缩放后的矩形位置和尺寸
//     int display_x = (int)(rect->x * global_scale + 0.5f);
//     int display_y = (int)(rect->y * global_scale + 0.5f);
//     int display_width = (int)(rect->width * global_scale + 0.5f);
//     int display_height = (int)(rect->height * global_scale + 0.5f);
    
//     ESP_LOGI("RENDER", "矩形(%d,%d) %dx%d, 内容类型:%d, 文本数:%d", 
//             display_x, display_y, display_width, display_height, rect->content_type, rect->text_count);
    
//     // ========== 新逻辑：如果矩形有定义文本内容列表，使用相对坐标渲染 ==========
//     if (rect->text_count > 0) {
//         ESP_LOGI("RENDER", "使用新文本数组渲染，共%d个文本", rect->text_count);
        
//         for (int i = 0; i < rect->text_count; i++) {
//             TextPositionInRect* text = &rect->texts[i];
            
//             // 计算文本在缩放后的矩形中的绝对位置
//             // rel_x, rel_y 是相对于原始矩形的，所以需要基于缩放后的矩形计算
//             int text_x = display_x + (int)(text->rel_x * display_width);
//             int text_y = display_y + (int)(text->rel_y * display_height);
            
//             // 计算可用宽度和高度（缩放后）
//             int available_width = text->max_width > 0 ? text->max_width : (display_width - (int)(text->rel_x * display_width));
//             int available_height = text->max_height > 0 ? text->max_height : (display_height - (int)(text->rel_y * display_height));
            
//             ESP_LOGI("RENDER", "  文本%d: 类型%d, 位置(%d,%d), 可用空间%dx%d, rel_x=%.3f, rel_y=%.3f", 
//                     i, text->type, text_x, text_y, available_width, available_height, text->rel_x, text->rel_y);
            
//             // 枚举值调试
//             ESP_LOGI("RENDER", "  CONTENT_WORD=%d, PHONETIC=%d, DEFINITION=%d, TRANSLATION=%d", 
//                     CONTENT_WORD, CONTENT_PHONETIC, CONTENT_DEFINITION, CONTENT_TRANSLATION);
            
//             // 根据内容类型获取要显示的文本
//             String display_text = "";
//             switch (text->type) {
//                 case CONTENT_WORD:
//                     display_text = entry.word.length() > 0 ? entry.word : "No Word";
//                     ESP_LOGI("RENDER", "  CONTENT_WORD: word='%s'", entry.word.c_str());
//                     break;
//                 case CONTENT_PHONETIC:
//                     display_text = entry.phonetic.length() > 0 ? formatPhonetic(entry.phonetic) : "";
//                     ESP_LOGI("RENDER", "  CONTENT_PHONETIC: phonetic='%s'", entry.phonetic.c_str());
//                     break;
//                 case CONTENT_DEFINITION:
//                     display_text = entry.definition.length() > 0 ? entry.definition : "";
//                     ESP_LOGI("RENDER", "  CONTENT_DEFINITION: definition='%s'", entry.definition.c_str());
//                     break;
//                 case CONTENT_TRANSLATION:
//                     display_text = entry.translation.length() > 0 ? entry.translation : "";
//                     ESP_LOGI("RENDER", "  CONTENT_TRANSLATION: translation='%s'", 
//                             entry.translation.c_str());
//                     break;
//                 default:
//                     ESP_LOGI("RENDER", "  未知文本类型: %d", text->type);
//                     continue;
//             }
            
//             if (display_text.length() == 0) {
//                 ESP_LOGI("RENDER", "  文本内容为空，跳过显示");
//                 continue;
//             }
            
//             // 根据内容类型选择字体大小
//             // CONTENT_WORD (3) = 24x24
//             // CONTENT_PHONETIC (4) = 16x16
//             // 其他 = 从配置读取或默认 16
//             uint8_t font_size = 16;  // 默认值
//             if (text->type == CONTENT_WORD) {
//                 font_size = 24;  // 单词使用 24x24 字体
//             } else if (text->type == CONTENT_PHONETIC) {
//                 font_size = 16;  // 音标使用 16x16 字体
//             } else if (text->font_size > 0) {
//                 font_size = text->font_size;  // 其他情况使用配置值
//             }
//             ESP_LOGI("RENDER", "  处理文本: 类型%d, 字体%d", text->type, font_size);
            
//             // 计算对齐位置
//             int final_x = text_x;
//             int final_y = text_y;
            
//             uint16_t text_width = calculateTextWidth(display_text.c_str(), font_size);
            
//             ESP_LOGI("RENDER", "  对齐前: final_x=%d, text_width=%d", final_x, text_width);
            
//             if (text->h_align == ALIGN_CENTER) {
//                 final_x = text_x - text_width / 2;
//                 if (final_x < 0) final_x = 0;  // 边界检查
//             } else if (text->h_align == ALIGN_RIGHT) {
//                 final_x = text_x - text_width;
//                 if (final_x < 0) final_x = 0;  // 边界检查
//             }
//             // ALIGN_LEFT: final_x = text_x (no change)
            
//             // 确保坐标在合理范围内
//             if (final_x > 1000) final_x = text_x;  // 检测溢出
            
//             if (text->v_align == ALIGN_MIDDLE) {
//                 final_y = text_y - font_size / 2;
//             } else if (text->v_align == ALIGN_BOTTOM) {
//                 final_y = text_y - font_size;
//             }
            
//             ESP_LOGI("RENDER", "  对齐后: final_x=%d, final_y=%d", final_x, final_y);
//             // ALIGN_TOP: final_y = text_y (no change)
            
//             // 根据内容类型选择合适的显示方法
//             if (text->type == CONTENT_TRANSLATION) {
//                 // 中文翻译使用换行显示
//                 String processedTranslation = convertChinesePunctuationsInString(display_text);
//                 displayWrappedText(final_x, final_y, 
//                                  processedTranslation.c_str(), 
//                                  font_size, BLACK, 
//                                  available_width, font_size + 4);
//                 ESP_LOGI("RENDER", "  中文翻译显示: '%s', 位置(%d,%d), 字体%d", 
//                         processedTranslation.c_str(), final_x, final_y, font_size);
//             } else if (text->type == CONTENT_DEFINITION) {
//                 // 英文释义使用换行显示
//                 displayEnglishWrapped(final_x, final_y, 
//                                     display_text.c_str(), 
//                                     font_size, BLACK, 
//                                     available_width, font_size + 4);
//                 ESP_LOGI("RENDER", "  英文释义显示: '%s', 位置(%d,%d), 字体%d", 
//                         display_text.c_str(), final_x, final_y, font_size);
//             } else {
//                 // 单词和音标使用单行显示
//                 updateDisplayWithString(final_x, final_y, 
//                                       (uint8_t*)display_text.c_str(), 
//                                       font_size, BLACK);
//                 ESP_LOGI("RENDER", "  文本显示: '%s', 位置(%d,%d), 字体%d", 
//                         display_text.c_str(), final_x, final_y, font_size);
//             }
//         }
        
//         return; // 使用新逻辑后直接返回
//     }
    
//     // ========== 检查自定义文本模式 ==========
//     if (rect->custom_text_mode) {
//         ESP_LOGI("RENDER", "矩形处于自定义文本模式，且无自定义文本，跳过默认内容显示");
//         return; // 自定义模式下如果没有文本就不显示任何默认内容
//     }
    
//     // ========== 旧逻辑（兼容）：基于 rect->content_type 的单一内容渲染 ==========
    
//     // 计算内容区域（减去padding）
//     int content_x = display_x + rect->padding_left;
//     int content_y = display_y + rect->padding_top;
//     int content_width = display_width - rect->padding_left - rect->padding_right;
//     int content_height = display_height - rect->padding_top - rect->padding_bottom;
    
//     // 根据内容类型渲染
//     switch (rect->content_type) {
//         case CONTENT_NONE:
//             // 仅显示背景图标（如果有）
//             break;
            
//         case CONTENT_STATUS_BAR:
//             // 状态栏（显示提示信息）
//             if (showPrompt != nullptr) {
//             //    showSimplePromptWithNail(showPrompt);
//             }
//             break;
            
//         case CONTENT_ICON_ONLY:
//             // 仅显示图标，不处理文本
//             break;
            
//         case CONTENT_WORD: {
//             // 显示英文单词
//             String word_to_display = entry.word;
//             if (word_to_display.length() == 0) {
//                 word_to_display = "No Word";
//             }
            
//             // 尝试不同字体大小
//             bool word_displayed = false;
//             uint8_t font_sizes[] = {rect->font_size, 20, 16, 14, 12};
            
//             for (int i = 0; i < sizeof(font_sizes)/sizeof(font_sizes[0]) && !word_displayed; i++) {
//                 uint8_t font_size = font_sizes[i];
//                 uint16_t word_width = calculateTextWidth(word_to_display.c_str(), font_size);
                
//                 if (word_width <= content_width) {
//                     // 计算对齐位置
//                     int text_x = content_x;
//                     if (rect->h_align == ALIGN_CENTER) {
//                         text_x = content_x + (content_width - word_width) / 2;
//                     } else if (rect->h_align == ALIGN_RIGHT) {
//                         text_x = content_x + content_width - word_width;
//                     }
                    
//                     int text_y = content_y;
//                     if (rect->v_align == ALIGN_MIDDLE) {
//                         text_y = content_y + (content_height - font_size) / 2;
//                     } else if (rect->v_align == ALIGN_BOTTOM) {
//                         text_y = content_y + content_height - font_size;
//                     }
                    
//                     updateDisplayWithString(text_x, text_y, 
//                                           (uint8_t*)word_to_display.c_str(), 
//                                           font_size, BLACK);
//                     word_displayed = true;
//                     ESP_LOGI("RENDER", "单词显示: %s, 位置(%d,%d), 字体%d", 
//                             word_to_display.c_str(), text_x, text_y, font_size);
//                 }
//             }
            
//             if (!word_displayed) {
//                 // 截断显示
//                 uint8_t font_size = 12;
//                 int max_chars = content_width / 6;
//                 if (max_chars > 3) {
//                     String truncated = word_to_display.substring(0, max_chars - 3) + "...";
//                     int text_x = content_x;
//                     int text_y = content_y + (content_height - font_size) / 2;
//                     updateDisplayWithString(text_x, text_y, 
//                                           (uint8_t*)truncated.c_str(), font_size, BLACK);
//                 }
//             }
//             break;
//         }
        
//         case CONTENT_PHONETIC: {
//             // 显示音标
//             if (entry.phonetic.length() > 0) {
//                 String formattedPhonetic = formatPhonetic(entry.phonetic);
                
//                 // 尝试不同字体大小
//                 uint8_t font_sizes[] = {rect->font_size, 14, 12};
//                 bool displayed = false;
                
//                 for (int i = 0; i < sizeof(font_sizes)/sizeof(font_sizes[0]) && !displayed; i++) {
//                     uint8_t font_size = font_sizes[i];
//                     uint16_t phonetic_width = calculateTextWidth(formattedPhonetic.c_str(), font_size);
                    
//                     if (phonetic_width <= content_width) {
//                         int text_x = content_x + (content_width - phonetic_width) / 2;
//                         int text_y = content_y + (content_height - font_size) / 2;
                        
//                         updateDisplayWithString(text_x, text_y, 
//                                               (uint8_t*)formattedPhonetic.c_str(), 
//                                               font_size, BLACK);
//                         displayed = true;
//                         ESP_LOGI("RENDER", "音标显示: %s", formattedPhonetic.c_str());
//                         break;
//                     }
//                 }
//             }
//             break;
//         }
        
//         case CONTENT_DEFINITION: {
//             // 显示英文释义/例句
//             if (entry.definition.length() > 0) {
//                 ESP_LOGI("RENDER", "显示英文释义: %s", entry.definition.c_str());
//                 displayEnglishWrapped(content_x, content_y, 
//                                     entry.definition.c_str(), 
//                                     rect->font_size, BLACK, 
//                                     content_width, rect->line_height);
//             }
//             break;
//         }
        
//         case CONTENT_TRANSLATION: {
//             // 显示中文翻译
//             if (entry.translation.length() > 0) {
//                 String processedTranslation = convertChinesePunctuationsInString(entry.translation);
//                 ESP_LOGI("RENDER", "显示中文翻译: %s", processedTranslation.c_str());
//                 displayWrappedText(content_x, content_y, 
//                                  processedTranslation.c_str(), 
//                                  rect->font_size, BLACK, 
//                                  content_width, rect->line_height);
//             }
//             break;
//         }
        
//         case CONTENT_SEPARATOR:
//             // 分隔线已通过图标显示，无需额外处理
//             break;
            
//         case CONTENT_CUSTOM:
//             // 自定义内容，暂不处理
//             break;
            
//         default:
//             ESP_LOGW("RENDER", "未知内容类型: %d", rect->content_type);
//             break;
//     }

// }
// void showSimplePromptWithNail(uint8_t *tempPrompt, int bg_x, int bg_y) {
//     if (tempPrompt == nullptr) {
//         return;
//     }
    
//     const char* promptText = (const char*)tempPrompt;
    
//     // 固定参数
//     int bg_width = 320;     // 背景框宽度
//     int bg_height = 36;     // 背景框高度
//     int border_margin = 6;  // 边框边距
//     int icon_width = 15;    // 图标宽度
//     int icon_height = 16;   // 图标高度
//     int icon_margin = 10;   // 图标左边距
//     int spacing = 8;        // 图标文字间距
//     int font_size = 16;     // 字体大小
    
//     // 1. 清空区域
//     clearDisplayArea(bg_x, bg_y, bg_width, bg_height);
    
//     // 2. 显示背景框
//     // 注：ZHONGJINGYUAN_3_7_promt 在 Pic.h 中未定义，暂时注释
//     // drawPictureScaled(bg_x, bg_y, bg_width, bg_height,
//     //                      ZHONGJINGYUAN_3_7_promt, BLACK);
    
//     // 3. 计算内部区域
//     int inner_x = bg_x + border_margin;
//     int inner_y = bg_y + border_margin;
//     int inner_width = bg_width - 2 * border_margin;
//     int inner_height = bg_height - 2 * border_margin;
    
//     // 4. 显示图标（左侧固定位置）
//     int icon_x = inner_x + icon_margin;
//     int icon_y = inner_y + (inner_height - icon_height) / 2;
    
//     drawPictureScaled(icon_x, icon_y, icon_width, icon_height,
//                          ZHONGJINGYUAN_3_7_NAIL, BLACK);
    
//     // 5. 计算文本位置（图标右侧）
//     int text_x = icon_x + icon_width + spacing;
//     int text_y = inner_y + (inner_height - font_size) / 2;
    
//     // 6. 检查文本宽度
//     int text_width = calculateTextWidth(promptText, font_size);
//     int max_width = inner_x + inner_width - text_x;
    
//     if (text_width > max_width) {
//         // 文本太长，截断
//         int max_chars = max_width / (font_size / 2);
//         if (max_chars > 3) {
//             char truncated[256];
//             strncpy(truncated, promptText, max_chars - 3);
//             truncated[max_chars - 3] = '\0';
//             strcat(truncated, "...");
            
//             promptText = truncated;
//             text_width = calculateTextWidth(truncated, font_size);
//         }
//     }
    
//     // 7. 显示文本
//     updateDisplayWithString(text_x, text_y, 
//                           (uint8_t*)promptText, font_size, BLACK);
    
//     ESP_LOGI("PROMPT", "显示提示: 图标(%d,%d) 文本(%d,%d) '%s'",
//             icon_x, icon_y, text_x, text_y, promptText);
// }

// void showPromptInfor(uint8_t *tempPrompt,bool isAllRefresh) {
//     static uint8_t *lastPrompt = nullptr;
//     static char lastPromptContent[256] = {0};
    
//     // 检查输入有效性
//     if (tempPrompt == nullptr) {
//      //   ESP_LOGW("PROMPT", "接收到空指针");
//         return;
//     }
    
//     const char* currentPrompt = (const char*)tempPrompt;
//     if(interfaceIndex !=1) {
    
//         // 检查内容是否变化（比较字符串内容而不是指针）
//         if (lastPrompt != nullptr && strcmp(currentPrompt, lastPromptContent) == 0) {
//             return;  // 内容相同，无需更新
//         }
//         ESP_LOGI("PROMPT", "更新提示信息: %s", currentPrompt);
        
//         // 保存当前内容用于下次比较
//         strncpy(lastPromptContent, currentPrompt, sizeof(lastPromptContent) - 1);
//         lastPromptContent[sizeof(lastPromptContent) - 1] = '\0';
//         lastPrompt = tempPrompt;

//         if(isAllRefresh) {
//             display.init(0, true, 2, true);
//             display.clearScreen();
//             display.display(true);  //局刷之前先对E-Paper进行清屏操作
//             display.setPartialWindow(0, 0, display.width(), display.height());
//             display.fillRect(30, 10, 340, 30, GxEPD_WHITE);
//             updateDisplayWithString(30,10, tempPrompt,16,BLACK);
//             display.display();
//             display.display(true);
//             display.powerOff();
//             vTaskDelay(1000);
//         } else {
//             display.fillRect(30, 10, 340, 30, GxEPD_WHITE);
//             updateDisplayWithString(30,10, tempPrompt,16,BLACK);
//         }

//     }
// }

// /**
//  * @brief 显示单词界面（完全基于矩形框配置）
//  * @param rects 矩形数组
//  * @param rect_count 矩形数量
//  * @param status_rect_index 状态栏矩形索引
//  * @param show_border 是否显示边框
//  */
// void enterVocabularyScreen() {
//     clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
//     ESP_LOGI(TAG,"############显示单词本界面（使用新图标布局系统）\r\n");
//     update_activity_time(); // 更新活动时间
    
//     // 重新加载单词界面配置，确保使用最新的网页端配置
//     if (loadVocabLayoutFromConfig()) {
//         ESP_LOGI(TAG, "单词界面布局已从配置文件重新加载");
//     } else {
//         ESP_LOGI(TAG, "使用当前单词界面布局");
//     }
    
//     // 直接从矩形数据计算有效矩形数量，不依赖配置值
//     extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
//     int valid_rect_count = 0;
//     for (int i = 0; i < MAX_VOCAB_RECTS; i++) {
//         if (vocab_rects[i].width > 0 && vocab_rects[i].height > 0) {
//             valid_rect_count++;
//             ESP_LOGI("FOCUS", "有效矩形%d: (%d,%d) %dx%d", i, 
//                     vocab_rects[i].x, vocab_rects[i].y, 
//                     vocab_rects[i].width, vocab_rects[i].height);
//         } else {
//             ESP_LOGI("FOCUS", "无效矩形%d: (%d,%d) %dx%d", i, 
//                     vocab_rects[i].x, vocab_rects[i].y, 
//                     vocab_rects[i].width, vocab_rects[i].height);
//             break; // 遇到第一个无效矩形就停止计数（假设矩形是连续的）
//         }
//     }
    
//     ESP_LOGI("FOCUS", "检测到有效矩形数量: %d", valid_rect_count);
    
//     // 使用实际检测到的有效矩形数量初始化焦点系统（默认全部可焦点）
//     initFocusSystem(valid_rect_count);
    
//     // 尝试从配置文件加载自定义的可焦点矩形列表
//     if (loadFocusableRectsFromConfig("vocab")) {
//         ESP_LOGI("FOCUS", "已从配置文件加载单词界面焦点矩形列表");
//     } else {
//         ESP_LOGI("FOCUS", "使用默认焦点配置（所有矩形都可焦点）");
//     }
    
//     // 加载子数组配置
//     if (loadAndApplySubArrayConfig("vocab")) {
//         ESP_LOGI("FOCUS", "已从配置文件加载并应用单词界面子数组配置");
//     } else {
//         ESP_LOGI("FOCUS", "未加载单词界面子数组配置或配置为空");
//     }
    
//     // 设置当前界面为单词界面
//     g_screen_manager.current_screen = SCREEN_VOCABULARY;
    
//     // 使用displayScreen统一接口，与主界面保持一致
//     // displayScreen 内部会调用 displayVocabularyScreen
//     displayScreen(SCREEN_VOCABULARY);
    
//     // 进入单词界面后，执行全局的局部刷新
//     // 先进行全屏刷新清除旧内容，再进行局部刷新显示新内容
//     ESP_LOGI(TAG, "[VOCAB] 执行全局的局部刷新...");
//     vTaskDelay(100);
//     display.display();          // 全局刷新 - 清除旧内容
//     vTaskDelay(100);
//     display.display(true);      // 局部刷新 - 显示新内容
//     display.powerOff();
//     ESP_LOGI(TAG, "[VOCAB] 全局的局部刷新完成");
    
//     vTaskDelay(1000);
// }
// void displayVocabularyScreen(RectInfo *rects, int rect_count, 
//                            int status_rect_index, int show_border) {
    
//     // 安全检查
//     if (rects == nullptr) {
//         ESP_LOGE("VOCAB", "错误：rects参数为空指针！");
//         return;
//     }
//     if (rect_count <= 0) {
//         ESP_LOGE("VOCAB", "错误：rect_count无效: %d", rect_count);
//         return;
//     }
    
//     ESP_LOGI("VOCAB", "参数检查通过 - rects: %p, count: %d, status_idx: %d, border: %d", 
//             rects, rect_count, status_rect_index, show_border);
    
//     // 计算缩放比例
//     float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
//     float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
//     float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
//     ESP_LOGI("VOCAB", "========== 开始显示单词界面 ==========");
//     ESP_LOGI("VOCAB", "屏幕尺寸: %dx%d, 缩放比例: %.4f", 
//             setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt, global_scale);
    
//     // 初始化显示
//     display.init(0, true, 2, true);
//     display.clearScreen();
//     display.display(true);
//     delay_ms(50);
//     display.setPartialWindow(0, 0, display.width(), display.height());
    
//     // 读取单词数据
//     readAndPrintRandomWord();
//     if(entry.word.length() == 0) {
//         ESP_LOGW("VOCAB", "未读取到单词数据");
//         entry.word = "Sample";
//         entry.phonetic = "ˈsæmpl";
//         entry.definition = "A sample word for testing.";
//         entry.translation = "示例单词";
//     }
    
//     ESP_LOGI("VOCAB", "当前单词: %s [%s]", entry.word.c_str(), entry.phonetic.c_str());
    
//     // ==================== 遍历所有矩形，显示背景图标和内容 ====================
//     for (int i = 0; i < rect_count; i++) {
//         RectInfo* rect = &rects[i];
        
//         // 跳过无效矩形（宽度或高度为0）
//         if (rect->width <= 0 || rect->height <= 0) {
//             ESP_LOGI("VOCAB", "--- 矩形%d: 无效矩形，跳过 ---", i);
//             continue;
//         }
        
//         // 计算缩放后的位置和尺寸
//         int display_x = (int)(rect->x * global_scale + 0.5f);
//         int display_y = (int)(rect->y * global_scale + 0.5f);
//         int display_width = (int)(rect->width * global_scale + 0.5f);
//         int display_height = (int)(rect->height * global_scale + 0.5f);
        
//         ESP_LOGI("VOCAB", "--- 矩形%d: (%d,%d) %dx%d, 类型:%d ---", 
//                 i, display_x, display_y, display_width, display_height, rect->content_type);
        
//         // 1. 显示背景图标（如果有）
//         if (rect->icon_count > 0) {
//             ESP_LOGI("VOCAB", "  矩形%d有%d个图标", i, rect->icon_count);
//             for (int j = 0; j < rect->icon_count; j++) {
//                 IconPositionInRect* icon = &rect->icons[j];
//                 int icon_index = icon->icon_index;
                
//                 ESP_LOGI("VOCAB", "  处理图标%d, index=%d", j, icon_index);
                
//                 if (icon_index >= 0 && icon_index < 21) {
//                     IconInfo* icon_info = &g_available_icons[icon_index];
                    
//                     // 安全检查图标数据
//                     if (icon_info->data == nullptr) {
//                         ESP_LOGW("VOCAB", "  图标%d数据为空，跳过", icon_index);
//                         continue;
//                     }
                    
//                     // 计算图标位置（基于左上角对齐）
//                     // rel_x, rel_y 表示图标左上角在矩形中的相对位置
//                     // 0.0, 0.0 = 图标左上角在矩形左上角
//                     // 使用原始坐标（未缩放），EPD_ShowPictureScaled内部会处理缩放
//                     int icon_x = rect->x + (int)(icon->rel_x * rect->width);
//                     int icon_y = rect->y + (int)(icon->rel_y * rect->height);
                    
//                     ESP_LOGI("VOCAB", "  显示图标%d: 原始位置(%d,%d) 尺寸%dx%d", 
//                             icon_index, icon_x, icon_y, icon_info->width, icon_info->height);
                    
//                     drawPictureScaled(icon_x, icon_y, 
//                                         icon_info->width, icon_info->height,
//                                         icon_info->data, BLACK);
//                 } else {
//                     ESP_LOGW("VOCAB", "  图标索引%d超出范围[0-20]，跳过", icon_index);
//                 }
//             }
//         }
        
//         // 2. 渲染矩形内容（仅当有文本内容或非自定义模式时）
//         // 参考主界面逻辑，只有明确需要内容显示的矩形才进行渲染
//         bool should_render_content = false;
        
//         if (rect->text_count > 0) {
//             // 有自定义文本内容，需要渲染
//             should_render_content = true;
//         } else if (!rect->custom_text_mode) {
//             // 非自定义模式，且有默认内容类型，需要渲染
//             if (rect->content_type == CONTENT_WORD || 
//                 rect->content_type == CONTENT_PHONETIC ||
//                 rect->content_type == CONTENT_DEFINITION ||
//                 rect->content_type == CONTENT_TRANSLATION) {
//                 should_render_content = true;
//             }
//         }
//         // 如果是自定义模式且没有自定义文本，不渲染任何默认内容（类似主界面只显示图标的逻辑）
        
//         if (should_render_content) {
//             ESP_LOGI("VOCAB", "  矩形%d需要渲染内容", i);
            
//             // 清空内容显示区域（自定义文本或默认内容都需要清空）
//           //  clearDisplayArea(display_x, display_y, display_width, display_height);
            
//             // 3. 渲染矩形内容
//             renderRectContent(rect, entry, global_scale);
//         } else {
//             ESP_LOGI("VOCAB", "  矩形%d跳过内容渲染（无内容或自定义模式无文本）", i);
//         }
//     }
    
//     // ==================== 显示状态栏WiFi/电池图标（特殊处理） ====================
//     // if (status_rect_index >= 0 && rect_count > status_rect_index) {
//     //     int wifi_x, wifi_y, battery_x, battery_y;
//     //     getStatusIconPositions(rects, rect_count, status_rect_index,
//     //                           &wifi_x, &wifi_y, &battery_x, &battery_y);
        
//     //     int wifi_display_x = (int)(wifi_x * global_scale + 0.5f);
//     //     int wifi_display_y = (int)(wifi_y * global_scale + 0.5f);
//     //     int battery_display_x = (int)(battery_x * global_scale + 0.5f);
//     //     int battery_display_y = (int)(battery_y * global_scale + 0.5f);
        
//     //     // WiFi图标
//     //     // drawPictureScaled(wifi_display_x, wifi_display_y, 32, 32, 
//     //     //                      ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK);
        
//     //     // 电池图标
//     //     #ifdef BATTERY_LEVEL
//     //         const uint8_t* battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
//     //         // 根据电量选择图标...
//     //     #else
//     //         const uint8_t* battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
//     //     #endif
//     //     // drawPictureScaled(battery_display_x, battery_display_y, 36, 24, 
//     //     //                      battery_icon, BLACK);
//     // }
    
//     // ==================== 显示矩形边框（调试用） ====================
//     if (show_border) {
//         ESP_LOGI("BORDER", "开始绘制矩形边框，共%d个矩形", rect_count);
        
//         for (int i = 0; i < rect_count; i++) {
//             RectInfo* rect = &rects[i];
            
//             // 跳过无效矩形（宽度或高度为0）
//             if (rect->width <= 0 || rect->height <= 0) {
//                 continue;
//             }
            
//             int display_x = (int)(rect->x * global_scale + 0.5f);
//             int display_y = (int)(rect->y * global_scale + 0.5f);
//             int display_width = (int)(rect->width * global_scale + 0.5f);
//             int display_height = (int)(rect->height * global_scale + 0.5f);
            
//             // 绘制边框
//             display.drawRect(display_x, display_y, 
//                             display_width, 
//                             display_height, 
//                             BLACK);
            
//             ESP_LOGI("BORDER", "矩形%d边框: (%d,%d) %dx%d", 
//                     i, display_x, display_y, display_width, display_height);
//         }
//     }
    
//     // ==================== 绘制焦点光标（调试模式） ====================
//     if (g_focus_mode_enabled && g_current_focus_rect >= 0 && g_current_focus_rect < rect_count) {
//         drawFocusCursor(rects, g_current_focus_rect, global_scale);
//         ESP_LOGI("FOCUS", "绘制焦点光标在矩形%d", g_current_focus_rect);
//     }
    
//     // 更新显示
//     display.display();
//     display.display(true);
//     display.powerOff();
//     delay_ms(100);
    
//     ESP_LOGI("VOCAB", "========== 单词显示完成 ==========");
// }

// uint16_t displayWrappedText(uint16_t x, uint16_t y, const char* text, uint8_t fontSize, 
//                            uint16_t color, uint16_t max_width, uint16_t line_height) {
//     if (text == nullptr || strlen(text) == 0) {
//         return y;
//     }
    
//     String processedText = convertChinesePunctuationsInString(String(text));
//     const char* processedStr = processedText.c_str();
    
//     ESP_LOGI("WRAP", "原始文本: %s", text);
//     ESP_LOGI("WRAP", "处理后文本: %s", processedStr);
    
//     uint16_t current_x = x;
//     uint16_t current_y = y;
//     uint16_t english_width = fontSize / 2;
//     uint16_t chinese_width = fontSize;
    
//     for (int i = 0; processedStr[i] != '\0'; ) {
//         // 处理换行符
//         if (processedStr[i] == '\\' && processedStr[i+1] == 'n') {
//             // 在 \n 换行的下方画线
//             uint16_t line_y = current_y + line_height - 2;
//             ESP_LOGI("WRAP", "\\n 换行画线 at Y=%d", line_y);
//             display.drawLine(x, line_y, x + max_width, line_y, BLACK);
            
//             current_x = x;
//             current_y += line_height;
//             i += 2;
//             continue;
//         }
//         else if (processedStr[i] == '\n') {
//             // 在 \n 换行的下方画线
//             uint16_t line_y = current_y + line_height - 2;
//             ESP_LOGI("WRAP", "换行符画线 at Y=%d", line_y);
//             display.drawLine(x, line_y, x + max_width, line_y, BLACK);
            
//             current_x = x;
//             current_y += line_height;
//             i++;
//             continue;
//         }
//         else if (processedStr[i] == '\r') {
//             i++;
//             continue;
//         }
        
//         if ((processedStr[i] & 0x80) == 0) {
//             // ASCII字符 - 自动换行不画线
//             if (current_x + english_width > x + max_width) {
//                 current_x = x;
//                 current_y += line_height;
//             }
            
//             // 使用 GXEPD2 绘制字符
//              display.drawChar(current_x, current_y, processedStr[i], color, WHITE, 1);
//             current_x += english_width;
//             i++;
            
//         } else if ((processedStr[i] & 0xE0) == 0xE0) {
//             // 中文字符 - 自动换行不画线
//             if (current_x + chinese_width > x + max_width) {
//                 current_x = x;
//                 current_y += line_height;
//             }
            
//             if (processedStr[i+1] != '\0' && processedStr[i+2] != '\0') {
//                 uint8_t chinese_char[4] = {processedStr[i], processedStr[i+1], processedStr[i+2], '\0'};
//                 // display.drawBitmap(中文显示);
//                 current_x += chinese_width;
//                 i += 3;
//             } else {
//                 i++;
//             }
//         } else {
//             i++;
//         }
//     }
    
//     ESP_LOGI("WRAP", "换行显示完成，最终Y坐标: %d", current_y + line_height);
//     return current_y + line_height;
// }
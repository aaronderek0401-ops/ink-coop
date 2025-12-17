#include "word_book.h"
#include "sleep_interface.h"
#include "ink_screen.h"
#include <GxEPD2_BW.h>
#include "wordbook_interface.h"
#include "./Pic.h"
// 居中换行文本显示
uint16_t display_centered_wrapped_text(uint16_t center_x, uint16_t y, const char* text, 
                                      uint8_t fontSize, uint16_t color, 
                                      uint16_t max_width, uint16_t line_height) {
    if (text == nullptr || strlen(text) == 0) return y;
    
    String processedText = convertChinesePunctuationsInString(String(text));
    const char* processedStr = processedText.c_str();
    
    uint16_t current_y = y;
    uint16_t english_width = fontSize / 2;
    uint16_t chinese_width = fontSize;
    
    // 按行分割文本
    char* text_copy = strdup(processedStr);
    char* line = strtok(text_copy, "\n");
    
    while (line != NULL) {
        // 计算当前行的宽度
        uint16_t line_width = calculateTextWidth(line, fontSize);
        uint16_t line_x = center_x - line_width / 2;
        
        // 显示当前行
        uint16_t line_end_y = display_centered_line(line_x, current_y, line, fontSize, color, max_width, line_height);
        current_y = line_end_y;
        
        line = strtok(NULL, "\n");
    }
    
    free(text_copy);
    return current_y;
}
void display_sleep_mode() {
    if (!has_sleep_data) {
        ESP_LOGW("SLEEP", "没有数据可显示");
        return;
    }
    
    display.init(0, true, 2, true);
    display.clearScreen();
    display.display(true);
    display.setPartialWindow(0, 0, display.width(), display.height());
    display.fillRect(10, 40, EPD_W, EPD_H - 40, GxEPD_WHITE);
    display.drawImage(ZHONGJINGYUAN_3_7_LOCK, 380, 40, 32, 32, true);
    uint16_t screen_width = 416;
    uint16_t screen_height = 240;
    uint16_t center_x = screen_width / 2;
    
    // 计算总内容高度并确定起始Y坐标
    uint16_t total_height = 0;
    uint16_t line_spacing = 16; // 行间距
    
    // 估算各部分高度
    uint16_t word_height = 24 + line_spacing;
    uint16_t phonetic_height = 20 + line_spacing;
    
    // 计算翻译和句子的行数 - 直接内联计算以避免函数重载混淆
    uint16_t defn_lines = 1, trans_lines = 1;
    
    // 计算定义行数
    if (sleep_mode_entry.definition.length() > 0) {
        uint16_t w = 0;
        const char* def_text = sleep_mode_entry.definition.c_str();
        for (int i = 0; def_text[i] != '\0'; i++) {
            uint16_t char_width = (def_text[i] & 0x80) == 0 ? 8 : 16;
            w += char_width;
            if (w > (screen_width - 40)) { defn_lines++; w = char_width; }
        }
    }
    
    // 计算翻译行数
    if (sleep_mode_entry.translation.length() > 0) {
        uint16_t w = 0;
        const char* trans_text = sleep_mode_entry.translation.c_str();
        for (int i = 0; trans_text[i] != '\0'; i++) {
            uint16_t char_width = (trans_text[i] & 0x80) == 0 ? 8 : 16;
            w += char_width;
            if (w > (screen_width - 40)) { trans_lines++; w = char_width; }
        }
    }
    
    uint16_t definition_lines = defn_lines;
    uint16_t translation_lines = trans_lines;
    
    uint16_t definition_height = definition_lines * (16 + 5) + line_spacing; // 16字体+5行间距
    uint16_t translation_height = translation_lines * (16 + 5) + line_spacing;
    
    total_height = word_height + phonetic_height + definition_height + translation_height;
    
    // 计算起始Y坐标（垂直居中）
    uint16_t start_y = (screen_height - total_height) / 2;
    uint16_t current_y = start_y;
    
    // 1. 居中显示单词
    if (sleep_mode_entry.word.length() > 0) {
        uint16_t word_width = calculateTextWidth(sleep_mode_entry.word.c_str(), 24);
        uint16_t word_x = center_x - word_width / 2;
        updateDisplayWithString(word_x, current_y, (uint8_t*)sleep_mode_entry.word.c_str(), 24, BLACK);
        current_y += 24 + line_spacing;
    }
    
    // 2. 居中显示音标
    if (sleep_mode_entry.phonetic.length() > 0) {
        String formattedPhonetic = formatPhonetic(sleep_mode_entry.phonetic.c_str());
        uint16_t phonetic_width = calculateTextWidth(formattedPhonetic.c_str(), 24);
        uint16_t phonetic_x = center_x - phonetic_width / 2;
        updateDisplayWithString(phonetic_x, current_y, (uint8_t*)formattedPhonetic.c_str(), 24, BLACK);
        current_y += 20 + line_spacing;
        ESP_LOGI("SLEEP", "显示音标: x=%d, y=%d, 内容: %s", phonetic_x, current_y, formattedPhonetic.c_str());
    }
    
    // 3. 居中显示翻译（多行）
    if (sleep_mode_entry.translation.length() > 0) {
        String processedTranslation = convertChinesePunctuationsInString(sleep_mode_entry.translation);
        current_y = display_centered_wrapped_text(center_x, current_y, processedTranslation.c_str(), 16, BLACK, 
                                                screen_width - 40, 21); // 16字体+5行间距
        current_y += line_spacing;
    }
    
    // 4. 居中显示句子（多行）
    if (sleep_mode_entry.definition.length() > 0) {
        String processedDefinition = convertChinesePunctuationsInString(sleep_mode_entry.definition);
        display_centered_wrapped_text(center_x, current_y, processedDefinition.c_str(), 16, BLACK, 
                                    screen_width - 40, 21); // 16字体+5行间距
    }
    
    display.display();
    display.display(true);
    display.powerOff();
}
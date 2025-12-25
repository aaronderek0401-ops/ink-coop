// #include "word_book.h"
// #include <FS.h>
// #include "../../Grbl.h"
// WordEntry entry;
// // 休眠模式显示的数据
//  WordEntry sleep_mode_entry;
//  bool has_sleep_data = false;
// static const char *TAG = "word_book.cpp";

// void safeDisplayWordEntry(const WordEntry& entry, uint16_t x, uint16_t y) {
//     // ESP_LOGI("DISPLAY", "Start displaying word entry...");
    
//     // if(entry.word.length() == 0) {
//     //     ESP_LOGE("DISPLAY", "Word field is empty");
//     //     return;
//     // }
    
//     // uint16_t current_y = y;
//     // uint16_t line_height_word = 30;    // 单词和音标行高
//     // uint16_t line_height_text = 20;    // 句子和翻译行高
//     // uint16_t screen_width = 416;
//     // uint16_t right_margin = 10;
//     // uint16_t max_word_width = screen_width - x - right_margin;

//     // // 第一行：单词（支持自动换行）
//     // ESP_LOGI("DISPLAY", "Display the word: %s", entry.word.c_str());
    
//     // // 计算单词是否需要换行
//     // uint16_t word_width = calculateTextWidth(entry.word.c_str(), 24);
//     // esp_task_wdt_reset();
    
//     // if (word_width <= max_word_width) {
//     //     // 单词可以在一行显示
//     //     updateDisplayWithString(x, current_y, (uint8_t*)entry.word.c_str(), 24, BLACK);
//     //     current_y += line_height_word;  // 移动到下一行
//     // } else {
//     //     // 单词需要换行显示
//     //     current_y = displayEnglishWrapped(x, current_y, entry.word.c_str(), 24, BLACK, 
//     //                                     max_word_width, line_height_word);
//     //     // displayEnglishWrapped 返回最后一行文本的底部Y坐标
//     //     // 不需要再额外增加行高
//     // }
    
//     // // 音标处理
//     // if(entry.phonetic.length() > 0) {
//     //     esp_task_wdt_reset();
//     //     String formattedPhonetic = formatPhonetic(entry.phonetic);
//     //     uint16_t phonetic_width = calculateTextWidth(formattedPhonetic.c_str(), 24);
//     //     uint16_t icon_width = 16;
//     //     uint16_t icon_height = 16;
//     //     uint16_t icon_offset_y = -16;  // 图标向上偏移16像素
        
//     //     // 计算可用空间和动态间距
//     //     uint16_t available_space = screen_width - right_margin - (x + word_width);
//     //     uint16_t min_spacing = 15;   // 最小间距
//     //     uint16_t max_spacing = 80;  // 最大间距
//     //     uint16_t spacing = min_spacing;
        
//     //     // 如果有充足空间，增加间距
//     //     if (available_space > phonetic_width + icon_width + min_spacing + 20) {
//     //         // 计算动态间距：可用空间的1/3，但不超出最大间距
//     //         spacing = (available_space - phonetic_width - icon_width) / 3;
//     //         if (spacing > max_spacing) {
//     //             spacing = max_spacing;
//     //         }
//     //     }
        
//     //     ESP_LOGI("DISPLAY", "Word width: %d, Phonetic width: %d, Available space: %d, Used spacing: %d", 
//     //             word_width, phonetic_width, available_space, spacing);
        
//     //     // 检查音标是否可以与单词同行显示（仅当单词没有换行时）
//     //     bool word_fits_one_line = (word_width <= max_word_width);
//     //     bool canDisplayInline = word_fits_one_line && 
//     //                            (x + word_width + spacing + phonetic_width + icon_width) <= (screen_width - right_margin);
        
//     //     if (canDisplayInline) {
//     //         // 同行显示音标（在单词右侧，使用动态间距）
//     //         uint16_t phonetic_x = x + word_width + spacing;
            
//     //         // 图标显示在音标正上方偏右
//     //         uint16_t icon_x = phonetic_x + phonetic_width - icon_width + 2; // 偏右2像素
//     //         uint16_t icon_y = y + icon_offset_y; // 向上偏移
            
//     //         EPD_ShowPicture(icon_x, icon_y, icon_width, icon_height, (uint8_t *)ZHONGJINGYUAN_3_7_HORN, BLACK);
//     //         updateDisplayWithString(phonetic_x, y, (uint8_t*)formattedPhonetic.c_str(), 24, BLACK);
//     //         // 音标与单词同行，Y坐标保持不变
            
//     //         ESP_LOGI("DISPLAY", "The phonetic transcription shows, spacing: %d", spacing);
//     //     } else {
//     //         // 换行显示音标 - 靠右显示，图标在音标正上方偏右
//     //         uint16_t phonetic_x = screen_width - right_margin - phonetic_width;
            
//     //         // 图标显示在音标正上方偏右
//     //         uint16_t icon_x = phonetic_x + phonetic_width - icon_width + 2; // 偏右2像素
//     //         uint16_t icon_y = current_y + icon_offset_y; // 向上偏移
            
//     //         EPD_ShowPicture(icon_x, icon_y, icon_width, icon_height, (uint8_t *)ZHONGJINGYUAN_3_7_HORN, BLACK);
//     //         updateDisplayWithString(phonetic_x, current_y, (uint8_t*)formattedPhonetic.c_str(), 24, BLACK);
//     //         current_y += line_height_word;  // 移动到音标下方
            
//     //         ESP_LOGI("DISPLAY", "Phonetic wrapped display");
//     //     }
//     // } else {
//     //     // 没有音标，确保Y坐标正确推进
//     //     if (word_width <= max_word_width) {
//     //         // 单词单行显示，Y坐标已经在上面推进过了
//     //     } else {
//     //         // 单词多行显示，current_y 已经在 displayEnglishWrapped 中正确设置
//     //         // 但可能需要增加一些间距
//     //         current_y += 5; // 增加小间距
//     //     }
//     // }
    
//     // // 句子和翻译显示 - 确保有足够的间距
//     // bool hasDefinition = entry.definition.length() > 0;
//     // bool hasTranslation = entry.translation.length() > 0;
    
//     // // 在显示句子和翻译前增加间距
//     // if (hasDefinition || hasTranslation) {
//     //     current_y += 10; // 增加段落间距
//     // }
    
//     // esp_task_wdt_reset();
    
//     // if(hasDefinition) {
//     //     ESP_LOGI("DISPLAY", "Display sentence: %s", entry.definition.c_str());
        
//     //     // 英语句子按字符自动换行
//     //     current_y = displayEnglishWrapped(x, current_y, entry.definition.c_str(), 16, BLACK, 
//     //                                     screen_width - x - right_margin, line_height_text);
        
//     //     // 在句子和翻译之间增加间距
//     //     if(hasTranslation) {
//     //         current_y += 8;
            
//     //         ESP_LOGI("DISPLAY", "Display translation: %s", entry.translation.c_str());
//     //         // 预处理翻译文本，转换中文标点
//     //         String processedTranslation = convertChinesePunctuationsInString(entry.translation);
//     //         // 翻译也支持自动换行
//     //         current_y = displayWrappedText(x, current_y, processedTranslation.c_str(), 16, BLACK, 
//     //                                  screen_width - x - right_margin, line_height_text);
//     //     }
//     // } else if(hasTranslation) {
//     //     ESP_LOGI("DISPLAY", "Display translation: %s", entry.translation.c_str());
//     //     // 预处理翻译文本，转换中文标点
//     //     String processedTranslation = convertChinesePunctuationsInString(entry.translation);
//     //     // 翻译支持自动换行
//     //     current_y = displayWrappedText(x, current_y, processedTranslation.c_str(), 16, BLACK, 
//     //                              screen_width - x - right_margin, line_height_text);
//     // }
    
//     // esp_task_wdt_reset();
//     // ESP_LOGI("DISPLAY", "Word entry display completed, final Y coordinate: %d", current_y);
// }

// int countLines(File &file) {
//   int count = 0;
//   file.seek(0);
//   while (file.available()) {
//     if (file.read() == '\n') count++;
//   }
//   file.seek(0);
//   return count;
// }

// WordEntry readLineAtPosition(File &file, int lineNumber) {
//   file.seek(0);
//   int currentLine = 0;
//   WordEntry entry;
  
//   while (file.available() && currentLine <= lineNumber) {
//     String line = file.readStringUntil('\n');
//     if (currentLine == lineNumber) {
//       parseCSVLine(line, entry);
//       break;
//     }
//     currentLine++;
//   }
  
//   return entry;
// }

// void parseCSVLine(String line, WordEntry &entry) {
//   int fieldCount = 0;
//   String field = "";
//   bool inQuotes = false;
//   char lastChar = 0;
  
//   for (int i = 0; i < line.length(); i++) {
//     char c = line[i];
    
//     if (lastChar != '\\' && c == '"') {
//       inQuotes = !inQuotes;
//     } else if (c == ',' && !inQuotes) {
//       // 字段结束
//       assignField(fieldCount, field, entry);
//       field = "";
//       fieldCount++;
//     } else {
//       field += c;
//     }
//     lastChar = c;
//   }
  
//   // 处理最后一个字段
//   if (fieldCount < 5) {
//     assignField(fieldCount, field, entry);
//   }
// }

// void assignField(int fieldCount, String &field, WordEntry &entry) {
//   // 移除字段两端的引号（如果存在）
//   if (field.length() >= 2 && field[0] == '"' && field[field.length()-1] == '"') {
//     field = field.substring(1, field.length()-1);
//   }
  
//   switch (fieldCount) {
//     case 0: entry.word = field; break;
//     case 1: entry.phonetic = field; break;
//     case 2: entry.definition = field; break;
//     case 3: entry.translation = field; break;
//     case 4: entry.pos = field; break;
//   }
// }

// void printWordEntry(WordEntry &entry, int lineNumber) {
//   ESP_LOGI(TAG, "line number: %d", lineNumber);
//   ESP_LOGI(TAG, "word: %s", entry.word.c_str());
//   if (entry.phonetic.length() > 0) {
//     ESP_LOGI(TAG, "symbol: %s", entry.phonetic.c_str());
//   }
  
//   if (entry.definition.length() > 0) {
//     ESP_LOGI(TAG, "English Definition: %s", entry.definition.c_str());
//   }
  
//   if (entry.translation.length() > 0) {
//     ESP_LOGI(TAG, "chinese Definition: %s", entry.translation.c_str());
//   }
  
//   if (entry.pos.length() > 0) {
//     ESP_LOGI(TAG, "part of speech: %s", entry.pos.c_str());
//   }
  
//   ESP_LOGI(TAG, "----------------------------------------");
// }

// void readAndPrintWords() {
//   File file = SD.open("/ecdict.mini.csv");
//   if (!file) {
//     ESP_LOGE(TAG, "Unable to open CSV file");
//     return;
//   }
  
//   // 读取前几行作为示例
//   ESP_LOGI(TAG, "Display first 5 words");
//   int lineCount = 0;
  
//   while (file.available() && lineCount < 5) {
//     String line = file.readStringUntil('\n');
//     if (line.length() > 0) {
//       WordEntry entry;
//       parseCSVLine(line, entry);
//       printWordEntry(entry, lineCount);
//       lineCount++;
//     }
//   }
  
//   file.close();
  
//   ESP_LOGI(TAG, "Start displaying random word");
// }

// void readAndPrintRandomWord() {
//   SDState state = get_sd_state(true);
//   if (state != SDState::Idle) {
//       if (state == SDState::NotPresent) {
//          ESP_LOGE(TAG, "No SD Card");
//       } else {
//           ESP_LOGE(TAG, "SD Card Busy");
//       }
//   }
//   File file = SD.open("/ecdict.mini.csv");
//   if (!file) {
//     ESP_LOGE(TAG, "Unable to open CSV file");
//     return;
//   }
  
//   // 计算总行数
//   int totalLines = countLines(file);
//   if (totalLines <= 1) {
//     ESP_LOGE(TAG, "Insufficient file content");
//     file.close();
//     return;
//   }
  
//   // 随机选择一行（跳过标题行）
//   int randomLine = random(1, totalLines);
//   entry = readLineAtPosition(file, randomLine);
//   file.close();
  
//   ESP_LOGI(TAG, "Random word");
//   printWordEntry(entry, randomLine);
// }
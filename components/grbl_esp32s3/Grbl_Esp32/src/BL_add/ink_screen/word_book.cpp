#include "word_book.h"
#include <FS.h>
#include "esp_heap_caps.h"
#include "../../Grbl.h"

WordEntry entry;
// 休眠模式显示的数据
WordEntry sleep_mode_entry;
bool has_sleep_data = false;
static const char *TAG = "word_book.cpp";

// ================== 单词本文本缓存（PSRAM）==================
// 单词各字段的文本缓存（存储格式化的字符串）
char* g_wordbook_word_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_phonetic_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_translation_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_translation1_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_translation2_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_pos_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_wrong_translation_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_option1_cache[WORDBOOK_CACHE_COUNT] = {nullptr};
char* g_wordbook_option2_cache[WORDBOOK_CACHE_COUNT] = {nullptr};

// 单词指针数组（供text_arrays使用）
const char* g_wordbook_word_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_phonetic_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_translation_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_translation1_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_translation2_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_pos_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_wrong_translation_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_option1_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
const char* g_wordbook_option2_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};

// 单词本是否已初始化
bool g_wordbook_text_initialized = false;

// 全局单词本缓存实例
WordBookCache g_wordbook_cache;

// ============ WordBookCache 类实现 ============

WordBookCache::WordBookCache() 
    : cache_(nullptr)
    , cache_size_(0)
    , current_line_(1)  // 从第1行开始（跳过标题行）
    , current_index_(0)
    , total_lines_(0)
    , is_initialized_(false)
    , csv_file_path_("")
{
}

WordBookCache::~WordBookCache() {
    if (cache_) {
        // 释放PSRAM内存
        heap_caps_free(cache_);
        cache_ = nullptr;
    }
}

bool WordBookCache::init(const char* csv_path, int cache_size) {
    if (is_initialized_) {
        ESP_LOGW(TAG, "单词本缓存已初始化");
        return true;
    }
    
    if (!csv_path || cache_size <= 0) {
        ESP_LOGE(TAG, "无效参数");
        return false;
    }
    
    csv_file_path_ = String(csv_path);
    cache_size_ = cache_size;
    
    // 检查文件是否存在并计算总行数
    File file = SD.open(csv_path);
    if (!file) {
        ESP_LOGE(TAG, "无法打开CSV文件: %s", csv_path);
        return false;
    }
    
    total_lines_ = countLines(file);
    file.close();
    
    if (total_lines_ <= 1) {
        ESP_LOGE(TAG, "文件内容不足（总行数: %d）", total_lines_);
        return false;
    }
    
    ESP_LOGI(TAG, "CSV文件总行数: %d (包含标题行)", total_lines_);
    
    // 从PSRAM分配缓存内存
    cache_ = (WordEntry*)heap_caps_malloc(cache_size_ * sizeof(WordEntry), MALLOC_CAP_SPIRAM);
    if (!cache_) {
        ESP_LOGE(TAG, "PSRAM内存分配失败 (%d 字节)", cache_size_ * sizeof(WordEntry));
        return false;
    }
    
    // 初始化缓存中的WordEntry对象
    for (int i = 0; i < cache_size_; i++) {
        new (&cache_[i]) WordEntry();
    }
    
    ESP_LOGI(TAG, "✅ 单词本缓存初始化成功");
    ESP_LOGI(TAG, "   - 文件路径: %s", csv_path);
    ESP_LOGI(TAG, "   - 缓存大小: %d 条", cache_size_);
    ESP_LOGI(TAG, "   - 内存占用: %d 字节 (PSRAM)", cache_size_ * sizeof(WordEntry));
    ESP_LOGI(TAG, "   - 有效行数: %d 行", total_lines_ - 1);
    
    is_initialized_ = true;
    
    // 预加载第一批单词
    return preloadNextBatch();
}

bool WordBookCache::preloadNextBatch() {
    if (!is_initialized_ || !cache_) {
        ESP_LOGE(TAG, "缓存未初始化");
        return false;
    }
    
    File file = SD.open(csv_file_path_.c_str());
    if (!file) {
        ESP_LOGE(TAG, "无法打开CSV文件");
        return false;
    }
    
    ESP_LOGI(TAG, "========== 开始预加载单词 ==========");
    ESP_LOGI(TAG, "从第 %d 行开始加载 %d 条单词", current_line_, cache_size_);
    
    int loaded = 0;
    file.seek(0);
    int line_num = 0;
    
    // 跳过标题行和之前的行
    while (file.available() && line_num < current_line_) {
        file.readStringUntil('\n');
        line_num++;
    }
    
    // 加载指定数量的单词
    while (file.available() && loaded < cache_size_) {
        String line = file.readStringUntil('\n');
        
        if (line.length() > 0) {
            parseCSVLine(line, cache_[loaded]);
            
            ESP_LOGD(TAG, "[%d/%d] 加载: %s", loaded + 1, cache_size_, 
                     cache_[loaded].word.c_str());
            
            loaded++;
            current_line_++;
            
            // 如果到达文件末尾，循环回到开头
            if (current_line_ >= total_lines_) {
                ESP_LOGI(TAG, "已到达文件末尾，循环回到第1行");
                current_line_ = 1;  // 重新从第1行开始（跳过标题）
                file.close();
                file = SD.open(csv_file_path_.c_str());
                if (!file) {
                    ESP_LOGE(TAG, "重新打开文件失败");
                    break;
                }
                
                // 跳过标题行
                if (file.available()) {
                    file.readStringUntil('\n');
                }
            }
        }
    }
    
    file.close();
    
    // 重置缓存索引
    current_index_ = 0;
    
    ESP_LOGI(TAG, "✅ 预加载完成: %d/%d 条单词", loaded, cache_size_);
    ESP_LOGI(TAG, "   - 当前SD卡位置: 第 %d 行", current_line_);
    ESP_LOGI(TAG, "========================================");
    
    return loaded > 0;
}

WordEntry* WordBookCache::getCurrentWord() {
    if (!is_initialized_ || !cache_ || current_index_ < 0 || current_index_ >= cache_size_) {
        return nullptr;
    }
    
    return &cache_[current_index_];
}

bool WordBookCache::moveNext() {
    if (!is_initialized_) {
        return false;
    }
    
    current_index_++;
    
    // 如果缓存已读完，预加载下一批
    if (current_index_ >= cache_size_) {
        ESP_LOGI(TAG, "缓存已读完，预加载下一批...");
        return preloadNextBatch();
    }
    
    return true;
}

void WordBookCache::reset() {
    current_line_ = 1;  // 从第1行开始（跳过标题）
    current_index_ = 0;
    
    if (is_initialized_) {
        preloadNextBatch();
    }
}

// ============ 便捷函数 ============

bool initWordBookCache(const char* csv_path) {
    return g_wordbook_cache.init(csv_path, WORDBOOK_PRELOAD_SIZE);
}

WordEntry* getNextWord() {
    WordEntry* current = g_wordbook_cache.getCurrentWord();
    if (current) {
        g_wordbook_cache.moveNext();
    }
    return current;
}

// ============ WordBookCache 类的私有工具函数实现 ============

int WordBookCache::countLines(File &file) {
  int count = 0;
  file.seek(0);
  while (file.available()) {
    if (file.read() == '\n') count++;
  }
  file.seek(0);
  return count;
}

void WordBookCache::parseCSVLine(String line, WordEntry &entry) {
  int fieldCount = 0;
  String field = "";
  bool inQuotes = false;
  char lastChar = 0;
  
  for (int i = 0; i < line.length(); i++) {
    char c = line[i];
    
    if (lastChar != '\\' && c == '"') {
      inQuotes = !inQuotes;
    } else if (c == ',' && !inQuotes) {
      // 字段结束
      assignField(fieldCount, field, entry);
      field = "";
      fieldCount++;
    } else {
      field += c;
    }
    lastChar = c;
  }
  
  // 处理最后一个字段
  if (fieldCount < 5) {
    assignField(fieldCount, field, entry);
  }
}

void WordBookCache::assignField(int fieldCount, String &field, WordEntry &entry) {
  // 移除字段两端的引号（如果存在）
  if (field.length() >= 2 && field[0] == '"' && field[field.length()-1] == '"') {
    field = field.substring(1, field.length()-1);
  }
  
  switch (fieldCount) {
    case 0: 
      // 清理单词：去除前导单引号、减号和其他特殊字符
      entry.word = field; 
      entry.word.trim();
      // 循环去除前导的特殊字符（单引号、减号等）
      while (entry.word.length() > 0 && 
             (entry.word[0] == '\'' || entry.word[0] == '-')) {
        entry.word = entry.word.substring(1);
      }
      break;
    case 1: entry.phonetic = field; break;
    case 2: entry.definition = field; break;
    case 3: 
      // translation 字段：保留原始内容（包括\n），由后续代码自行处理
      entry.translation = field;
      entry.translation.trim();
      break;
    case 4: entry.pos = field; break;
  }
}

/**
 * @brief 从翻译字段中提取前N个释义
 * @param translation 完整的翻译文本（可能包含多行，用\n分隔）
 * @param count 需要提取的释义数量
 * @return 提取后的文本
 */
String WordBookCache::extractFirstNMeanings(const String& translation, int count) {
  if (translation.length() == 0 || count <= 0) {
    return "";
  }
  
  String result = "";
  int meaningCount = 0;
  int startPos = 0;
  
  // 查找每一行（以字面字符串"\n"分隔，注意这是两个字符：反斜杠+n）
  for (int i = 0; i < translation.length() && meaningCount < count; i++) {
    // 检查是否是字面的"\n"（两个字符）
    bool isNewline = false;
    if (i < translation.length() - 1 && translation[i] == '\\' && translation[i+1] == 'n') {
      isNewline = true;
    }
    
    if (isNewline || i == translation.length() - 1) {
      // 提取一行
      int endPos = isNewline ? i : (i + 1);
      String line = translation.substring(startPos, endPos);
      line.trim();  // 去除首尾空格
      
      // 只过滤掉空行，保留所有标记（包括 [网络]、[医]、[化] 等）
      if (line.length() > 0) {
        if (result.length() > 0) {
          result += " ";  // 用空格分隔多个释义，而不是换行符
        }
        result += line;
        meaningCount++;
      }
      
      if (isNewline) {
        startPos = i + 2;  // 跳过"\n"（两个字符）
        i++;  // 额外跳过'n'
      }
    }
  }
  
  // 如果没有找到有效释义，返回原始文本的前部分
  if (meaningCount == 0 && translation.length() > 0) {
    // 查找字面的"\n"
    int cutPos = translation.indexOf("\\n");
    if (cutPos > 0 && cutPos < 100) {
      return translation.substring(0, cutPos);
    } else if (translation.length() > 100) {
      return translation.substring(0, 100) + "...";
    } else {
      return translation;
    }
  }
  
  return result;
}

// ============ 调试/测试函数 ============

/**
 * @brief 从缓存中读取指定数量的单词并打印到串口
 * @param count 要读取的单词数量
 */
void printWordsFromCache(int count) {
    ESP_LOGI(TAG, "========== 开始读取缓存单词 ==========");
    
    if (!g_wordbook_cache.isInitialized()) {
        ESP_LOGE(TAG, "❌ 单词本缓存未初始化，请先调用 initWordBookCache()");
        return;
    }
    
    ESP_LOGI(TAG, "✅ 缓存已初始化");
    ESP_LOGI(TAG, "   - 缓存大小: %d 条", g_wordbook_cache.getCacheSize());
    ESP_LOGI(TAG, "   - 总行数: %d 行", g_wordbook_cache.getTotalLines());
    ESP_LOGI(TAG, "   - 当前行号: %d", g_wordbook_cache.getCurrentLine());
    ESP_LOGI(TAG, "");
    
    int success_count = 0;
    
    for (int i = 0; i < count; i++) {
        WordEntry* word = getNextWord();
        
        if (!word) {
            ESP_LOGE(TAG, "❌ 获取第 %d 个单词失败", i + 1);
            break;
        }
        
        // 打印单词信息
        ESP_LOGI(TAG, "━━━━━━━━━━━━━━ 单词 %d/%d ━━━━━━━━━━━━━━", i + 1, count);
        ESP_LOGI(TAG, "📖 Word:       %s", word->word.c_str());
        
        if (word->phonetic.length() > 0) {
            ESP_LOGI(TAG, "🔊 Phonetic:   /%s/", word->phonetic.c_str());
        }
        
        if (word->definition.length() > 0) {
            ESP_LOGI(TAG, "📝 Definition: %s", word->definition.c_str());
        }
        
        if (word->translation.length() > 0) {
            // 处理多行翻译
            String trans = word->translation;
            trans.replace("\n", " | ");  // 用 | 分隔多个释义
            ESP_LOGI(TAG, "🇨🇳 Translation: %s", trans.c_str());
        }
        
        if (word->pos.length() > 0) {
            ESP_LOGI(TAG, "📌 POS:        %s", word->pos.c_str());
        }
        
        ESP_LOGI(TAG, "");
        success_count++;
        
        // 避免刷屏太快，稍微延迟
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "========== 读取完成 ==========");
    ESP_LOGI(TAG, "✅ 成功读取: %d/%d 个单词", success_count, count);
    ESP_LOGI(TAG, "   - 当前行号: %d", g_wordbook_cache.getCurrentLine());
    ESP_LOGI(TAG, "   - 总行数: %d", g_wordbook_cache.getTotalLines());
}

// ================== 单词本文本缓存管理函数实现 ==================

/**
 * @brief 初始化单词本文本缓存（开机时调用）
 * @return true 成功，false 失败
 */
bool initWordBookTextCache() {
    if (g_wordbook_text_initialized) {
        ESP_LOGW(TAG, "单词本文本缓存已初始化");
        return true;
    }
    
    ESP_LOGI(TAG, "========== 初始化单词本文本缓存 ==========");
    
    // 1. 初始化单词本缓存（从SD卡加载）
    if (!initWordBookCache("/ecdict.mini.csv")) {
        ESP_LOGE(TAG, "❌ 单词本缓存初始化失败");
        return false;
    }
    
    // 2. 加载前WORDBOOK_CACHE_COUNT个单词并分别格式化各个字段
    int loaded_count = 0;
    for (int i = 0; i < WORDBOOK_CACHE_COUNT; i++) {
        WordEntry* word = getNextWord();
        if (!word) {
            ESP_LOGW(TAG, "获取单词%d失败", i);
            break;
        }
        
        // === 分配并格式化：单词本身 ===
        int word_len = word->word.length() + 10;
        g_wordbook_word_cache[i] = (char*)heap_caps_malloc(word_len, MALLOC_CAP_SPIRAM);
        if (g_wordbook_word_cache[i]) {
            snprintf(g_wordbook_word_cache[i], word_len, "%s", word->word.c_str());
            g_wordbook_word_ptrs[i] = g_wordbook_word_cache[i];
        }
        
        // === 分配并格式化：音标（带方括号） ===
        int phonetic_len = word->phonetic.length() + 10;
        g_wordbook_phonetic_cache[i] = (char*)heap_caps_malloc(phonetic_len, MALLOC_CAP_SPIRAM);
        if (g_wordbook_phonetic_cache[i]) {
            if (word->phonetic.length() > 0) {
                snprintf(g_wordbook_phonetic_cache[i], phonetic_len, "[%s]", word->phonetic.c_str());
            } else {
                g_wordbook_phonetic_cache[i][0] = '\0';  // 设置为空字符串
            }
            g_wordbook_phonetic_ptrs[i] = g_wordbook_phonetic_cache[i];
        }
        
        // === 分配并格式化：翻译（按\n分割，提取前两个释义） ===
        String trans_full = word->translation;
        trans_full.trim();
        
        // 完整翻译
        int trans_len = trans_full.length() + 10;
        g_wordbook_translation_cache[i] = (char*)heap_caps_malloc(trans_len, MALLOC_CAP_SPIRAM);
        if (g_wordbook_translation_cache[i]) {
            snprintf(g_wordbook_translation_cache[i], trans_len, "%s", trans_full.c_str());
            g_wordbook_translation_ptrs[i] = g_wordbook_translation_cache[i];
        }
        
        // 按 \n 分割（注意：CSV中是字面量反斜杠+n，在String中需要查找两个连续字符）
        String trans1 = "";
        String trans2 = "";
        
        // 手动查找 反斜杠+n 的位置
        int first_newline = -1;
        for (int j = 0; j < trans_full.length() - 1; j++) {
            if (trans_full[j] == '\\' && trans_full[j+1] == 'n') {
                first_newline = j;
                break;
            }
        }
        
        // 调试：打印原始翻译内容
        ESP_LOGI(TAG, "原始翻译[%d]: [%s], 长度:%d, 首个\\n位置:%d", 
                 i, trans_full.c_str(), trans_full.length(), first_newline);
        
        if (first_newline != -1) {
            // 提取第一个释义
            trans1 = trans_full.substring(0, first_newline);
            trans1.trim();
            
            // 查找第二个 \n
            int second_newline = -1;
            for (int j = first_newline + 2; j < trans_full.length() - 1; j++) {
                if (trans_full[j] == '\\' && trans_full[j+1] == 'n') {
                    second_newline = j;
                    break;
                }
            }
            
            if (second_newline != -1) {
                // 提取第二个释义
                trans2 = trans_full.substring(first_newline + 2, second_newline);
                trans2.trim();
            } else {
                // 只有一个\n，第二个释义是剩余部分
                trans2 = trans_full.substring(first_newline + 2);
                trans2.trim();
            }
        } else {
            // 没有\n，整个作为第一个释义
            trans1 = trans_full;
            trans2 = "";
        }
        
        // 分配第一个释义
        if (trans1.length() > 0) {
            int trans1_len = trans1.length() + 10;
            g_wordbook_translation1_cache[i] = (char*)heap_caps_malloc(trans1_len, MALLOC_CAP_SPIRAM);
            if (g_wordbook_translation1_cache[i]) {
                snprintf(g_wordbook_translation1_cache[i], trans1_len, "%s", trans1.c_str());
                g_wordbook_translation1_ptrs[i] = g_wordbook_translation1_cache[i];
            }
        } else {
            g_wordbook_translation1_cache[i] = (char*)heap_caps_malloc(10, MALLOC_CAP_SPIRAM);
            if (g_wordbook_translation1_cache[i]) {
                snprintf(g_wordbook_translation1_cache[i], 10, "-");
                g_wordbook_translation1_ptrs[i] = g_wordbook_translation1_cache[i];
            }
        }
        
        // 分配第二个释义
        if (trans2.length() > 0) {
            int trans2_len = trans2.length() + 10;
            g_wordbook_translation2_cache[i] = (char*)heap_caps_malloc(trans2_len, MALLOC_CAP_SPIRAM);
            if (g_wordbook_translation2_cache[i]) {
                snprintf(g_wordbook_translation2_cache[i], trans2_len, "%s", trans2.c_str());
                g_wordbook_translation2_ptrs[i] = g_wordbook_translation2_cache[i];
            }
        } else {
            g_wordbook_translation2_cache[i] = (char*)heap_caps_malloc(10, MALLOC_CAP_SPIRAM);
            if (g_wordbook_translation2_cache[i]) {
                snprintf(g_wordbook_translation2_cache[i], 10, "-");
                g_wordbook_translation2_ptrs[i] = g_wordbook_translation2_cache[i];
            }
        }
        
        // === 分配并格式化：词性 ===
        int pos_len = word->pos.length() + 20;
        g_wordbook_pos_cache[i] = (char*)heap_caps_malloc(pos_len, MALLOC_CAP_SPIRAM);
        if (g_wordbook_pos_cache[i]) {
            if (word->pos.length() > 0) {
                snprintf(g_wordbook_pos_cache[i], pos_len, "(%s)", word->pos.c_str());
            } else {
                g_wordbook_pos_cache[i][0] = '\0';  // 设置为空字符串
            }
            g_wordbook_pos_ptrs[i] = g_wordbook_pos_cache[i];
        }
        
        loaded_count++;
        
        ESP_LOGI(TAG, "  [%d] %s %s - %s", i, 
                 g_wordbook_word_ptrs[i], 
                 g_wordbook_phonetic_ptrs[i],
                 g_wordbook_pos_ptrs[i] ? g_wordbook_pos_ptrs[i] : "(no pos)");
    }
    
    // === 第二遍：为所有单词生成错误翻译 ===
    if (loaded_count > 1) {
        ESP_LOGI(TAG, "========== 生成错误翻译 ==========");
        for (int i = 0; i < loaded_count; i++) {
            // 随机选择另一个单词的翻译作为错误答案
            int wrong_index = (i + 1 + (esp_random() % (loaded_count - 1))) % loaded_count;
            
            // 使用已经加载的翻译缓存
            if (g_wordbook_translation_cache[wrong_index]) {
                int wrong_len = strlen(g_wordbook_translation_cache[wrong_index]) + 10;
                g_wordbook_wrong_translation_cache[i] = (char*)heap_caps_malloc(wrong_len, MALLOC_CAP_SPIRAM);
                if (g_wordbook_wrong_translation_cache[i]) {
                    snprintf(g_wordbook_wrong_translation_cache[i], wrong_len, "%s", g_wordbook_translation_cache[wrong_index]);
                    g_wordbook_wrong_translation_ptrs[i] = g_wordbook_wrong_translation_cache[i];
                    ESP_LOGI(TAG, "  [%d] 错误翻译: %s (来自单词%d: %s)", 
                             i, g_wordbook_wrong_translation_cache[i], 
                             wrong_index, g_wordbook_word_cache[wrong_index]);
                }
            } else {
                // 如果获取失败，使用默认错误翻译
                g_wordbook_wrong_translation_cache[i] = (char*)heap_caps_malloc(20, MALLOC_CAP_SPIRAM);
                if (g_wordbook_wrong_translation_cache[i]) {
                    snprintf(g_wordbook_wrong_translation_cache[i], 20, "错误%d", i);
                    g_wordbook_wrong_translation_ptrs[i] = g_wordbook_wrong_translation_cache[i];
                }
            }
        }
    }
    
    // === 第三遍：随机生成选项1和选项2（正确翻译随机出现在任一选项） ===
    if (loaded_count > 0) {
        ESP_LOGI(TAG, "========== 随机分配选项 ==========");
        for (int i = 0; i < loaded_count; i++) {
            // 随机决定正确翻译的位置（0=选项1, 1=选项2）
            bool correct_in_option1 = (esp_random() % 2) == 0;
            
            const char* option1_src = correct_in_option1 ? 
                g_wordbook_translation1_ptrs[i] : g_wordbook_wrong_translation_ptrs[i];
            const char* option2_src = correct_in_option1 ? 
                g_wordbook_wrong_translation_ptrs[i] : g_wordbook_translation1_ptrs[i];
            
            // 分配选项1
            if (option1_src) {
                int len1 = strlen(option1_src) + 10;
                g_wordbook_option1_cache[i] = (char*)heap_caps_malloc(len1, MALLOC_CAP_SPIRAM);
                if (g_wordbook_option1_cache[i]) {
                    snprintf(g_wordbook_option1_cache[i], len1, "%s", option1_src);
                    g_wordbook_option1_ptrs[i] = g_wordbook_option1_cache[i];
                }
            }
            
            // 分配选项2
            if (option2_src) {
                int len2 = strlen(option2_src) + 10;
                g_wordbook_option2_cache[i] = (char*)heap_caps_malloc(len2, MALLOC_CAP_SPIRAM);
                if (g_wordbook_option2_cache[i]) {
                    snprintf(g_wordbook_option2_cache[i], len2, "%s", option2_src);
                    g_wordbook_option2_ptrs[i] = g_wordbook_option2_cache[i];
                }
            }
            
            ESP_LOGI(TAG, "  [%d] 正确翻译在%s: 选项1=[%s], 选项2=[%s]", 
                     i, correct_in_option1 ? "选项1" : "选项2",
                     g_wordbook_option1_ptrs[i] ? g_wordbook_option1_ptrs[i] : "NULL",
                     g_wordbook_option2_ptrs[i] ? g_wordbook_option2_ptrs[i] : "NULL");
        }
    }
    
    if (loaded_count > 0) {
        g_wordbook_text_initialized = true;
        ESP_LOGI(TAG, "✅ 单词本文本缓存初始化成功：%d/%d 个单词", 
                 loaded_count, WORDBOOK_CACHE_COUNT);
        
        #if CONFIG_ESP32S3_SPIRAM_SUPPORT || CONFIG_SPIRAM
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "   PSRAM剩余: %u 字节", free_psram);
        #endif
        
        return true;
    } else {
        ESP_LOGE(TAG, "❌ 未能加载任何单词");
        return false;
    }
}

/**
 * @brief 释放单词本文本缓存（关机时调用）
 */
void freeWordBookTextCache() {
    if (!g_wordbook_text_initialized) return;
    
    for (int i = 0; i < WORDBOOK_CACHE_COUNT; i++) {
        if (g_wordbook_word_cache[i]) {
            heap_caps_free(g_wordbook_word_cache[i]);
            g_wordbook_word_cache[i] = nullptr;
            g_wordbook_word_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_phonetic_cache[i]) {
            heap_caps_free(g_wordbook_phonetic_cache[i]);
            g_wordbook_phonetic_cache[i] = nullptr;
            g_wordbook_phonetic_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_translation_cache[i]) {
            heap_caps_free(g_wordbook_translation_cache[i]);
            g_wordbook_translation_cache[i] = nullptr;
            g_wordbook_translation_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_translation1_cache[i]) {
            heap_caps_free(g_wordbook_translation1_cache[i]);
            g_wordbook_translation1_cache[i] = nullptr;
            g_wordbook_translation1_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_translation2_cache[i]) {
            heap_caps_free(g_wordbook_translation2_cache[i]);
            g_wordbook_translation2_cache[i] = nullptr;
            g_wordbook_translation2_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_pos_cache[i]) {
            heap_caps_free(g_wordbook_pos_cache[i]);
            g_wordbook_pos_cache[i] = nullptr;
            g_wordbook_pos_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_wrong_translation_cache[i]) {
            heap_caps_free(g_wordbook_wrong_translation_cache[i]);
            g_wordbook_wrong_translation_cache[i] = nullptr;
            g_wordbook_wrong_translation_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_option1_cache[i]) {
            heap_caps_free(g_wordbook_option1_cache[i]);
            g_wordbook_option1_cache[i] = nullptr;
            g_wordbook_option1_ptrs[i] = nullptr;
        }
        
        if (g_wordbook_option2_cache[i]) {
            heap_caps_free(g_wordbook_option2_cache[i]);
            g_wordbook_option2_cache[i] = nullptr;
            g_wordbook_option2_ptrs[i] = nullptr;
        }
    }
    
    g_wordbook_text_initialized = false;
    ESP_LOGI(TAG, "单词本文本缓存已释放");
}

/**
 * @brief 获取单词本身
 */
const char* getWordBookWord(int index) {
    if (!g_wordbook_text_initialized) return "Not Init";
    if (index < 0 || index >= WORDBOOK_CACHE_COUNT) return "ERR";
    if (!g_wordbook_word_ptrs[index]) return "NULL";
    return g_wordbook_word_ptrs[index];
}

/**
 * @brief 获取单词音标
 */
const char* getWordBookPhonetic(int index) {
    if (!g_wordbook_text_initialized) return "";
    if (index < 0 || index >= WORDBOOK_CACHE_COUNT) return "";
    if (!g_wordbook_phonetic_ptrs[index]) return "";
    return g_wordbook_phonetic_ptrs[index];
}

/**
 * @brief 获取单词翻译（完整）
 */
const char* getWordBookTranslation(int index) {
    if (!g_wordbook_text_initialized) return "Not Init";
    if (index < 0 || index >= WORDBOOK_CACHE_COUNT) return "ERR";
    if (!g_wordbook_translation_ptrs[index]) return "NULL";
    return g_wordbook_translation_ptrs[index];
}

/**
 * @brief 获取单词第一个释义
 */
const char* getWordBookTranslation1(int index) {
    if (!g_wordbook_text_initialized) return "";
    if (index < 0 || index >= WORDBOOK_CACHE_COUNT) return "";
    if (!g_wordbook_translation1_ptrs[index]) return "-";
    return g_wordbook_translation1_ptrs[index];
}

/**
 * @brief 获取单词第二个释义
 */
const char* getWordBookTranslation2(int index) {
    if (!g_wordbook_text_initialized) return "";
    if (index < 0 || index >= WORDBOOK_CACHE_COUNT) return "";
    if (!g_wordbook_translation2_ptrs[index]) return "-";
    return g_wordbook_translation2_ptrs[index];
}

/**
 * @brief 获取单词词性
 */
const char* getWordBookPos(int index) {
    if (!g_wordbook_text_initialized) return "";
    if (index < 0 || index >= WORDBOOK_CACHE_COUNT) return "";
    if (!g_wordbook_pos_ptrs[index]) return "";
    return g_wordbook_pos_ptrs[index];
}

/**
 * @brief 获取错误翻译（用于测试）
 */
const char* getWordBookWrongTranslation(int index) {
    if (!g_wordbook_text_initialized) return "";
    if (index < 0 || index >= WORDBOOK_CACHE_COUNT) return "";
    if (!g_wordbook_wrong_translation_ptrs[index]) return "";
    return g_wordbook_wrong_translation_ptrs[index];
}

/**
 * @brief 获取单词本文本指针（用于text_roll）
 * @param index 单词索引 (0-4)
 * @return 单词文本指针，失败返回"ERR"
 * @deprecated 请使用 getWordBookWord/Phonetic/Translation/Pos
 */
const char* getWordBookText(int index) {
    return getWordBookWord(index);  // 默认返回单词本身
}

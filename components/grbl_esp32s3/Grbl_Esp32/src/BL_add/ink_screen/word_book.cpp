#include "word_book.h"
#include <FS.h>
#include "esp_heap_caps.h"
#include "../../Grbl.h"

WordEntry entry;
// 休眠模式显示的数据
WordEntry sleep_mode_entry;
bool has_sleep_data = false;
static const char *TAG = "word_book.cpp";

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
    case 0: entry.word = field; break;
    case 1: entry.phonetic = field; break;
    case 2: entry.definition = field; break;
    case 3: 
      // translation 字段：只保留前2个释义
      entry.translation = extractFirstNMeanings(field, 2);
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
  
  // 查找每一行（以\n分隔）
  for (int i = 0; i < translation.length() && meaningCount < count; i++) {
    if (translation[i] == '\n' || i == translation.length() - 1) {
      // 提取一行
      int endPos = (i == translation.length() - 1) ? i + 1 : i;
      String line = translation.substring(startPos, endPos);
      line.trim();  // 去除首尾空格
      
      // 只过滤掉空行，保留所有标记（包括 [网络]、[医]、[化] 等）
      if (line.length() > 0) {
        if (result.length() > 0) {
          result += "\n";  // 添加换行符
        }
        result += line;
        meaningCount++;
      }
      
      startPos = i + 1;
    }
  }
  
  // 如果没有找到有效释义，返回原始文本的前部分
  if (meaningCount == 0 && translation.length() > 0) {
    int cutPos = translation.indexOf('\n');
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

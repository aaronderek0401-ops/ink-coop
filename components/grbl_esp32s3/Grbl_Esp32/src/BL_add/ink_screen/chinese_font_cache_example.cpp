/**
 * @file chinese_font_cache_example.cpp
 * @brief 中文字库混合缓存系统使用示例
 */

#include "chinese_font_cache.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "FontCacheExample";

// ============ 示例1: 基础初始化 ============
void example_basic_init() {
    ESP_LOGI(TAG, "=== Example 1: Basic Initialization ===");
    
    // 获取单例
    ChineseFontCache& cache = getFontCache();
    
    // 初始化缓存系统
    bool success = cache.init("/sd/fangsong_gb2312_16x16.bin", true);
    if (!success) {
        ESP_LOGE(TAG, "Failed to init font cache");
        return;
    }
    
    // 加载常用字到PSRAM缓存
    int loaded = cache.loadCommonCharacters();
    ESP_LOGI(TAG, "Loaded %d common characters", loaded);
    
    // 打印状态
    cache.printStatus();
}

// ============ 示例2: 显示单个汉字 ============
void example_display_single_char() {
    ESP_LOGI(TAG, "=== Example 2: Display Single Character ===");
    
    uint8_t glyph_buffer[32];  // 16x16字模需要32字节
    
    // 显示"你"字 (Unicode: 0x4F60)
    bool success = getChineseChar(0x4F60, FONT_16x16, glyph_buffer);
    
    if (success) {
        ESP_LOGI(TAG, "Got glyph for '你' (0x4F60)");
        
        // TODO: 将glyph_buffer发送到墨水屏或OLED显示
        // displayGlyphOnScreen(x, y, glyph_buffer);
        
    } else {
        ESP_LOGE(TAG, "Failed to get glyph");
    }
}

// ============ 示例3: 显示一段文本 ============
void example_display_text() {
    ESP_LOGI(TAG, "=== Example 3: Display Text ===");
    
    const char* text = "你好,世界!";
    uint8_t glyph_buffer[32];
    
    int x = 0, y = 0;
    const uint8_t* p = (const uint8_t*)text;
    
    while (*p) {
        uint16_t unicode = 0;
        
        // UTF-8解码
        if ((*p & 0xF0) == 0xE0) {
            // 3字节UTF-8 (中文)
            unicode = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
            p += 3;
        } else if ((*p & 0x80) == 0) {
            // ASCII
            unicode = *p;
            p++;
        } else {
            p++;
            continue;
        }
        
        // 获取字模
        if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
            if (getChineseChar(unicode, FONT_16x16, glyph_buffer)) {
                // TODO: 显示到屏幕
                // displayGlyphOnScreen(x, y, glyph_buffer);
                ESP_LOGD(TAG, "Display char: 0x%04X at (%d, %d)", unicode, x, y);
                x += 16;  // 下一个字符位置
            }
        }
    }
}

// ============ 示例4: 电子书阅读模式 ============
void example_ebook_reading() {
    ESP_LOGI(TAG, "=== Example 4: E-book Reading ===");
    
    ChineseFontCache& cache = getFontCache();
    
    // 模拟电子书内容
    const char* pages[] = {
        "第一章 开始\n这是一个关于学习的故事。",
        "第二章 成长\n每天都有新的收获和进步。",
        "第三章 总结\n坚持是成功的关键。"
    };
    
    int current_page = 0;
    
    // 预加载当前页
    ESP_LOGI(TAG, "Loading page %d...", current_page);
    int preloaded = cache.preloadPage(pages[current_page], FONT_16x16);
    ESP_LOGI(TAG, "Preloaded %d characters", preloaded);
    
    // 显示当前页
    // TODO: 实际显示逻辑
    ESP_LOGI(TAG, "Displaying: %s", pages[current_page]);
    
    // 后台预加载下一页
    if (current_page + 1 < 3) {
        ESP_LOGI(TAG, "Preloading next page in background...");
        cache.preloadPage(pages[current_page + 1], FONT_16x16);
    }
    
    // 翻页
    current_page++;
    
    // 清除旧页面缓存(可选,节省内存)
    // cache.clearPageCache();
    
    // 打印缓存统计
    cache.printStatus();
}

// ============ 示例5: 单词卡片显示 ============
void example_word_card() {
    ESP_LOGI(TAG, "=== Example 5: Word Card Display ===");
    
    ChineseFontCache& cache = getFontCache();
    
    // 模拟单词数据
    struct Word {
        const char* english;
        const char* chinese;
        const char* example;
    };
    
    Word words[] = {
        {"abandon", "放弃", "Don't abandon hope."},
        {"ability", "能力", "He has the ability to succeed."},
        {"knowledge", "知识", "Knowledge is power."}
    };
    
    // 随机显示一个单词
    int random_idx = 1;  // 实际应该用随机数
    Word& word = words[random_idx];
    
    ESP_LOGI(TAG, "Word: %s", word.english);
    ESP_LOGI(TAG, "Translation: %s", word.chinese);
    
    // 预加载这个单词的所有汉字
    int preloaded = cache.preloadWord(word.chinese, FONT_16x16);
    ESP_LOGI(TAG, "Preloaded %d characters for word", preloaded);
    
    // 显示单词
    uint8_t glyph_buffer[32];
    
    // 显示中文释义
    const uint8_t* p = (const uint8_t*)word.chinese;
    int x = 0;
    
    while (*p) {
        if ((*p & 0xF0) == 0xE0) {
            uint16_t unicode = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
            
            if (getChineseChar(unicode, FONT_16x16, glyph_buffer)) {
                // TODO: 显示到屏幕
                ESP_LOGD(TAG, "Display: 0x%04X", unicode);
            }
            
            p += 3;
        } else {
            p++;
        }
    }
}

// ============ 示例6: 自定义常用字列表 ============
void example_custom_common_chars() {
    ESP_LOGI(TAG, "=== Example 6: Custom Common Chars ===");
    
    ChineseFontCache& cache = getFontCache();
    
    // 自定义常用字列表(例如:只缓存单词本最常用的100个汉字)
    const uint16_t custom_chars[] = {
        0x7684, 0x4E00, 0x662F, 0x5728, 0x4EBA,  // 的一是在人
        0x6709, 0x4E2A, 0x4E0A, 0x4E2D, 0x5927,  // 有个上中大
        // ... 添加更多常用字
    };
    
    int count = sizeof(custom_chars) / sizeof(uint16_t);
    
    // 设置自定义列表
    cache.setCommonCharList(custom_chars, count);
    
    // 加载到缓存
    int loaded = cache.loadCommonCharacters();
    ESP_LOGI(TAG, "Loaded %d custom common chars", loaded);
}

// ============ 示例7: 性能测试 ============
void example_performance_test() {
    ESP_LOGI(TAG, "=== Example 7: Performance Test ===");
    
    ChineseFontCache& cache = getFontCache();
    uint8_t glyph_buffer[32];
    
    // 测试文本
    const uint16_t test_chars[] = {
        0x4F60, 0x597D, 0x4E16, 0x754C,  // 你好世界
        0x7684, 0x4E00, 0x662F, 0x5728   // 的一是在(常用字)
    };
    
    int test_count = sizeof(test_chars) / sizeof(uint16_t);
    
    // 重置统计
    cache.resetStats();
    
    // 执行1000次查询
    uint32_t start_time = esp_timer_get_time();
    
    for (int i = 0; i < 1000; i++) {
        uint16_t unicode = test_chars[i % test_count];
        getChineseChar(unicode, FONT_16x16, glyph_buffer);
    }
    
    uint32_t end_time = esp_timer_get_time();
    uint32_t elapsed_us = end_time - start_time;
    
    ESP_LOGI(TAG, "1000 queries in %d us (%.2f us per char)", 
             elapsed_us, (float)elapsed_us / 1000);
    
    // 打印详细统计
    cache.printStatus();
    CacheStats stats = cache.getStats();
    ESP_LOGI(TAG, "Cache hit rate: %.1f%%", stats.hit_rate);
}

// ============ 示例8: 内存监控 ============
void example_memory_monitor() {
    ESP_LOGI(TAG, "=== Example 8: Memory Monitor ===");
    
    ChineseFontCache& cache = getFontCache();
    
    // 显示内存使用情况
    uint32_t cache_memory = cache.getMemoryUsage();
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    
    ESP_LOGI(TAG, "Cache memory: %d bytes (%.1f KB)", 
             cache_memory, cache_memory / 1024.0f);
    ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
    ESP_LOGI(TAG, "Free PSRAM: %d bytes", free_psram);
    
    // 获取详细统计
    CacheStats stats = cache.getStats();
    ESP_LOGI(TAG, "Total requests: %d", stats.total_requests);
    ESP_LOGI(TAG, "Memory efficiency: %.2f requests per KB", 
             (float)stats.total_requests / (cache_memory / 1024.0f));
}

// ============ 主函数 - 运行所有示例 ============
extern "C" void app_main_font_cache_examples() {
    ESP_LOGI(TAG, "Starting Font Cache Examples...\n");
    
    // 运行示例
    example_basic_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    example_display_single_char();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    example_display_text();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    example_ebook_reading();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    example_word_card();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    example_performance_test();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    example_memory_monitor();
    
    ESP_LOGI(TAG, "\nAll examples completed!");
}

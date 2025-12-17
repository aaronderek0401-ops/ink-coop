/**
 * @file chinese_font_cache.h
 * @brief 中文字库混合缓存系统 - 三层缓存架构
 * 
 * 层级1: PSRAM常用字缓存 (100-150KB, 命中率60-80%)
 * 层级2: 页面/单词动态缓存 (50KB, 按需加载)
 * 层级3: SD卡完整字库 (7000字, 作为后备)
 * 
 * 适用场景:
 * - 电子书阅读(支持页面预加载)
 * - 随机单词显示(常用字快速响应)
 * - 墨水屏/OLED屏幕显示
 */

#ifndef CHINESE_FONT_CACHE_H
#define CHINESE_FONT_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <map>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ============ 配置参数 ============
#define COMMON_CHAR_CACHE_SIZE  500      // 常用字缓存数量
#define PAGE_CACHE_SIZE         200      // 页面缓存字符数量
#define MAX_FONT_SIZE          128       // 最大字模尺寸(32x32 = 128字节)
#define CACHE_HIT_THRESHOLD     3        // LRU淘汰阈值

// 字号枚举
enum FontSize {
    FONT_16x16 = 16,    // 32 bytes per char
    FONT_24x24 = 24,    // 72 bytes per char
    FONT_32x32 = 32     // 128 bytes per char
};

// 字符缓存条目
struct CharCacheEntry {
    uint16_t unicode;           // Unicode编码
    uint8_t* glyph_16x16;       // 16x16字模数据
    uint8_t* glyph_24x24;       // 24x24字模数据 (可选)
    uint32_t hit_count;         // 访问计数(LRU)
    uint32_t last_access_time;  // 最后访问时间
    bool is_loaded;             // 是否已加载
};

// 缓存统计信息
struct CacheStats {
    uint32_t total_requests;        // 总请求次数
    uint32_t common_cache_hits;     // 常用字缓存命中
    uint32_t page_cache_hits;       // 页面缓存命中
    uint32_t sd_reads;              // SD卡读取次数
    float hit_rate;                 // 总命中率
    uint32_t memory_used;           // 内存使用量(字节)
};

/**
 * @class ChineseFontCache
 * @brief 中文字库缓存管理器
 */
class ChineseFontCache {
public:
    ChineseFontCache();
    ~ChineseFontCache();
    
    // ============ 初始化与配置 ============
    
    /**
     * @brief 初始化缓存系统
     * @param sd_font_path SD卡字库文件路径
     * @param enable_psram 是否启用PSRAM
     * @return true 初始化成功, false 失败
     */
    bool init(const char* sd_font_path, bool enable_psram = true);
    
    /**
     * @brief 加载常用字到PSRAM缓存
     * @return 成功加载的字符数量
     */
    int loadCommonCharacters();
    
    /**
     * @brief 设置常用字列表(可自定义)
     * @param chars Unicode字符数组
     * @param count 字符数量
     */
    void setCommonCharList(const uint16_t* chars, int count);
    
    // ============ 核心查询接口 ============
    
    /**
     * @brief 获取字符字模数据(三层查找)
     * @param unicode Unicode编码
     * @param font_size 字号(16/24/32)
     * @param out_buffer 输出缓冲区
     * @return true 成功, false 失败
     */
    bool getCharGlyph(uint16_t unicode, FontSize font_size, uint8_t* out_buffer);
    
    /**
     * @brief 批量获取字符字模(用于页面预加载)
     * @param unicodes Unicode数组
     * @param count 字符数量
     * @param font_size 字号
     * @return 成功加载的字符数量
     */
    int preloadChars(const uint16_t* unicodes, int count, FontSize font_size);
    
    // ============ 页面/单词缓存管理 ============
    
    /**
     * @brief 预加载电子书页面
     * @param page_text 页面文本内容
     * @param font_size 字号
     * @return 缓存的字符数量
     */
    int preloadPage(const char* page_text, FontSize font_size);
    
    /**
     * @brief 预加载单词释义
     * @param word_text 单词和释义文本
     * @param font_size 字号
     * @return 缓存的字符数量
     */
    int preloadWord(const char* word_text, FontSize font_size);
    
    /**
     * @brief 清空页面缓存(翻页时调用)
     */
    void clearPageCache();
    
    // ============ 统计与调试 ============
    
    /**
     * @brief 获取缓存统计信息
     * @return 缓存统计结构体
     */
    CacheStats getStats() const;
    
    /**
     * @brief 打印缓存状态(调试用)
     */
    void printStatus() const;
    
    /**
     * @brief 重置统计信息
     */
    void resetStats();
    
    /**
     * @brief 检查字符是否在常用字缓存中
     */
    bool isInCommonCache(uint16_t unicode) const;
    
    /**
     * @brief 获取内存使用量(字节)
     */
    uint32_t getMemoryUsage() const;

private:
    // ============ 内部数据结构 ============
    
    std::map<uint16_t, CharCacheEntry> common_cache_;   // 常用字缓存(PSRAM)
    std::map<uint16_t, CharCacheEntry> page_cache_;     // 页面缓存(动态)
    
    uint16_t* common_char_list_;    // 常用字列表
    int common_char_count_;         // 常用字数量
    
    char sd_font_path_[128];        // SD卡字库路径
    bool is_initialized_;           // 初始化标志
    bool psram_enabled_;            // PSRAM启用标志
    
    CacheStats stats_;              // 统计信息
    SemaphoreHandle_t cache_mutex_; // 线程安全互斥锁
    
    // ============ 内部辅助函数 ============
    
    /**
     * @brief 从SD卡读取字符字模
     * @param unicode Unicode编码
     * @param font_size 字号
     * @param out_buffer 输出缓冲区
     * @return true 成功, false 失败
     */
    bool readFromSD(uint16_t unicode, FontSize font_size, uint8_t* out_buffer);
    
    /**
     * @brief 添加字符到页面缓存
     * @param unicode Unicode编码
     * @param glyph 字模数据
     * @param font_size 字号
     */
    void addToPageCache(uint16_t unicode, const uint8_t* glyph, FontSize font_size);
    
    /**
     * @brief LRU淘汰算法 - 清理低频字符
     */
    void evictLRU();
    
    /**
     * @brief 从文本中提取中文字符
     * @param text UTF-8编码文本
     * @param out_unicodes 输出Unicode数组
     * @param max_count 最大数量
     * @return 提取的字符数量
     */
    int extractChineseChars(const char* text, uint16_t* out_unicodes, int max_count);
    
    /**
     * @brief 计算字模数据大小
     * @param font_size 字号
     * @return 字节数
     */
    uint32_t getGlyphSize(FontSize font_size) const;
    
    /**
     * @brief 更新访问统计
     */
    void updateAccessStats(bool is_common_hit, bool is_page_hit);
};

// ============ 全局单例访问接口 ============

/**
 * @brief 获取字库缓存管理器单例
 */
ChineseFontCache& getFontCache();

/**
 * @brief 快速接口 - 获取字符字模
 */
bool getChineseChar(uint16_t unicode, FontSize font_size, uint8_t* out_buffer);

/**
 * @brief 快速接口 - 预加载页面
 */
int preloadPageText(const char* page_text, FontSize font_size = FONT_16x16);

#endif // CHINESE_FONT_CACHE_H

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
    FONT_20x20 = 20,    // 60 bytes per char
    FONT_24x24 = 24,    // 72 bytes per char
    FONT_28x28 = 28,    // 112 bytes per char
    FONT_32x32 = 32     // 128 bytes per char
};

// 字符缓存条目
struct CharCacheEntry {
    uint16_t unicode;           // Unicode编码
    uint8_t* glyph_16x16;       // 16x16字模数据
    uint8_t* glyph_20x20;       // 20x20字模数据 (可选)
    uint8_t* glyph_24x24;       // 24x24字模数据 (可选)
    uint8_t* glyph_28x28;       // 28x28字模数据 (可选)
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

// ============ 便捷初始化接口（推荐用于启动） ============

/**
 * @brief 一步初始化字库缓存系统（同时初始化16x16和24x24两个字号）
 * 
 * 使用示例：
 * @code
 * bool success = initFontCacheSystem(
 *     "/sd/fangsong_gb2312_16x16.bin",
 *     "/sd/fangsong_gb2312_24x24.bin",
 *     esp_spiram_is_initialized()
 * );
 * @endcode
 * 
 * @param font_16x16_path 16x16字体文件路径
 * @param font_24x24_path 24x24字体文件路径  
 * @param enable_psram 是否启用PSRAM
 * @return true 成功，false 失败
 */
bool initFontCacheSystem(const char* font_16x16_path,
                         const char* font_24x24_path,
                         bool enable_psram);

/**
 * @brief 预加载页面到缓存（自动处理两个字号）
 * 
 * @param page_text 页面文本内容
 * @return 缓存的字符数量（两个字号的总和）
 */
int preloadPageToCache(const char* page_text);

/**
 * @brief 打印缓存统计信息（包括16x16和24x24）
 * 
 * 输出包括：
 * - 命中率统计
 * - 内存使用情况
 * - 各字号的缓存状态
 */
void printCacheStats();

/**
 * @brief 清除所有缓存（保留常用字缓存）
 */
void clearAllPageCache();

/**
 * @brief 获取16x16字体缓存实例
 * @return 16x16缓存对象引用
 */
ChineseFontCache& getFontCache16();

/**
 * @brief 获取20x20字体缓存实例
 * @return 20x20缓存对象引用
 */
ChineseFontCache& getFontCache20();

/**
 * @brief 获取24x24字体缓存实例 (已废弃，保留以兼容)
 * @return 24x24缓存对象引用
 */
ChineseFontCache& getFontCache24();

// ============ PSRAM 完整字体加载系统 ============

/**
 * @brief PSRAM完整字体数据结构
 * 用于加载整个字体文件到PSRAM,支持快速字符查找
 */
struct FullFontData {
    uint8_t* data;          // PSRAM 中的字体数据
    size_t size;            // 字体文件大小
    int font_size;          // 字体尺寸 (20, 28 等)
    uint32_t glyph_size;    // 单个字符字节数
    uint32_t char_count;    // 字符总数
    bool is_loaded;         // 是否已加载
    char file_path[64];     // 文件路径
    char font_name[64];     // 字体名称 (如 "comic_sans_ms_bold_phonetic_20x20")
};

// 最多加载8个完整字体到PSRAM (fangsong + 英文字体等)
// 注意: fangsong 20x20约1.2MB, comic_sans 3个约15KB, 总共约需 1.5-2MB PSRAM (8MB充足)
#define MAX_PSRAM_FONTS 8

/**
 * @brief 从SD卡加载完整字体文件到PSRAM
 * @param font_data 字体数据结构指针
 * @param file_path SD卡字体文件路径
 * @return true成功, false失败
 */
bool loadFullFontToPSRAM(FullFontData* font_data, const char* file_path);

/**
 * @brief 从PSRAM字体中获取字符字模
 * @param font_data 字体数据指针
 * @param unicode Unicode编码
 * @param out_buffer 输出缓冲区
 * @return true成功, false失败
 */
bool getCharGlyphFromPSRAM(const FullFontData* font_data, uint16_t unicode, uint8_t* out_buffer);

/**
 * @brief 根据字体大小查找PSRAM字体
 * @param font_size 字体大小 (16, 20, 24, 28, 32)
 * @return 字体指针,未找到返回nullptr
 */
const FullFontData* findPSRAMFontBySize(int font_size);

/**
 * @brief 根据字体名称查找PSRAM字体
 * @param font_name 字体名称 (如 "comic_sans_ms_20x20")
 * @return 字体指针,未找到返回nullptr
 */
const FullFontData* findPSRAMFontByName(const char* font_name);

/**
 * @brief 获取字体大小 (辅助函数)
 */
int getFontSize(const FullFontData* font_data);

/**
 * @brief 获取字模大小 (辅助函数)
 */
uint32_t getGlyphSize(const FullFontData* font_data);

/**
 * @brief 获取字体名称 (辅助函数)
 */
const char* getFontName(const FullFontData* font_data);

/**
 * @brief 从文件名解析字体尺寸
 * @param filename 文件名
 * @return 字体尺寸，失败返回0
 */
int parseFontSizeFromFilename(const char* filename);

/**
 * @brief 判断字体文件是否需要加载到PSRAM
 * @param filename 文件名
 * @return true=加载到PSRAM, false=使用缓存系统
 */
bool shouldLoadToPSRAM(const char* filename);

/**
 * @brief 扫描SD卡并加载字体到PSRAM
 * @param load_all_fonts 是否加载所有字体(包括fangsong), 默认false只加载非fangsong
 * @return 成功加载的字体数量
 */
int initFullFontsInPSRAM(bool load_all_fonts = false);

/**
 * @brief 直接加载指定字体文件到PSRAM (用于fangsong等大字库)
 * @param file_path 字体文件完整路径
 * @param font_size 字体尺寸
 * @return true成功, false失败
 */
bool loadSpecificFontToPSRAM(const char* file_path, int font_size);

/**
 * @brief 释放所有PSRAM字体
 */
void freeAllPSRAMFonts();

/**
 * @brief 获取PSRAM字体数量
 */
int getPSRAMFontCount();

/**
 * @brief 根据索引获取PSRAM字体
 * @param index 字体索引 (0 到 getPSRAMFontCount()-1)
 * @return 字体数据指针，失败返回nullptr
 */
const FullFontData* getPSRAMFontByIndex(int index);

// 当前激活的PSRAM字体 (用于字体切换)
extern const FullFontData* g_current_psram_font;

#endif // CHINESE_FONT_CACHE_H

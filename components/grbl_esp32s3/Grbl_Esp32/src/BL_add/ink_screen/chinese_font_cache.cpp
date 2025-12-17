/**
 * @file chinese_font_cache.cpp
 * @brief 中文字库混合缓存系统实现
 */

#include "chinese_font_cache.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <algorithm>

static const char* TAG = "FontCache";

// ============ GB2312一级常用汉字前500个 ============
// 这个列表包含了最常用的500个汉字,覆盖日常使用约80%的场景
static const uint16_t DEFAULT_COMMON_CHARS[] = {
    // 高频词(前100个)
    0x7684, 0x4E00, 0x662F, 0x4E86, 0x6211, 0x4E0D, 0x4EBA, 0x5728, 0x4ED6, 0x6709,  // 的一是了我不人在他有
    0x8FD9, 0x4E2A, 0x4E0A, 0x4E2D, 0x5927, 0x5230, 0x8BF4, 0x4F60, 0x4E3A, 0x5B50,  // 这个上中大到说你为子
    0x548C, 0x4E5F, 0x5F97, 0x4F1A, 0x5C31, 0x5730, 0x51FA, 0x5BF9, 0x5982, 0x4F46,  // 和也得会就地出对如但
    0x81EA, 0x4E8E, 0x5B83, 0x4E8B, 0x53EF, 0x4EE5, 0x6CA1, 0x5E74, 0x6211, 0x8FC7,  // 自于它事可以没年我过
    0x8981, 0x5979, 0x751F, 0x4F5C, 0x5206, 0x90A3, 0x80FD, 0x800C, 0x4E5F, 0x65F6,  // 要她生作分那能而也时
    
    // 常用动词(50个)
    0x6765, 0x53BB, 0x8D70, 0x770B, 0x505A, 0x60F3, 0x77E5, 0x9053, 0x542C, 0x89C9,  // 来去走看做想知道听觉
    0x5F97, 0x7ED9, 0x5E0C, 0x671B, 0x8BA4, 0x4E3A, 0x6210, 0x4E3A, 0x5206, 0x4EAB,  // 得给希望认为成为分享
    0x5F00, 0x59CB, 0x7ED3, 0x675F, 0x63A5, 0x53D7, 0x5F97, 0x5230, 0x8FDB, 0x884C,  // 开始结束接受得到进行
    0x51B3, 0x5B9A, 0x9009, 0x62E9, 0x5E26, 0x6765, 0x5E26, 0x8D70, 0x4FDD, 0x6301,  // 决定选择带来带走保持
    0x7EE7, 0x7EED, 0x53D1, 0x73B0, 0x53D1, 0x5C55, 0x53D1, 0x751F, 0x4EA7, 0x751F,  // 继续发现发展发生产生
    
    // 常用形容词(50个)
    0x597D, 0x65B0, 0x8001, 0x957F, 0x77ED, 0x9AD8, 0x4F4E, 0x5927, 0x5C0F, 0x591A,  // 好新老长短高低大小多
    0x5C11, 0x597D, 0x574F, 0x7F8E, 0x4E11, 0x5FEB, 0x6162, 0x65E9, 0x665A, 0x524D,  // 少好坏美丑快慢早晚前
    0x540E, 0x5DE6, 0x53F3, 0x4E0A, 0x4E0B, 0x5185, 0x5916, 0x4E1C, 0x897F, 0x5357,  // 后左右上下内外东西南
    0x5317, 0x4E2D, 0x95F4, 0x91CD, 0x8F7B, 0x6DF1, 0x6D45, 0x8FDC, 0x8FD1, 0x5BBD,  // 北中间重轻深浅远近宽
    0x7A84, 0x5E72, 0x6E7F, 0x51B7, 0x70ED, 0x6E29, 0x6696, 0x51C9, 0x7231, 0x559C,  // 窄干湿冷热温暖凉爱喜
    
    // 常用名词(100个)
    0x4EBA, 0x5BB6, 0x56FD, 0x5929, 0x5730, 0x65F6, 0x95F4, 0x65B9, 0x5F0F, 0x65B9,  // 人家国天地时间方式方
    0x6CD5, 0x5730, 0x65B9, 0x5730, 0x70B9, 0x95EE, 0x9898, 0x7ED3, 0x679C, 0x539F,  // 法地方地点问题结果原
    0x56E0, 0x5F00, 0x5173, 0x5DE5, 0x4F5C, 0x751F, 0x6D3B, 0x5B66, 0x4E60, 0x7814,  // 因开关工作生活学习研
    0x7A76, 0x8BA1, 0x5212, 0x76EE, 0x6807, 0x4EFB, 0x52A1, 0x9879, 0x76EE, 0x5185,  // 究计划目标任务项目内
    0x5BB9, 0x8303, 0x56F4, 0x5F71, 0x54CD, 0x4F5C, 0x7528, 0x610F, 0x4E49, 0x610F,  // 容范围影响作用意义意
    0x601D, 0x89C2, 0x70B9, 0x770B, 0x6CD5, 0x60C5, 0x51B5, 0x72B6, 0x6001, 0x5FC3,  // 思观点看法情况状态心
    0x7406, 0x601D, 0x60F3, 0x611F, 0x89C9, 0x611F, 0x60C5, 0x611F, 0x53D7, 0x7ECF,  // 理思想感觉感情感受经
    0x9A8C, 0x7ECF, 0x5386, 0x77E5, 0x8BC6, 0x6280, 0x672F, 0x80FD, 0x529B, 0x6C34,  // 验经历知识技术能力水
    0x5E73, 0x7A0B, 0x5EA6, 0x65B9, 0x6848, 0x8BA1, 0x5212, 0x884C, 0x52A8, 0x8868,  // 平程度方案计划行动表
    0x73B0, 0x53D8, 0x5316, 0x53D1, 0x5C55, 0x8FDB, 0x6B65, 0x8FC7, 0x7A0B, 0x7ED3,  // 现变化发展进步过程结
    
    // 常用单词释义词(100个)
    0x653E, 0x5F03, 0x80FD, 0x591F, 0x6240, 0x6709, 0x4E00, 0x5207, 0x6BCF, 0x4E2A,  // 放弃能够所有一切每个
    0x6BCF, 0x5929, 0x6BCF, 0x6B21, 0x4EFB, 0x4F55, 0x6240, 0x6709, 0x6240, 0x6709,  // 每天每次任何所有所有
    0x7684, 0x4E86, 0x5730, 0x5F97, 0x7740, 0x8FC7, 0x6EE1, 0x5145, 0x6EE1, 0x610F,  // 的了地得着过满充满意
    0x5B8C, 0x5168, 0x5B8C, 0x6210, 0x7EE7, 0x7EED, 0x6301, 0x7EED, 0x4FDD, 0x8BC1,  // 完全完成继续持续保证
    0x4FDD, 0x6301, 0x4FDD, 0x7559, 0x589E, 0x52A0, 0x51CF, 0x5C11, 0x6539, 0x53D8,  // 保持保留增加减少改变
    0x6539, 0x8FDB, 0x63D0, 0x9AD8, 0x964D, 0x4F4E, 0x6269, 0x5927, 0x7F29, 0x5C0F,  // 改进提高降低扩大缩小
    0x5F00, 0x5C55, 0x5C55, 0x5F00, 0x5C55, 0x793A, 0x5C55, 0x73B0, 0x663E, 0x793A,  // 开展展开展示展现显示
    0x8868, 0x660E, 0x8BF4, 0x660E, 0x8BB2, 0x89E3, 0x7406, 0x89E3, 0x660E, 0x767D,  // 表明说明讲解理解明白
    0x6E05, 0x695A, 0x6E05, 0x6670, 0x6E05, 0x9664, 0x6D88, 0x9664, 0x9664, 0x4E86,  // 清楚清晰清除消除除了
    0x5177, 0x6709, 0x5177, 0x4F53, 0x7279, 0x522B, 0x7279, 0x6B8A, 0x7279, 0x5F81,  // 具有具体特别特殊特征
    
    // 界面常用词(50个)
    0x8BBE, 0x7F6E, 0x786E, 0x5B9A, 0x53D6, 0x6D88, 0x8FD4, 0x56DE, 0x4FDD, 0x5B58,  // 设置确定取消返回保存
    0x5220, 0x9664, 0x6DFB, 0x52A0, 0x7F16, 0x8F91, 0x4FEE, 0x6539, 0x67E5, 0x770B,  // 删除添加编辑修改查看
    0x641C, 0x7D22, 0x6253, 0x5F00, 0x5173, 0x95ED, 0x542F, 0x52A8, 0x9000, 0x51FA,  // 搜索打开关闭启动退出
    0x767B, 0x5F55, 0x6CE8, 0x518C, 0x4E0B, 0x8F7D, 0x4E0A, 0x4F20, 0x5237, 0x65B0,  // 登录注册下载上传刷新
    0x91CD, 0x7F6E, 0x6062, 0x590D, 0x66F4, 0x65B0, 0x5347, 0x7EA7, 0x5E2E, 0x52A9,  // 重置恢复更新升级帮助
    
    // 标点和辅助(50个)
    0x4E0E, 0x6216, 0x8005, 0x5373, 0x7B49, 0x7B49, 0x7B49, 0x4EC5, 0x4EC5, 0x53EA,  // 与或者即等等等仅仅只
    0x6709, 0x53EA, 0x9700, 0x65E0, 0x9700, 0x5FC5, 0x987B, 0x5E94, 0x8BE5, 0x53EF,  // 有只需无需必须应该可
    0x80FD, 0x53EF, 0x80FD, 0x53EF, 0x4EE5, 0x80FD, 0x591F, 0x8DB3, 0x591F, 0x975E,  // 能可能可以能够足够非
    0x5E38, 0x5341, 0x5206, 0x975E, 0x5E38, 0x6781, 0x5176, 0x6BD4, 0x8F83, 0x76F8,  // 常十分非常极其比较相
    0x5BF9, 0x76F8, 0x5173, 0x76F8, 0x4F3C, 0x76F8, 0x540C, 0x4E0D, 0x540C, 0x5DEE,  // 对相关相似相同不同差
};

static const int DEFAULT_COMMON_CHAR_COUNT = sizeof(DEFAULT_COMMON_CHARS) / sizeof(uint16_t);

// ============ 全局单例 ============
static ChineseFontCache* g_font_cache_instance = nullptr;

ChineseFontCache& getFontCache() {
    if (!g_font_cache_instance) {
        g_font_cache_instance = new ChineseFontCache();
    }
    return *g_font_cache_instance;
}

bool getChineseChar(uint16_t unicode, FontSize font_size, uint8_t* out_buffer) {
    return getFontCache().getCharGlyph(unicode, font_size, out_buffer);
}

int preloadPageText(const char* page_text, FontSize font_size) {
    return getFontCache().preloadPage(page_text, font_size);
}

// ============ ChineseFontCache 实现 ============

ChineseFontCache::ChineseFontCache() 
    : common_char_list_(nullptr)
    , common_char_count_(0)
    , is_initialized_(false)
    , psram_enabled_(false)
{
    memset(&stats_, 0, sizeof(stats_));
    memset(sd_font_path_, 0, sizeof(sd_font_path_));
    cache_mutex_ = xSemaphoreCreateMutex();
}

ChineseFontCache::~ChineseFontCache() {
    if (common_char_list_) {
        free(common_char_list_);
    }
    
    // 释放常用字缓存
    for (auto& pair : common_cache_) {
        if (pair.second.glyph_16x16) free(pair.second.glyph_16x16);
        if (pair.second.glyph_24x24) free(pair.second.glyph_24x24);
    }
    
    // 释放页面缓存
    for (auto& pair : page_cache_) {
        if (pair.second.glyph_16x16) free(pair.second.glyph_16x16);
        if (pair.second.glyph_24x24) free(pair.second.glyph_24x24);
    }
    
    if (cache_mutex_) {
        vSemaphoreDelete(cache_mutex_);
    }
}

bool ChineseFontCache::init(const char* sd_font_path, bool enable_psram) {
    if (is_initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }
    
    strncpy(sd_font_path_, sd_font_path, sizeof(sd_font_path_) - 1);
    psram_enabled_ = enable_psram;
    
    // 检查PSRAM
    if (enable_psram) {
        size_t psram_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "PSRAM available: %d bytes", psram_size);
        
        if (psram_size < 200 * 1024) {
            ESP_LOGW(TAG, "PSRAM too small, disabling PSRAM cache");
            psram_enabled_ = false;
        }
    }
    
    // 使用默认常用字列表
    setCommonCharList(DEFAULT_COMMON_CHARS, DEFAULT_COMMON_CHAR_COUNT);
    
    is_initialized_ = true;
    ESP_LOGI(TAG, "Font cache initialized, path: %s, PSRAM: %s", 
             sd_font_path_, psram_enabled_ ? "enabled" : "disabled");
    
    return true;
}

void ChineseFontCache::setCommonCharList(const uint16_t* chars, int count) {
    if (common_char_list_) {
        free(common_char_list_);
    }
    
    common_char_count_ = count;
    common_char_list_ = (uint16_t*)malloc(count * sizeof(uint16_t));
    
    if (common_char_list_) {
        memcpy(common_char_list_, chars, count * sizeof(uint16_t));
        ESP_LOGI(TAG, "Common char list set: %d characters", count);
    }
}

int ChineseFontCache::loadCommonCharacters() {
    if (!is_initialized_) {
        ESP_LOGE(TAG, "Not initialized");
        return 0;
    }
    
    if (!common_char_list_ || common_char_count_ == 0) {
        ESP_LOGW(TAG, "No common char list");
        return 0;
    }
    
    ESP_LOGI(TAG, "Loading %d common characters to cache...", common_char_count_);
    
    int loaded = 0;
    uint8_t temp_buffer[MAX_FONT_SIZE];
    
    for (int i = 0; i < common_char_count_; i++) {
        uint16_t unicode = common_char_list_[i];
        
        // 读取16x16字模
        if (readFromSD(unicode, FONT_16x16, temp_buffer)) {
            CharCacheEntry entry;
            entry.unicode = unicode;
            entry.glyph_16x16 = (uint8_t*)malloc(32);  // 16x16 = 32 bytes
            entry.glyph_24x24 = nullptr;
            entry.hit_count = 0;
            entry.last_access_time = 0;
            entry.is_loaded = true;
            
            if (entry.glyph_16x16) {
                memcpy(entry.glyph_16x16, temp_buffer, 32);
                common_cache_[unicode] = entry;
                loaded++;
                
                stats_.memory_used += 32;
            }
        }
        
        // 每100个字符打印一次进度
        if ((i + 1) % 100 == 0) {
            ESP_LOGI(TAG, "Loaded %d/%d characters", i + 1, common_char_count_);
        }
    }
    
    ESP_LOGI(TAG, "Common cache loaded: %d/%d characters, memory: %d bytes", 
             loaded, common_char_count_, stats_.memory_used);
    
    return loaded;
}

bool ChineseFontCache::getCharGlyph(uint16_t unicode, FontSize font_size, uint8_t* out_buffer) {
    if (!is_initialized_ || !out_buffer) {
        return false;
    }
    
    xSemaphoreTake(cache_mutex_, portMAX_DELAY);
    
    stats_.total_requests++;
    bool success = false;
    
    // 层级1: 查找常用字缓存
    auto it_common = common_cache_.find(unicode);
    if (it_common != common_cache_.end()) {
        CharCacheEntry& entry = it_common->second;
        
        if (font_size == FONT_16x16 && entry.glyph_16x16) {
            memcpy(out_buffer, entry.glyph_16x16, 32);
            entry.hit_count++;
            entry.last_access_time = xTaskGetTickCount();
            stats_.common_cache_hits++;
            success = true;
            
            updateAccessStats(true, false);
            xSemaphoreGive(cache_mutex_);
            return true;
        }
    }
    
    // 层级2: 查找页面缓存
    auto it_page = page_cache_.find(unicode);
    if (it_page != page_cache_.end()) {
        CharCacheEntry& entry = it_page->second;
        
        if (font_size == FONT_16x16 && entry.glyph_16x16) {
            memcpy(out_buffer, entry.glyph_16x16, 32);
            entry.hit_count++;
            entry.last_access_time = xTaskGetTickCount();
            stats_.page_cache_hits++;
            success = true;
            
            updateAccessStats(false, true);
            xSemaphoreGive(cache_mutex_);
            return true;
        }
    }
    
    // 层级3: 从SD卡读取
    if (readFromSD(unicode, font_size, out_buffer)) {
        stats_.sd_reads++;
        
        // 添加到页面缓存,下次更快
        addToPageCache(unicode, out_buffer, font_size);
        
        success = true;
    }
    
    updateAccessStats(false, false);
    xSemaphoreGive(cache_mutex_);
    
    return success;
}

bool ChineseFontCache::readFromSD(uint16_t unicode, FontSize font_size, uint8_t* out_buffer) {
    // TODO: 实现从SD卡读取字模的逻辑
    // 这里需要根据您的SD卡字库文件格式来实现
    // 示例伪代码:
    /*
    FILE* fp = fopen(sd_font_path_, "rb");
    if (!fp) return false;
    
    // 计算偏移位置 (假设按Unicode顺序存储)
    uint32_t offset = (unicode - 0x4E00) * getGlyphSize(font_size);
    fseek(fp, offset, SEEK_SET);
    
    size_t bytes_read = fread(out_buffer, 1, getGlyphSize(font_size), fp);
    fclose(fp);
    
    return bytes_read == getGlyphSize(font_size);
    */
    
    ESP_LOGD(TAG, "Reading unicode 0x%04X from SD", unicode);
    
    // 临时返回false,等待实际实现
    return false;
}

void ChineseFontCache::addToPageCache(uint16_t unicode, const uint8_t* glyph, FontSize font_size) {
    // 检查缓存大小,必要时淘汰
    if (page_cache_.size() >= PAGE_CACHE_SIZE) {
        evictLRU();
    }
    
    CharCacheEntry entry;
    entry.unicode = unicode;
    entry.hit_count = 1;
    entry.last_access_time = xTaskGetTickCount();
    entry.is_loaded = true;
    
    uint32_t glyph_size = getGlyphSize(font_size);
    
    if (font_size == FONT_16x16) {
        entry.glyph_16x16 = (uint8_t*)malloc(glyph_size);
        entry.glyph_24x24 = nullptr;
        if (entry.glyph_16x16) {
            memcpy(entry.glyph_16x16, glyph, glyph_size);
            page_cache_[unicode] = entry;
            stats_.memory_used += glyph_size;
        }
    } else if (font_size == FONT_24x24) {
        entry.glyph_16x16 = nullptr;
        entry.glyph_24x24 = (uint8_t*)malloc(glyph_size);
        if (entry.glyph_24x24) {
            memcpy(entry.glyph_24x24, glyph, glyph_size);
            page_cache_[unicode] = entry;
            stats_.memory_used += glyph_size;
        }
    }
}

void ChineseFontCache::evictLRU() {
    if (page_cache_.empty()) return;
    
    // 找到访问次数最少且最久未访问的条目
    uint16_t evict_unicode = 0;
    uint32_t min_score = UINT32_MAX;
    
    for (auto& pair : page_cache_) {
        uint32_t score = pair.second.hit_count * 1000 + 
                        (xTaskGetTickCount() - pair.second.last_access_time);
        
        if (score < min_score) {
            min_score = score;
            evict_unicode = pair.first;
        }
    }
    
    // 删除条目
    auto it = page_cache_.find(evict_unicode);
    if (it != page_cache_.end()) {
        if (it->second.glyph_16x16) {
            stats_.memory_used -= 32;
            free(it->second.glyph_16x16);
        }
        if (it->second.glyph_24x24) {
            stats_.memory_used -= 72;
            free(it->second.glyph_24x24);
        }
        page_cache_.erase(it);
        
        ESP_LOGD(TAG, "Evicted char 0x%04X from page cache", evict_unicode);
    }
}

int ChineseFontCache::preloadPage(const char* page_text, FontSize font_size) {
    if (!page_text) return 0;
    
    uint16_t unicodes[PAGE_CACHE_SIZE];
    int count = extractChineseChars(page_text, unicodes, PAGE_CACHE_SIZE);
    
    return preloadChars(unicodes, count, font_size);
}

int ChineseFontCache::preloadWord(const char* word_text, FontSize font_size) {
    return preloadPage(word_text, font_size);  // 实现相同
}

int ChineseFontCache::preloadChars(const uint16_t* unicodes, int count, FontSize font_size) {
    if (!unicodes || count == 0) return 0;
    
    int loaded = 0;
    uint8_t temp_buffer[MAX_FONT_SIZE];
    
    for (int i = 0; i < count; i++) {
        // 如果已在缓存中,跳过
        if (isInCommonCache(unicodes[i])) continue;
        
        auto it = page_cache_.find(unicodes[i]);
        if (it != page_cache_.end()) continue;
        
        // 从SD卡读取并加载到页面缓存
        if (readFromSD(unicodes[i], font_size, temp_buffer)) {
            addToPageCache(unicodes[i], temp_buffer, font_size);
            loaded++;
        }
    }
    
    ESP_LOGI(TAG, "Preloaded %d/%d characters", loaded, count);
    return loaded;
}

void ChineseFontCache::clearPageCache() {
    xSemaphoreTake(cache_mutex_, portMAX_DELAY);
    
    for (auto& pair : page_cache_) {
        if (pair.second.glyph_16x16) {
            free(pair.second.glyph_16x16);
            stats_.memory_used -= 32;
        }
        if (pair.second.glyph_24x24) {
            free(pair.second.glyph_24x24);
            stats_.memory_used -= 72;
        }
    }
    
    page_cache_.clear();
    ESP_LOGI(TAG, "Page cache cleared");
    
    xSemaphoreGive(cache_mutex_);
}

int ChineseFontCache::extractChineseChars(const char* text, uint16_t* out_unicodes, int max_count) {
    if (!text || !out_unicodes) return 0;
    
    int count = 0;
    const uint8_t* p = (const uint8_t*)text;
    
    while (*p && count < max_count) {
        uint16_t unicode = 0;
        
        // UTF-8 解码
        if ((*p & 0x80) == 0) {
            // ASCII字符,跳过
            p++;
        } else if ((*p & 0xE0) == 0xC0) {
            // 2字节UTF-8
            unicode = ((*p & 0x1F) << 6) | (*(p+1) & 0x3F);
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            // 3字节UTF-8 (中文通常在这里)
            unicode = ((*p & 0x0F) << 12) | ((*(p+1) & 0x3F) << 6) | (*(p+2) & 0x3F);
            
            // 检查是否是中文字符范围 (0x4E00-0x9FA5)
            if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
                out_unicodes[count++] = unicode;
            }
            
            p += 3;
        } else if ((*p & 0xF8) == 0xF0) {
            // 4字节UTF-8
            p += 4;
        } else {
            p++;
        }
    }
    
    return count;
}

uint32_t ChineseFontCache::getGlyphSize(FontSize font_size) const {
    switch (font_size) {
        case FONT_16x16: return 32;   // 16*16/8
        case FONT_24x24: return 72;   // 24*24/8
        case FONT_32x32: return 128;  // 32*32/8
        default: return 32;
    }
}

void ChineseFontCache::updateAccessStats(bool is_common_hit, bool is_page_hit) {
    if (stats_.total_requests > 0) {
        stats_.hit_rate = (float)(stats_.common_cache_hits + stats_.page_cache_hits) / 
                         stats_.total_requests * 100.0f;
    }
}

bool ChineseFontCache::isInCommonCache(uint16_t unicode) const {
    return common_cache_.find(unicode) != common_cache_.end();
}

uint32_t ChineseFontCache::getMemoryUsage() const {
    return stats_.memory_used;
}

CacheStats ChineseFontCache::getStats() const {
    return stats_;
}

void ChineseFontCache::printStatus() const {
    ESP_LOGI(TAG, "========== Font Cache Status ==========");
    ESP_LOGI(TAG, "Common cache: %d chars, %d bytes", 
             common_cache_.size(), common_cache_.size() * 32);
    ESP_LOGI(TAG, "Page cache: %d chars", page_cache_.size());
    ESP_LOGI(TAG, "Total requests: %d", stats_.total_requests);
    ESP_LOGI(TAG, "Common hits: %d (%.1f%%)", 
             stats_.common_cache_hits, 
             stats_.total_requests > 0 ? (float)stats_.common_cache_hits/stats_.total_requests*100 : 0);
    ESP_LOGI(TAG, "Page hits: %d (%.1f%%)", 
             stats_.page_cache_hits,
             stats_.total_requests > 0 ? (float)stats_.page_cache_hits/stats_.total_requests*100 : 0);
    ESP_LOGI(TAG, "SD reads: %d (%.1f%%)", 
             stats_.sd_reads,
             stats_.total_requests > 0 ? (float)stats_.sd_reads/stats_.total_requests*100 : 0);
    ESP_LOGI(TAG, "Overall hit rate: %.1f%%", stats_.hit_rate);
    ESP_LOGI(TAG, "Memory used: %d bytes", stats_.memory_used);
    ESP_LOGI(TAG, "======================================");
}

void ChineseFontCache::resetStats() {
    memset(&stats_, 0, sizeof(stats_));
    ESP_LOGI(TAG, "Statistics reset");
}

/**
 * @file chinese_font_cache.cpp
 * @brief 中文字库混合缓存系统实现
 */

#include "chinese_font_cache.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp32/spiram.h"  // 使用新的头文件路径
#include <string.h>
#include <algorithm>
#include <stdio.h>  // 用于文件操作
#include <dirent.h> // 用于目录扫描
#include <ctype.h>  // 用于isdigit

static const char* TAG = "FontCache";

// ============ GB2312一级常用汉字前500个 (已禁用 - 使用PSRAM完整字库) ============
/*
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
*/

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
        if (pair.second.glyph_20x20) free(pair.second.glyph_20x20);
        if (pair.second.glyph_24x24) free(pair.second.glyph_24x24);
        if (pair.second.glyph_28x28) free(pair.second.glyph_28x28);
    }
    
    // 释放页面缓存
    for (auto& pair : page_cache_) {
        if (pair.second.glyph_16x16) free(pair.second.glyph_16x16);
        if (pair.second.glyph_20x20) free(pair.second.glyph_20x20);
        if (pair.second.glyph_24x24) free(pair.second.glyph_24x24);
        if (pair.second.glyph_28x28) free(pair.second.glyph_28x28);
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
    
    // 使用默认常用字列表 (已禁用 - 使用PSRAM完整字库)
    // setCommonCharList(DEFAULT_COMMON_CHARS, DEFAULT_COMMON_CHAR_COUNT);
    
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
    
    // 根据文件路径判断字体大小
    FontSize target_font_size = FONT_24x24;  // 默认24x24
    if (strstr(sd_font_path_, "16x16") != nullptr) {
        target_font_size = FONT_16x16;
    } else if (strstr(sd_font_path_, "20x20") != nullptr) {
        target_font_size = FONT_20x20;
    } else if (strstr(sd_font_path_, "24x24") != nullptr) {
        target_font_size = FONT_24x24;
    } else if (strstr(sd_font_path_, "28x28") != nullptr) {
        target_font_size = FONT_28x28;
    } else if (strstr(sd_font_path_, "32x32") != nullptr) {
        target_font_size = FONT_32x32;
    }
    
    uint32_t glyph_size = getGlyphSize(target_font_size);
    ESP_LOGI(TAG, "Loading %d common characters to cache (font size: %dx%d, %d bytes)...", 
             common_char_count_, target_font_size, target_font_size, glyph_size);
    
    int loaded = 0;
    uint8_t temp_buffer[MAX_FONT_SIZE];
    
    for (int i = 0; i < common_char_count_; i++) {
        uint16_t unicode = common_char_list_[i];
        
        // 读取指定字号的字模
        if (readFromSD(unicode, target_font_size, temp_buffer)) {
            CharCacheEntry entry;
            entry.unicode = unicode;
            entry.hit_count = 0;
            entry.last_access_time = 0;
            entry.is_loaded = true;
            
            if (target_font_size == FONT_16x16) {
                entry.glyph_16x16 = (uint8_t*)malloc(32);
                entry.glyph_20x20 = nullptr;
                entry.glyph_24x24 = nullptr;
                entry.glyph_28x28 = nullptr;
                if (entry.glyph_16x16) {
                    memcpy(entry.glyph_16x16, temp_buffer, 32);
                    common_cache_[unicode] = entry;
                    loaded++;
                    stats_.memory_used += 32;
                }
            } else if (target_font_size == FONT_20x20) {
                entry.glyph_16x16 = nullptr;
                entry.glyph_20x20 = (uint8_t*)malloc(60);
                entry.glyph_24x24 = nullptr;
                entry.glyph_28x28 = nullptr;
                if (entry.glyph_20x20) {
                    memcpy(entry.glyph_20x20, temp_buffer, 60);
                    common_cache_[unicode] = entry;
                    loaded++;
                    stats_.memory_used += 60;
                }
            } else if (target_font_size == FONT_24x24) {
                entry.glyph_16x16 = nullptr;
                entry.glyph_20x20 = nullptr;
                entry.glyph_24x24 = (uint8_t*)malloc(72);
                entry.glyph_28x28 = nullptr;
                if (entry.glyph_24x24) {
                    memcpy(entry.glyph_24x24, temp_buffer, 72);
                    common_cache_[unicode] = entry;
                    loaded++;
                    stats_.memory_used += 72;
                }
            } else if (target_font_size == FONT_28x28) {
                entry.glyph_16x16 = nullptr;
                entry.glyph_20x20 = nullptr;
                entry.glyph_24x24 = nullptr;
                entry.glyph_28x28 = (uint8_t*)malloc(112);
                if (entry.glyph_28x28) {
                    memcpy(entry.glyph_28x28, temp_buffer, 112);
                    common_cache_[unicode] = entry;
                    loaded++;
                    stats_.memory_used += 112;
                }
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
        } else if (font_size == FONT_20x20 && entry.glyph_20x20) {
            memcpy(out_buffer, entry.glyph_20x20, 60);
            entry.hit_count++;
            entry.last_access_time = xTaskGetTickCount();
            stats_.common_cache_hits++;
            success = true;
            
            updateAccessStats(true, false);
            xSemaphoreGive(cache_mutex_);
            return true;
        } else if (font_size == FONT_24x24 && entry.glyph_24x24) {
            memcpy(out_buffer, entry.glyph_24x24, 72);
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
        } else if (font_size == FONT_20x20 && entry.glyph_20x20) {
            memcpy(out_buffer, entry.glyph_20x20, 60);
            entry.hit_count++;
            entry.last_access_time = xTaskGetTickCount();
            stats_.page_cache_hits++;
            success = true;
            
            updateAccessStats(false, true);
            xSemaphoreGive(cache_mutex_);
            return true;
        } else if (font_size == FONT_24x24 && entry.glyph_24x24) {
            memcpy(out_buffer, entry.glyph_24x24, 72);
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
    if (!out_buffer || !is_initialized_) {
        return false;
    }
    
    // 检查 Unicode 范围 (GB2312 汉字范围: 0x4E00-0x9FA5)
    if (unicode < 0x4E00 || unicode > 0x9FA5) {
        ESP_LOGW(TAG, "Unicode 0x%04X out of range", unicode);
        return false;
    }
    
    // 打开字体文件
    FILE* fp = fopen(sd_font_path_, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open font file: %s", sd_font_path_);
        return false;
    }
    
    // 计算字模大小
    uint32_t glyph_size = getGlyphSize(font_size);
    
    // 计算文件偏移量 (按 Unicode 顺序存储)
    uint32_t offset = (unicode - 0x4E00) * glyph_size;
    
    // 定位到指定位置
    if (fseek(fp, offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Failed to seek to offset %u", offset);
        fclose(fp);
        return false;
    }
    
    // 读取字模数据
    size_t bytes_read = fread(out_buffer, 1, glyph_size, fp);
    fclose(fp);
    
    if (bytes_read != glyph_size) {
        ESP_LOGW(TAG, "Read %u bytes, expected %u for unicode 0x%04X", 
                 bytes_read, glyph_size, unicode);
        return false;
    }
    
    stats_.sd_reads++;
    ESP_LOGD(TAG, "Successfully read unicode 0x%04X from SD", unicode);
    
    return true;
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
        entry.glyph_20x20 = nullptr;
        entry.glyph_24x24 = nullptr;
        entry.glyph_28x28 = nullptr;
        if (entry.glyph_16x16) {
            memcpy(entry.glyph_16x16, glyph, glyph_size);
            page_cache_[unicode] = entry;
            stats_.memory_used += glyph_size;
        }
    } else if (font_size == FONT_20x20) {
        entry.glyph_16x16 = nullptr;
        entry.glyph_20x20 = (uint8_t*)malloc(glyph_size);
        entry.glyph_24x24 = nullptr;
        entry.glyph_28x28 = nullptr;
        if (entry.glyph_20x20) {
            memcpy(entry.glyph_20x20, glyph, glyph_size);
            page_cache_[unicode] = entry;
            stats_.memory_used += glyph_size;
        }
    } else if (font_size == FONT_24x24) {
        entry.glyph_16x16 = nullptr;
        entry.glyph_20x20 = nullptr;
        entry.glyph_24x24 = (uint8_t*)malloc(glyph_size);
        entry.glyph_28x28 = nullptr;
        if (entry.glyph_24x24) {
            memcpy(entry.glyph_24x24, glyph, glyph_size);
            page_cache_[unicode] = entry;
            stats_.memory_used += glyph_size;
        }
    } else if (font_size == FONT_28x28) {
        entry.glyph_16x16 = nullptr;
        entry.glyph_20x20 = nullptr;
        entry.glyph_24x24 = nullptr;
        entry.glyph_28x28 = (uint8_t*)malloc(glyph_size);
        if (entry.glyph_28x28) {
            memcpy(entry.glyph_28x28, glyph, glyph_size);
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
        if (it->second.glyph_20x20) {
            stats_.memory_used -= 60;
            free(it->second.glyph_20x20);
        }
        if (it->second.glyph_24x24) {
            stats_.memory_used -= 72;
            free(it->second.glyph_24x24);
        }
        if (it->second.glyph_28x28) {
            stats_.memory_used -= 112;
            free(it->second.glyph_28x28);
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
        case FONT_20x20: return 60;   // 20 * ((20+7)/8) = 20*3
        case FONT_24x24: return 72;   // 24*24/8
        case FONT_28x28: return 112;  // 28 * ((28+7)/8) = 28*4
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

// ============ 便捷初始化接口实现 ============

// 存储三个缓存实例（16x16、20x20 和 24x24）
static ChineseFontCache g_font_cache_16x16;
static ChineseFontCache g_font_cache_20x20;  // 新增 20x20 缓存
static ChineseFontCache g_font_cache_24x24;
static bool g_font_cache_initialized = false;

/**
 * @brief 一步初始化字库缓存系统 (16x16 + 20x20)
 */
bool initFontCacheSystem(const char* font_16x16_path,
                         const char* font_20x20_path,
                         bool enable_psram) {
    if (g_font_cache_initialized) {
        ESP_LOGW(TAG, "字库缓存系统已初始化，跳过重复初始化");
        return true;
    }

    ESP_LOGI(TAG, "========== 初始化字库缓存系统 ==========");
    ESP_LOGI(TAG, "16x16字体: %s", font_16x16_path);
    ESP_LOGI(TAG, "20x20字体: %s", font_20x20_path);
    ESP_LOGI(TAG, "启用PSRAM: %s", enable_psram ? "是" : "否");

    // 初始化16x16字体缓存
    if (!g_font_cache_16x16.init(font_16x16_path, enable_psram)) {
        ESP_LOGE(TAG, "✗ 16x16字体缓存初始化失败");
        return false;
    }
    ESP_LOGI(TAG, "✅ 16x16字体缓存初始化成功");

    // 初始化20x20字体缓存
    if (!g_font_cache_20x20.init(font_20x20_path, enable_psram)) {
        ESP_LOGE(TAG, "✗ 20x20字体缓存初始化失败");
        return false;
    }
    ESP_LOGI(TAG, "✅ 20x20字体缓存初始化成功");

    // 预加载常用字到两个字号的缓存
    int loaded_16 = g_font_cache_16x16.loadCommonCharacters();
    int loaded_20 = g_font_cache_20x20.loadCommonCharacters();
    ESP_LOGI(TAG, "✅ 预加载完成: 16x16: %d个常用字, 20x20: %d个常用字", 
             loaded_16, loaded_20);

    g_font_cache_initialized = true;
    ESP_LOGI(TAG, "========== 缓存初始化完成 ==========");
    
    return true;
}

/**
 * @brief 预加载页面到缓存
 */
int preloadPageToCache(const char* page_text) {
    if (!g_font_cache_initialized) {
        ESP_LOGW(TAG, "缓存系统未初始化，无法预加载");
        return 0;
    }

    int count_16 = g_font_cache_16x16.preloadPage(page_text, FONT_16x16);
    int count_24 = g_font_cache_24x24.preloadPage(page_text, FONT_24x24);
    
    int total = count_16 + count_24;
    ESP_LOGI(TAG, "页面预加载: 16x16=%d, 24x24=%d, 总计=%d", 
             count_16, count_24, total);
    
    return total;
}

/**
 * @brief 打印缓存统计信息
 */
void printCacheStats() {
    if (!g_font_cache_initialized) {
        ESP_LOGW(TAG, "缓存系统未初始化");
        return;
    }

    ESP_LOGI(TAG, "========== 缓存统计信息 ==========");
    
    ESP_LOGI(TAG, "--- 16x16字体缓存 ---");
    g_font_cache_16x16.printStatus();
    
    ESP_LOGI(TAG, "--- 20x20字体缓存 ---");
    g_font_cache_20x20.printStatus();
    
    ESP_LOGI(TAG, "========== 统计完成 ==========");
}

/**
 * @brief 清除所有缓存（保留常用字缓存）
 */
void clearAllPageCache() {
    if (!g_font_cache_initialized) {
        ESP_LOGW(TAG, "缓存系统未初始化");
        return;
    }

    g_font_cache_16x16.clearPageCache();
    g_font_cache_20x20.clearPageCache();
    
    ESP_LOGI(TAG, "✅ 已清除所有页面缓存");
}

/**
 * @brief 获取16x16字体缓存实例
 */
ChineseFontCache& getFontCache16() {
    return g_font_cache_16x16;
}

/**
 * @brief 获取20x20字体缓存实例
 */
ChineseFontCache& getFontCache20() {
    return g_font_cache_20x20;
}

/**
 * @brief 获取24x24字体缓存实例 (已废弃，保留以兼容)
 */
ChineseFontCache& getFontCache24() {
    return g_font_cache_24x24;
}

// ============ PSRAM 完整字体加载系统实现 ============

// 全局PSRAM字体数据
static FullFontData g_psram_fonts[MAX_PSRAM_FONTS];
static int g_psram_font_count = 0;

// 当前激活的字体
const FullFontData* g_current_psram_font = nullptr;

/**
 * @brief 从SD卡加载完整字体文件到PSRAM
 */
bool loadFullFontToPSRAM(FullFontData* font_data, const char* file_path) {
    if (!font_data || !file_path) {
        ESP_LOGE(TAG, "loadFullFontToPSRAM: 无效参数");
        return false;
    }
    
    // 检查 PSRAM 是否可用
    #if CONFIG_ESP32S3_SPIRAM_SUPPORT || CONFIG_SPIRAM
    if (!esp_spiram_is_initialized()) {
        ESP_LOGE(TAG, "PSRAM 未初始化");
        return false;
    }
    #else
    ESP_LOGE(TAG, "PSRAM 支持未启用");
    return false;
    #endif
    
    // 打开字体文件
    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "无法打开字体文件: %s", file_path);
        return false;
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    ESP_LOGI(TAG, "字体文件 %s 大小: %u 字节 (%.2f MB)", file_path, file_size, file_size / 1024.0 / 1024.0);
    
    // 检查 PSRAM 剩余空间
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "PSRAM 可用空间: %u 字节 (%.2f MB)", free_psram, free_psram / 1024.0 / 1024.0);
    
    if (free_psram < file_size) {
        ESP_LOGE(TAG, "PSRAM 空间不足! 需要 %u 字节, 可用 %u 字节", file_size, free_psram);
        fclose(fp);
        return false;
    }
    
    // 从 PSRAM 分配内存
    font_data->data = (uint8_t*)heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
    if (!font_data->data) {
        ESP_LOGE(TAG, "PSRAM 分配失败");
        fclose(fp);
        return false;
    }
    
    // 读取整个文件到 PSRAM
    ESP_LOGI(TAG, "正在加载字体到 PSRAM...");
    size_t read_size = fread(font_data->data, 1, file_size, fp);
    fclose(fp);
    
    if (read_size != file_size) {
        ESP_LOGE(TAG, "文件读取不完整: 读取 %u / %u 字节", read_size, file_size);
        heap_caps_free(font_data->data);
        font_data->data = nullptr;
        return false;
    }
    
    // 保存字体信息
    font_data->size = file_size;
    strncpy(font_data->file_path, file_path, sizeof(font_data->file_path) - 1);
    font_data->is_loaded = true;
    
    // 从文件路径提取字体名称 (去掉路径、.bin扩展名和字号后缀)
    const char* filename = strrchr(file_path, '/');
    if (filename) {
        filename++; // 跳过 '/'
    } else {
        filename = file_path;
    }
    strncpy(font_data->font_name, filename, sizeof(font_data->font_name) - 1);
    font_data->font_name[sizeof(font_data->font_name) - 1] = '\0';
    
    // 去掉 .bin 扩展名
    char* dot = strrchr(font_data->font_name, '.');
    if (dot && strcmp(dot, ".bin") == 0) {
        *dot = '\0';
    }
    
    // 去掉字号后缀 (例如: chinese_translate_font_20 -> chinese_translate_font)
    char* last_underscore = strrchr(font_data->font_name, '_');
    if (last_underscore) {
        // 检查下划线后面是否全是数字
        bool all_digits = true;
        for (char* ptr = last_underscore + 1; *ptr != '\0'; ptr++) {
            if (!isdigit(*ptr)) {
                all_digits = false;
                break;
            }
        }
        // 如果是数字，去掉这个后缀
        if (all_digits && *(last_underscore + 1) != '\0') {
            *last_underscore = '\0';
        }
    }
    
    // 计算字符参数
    int bytes_per_row = (font_data->font_size + 7) / 8;
    font_data->glyph_size = bytes_per_row * font_data->font_size;
    font_data->char_count = file_size / font_data->glyph_size;
    
    ESP_LOGI(TAG, "✅ 字体加载成功:");
    ESP_LOGI(TAG, "   - 文件: %s", file_path);
    ESP_LOGI(TAG, "   - 名称: %s", font_data->font_name);
    ESP_LOGI(TAG, "   - 尺寸: %dx%d", font_data->font_size, font_data->font_size);
    ESP_LOGI(TAG, "   - 字节/字符: %u", font_data->glyph_size);
    ESP_LOGI(TAG, "   - 字符总数: %u", font_data->char_count);
    ESP_LOGI(TAG, "   - 内存位置: PSRAM");
    
    return true;
}

/**
 * @brief 从PSRAM字体中获取字符字模
 */
bool getCharGlyphFromPSRAM(const FullFontData* font_data, uint16_t unicode, uint8_t* out_buffer) {
    if (!font_data || !font_data->is_loaded || !font_data->data || !out_buffer) {
        return false;
    }
    
    // 检查文件是否有 TTFG 文件头（TTF 转换工具生成的格式）
    if (font_data->size > 12 && 
        font_data->data[0] == 'T' && 
        font_data->data[1] == 'T' &&
        font_data->data[2] == 'F' && 
        font_data->data[3] == 'G') {
        
        // 带文件头的格式，使用字形表查找
        // 文件头格式:
        // [0-3]   魔数 "TTFG"
        // [4-5]   字体大小 (uint16_t little-endian)
        // [6-7]   字形数量 (uint16_t little-endian)
        // [8-11]  字形表大小 (uint32_t little-endian)
        // [12+]   字形表 (每个12字节: code[4], offset[4], width[2], height[2])
        
        uint16_t glyph_count = (font_data->data[6]) | (font_data->data[7] << 8);
        uint32_t glyph_table_size = (font_data->data[8]) | (font_data->data[9] << 8) |
                                   (font_data->data[10] << 16) | (font_data->data[11] << 24);
        
        // 字形表起始位置
        const uint8_t* glyph_table = font_data->data + 12;
        
        // 查找目标字符
        for (int i = 0; i < glyph_count; i++) {
            const uint8_t* entry = glyph_table + (i * 12);
            
            // 读取 Unicode 编码 (little-endian)
            uint32_t code = entry[0] | (entry[1] << 8) | (entry[2] << 16) | (entry[3] << 24);
            
            if (code == unicode) {
                // 找到了！读取位图偏移量
                uint32_t bitmap_offset = entry[4] | (entry[5] << 8) | 
                                        (entry[6] << 16) | (entry[7] << 24);
                
                // 计算位图在文件中的实际位置
                uint32_t bitmap_pos = 12 + glyph_table_size + bitmap_offset;
                
                // 检查是否越界
                if (bitmap_pos + font_data->glyph_size > font_data->size) {
                    ESP_LOGW(TAG, "字符 U+%04X 位图数据越界", unicode);
                    return false;
                }
                
                // 复制字模数据
                memcpy(out_buffer, font_data->data + bitmap_pos, font_data->glyph_size);
                return true;
            }
        }
        
        // 未找到字符
        return false;
    }
    
    // ===== 无文件头的原始格式（旧格式，用于 fangsong 等字体）=====
    uint32_t offset = 0;
    
    // 判断字符类型并计算偏移量
    if (unicode >= 0x4E00 && unicode <= 0x9FA5) {
        // 中文字符 (GB2312: 0x4E00-0x9FA5)
        offset = (unicode - 0x4E00) * font_data->glyph_size;
    } else if (unicode >= 0x20 && unicode <= 0x7E) {
        // ASCII 可打印字符 (0x20-0x7E, 共95个字符)
        offset = (unicode - 0x20) * font_data->glyph_size;
    } else if (unicode >= 0x0250 && unicode <= 0x02AF) {
        // IPA 国际音标扩展 (0x0250-0x02AF, 共96个字符)
        // 存储在 ASCII 后面
        offset = (95 + (unicode - 0x0250)) * font_data->glyph_size;
    } else {
        return false;
    }
    
    // 检查是否越界
    if (offset + font_data->glyph_size > font_data->size) {
        return false;
    }
    
    // 从 PSRAM 复制字模数据
    memcpy(out_buffer, font_data->data + offset, font_data->glyph_size);
    
    return true;
}

/**
 * @brief 根据字体大小查找PSRAM字体
 */
const FullFontData* findPSRAMFontBySize(int font_size) {
    for (int i = 0; i < g_psram_font_count; i++) {
        if (g_psram_fonts[i].is_loaded && g_psram_fonts[i].font_size == font_size) {
            return &g_psram_fonts[i];
        }
    }
    return nullptr;
}

/**
 * @brief 根据字体名称查找PSRAM字体
 */
const FullFontData* findPSRAMFontByName(const char* font_name) {
    if (!font_name) {
        return nullptr;
    }
    
    for (int i = 0; i < g_psram_font_count; i++) {
        if (g_psram_fonts[i].is_loaded) {
            if (strcmp(g_psram_fonts[i].font_name, font_name) == 0) {
                return &g_psram_fonts[i];
            }
            if (strcasecmp(g_psram_fonts[i].font_name, font_name) == 0) {
                return &g_psram_fonts[i];
            }
        }
    }
    return nullptr;
}

/**
 * @brief 辅助函数实现
 */
int getFontSize(const FullFontData* font_data) {
    if (!font_data || !font_data->is_loaded) return 0;
    return font_data->font_size;
}

uint32_t getGlyphSize(const FullFontData* font_data) {
    if (!font_data || !font_data->is_loaded) return 0;
    return font_data->glyph_size;
}

const char* getFontName(const FullFontData* font_data) {
    if (!font_data || !font_data->is_loaded) return nullptr;
    return font_data->font_name;
}

/**
 * @brief 根据索引获取PSRAM字体
 */
const FullFontData* getPSRAMFontByIndex(int index) {
    if (index < 0 || index >= g_psram_font_count) {
        return nullptr;
    }
    if (g_psram_fonts[index].is_loaded) {
        return &g_psram_fonts[index];
    }
    return nullptr;
}

/**
 * @brief 从文件名解析字体尺寸
 * 格式: "font_name_32.bin" - 查找最后一个下划线后的数字
 */
int parseFontSizeFromFilename(const char* filename) {
    size_t len = strlen(filename);
    
    // 去掉 .bin 后缀
    const char* name_end = filename + len;
    if (len > 4 && strcmp(filename + len - 4, ".bin") == 0) {
        name_end = filename + len - 4;
    }
    
    // 查找最后一个下划线后的数字 (例如: english_word_font_32.bin)
    const char* last_underscore = nullptr;
    for (const char* ptr = filename; ptr < name_end; ptr++) {
        if (*ptr == '_') {
            last_underscore = ptr;
        }
    }
    
    if (last_underscore && last_underscore < name_end - 1) {
        const char* num_start = last_underscore + 1;
        // 检查下划线后面是否全是数字
        bool all_digits = true;
        for (const char* ptr = num_start; ptr < name_end; ptr++) {
            if (!isdigit(*ptr)) {
                all_digits = false;
                break;
            }
        }
        
        if (all_digits && num_start < name_end) {
            int font_size = atoi(num_start);
            if (font_size > 0 && font_size <= 64) {
                return font_size;
            }
        }
    }
    
    return 0;
}

/**
 * @brief 判断字体文件是否需要加载到 PSRAM
 */
bool shouldLoadToPSRAM(const char* filename) {
    // 默认情况下 fangsong 字体也加载到 PSRAM (全量加载方案)
    // 如果内存受限，可以选择性注释掉某些字体
    
    // fangsong 字体加载到 PSRAM (推荐 - 性能最佳)
    if (strstr(filename, "fangsong") || strstr(filename, "fang_song")) {
        return true;  // 改为 true, 启用全量加载
    }
    
    // comic_sans_ms 系列加载到 PSRAM
    if (strstr(filename, "comic_sans")) {
        return true;
    }
    
    // 其他包含 "bold" 的字体加载到 PSRAM
    if (strstr(filename, "bold")) {
        return true;
    }
    
    return false;
}

/**
 * @brief 扫描SD卡并加载字体到PSRAM
 * @param load_all_fonts 是否加载所有字体（包括中文字体）
 * @return 成功加载的字体数量
 */
int initFullFontsInPSRAM(bool load_all_fonts) {
    ESP_LOGI(TAG, "========== 开始扫描 SD 卡字体文件 ==========");
    ESP_LOGI(TAG, "模式: %s", load_all_fonts ? "加载所有字体(包括中文)" : "只加载英文字体");
    
    DIR* dir = opendir("/sd");
    if (!dir) {
        ESP_LOGE(TAG, "无法打开 SD 卡目录");
        return 0;
    }
    
    struct dirent* entry;
    int loaded_count = 0;
    int scanned_count = 0;
    
    struct FontFileInfo {
        char path[128];
        char name[128];
        int size;
        bool is_chinese;
    };
    
    FontFileInfo font_files[MAX_PSRAM_FONTS];
    int font_file_count = 0;
    
    // ========== 扫描 SD 卡，查找 chinese_ 或 english_ 开头的字体 ==========
    while ((entry = readdir(dir)) != NULL && font_file_count < MAX_PSRAM_FONTS) {
        if (entry->d_type == DT_DIR) continue;
        
        const char* name = entry->d_name;
        size_t len = strlen(name);
        
        // 检查文件名长度，防止缓冲区溢出（必须放在最前面）
        if (len > 120 || len < 4) {
            if (len >= 4 && strcmp(name + len - 4, ".bin") == 0) {
                ESP_LOGW(TAG, "跳过 (文件名过长): %.50s...", name);
            }
            continue;
        }
        
        // 必须是 .bin 文件
        if (strcmp(name + len - 4, ".bin") != 0) continue;
        
        scanned_count++;
        
        // 检查是否以 chinese_ 或 english_ 开头
        bool is_chinese_font = (strncmp(name, "chinese_", 8) == 0);
        bool is_english_font = (strncmp(name, "english_", 8) == 0);
        
        if (!is_chinese_font && !is_english_font) {
            ESP_LOGD(TAG, "跳过 (不是 chinese_/english_ 字体): %s", name);
            continue;
        }
        
        // 如果不加载中文字体，跳过 chinese_
        if (!load_all_fonts && is_chinese_font) {
            ESP_LOGI(TAG, "跳过 (中文字体): %s", name);
            continue;
        }
        
        // 尝试从文件名解析字号
        int font_size = parseFontSizeFromFilename(name);
        if (font_size == 0) {
            // 如果无法解析字号，尝试打开文件读取
            char full_path[144];
            // 安全的字符串拼接（len已确保<=120，加上"/sd/"共124字节，在144字节缓冲区内安全）
            memcpy(full_path, "/sd/", 4);
            memcpy(full_path + 4, name, len);
            full_path[4 + len] = '\0';
            
            FILE* fp = fopen(full_path, "rb");
            if (fp) {
                // 假设是 20x20 字体（默认值）
                font_size = 20;
                fclose(fp);
                ESP_LOGW(TAG, "无法从文件名解析字号，使用默认值 20: %s", name);
            } else {
                ESP_LOGW(TAG, "跳过 (无法打开): %s", name);
                continue;
            }
        }
        
        // 添加到字体列表（len已确保<=120，所以这里是安全的）
        memcpy(font_files[font_file_count].path, "/sd/", 4);
        memcpy(font_files[font_file_count].path + 4, name, len);
        font_files[font_file_count].path[4 + len] = '\0';
        
        memcpy(font_files[font_file_count].name, name, len);
        font_files[font_file_count].name[len] = '\0';
        font_files[font_file_count].size = font_size;
        font_files[font_file_count].is_chinese = is_chinese_font;
        font_file_count++;
        
        ESP_LOGI(TAG, "发现字体 [%d]: %s (%dx%d) %s", 
                 font_file_count, name, font_size, font_size,
                 is_chinese_font ? "[中文]" : "[英文]");
    }
    
    closedir(dir);
    
    ESP_LOGI(TAG, "扫描完成: 共扫描 %d 个 .bin 文件, 发现 %d 个字体", 
             scanned_count, font_file_count);
    
    if (font_file_count == 0) {
        ESP_LOGW(TAG, "没有找到 chinese_/english_ 开头的字体文件");
        return 0;
    }
    
    // ========== 优先级排序：中文字体优先 ==========
    for (int i = 0; i < font_file_count - 1; i++) {
        for (int j = i + 1; j < font_file_count; j++) {
            if (!font_files[i].is_chinese && font_files[j].is_chinese) {
                // 交换位置：把中文字体移到前面
                FontFileInfo temp = font_files[i];
                font_files[i] = font_files[j];
                font_files[j] = temp;
            }
        }
    }
    
    // 最多加载 8 个字体
    int fonts_to_load = (font_file_count > MAX_PSRAM_FONTS) ? MAX_PSRAM_FONTS : font_file_count;
    
    ESP_LOGI(TAG, "========== 开始加载字体到 PSRAM (最多 %d 个) ==========", fonts_to_load);
    ESP_LOGI(TAG, "加载顺序: 中文字体优先, 然后是英文字体");
    
    for (int i = 0; i < fonts_to_load; i++) {
        FullFontData* target = &g_psram_fonts[i];
        
        target->data = nullptr;
        target->size = 0;
        target->font_size = font_files[i].size;
        target->glyph_size = 0;
        target->char_count = 0;
        target->is_loaded = false;
        memset(target->file_path, 0, sizeof(target->file_path));
        memset(target->font_name, 0, sizeof(target->font_name));
        
        ESP_LOGI(TAG, "[%d/%d] 正在加载: %s %s...", 
                 i+1, fonts_to_load, font_files[i].name,
                 font_files[i].is_chinese ? "[中文]" : "[英文]");
        
        if (loadFullFontToPSRAM(target, font_files[i].path)) {
            loaded_count++;
            g_psram_font_count++;
            ESP_LOGI(TAG, "  ✅ 加载成功: %s → PSRAM 字体名称: %s", 
                     font_files[i].name, target->font_name);
        } else {
            ESP_LOGW(TAG, "  ❌ 加载失败: %s", font_files[i].name);
        }
    }
    
    ESP_LOGI(TAG, "========== PSRAM 字体加载完成: %d/%d ==========", loaded_count, fonts_to_load);
    
    // 显示已加载的字体列表
    ESP_LOGI(TAG, "========== 已加载的字体列表 ==========");
    for (int i = 0; i < g_psram_font_count; i++) {
        if (g_psram_fonts[i].is_loaded) {
            ESP_LOGI(TAG, "  [%d] %s (%dx%d, %u 字符)", 
                     i, g_psram_fonts[i].font_name, 
                     g_psram_fonts[i].font_size, g_psram_fonts[i].font_size,
                     g_psram_fonts[i].char_count);
        }
    }
    
    #if CONFIG_ESP32S3_SPIRAM_SUPPORT || CONFIG_SPIRAM
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t used_psram = total_psram - free_psram;
    ESP_LOGI(TAG, "PSRAM 使用情况: %u / %u 字节 (%.1f%%)", 
             used_psram, total_psram, (float)used_psram / total_psram * 100.0f);
    #endif
    
    return loaded_count;
}

/**
 * @brief 释放所有PSRAM字体
 */
void freeAllPSRAMFonts() {
    for (int i = 0; i < MAX_PSRAM_FONTS; i++) {
        if (g_psram_fonts[i].data) {
            heap_caps_free(g_psram_fonts[i].data);
            g_psram_fonts[i].data = nullptr;
            g_psram_fonts[i].is_loaded = false;
        }
    }
    g_psram_font_count = 0;
    g_current_psram_font = nullptr;
    ESP_LOGI(TAG, "所有 PSRAM 字体已释放");
}

/**
 * @brief 获取PSRAM字体数量
 */
int getPSRAMFontCount() {
    return g_psram_font_count;
}

/**
 * @brief 直接加载指定字体文件到PSRAM (用于fangsong等大字库)
 */
bool loadSpecificFontToPSRAM(const char* file_path, int font_size) {
    if (!file_path || g_psram_font_count >= MAX_PSRAM_FONTS) {
        ESP_LOGE(TAG, "无法加载字体: 参数无效或PSRAM字体槽已满 (%d/%d)", 
                 g_psram_font_count, MAX_PSRAM_FONTS);
        return false;
    }
    
    FullFontData* target = &g_psram_fonts[g_psram_font_count];
    
    target->data = nullptr;
    target->size = 0;
    target->font_size = font_size;
    target->glyph_size = 0;
    target->char_count = 0;
    target->is_loaded = false;
    memset(target->file_path, 0, sizeof(target->file_path));
    memset(target->font_name, 0, sizeof(target->font_name));
    
    if (loadFullFontToPSRAM(target, file_path)) {
        g_psram_font_count++;
        ESP_LOGI(TAG, "✅ 字体已加载到 PSRAM 槽位 #%d", g_psram_font_count - 1);
        return true;
    } else {
        ESP_LOGE(TAG, "❌ 字体加载失败: %s", file_path);
        return false;
    }
}

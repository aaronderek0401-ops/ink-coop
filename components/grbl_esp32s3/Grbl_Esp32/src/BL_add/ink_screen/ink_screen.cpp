#include "ink_screen.h"
#include "./SPI_Init.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"
#include <stdio.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_370_GDEY037T03.h>
#include<dirent.h>
#include <SPI.h>
#include "../../grbl_esp32s3/Grbl_Esp32/src/Machines/inkScreen.h"
#include "image_loader.h"          // SD卡图片加载器
#include "chinese_text_display_impl.h"  // 中文字库支持
#include "chinese_font_cache.h"    // 中文字库缓存系统
#include "word_book.h"             // 单词本缓存系统

extern "C" {
#include "./Pic.h"
}
GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(
    GxEPD2_370_GDEY037T03(BSP_SPI_CS_GPIO_PIN, DC_GPIO_PIN, RES_GPIO_PIN, BUSY_GPIO_PIN)
);

// ========== SPI 全局变量定义 ==========
// 必须为全局变量，否则会在 ink_screen_init() 返回时被销毁，导致 display 对象的 SPI 引用失效
SPIClass EPD_SPI(HSPI);  // 创建独立的 SPI3 实例用于墨水屏

// ========== 全局变量定义 ==========
// 矩形总数 - 从屏幕管理器中获取实际配置的数量
int rect_count = 0;  // 将在ink_screen_init()中设置为实际值

// 主界面矩形数组
RectInfo rects[MAX_MAIN_RECTS] = {0};

// 单词界面矩形数组
RectInfo vocab_rects[MAX_VOCAB_RECTS] = {0};

// 常量定义
#define MAX_RECTS 50
#define MAX_ICONS_PER_RECT 4
// 全局变量定义
int g_last_underline_x = 0;
int g_last_underline_y = 0;
int g_last_underline_width = 0;
// 全局变量：当前选中的图标索引
int g_selected_icon_index = -1;

// ======== 焦点系统变量 ========
static int g_current_focus_rect = 0;  // 当前焦点所在的矩形索引
static int g_total_focusable_rects = 0;  // 可获得焦点的矩形总数
static bool g_focus_mode_enabled = false;  // 是否启用焦点模式

// 可配置的焦点矩形列表
static int g_focusable_rect_indices[MAX_FOCUSABLE_RECTS];  // 可焦点矩形的索引数组（母数组）
static int g_focusable_rect_count = 0;  // 实际可焦点矩形数量
static int g_current_focus_index = 0;  // 当前焦点在g_focusable_rect_indices中的位置

// ======== 子母数组系统变量 ========
static bool g_in_sub_array = false;  // 是否在子数组模式
static int g_sub_array_indices[MAX_FOCUSABLE_RECTS][MAX_FOCUSABLE_RECTS];  // 每个母数组元素对应的子数组
static int g_sub_array_counts[MAX_FOCUSABLE_RECTS];  // 每个母数组元素对应的子数组长度
static int g_current_sub_focus_index = 0;  // 当前在子数组中的焦点位置
static int g_parent_focus_index_backup = 0;  // 进入子数组前的母数组焦点位置备份

int g_global_icon_count = 0;     // 已分配的全局图标计数

IconPosition g_icon_positions[MAX_GLOBAL_ICONS];

// ==================== 图标缓存系统 ====================
// 图标缓存结构（用于预加载到PSRAM）
typedef struct {
    const char* filename;      // 文件名（用于调试）
    uint8_t* data;            // 图标位图数据（存储在PSRAM）
    uint32_t width;           // 图标宽度
    uint32_t height;          // 图标高度
    bool loaded;              // 是否已加载
} IconCache;

// ==================== 图标名称到索引映射 ====================
typedef struct {
    const char* name;
    int index;
} IconMapping;

static const IconMapping icon_mappings[] = {
    {"book", 0},          // 书籍 -> /book.bin
    {"game", 1},          // 游戏 -> /game.bin
    {"settings", 2},      // 设置 -> /settings.bin
    {"folder", 3},         // 文件夹 -> /folder.bin
    {"horn", 4},          // 喇叭 -> /horn.bin
    {"wifi_off", 5},       // WiFi关闭 -> /wifi_off.bin
    {"battery", 6},        // 电池 -> /battery.bin
    {"word", 7},           // 单词 -> /word.bin
    {"message", 8}         // 消息 -> /message.bin
};

// 自动计算图标数量
#define ICON_CACHE_COUNT (sizeof(icon_mappings) / sizeof(icon_mappings[0]))

// 全局图标缓存数组（自动适应icon_mappings数量）
static IconCache g_icon_cache[ICON_CACHE_COUNT] = {0};

const char *TAG = "ink_screen.cpp";
static TaskHandle_t _eventTaskHandle = NULL;
uint8_t inkScreenTestFlag = 0;
uint8_t inkScreenTestFlagTwo = 0;
// 全局变量记录上次显示信息
static char lastDisplayedText[256] = {0};

static esp_timer_handle_t sleep_timer;
static bool is_sleep_mode = false;
static uint32_t last_activity_time = 0;
// 注意：has_sleep_data 已在 word_book.h 中声明为 extern，这里不再重复声明
InkScreenSize setInkScreenSize;
TimerHandle_t inkScreenDebounceTimer = NULL;
uint8_t interfaceIndex = 1;  // 界面索引，已删除的测试变量但代码中仍然使用
// 全局图标数组
IconInfo g_available_icons[22] = {
    {ZHONGJINGYUAN_3_7_ICON_1, 62, 64},//0
    {ZHONGJINGYUAN_3_7_ICON_2, 64, 64},//1
    {ZHONGJINGYUAN_3_7_ICON_3, 86, 64},//2
    {ZHONGJINGYUAN_3_7_ICON_4, 71, 56},//3
    {ZHONGJINGYUAN_3_7_ICON_5, 76, 56},//4
    {ZHONGJINGYUAN_3_7_ICON_6, 94, 64},//5
    {ZHONGJINGYUAN_3_7_NAIL,15,16},//6
    {ZHONGJINGYUAN_3_7_LOCK,32,32},//7
    {ZHONGJINGYUAN_3_7_HORN,16,16},//8
    {ZHONGJINGYUAN_3_7_BATTERY_1,36,24},//9
    {ZHONGJINGYUAN_3_7_WIFI_DISCONNECT,32,32},//10
    {ZHONGJINGYUAN_3_7_WIFI_CONNECT,32,32},//11
    {ZHONGJINGYUAN_3_7_UNDERLINE,60,16},//12
    {ZHONGJINGYUAN_3_7_promt,320,36},//13
    {ZHONGJINGYUAN_3_7_wifi_battry,80,36},//14
    {ZHONGJINGYUAN_3_7_word,336,48},//15
    {ZHONGJINGYUAN_3_7_Translation1,416,24},//16
    {ZHONGJINGYUAN_3_7_separate,416,16},//17
    {ZHONGJINGYUAN_3_7_horn,80,16},//18
    {ZHONGJINGYUAN_3_7_pon,80,32},//19
    {ZHONGJINGYUAN_3_7_definition,416,72},//20
    {jpg_bw,224,400}//21
};

#define ICON_COUNT 220  // 图标总数（包括索引0）

// 全局界面管理器实例
ScreenManager g_screen_manager;

// ==================== JSON布局全局变量 ====================
// 保存当前JSON加载的矩形数据，用于按键交互
RectInfo* g_json_rects = nullptr;
int g_json_rect_count = 0;
int g_json_status_rect_index = -1;

// ==================== JSON函数前置声明 ====================
void saveJsonLayoutForInteraction(RectInfo* rects, int rect_count, int status_rect_index);
void redrawJsonLayout();
void jsonLayoutFocusNext();
void jsonLayoutFocusPrev();  
void jsonLayoutConfirm();

// 通过图标名称获取索引
int getIconIndexByName(const char* name) {
    if (!name) return -1;
    
    int count = sizeof(icon_mappings) / sizeof(icon_mappings[0]);
    for (int i = 0; i < count; i++) {
        if (strcmp(icon_mappings[i].name, name) == 0) {
            return icon_mappings[i].index;
        }
    }
    
    ESP_LOGW("JSON", "未找到图标名称: %s", name);
    return -1;
}

// 通过图标索引获取图标尺寸
void getIconSizeByIndex(int icon_index, int* width, int* height) {
    *width = 15;  // 默认宽度
    *height = 16; // 默认高度
    
    // 检查索引有效性
    if (icon_index < 0 || icon_index >= ICON_COUNT) {
        ESP_LOGW("ICON", "无效图标索引: %d", icon_index);
        return;
    }
    
    // 从图标数组获取实际尺寸
    const IconInfo* icon_info = &g_available_icons[icon_index];
    *width = icon_info->width;
    *height = icon_info->height;
    
    ESP_LOGD("ICON", "图标%d尺寸: %dx%d", icon_index, *width, *height);
}

// 通过图标索引获取图标数据指针
const uint8_t* getIconDataByIndex(int icon_index) {
    // 检查索引有效性
    if (icon_index < 0 || icon_index >= ICON_COUNT) {
        ESP_LOGW("ICON", "无效图标索引: %d, 使用默认图标", icon_index);
        return ZHONGJINGYUAN_3_7_NAIL;  // 返回默认图标
    }
    
    // 从图标数组获取图标数据
    return g_available_icons[icon_index].data;
}


// 通过图标索引获取图标文件名（用于从SPIFFS加载）
const char* getIconFileNameByIndex(int icon_index) {
    // 图标索引到文件名的映射表
    static const char* icon_files[] = {
        "/book.bin",          // 0
        "/game.bin",          // 1
        "/settings.bin",      // 2
        "/folder.bin",        // 3
        "/horn.bin",          // 4
        "/wifi_off.bin",    // 5
        "/battery.bin",    // 6
        "/word.bin",    // 7
        "/message.bin",    // 8

    };
    
    // 检查索引有效性
    if (icon_index < 0 || icon_index >= sizeof(icon_files)/sizeof(icon_files[0])) {
        ESP_LOGW("ICON", "无效图标索引: %d, 使用默认图标", icon_index);
        return "/book.bin";  // 返回默认图标文件（索引0）
    }
    
    return icon_files[icon_index];
}

/**
 * @brief 从SD卡预加载所有图标到PSRAM缓存
 * @return true 全部成功或部分成功, false 全部失败
 */
bool preloadIconsFromSD() {
    ESP_LOGI("ICON_CACHE", "开始预加载图标到PSRAM...");
    
    int success_count = 0;
    
    // 自动遍历所有icon_mappings中定义的图标
    for (int i = 0; i < ICON_CACHE_COUNT; i++) {
        // 根据icon_mappings生成文件路径
        const char* icon_file = getIconFileNameByIndex(i);
        
        // 获取图片信息
        uint32_t width, height;
        if (!getImageInfo(icon_file, &width, &height)) {
            ESP_LOGW("ICON_CACHE", "无法获取图标%d信息: %s", i, icon_file);
            continue;
        }
        
        // 计算需要的缓冲区大小
        uint32_t buffer_size = ((width + 7) / 8) * height;
        
        // 在PSRAM中分配内存
        uint8_t* buffer = (uint8_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
        if (!buffer) {
            ESP_LOGE("ICON_CACHE", "PSRAM分配失败: %d bytes for icon %d", buffer_size, i);
            // 尝试使用内部RAM作为备用
            buffer = (uint8_t*)malloc(buffer_size);
            if (!buffer) {
                ESP_LOGE("ICON_CACHE", "内存分配完全失败: icon %d", i);
                continue;
            }
            ESP_LOGW("ICON_CACHE", "使用内部RAM (PSRAM不足)");
        }
        
        // 加载图片数据到缓冲区
        if (loadImageToBuffer(icon_file, buffer, &width, &height)) {
            g_icon_cache[i].filename = icon_file;
            g_icon_cache[i].data = buffer;
            g_icon_cache[i].width = width;
            g_icon_cache[i].height = height;
            g_icon_cache[i].loaded = true;
            success_count++;
            ESP_LOGI("ICON_CACHE", "✅ 预加载图标%d成功: %s (%dx%d, %d bytes)", 
                    i, icon_file, width, height, buffer_size);
        } else {
            free(buffer);
            ESP_LOGE("ICON_CACHE", "❌ 加载图标%d失败: %s", i, icon_file);
        }
    }
    
    ESP_LOGI("ICON_CACHE", "预加载完成: %d/%d 个图标成功加载", success_count, ICON_CACHE_COUNT);
    return success_count > 0;
}

/**
 * @brief 从缓存获取图标数据
 * @param icon_index 图标索引 (0-3)
 * @param width 输出参数 - 图标宽度
 * @param height 输出参数 - 图标高度
 * @return 图标数据指针，失败返回nullptr
 */
const uint8_t* getIconDataFromCache(int icon_index, uint32_t* width, uint32_t* height) {
    if (icon_index < 0 || icon_index >= ICON_CACHE_COUNT) {
        ESP_LOGW("ICON_CACHE", "无效的图标索引: %d (范围: 0-%d)", icon_index, ICON_CACHE_COUNT-1);
        return nullptr;
    }
    
    if (!g_icon_cache[icon_index].loaded) {
        ESP_LOGW("ICON_CACHE", "图标%d未加载到缓存", icon_index);
        return nullptr;
    }
    
    *width = g_icon_cache[icon_index].width;
    *height = g_icon_cache[icon_index].height;
    return g_icon_cache[icon_index].data;
}

/**
 * @brief 释放所有图标缓存（关机时调用）
 */
void freeIconCache() {
    ESP_LOGI("ICON_CACHE", "释放图标缓存...");
    for (int i = 0; i < ICON_CACHE_COUNT; i++) {
        if (g_icon_cache[i].loaded && g_icon_cache[i].data) {
            free(g_icon_cache[i].data);
            g_icon_cache[i].data = nullptr;
            g_icon_cache[i].loaded = false;
            ESP_LOGI("ICON_CACHE", "释放图标%d缓存", i);
        }
    }
}

// ================== 图标数组定义 ==================
// 定义各种动画的图标序列（索引对应: 0=book, 1=game, 2=settings, 3=folder）
static const int cat_jump_sequence[] = {0, 1, 2, 3};  // 依次显示: book -> game -> settings -> folder
static const int cat_walk_sequence[] = {3, 2, 1, 0};  // 依次显示: folder -> settings -> game -> book
// 可以添加更多动画序列...



// 图标数组注册表
typedef struct {
    const char* name;      // 数组名称 (用于JSON中的"icon_arr")
    const char* var_name;  // 变量名称 (用于JSON中的"idx")
    const int* sequence;   // 图标索引序列
    int count;            // 序列长度
} IconArrayEntry;

static const IconArrayEntry g_icon_arrays[] = {
    {"cat_jump", "$cat_jump_idx", cat_jump_sequence, sizeof(cat_jump_sequence)/sizeof(cat_jump_sequence[0])},
    {"cat_walk", "$cat_walk_idx", cat_walk_sequence, sizeof(cat_walk_sequence)/sizeof(cat_walk_sequence[0])},
    // 新增图标数组只需要在这里添加一行即可！
};
static const int g_icon_arrays_count = sizeof(g_icon_arrays) / sizeof(g_icon_arrays[0]);

// ================== 文本数组定义 ==================
// 定义各种文本序列
static const char* message_remind_sequence[] = {"ss1", "提醒2", "提醒3", "注意"};
static const char* status_text_sequence[] = {"sss", "运行中", "完成", "错误"};
// 可以添加更多文本序列...

// ================== 提示信息缓存（PSRAM）==================
#define PROMPT_CACHE_COUNT 10  // 缓存最近10条提示信息

// 提示信息缓存（存储动态传入的提示文本）
static char* g_prompt_cache[PROMPT_CACHE_COUNT] = {nullptr};
// 提示信息指针数组（供text_arrays使用）
static const char* g_prompt_ptrs[PROMPT_CACHE_COUNT] = {nullptr};
// 当前提示信息索引（循环写入）
static int g_prompt_current_index = 0;
// 提示信息总数
static int g_prompt_total_count = 0;
// 提示信息是否已初始化
static bool g_prompt_initialized = false;

// ================== 番茄钟状态管理 ==================
static const char* g_pomodoro_state_texts[] = {"开始", "暂停"};
static const char** g_pomodoro_state_ptrs = g_pomodoro_state_texts;
static int g_pomodoro_state_idx = 0;  // 0=开始, 1=暂停
static bool g_pomodoro_running = false;
static int g_pomodoro_remaining_seconds = 180;  // 3分钟 = 180秒
static char g_pomodoro_time_text[32] = "03:00";
static const char* g_pomodoro_time_ptr = g_pomodoro_time_text;
static const char** g_pomodoro_time_ptrs = &g_pomodoro_time_ptr;  // 指针数组
static uint32_t g_pomodoro_last_update = 0;
static uint32_t g_pomodoro_last_refresh = 0;

// 番茄钟按钮文本
static const char* g_pomodoro_reset_text[] = {"重置"};
static const char** g_pomodoro_reset_ptrs = g_pomodoro_reset_text;
static int g_pomodoro_reset_idx = 0;

static const char* g_pomodoro_settings_text[] = {"设置"};
static const char** g_pomodoro_settings_ptrs = g_pomodoro_settings_text;
static int g_pomodoro_settings_idx = 0;

static int g_pomodoro_time_idx = 0;

// ================== 单词本文本缓存（PSRAM）==================
#define WORDBOOK_CACHE_COUNT 5  // 缓存的单词数量

// 单词各字段的文本缓存（存储格式化的字符串）
static char* g_wordbook_word_cache[WORDBOOK_CACHE_COUNT] = {nullptr};       // 单词本身
static char* g_wordbook_phonetic_cache[WORDBOOK_CACHE_COUNT] = {nullptr};   // 音标
static char* g_wordbook_translation_cache[WORDBOOK_CACHE_COUNT] = {nullptr}; // 翻译（完整）
static char* g_wordbook_translation1_cache[WORDBOOK_CACHE_COUNT] = {nullptr}; // 第一个释义
static char* g_wordbook_translation2_cache[WORDBOOK_CACHE_COUNT] = {nullptr}; // 第二个释义
static char* g_wordbook_pos_cache[WORDBOOK_CACHE_COUNT] = {nullptr};        // 词性

// 单词指针数组（供text_arrays使用）
static const char* g_wordbook_word_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
static const char* g_wordbook_phonetic_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
static const char* g_wordbook_translation_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
static const char* g_wordbook_translation1_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
static const char* g_wordbook_translation2_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};
static const char* g_wordbook_pos_ptrs[WORDBOOK_CACHE_COUNT] = {nullptr};

// 单词本是否已初始化
static bool g_wordbook_text_initialized = false;

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
    
    // 2. 加载前5个单词并分别格式化各个字段
    int loaded_count = 0;
    for (int i = 0; i < WORDBOOK_CACHE_COUNT; i++) {
        WordEntry* word = getNextWord();
        if (!word) {
            ESP_LOGW(TAG, "只加载了 %d 个单词", i);
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
                trans2 = trans_full.substring(first_newline + 2, second_newline);
            } else {
                trans2 = trans_full.substring(first_newline + 2);
            }
            trans2.trim();
            
            ESP_LOGI(TAG, "  -> 释义1: [%s]", trans1.c_str());
            ESP_LOGI(TAG, "  -> 释义2: [%s]", trans2.c_str());
        } else {
            // 没有 \n，整个作为第一个释义
            trans1 = trans_full;
            ESP_LOGI(TAG, "  -> 未找到\\n，全部作为释义1: [%s]", trans1.c_str());
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
                snprintf(g_wordbook_pos_cache[i], pos_len, "%s", word->pos.c_str());
            } else {
                g_wordbook_pos_cache[i][0] = '\0';  // 词性为空时设置为空字符串
            }
            g_wordbook_pos_ptrs[i] = g_wordbook_pos_cache[i];
        }
        
        loaded_count++;
        
        ESP_LOGI(TAG, "  [%d] %s %s - %s", i, 
                 g_wordbook_word_ptrs[i], 
                 g_wordbook_phonetic_ptrs[i],
                 g_wordbook_pos_ptrs[i] ? g_wordbook_pos_ptrs[i] : "(no pos)");
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
 * @brief 获取单词本文本指针（用于text_roll）
 * @param index 单词索引 (0-4)
 * @return 单词文本指针，失败返回"ERR"
 * @deprecated 请使用 getWordBookWord/Phonetic/Translation/Pos
 */
const char* getWordBookText(int index) {
    return getWordBookWord(index);  // 默认返回单词本身
}

// 文本数组注册表
typedef struct {
    const char* name;        // 数组名称 (用于JSON中的"text_arr")
    const char* var_name;    // 变量名称 (用于JSON中的"idx")
    const char** sequence;   // 文本序列
    int count;              // 序列长度
} TextArrayEntry;

static const TextArrayEntry g_text_arrays[] = {
    {"message_remind", "$message_idx", message_remind_sequence, sizeof(message_remind_sequence)/sizeof(message_remind_sequence[0])},
    {"status_text", "$status_idx", status_text_sequence, sizeof(status_text_sequence)/sizeof(status_text_sequence[0])},
    {"wordbook_word", "$wordbook_idx", g_wordbook_word_ptrs, WORDBOOK_CACHE_COUNT},           // 单词本身
    {"wordbook_phonetic", "$wordbook_idx", g_wordbook_phonetic_ptrs, WORDBOOK_CACHE_COUNT},   // 音标
    {"wordbook_translation", "$wordbook_idx", g_wordbook_translation_ptrs, WORDBOOK_CACHE_COUNT}, // 完整翻译
    {"wordbook_translation_1", "$wordbook_idx", g_wordbook_translation1_ptrs, WORDBOOK_CACHE_COUNT}, // 第一个释义
    {"wordbook_translation_2", "$wordbook_idx", g_wordbook_translation2_ptrs, WORDBOOK_CACHE_COUNT}, // 第二个释义
    {"wordbook_pos", "$wordbook_idx", g_wordbook_pos_ptrs, WORDBOOK_CACHE_COUNT},            // 词性
    {"prompt_messages", "$prompt_idx", g_prompt_ptrs, PROMPT_CACHE_COUNT},                   // 提示信息
    {"pomodoro_state", "$pomodoro_state_idx", g_pomodoro_state_ptrs, 2},                     // 番茄钟状态
    {"pomodoro_time_text", "$pomodoro_time_idx", g_pomodoro_time_ptrs, 1},                   // 番茄钟时间
    {"pomodoro_reset_text", "$pomodoro_reset_idx", g_pomodoro_reset_ptrs, 1},                // 重置按钮
    {"pomodoro_settings_text", "$pomodoro_settings_idx", g_pomodoro_settings_ptrs, 1},       // 设置按钮
    // 新增文本数组只需要在这里添加一行即可！
};
static const int g_text_arrays_count = sizeof(g_text_arrays) / sizeof(g_text_arrays[0]);

// 全局索引数组，对应g_icon_arrays中每个动画的当前帧索引
static int g_animation_indices[sizeof(g_icon_arrays) / sizeof(g_icon_arrays[0])] = {0};

// 全局索引数组，对应g_text_arrays中每个文本动画的当前索引
static int g_text_animation_indices[sizeof(g_text_arrays) / sizeof(g_text_arrays[0])] = {0};

// ==================== 提示信息管理函数 ====================

/**
 * @brief 初始化提示信息缓存
 */
void initPromptCache() {
    if (g_prompt_initialized) {
        return;
    }
    
    // 初始化所有缓存为空字符串
    for (int i = 0; i < PROMPT_CACHE_COUNT; i++) {
        #if CONFIG_ESP32S3_SPIRAM_SUPPORT || CONFIG_SPIRAM
        g_prompt_cache[i] = (char*)heap_caps_malloc(256, MALLOC_CAP_SPIRAM);
        #else
        g_prompt_cache[i] = (char*)malloc(256);
        #endif
        
        if (g_prompt_cache[i]) {
            strcpy(g_prompt_cache[i], "--");
            g_prompt_ptrs[i] = g_prompt_cache[i];
        } else {
            ESP_LOGE(TAG, "提示信息缓存[%d]分配失败", i);
        }
    }
    
    g_prompt_current_index = 0;
    g_prompt_total_count = 0;
    g_prompt_initialized = true;
    
    ESP_LOGI(TAG, "✅ 提示信息缓存已初始化 (%d条)", PROMPT_CACHE_COUNT);
}

/**
 * @brief 添加新的提示信息到缓存（循环队列）
 * @param prompt 提示信息文本
 */
void addPromptToCache(const char* prompt) {
    if (!g_prompt_initialized) {
        initPromptCache();
    }
    
    if (!prompt || strlen(prompt) == 0) {
        return;
    }
    
    // 写入当前索引位置
    if (g_prompt_cache[g_prompt_current_index]) {
        strncpy(g_prompt_cache[g_prompt_current_index], prompt, 255);
        g_prompt_cache[g_prompt_current_index][255] = '\0';
        
        ESP_LOGI("PROMPT_CACHE", "📝 添加提示[%d]: %s", g_prompt_current_index, prompt);
        ESP_LOGI("PROMPT_CACHE", "🔍 验证写入: %s", g_prompt_cache[g_prompt_current_index]);
        ESP_LOGI("PROMPT_CACHE", "🔍 验证指针: %s", g_prompt_ptrs[g_prompt_current_index] ? g_prompt_ptrs[g_prompt_current_index] : "NULL");
        
        // 更新 $prompt_idx 索引，指向刚写入的位置（当前位置）
        for (int i = 0; i < g_text_arrays_count; i++) {
            if (strcmp(g_text_arrays[i].var_name, "$prompt_idx") == 0) {
                g_text_animation_indices[i] = g_prompt_current_index;
                ESP_LOGI("PROMPT_CACHE", "✅ 更新索引: $prompt_idx -> %d", g_prompt_current_index);
                break;
            }
        }
    }
    
    // 更新索引（循环）
    g_prompt_current_index = (g_prompt_current_index + 1) % PROMPT_CACHE_COUNT;
    
    // 更新总数（最多PROMPT_CACHE_COUNT）
    if (g_prompt_total_count < PROMPT_CACHE_COUNT) {
        g_prompt_total_count++;
    }
    
    // 刷新屏幕显示最新提示信息
    if (g_json_rects && g_json_rect_count > 0) {
        redrawJsonLayout();
        ESP_LOGI("PROMPT_CACHE", "✅ 屏幕已刷新显示最新提示");
    }
}

/**
 * @brief 释放提示信息缓存
 */
void freePromptCache() {
    if (!g_prompt_initialized) {
        return;
    }
    
    for (int i = 0; i < PROMPT_CACHE_COUNT; i++) {
        if (g_prompt_cache[i]) {
            free(g_prompt_cache[i]);
            g_prompt_cache[i] = nullptr;
        }
        g_prompt_ptrs[i] = nullptr;
    }
    
    g_prompt_current_index = 0;
    g_prompt_total_count = 0;
    g_prompt_initialized = false;
    
    ESP_LOGI(TAG, "提示信息缓存已释放");
}

/**
 * @brief 获取提示信息总数
 */
int getPromptCount() {
    return g_prompt_total_count;
}

/**
 * @brief 获取最新的提示信息
 */
const char* getLatestPrompt() {
    if (!g_prompt_initialized || g_prompt_total_count == 0) {
        return "--";
    }
    
    int latest = (g_prompt_current_index - 1 + PROMPT_CACHE_COUNT) % PROMPT_CACHE_COUNT;
    return g_prompt_ptrs[latest];
}

// ==================== 番茄钟管理函数 ====================

/**
 * @brief 更新番茄钟时间显示文本
 */
void updatePomodoroTimeText() {
    int minutes = g_pomodoro_remaining_seconds / 60;
    int seconds = g_pomodoro_remaining_seconds % 60;
    snprintf(g_pomodoro_time_text, sizeof(g_pomodoro_time_text), "%02d:%02d", minutes, seconds);
}

/**
 * @brief 初始化番茄钟
 */
void initPomodoro() {
    g_pomodoro_state_idx = 0;  // 开始状态
    g_pomodoro_reset_idx = 0;
    g_pomodoro_settings_idx = 0;
    g_pomodoro_time_idx = 0;
    g_pomodoro_running = false;
    g_pomodoro_remaining_seconds = 180;  // 3分钟
    updatePomodoroTimeText();
    g_pomodoro_last_update = millis();
    g_pomodoro_last_refresh = millis();
    
    // 同步所有番茄钟相关的文本动画索引
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(g_text_arrays[i].var_name, "$pomodoro_state_idx") == 0) {
            g_text_animation_indices[i] = g_pomodoro_state_idx;
        } else if (strcmp(g_text_arrays[i].var_name, "$pomodoro_reset_idx") == 0) {
            g_text_animation_indices[i] = g_pomodoro_reset_idx;
        } else if (strcmp(g_text_arrays[i].var_name, "$pomodoro_settings_idx") == 0) {
            g_text_animation_indices[i] = g_pomodoro_settings_idx;
        } else if (strcmp(g_text_arrays[i].var_name, "$pomodoro_time_idx") == 0) {
            g_text_animation_indices[i] = g_pomodoro_time_idx;
        }
    }
    
    ESP_LOGI("POMODORO", "番茄钟已初始化: 3分钟");
}

/**
 * @brief 番茄钟主循环更新
 */
void updatePomodoro() {
    if (!g_pomodoro_running) {
        return;
    }
    
    uint32_t current_time = millis();
    
    // 每秒更新一次倒计时
    if (current_time - g_pomodoro_last_update >= 1000) {
        g_pomodoro_last_update = current_time;
        
        if (g_pomodoro_remaining_seconds > 0) {
            g_pomodoro_remaining_seconds--;
            updatePomodoroTimeText();
            
            ESP_LOGD("POMODORO", "剩余时间: %s", g_pomodoro_time_text);
        } else {
            // 倒计时结束
            g_pomodoro_running = false;
            g_pomodoro_state_idx = 0;  // 切换回"开始"状态
            ESP_LOGI("POMODORO", "番茄钟完成!");
            
            // 立即刷新显示
            if (g_json_rects && g_json_rect_count > 0) {
                redrawJsonLayout();
            }
        }
    }
    
    // 每20秒刷新一次屏幕
    if (current_time - g_pomodoro_last_refresh >= 20000) {
        g_pomodoro_last_refresh = current_time;
        
        if (g_json_rects && g_json_rect_count > 0) {
            ESP_LOGI("POMODORO", "刷新屏幕显示: %s", g_pomodoro_time_text);
            redrawJsonLayout();
        }
    }
}

/**
 * @brief 番茄钟开始/暂停操作
 */
void pomodoroStartPause() {
    g_pomodoro_running = !g_pomodoro_running;
    g_pomodoro_state_idx = g_pomodoro_running ? 1 : 0;  // 1=暂停, 0=开始
    
    if (g_pomodoro_running) {
        ESP_LOGI("POMODORO", "开始倒计时: %s", g_pomodoro_time_text);
        g_pomodoro_last_update = millis();
        g_pomodoro_last_refresh = millis();
    } else {
        ESP_LOGI("POMODORO", "暂停倒计时: %s", g_pomodoro_time_text);
    }
    
    // 更新$pomodoro_state_idx索引
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(g_text_arrays[i].var_name, "$pomodoro_state_idx") == 0) {
            g_text_animation_indices[i] = g_pomodoro_state_idx;
            break;
        }
    }
    
    // 立即刷新屏幕
    if (g_json_rects && g_json_rect_count > 0) {
        redrawJsonLayout();
    }
}

/**
 * @brief 番茄钟重置操作
 */
void pomodoroReset() {
    g_pomodoro_running = false;
    g_pomodoro_state_idx = 0;  // 开始状态
    g_pomodoro_remaining_seconds = 180;  // 重置为3分钟
    updatePomodoroTimeText();
    
    ESP_LOGI("POMODORO", "番茄钟已重置");
    
    // 更新$pomodoro_state_idx索引
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(g_text_arrays[i].var_name, "$pomodoro_state_idx") == 0) {
            g_text_animation_indices[i] = g_pomodoro_state_idx;
            break;
        }
    }
    
    // 立即刷新屏幕
    if (g_json_rects && g_json_rect_count > 0) {
        redrawJsonLayout();
    }
}

/**
 * @brief 番茄钟设置操作（预留）
 */
void pomodoroSettings() {
    ESP_LOGI("POMODORO", "打开设置界面（待实现）");
    // TODO: 实现设置界面，允许调整时长等参数
}

// auto_roll定时器相关变量
static unsigned long g_last_auto_roll_time = 0;
static const unsigned long AUTO_ROLL_INTERVAL = 2000; // 2000ms间隔（减少内存压力）

/**
 * @brief 根据变量名获取当前索引值
 */
int getVariableIndex(const char* var_name) {
    if (!var_name) return 0;
    
    // 先在图标数组注册表中查找
    for (int i = 0; i < g_icon_arrays_count; i++) {
        if (strcmp(var_name, g_icon_arrays[i].var_name) == 0) {
            return g_animation_indices[i];
        }
    }
    
    // 再在文本数组注册表中查找
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(var_name, g_text_arrays[i].var_name) == 0) {
            return g_text_animation_indices[i];
        }
    }
    
    ESP_LOGW("ICON_ROLL", "未知变量: %s", var_name);
    return 0;
}

/**
 * @brief 根据icon_roll配置获取当前应该显示的图标索引
 */
int getIconRollCurrentIndex(const IconRollInRect* icon_roll) {
    if (!icon_roll) return -1;
    
    // 获取当前索引值
    int current_idx = getVariableIndex(icon_roll->idx);
    
    // 在图标数组注册表中查找对应的数组
    for (int i = 0; i < g_icon_arrays_count; i++) {
        if (strcmp(icon_roll->icon_arr, g_icon_arrays[i].name) == 0) {
            const IconArrayEntry* entry = &g_icon_arrays[i];
            int frame_idx = current_idx % entry->count;
            int icon_index = entry->sequence[frame_idx];
            
            ESP_LOGI("ICON_ROLL", "数组[%s] 索引[%d] -> 图标[%d]", 
                    entry->name, frame_idx, icon_index);
            return icon_index;
        }
    }
    
    ESP_LOGW("ICON_ROLL", "未找到图标数组: %s", icon_roll->icon_arr);
    return -1;
}

/**
 * @brief 根据text_roll配置获取当前应该显示的文本
 */
const char* getTextRollCurrentText(const TextRollInRect* text_roll) {
    if (!text_roll) return "ERR";
    
    // 获取当前索引值
    int current_idx = getVariableIndex(text_roll->idx);
    
    ESP_LOGI("TEXT_ROLL", "🔍 查找文本数组: %s, 变量: %s, 索引: %d", 
             text_roll->text_arr, text_roll->idx, current_idx);
    
    // 在文本数组注册表中查找对应的数组
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(text_roll->text_arr, g_text_arrays[i].name) == 0) {
            const TextArrayEntry* entry = &g_text_arrays[i];
            int text_idx = current_idx % entry->count;
            const char* text = entry->sequence[text_idx];
            
            ESP_LOGI("TEXT_ROLL", "✅ 数组[%s] 索引[%d/%d] -> 文本[%s]", 
                    entry->name, text_idx, entry->count, text ? text : "NULL");
            return text ? text : "--";
        }
    }
    
    ESP_LOGW("TEXT_ROLL", "❌ 未找到文本数组: %s", text_roll->text_arr);
    return "N/A";
}

/**
 * @brief 更新所有动态图标索引（自动递增）
 */
void updateIconRollIndices() {
    for (int i = 0; i < g_icon_arrays_count; i++) {
        g_animation_indices[i] = (g_animation_indices[i] + 1) % g_icon_arrays[i].count;
        ESP_LOGI("ICON_ROLL", "更新%s: %d", g_icon_arrays[i].var_name, g_animation_indices[i]);
    }
}

/**
 * @brief 根据变量名更新特定动画的索引（支持图标和文本）
 */
void updateSpecificRoll(const char* var_name) {
    if (!var_name) return;
    
    // 先尝试更新图标动画
    for (int i = 0; i < g_icon_arrays_count; i++) {
        if (strcmp(var_name, g_icon_arrays[i].var_name) == 0) {
            g_animation_indices[i] = (g_animation_indices[i] + 1) % g_icon_arrays[i].count;
            ESP_LOGI("ICON_ROLL", "更新特定图标动画%s: %d", var_name, g_animation_indices[i]);
            return;
        }
    }
    
    // 再尝试更新文本动画
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(var_name, g_text_arrays[i].var_name) == 0) {
            g_text_animation_indices[i] = (g_text_animation_indices[i] + 1) % g_text_arrays[i].count;
            ESP_LOGI("TEXT_ROLL", "更新特定文本动画%s: %d", var_name, g_text_animation_indices[i]);
            return;
        }
    }
}

/**
 * @brief 检查并处理auto_roll动画（需要定期调用）
 */
void processAutoRollAnimations() {
    if (!g_json_rects || g_json_rect_count == 0) return;
    
    unsigned long current_time = millis();
    if (current_time - g_last_auto_roll_time < AUTO_ROLL_INTERVAL) {
        return; // 还没到间隔时间
    }
    
    bool need_refresh = false;
    
    // 遍历所有矩形的icon_roll和text_roll，检查哪些需要auto_roll
    for (int rect_idx = 0; rect_idx < g_json_rect_count; rect_idx++) {
        RectInfo* rect = &g_json_rects[rect_idx];
        
        // 检查icon_roll
        for (int roll_idx = 0; roll_idx < rect->icon_roll_count; roll_idx++) {
            IconRollInRect* icon_roll = &rect->icon_rolls[roll_idx];
            
            if (icon_roll->auto_roll) {
                updateSpecificRoll(icon_roll->idx);
                need_refresh = true;
                ESP_LOGI("ICON_ROLL", "自动滚动图标: %s", icon_roll->icon_arr);
            }
        }
        
        // 检查text_roll
        for (int roll_idx = 0; roll_idx < rect->text_roll_count; roll_idx++) {
            TextRollInRect* text_roll = &rect->text_rolls[roll_idx];
            
            if (text_roll->auto_roll) {
                updateSpecificRoll(text_roll->idx);
                need_refresh = true;
                ESP_LOGI("TEXT_ROLL", "自动滚动文本: %s", text_roll->text_arr);
            }
        }
    }
    
    // 如果有动画更新，刷新显示（使用更长间隔减少内存压力）
    if (need_refresh) {
        updateDisplayWithMain(g_json_rects, g_json_rect_count, -1, 1);
        ESP_LOGI("AUTO_ROLL", "动画已更新并刷新显示");
    }
    
    g_last_auto_roll_time = current_time;
}

// ================= Example onConfirm callbacks and registry =================
// 示例回调1：打开菜单（示例，实际实现可替换）
void onConfirmOpenMenu(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "示例回调：打开菜单，矩形 %d", idx);
    // TODO: 在此处添加实际打开菜单的逻辑
}

// 示例回调2：播放提示音（示例）
void onConfirmPlaySound(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "示例回调：播放提示音，矩形 %d", idx);
    // TODO: 在此处添加实际播放声音的逻辑
}

// 单词本：下一个单词
void onConfirmNextWord(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到下一个单词，矩形 %d", idx);
    
    if (!g_wordbook_text_initialized) {
        ESP_LOGW("WORDBOOK", "单词本文本缓存未初始化");
        return;
    }
    
    // 在文本数组注册表中查找 $wordbook_idx
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(g_text_arrays[i].var_name, "$wordbook_idx") == 0) {
            int old_index = g_text_animation_indices[i];
            int new_index = (old_index + 1) % g_text_arrays[i].count;
            g_text_animation_indices[i] = new_index;
            
            ESP_LOGI("WORDBOOK", "单词本索引已更新: %d -> %d (共%d个单词)", 
                     old_index, new_index, g_text_arrays[i].count);
            ESP_LOGI("WORDBOOK", "  当前单词: %s", getWordBookWord(new_index));
            ESP_LOGI("WORDBOOK", "  音标: %s", getWordBookPhonetic(new_index));
            ESP_LOGI("WORDBOOK", "  翻译: %s", getWordBookTranslation(new_index));
            
            // 刷新显示
            // if (g_json_rects && g_json_rect_count > 0) {
            //     updateDisplayWithMain(g_json_rects, g_json_rect_count, -1, 1);
            //     ESP_LOGI("WORDBOOK", "界面已刷新显示新单词");
            // }
            return;
        }
    }
    
    ESP_LOGW("WORDBOOK", "未找到$wordbook_idx变量");
}

// 界面切换：切换到第一个界面（layout.json）
void onConfirmSwitchToLayout0(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面0 (layout.json)，矩形 %d", idx);
    
    if (switchToScreen(0)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面0: %s", getScreenName(0));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面0失败");
    }
}

// 界面切换：切换到第二个界面（layout_1.json）
void onConfirmSwitchToLayout1(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面1 (layout_1.json)，矩形 %d", idx);
    
    if (switchToScreen(1)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面1: %s", getScreenName(1));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面1失败");
    }
}

// 番茄钟回调：开始/暂停
void onConfirmPomodoroStartPause(RectInfo* rect, int idx) {
    ESP_LOGI("POMODORO", "番茄钟开始/暂停按钮被按下");
    pomodoroStartPause();
}

// 番茄钟回调：重置
void onConfirmPomodoroReset(RectInfo* rect, int idx) {
    ESP_LOGI("POMODORO", "番茄钟重置按钮被按下");
    pomodoroReset();
}

// 番茄钟回调：设置
void onConfirmPomodoroSettings(RectInfo* rect, int idx) {
    ESP_LOGI("POMODORO", "番茄钟设置按钮被按下");
    pomodoroSettings();
}

// 动作注册表
ActionEntry g_action_registry[] = {
    {"open_menu", "打开菜单", onConfirmOpenMenu},
    {"play_sound", "播放提示音", onConfirmPlaySound},
    {"next_word", "下一个单词", onConfirmNextWord},
    {"switch_to_layout_0", "切换到界面0", onConfirmSwitchToLayout0},
    {"switch_to_layout_1", "切换到界面1", onConfirmSwitchToLayout1},
    {"pomodoro_start_pause", "番茄钟开始/暂停", onConfirmPomodoroStartPause},
    {"pomodoro_reset", "番茄钟重置", onConfirmPomodoroReset},
    {"pomodoro_settings", "番茄钟设置", onConfirmPomodoroSettings}
};
int g_action_registry_count = sizeof(g_action_registry) / sizeof(g_action_registry[0]);

// 通过动作ID查找函数指针（返回NULL表示未找到）
OnConfirmFn find_action_by_id(const char* id) {
    if (!id) return NULL;
    for (int i = 0; i < g_action_registry_count; i++) {
        if (g_action_registry[i].id && strcmp(g_action_registry[i].id, id) == 0) {
            return g_action_registry[i].fn;
        }
    }
    return NULL;
}

// 清除上次绘制的下划线
void clearLastUnderline() {
    if (g_last_underline_width > 0) {
        ESP_LOGI(TAG, "开始清除上次下划线: 位置(%d,%d), 宽度%d", 
                g_last_underline_x, g_last_underline_y, g_last_underline_width);
        
        // 用白色矩形覆盖上次的下划线
        display.fillRect(g_last_underline_x, g_last_underline_y, 
                        g_last_underline_width, 3, GxEPD_WHITE);
        
        ESP_LOGI(TAG, "清除上次下划线完成");
        // 重置记录
        g_last_underline_width = 0;
    }
}

void drawUnderlineForIconEx(int icon_index) {
    if (icon_index < 0 || icon_index >= MAX_GLOBAL_ICONS) {
        ESP_LOGE(TAG, "无效的图标索引: %d", icon_index);
        return;
    }
    
    // 获取图标位置信息
    IconPosition* icon = &g_icon_positions[icon_index];
    
    if (icon->width == 0 || icon->height == 0) {
        ESP_LOGE(TAG, "图标%d位置未初始化", icon_index);
        return;
    }
    
    // 调试：显示图标信息
    ESP_LOGI(TAG, "图标%d信息: 原始坐标(%d,%d), 原始尺寸(%dx%d)", 
            icon_index, icon->x, icon->y, icon->width, icon->height);
    
    // 初始化显示（确保可以绘制）
    display.setFullWindow();
    display.firstPage();
    {
        display.fillScreen(GxEPD_WHITE);
        
        // 清除上次的下划线（如果有）
        clearLastUnderline();
        
        // 计算缩放比例
        float scale_x = (float)display.width() / 416.0f;
        float scale_y = (float)display.height() / 240.0f;
        float scale = (scale_x < scale_y) ? scale_x : scale_y;
        
        ESP_LOGI(TAG, "屏幕尺寸: %dx%d, 缩放比例: X=%.2f, Y=%.2f, 使用: %.2f", 
                display.width(), display.height(), 
                scale_x, scale_y, scale);
        
        // 计算实际显示位置
        int x = (int)(icon->x * scale);
        int y = (int)(icon->y * scale);
        int width = (int)(icon->width * scale);
        int height = (int)(icon->height * scale);
        
        // 调试：显示计算后的位置
        ESP_LOGI(TAG, "计算后位置: X=%d, Y=%d, 宽度=%d, 高度=%d", x, y, width, height);
        
        // 绘制下划线（在图标下方，跟图标同宽）
        int underline_y = y + height + 3;  // 图标下方3像素
        
        // 绘制2像素粗的线
        display.drawLine(x, underline_y, x + width, underline_y, GxEPD_BLACK);
        display.drawLine(x, underline_y + 1, x + width, underline_y + 1, GxEPD_BLACK);
        
        // 记录这次的下划线位置（用于下次清除）
        g_last_underline_x = x;
        g_last_underline_y = underline_y;
        g_last_underline_width = width;
    }
    
    // 执行单次刷新
    display.nextPage();
    
    // 进入部分刷新模式
    display.setPartialWindow(0, 0, display.width(), display.height());
    
    ESP_LOGI(TAG, "在图标%d下方绘制下划线完成: 位置(%d,%d), 宽度%d", 
            icon_index, g_last_underline_x, g_last_underline_y, g_last_underline_width);
    
    // 短暂延迟，确保显示更新
    vTaskDelay(100 / portTICK_PERIOD_MS);
}


// 清除指定坐标范围的区域（通用方法）
void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    // 确保坐标在屏幕范围内
    if (start_x > display.width()) start_x = display.width();
    if (start_y > display.height()) start_y = display.height();
    if (end_x > display.width()) end_x = display.width();
    if (end_y > display.height()) end_y = display.height();
    
    // 使用GXEPD2填充白色矩形
    display.fillRect(start_x, start_y, end_x - start_x, end_y - start_y, GxEPD_WHITE);
    
    ESP_LOGI(TAG, "清除显示区域: (%d,%d)到(%d,%d)", start_x, start_y, end_x, end_y);
}

/**
 * @brief 四舍五入浮点数到整数
 */
int round_float(float x) {
    return (int)(x + 0.5f);
}

/**
 * @brief 求最大值
 */
int max_int(int a, int b) {
    return (a > b) ? a : b;
}

/**
 * @brief 求最小值
 */
int min_int(int a, int b) {
    return (a < b) ? a : b;
}


int icon_totalCount = sizeof(g_available_icons) / sizeof(g_available_icons[0]);


// 
void initIconPositions() {
    for (int i = 0; i < 6; i++) {
        g_icon_positions[i].x = 0;
        g_icon_positions[i].y = 0;
        g_icon_positions[i].width = 0;
        g_icon_positions[i].height = 0;
        g_icon_positions[i].selected = false;
        g_icon_positions[i].icon_index = -1;
        g_icon_positions[i].data = NULL;
    }
    g_selected_icon_index = -1;
    g_global_icon_count = 0;
}
// 获取状态栏图标位置（独立函数）
void getStatusIconPositions(RectInfo *rects, int rect_count, int status_rect_index,
                           int* wifi_x, int* wifi_y, 
                           int* battery_x, int* battery_y) {
    const int wifi_orig_width = 32;
    const int wifi_orig_height = 32;
    const int battery_orig_width = 36;
    const int battery_orig_height = 24;
    const int status_spacing = 5;
    
    // 默认位置（右上角）
    int default_margin = 5;
    *wifi_x = 416 - default_margin - wifi_orig_width;
    *wifi_y = default_margin;
    *battery_x = *wifi_x - battery_orig_width - status_spacing;
    *battery_y = default_margin + (wifi_orig_height - battery_orig_height) / 2;
    
    // 如果指定了状态栏矩形，计算在矩形内的位置
    if (status_rect_index >= 0 && status_rect_index < rect_count) {
        RectInfo* status_rect = &rects[status_rect_index];
        
        if (status_rect->width >= (wifi_orig_width + battery_orig_width + status_spacing) &&
            status_rect->height >= (wifi_orig_height > battery_orig_height ? wifi_orig_height : battery_orig_height)) {
            
            *battery_x = status_rect->x + status_rect->width - battery_orig_width;
            *battery_y = status_rect->y + (status_rect->height - battery_orig_height) / 2;
            *wifi_x = *battery_x - wifi_orig_width - status_spacing;
            *wifi_y = status_rect->y + (status_rect->height - wifi_orig_height) / 2;
            
            if (*wifi_x < status_rect->x) {
                *wifi_x = status_rect->x;
                *battery_x = *wifi_x + wifi_orig_width + status_spacing;
            }
        }
    }
    
    // 边界检查
    if (*wifi_x < 0) *wifi_x = 0;
    if (*wifi_y < 0) *wifi_y = 0;
    if (*battery_x < 0) *battery_x = 0;
    if (*battery_y < 0) *battery_y = 0;
}

// 为单个矩形写入指定图标到指定位置
// rect: 目标矩形
// icon_index: 图标索引（0-5对应6个图标）
// rel_x: 相对x位置 (0.0-1.0)
// rel_y: 相对y位置 (0.0-1.0)
// 返回: 成功返回图标在全局数组中的索引，失败返回-1
int populateRectWithIcon(RectInfo* rect, int icon_index, float rel_x, float rel_y) {
    // 检查参数有效性
    if (rect == NULL) {
        ESP_LOGE("POPULATE_RECT", "矩形指针为空");
        return -1;
    }
    
    if (icon_index < 0 || icon_index >= (int)(sizeof(g_available_icons) / sizeof(IconInfo))) {
        ESP_LOGE("POPULATE_RECT", "图标索引无效: %d", icon_index);
        return -1;
    }
    
    if (rel_x < 0.0f || rel_x > 1.0f || rel_y < 0.0f || rel_y > 1.0f) {
        ESP_LOGE("POPULATE_RECT", "相对位置无效: (%.2f, %.2f)", rel_x, rel_y);
        return -1;
    }
    
    // 检查是否还有空间存储图标
    if (g_global_icon_count >= MAX_GLOBAL_ICONS) {
        ESP_LOGE("POPULATE_RECT", "已达到最大图标数量(20个)");
        return -1;
    }
    
    // 获取图标信息
    IconInfo* icon = &g_available_icons[icon_index];
    
    // 计算图标在矩形内的具体位置（基于左上角对齐）
    // rel_x, rel_y 表示图标左上角在矩形中的相对位置
    // 例如：rel_x=0.0, rel_y=0.0 表示图标左上角在矩形左上角
    int icon_x = rect->x + (int)(rel_x * rect->width);
    int icon_y = rect->y + (int)(rel_y * rect->height);
    // 边界检查
    // if (icon_x < 0) {
    //     ESP_LOGW("POPULATE_RECT", "图标x坐标调整: %d -> 0", icon_x);
    //     icon_x = 0;
    // }
    
    // if (icon_y < 0) {
    //     ESP_LOGW("POPULATE_RECT", "图标y坐标调整: %d -> 0", icon_y);
    //     icon_y = 0;
    // }
    
    // if (icon_x + icon->width > 416) {
    //     int new_x = 416 - icon->width;
    //     ESP_LOGW("POPULATE_RECT", "图标x坐标超出屏幕右边界: %d -> %d", icon_x, new_x);
    //     icon_x = (new_x > 0) ? new_x : 0;
    // }
    
    // if (icon_y + icon->height > 240) {
    //     int new_y = 240 - icon->height;
    //     ESP_LOGW("POPULATE_RECT", "图标y坐标超出屏幕下边界: %d -> %d", icon_y, new_y);
    //     icon_y = (new_y > 0) ? new_y : 0;
    // }
    // 修改边界检查：
    if (icon_x < 0) {
        ESP_LOGW("POPULATE_RECT", "图标x坐标调整: %d -> 0", icon_x);
        icon_x = 0;
    }

    if (icon_y < 0) {
        ESP_LOGW("POPULATE_RECT", "图标y坐标调整: %d -> 0", icon_y);
        icon_y = 0;
    }

    if (icon_x + icon->width > 416) {
        int new_x = 416 - icon->width;
        ESP_LOGW("POPULATE_RECT", "图标x坐标超出屏幕右边界: %d -> %d", icon_x, new_x);
        icon_x = (new_x > 0) ? new_x : 0;
    }

    if (icon_y + icon->height > 240) {
        int new_y = 240 - icon->height;
        ESP_LOGW("POPULATE_RECT", "图标y坐标超出屏幕下边界: %d -> %d", icon_y, new_y);
        icon_y = (new_y > 0) ? new_y : 0;
    }
    // 记录到全局图标位置数组
    int global_index = g_global_icon_count;
    g_icon_positions[global_index].x = (uint16_t)icon_x;
    g_icon_positions[global_index].y = (uint16_t)icon_y;
    g_icon_positions[global_index].width = (uint16_t)icon->width;
    g_icon_positions[global_index].height = (uint16_t)icon->height;
    g_icon_positions[global_index].selected = false;
    g_icon_positions[global_index].icon_index = icon_index;
    g_icon_positions[global_index].data = icon->data;
    
    g_global_icon_count++;
    
ESP_LOGI("POPULATE_RECT", 
        "矩形[%d,%d %dx%d] 图标尺寸[%dx%d] rel=(%.2f,%.2f) 计算: x=%d+(%d*%.2f)=%d, y=%d+(%d*%.2f)=%d",
        rect->x, rect->y, rect->width, rect->height,
        icon->width, icon->height,
        rel_x, rel_y,
        rect->x, rect->width, rel_x, icon_x,
        rect->y, rect->height, rel_y, icon_y);
    
    return global_index;
}

void populateRectsWithCustomIcons(RectInfo *rects, int rect_count, 
                                  IconConfig* icon_configs, int config_count) {
    // 重置全局图标计数
    initIconPositions();
    
    for (int i = 0; i < config_count && g_global_icon_count < 6; i++) {
        IconConfig* config = &icon_configs[i];
        
        // 检查矩形索引是否有效
        if (config->rect_index < 0 || config->rect_index >= rect_count) {
            ESP_LOGE("POPULATE_CUSTOM", "矩形索引无效: %d", config->rect_index);
            continue;
        }
        
        // 填充图标
        int result = populateRectWithIcon(&rects[config->rect_index], 
                                        config->icon_index, 
                                        config->rel_x, 
                                        config->rel_y);
        
        if (result < 0) {
            ESP_LOGE("POPULATE_CUSTOM", "填充图标失败: 矩形%d, 图标%d", 
                    config->rect_index, config->icon_index);
        }
    }
}



/**
 * @brief 添加文本内容到矩形
 */
bool addTextToRect(RectInfo* rects, int rect_index, RectContentType content_type,
                   float rel_x, float rel_y, uint8_t font_size,
                   TextAlignment h_align, TextAlignment v_align) {
    if (rects == nullptr || rect_index < 0) {
        ESP_LOGE("TEXT", "参数无效");
        return false;
    }
    
    RectInfo* rect = &rects[rect_index];
    
    if (rect->text_count >= 4) {
        ESP_LOGE("TEXT", "矩形%d已达到最大文本数量(4个)", rect_index);
        return false;
    }
    
    TextPositionInRect* text = &rect->texts[rect->text_count];
    text->rel_x = rel_x;
    text->rel_y = rel_y;
    text->type = content_type;
    text->font_size = font_size;
    text->h_align = h_align;
    text->v_align = v_align;
    text->max_width = 0;  // 0表示使用矩形剩余宽度
    text->max_height = 0; // 0表示使用矩形剩余高度
    
    rect->text_count++;
    
    ESP_LOGI("TEXT", "矩形%d添加文本类型%d，位置(%.2f, %.2f), 字体%d",
             rect_index, content_type, rel_x, rel_y, font_size);
    
    return true;
}

/**
 * @brief 清空矩形内所有文本
 */
void clearRectTexts(RectInfo* rect) {
    if (rect == nullptr) return;
    rect->text_count = 0;
}

// ================= IPA 国际音标测试函数 =================
/**
 * @brief 测试国际音标(IPA)字体显示
 * 
 * 测试说明:
 * - 字体文件: comic_sans_ms_bold_phonetic_20x20.bin
 * - 字符范围: ASCII (0x20-0x7E, 95字符) + IPA (0x0250-0x02AF, 96字符)
 * - 总字符数: 191个字符
 * 
 * IPA音标示例:
 * - ə (schwa, U+0259)
 * - ʃ (esh, U+0283)  
 * - ʒ (ezh, U+0292)
 * - θ (theta, U+03B8)
 * - ð (eth, U+00F0)
 * - ŋ (eng, U+014B)
 */
void test_ipa_phonetic_font()
{
    ESP_LOGI(TAG, "=== 开始测试 IPA 国际音标字体 ===");
    
    // 先列出所有已加载的PSRAM字体
    int font_count = getPSRAMFontCount();
    ESP_LOGI(TAG, "当前已加载 %d 个PSRAM字体:", font_count);
    for (int i = 0; i < font_count; i++) {
        const FullFontData* loaded_font = getPSRAMFontByIndex(i);
        if (loaded_font) {
            ESP_LOGI(TAG, "  [%d] %s (%dx%d)", i, getFontName(loaded_font), 
                     getFontSize(loaded_font), getFontSize(loaded_font));
        }
    }
    
    // 1. 切换到音标字体 (注意文件名有空格)
    const char* font_name = "english_phonetic_font";  // 注意 phonetic 后有空格
    ESP_LOGI(TAG, "尝试切换到字体: '%s'", font_name);
    
    const FullFontData* font = findPSRAMFontByName(font_name);
    if (!font) {
        ESP_LOGE(TAG, "未找到字体: %s", font_name);
        ESP_LOGE(TAG, "请确认SD卡中存在文件: %s.bin", font_name);
        ESP_LOGE(TAG, "或者该字体未被加载到PSRAM (可能被shouldLoadToPSRAM()过滤)");
        return;
    }
    
    g_current_psram_font = font;
    ESP_LOGI(TAG, "✅ 成功切换到字体: %s", getFontName(font));
    ESP_LOGI(TAG, "   字体大小: %dx%d", getFontSize(font), getFontSize(font));
    ESP_LOGI(TAG, "   字模大小: %u 字节", getGlyphSize(font));
    ESP_LOGI(TAG, "   字符总数: %u", font->char_count);
    ESP_LOGI(TAG, "   文件大小: %u 字节", font->size);
    
    // 验证字符范围计算
    ESP_LOGI(TAG, "字符范围验证:");
    ESP_LOGI(TAG, "   ASCII: 0x20-0x7E (95字符) -> 偏移 0-%u", 95 * getGlyphSize(font) - 1);
    ESP_LOGI(TAG, "   IPA: 0x0250-0x02AF (96字符) -> 偏移 %u-%u", 
             95 * getGlyphSize(font), 191 * getGlyphSize(font) - 1);
    
    // 直接检查PSRAM中IPA字符区域的数据
    ESP_LOGI(TAG, "检查PSRAM中IPA区域数据:");
    uint32_t ipa_start_offset = 95 * getGlyphSize(font);  // IPA区域起始偏移
    int non_zero_glyphs = 0;
    for (int i = 0; i < 10; i++) {  // 检查前10个IPA字符
        uint32_t offset = ipa_start_offset + i * getGlyphSize(font);
        bool has_data = false;
        for (uint32_t j = 0; j < getGlyphSize(font); j++) {
            if (font->data[offset + j] != 0) {
                has_data = true;
                break;
            }
        }
        if (has_data) non_zero_glyphs++;
    }
    ESP_LOGI(TAG, "   前10个IPA字符中有 %d 个非空字模", non_zero_glyphs);
    
    if (non_zero_glyphs == 0) {
        ESP_LOGE(TAG, "❌ 严重警告: 字体文件的IPA区域全部为空!");
        ESP_LOGE(TAG, "   原因: Comic Sans MS Bold 字体不包含IPA扩展字符");
        ESP_LOGE(TAG, "   解决: 需要使用支持IPA的字体,如:");
        ESP_LOGE(TAG, "   - Noto Sans (推荐)");
        ESP_LOGE(TAG, "   - DejaVu Sans");
        ESP_LOGE(TAG, "   - Gentium Plus");
    }
    
    int font_size = getFontSize(font);
    uint32_t glyph_size = getGlyphSize(font);
    
    // 分配字模缓冲区
    uint8_t* glyph_buffer = (uint8_t*)malloc(glyph_size);
    if (!glyph_buffer) {
        ESP_LOGE(TAG, "无法分配字模缓冲区");
        return;
    }
    
    // 2. 清屏准备显示
    display.setFullWindow();
    display.firstPage();
    
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        
        int x = 10;
        int y = 10;
        
        // === 显示标题 "IPA Test" ===
        const char* title = "IPA Test";
        for (int i = 0; title[i] != '\0'; i++) {
            uint16_t unicode = (uint16_t)title[i];
            if (getCharGlyphFromPSRAM(font, unicode, glyph_buffer)) {
                display.drawBitmap(x, y, glyph_buffer, font_size, font_size, GxEPD_BLACK);
                x += font_size;
            }
        }
        
        // === 显示 ASCII "Hello!" ===
        x = 10;
        y += font_size + 5;
        const char* ascii_text = "Hello!";
        for (int i = 0; ascii_text[i] != '\0'; i++) {
            uint16_t unicode = (uint16_t)ascii_text[i];
            if (getCharGlyphFromPSRAM(font, unicode, glyph_buffer)) {
                display.drawBitmap(x, y, glyph_buffer, font_size, font_size, GxEPD_BLACK);
                x += font_size;
            }
        }
        
        // === 显示 IPA 音标 ===
        // schwa ə (U+0259)
        x = 10;
        y += font_size + 5;
        
        // 测试每个IPA字符
        ESP_LOGI(TAG, "开始测试IPA字符:");
        
        const uint16_t ipa_chars[] = {
            0x0259,  // ə schwa
            0x0020,  // space
            0x0283,  // ʃ esh
            0x0020,  // space
            0x0292,  // ʒ ezh
            0x0020,  // space
            0x03B8,  // θ theta (不在IPA范围,可能不显示)
            0x0000   // 结束符
        };
        
        for (int i = 0; ipa_chars[i] != 0; i++) {
            uint16_t unicode = ipa_chars[i];
            bool got_glyph = getCharGlyphFromPSRAM(font, unicode, glyph_buffer);
            
            ESP_LOGI(TAG, "  字符 U+%04X: %s", unicode, got_glyph ? "找到" : "未找到");
            
            if (got_glyph) {
                // 检查字模是否全为0
                bool all_zero = true;
                int non_zero_count = 0;
                for (uint32_t j = 0; j < glyph_size; j++) {
                    if (glyph_buffer[j] != 0) {
                        all_zero = false;
                        non_zero_count++;
                    }
                }
                
                if (all_zero) {
                    ESP_LOGW(TAG, "    ❌ 警告: 字模数据全为0 (字体文件中该字符为空白)");
                } else {
                    ESP_LOGI(TAG, "    ✅ 字模数据正常 (非零字节数: %d/%u, 前4字节: %02X %02X %02X %02X)", 
                             non_zero_count, glyph_size,
                             glyph_buffer[0], glyph_buffer[1], glyph_buffer[2], glyph_buffer[3]);
                }
                
                display.drawBitmap(x, y, glyph_buffer, font_size, font_size, GxEPD_BLACK);
                x += font_size;
            } else {
                ESP_LOGW(TAG, "    字符不在字体中");
            }
        }
        
        // === 显示更多IPA ===
        x = 10;
        y += font_size + 5;
        const uint16_t ipa_chars2[] = {
            0x0250,  // ɐ turned a
            0x0020,  // space
            0x0251,  // ɑ alpha
            0x0020,  // space  
            0x0252,  // ɒ turned alpha
            0x0020,  // space
            0x0254,  // ɔ open o
            0x0000   // 结束符
        };
        
        for (int i = 0; ipa_chars2[i] != 0; i++) {
            if (getCharGlyphFromPSRAM(font, ipa_chars2[i], glyph_buffer)) {
                display.drawBitmap(x, y, glyph_buffer, font_size, font_size, GxEPD_BLACK);
                x += font_size;
            }
        }
        
        // === 显示字体信息 ===
        x = 10;
        y += font_size + 10;
        char info_text[32];
        snprintf(info_text, sizeof(info_text), "Font:%dx%d", font_size, font_size);
        for (int i = 0; info_text[i] != '\0'; i++) {
            uint16_t unicode = (uint16_t)info_text[i];
            if (getCharGlyphFromPSRAM(font, unicode, glyph_buffer)) {
                display.drawBitmap(x, y, glyph_buffer, font_size, font_size, GxEPD_BLACK);
                x += font_size;
            }
        }
        
    } while (display.nextPage());
    
    free(glyph_buffer);
    
    ESP_LOGI(TAG, "=== IPA 音标字体测试完成 ===");
    ESP_LOGI(TAG, "如果音标显示不正确，请检查:");
    ESP_LOGI(TAG, "1. SD卡中是否有 %s.bin 文件", font_name);
    ESP_LOGI(TAG, "2. 字体文件是否包含 IPA 字符范围 (0x0250-0x02AF)");
    ESP_LOGI(TAG, "3. 字体生成时是否使用了 char_range='english_ipa' 参数");
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    display.hibernate();
}

void drawPictureScaled(uint16_t orig_x, uint16_t orig_y, 
                           uint16_t orig_w, uint16_t orig_h,
                           const uint8_t* BMP, uint16_t color) {
    // 安全检查：图片数据不能为空
    if (BMP == nullptr) {
        ESP_LOGE(TAG, "drawPictureScaled: BMP数据为空指针！位置(%d,%d) 尺寸%dx%d", 
                orig_x, orig_y, orig_w, orig_h);
        return;
    }
    
    // 使用统一的缩放因子保持长宽比（按最小缩放因子适配到屏幕）
    float scale_x = (float) setInkScreenSize.screenWidth / (float)416;
    float scale_y = (float)setInkScreenSize.screenHeigt / (float)240;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
                            
    // 计算缩放后的位置和尺寸（统一缩放，保持比例）
    uint16_t new_x = (uint16_t)(orig_x * scale);
    uint16_t new_y = (uint16_t)(orig_y * scale);
    uint16_t new_w = (uint16_t)(orig_w * scale);
    uint16_t new_h = (uint16_t)(orig_h * scale);

    // 确保最小尺寸
    if (new_w < 4) new_w = 4;
    if (new_h < 4) new_h = 4;

    // 边界裁剪，避免越界写入缓冲区
    const uint16_t screen_w = setInkScreenSize.screenWidth;
    const uint16_t screen_h = setInkScreenSize.screenHeigt;
    if (new_x >= screen_w || new_y >= screen_h) {
        ESP_LOGW(TAG, "drawPictureScaled: 位置(%d,%d)超出屏幕(%d,%d)，跳过绘制", new_x, new_y, screen_w, screen_h);
        return;
    }

    if (new_x + new_w > screen_w) {
        uint16_t clipped_w = screen_w - new_x;
        ESP_LOGW(TAG, "drawPictureScaled: 宽度裁剪 %d -> %d (屏幕宽度=%d)", new_w, clipped_w, screen_w);
        new_w = clipped_w;
    }

    if (new_y + new_h > screen_h) {
        uint16_t clipped_h = screen_h - new_y;
        ESP_LOGW(TAG, "drawPictureScaled: 高度裁剪 %d -> %d (屏幕高度=%d)", new_h, clipped_h, screen_h);
        new_h = clipped_h;
    }

    if (new_w == 0 || new_h == 0) {
        ESP_LOGW(TAG, "drawPictureScaled: 裁剪后尺寸为0，跳过绘制");
        return;
    }

    // === 重要：BIN 格式是按行存储的（逐行扫描） ===
    // 每行占用的字节数（向上取整到8的倍数）
    uint16_t src_bytes_per_row = (orig_w + 7) / 8;

    // 对目标区域每个像素进行最近邻采样
    for (uint16_t dy = 0; dy < new_h; dy++) {
        for (uint16_t dx = 0; dx < new_w; dx++) {
            // 映射回源图坐标
            uint16_t sx = dx * orig_w / new_w;
            uint16_t sy = dy * orig_h / new_h;

            // 计算源图中该像素的字节和位（按行存储）
            uint32_t src_byte_idx = (uint32_t)sy * src_bytes_per_row + sx / 8;
            uint8_t src_bit_pos = sx % 8;
            uint8_t src_byte = BMP[src_byte_idx];

            // 读取源像素（MSB first：bit 7 对应第 0 列）
            bool pixel_on = (src_byte & (0x80 >> src_bit_pos)) != 0;

            // 设置目标像素
            display.drawPixel(new_x + dx, new_y + dy, pixel_on ? color : !color);
        }
    }

    ESP_LOGD(TAG, "Scaled icon: (%d,%d) %dx%d -> (%d,%d) %dx%d",
             orig_x, orig_y, orig_w, orig_h, new_x, new_y, new_w, new_h);
}


// 显示主界面（使用已填充的图标数据）
void displayMainScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border) {
    // 计算缩放比例（统一使用 setInkScreenSize，与 drawPictureScaled 保持一致）
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    ESP_LOGI("DISPLAY", "屏幕尺寸: %dx%d, 缩放比例: %.4f, 图标数量: %d", 
            setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt, 
            global_scale, g_global_icon_count);
    
    // 初始化显示 - 全屏刷新
    display.setFullWindow();
    display.firstPage();
    {
        // 清空背景
        display.fillScreen(GxEPD_WHITE);
        // ==================== 显示状态栏图标 ====================
        int wifi_x, wifi_y, battery_x, battery_y;
        // getStatusIconPositions(rects, rect_count, status_rect_index, 
        //                       &wifi_x, &wifi_y, &battery_x, &battery_y);

        #ifdef BATTERY_LEVEL
            const uint8_t* battery_icon = NULL;
            if (BATTERY_LEVEL >= 80) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_4;
            else if (BATTERY_LEVEL >= 60) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_3;
            else if (BATTERY_LEVEL >= 40) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_2;
            else if (BATTERY_LEVEL >= 20) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
            else battery_icon = ZHONGJINGYUAN_3_7_BATTERY_0;
            
            // 使用 drawPictureScaled 自动缩放电池图标（原始尺寸 36x24）
            drawPictureScaled(battery_x, battery_y, 36, 24, battery_icon, GxEPD_BLACK);
        #endif
        
        // ==================== 遍历所有矩形，显示图标 ====================
        for (int i = 0; i < rect_count; i++) {
            RectInfo* rect = &rects[i];
            
            // 计算缩放后的矩形位置和尺寸
            int display_x = (int)(rect->x * global_scale + 0.5f);
            int display_y = (int)(rect->y * global_scale + 0.5f);
            int display_width = (int)(rect->width * global_scale + 0.5f);
            int display_height = (int)(rect->height * global_scale + 0.5f);
            
            ESP_LOGI("MAIN", "--- 矩形%d: (%d,%d) %dx%d, 图标数:%d, text_roll数:%d ---", 
                    i, display_x, display_y, display_width, display_height, rect->icon_count, rect->text_roll_count);
            
            // 显示该矩形内的所有图标
            if (rect->icon_count > 0) {
                for (int j = 0; j < rect->icon_count; j++) {
                    IconPositionInRect* icon = &rect->icons[j];
                    int icon_index = icon->icon_index;
                    
                    ESP_LOGI("MAIN", "  处理图标%d, index=%d", j, icon_index);
                    
                    if (icon_index >= 0 && icon_index < 22) {
                        IconInfo* icon_info = &g_available_icons[icon_index];
                        
                        // 安全检查图标数据
                        if (icon_info->data == nullptr) {
                            ESP_LOGW("MAIN", "  图标%d数据为空，跳过", icon_index);
                            continue;
                        }
                        
                        // 计算图标位置（基于左上角对齐）
                        int icon_x = rect->x + (int)(icon->rel_x * rect->width);
                        int icon_y = rect->y + (int)(icon->rel_y * rect->height);
                        
                        ESP_LOGI("MAIN", "  显示图标%d: 原始位置(%d,%d) 尺寸%dx%d (使用drawPictureScaled自动缩放)", 
                                icon_index, icon_x, icon_y, icon_info->width, icon_info->height);
                        
                        // 优先从缓存读取，失败则从SD卡加载
                        uint32_t cache_width, cache_height;
                        const uint8_t* cached_data = getIconDataFromCache(icon_index, &cache_width, &cache_height);
                        if (cached_data) {
                            // 使用 drawPictureScaled 自动根据屏幕尺寸缩放图标
                            drawPictureScaled(icon_x, icon_y, cache_width, cache_height, cached_data, GxEPD_BLACK);
                            ESP_LOGD("MAIN", "  从缓存显示图标%d (自动缩放)", icon_index);
                        } else {
                            ESP_LOGW("MAIN", "  图标%d缓存未命中，从SD卡加载会导致无法缩放，建议预加载", icon_index);
                            // 注意：displayImageFromSD 不支持缩放，这里只能显示原始尺寸
                            const char* icon_file = getIconFileNameByIndex(icon_index);
                            displayImageFromSD(icon_file, (int)(icon_x * global_scale), (int)(icon_y * global_scale), display);
                        }
                    } else {
                        ESP_LOGW("MAIN", "  图标索引%d超出范围[0-21]，跳过", icon_index);
                    }
                }
            }
            
            // 显示该矩形内的所有动态图标组（icon_roll）
            if (rect->icon_roll_count > 0) {
                for (int j = 0; j < rect->icon_roll_count; j++) {
                    IconRollInRect* icon_roll = &rect->icon_rolls[j];
                    
                    // 获取当前应该显示的图标索引
                    int current_icon_index = getIconRollCurrentIndex(icon_roll);
                    
                    ESP_LOGI("MAIN", "  处理动态图标组%d: arr=%s, idx=%s, 当前图标=%d", 
                            j, icon_roll->icon_arr, icon_roll->idx, current_icon_index);
                    
                    if (current_icon_index >= 0 && current_icon_index < 21) {
                        IconInfo* icon_info = &g_available_icons[current_icon_index];
                        
                        // 安全检查图标数据
                        if (icon_info->data == nullptr) {
                            ESP_LOGW("MAIN", "  动态图标%d数据为空，跳过", current_icon_index);
                            continue;
                        }
                        
                        // 计算图标位置（基于左上角对齐）
                        int icon_x = rect->x + (int)(icon_roll->rel_x * rect->width);
                        int icon_y = rect->y + (int)(icon_roll->rel_y * rect->height);
                        
                        ESP_LOGI("MAIN", "  显示动态图标%d: 原始位置(%d,%d) 尺寸%dx%d (使用drawPictureScaled自动缩放)", 
                                current_icon_index, icon_x, icon_y, icon_info->width, icon_info->height);
                        
                          // 优先从缓存读取，失败则从SD卡加载
                        uint32_t cache_width, cache_height;
                        const uint8_t* cached_data = getIconDataFromCache(current_icon_index, &cache_width, &cache_height);
                        if (cached_data) {
                            // 使用 drawPictureScaled 自动根据屏幕尺寸缩放图标
                            drawPictureScaled(icon_x, icon_y, cache_width, cache_height, cached_data, GxEPD_BLACK);
                            ESP_LOGD("MAIN", "  从缓存显示动态图标%d (自动缩放)", current_icon_index);
                        } else {
                            ESP_LOGW("MAIN", "  动态图标%d缓存未命中，从SD卡加载会导致无法缩放", current_icon_index);
                            const char* icon_file = getIconFileNameByIndex(current_icon_index);
                            // 注意：displayImageFromSD 不支持缩放，这里只能显示原始尺寸
                            float scale = (float)setInkScreenSize.screenWidth / 416.0f;
                            displayImageFromSD(icon_file, (int)(icon_x * scale), (int)(icon_y * scale), display);
                        }
                    } else {
                        ESP_LOGW("MAIN", "  动态图标索引%d超出范围[0-20]，跳过", current_icon_index);
                    }
                }
            }
            switchToPSRAMFont("chinese_translate_font");
            // 显示该矩形内的所有动态文本组（text_roll）
            if (rect->text_roll_count >= 0) {
                for (int j = 0; j < rect->text_roll_count; j++) {
                    TextRollInRect* text_roll = &rect->text_rolls[j];
                    
                    // 获取当前应该显示的文本
                    const char* current_text = getTextRollCurrentText(text_roll);
                    
                    ESP_LOGI("MAIN", "  处理动态文本组%d: arr=%s, idx=%s, 当前文本=%s", 
                            j, text_roll->text_arr, text_roll->idx, current_text);
                    
                    if (current_text && strcmp(current_text, "ERR") != 0) {
                        // 计算文本位置（基于左上角对齐）
                        int text_x = rect->x + (int)(text_roll->rel_x * rect->width);
                        int text_y = rect->y + (int)(text_roll->rel_y * rect->height);
                        
                        // 应用缩放
                        int scaled_x = (int)(text_x * global_scale);
                        int scaled_y = (int)(text_y * global_scale);
                        
                        ESP_LOGI("MAIN", "  显示动态文本: 位置(%d,%d) 内容[%s]", 
                                scaled_x, scaled_y, current_text);
                        
                        // === 根据text_roll配置选择字体 ===
                        const char* font_name = nullptr;
                        
                        // 如果JSON中配置了字体，直接使用
                        if (text_roll->font[0] != '\0') {
                            font_name = text_roll->font;
                            ESP_LOGI("TEXT_DISPLAY", "使用JSON配置的字体: %s", font_name);
                        } else {
                            // 如果没有配置字体，根据text_arr类型自动选择（向后兼容）
                            font_name = "english_sentence_font";  // 默认英文字体
                            
                            if (strstr(text_roll->text_arr, "wordbook_word") != nullptr) {
                                // 单词：使用英文字体
                                font_name = "english_sentence_font";
                                ESP_LOGD("TEXT_DISPLAY", "自动选择英文字体显示单词");
                            } 
                            else if (strstr(text_roll->text_arr, "wordbook_phonetic") != nullptr) {
                                // 音标：使用音标字体（IPA字符）
                                font_name = "english_phonetic_font";
                                ESP_LOGD("TEXT_DISPLAY", "自动选择音标字体显示音标");
                            } 
                            else if (strstr(text_roll->text_arr, "wordbook_translation") != nullptr) {
                                // 翻译：使用中文字体
                                font_name = "chinese_translate_font";
                                ESP_LOGD("TEXT_DISPLAY", "自动选择中文字体显示翻译");
                            }
                            else if (strstr(text_roll->text_arr, "wordbook_pos") != nullptr) {
                                // 词性：使用英文字体（词性通常是英文缩写）
                                font_name = "english_sentence_font";
                                ESP_LOGD("TEXT_DISPLAY", "自动选择英文字体显示词性");
                            }
                        }
                        
                        // 切换到对应字体
                        if (switchToPSRAMFont(font_name)) {
                            ESP_LOGI("TEXT_DISPLAY", "准备显示文本 [%s]: (%d,%d) \"%s\"", 
                                     font_name, scaled_x, scaled_y, current_text);
                            
                            // 根据字体类型选择绘制函数
                            if (strcmp(font_name, "chinese_translate_font") == 0) {
                                // 中文字体：使用中文绘制函数
                                drawChineseTextWithCache(display, scaled_x, scaled_y, current_text, GxEPD_BLACK);
                            } else {
                                // 英文/音标字体：使用英文绘制函数
                                drawEnglishText(display, scaled_x, scaled_y, current_text, GxEPD_BLACK);
                            }
                            
                            ESP_LOGI("TEXT_DISPLAY", "文本已显示: (%d,%d) \"%s\"", 
                                     scaled_x, scaled_y, current_text);
                        } else {
                            ESP_LOGW("TEXT_DISPLAY", "字体切换失败: %s", font_name);
                        }
                    } else {
                        ESP_LOGW("MAIN", "  动态文本内容错误或为空，跳过");
                    }
                }
            }
        }
        
        // ==================== 显示矩形边框 ====================
        if (show_border) {
            ESP_LOGI("BORDER", "开始绘制矩形边框，共%d个矩形", rect_count);
            
            for (int i = 0; i < rect_count; i++) {
                RectInfo* rect = &rects[i];
                
                // 计算缩放后的边框位置
                int border_display_x = (int)(rect->x * global_scale + 0.5f);
                int border_display_y = (int)(rect->y * global_scale + 0.5f);
                int border_display_width = (int)(rect->width * global_scale + 0.5f);
                int border_display_height = (int)(rect->height * global_scale + 0.5f);
                
                // 边界检查
                if (border_display_x < 0) border_display_x = 0;
                if (border_display_y < 0) border_display_y = 0;
                if (border_display_x + border_display_width > display.width()) {
                    border_display_width = display.width() - border_display_x;
                }
                if (border_display_y + border_display_height > display.height()) {
                    border_display_height = display.height() - border_display_y;
                }
                
                if (border_display_width > 0 && border_display_height > 0) {
                    // 绘制矩形边框
                    display.drawRect(border_display_x, border_display_y, 
                                    border_display_width, border_display_height, 
                                    GxEPD_BLACK);
                }
            }
        }
        
        // ==================== 绘制焦点光标 ====================
        if (g_focus_mode_enabled && g_current_focus_rect >= 0 && g_current_focus_rect < rect_count) {
            drawFocusCursor(rects, rect_count, g_current_focus_rect, global_scale);
            ESP_LOGI("FOCUS", "主界面绘制焦点光标在矩形%d", g_current_focus_rect);
        }
    }
    //   displayImageFromSD("/test.bin",0,0,display);
    //displayImageFromSPIFFS("/book.bin", 0, 0, display);
    
   // ===== 测试1: 使用20x20缓存字库显示中文文本 =====
        // 使用常用字列表中确定存在的字符进行测试
        // ESP_LOGI(TAG, "测试中文字体显示(20x20)...");
        // // 切换到中文仿宋字体
        // if (switchToPSRAMFont("chinese_translate_font")) {
        //     drawChineseTextWithCache(display, 10, 10, "的一是了我不人在", GxEPD_BLACK);
        //     drawChineseTextWithCache(display, 10, 40, "他有这个上中大到", GxEPD_BLACK);
        //     drawChineseTextWithCache(display, 10, 70, "说你为子和也得会", GxEPD_BLACK);
        // }
        
        // // ===== 测试1.5: Comic Sans PSRAM 英文字体测试 (使用字体名称切换) =====
        // ESP_LOGI(TAG, "测试Comic Sans英文字体显示(从PSRAM)...");
        // // 切换到 Comic Sans 20x20 字体
        // if (switchToPSRAMFont("english_sentence_font")) {
        //     drawEnglishText(display, 10, 110, "Hello World!", GxEPD_BLACK);
        //     drawEnglishText(display, 10, 140, "ABC abc 123", GxEPD_BLACK);
        //     drawEnglishText(display, 10, 170, "Test PSRAM", GxEPD_BLACK);
        // }
        
        // // ===== 测试2: 中英文混合显示 =====
        // ESP_LOGI(TAG, "测试中英文混合显示...");
        // // 切换回中文字体
        // if (switchToPSRAMFont("chinese_translate_font")) {
        //     drawChineseTextWithCache(display, 10, 210, "世界", GxEPD_BLACK);
        // }
        
        // // ===== 测试3: 32x32字体测试 =====
        // ESP_LOGI(TAG, "测试32x32字体显示...");
        // //drawChineseTextWithCache(display, 10, 240, "大字测试", GxEPD_BLACK, 28);    // 28x28中文
        // if (switchToPSRAMFont("english_word_font")) {
        //     drawEnglishText(display, 10 + 150, 210, "BIG", GxEPD_BLACK);           // 28x28英文 Bold
        // }
        // test_ipa_phonetic_font();

    // 执行单次刷新
    display.nextPage();
    
    // ===== 测试4: 单词本显示测试 =====
    // 注释掉上面的测试，取消注释下面的代码来测试单词本显示
    
    ESP_LOGI(TAG, "========== 单词本显示测试 ==========");
    
    // 1. 初始化单词本缓存（从SD卡加载）
    // if (initWordBookCache("/ecdict.mini.csv")) {
    //     ESP_LOGI(TAG, "✅ 单词本缓存初始化成功");
        
    //     // 2. 在墨水屏上显示3个单词（含完整信息：单词、词性、音标、翻译）
    //     // 注意：每个单词约占60-70px高度，建议显示3-4个
    //     testDisplayWordsOnScreen(display, 3);
        
    //     // 可选：打印到串口查看详细信息
    //     // printWordsFromCache(3);
    // } else {
    //     ESP_LOGE(TAG, "❌ 单词本缓存初始化失败");
        
    //     // 显示错误信息
    //     display.setFullWindow();
    //     display.firstPage();
    //     do {
    //         if (switchToPSRAMFont("chinese_translate_font")) {
    //             drawChineseTextWithCache(display, 10, 10, "错误：", GxEPD_BLACK);
    //             drawChineseTextWithCache(display, 10, 40, "无法加载单词本", GxEPD_BLACK);
    //         }
    //     } while (display.nextPage());
    // }
    
    
    // 等待屏幕完成刷新
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // 进入部分刷新模式
  //  display.setPartialWindow(0, 0, display.width(), display.height());
    
    ESP_LOGI("MAIN", "主界面显示完成");
}

// 原来的主函数，保持兼容性
void updateDisplayWithMain(RectInfo *rects, int rect_count, int status_rect_index, int show_border) {

    // 1. 重置图标
    initIconPositions();
    // 2. 直接使用rects数组中的图标配置
    // 遍历所有矩形（除了状态栏）
    for (int i = 0; i < rect_count; i++) {
        // 跳过状态栏矩形
        if (i == status_rect_index) {
            continue;
        }
        
        RectInfo *rect = &rects[i];
        
        // 如果有图标配置，就设置
        for (int j = 0; j < rect->icon_count; j++) {
            IconPositionInRect *icon = &rect->icons[j];
            
            // 直接使用存储的相对位置
            float rel_x = icon->rel_x;
            float rel_y = icon->rel_y;
            int icon_index = icon->icon_index;
            
            // 边界检查
            if (rel_x < 0.0f) rel_x = 0.0f;
            if (rel_x > 1.0f) rel_x = 1.0f;
            if (rel_y < 0.0f) rel_y = 0.0f;
            if (rel_y > 1.0f) rel_y = 1.0f;
            if (icon_index < 0) icon_index = 0;
            if (icon_index >= 6) icon_index = 5;
            
            // 设置图标
            populateRectWithIcon(rect, icon_index, rel_x, rel_y);
        }
    }
    // 4. 显示
    displayMainScreen(rects, rect_count, status_rect_index, show_border);
}
#define DEBUG_LAYOUT 1


void update_activity_time() {
    last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (is_sleep_mode) {
        ESP_LOGI("SLEEP", "退出休眠模式");
        is_sleep_mode = false;
        has_sleep_data = false;
    }
}

void changeInkSreenSize() {
    uint16_t newWidth = WebUI::inkScreenXSizeSet->get();
    uint16_t newHeight = WebUI::inkScreenYSizeSet->get();
    
    if(setInkScreenSize.screenWidth != newWidth && 
       setInkScreenSize.screenHeigt != newHeight) {
        setInkScreenSize.screenHeigt = newHeight;
        setInkScreenSize.screenWidth = newWidth;
        ESP_LOGI(TAG,"screenHeigt:%d,screenWidth:%d\r\n", setInkScreenSize.screenHeigt, setInkScreenSize.screenWidth);
        inkScreenTestFlag = 7;

    }
}

/**
 * @brief 设置可焦点矩形列表
 * @param rect_indices 矩形索引数组
 * @param count 数组长度
 */
void setFocusableRects(int* rect_indices, int count) {
    if (!rect_indices || count <= 0 || count > MAX_FOCUSABLE_RECTS) {
        ESP_LOGW("FOCUS", "无效的焦点矩形列表参数: count=%d", count);
        return;
    }
    
    g_focusable_rect_count = count;
    for (int i = 0; i < count; i++) {
        g_focusable_rect_indices[i] = rect_indices[i];
    }
    
    g_current_focus_index = 0;
    g_current_focus_rect = g_focusable_rect_indices[0];
    
    // 清空所有子数组配置
    for (int i = 0; i < MAX_FOCUSABLE_RECTS; i++) {
        g_sub_array_counts[i] = 0;
    }
    
    ESP_LOGI("FOCUS", "已设置可焦点矩形列表，共%d个矩形:", count);
    for (int i = 0; i < count; i++) {
        ESP_LOGI("FOCUS", "  [%d] 矩形索引: %d", i, g_focusable_rect_indices[i]);
    }
}

/**
 * @brief 为母数组中的某个元素设置子数组
 * @param parent_index 母数组中的索引位置（0-based）
 * @param sub_indices 子数组的矩形索引数组
 * @param sub_count 子数组长度
 */
void setSubArrayForParent(int parent_index, int* sub_indices, int sub_count) {
    if (parent_index < 0 || parent_index >= g_focusable_rect_count) {
        ESP_LOGW("FOCUS", "无效的母数组索引: %d (母数组共%d个元素)", parent_index, g_focusable_rect_count);
        return;
    }
    
    if (!sub_indices || sub_count <= 0 || sub_count > MAX_FOCUSABLE_RECTS) {
        ESP_LOGW("FOCUS", "无效的子数组参数: count=%d", sub_count);
        return;
    }
    
    g_sub_array_counts[parent_index] = sub_count;
    for (int i = 0; i < sub_count; i++) {
        g_sub_array_indices[parent_index][i] = sub_indices[i];
    }
    
    ESP_LOGI("FOCUS", "已为母数组索引%d（矩形%d）设置子数组，共%d个元素:", 
             parent_index, g_focusable_rect_indices[parent_index], sub_count);
    for (int i = 0; i < sub_count; i++) {
        ESP_LOGI("FOCUS", "  [%d] 子数组矩形索引: %d", i, sub_indices[i]);
    }
}

/**
 * @brief 为指定矩形索引设置子数组（自动查找该矩形在母数组中的位置）
 * @param rect_index 矩形的实际索引（例如：矩形0, 矩形1）
 * @param sub_indices 子数组的矩形索引数组
 * @param sub_count 子数组长度
 */
void setSubArrayForRect(int rect_index, int* sub_indices, int sub_count) {
    // 查找rect_index在母数组g_focusable_rect_indices中的位置
    int parent_pos = -1;
    for (int i = 0; i < g_focusable_rect_count; i++) {
        if (g_focusable_rect_indices[i] == rect_index) {
            parent_pos = i;
            break;
        }
    }
    
    if (parent_pos == -1) {
        ESP_LOGW("FOCUS", "矩形%d不在母数组中，无法设置子数组", rect_index);
        ESP_LOGI("FOCUS", "当前母数组包含的矩形:");
        for (int i = 0; i < g_focusable_rect_count; i++) {
            ESP_LOGI("FOCUS", "  [%d] 矩形%d", i, g_focusable_rect_indices[i]);
        }
        return;
    }
    
    ESP_LOGI("FOCUS", "矩形%d在母数组中的位置: %d", rect_index, parent_pos);
    setSubArrayForParent(parent_pos, sub_indices, sub_count);
}

/**
 * @brief 初始化焦点系统（默认所有矩形都可焦点）
 * @param total_rects 总矩形数量
 */
void initFocusSystem(int total_rects) {
    g_total_focusable_rects = total_rects;
    
    // 默认情况：所有矩形都可焦点
    g_focusable_rect_count = (total_rects < MAX_FOCUSABLE_RECTS) ? total_rects : MAX_FOCUSABLE_RECTS;
    for (int i = 0; i < g_focusable_rect_count; i++) {
        g_focusable_rect_indices[i] = i;
    }
    
    g_current_focus_index = 0;
    
    // 🔧 查找第一个"mom"类型的矩形作为初始焦点
    g_current_focus_rect = -1;  // 先设为无效值
    if (g_json_rects && g_json_rect_count > 0) {
        ESP_LOGI("FOCUS", "🔍 开始查找第一个mom类型矩形，共%d个矩形", g_json_rect_count);
        for (int i = 0; i < g_json_rect_count; i++) {
            ESP_LOGI("FOCUS", "  检查矩形%d: is_mother='%s'", i, g_json_rects[i].is_mother);
            if (strcmp(g_json_rects[i].is_mother, "mom") == 0) {
                g_current_focus_rect = i;
                ESP_LOGI("FOCUS", "✅ 找到第一个母数组矩形：索引 %d", i);
                break;
            }
        }
        
        if (g_current_focus_rect == -1) {
            ESP_LOGW("FOCUS", "⚠️ 未找到任何母数组矩形，焦点系统可能无法正常工作");
            g_current_focus_rect = 0;  // 降级处理
        }
    } else {
        g_current_focus_rect = 0;  // 如果JSON数据不可用，使用默认值
        ESP_LOGW("FOCUS", "⚠️ JSON数据未准备好，使用默认焦点索引0");
    }
    
    g_focus_mode_enabled = true;
    
    ESP_LOGI("FOCUS", "焦点系统已初始化，共%d个矩形，初始焦点：矩形%d", total_rects, g_current_focus_rect);
}

/**
 * @brief 移动焦点到下一个矩形（在可焦点列表中）
 */
void moveFocusNext() {
    ESP_LOGI("FOCUS", "moveFocusNext called, current: %d", g_current_focus_rect);
    if (!g_json_rects) {
        ESP_LOGW("FOCUS", "JSON矩形数据未初始化");
        return;
    }
    
    if (g_json_rect_count <= 0) {
        ESP_LOGW("FOCUS", "JSON矩形数量为0");
        return;
    }
    
    if (g_in_sub_array) {
        // 子数组模式：在当前母数组的子数组中移动
        RectInfo* mother_rect = &g_json_rects[g_parent_focus_index_backup];
        if (mother_rect->group_count <= 0) {
            ESP_LOGW("FOCUS", "子数组为空");
            return;
        }
        
        g_current_sub_focus_index = (g_current_sub_focus_index + 1) % mother_rect->group_count;
        g_current_focus_rect = mother_rect->group_indices[g_current_sub_focus_index];
        
        ESP_LOGI("FOCUS", "【子数组】焦点向下移动: 子索引%d -> 矩形%d", g_current_sub_focus_index, g_current_focus_rect);
    } else {
        // 母数组模式：查找下一个母数组矩形（只在mom类型之间移动）
        int next_rect = g_current_focus_rect;
        int attempts = 0;
        
        do {
            next_rect = (next_rect + 1) % g_json_rect_count;
            attempts++;
            
            // 只移动到母数组类型的矩形
            if (strcmp(g_json_rects[next_rect].is_mother, "mom") == 0) {
                g_current_focus_rect = next_rect;
                ESP_LOGI("FOCUS", "【母数组】焦点向下移动: 矩形%d (母数组)", g_current_focus_rect);
                return;
            }
        } while (attempts < g_json_rect_count);
        
        ESP_LOGW("FOCUS", "没有找到可选中的矩形");
    }
}

/**
 * @brief 移动焦点到前一个矩形（在可焦点列表中）
 */
void moveFocusPrev() {
    if (!g_json_rects) {
        ESP_LOGW("FOCUS", "JSON矩形数据未初始化");
        return;
    }
    
    if (g_in_sub_array) {
        // 子数组模式：在当前母数组的子数组中移动
        RectInfo* mother_rect = &g_json_rects[g_parent_focus_index_backup];
        if (mother_rect->group_count <= 0) {
            ESP_LOGW("FOCUS", "子数组为空");
            return;
        }
        
        g_current_sub_focus_index = (g_current_sub_focus_index - 1 + mother_rect->group_count) % mother_rect->group_count;
        g_current_focus_rect = mother_rect->group_indices[g_current_sub_focus_index];
        
        ESP_LOGI("FOCUS", "【子数组】焦点向上移动: 子索引%d -> 矩形%d", g_current_sub_focus_index, g_current_focus_rect);
    } else {
        // 母数组模式：查找上一个母数组矩形（只在mom类型之间移动）
        int prev_rect = g_current_focus_rect;
        int attempts = 0;
        
        do {
            prev_rect = (prev_rect - 1 + g_json_rect_count) % g_json_rect_count;
            attempts++;
            
            // 只移动到母数组类型的矩形
            if (strcmp(g_json_rects[prev_rect].is_mother, "mom") == 0) {
                g_current_focus_rect = prev_rect;
                ESP_LOGI("FOCUS", "【母数组】焦点向上移动: 矩形%d (母数组)", g_current_focus_rect);
                return;
            }
        } while (attempts < g_json_rect_count);
        
        ESP_LOGW("FOCUS", "没有找到可选中的矩形");
    }
}

/**
 * @brief 获取当前焦点矩形索引
 */
int getCurrentFocusRect() {
    return g_current_focus_rect;
}

/**
 * @brief 进入子数组模式
 * @return 是否成功进入（如果当前母数组元素没有子数组则返回false）
 */
bool enterSubArray() {
    if (g_in_sub_array) {
        ESP_LOGW("FOCUS", "已经在子数组模式中");
        return false;
    }
    
    // 获取当前焦点矩形
    RectInfo* current_rect = nullptr;
    if (g_json_rects && g_current_focus_rect >= 0 && g_current_focus_rect < g_json_rect_count) {
        current_rect = &g_json_rects[g_current_focus_rect];
    }
    
    if (!current_rect) {
        ESP_LOGW("FOCUS", "无法获取当前焦点矩形");
        return false;
    }
    
    // 检查是否为母数组
    if (strcmp(current_rect->is_mother, "mom") != 0) {
        ESP_LOGI("FOCUS", "当前矩形%d不是母数组（类型：%s）", g_current_focus_rect, current_rect->is_mother);
        return false;
    }
    
    // 检查是否有子数组
    if (current_rect->group_count <= 0) {
        ESP_LOGI("FOCUS", "母数组%d没有配置子数组", g_current_focus_rect);
        return false;
    }
    
    // 备份母数组焦点位置
    g_parent_focus_index_backup = g_current_focus_rect;
    
    // 进入子数组模式：焦点切换到第一个子数组元素
    g_in_sub_array = true;
    g_current_sub_focus_index = 0;
    g_current_focus_rect = current_rect->group_indices[0];
    
    ESP_LOGI("FOCUS", "✓ 进入子数组模式：母数组%d -> 子数组（共%d个元素），初始焦点在矩形%d",
             g_parent_focus_index_backup, 
             current_rect->group_count,
             g_current_focus_rect);

    // 注释掉这里的屏幕刷新，统一在case语句中处理，避免双重刷新
    // if (g_json_rects && g_json_rect_count > 0) {
    //     updateDisplayWithMain(g_json_rects, g_json_rect_count, -1, 1);
    // }
    
    return true;
}

/**
 * @brief 退出子数组模式，返回母数组
 */
void exitSubArray() {
    if (!g_in_sub_array) {
        ESP_LOGW("FOCUS", "当前不在子数组模式中");
        return;
    }
    
    // 恢复母数组焦点位置
    g_current_focus_rect = g_parent_focus_index_backup;
    
    // 退出子数组模式
    g_in_sub_array = false;
    g_current_sub_focus_index = 0;
    
    ESP_LOGI("FOCUS", "✓ 退出子数组模式，返回母数组，焦点在矩形%d", g_current_focus_rect);
}

/**
 * @brief 处理焦点确认动作
 */
void handleFocusConfirm() {
    if (g_in_sub_array) {
        // 在子数组中：按确认键返回母数组
        ESP_LOGI("FOCUS", "【子数组】确认操作：退出子数组，返回母数组");
        exitSubArray();
    } else {
        // 在母数组中：尝试进入子数组
        ESP_LOGI("FOCUS", "【母数组】确认操作：尝试进入子数组（当前焦点在矩形%d）", g_current_focus_rect);
        if (!enterSubArray()) {
            ESP_LOGI("FOCUS", "当前矩形没有子数组，尝试调用矩形的 onConfirm 回调");
            // 尝试调用当前矩形的回调（如果已设置）
            ScreenConfig* cfg = &g_screen_manager.screens[g_screen_manager.current_screen];
            if (cfg && cfg->rects && g_current_focus_rect >= 0 && g_current_focus_rect < cfg->rect_count) {
                RectInfo* cur = &cfg->rects[g_current_focus_rect];
                if (cur->onConfirm) {
                    ESP_LOGI("FOCUS", "调用矩形%d的 onConfirm 回调", g_current_focus_rect);
                    cur->onConfirm(cur, g_current_focus_rect);
                    return;
                }
            }
            ESP_LOGI("FOCUS", "矩形没有注册 onConfirm，执行默认确认操作");
            // 这里可以添加其他默认确认逻辑
        }
    }
}

/**
 * @brief 绘制焦点光标
 * @param rects 矩形数组
 * @param focus_index 焦点矩形索引
 * @param global_scale 全局缩放比例
 */
void drawFocusCursor(RectInfo *rects, int rect_count, int focus_index, float global_scale) {
    if (!rects || focus_index < 0 || focus_index >= rect_count) {
        ESP_LOGW("FOCUS", "无效的焦点索引: %d (总数: %d)", focus_index, rect_count);
        return;
    }
    
    RectInfo* rect = &rects[focus_index];
    
    // 检查矩形是否有效
    if (rect->width <= 0 || rect->height <= 0) {
        ESP_LOGW("FOCUS", "焦点矩形%d无效: (%d,%d) %dx%d", focus_index,
                rect->x, rect->y, rect->width, rect->height);
        return;
    }
    
    // 计算缩放后的位置和尺寸
    int display_x = (int)(rect->x * global_scale + 0.5f);
    int display_y = (int)(rect->y * global_scale + 0.5f);
    int display_width = (int)(rect->width * global_scale + 0.5f);
    int display_height = (int)(rect->height * global_scale + 0.5f);
    
    // 绘制焦点光标（在矩形四角绘制小三角形或方块）
    int cursor_size = 6;  // 光标大小

    // 确定要使用的焦点图标
    int focus_icon_index = 7;  // 默认图标索引（nail）
    if (rect->focus_icon_index >= 0 && rect->focus_icon_index < ICON_COUNT) {
        focus_icon_index = rect->focus_icon_index;
        ESP_LOGI("FOCUS", "使用自定义焦点图标: %d", focus_icon_index);
    } else {
        ESP_LOGI("FOCUS", "使用默认焦点图标: NAIL");
    }
    
    // 优先从缓存获取焦点图标数据
    uint32_t icon_width, icon_height;
    const uint8_t* focus_icon_data = getIconDataFromCache(focus_icon_index, &icon_width, &icon_height);
    bool use_cache = (focus_icon_data != nullptr);
    
    if (!use_cache) {
        // 缓存未命中，需要从SD卡加载时获取尺寸
        getIconSizeByIndex(focus_icon_index, (int*)&icon_width, (int*)&icon_height);
        ESP_LOGW("FOCUS", "焦点图标%d缓存未命中，将从SD卡加载", focus_icon_index);
    }
    
    // 支持不同模式的焦点显示：从矩形自身的 focus_mode 字段读取
    // mode == FOCUS_MODE_DEFAULT : 默认（使用指定的焦点图标）
    // mode == FOCUS_MODE_CORNERS : 在四角绘制图标
    // mode == FOCUS_MODE_BORDER  : 绘制在边框位置
    FocusMode mode_to_use = rect->focus_mode;
    if (mode_to_use == FOCUS_MODE_BORDER) {
        // 在右下角显示焦点图标（原始坐标，由 drawPictureScaled 自动缩放）
        int icon_x = rect->x + rect->width;
        int icon_y = rect->y + rect->height - icon_height;
        if (use_cache) {
            drawPictureScaled(icon_x, icon_y, icon_width, icon_height, focus_icon_data, GxEPD_BLACK);
        } else {
            ESP_LOGW("FOCUS", "焦点图标缓存未命中，无法使用drawPictureScaled");
            const char* icon_file = getIconFileNameByIndex(focus_icon_index);
            displayImageFromSD(icon_file, (int)(icon_x * global_scale), (int)(icon_y * global_scale), display);
        }
        ESP_LOGI("FOCUS", "BORDER模式: 原始位置(%d,%d) 尺寸%dx%d (自动缩放)", icon_x, icon_y, icon_width, icon_height);

    } else if (mode_to_use == FOCUS_MODE_CORNERS) {
        // 在右上角显示焦点图标（原始坐标，由 drawPictureScaled 自动缩放）
        int icon_x = rect->x + rect->width;
        int icon_y = rect->y;
        if (use_cache) {
            drawPictureScaled(icon_x, icon_y, icon_width, icon_height, focus_icon_data, GxEPD_BLACK);
        } else {
            ESP_LOGW("FOCUS", "焦点图标缓存未命中，无法使用drawPictureScaled");
            const char* icon_file = getIconFileNameByIndex(focus_icon_index);
            displayImageFromSD(icon_file, (int)(icon_x * global_scale), (int)(icon_y * global_scale), display);
        }
        ESP_LOGI("FOCUS", "CORNERS模式: 原始位置(%d,%d) 尺寸%dx%d (自动缩放)", icon_x, icon_y, icon_width, icon_height);
    } else if (mode_to_use == FOCUS_MODE_DEFAULT) {
        // 默认模式：使用指定的焦点图标居中显示在矩形左侧中间（原始坐标，由 drawPictureScaled 自动缩放）
        int icon_x = rect->x;
        int icon_y = rect->y;
        if (use_cache) {
            drawPictureScaled(icon_x, icon_y, icon_width, icon_height, focus_icon_data, GxEPD_BLACK);
        } else {
            ESP_LOGW("FOCUS", "焦点图标缓存未命中，无法使用drawPictureScaled");
            const char* icon_file = getIconFileNameByIndex(focus_icon_index);
            displayImageFromSD(icon_file, (int)(icon_x * global_scale), (int)(icon_y * global_scale), display);
        }
        ESP_LOGI("FOCUS", "DEFAULT模式: 原始位置(%d,%d) 尺寸%dx%d (自动缩放)", icon_x, icon_y, icon_width, icon_height);
    }
    

    ESP_LOGI("FOCUS", "已绘制焦点光标，位置：(%d,%d) 尺寸：%dx%d", 
            display_x, display_y, display_width, display_height);
}

/**
 * @brief 启动焦点测试模式
 * 直接进入单词界面并启用焦点功能，用于调试
 */
void startFocusTestMode() {
    ESP_LOGI("FOCUS", "========== 启动焦点测试模式 ==========");
    
    // 清屏
    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
    
    // 重新加载单词界面配置
    if (loadVocabLayoutFromConfig()) {
        ESP_LOGI("FOCUS", "单词界面布局已重新加载");
    }
    
    // 直接从矩形数据计算有效矩形数量，不依赖配置值
    extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
    int valid_rect_count = 0;
    for (int i = 0; i < MAX_VOCAB_RECTS; i++) {
        if (vocab_rects[i].width > 0 && vocab_rects[i].height > 0) {
            valid_rect_count++;
        } else {
            break; // 遇到第一个无效矩形就停止计数
        }
    }
    
    initFocusSystem(valid_rect_count);
    ESP_LOGI("FOCUS", "焦点系统已初始化，共%d个有效矩形", valid_rect_count);
    
    // 显示单词界面
    displayScreen(SCREEN_VOCABULARY);
    
    // 设置为界面2（单词界面）
    interfaceIndex = 2;
    inkScreenTestFlag = 0;
    inkScreenTestFlagTwo = 0;
    
    ESP_LOGI("FOCUS", "========== 焦点测试模式已启动 ==========");
    ESP_LOGI("FOCUS", "使用按键控制：");
    ESP_LOGI("FOCUS", "  按键1 - 向上选择（前一个矩形）");
    ESP_LOGI("FOCUS", "  按键2 - 确认选择");
    ESP_LOGI("FOCUS", "  按键3 - 向下选择（下一个矩形）");
}

/**
 * @brief 显示休眠界面
 */
void displaySleepScreen(RectInfo *rects, int rect_count,
                       int status_rect_index, int show_border) {
    // 简单的休眠界面显示
    display.setFullWindow();
    display.firstPage();
    {
        display.fillScreen(GxEPD_WHITE);
        // 显示时间和提示信息
        // TODO: 实现休眠界面的具体显示逻辑
        display.setCursor(0, 0);
        display.setFont();
        display.setTextColor(GxEPD_BLACK);
        display.println("Sleep Mode");
    }
    
    // 执行单次刷新
    display.nextPage();
    
    // 进入深度睡眠
    display.powerOff();
}

// 修改主显示函数，支持不同的界面
void displayScreen(ScreenType screen_type) {
    switch(screen_type) {
        case SCREEN_HOME:
            displayMainScreen(g_screen_manager.screens[screen_type].rects,
                            g_screen_manager.screens[screen_type].rect_count,
                            g_screen_manager.screens[screen_type].status_rect_index,
                            g_screen_manager.screens[screen_type].show_border);
            break;
            
        case SCREEN_VOCABULARY:
            ESP_LOGI(TAG, "准备显示单词界面，矩形数量: %d", g_screen_manager.screens[screen_type].rect_count);
            // displayVocabularyScreen(g_screen_manager.screens[screen_type].rects,
            //                 g_screen_manager.screens[screen_type].rect_count,
            //                 g_screen_manager.screens[screen_type].status_rect_index,
            //                 g_screen_manager.screens[screen_type].show_border);
            break;
            
        case SCREEN_SLEEP:
            // displaySleepScreen(sleep_rects, 3, -1, 0);  // 已弃用，改用JSON布局
            ESP_LOGI("DISPLAY", "SCREEN_SLEEP 已弃用，请使用JSON布局");
            break;
            
        default:
            ESP_LOGE("DISPLAY", "未知的界面类型: %d", screen_type);
            break;
    }
}

// 单词本界面图标设置函数
void setupVocabularyScreenIcons() {
    ESP_LOGI("SETUP_ICONS", "设置单词本界面图标");
    
    // 重置图标
    initIconPositions();
    
    // 遍历主界面的所有矩形（除了状态栏）
    for (int i = 0; i < g_screen_manager.screens[SCREEN_VOCABULARY].rect_count; i++) {
        // 跳过状态栏矩形
        if (i == g_screen_manager.screens[SCREEN_VOCABULARY].status_rect_index) {
            continue;
        }
        
        RectInfo *rect = &vocab_rects[i];
        
        // 如果有图标配置，就设置
        for (int j = 0; j < rect->icon_count; j++) {
            IconPositionInRect *icon = &rect->icons[j];
            
            // 直接使用存储的相对位置
            float rel_x = icon->rel_x;
            float rel_y = icon->rel_y;
            int icon_index = icon->icon_index;
            
            // 边界检查
            if (rel_x < 0.0f) rel_x = 0.0f;
            if (rel_x > 1.0f) rel_x = 1.0f;
            if (rel_y < 0.0f) rel_y = 0.0f;
            if (rel_y > 1.0f) rel_y = 1.0f;
            if (icon_index < 0) icon_index = 0;
            if (icon_index >= 6) icon_index = 5;
            
            // 设置图标（主界面）
            populateRectWithIcon(rect, icon_index, rel_x, rel_y);
        }
    }
}

// 休眠界面图标设置函数
void setupSleepScreenIcons() {
    ESP_LOGI("SETUP_ICONS", "设置休眠界面图标");
    
    // 重置图标
    initIconPositions();
    
    // 休眠界面的图标配置 - 已弃用，改用JSON布局
    /*
    RectInfo* sleep_rects = g_screen_manager.screens[SCREEN_SLEEP].rects;
    int rect_count = g_screen_manager.screens[SCREEN_SLEEP].rect_count;
    
    // 休眠界面图标通常较少，主要用于显示时间、日期等
    IconConfig sleep_icons[] = {
        // 矩形0：时间显示（使用图标0，精确居中）
        {0, 0, 0.5f, 0.5f},
        
        // 矩形1：日期显示（使用图标1，精确居中）
        {1, 1, 0.5f, 0.5f},
        
        // 矩形2：唤醒提示（使用图标2，居中）
        {2, 2, 0.5f, 0.5f}
    };
    
    // 设置休眠界面图标
    populateRectsWithCustomIcons(sleep_rects, rect_count, 
                                sleep_icons, sizeof(sleep_icons) / sizeof(sleep_icons[0]));
    */
}

/**
 * @brief 获取主界面的矩形数量
 * @return 主界面的矩形数量
 */
int getMainScreenRectCount() {
    return g_screen_manager.screens[SCREEN_HOME].rect_count;
}

/**
 * @brief 获取单词界面的矩形数量
 * @return 单词界面的矩形数量
 */
int getVocabScreenRectCount() {
    return g_screen_manager.screens[SCREEN_VOCABULARY].rect_count;
}

void ink_screen_show(void *args)
{
    float num=12.05;
    uint8_t dat=0;
	grbl_msg_sendf(CLIENT_SERIAL, MsgLevel::Info,"ink_screen_show");
	Uart0.printf("ink_screen_show\r\n");
   //init_sleep_timer();
	while(1)
	{
        if (inkScreenTestFlag == 99) {
            ESP_LOGI("SLEEP", "显示休眠模式界面");
           // display_sleep_mode();
            inkScreenTestFlag = 0;
            vTaskDelay(1000);
            continue;
        }
		switch(inkScreenTestFlag)
		{
			case 0:
				switch(inkScreenTestFlagTwo)
				{
					case 0:
					break;
					case 11:
                    {
                        clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                        ESP_LOGI(TAG,"############显示单词本界面（使用新图标布局系统）\r\n");
                        update_activity_time(); // 更新活动时间
                        
                        // 重新加载单词界面配置，确保使用最新的网页端配置
                        if (loadVocabLayoutFromConfig()) {
                            ESP_LOGI(TAG, "单词界面布局已从配置文件重新加载");
                        } else {
                            ESP_LOGI(TAG, "使用当前单词界面布局");
                        }
                        
                        // 直接从矩形数据计算有效矩形数量，不依赖配置值
                        extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
                        int valid_rect_count = 0;
                        for (int i = 0; i < MAX_VOCAB_RECTS; i++) {
                            if (vocab_rects[i].width > 0 && vocab_rects[i].height > 0) {
                                valid_rect_count++;
                                ESP_LOGI("FOCUS", "有效矩形%d: (%d,%d) %dx%d", i, 
                                        vocab_rects[i].x, vocab_rects[i].y, 
                                        vocab_rects[i].width, vocab_rects[i].height);
                            } else {
                                ESP_LOGI("FOCUS", "无效矩形%d: (%d,%d) %dx%d", i, 
                                        vocab_rects[i].x, vocab_rects[i].y, 
                                        vocab_rects[i].width, vocab_rects[i].height);
                                break; // 遇到第一个无效矩形就停止计数（假设矩形是连续的）
                            }
                        }
                        
                        ESP_LOGI("FOCUS", "检测到有效矩形数量: %d", valid_rect_count);
                        
                        // 使用实际检测到的有效矩形数量初始化焦点系统（默认全部可焦点）
                        initFocusSystem(valid_rect_count);
                        
                        // 尝试从配置文件加载自定义的可焦点矩形列表
                        if (loadFocusableRectsFromConfig("vocab")) {
                            ESP_LOGI("FOCUS", "已从配置文件加载单词界面焦点矩形列表");
                        } else {
                            ESP_LOGI("FOCUS", "使用默认焦点配置（所有矩形都可焦点）");
                        }
                        
                        // 加载子数组配置
                        if (loadAndApplySubArrayConfig("vocab")) {
                            ESP_LOGI("FOCUS", "已从配置文件加载并应用单词界面子数组配置");
                        } else {
                            ESP_LOGI("FOCUS", "未加载单词界面子数组配置或配置为空");
                        }
                        
                        // 设置当前界面为单词界面
                        g_screen_manager.current_screen = SCREEN_VOCABULARY;
                        
                        // 使用displayScreen统一接口，与主界面保持一致
                        displayScreen(SCREEN_VOCABULARY);
                        
                        vTaskDelay(1000);
                        inkScreenTestFlagTwo = 0;
                        inkScreenTestFlag = 0;
                        interfaceIndex = 2;
                    }
					break;
					case 22:
					break;
					case 33:
					break;
					case 44:
					break;
					case 55:
                        update_activity_time(); // 更新活动时间
                        // 清屏并进入深睡眠
                        display.setFullWindow();
                        display.firstPage();
                        do {
                            display.fillScreen(GxEPD_WHITE);
                        } while (display.nextPage());
						ESP_LOGI(TAG,"inkScreenTestFlagTwo\r\n");
						inkScreenTestFlagTwo = 0;
                        inkScreenTestFlag = 0;
                        interfaceIndex =2;
					break;
					case 66:
					break;
                    default:
                        break;

				}
			break;
			case 1:  // 按键1：上一个焦点/向上选择
                update_activity_time(); 
                
                // 优先检查是否为JSON布局模式
                if (g_json_rects != nullptr && g_json_rect_count > 0) {
                    // JSON布局模式：调用JSON按键处理
                    ESP_LOGI("JSON_KEY", "按键1：JSON布局向上移动焦点");
                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                    jsonLayoutFocusPrev();
                } else if (g_focus_mode_enabled) {
                    // 原有焦点模式：向上移动焦点
                    moveFocusPrev();
                    ESP_LOGI("FOCUS", "按键1：焦点向上移动到矩形%d", g_current_focus_rect);
                    
                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                    // 根据当前界面类型重新显示
                    displayScreen(g_screen_manager.current_screen);
                } else {
                    // 原有逻辑：绘制下划线
                    drawUnderlineForIconEx(0);
                }
                
				inkScreenTestFlag = 0;
			break;
			case 2:  // 按键2：确认/选择
                update_activity_time(); 
                
                // 优先检查是否为JSON布局模式
                if (g_json_rects != nullptr && g_json_rect_count > 0) {
                    // JSON布局模式：调用JSON确认处理
                    ESP_LOGI("JSON_KEY", "按键2：JSON布局确认操作");
                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                    jsonLayoutConfirm();
                    
                    // 子母数组切换后更新显示
                    // updateDisplayWithMain(g_json_rects, g_json_rect_count, -1, 1);
                    redrawJsonLayout();

                } else if (g_focus_mode_enabled) {
                    // 原有焦点模式：确认操作
                    handleFocusConfirm();
                    ESP_LOGI("FOCUS", "按键2：确认操作，当前焦点在矩形%d", g_current_focus_rect);

                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                    // 根据当前界面类型重新显示
                    displayScreen(g_screen_manager.current_screen);
                } else {
                    // 原有逻辑：绘制下划线
                    drawUnderlineForIconEx(1);
                }
                
				inkScreenTestFlag = 0;
			break;
			case 3:  // 按键3：下一个焦点/向下选择
                update_activity_time(); 
                
                // 优先检查是否为JSON布局模式
                if (g_json_rects != nullptr && g_json_rect_count > 0) {
                    // JSON布局模式：调用JSON按键处理
                    ESP_LOGI("JSON_KEY", "按键3：JSON布局向下移动焦点");
                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                    jsonLayoutFocusNext();
                    
                    // 焦点移动后更新显示
                    // updateDisplayWithMain(g_json_rects, g_json_rect_count, -1, 1);
                } else if (g_focus_mode_enabled) {
                    // 原有焦点模式：向下移动焦点
                    moveFocusNext();
                    ESP_LOGI("FOCUS", "按键3：焦点向下移动到矩形%d", g_current_focus_rect);
                        
                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
                    
                    // 根据当前界面类型重新显示
                    displayScreen(g_screen_manager.current_screen);
                } else {
                    // 原有逻辑：绘制下划线
                    drawUnderlineForIconEx(2);
                }
                
				inkScreenTestFlag = 0;
			break;
			case 4:
                update_activity_time(); 
                drawUnderlineForIconEx(3);
				inkScreenTestFlag = 0;
			break;
			case 5:
                update_activity_time(); 
				drawUnderlineForIconEx(4);
				inkScreenTestFlag = 0;
			break;
			case 6:
                update_activity_time(); 
                drawUnderlineForIconEx(5);
				inkScreenTestFlag = 0;
			break;
			case 7:
            {
                clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                // ESP_LOGI(TAG,"############显示主界面（使用新图标布局系统）\r\n");
                // update_activity_time(); 
                
                // // 直接从矩形数据计算有效矩形数量
                // extern RectInfo rects[MAX_MAIN_RECTS];
                // int valid_rect_count = 0;
                // for (int i = 0; i < MAX_MAIN_RECTS; i++) {
                //     if (rects[i].width > 0 && rects[i].height > 0) {
                //         valid_rect_count++;
                //         ESP_LOGI("FOCUS", "有效矩形%d: (%d,%d) %dx%d", i, 
                //                 rects[i].x, rects[i].y, 
                //                 rects[i].width, rects[i].height);
                //     } else {
                //         ESP_LOGI("FOCUS", "无效矩形%d: (%d,%d) %dx%d", i, 
                //                 rects[i].x, rects[i].y, 
                //                 rects[i].width, rects[i].height);
                //         break; // 遇到第一个无效矩形就停止计数
                //     }
                // }
                
                // ESP_LOGI("FOCUS", "检测到有效矩形数量: %d", valid_rect_count);
                
                // // 使用实际检测到的有效矩形数量初始化焦点系统
                // initFocusSystem(valid_rect_count);
                
                // // 尝试从配置文件加载自定义的可焦点矩形列表（母数组）
                // if (loadFocusableRectsFromConfig("main")) {
                //     ESP_LOGI("FOCUS", "已从配置文件加载主界面焦点矩形列表");
                // } else {
                //     ESP_LOGI("FOCUS", "使用默认焦点配置（所有矩形都可焦点）");
                // }
                
                // // 加载子数组配置
                // if (loadAndApplySubArrayConfig("main")) {
                //     ESP_LOGI("FOCUS", "已从配置文件加载并应用主界面子数组配置");
                // } else {
                //     ESP_LOGI("FOCUS", "未加载主界面子数组配置或配置为空");
                // }
                
                // // 设置当前界面为主界面
                // g_screen_manager.current_screen = SCREEN_HOME;
                
                // // 显示主界面
                // displayScreen(SCREEN_HOME);
                switchToScreen(0); 
                interfaceIndex = 1;
				inkScreenTestFlag = 0;
            }
			break;
			case 8:  // 新增：启动焦点测试模式
                ESP_LOGI("FOCUS", "========== 按键8：启动焦点测试模式 ==========");
                startFocusTestMode();
                inkScreenTestFlag = 0;
            break;
            default:
                break;
		}
        showPromptInfor(showPrompt,true);
	//	updateDisplayWithWifiIcon();
        changeInkSreenSize();
       // ESP_LOGI(TAG,"ink_screen_show\r\n");
       
        // 更新番茄钟（在主循环中每次迭代都调用）
        updatePomodoro();
        
        vTaskDelay(100);
	}
}

void ink_screen_init()
{
    ESP_LOGI(TAG, "🔥 [DEBUG] ink_screen_init() 开始执行");
    // 注：移除 Uart0.printf() 避免 UART 驱动未初始化导致的错误
    
    ESP_LOGI(TAG, "🔥 [DEBUG] 1. 加载屏幕尺寸设置");
    WebUI::inkScreenXSizeSet->load();
    WebUI::inkScreenYSizeSet->load();
	setInkScreenSize.screenWidth = WebUI::inkScreenXSizeSet->get();
    setInkScreenSize.screenHeigt = WebUI::inkScreenYSizeSet->get();
        // 初始化 SD 卡（这是唯一一次必须的 get_sd_state(true) 调用）
         ESP_LOGI(TAG, "🔥 [DEBUG] 6. 初始化 SD 卡...");
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        if (state == SDState::NotPresent) {
            ESP_LOGE(TAG, "❌ 未检测到 SD 卡");
        } else {
            ESP_LOGE(TAG, "⚠️  SD 卡忙碌或出错");
        }
    } else {
        ESP_LOGI(TAG, "✅ SD 卡初始化成功");
    }

    // ===== 加载字体到 PSRAM (全量加载方案 - 包括 fangsong) =====
    ESP_LOGI(TAG, "========== 步骤 1: 加载完整字体到 PSRAM (包括fangsong) ==========");
    
    // 方案 A: 全自动扫描加载（包括 fangsong）
    int psram_fonts_loaded = initFullFontsInPSRAM(true);  // true = 加载所有字体包括fangsong
    
    // 方案 B: 手动加载特定 fangsong 字体（可选，如果方案A失败）
    // if (psram_fonts_loaded == 0) {
    //     ESP_LOGW(TAG, "自动扫描失败，尝试手动加载 fangsong...");
    //     if (loadSpecificFontToPSRAM("/sd/chinese_translate_font.bin", 20)) {
    //         psram_fonts_loaded++;
    //         ESP_LOGI(TAG, "✅ 手动加载 fangsong 20x20 成功");
    //     }
    // }
    
    if (psram_fonts_loaded > 0) {
        ESP_LOGI(TAG, "✅ 成功加载 %d 个字体到 PSRAM", psram_fonts_loaded);
        
        // 打印已加载的字体列表
        ESP_LOGI(TAG, "已加载的 PSRAM 字体列表:");
        for (int i = 0; i < getPSRAMFontCount(); i++) {
            const FullFontData* font = getPSRAMFontByIndex(i);
            if (font && font->is_loaded) {
                ESP_LOGI(TAG, "  [%d] %s (%dx%d, %.2f KB)", 
                         i, font->font_name, font->font_size, font->font_size,
                         font->size / 1024.0);
            }
        }
    } else {
        ESP_LOGW(TAG, "⚠️ 未加载任何字体到 PSRAM");
    }
    
    // ===== 步骤 2: 初始化单词本文本缓存（用于text_roll显示）=====
    // ESP_LOGI(TAG, "========== 步骤 2: 初始化单词本文本缓存 ==========");
    
    if (initWordBookTextCache()) {
        ESP_LOGI(TAG, "✅ 单词本文本缓存初始化成功");
        
        // 打印已加载的单词信息
        ESP_LOGI(TAG, "已缓存的单词列表:");
        for (int i = 0; i < WORDBOOK_CACHE_COUNT; i++) {
            ESP_LOGI(TAG, "  [$wordbook_idx=%d]", i);
            ESP_LOGI(TAG, "    word:        %s", getWordBookWord(i));
            ESP_LOGI(TAG, "    phonetic:    %s", getWordBookPhonetic(i));
            ESP_LOGI(TAG, "    translation: %s", getWordBookTranslation(i));
            ESP_LOGI(TAG, "    pos:         %s", getWordBookPos(i));
        }
        
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "JSON配置示例:");
        ESP_LOGI(TAG, "  {\"text_arr\": \"wordbook_word\", \"idx\": \"$wordbook_idx\"}        // 显示单词");
        ESP_LOGI(TAG, "  {\"text_arr\": \"wordbook_phonetic\", \"idx\": \"$wordbook_idx\"}   // 显示音标");
        ESP_LOGI(TAG, "  {\"text_arr\": \"wordbook_translation\", \"idx\": \"$wordbook_idx\"} // 显示翻译");
        ESP_LOGI(TAG, "  {\"text_arr\": \"wordbook_pos\", \"idx\": \"$wordbook_idx\"}        // 显示词性");
    } else {
        ESP_LOGW(TAG, "⚠️ 单词本文本缓存初始化失败");
    }
     // 1. 初始化单词本缓存（从SD卡加载）
    // if (initWordBookCache("/ecdict.mini.csv")) {
    //     ESP_LOGI(TAG, "✅ 单词本缓存初始化成功");
        
    //     // 2. 在墨水屏上显示3个单词（含完整信息：单词、词性、音标、翻译）
    //     // 注意：每个单词约占60-70px高度，建议显示3-4个
    //     testDisplayWordsOnScreen(display, 3);
        
    //     // 可选：打印到串口查看详细信息
    //     // printWordsFromCache(3);
    // } else {
    //     ESP_LOGE(TAG, "❌ 单词本缓存初始化失败");
        
    //     // 显示错误信息
    //     display.setFullWindow();
    //     display.firstPage();
    //     do {
    //         if (switchToPSRAMFont("chinese_translate_font")) {
    //             drawChineseTextWithCache(display, 10, 10, "错误：", GxEPD_BLACK);
    //             drawChineseTextWithCache(display, 10, 40, "无法加载单词本", GxEPD_BLACK);
    //         }
    //     } while (display.nextPage());
    // }

    // ===== 步骤 1: 初始化墨水屏专用的 SPI3 (HSPI) =====
    ESP_LOGI(TAG, "初始化墨水屏专用 SPI3 (SCK=48, MOSI=47)...");
    // 注意: EPD_SPI 是全局变量，在文件顶部定义
    // begin(SCK, MISO, MOSI, SS) - MISO可以设为-1（不使用）
    EPD_SPI.begin(BSP_SPI_CLK_GPIO_PIN, -1, BSP_SPI_MOSI_GPIO_PIN, BSP_SPI_CS_GPIO_PIN);
    ESP_LOGI(TAG, "墨水屏 SPI3 初始化完成");
    ESP_LOGI(TAG, "注意: SD 卡使用 SPI2 (默认 SPI), 墨水屏使用 SPI3 (EPD_SPI), 两者独立工作");
    
    // ===== 步骤 2: 初始化显示 (使用 EPD_SPI) =====
    ESP_LOGI(TAG, "初始化 GXEPD2 显示驱动...");
    display.epd2.selectSPI(EPD_SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));  // 设置使用 EPD_SPI
    ESP_LOGI(TAG, "准备调用 display.init(0)...");
    display.init(0);  // 初始化（0=不启用调试输出）
    vTaskDelay(500 / portTICK_PERIOD_MS);  // 增加延迟给屏幕初始化时间
    ESP_LOGI(TAG, "GXEPD2 初始化完成");
    
    // ===== 步骤 3: 设置旋转（可选）=====
    display.setRotation(1);  // 竖屏模式
    ESP_LOGI(TAG, "设置旋转: 竖屏模式 (rotation=1)");

    // ===== 步骤 4: 设置文本颜色 =====
    display.setTextColor(GxEPD_BLACK);
    
    // ===== 步骤 5: 首次全屏刷新（清白）=====
    ESP_LOGI(TAG, "准备首次全屏刷新（清白）...");
    display.setFullWindow();
    display.firstPage();
    {
        display.fillScreen(GxEPD_WHITE);
    }
    ESP_LOGI(TAG, "执行 display.nextPage()...");
    display.nextPage();
    ESP_LOGI(TAG, "display.nextPage() 完成，等待屏幕刷新...");
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // 等待屏幕完成刷新
    ESP_LOGI(TAG, "首次全屏刷新完成");
        // ===== 步骤 6: 预加载图标到PSRAM =====
    ESP_LOGI(TAG, "� [DEBUG] 6. 开始预加载图标到PSRAM...");
    if (preloadIconsFromSD()) {
        ESP_LOGI(TAG, "✅ 图标预加载完成");
    } else {
        ESP_LOGW(TAG, "⚠️ 图标预加载失败或部分失败");
    }
    
    ESP_LOGI(TAG, "� [DEBUG] 7. 初始化简化完成，准备创建任务");
    
    ESP_LOGI(TAG, "✅ 墨水屏初始化完成，现在只支持JSON布局系统");
    // 注：移除 Uart0.printf() 避免 UART 驱动问题

    ESP_LOGI(TAG, "🔥 [DEBUG] 7. 准备创建ink_screen_show任务");

    // 启动墨水屏任务来处理按键
    BaseType_t task_created = xTaskCreatePinnedToCore(ink_screen_show, 
                                                        "ink_screen_show", 
                                                        8192, 
                                                        NULL, 
                                                        4, 
                                                        &_eventTaskHandle, 
                                                        0);
    if (task_created == pdPASS) {
        ESP_LOGI(TAG, "✅ ink_screen_show任务已启动");
    } else {
        ESP_LOGE(TAG, "❌ ink_screen_show任务启动失败");
    }
    
    ESP_LOGI(TAG, "🔥 [DEBUG] 7. ink_screen_init() 执行完毕");
}

// ==================== 简化的本地JSON解析和显示功能 ====================

/**
 * @brief 从JSON字符串解析布局并显示到墨水屏
 * @param json_str JSON字符串内容
 * @return true 成功, false 失败
 */
bool loadAndDisplayFromJSON(const char* json_str) {
    uint32_t start_time = esp_timer_get_time() / 1000;  // 开始时间(毫秒)
    
    ESP_LOGI("JSON", "🔥 [DEBUG] loadAndDisplayFromJSON() 开始执行");
    
    if (!json_str) {
        ESP_LOGE("JSON", "JSON字符串为空");
        return false;
    }

    ESP_LOGI("JSON", "🔥 [DEBUG] JSON字符串验证通过，准备计算长度");
    
    // 打印内存和JSON长度信息
    size_t json_len = strlen(json_str);
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI("JSON", "JSON字符串长度: %zu 字节, 可用内存: %zu 字节", json_len, free_heap);

    ESP_LOGI("JSON", "🔥 [DEBUG] 准备解析JSON");
    // 解析JSON
    uint32_t parse_start = esp_timer_get_time() / 1000;
    cJSON* root = cJSON_Parse(json_str);
    uint32_t parse_end = esp_timer_get_time() / 1000;
    ESP_LOGI("JSON", "🔥 [DEBUG] JSON解析完成，耗时: %lu ms", parse_end - parse_start);
    
    if (!root) {
        const char* err = cJSON_GetErrorPtr();
        if (err) {
            ESP_LOGE("JSON", "JSON解析失败，位置附近: %.64s", err);
        } else {
            ESP_LOGE("JSON", "JSON解析失败");
        }
        return false;
    }

    // 获取矩形数量
    cJSON* rect_count_item = cJSON_GetObjectItem(root, "rect_count");
    if (!rect_count_item || !cJSON_IsNumber(rect_count_item)) {
        ESP_LOGE("JSON", "未找到rect_count字段");
        cJSON_Delete(root);
        return false;
    }
    int rect_count = rect_count_item->valueint;
    
    if (rect_count <= 0 || rect_count > MAX_RECTS) {
        ESP_LOGE("JSON", "矩形数量无效: %d", rect_count);
        cJSON_Delete(root);
        return false;
    }

    // 获取矩形数组
    cJSON* rectangles = cJSON_GetObjectItem(root, "rectangles");
    if (!rectangles || !cJSON_IsArray(rectangles)) {
        ESP_LOGE("JSON", "未找到rectangles数组");
        cJSON_Delete(root);
        return false;
    }

    // 创建矩形数组
    static RectInfo rects[MAX_RECTS];
    memset(rects, 0, sizeof(rects));

    // 解析每个矩形
    int actual_count = 0;
    cJSON* rect_item = NULL;
    cJSON_ArrayForEach(rect_item, rectangles) {
        if (actual_count >= rect_count) break;

        RectInfo* rect = &rects[actual_count];

        // 一次性获取所有基本字段，减少cJSON查找次数
        cJSON* x = cJSON_GetObjectItem(rect_item, "x");
        cJSON* y = cJSON_GetObjectItem(rect_item, "y");
        cJSON* width = cJSON_GetObjectItem(rect_item, "width");
        cJSON* height = cJSON_GetObjectItem(rect_item, "height");
        cJSON* x_rel = cJSON_GetObjectItem(rect_item, "x_");
        cJSON* y_rel = cJSON_GetObjectItem(rect_item, "y_");
        cJSON* width_rel = cJSON_GetObjectItem(rect_item, "width_");
        cJSON* height_rel = cJSON_GetObjectItem(rect_item, "height_");
        cJSON* focus_mode = cJSON_GetObjectItem(rect_item, "focus_mode");
        cJSON* is_mother = cJSON_GetObjectItem(rect_item, "is_mother");
        cJSON* group = cJSON_GetObjectItem(rect_item, "Group");
        cJSON* focus_icon = cJSON_GetObjectItem(rect_item, "focus_icon");
        cJSON* on_confirm_action = cJSON_GetObjectItem(rect_item, "on_confirm_action");
        cJSON* icons = cJSON_GetObjectItem(rect_item, "icons");
        cJSON* icon_roll = cJSON_GetObjectItem(rect_item, "icon_roll");
        cJSON* texts = cJSON_GetObjectItem(rect_item, "texts");
        cJSON* text_rolls = cJSON_GetObjectItem(rect_item, "text_roll");

        // 优先使用相对坐标，如果没有则使用绝对坐标
        if (x_rel && cJSON_IsNumber(x_rel)) {
            rect->x = (int)(x_rel->valuedouble * setInkScreenSize.screenWidth);
        } else if (x && cJSON_IsNumber(x)) {
            rect->x = x->valueint;
        }

        if (y_rel && cJSON_IsNumber(y_rel)) {
            rect->y = (int)(y_rel->valuedouble * setInkScreenSize.screenHeigt);
        } else if (y && cJSON_IsNumber(y)) {
            rect->y = y->valueint;
        }

        if (width_rel && cJSON_IsNumber(width_rel)) {
            rect->width = (int)(width_rel->valuedouble * setInkScreenSize.screenWidth);
        } else if (width && cJSON_IsNumber(width)) {
            rect->width = width->valueint;
        }

        if (height_rel && cJSON_IsNumber(height_rel)) {
            rect->height = (int)(height_rel->valuedouble * setInkScreenSize.screenHeigt);
        } else if (height && cJSON_IsNumber(height)) {
            rect->height = height->valueint;
        }

        if (focus_mode && cJSON_IsNumber(focus_mode)) rect->focus_mode = (FocusMode)focus_mode->valueint;

        // 解析focus_icon（焦点图标）
        rect->focus_icon_index = -1; // 默认值：使用默认焦点样式
        if (focus_icon && cJSON_IsString(focus_icon)) {
            int icon_index = getIconIndexByName(focus_icon->valuestring);
            if (icon_index >= 0) {
                rect->focus_icon_index = icon_index;
            }
        }

        // ========== 解析子母数组相关字段 ==========
        // 解析is_mother字段
        strcpy(rect->is_mother, "mom");  // 默认值
        if (is_mother && cJSON_IsString(is_mother)) {
            strncpy(rect->is_mother, is_mother->valuestring, sizeof(rect->is_mother) - 1);
            rect->is_mother[sizeof(rect->is_mother) - 1] = '\0';
        }

        // 解析Group字段（仅对母数组有效）
        rect->group_count = 0;
        memset(rect->group_indices, 0, sizeof(rect->group_indices));
        if (strcmp(rect->is_mother, "mom") == 0) {
            cJSON* group = cJSON_GetObjectItem(rect_item, "Group");
            if (group && cJSON_IsArray(group)) {
                int group_size = cJSON_GetArraySize(group);
                if (group_size > 8) group_size = 8;  // 最多支持8个子数组
                
                for (int i = 0; i < group_size; i++) {
                    cJSON* item = cJSON_GetArrayItem(group, i);
                    if (item && cJSON_IsNumber(item)) {
                        rect->group_indices[rect->group_count] = item->valueint;
                        rect->group_count++;
                    }
                }
                ESP_LOGI("JSON", "母数组%d包含%d个子数组", actual_count, rect->group_count);
            }
        }

        // 解析on_confirm_action
        if (on_confirm_action && cJSON_IsString(on_confirm_action)) {
            const char* action_id = on_confirm_action->valuestring;
            strncpy(rect->on_confirm_action, action_id, sizeof(rect->on_confirm_action) - 1);
            
            // 查找对应的回调函数
            rect->onConfirm = find_action_by_id(action_id);
            if (rect->onConfirm) {
                ESP_LOGI("JSON", "矩形%d绑定回调: %s", actual_count, action_id);
            }
        }

        // 解析静态图标（支持icon_name和icon_index）
        rect->icon_count = 0;
        if (icons && cJSON_IsArray(icons)) {
            int icon_count = 0;
            cJSON* icon_item = NULL;
            cJSON_ArrayForEach(icon_item, icons) {
                if (icon_count >= MAX_ICONS_PER_RECT) break;

                // 支持两种格式：icon_index (数字) 或 icon_name (字符串)
                cJSON* icon_index = cJSON_GetObjectItem(icon_item, "icon_index");
                cJSON* icon_name = cJSON_GetObjectItem(icon_item, "icon_name");
                cJSON* rel_x = cJSON_GetObjectItem(icon_item, "rel_x");
                cJSON* rel_y = cJSON_GetObjectItem(icon_item, "rel_y");

                int final_icon_index = -1;
                
                // 优先使用icon_name，如果没有则使用icon_index
                if (icon_name && cJSON_IsString(icon_name)) {
                    final_icon_index = getIconIndexByName(icon_name->valuestring);
                } else if (icon_index && cJSON_IsNumber(icon_index)) {
                    final_icon_index = icon_index->valueint;
                }

                if (final_icon_index >= 0 &&
                    rel_x && cJSON_IsNumber(rel_x) &&
                    rel_y && cJSON_IsNumber(rel_y)) {
                    
                    IconPositionInRect* icon = &rect->icons[icon_count];
                    icon->icon_index = final_icon_index;
                    icon->rel_x = (float)rel_x->valuedouble;
                    icon->rel_y = (float)rel_y->valuedouble;
                    icon_count++;
                }
            }
            rect->icon_count = icon_count;
        }

        // 解析动态图标组（icon_roll）
        cJSON* icon_rolls = cJSON_GetObjectItem(rect_item, "icon_roll");
        if (icon_rolls && cJSON_IsArray(icon_rolls)) {
            int icon_roll_count = 0;
            cJSON* icon_roll_item = NULL;
            cJSON_ArrayForEach(icon_roll_item, icon_rolls) {
                if (icon_roll_count >= 4) break; // 最多4个动态图标组

                cJSON* icon_arr = cJSON_GetObjectItem(icon_roll_item, "icon_arr");
                cJSON* idx = cJSON_GetObjectItem(icon_roll_item, "idx");
                cJSON* rel_x = cJSON_GetObjectItem(icon_roll_item, "rel_x");
                cJSON* rel_y = cJSON_GetObjectItem(icon_roll_item, "rel_y");
                cJSON* auto_roll = cJSON_GetObjectItem(icon_roll_item, "auto_roll");

                if (icon_arr && cJSON_IsString(icon_arr) &&
                    idx && cJSON_IsString(idx) &&
                    rel_x && cJSON_IsNumber(rel_x) &&
                    rel_y && cJSON_IsNumber(rel_y)) {
                    
                    IconRollInRect* icon_roll = &rect->icon_rolls[icon_roll_count];
                    
                    // 复制字符串，确保不超出缓冲区
                    strncpy(icon_roll->icon_arr, icon_arr->valuestring, sizeof(icon_roll->icon_arr) - 1);
                    icon_roll->icon_arr[sizeof(icon_roll->icon_arr) - 1] = '\0';
                    
                    strncpy(icon_roll->idx, idx->valuestring, sizeof(icon_roll->idx) - 1);
                    icon_roll->idx[sizeof(icon_roll->idx) - 1] = '\0';
                    
                    icon_roll->rel_x = (float)rel_x->valuedouble;
                    icon_roll->rel_y = (float)rel_y->valuedouble;
                    
                    // 解析auto_roll字段，默认为false
                    icon_roll->auto_roll = false;
                    if (auto_roll && cJSON_IsBool(auto_roll)) {
                        icon_roll->auto_roll = cJSON_IsTrue(auto_roll);
                    }
                    
                    ESP_LOGI("JSON", "解析动态图标组%d: arr=%s, idx=%s, pos=(%.2f,%.2f), auto_roll=%s", 
                            icon_roll_count, icon_roll->icon_arr, icon_roll->idx, 
                            icon_roll->rel_x, icon_roll->rel_y, icon_roll->auto_roll ? "true" : "false");
                    
                    icon_roll_count++;
                }
            }
            rect->icon_roll_count = icon_roll_count;
        } else {
            rect->icon_roll_count = 0;
        }

        // 解析文本（如果需要）
        if (texts && cJSON_IsArray(texts)) {
            // TODO: 文本解析逻辑（如果需要）
            rect->text_count = 0;
        }

        // 解析动态文本组（text_roll）
        rect->text_roll_count = 0;
        ESP_LOGI("JSON_DEBUG", "准备解析text_roll, text_rolls指针=%p, 是否为数组=%d", 
                text_rolls, text_rolls ? cJSON_IsArray(text_rolls) : -1);
        if (text_rolls && cJSON_IsArray(text_rolls)) {
            int text_roll_count = 0;
            int array_size = cJSON_GetArraySize(text_rolls);
            ESP_LOGI("JSON_DEBUG", "text_roll数组大小=%d", array_size);
            cJSON* text_roll_item = NULL;
            cJSON_ArrayForEach(text_roll_item, text_rolls) {
                if (text_roll_count >= 4) break; // 最多4个动态文本组

                cJSON* text_arr = cJSON_GetObjectItem(text_roll_item, "text_arr");
                cJSON* idx = cJSON_GetObjectItem(text_roll_item, "idx");
                cJSON* rel_x = cJSON_GetObjectItem(text_roll_item, "rel_x");
                cJSON* rel_y = cJSON_GetObjectItem(text_roll_item, "rel_y");
                cJSON* font = cJSON_GetObjectItem(text_roll_item, "font");
                cJSON* auto_roll = cJSON_GetObjectItem(text_roll_item, "auto_roll");

                if (text_arr && cJSON_IsString(text_arr) &&
                    idx && cJSON_IsString(idx) &&
                    rel_x && cJSON_IsNumber(rel_x) &&
                    rel_y && cJSON_IsNumber(rel_y)) {
                    
                    TextRollInRect* text_roll = &rect->text_rolls[text_roll_count];
                    
                    // 复制字符串，确保不超出缓冲区
                    strncpy(text_roll->text_arr, text_arr->valuestring, sizeof(text_roll->text_arr) - 1);
                    text_roll->text_arr[sizeof(text_roll->text_arr) - 1] = '\0';
                    
                    strncpy(text_roll->idx, idx->valuestring, sizeof(text_roll->idx) - 1);
                    text_roll->idx[sizeof(text_roll->idx) - 1] = '\0';
                    
                    // 解析font字段，如果没有则为空（将使用默认字体逻辑）
                    if (font && cJSON_IsString(font)) {
                        strncpy(text_roll->font, font->valuestring, sizeof(text_roll->font) - 1);
                        text_roll->font[sizeof(text_roll->font) - 1] = '\0';
                    } else {
                        text_roll->font[0] = '\0';  // 空字符串表示使用默认字体
                    }
                    
                    text_roll->rel_x = (float)rel_x->valuedouble;
                    text_roll->rel_y = (float)rel_y->valuedouble;
                    
                    // 解析auto_roll字段，默认为false
                    text_roll->auto_roll = false;
                    if (auto_roll && cJSON_IsBool(auto_roll)) {
                        text_roll->auto_roll = cJSON_IsTrue(auto_roll);
                    }
                    
                    ESP_LOGI("JSON", "解析动态文本组%d: arr=%s, idx=%s, font=%s, pos=(%.2f,%.2f), auto_roll=%s", 
                            text_roll_count, text_roll->text_arr, text_roll->idx, 
                            text_roll->font[0] ? text_roll->font : "auto",
                            text_roll->rel_x, text_roll->rel_y, text_roll->auto_roll ? "true" : "false");
                    
                    text_roll_count++;
                }
            }
            rect->text_roll_count = text_roll_count;
        } else {
            rect->text_roll_count = 0;
        }

        actual_count++;
    }

    cJSON_Delete(root);
    uint32_t parse_total = esp_timer_get_time() / 1000;

    ESP_LOGI("JSON", "成功解析%d个矩形，解析耗时: %lu ms", actual_count, parse_total - start_time);

    // 清除屏幕旧内容（重要！避免新旧图标叠加）
    uint32_t display_start = esp_timer_get_time() / 1000;
    ESP_LOGI("JSON", "开始清屏和显示...");
    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
    
    // 保存布局数据供按键交互使用（需要在initFocusSystem之前调用）
    saveJsonLayoutForInteraction(rects, actual_count, -1);
    
    // 初始化焦点系统（会自动找到第一个mom类型的矩形）
    initFocusSystem(actual_count);
    g_in_sub_array = false;
    
    // 显示到墨水屏
    updateDisplayWithMain(rects, actual_count, -1, 1);  // -1表示没有专门的状态栏，1表示显示边框

    uint32_t total_time = esp_timer_get_time() / 1000 - start_time;
    uint32_t display_time = esp_timer_get_time() / 1000 - display_start;
    ESP_LOGI("JSON", "布局显示完成！总耗时: %lu ms (解析: %lu ms, 显示: %lu ms)", 
             total_time, parse_total - start_time, display_time);
    return true;
}

/**
 * @brief 从文件读取JSON并显示
 * @param file_path 文件路径
 * @return true 成功, false 失败
 */
bool loadAndDisplayFromFile(const char* file_path) {
    ESP_LOGI("JSON", "🔥 使用流式解析，无需加载整个文件到内存");
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        ESP_LOGE("JSON", "无法打开文件: %s", file_path);
        return false;
    }

    // 获取文件大小（仅用于日志）
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    ESP_LOGI("JSON", "文件大小: %ld 字节，开始流式解析", file_size);

    // 使用小缓冲区逐行读取
    const size_t buffer_size = 512;  // 512字节缓冲区足够读取一行
    char* line_buffer = (char*)malloc(buffer_size);
    if (!line_buffer) {
        ESP_LOGE("JSON", "无法分配512字节行缓冲区");
        fclose(file);
        return false;
    }
    
    ESP_LOGI("JSON", "开始流式解析JSON文件");
    
    // 读取矩形数量
    int rect_count = 0;
    int status_rect_index = -1;
    bool found_rect_count = false;
    bool found_status_index = false;
    
    // 扫描文件查找rect_count和status_rect_index
    while (fgets(line_buffer, buffer_size, file)) {
        if (!found_rect_count && strstr(line_buffer, "\"rect_count\"")) {
            sscanf(line_buffer, " \"rect_count\" : %d", &rect_count);
            found_rect_count = true;
            ESP_LOGI("JSON", "找到rect_count: %d", rect_count);
        }
        if (!found_status_index && strstr(line_buffer, "\"status_rect_index\"")) {
            sscanf(line_buffer, " \"status_rect_index\" : %d", &status_rect_index);
            found_status_index = true;
            ESP_LOGI("JSON", "找到status_rect_index: %d", status_rect_index);
        }
        if (found_rect_count && found_status_index) {
            break;
        }
    }
    
    if (!found_rect_count || rect_count <= 0 || rect_count > 50) {
        ESP_LOGE("JSON", "无效的rect_count: %d", rect_count);
        free(line_buffer);
        fclose(file);
        return false;
    }
    
    // 分配矩形数组
    RectInfo* rects = (RectInfo*)malloc(rect_count * sizeof(RectInfo));
    if (!rects) {
        ESP_LOGE("JSON", "无法分配矩形数组");
        free(line_buffer);
        fclose(file);
        return false;
    }
    
    ESP_LOGI("JSON", "已分配%d个矩形的数组，开始流式解析矩形数据", rect_count);
    
    // 重置文件指针，查找rectangles数组
    fseek(file, 0, SEEK_SET);
    bool in_rectangles = false;
    int current_rect = 0;
    RectInfo temp_rect = {};
    bool parsing_rect = false;
    bool in_icons = false;
    bool in_text_roll = false;
    bool in_group_array = false;  // 标记是否在Group数组中
    int current_icon = 0;
    int current_text_roll = 0;
    char temp_icon_name[32] = {0};
    
    while (fgets(line_buffer, buffer_size, file) && current_rect < rect_count) {
        // 移除行尾的换行符和空格
        size_t len = strlen(line_buffer);
        while (len > 0 && (line_buffer[len-1] == '\n' || line_buffer[len-1] == '\r' || line_buffer[len-1] == ' ')) {
            line_buffer[--len] = '\0';
        }
        
        // 检测进入rectangles数组
        if (strstr(line_buffer, "\"rectangles\"")) {
            in_rectangles = true;
            ESP_LOGI("JSON", "找到rectangles数组");
            continue;
        }
        
        if (!in_rectangles) continue;
        
        // 检测矩形对象开始（包含"index"的行是矩形开始）
        if (strstr(line_buffer, "\"index\"") && !parsing_rect) {
            parsing_rect = true;
            memset(&temp_rect, 0, sizeof(RectInfo));
        }
        
        // 解析矩形属性
        if (parsing_rect) {
            if (strstr(line_buffer, "\"x_\"")) {
                float x_rel;
                sscanf(line_buffer, " \"x_\" : %f", &x_rel);
                temp_rect.x = (int)(x_rel * 416);  // 416是屏幕宽度
            }
            else if (strstr(line_buffer, "\"y_\"")) {
                float y_rel;
                sscanf(line_buffer, " \"y_\" : %f", &y_rel);
                temp_rect.y = (int)(y_rel * 240);  // 240是屏幕高度
            }
            else if (strstr(line_buffer, "\"width_\"")) {
                float w_rel;
                sscanf(line_buffer, " \"width_\" : %f", &w_rel);
                temp_rect.width = (int)(w_rel * 416);
            }
            else if (strstr(line_buffer, "\"height_\"")) {
                float h_rel;
                sscanf(line_buffer, " \"height_\" : %f", &h_rel);
                temp_rect.height = (int)(h_rel * 240);
            }
            else if (strstr(line_buffer, "\"focus_mode\"")) {
                int focus_val;
                sscanf(line_buffer, " \"focus_mode\" : %d", &focus_val);
                // 0=默认(钉子), 1=四角, 2=边框
                if (focus_val == 0) temp_rect.focus_mode = FOCUS_MODE_DEFAULT;
                else if (focus_val == 1) temp_rect.focus_mode = FOCUS_MODE_CORNERS;
                else if (focus_val == 2) temp_rect.focus_mode = FOCUS_MODE_BORDER;
                else temp_rect.focus_mode = FOCUS_MODE_DEFAULT;
            }
            else if (strstr(line_buffer, "\"is_mother\"")) {
                // 解析is_mother字段: "non", "mom", "son"
                char mother_type[16] = {0};
                sscanf(line_buffer, " \"is_mother\" : \"%15[^\"]\"", mother_type);
                strncpy(temp_rect.is_mother, mother_type, sizeof(temp_rect.is_mother) - 1);
                temp_rect.is_mother[sizeof(temp_rect.is_mother) - 1] = '\0';
            }
            else if (strstr(line_buffer, "\"focus_icon\"")) {
                // 解析focus_icon字段: "nail", "corner", "border" 等
                char icon_name[32] = {0};
                sscanf(line_buffer, " \"focus_icon\" : \"%31[^\"]\"", icon_name);
                temp_rect.focus_icon_index = getIconIndexByName(icon_name);
            }
            else if (strstr(line_buffer, "\"on_confirm_action\"")) {
                // 解析on_confirm_action字段
                char action_name[32] = {0};
                sscanf(line_buffer, " \"on_confirm_action\" : \"%31[^\"]\"", action_name);
                strncpy(temp_rect.on_confirm_action, action_name, sizeof(temp_rect.on_confirm_action) - 1);
                temp_rect.on_confirm_action[sizeof(temp_rect.on_confirm_action) - 1] = '\0';
                // 查找对应的回调函数
                temp_rect.onConfirm = find_action_by_id(action_name);
            }
            else if (strstr(line_buffer, "\"icon_count\"")) {
                sscanf(line_buffer, " \"icon_count\" : %d", &temp_rect.icon_count);
            }
            else if (strstr(line_buffer, "\"text_count\"")) {
                sscanf(line_buffer, " \"text_count\" : %d", &temp_rect.text_count);
            }
            else if (strstr(line_buffer, "\"Group\"")) {
              // 检测Group数组开始
                if (strstr(line_buffer, "[")) {
                    in_group_array = true;
                    temp_rect.group_count = 0;
                    
                    // 检查是否在同一行结束 "Group": [1, 2]
                    char* bracket_end = strchr(line_buffer, ']');
                    if (bracket_end) {
                        // 单行数组，按原逻辑处理
                        char* bracket_start = strchr(line_buffer, '[');
                        if (bracket_start && bracket_end > bracket_start) {
                            char group_str[64] = {0};
                            int len = bracket_end - bracket_start - 1;
                            if (len > 0 && len < 63) {
                                strncpy(group_str, bracket_start + 1, len);
                                group_str[len] = '\0';
                                char* token = strtok(group_str, ", ");
                                while (token && temp_rect.group_count < 8) {
                                    temp_rect.group_indices[temp_rect.group_count] = atoi(token);
                                    temp_rect.group_count++;
                                    token = strtok(NULL, ", ");
                                }
                            }
                        }
                        in_group_array = false;
                    }
                }
            }
            // 在Group数组中，逐行读取数字
            else if (in_group_array) {
                // 检测数组结束
                if (strstr(line_buffer, "]")) {
                    in_group_array = false;
                    ESP_LOGI("CACHE", "矩形%d Group数组解析完成，共%d个元素", current_rect, temp_rect.group_count);
                } else {
                    // 提取当前行的数字
                    char* p = line_buffer;
                    while (*p && temp_rect.group_count < 8) {
                        if (isdigit(*p)) {
                            int num = atoi(p);
                            temp_rect.group_indices[temp_rect.group_count] = num;
                            temp_rect.group_count++;
                            // 跳过当前数字
                            while (*p && isdigit(*p)) p++;
                        } else {
                            p++;
                        }
                    }
                }
            }
            
            // 检测进入icons数组
            if (strstr(line_buffer, "\"icons\"") && strstr(line_buffer, "[")) {
                in_icons = true;
                current_icon = 0;
            }
            // 检测退出icons数组
            else if (in_icons && strstr(line_buffer, "]") && !strstr(line_buffer, "\"")) {
                in_icons = false;
            }
            // 解析icon对象
            else if (in_icons) {
                if (strstr(line_buffer, "\"icon_name\"")) {
                    sscanf(line_buffer, " \"icon_name\" : \"%31[^\"]\"", temp_icon_name);
                }
                else if (strstr(line_buffer, "\"rel_x\"")) {
                    float rel_x;
                    sscanf(line_buffer, " \"rel_x\" : %f", &rel_x);
                    if (current_icon < 4) {
                        temp_rect.icons[current_icon].rel_x = rel_x;
                    }
                }
                else if (strstr(line_buffer, "\"rel_y\"")) {
                    float rel_y;
                    sscanf(line_buffer, " \"rel_y\" : %f", &rel_y);
                    if (current_icon < 4) {
                        temp_rect.icons[current_icon].rel_y = rel_y;
                        temp_rect.icons[current_icon].icon_index = getIconIndexByName(temp_icon_name);
                        current_icon++;
                    }
                }
            }
            
            // 检测进入text_roll数组
            if (strstr(line_buffer, "\"text_roll\"") && strstr(line_buffer, "[")) {
                in_text_roll = true;
                current_text_roll = 0;
            }
            // 检测退出text_roll数组
            else if (in_text_roll && strstr(line_buffer, "]") && !strstr(line_buffer, "\"")) {
                in_text_roll = false;
            }
            // 解析text_roll对象
            else if (in_text_roll) {
                if (strstr(line_buffer, "\"text_arr\"")) {
                    if (current_text_roll < 4) {
                        sscanf(line_buffer, " \"text_arr\" : \"%31[^\"]\"", temp_rect.text_rolls[current_text_roll].text_arr);
                    }
                }
                else if (strstr(line_buffer, "\"idx\"")) {
                    if (current_text_roll < 4) {
                        sscanf(line_buffer, " \"idx\" : \"%15[^\"]\"", temp_rect.text_rolls[current_text_roll].idx);
                    }
                }
                else if (strstr(line_buffer, "\"font\"")) {
                    if (current_text_roll < 4) {
                        sscanf(line_buffer, " \"font\" : \"%31[^\"]\"", temp_rect.text_rolls[current_text_roll].font);
                    }
                }
                else if (strstr(line_buffer, "\"rel_x\"")) {
                    float rel_x;
                    sscanf(line_buffer, " \"rel_x\" : %f", &rel_x);
                    if (current_text_roll < 4) {
                        temp_rect.text_rolls[current_text_roll].rel_x = rel_x;
                    }
                }
                else if (strstr(line_buffer, "\"rel_y\"")) {
                    float rel_y;
                    sscanf(line_buffer, " \"rel_y\" : %f", &rel_y);
                    if (current_text_roll < 4) {
                        temp_rect.text_rolls[current_text_roll].rel_y = rel_y;
                    }
                }
                else if (strstr(line_buffer, "\"auto_roll\"")) {
                    if (current_text_roll < 4) {
                        temp_rect.text_rolls[current_text_roll].auto_roll = strstr(line_buffer, "true") != NULL;
                        current_text_roll++;
                        temp_rect.text_roll_count = current_text_roll;
                    }
                }
            }
            
            // 检测矩形对象结束
            if (strstr(line_buffer, "}") && strstr(line_buffer, ",") == NULL) {
                // 确保这是矩形对象的结束，而不是嵌套对象
                rects[current_rect] = temp_rect;
                current_rect++;
                parsing_rect = false;
                in_icons = false;
                in_text_roll = false;
                ESP_LOGI("JSON", "矩形 %d: (%d,%d) %dx%d, is_mother:%s, icons:%d, text_rolls:%d", 
                         current_rect, temp_rect.x, temp_rect.y, 
                         temp_rect.width, temp_rect.height, 
                         temp_rect.is_mother,
                         temp_rect.icon_count, temp_rect.text_roll_count);
                
                // 每解析5个矩形就喂一次看门狗
                if (current_rect % 5 == 0) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            }
        }
    }
    
    free(line_buffer);
    fclose(file);
    
    if (current_rect != rect_count) {
        ESP_LOGW("JSON", "解析的矩形数量(%d)与声明的不一致(%d)", current_rect, rect_count);
        rect_count = current_rect;  // 使用实际解析的数量
    }
    
    ESP_LOGI("JSON", "✅ 流式解析完成！共解析 %d 个矩形", rect_count);
    
    // 保存布局数据供按键交互使用
    saveJsonLayoutForInteraction(rects, rect_count, status_rect_index);
    
    // 初始化焦点系统
    initFocusSystem(rect_count);
    ESP_LOGI("JSON", "✅ 焦点系统已初始化，共 %d 个可焦点矩形", rect_count);
    
    // 显示到墨水屏
    ESP_LOGI("JSON", "开始显示到墨水屏...");
    updateDisplayWithMain(rects, rect_count, status_rect_index, 1);
    ESP_LOGI("JSON", "✅ 显示完成！");
    
    // 不释放rects，保留给交互系统使用
    return true;
}

// ==================== JSON布局的按键交互支持（实现） ====================

/**
 * @brief 保存JSON布局数据供按键交互使用
 */
void saveJsonLayoutForInteraction(RectInfo* rects, int rect_count, int status_rect_index) {
    g_json_rects = rects;
    g_json_rect_count = rect_count;
    g_json_status_rect_index = status_rect_index;
}

/**
 * @brief 重绘当前JSON布局（用于焦点变化后刷新显示）
 */
void redrawJsonLayout() {
    if (g_json_rects == nullptr || g_json_rect_count == 0) {
        ESP_LOGW("JSON", "没有可重绘的JSON布局");
        return;
    }
    
    ESP_LOGI("JSON", "重绘JSON布局...");
    updateDisplayWithMain(g_json_rects, g_json_rect_count, g_json_status_rect_index, 1);
}

/**
 * @brief 按键：向下移动焦点（用于JSON布局）
 */
void jsonLayoutFocusNext() {
    ESP_LOGI("JSON", "jsonLayoutFocusNext called");
    moveFocusNext();
    redrawJsonLayout();
    ESP_LOGI("JSON", "焦点向下，当前焦点矩形: %d", getCurrentFocusRect());
}

/**
 * @brief 按键：向上移动焦点（用于JSON布局）
 */
void jsonLayoutFocusPrev() {
    moveFocusPrev();
    redrawJsonLayout();
    ESP_LOGI("JSON", "焦点向上，当前焦点矩形: %d", getCurrentFocusRect());
}

/**
 * @brief 按键：确认当前焦点矩形（触发回调并处理子母数组切换）
 */
void jsonLayoutConfirm() {
    if (g_json_rects == nullptr || g_json_rect_count == 0) {
        ESP_LOGW("JSON", "没有可确认的JSON布局");
        return;
    }
    
    int current = getCurrentFocusRect();
    if (current >= 0 && current < g_json_rect_count) {
        RectInfo* rect = &g_json_rects[current];
        // 调试信息：打印矩形详细信息
        ESP_LOGI("JSON", "确认操作：矩形%d", current);
        ESP_LOGI("JSON", "  is_mother='%s'", rect->is_mother);
        ESP_LOGI("JSON", "  group_count=%d", rect->group_count);
        ESP_LOGI("JSON", "  g_in_sub_array=%d", g_in_sub_array);
        // 先触发回调
        if (rect->onConfirm != nullptr) {
            rect->onConfirm(rect, current);
            ESP_LOGI("JSON", "触发矩形%d的回调", current);
        } else {
            ESP_LOGI("JSON", "矩形%d没有绑定回调", current);
        }
        
        // 回调后处理子母数组切换逻辑
        bool need_redraw = false;
        if (!g_in_sub_array) {
            // 当前在母数组模式，检查是否需要进入子数组
            if (strcmp(rect->is_mother, "mom") == 0 && rect->group_count > 0) {
                ESP_LOGI("JSON", "进入矩形%d的子数组", current);
                if (enterSubArray()) {
                    need_redraw = true;
                }
            }
        } else {
                // 当前在子数组模式，退出到母数组
                ESP_LOGI("JSON", "从子数组退出到母数组");
                exitSubArray();
                need_redraw = true;
            }
        
        // 如果发生了子母数组切换，重绘界面
        if (need_redraw) {
            ESP_LOGI("JSON", "子母数组切换完成，重绘界面");
            redrawJsonLayout();
        }
    }
}

// ==================== 界面缓存管理系统实现 ====================

// 全局界面缓存数组
static ScreenCache g_screen_cache[MAX_CACHED_SCREENS];
static int g_screen_cache_count = 0;
static int g_current_screen_index = -1;
//用到
/**
 * @brief 从文件加载界面但不显示（仅解析到内存）
 */
bool loadScreenToMemory(const char* file_path, RectInfo** out_rects, 
                        int* out_rect_count, int* out_status_index) {
    if (!file_path || !out_rects || !out_rect_count || !out_status_index) {
        ESP_LOGE("CACHE", "无效参数");
        return false;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        ESP_LOGE("CACHE", "无法打开文件: %s", file_path);
        return false;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    ESP_LOGI("CACHE", "加载 %s (大小: %ld 字节)", file_path, file_size);

    // 使用小缓冲区逐行读取
    const size_t buffer_size = 512;
    char* line_buffer = (char*)malloc(buffer_size);
    if (!line_buffer) {
        ESP_LOGE("CACHE", "无法分配行缓冲区");
        fclose(file);
        return false;
    }
    
    // 读取rect_count和status_rect_index
    int rect_count = 0;
    int status_rect_index = -1;
    bool found_rect_count = false;
    bool found_status_index = false;
    
    ESP_LOGI("CACHE", "开始第一次扫描：查找rect_count和status_rect_index...");
    
    while (fgets(line_buffer, buffer_size, file)) {
        if (!found_rect_count && strstr(line_buffer, "\"rect_count\"")) {
            sscanf(line_buffer, " \"rect_count\" : %d", &rect_count);
            found_rect_count = true;
            ESP_LOGI("CACHE", "找到rect_count: %d", rect_count);
        }
        if (!found_status_index && strstr(line_buffer, "\"status_rect_index\"")) {
            sscanf(line_buffer, " \"status_rect_index\" : %d", &status_rect_index);
            found_status_index = true;
            ESP_LOGI("CACHE", "找到status_rect_index: %d", status_rect_index);
        }
        if (found_rect_count && found_status_index) {
            break;
        }
    }
    
    ESP_LOGI("CACHE", "第一次扫描完成");
    
    if (!found_rect_count || rect_count <= 0 || rect_count > 50) {
        ESP_LOGE("CACHE", "无效的rect_count: %d", rect_count);
        free(line_buffer);
        fclose(file);
        return false;
    }
    
    // 使用PSRAM分配矩形数组（优先使用外部RAM）
    size_t alloc_size = rect_count * sizeof(RectInfo);
    ESP_LOGI("CACHE", "准备分配PSRAM内存: %d个矩形 × %d字节 = %d字节", 
             rect_count, sizeof(RectInfo), alloc_size);
    ESP_LOGI("CACHE", "当前PSRAM可用: %d字节", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    
    // 直接使用malloc，系统会自动选择PSRAM（因为配置了SPIRAM_USE_MALLOC）
    RectInfo* rects = (RectInfo*)malloc(alloc_size);
    
    ESP_LOGI("CACHE", "malloc调用完成");
    
    if (!rects) {
        ESP_LOGE("CACHE", "无法分配矩形数组 (需要 %d 字节)", alloc_size);
        free(line_buffer);
        fclose(file);
        return false;
    }
    
    ESP_LOGI("CACHE", "✅ 已分配%d个矩形的数组", rect_count);
    
    // 重置文件指针，解析矩形数据（复用原有的流式解析逻辑）
    ESP_LOGI("CACHE", "开始第二次扫描：解析矩形数据...");
    fseek(file, 0, SEEK_SET);
    bool in_rectangles = false;
    int current_rect = 0;
    RectInfo temp_rect = {};
    bool parsing_rect = false;
    bool in_icons = false;
    bool in_text_roll = false;
    bool in_group_array = false;  // 新增：标记是否在Group数组中
    int current_icon = 0;
    int current_text_roll = 0;
    char temp_icon_name[32] = {0};
    int line_count = 0;  // 行计数器，用于调试
    
    while (fgets(line_buffer, buffer_size, file) && current_rect < rect_count) {
        line_count++;
        
        // 每100行喂一次狗并打印进度
        if (line_count % 100 == 0) {
            ESP_LOGI("CACHE", "解析进度: 已读取%d行，已解析%d个矩形", line_count, current_rect);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // 移除行尾的换行符和空格
        size_t len = strlen(line_buffer);
        while (len > 0 && (line_buffer[len-1] == '\n' || line_buffer[len-1] == '\r' || line_buffer[len-1] == ' ')) {
            line_buffer[--len] = '\0';
        }
        
        // 检测进入rectangles数组
        if (strstr(line_buffer, "\"rectangles\"")) {
            in_rectangles = true;
            continue;
        }
        
        if (!in_rectangles) continue;
        
        // 检测矩形对象开始
        if (strstr(line_buffer, "\"index\"") && !parsing_rect) {
            parsing_rect = true;
            memset(&temp_rect, 0, sizeof(RectInfo));
        }
        
        // 解析矩形属性（简化版，只解析核心字段）
        if (parsing_rect) {
            if (strstr(line_buffer, "\"x_\"")) {
                float x_rel;
                sscanf(line_buffer, " \"x_\" : %f", &x_rel);
                temp_rect.x = (int)(x_rel * 416);
            }
            else if (strstr(line_buffer, "\"y_\"")) {
                float y_rel;
                sscanf(line_buffer, " \"y_\" : %f", &y_rel);
                temp_rect.y = (int)(y_rel * 240);
            }
            else if (strstr(line_buffer, "\"width_\"")) {
                float w_rel;
                sscanf(line_buffer, " \"width_\" : %f", &w_rel);
                temp_rect.width = (int)(w_rel * 416);
            }
            else if (strstr(line_buffer, "\"height_\"")) {
                float h_rel;
                sscanf(line_buffer, " \"height_\" : %f", &h_rel);
                temp_rect.height = (int)(h_rel * 240);
            }
            else if (strstr(line_buffer, "\"focus_mode\"")) {
                int focus_val;
                sscanf(line_buffer, " \"focus_mode\" : %d", &focus_val);
                if (focus_val == 0) temp_rect.focus_mode = FOCUS_MODE_DEFAULT;
                else if (focus_val == 1) temp_rect.focus_mode = FOCUS_MODE_CORNERS;
                else if (focus_val == 2) temp_rect.focus_mode = FOCUS_MODE_BORDER;
                else temp_rect.focus_mode = FOCUS_MODE_DEFAULT;
            }
            else if (strstr(line_buffer, "\"is_mother\"")) {
                char mother_type[16] = {0};
                sscanf(line_buffer, " \"is_mother\" : \"%15[^\"]\"", mother_type);
                strncpy(temp_rect.is_mother, mother_type, sizeof(temp_rect.is_mother) - 1);
                temp_rect.is_mother[sizeof(temp_rect.is_mother) - 1] = '\0';
            }
            else if (strstr(line_buffer, "\"focus_icon\"")) {
                char icon_name[32] = {0};
                sscanf(line_buffer, " \"focus_icon\" : \"%31[^\"]\"", icon_name);
                temp_rect.focus_icon_index = getIconIndexByName(icon_name);
            }
            else if (strstr(line_buffer, "\"on_confirm_action\"")) {
                char action_name[32] = {0};
                sscanf(line_buffer, " \"on_confirm_action\" : \"%31[^\"]\"", action_name);
                strncpy(temp_rect.on_confirm_action, action_name, sizeof(temp_rect.on_confirm_action) - 1);
                temp_rect.on_confirm_action[sizeof(temp_rect.on_confirm_action) - 1] = '\0';
                temp_rect.onConfirm = find_action_by_id(action_name);
            }
            else if (strstr(line_buffer, "\"icon_count\"")) {
                sscanf(line_buffer, " \"icon_count\" : %d", &temp_rect.icon_count);
            }
            else if (strstr(line_buffer, "\"text_count\"")) {
                sscanf(line_buffer, " \"text_count\" : %d", &temp_rect.text_count);
            }
            else if (strstr(line_buffer, "\"Group\"")) {
                // 检测Group数组开始
                if (strstr(line_buffer, "[")) {
                    in_group_array = true;
                    temp_rect.group_count = 0;
                    
                    // 检查是否在同一行结束 "Group": [1, 2]
                    char* bracket_end = strchr(line_buffer, ']');
                    if (bracket_end) {
                        // 单行数组，按原逻辑处理
                        char* bracket_start = strchr(line_buffer, '[');
                        if (bracket_start && bracket_end > bracket_start) {
                            char group_str[64] = {0};
                            int len = bracket_end - bracket_start - 1;
                            if (len > 0 && len < 63) {
                                strncpy(group_str, bracket_start + 1, len);
                                group_str[len] = '\0';
                                char* token = strtok(group_str, ", ");
                                while (token && temp_rect.group_count < 8) {
                                    temp_rect.group_indices[temp_rect.group_count] = atoi(token);
                                    temp_rect.group_count++;
                                    token = strtok(NULL, ", ");
                                }
                            }
                        }
                        in_group_array = false;
                    }
                }
            }
            // 在Group数组中，逐行读取数字
            else if (in_group_array) {
                // 检测数组结束
                if (strstr(line_buffer, "]")) {
                    in_group_array = false;
                    ESP_LOGI("CACHE", "矩形%d Group数组解析完成，共%d个元素", current_rect, temp_rect.group_count);
                } else {
                    // 提取当前行的数字
                    char* p = line_buffer;
                    while (*p && temp_rect.group_count < 8) {
                        if (isdigit(*p)) {
                            int num = atoi(p);
                            temp_rect.group_indices[temp_rect.group_count] = num;
                            temp_rect.group_count++;
                            // 跳过当前数字
                            while (*p && isdigit(*p)) p++;
                        } else {
                            p++;
                        }
                    }
                }
            }
            
            // 检测进入icons数组
            if (strstr(line_buffer, "\"icons\"") && strstr(line_buffer, "[")) {
                in_icons = true;
                current_icon = 0;
            }
            else if (in_icons && strstr(line_buffer, "]") && !strstr(line_buffer, "\"")) {
                in_icons = false;
            }
            else if (in_icons) {
                if (strstr(line_buffer, "\"icon_name\"")) {
                    sscanf(line_buffer, " \"icon_name\" : \"%31[^\"]\"", temp_icon_name);
                }
                else if (strstr(line_buffer, "\"rel_x\"")) {
                    float rel_x;
                    sscanf(line_buffer, " \"rel_x\" : %f", &rel_x);
                    if (current_icon < 4) {
                        temp_rect.icons[current_icon].rel_x = rel_x;
                    }
                }
                else if (strstr(line_buffer, "\"rel_y\"")) {
                    float rel_y;
                    sscanf(line_buffer, " \"rel_y\" : %f", &rel_y);
                    if (current_icon < 4) {
                        temp_rect.icons[current_icon].rel_y = rel_y;
                        temp_rect.icons[current_icon].icon_index = getIconIndexByName(temp_icon_name);
                        current_icon++;
                    }
                }
            }
            
            // 检测进入text_roll数组
            if (strstr(line_buffer, "\"text_roll\"") && strstr(line_buffer, "[")) {
                in_text_roll = true;
                current_text_roll = 0;
            }
            else if (in_text_roll && strstr(line_buffer, "]") && !strstr(line_buffer, "\"")) {
                in_text_roll = false;
            }
            else if (in_text_roll) {
                if (strstr(line_buffer, "\"text_arr\"")) {
                    if (current_text_roll < 4) {
                        sscanf(line_buffer, " \"text_arr\" : \"%31[^\"]\"", temp_rect.text_rolls[current_text_roll].text_arr);
                    }
                }
                else if (strstr(line_buffer, "\"idx\"")) {
                    if (current_text_roll < 4) {
                        sscanf(line_buffer, " \"idx\" : \"%15[^\"]\"", temp_rect.text_rolls[current_text_roll].idx);
                    }
                }
                else if (strstr(line_buffer, "\"font\"")) {
                    if (current_text_roll < 4) {
                        sscanf(line_buffer, " \"font\" : \"%31[^\"]\"", temp_rect.text_rolls[current_text_roll].font);
                    }
                }
                else if (strstr(line_buffer, "\"rel_x\"")) {
                    float rel_x;
                    sscanf(line_buffer, " \"rel_x\" : %f", &rel_x);
                    if (current_text_roll < 4) {
                        temp_rect.text_rolls[current_text_roll].rel_x = rel_x;
                    }
                }
                else if (strstr(line_buffer, "\"rel_y\"")) {
                    float rel_y;
                    sscanf(line_buffer, " \"rel_y\" : %f", &rel_y);
                    if (current_text_roll < 4) {
                        temp_rect.text_rolls[current_text_roll].rel_y = rel_y;
                    }
                }
                else if (strstr(line_buffer, "\"auto_roll\"")) {
                    if (current_text_roll < 4) {
                        temp_rect.text_rolls[current_text_roll].auto_roll = strstr(line_buffer, "true") != NULL;
                        current_text_roll++;
                        temp_rect.text_roll_count = current_text_roll;
                    }
                }
            }
            
            // 检测矩形对象结束（可能是 }, 或者 }）
            if (parsing_rect && strstr(line_buffer, "}")) {
                // 检查是否是矩形对象的结束括号（不是数组的结束）
                char* trimmed = line_buffer;
                while (*trimmed && isspace(*trimmed)) trimmed++;
                if (*trimmed == '}') {
                    rects[current_rect] = temp_rect;
                    ESP_LOGI("CACHE", "✅ 矩形[%d]解析完成", current_rect);
                    current_rect++;
                    parsing_rect = false;
                    in_icons = false;
                    in_text_roll = false;
                    temp_rect = {};  // 重置temp_rect
                    current_icon = 0;
                    current_text_roll = 0;
                }
            }
        }
    }
    
    ESP_LOGI("CACHE", "第二次扫描完成，共读取%d行", line_count);
    
    free(line_buffer);
    fclose(file);
    
    if (current_rect != rect_count) {
        ESP_LOGW("CACHE", "解析的矩形数量(%d)与声明的不一致(%d)", current_rect, rect_count);
        rect_count = current_rect;
    }
    
    *out_rects = rects;
    *out_rect_count = rect_count;
    *out_status_index = status_rect_index;
    
    ESP_LOGI("CACHE", "✅ 界面加载到内存成功: %d个矩形", rect_count);
    return true;
}

/**
 * @brief 扫描/spiffs目录下所有.json文件并预加载到缓存
 */
int preloadAllScreens() {
    ESP_LOGI("CACHE", "========== 开始预加载所有界面 ==========");
    
    // 清空缓存
    g_screen_cache_count = 0;
    memset(g_screen_cache, 0, sizeof(g_screen_cache));
    
    // 手动定义要加载的文件列表（因为ESP32的SPIFFS不支持目录遍历）
    const char* json_files[] = {
        "/spiffs/layout.json",
        "/spiffs/layout_1.json",
        "/spiffs/layout_clock.json",
    };
    int file_count = sizeof(json_files) / sizeof(json_files[0]);
    
    int loaded_count = 0;
    for (int i = 0; i < file_count && loaded_count < MAX_CACHED_SCREENS; i++) {
        const char* file_path = json_files[i];
        
        // 检查文件是否存在
        FILE* test = fopen(file_path, "r");
        if (!test) {
            ESP_LOGW("CACHE", "文件不存在: %s", file_path);
            continue;
        }
        fclose(test);
        
        // 加载到内存
        RectInfo* rects = nullptr;
        int rect_count = 0;
        int status_index = -1;
        
        if (loadScreenToMemory(file_path, &rects, &rect_count, &status_index)) {
            // 保存到缓存
            ScreenCache* cache = &g_screen_cache[loaded_count];
            strncpy(cache->file_path, file_path, sizeof(cache->file_path) - 1);
            
            // 从文件路径提取界面名称
            const char* name_start = strrchr(file_path, '/');
            if (name_start) {
                name_start++;  // 跳过 '/'
            } else {
                name_start = file_path;
            }
            const char* ext = strrchr(name_start, '.');
            int name_len = ext ? (ext - name_start) : strlen(name_start);
            if (name_len > 31) name_len = 31;
            strncpy(cache->screen_name, name_start, name_len);
            cache->screen_name[name_len] = '\0';
            
            cache->rects = rects;
            cache->rect_count = rect_count;
            cache->status_rect_index = status_index;
            cache->is_loaded = true;
            cache->last_access_time = millis();
            
            ESP_LOGI("CACHE", "✅ [%d] %s 加载成功 (%d个矩形)", 
                     loaded_count, cache->screen_name, rect_count);
            loaded_count++;
        }
        
        // 喂狗，防止看门狗超时
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    g_screen_cache_count = loaded_count;
    
    ESP_LOGI("CACHE", "========== 预加载完成！共加载 %d 个界面 ==========", loaded_count);
    ESP_LOGI("CACHE", "PSRAM使用情况:");
    ESP_LOGI("CACHE", "  ├─ PSRAM剩余: %d bytes (%.1f MB)", 
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024.0f / 1024.0f);
    ESP_LOGI("CACHE", "  └─ 总内存剩余: %d bytes (%.1f MB)", 
             heap_caps_get_free_size(MALLOC_CAP_8BIT),
             heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024.0f / 1024.0f);
    
    return loaded_count;
}

/**
 * @brief 根据索引切换到指定界面（从缓存中快速显示）
 */
bool switchToScreen(int screen_index) {
    if (screen_index < 0 || screen_index >= g_screen_cache_count) {
        ESP_LOGE("CACHE", "无效的界面索引: %d (总共%d个界面)", screen_index, g_screen_cache_count);
        return false;
    }
    
    ScreenCache* cache = &g_screen_cache[screen_index];
    if (!cache->is_loaded) {
        ESP_LOGE("CACHE", "界面[%d]未加载", screen_index);
        return false;
    }
    
    ESP_LOGI("CACHE", "切换到界面[%d]: %s", screen_index, cache->screen_name);
    
    // 更新访问时间
    cache->last_access_time = millis();
    g_current_screen_index = screen_index;
    
    // 保存布局数据供按键交互使用
    saveJsonLayoutForInteraction(cache->rects, cache->rect_count, cache->status_rect_index);
    
    // 如果是番茄钟界面，初始化番茄钟
    if (strstr(cache->screen_name, "layout_clock") != nullptr) {
        ESP_LOGI("POMODORO", "检测到番茄钟界面，初始化...");
        initPomodoro();
    }
    
    clearDisplayArea(0, 0, 416, 240);

    // 初始化焦点系统
    initFocusSystem(cache->rect_count);
    
    // 显示到墨水屏
    updateDisplayWithMain(cache->rects, cache->rect_count, cache->status_rect_index, 1);
    
    ESP_LOGI("CACHE", "✅ 界面切换完成！");
    return true;
}

/**
 * @brief 根据文件名切换到指定界面
 */
bool switchToScreenByPath(const char* file_path) {
    if (!file_path) {
        ESP_LOGE("CACHE", "文件路径为空");
        return false;
    }
    
    // 在缓存中查找
    for (int i = 0; i < g_screen_cache_count; i++) {
        if (strcmp(g_screen_cache[i].file_path, file_path) == 0) {
            return switchToScreen(i);
        }
    }
    
    ESP_LOGE("CACHE", "未找到界面: %s", file_path);
    return false;
}

/**
 * @brief 获取已缓存的界面数量
 */
int getCachedScreenCount() {
    return g_screen_cache_count;
}

/**
 * @brief 获取指定索引的界面名称
 */
const char* getScreenName(int screen_index) {
    if (screen_index < 0 || screen_index >= g_screen_cache_count) {
        return nullptr;
    }
    return g_screen_cache[screen_index].screen_name;
}

/**
 * @brief 释放所有界面缓存
 */
void freeAllScreenCache() {
    ESP_LOGI("CACHE", "释放所有界面缓存...");
    for (int i = 0; i < g_screen_cache_count; i++) {
        if (g_screen_cache[i].rects) {
            free(g_screen_cache[i].rects);
            g_screen_cache[i].rects = nullptr;
        }
        g_screen_cache[i].is_loaded = false;
    }
    g_screen_cache_count = 0;
    g_current_screen_index = -1;
    ESP_LOGI("CACHE", "✅ 缓存已清空");
}

/**
 * @brief 获取当前显示的界面索引
 */
int getCurrentScreenIndex() {
    return g_current_screen_index;
}
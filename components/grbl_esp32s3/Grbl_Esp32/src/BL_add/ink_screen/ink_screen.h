#pragma once
#include "../../Grbl.h"
#include <FS.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_370_GDEY037T03.h>

// 全局墨水屏显示对象（定义于 ink_screen.cpp）
extern GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display;
void ink_screen_init();
//全局变量记录选中的图标位置
// 修改IconPosition结构体，增加存储图标索引和数据的字段
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    bool selected;
    int icon_index;          // 存储图标在g_available_icons中的索引
    const uint8_t* data;    // 存储图标数据指针
} IconPosition;

// 图标信息结构体
typedef struct {
    const uint8_t* data;
    int width;
    int height;
} IconInfo;
// 图标配置结构体
typedef struct {
    int rect_index;        // 矩形索引
    int icon_index;        // 图标索引
    float rel_x;           // 相对x位置 (0.0-1.0)
    float rel_y;           // 相对y位置 (0.0-1.0)
} IconConfig;


typedef struct {
    uint16_t screenWidth;
    uint16_t screenHeigt;
}InkScreenSize;


//用到
/**
 * @brief 矩形内容类型枚举
 */
typedef enum {
    CONTENT_NONE = 0,        // 无内容（仅显示背景图标）
    CONTENT_STATUS_BAR,      // 状态栏（提示信息）
    CONTENT_ICON_ONLY,       // 仅显示图标
    CONTENT_WORD,            // 英文单词
    CONTENT_PHONETIC,        // 音标
    CONTENT_DEFINITION,      // 英文释义/例句
    CONTENT_TRANSLATION,     // 中文翻译
    CONTENT_SEPARATOR,       // 分隔线
    CONTENT_CUSTOM           // 自定义内容
} RectContentType;
//用到
/**
 * @brief 文本对齐方式枚举
 */
typedef enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT,
    ALIGN_TOP,
    ALIGN_MIDDLE,
    ALIGN_BOTTOM
} TextAlignment;

//用到
/*
 * @brief 矩形内图标位置定义
 */
typedef struct {
    float rel_x;    // 图标在矩形内的水平相对位置 (0.0-1.0, 0.5为中心)
    float rel_y;    // 图标在矩形内的垂直相对位置 (0.0-1.0, 0.5为中心)
    int icon_index; // 使用的图标索引
} IconPositionInRect;
//用到
/*
 * @brief 矩形内动态图标组位置定义 (icon_roll)
 */
typedef struct {
    float rel_x;           // 图标在矩形内的水平相对位置 (0.0-1.0, 0.5为中心)
    float rel_y;           // 图标在矩形内的垂直相对位置 (0.0-1.0, 0.5为中心)
    char icon_arr[32];     // 图标数组名称 (如: "cat_jump")
    char idx[16];          // 索引变量名称 (如: "$cat_jump_idx")
    bool auto_roll;        // 是否自动滚动 (true=自动100ms切换, false=固定)
} IconRollInRect;
//用到
/*
 * @brief 矩形内动态文本组位置定义 (text_roll)
 */
typedef struct {
    float rel_x;           // 文本在矩形内的水平相对位置 (0.0-1.0)
    float rel_y;           // 文本在矩形内的垂直相对位置 (0.0-1.0)
    char text_arr[32];     // 文本数组名称 (如: "message_remind")
    char idx[32];          // 索引变量名称 (如: "$message_idx") - 增大到32字符
    char font[32];         // 字体名称 (如: "chinese_translate_font", "english_sentence_font", "english_phonetic_font")
    bool auto_roll;        // 是否自动滚动 (true=自动100ms切换, false=固定)
} TextRollInRect;

// 文本数组注册表类型（用于 text_roll / 动态文本）
typedef struct {
    const char* name;      // 数组名称 (用于JSON中的"text_arr")
    const char* var_name;  // 变量名称 (用于JSON中的"idx")
    const char** sequence; // 文本序列
    int count;             // 序列长度
} TextArrayEntry;
//用到
/*
 * @brief 矩形内文本内容位置定义
 */
typedef struct {
    float rel_x;              // 文本在矩形内的水平相对位置 (0.0-1.0)
    float rel_y;              // 文本在矩形内的垂直相对位置 (0.0-1.0)
    RectContentType type;     // 内容类型（WORD/PHONETIC/DEFINITION/TRANSLATION）
    uint8_t font_size;        // 字体大小
    TextAlignment h_align;    // 水平对齐
    TextAlignment v_align;    // 垂直对齐
    int max_width;            // 最大宽度（0表示使用矩形剩余宽度）
    int max_height;           // 最大高度（0表示使用矩形剩余高度）
} TextPositionInRect;
//用到
/**
 * @brief 焦点显示模式枚举（放在 RectInfo 之前以便内嵌）
 */
typedef enum {
    FOCUS_MODE_DEFAULT = 0, // 默认（钉子图标）
    FOCUS_MODE_CORNERS = 1, // 四角小方块
    FOCUS_MODE_BORDER = 2   // 绘制边框
} FocusMode;
//用到
/**
 * @brief 框架结构体（只定义位置和内容类型，不含图标）
 */
typedef struct RectInfo {
    int x;          // X坐标（原始坐标，基准416x240）
    int y;          // Y坐标（原始坐标，基准416x240）
    int width;      // 宽度（原始尺寸）
    int height;     // 高度（原始尺寸）
    
    // ========== 内容配置 ==========
    RectContentType content_type; // 矩形内容类型
    uint8_t font_size;            // 字体大小（默认16）
    TextAlignment h_align;        // 水平对齐方式（默认居中）
    TextAlignment v_align;        // 垂直对齐方式（默认居中）
    int padding_left;             // 左内边距（像素）
    int padding_right;            // 右内边距（像素）
    int padding_top;              // 上内边距（像素）
    int padding_bottom;           // 下内边距（像素）
    int line_height;              // 行高（用于多行文本）
    
    // ========== 图标列表（动态填充） ==========
    IconPositionInRect icons[4];  // 每个矩形最多支持4个图标
    int icon_count;               // 实际图标数量
    
    // ========== 动态图标组列表（icon_roll） ==========
    IconRollInRect icon_rolls[4]; // 每个矩形最多支持4个动态图标组
    int icon_roll_count;          // 实际动态图标组数量
    
    // ========== 文本内容列表（动态填充） ==========
    TextPositionInRect texts[4];  // 每个矩形最多支持4个文本内容
    int text_count;               // 实际文本数量
    bool custom_text_mode;        // Web自定义文本模式，true则禁用默认回退
    
    // ========== 动态文本组列表（text_roll） ==========
    TextRollInRect text_rolls[4]; // 每个矩形最多支持4个动态文本组
    int text_roll_count;          // 实际动态文本组数量
    
    FocusMode focus_mode;         // 每个矩形独立的焦点显示模式
    int focus_icon_index;         // 焦点光标使用的图标索引 (-1表示使用默认样式)
    
    // ========== 子母数组相关 ==========
    char is_mother[8];            // "non"不可选中, "mom"母数组, "son"子数组
    int group_indices[8];         // 子数组索引列表（母数组专用）
    int group_count;              // 子数组数量
    
    // 每个矩形在确认按键时的回调（可为NULL）
    void (*onConfirm)(struct RectInfo* rect, int rect_index);
    // 绑定的动作ID（用于在Web/UI之间传递），以C字符串形式保存
    char on_confirm_action[32];
    
} RectInfo;

// 回调类型与动作注册项
typedef void (*OnConfirmFn)(struct RectInfo* rect, int rect_index);
typedef struct {
    const char* id;    // 动作ID（用于JSON/web端标识）
    const char* name;  // 显示名称
    OnConfirmFn fn;    // 函数指针
} ActionEntry;

// 在 ink_screen.cpp 中定义的动作注册表
extern ActionEntry g_action_registry[];
extern int g_action_registry_count;

// 根据动作ID查找对应的回调函数（若不存在返回NULL）
OnConfirmFn find_action_by_id(const char* id);

// Expose wordbook/pomodoro related symbols used by onconfirm callbacks
extern bool g_wordbook_text_initialized;
extern const int g_text_arrays_count;
extern const TextArrayEntry g_text_arrays[];
extern int g_text_animation_indices[];

// Wordbook helper accessors
const char* getWordBookWord(int index);
const char* getWordBookPhonetic(int index);
const char* getWordBookTranslation(int index);

// Pomodoro controls
void pomodoroStartPause();
void pomodoroReset();
void pomodoroSettings();
void setPomodoroDurationSeconds(int seconds);

/**
 * @brief 添加文本内容到矩形
 * @param rects 矩形数组
 * @param rect_index 目标矩形索引
 * @param content_type 内容类型（WORD/PHONETIC/DEFINITION/TRANSLATION）
 * @param rel_x 相对X位置（0.0-1.0）
 * @param rel_y 相对Y位置（0.0-1.0）
 * @param font_size 字体大小
 * @param h_align 水平对齐
 * @param v_align 垂直对齐
 * @return 是否成功添加
 */
bool addTextToRect(RectInfo* rects, int rect_index, RectContentType content_type,
                   float rel_x, float rel_y, uint8_t font_size,
                   TextAlignment h_align, TextAlignment v_align);

/**
 * @brief 清空矩形内所有文本
 */
void clearRectTexts(RectInfo* rect);

typedef struct {
    int icon_x;
    int icon_y;
    int icon_width;
    int icon_height;
    const uint8_t* icon_data;
    int text_x;
    int text_y;
    int text_width;
    int text_height;
    uint8_t font_size;
} PromptLayout;

// 界面类型枚举
typedef enum {
    SCREEN_HOME = 0,      // 主界面（现有界面）
    SCREEN_VOCABULARY,    // 单词本界面
    SCREEN_SLEEP,         // 休眠界面
    SCREEN_COUNT          // 界面总数
} ScreenType;

// 界面配置结构体
// 界面配置结构体
typedef struct {
    ScreenType type;
    const char* name;
    RectInfo* rects;      // 该界面的矩形数组
    int rect_count;       // 矩形数量
    int status_rect_index; // 状态栏矩形索引
    int show_border;      // 是否显示边框
    void (*init_func)(void);        // 初始化函数
    void (*setup_icons_func)(void); // 设置图标函数
    void (*update_func)(void);      // 更新函数
} ScreenConfig;

// 全局界面管理器
typedef struct {
    ScreenType current_screen;
    ScreenConfig screens[SCREEN_COUNT];
    bool screen_initialized[SCREEN_COUNT];
} ScreenManager;


#define MAX_GLOBAL_ICONS 20  // 增加到20个图标
#define MAX_MAIN_RECTS 20    // 主界面最大矩形数量
#define MAX_VOCAB_RECTS 20   // 单词界面最大矩形数量

void update_activity_time();
void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);
void updateDisplayWithMain(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
void getStatusIconPositions(RectInfo *rects, int rect_count, int status_rect_index,
                           int* wifi_x, int* wifi_y, 
                           int* battery_x, int* battery_y);
void displayMainScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
void displaySleepScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
void displayScreen(ScreenType screen_type);  // 统一屏幕显示函数
void initIconPositions();

// ======== 焦点系统函数声明 ========
#define MAX_FOCUSABLE_RECTS 20  // 最大可焦点矩形数量

void initFocusSystem(int total_rects);
void setFocusableRects(int* rect_indices, int count);  // 设置可焦点矩形列表（母数组）
void setSubArrayForParent(int parent_index, int* sub_indices, int sub_count);  // 为母数组元素设置子数组（parent_index是母数组中的位置）
void setSubArrayForRect(int rect_index, int* sub_indices, int sub_count);  // 为指定矩形索引设置子数组（自动转换）
bool enterSubArray();  // 进入子数组模式
void exitSubArray();   // 退出子数组，返回母数组
void moveFocusNext();
void moveFocusPrev();
int getCurrentFocusRect();
void handleFocusConfirm();
void drawFocusCursor(RectInfo *rects, int rect_count, int focus_index, float global_scale);
void startFocusTestMode();  // 焦点测试模式启动函数
int getIconIndexByName(const char* name);  // 通过图标名称获取索引
void getIconSizeByIndex(int icon_index, int* width, int* height);  // 通过图标索引获取尺寸
const uint8_t* getIconDataByIndex(int icon_index);  // 通过图标索引获取数据指针

extern uint8_t inkScreenTestFlag;
extern uint8_t inkScreenTestFlagTwo;
extern RectInfo rects[MAX_MAIN_RECTS];
extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
extern int rect_count;
extern int g_global_icon_count;
extern IconPosition g_icon_positions[MAX_GLOBAL_ICONS];
extern IconInfo g_available_icons[22];
extern InkScreenSize setInkScreenSize;

int getVocabScreenRectCount();
int getMainScreenRectCount();
//用到
void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);

// ==================== 简化的本地JSON加载和显示 ====================
/**
 * @brief 从JSON字符串解析布局并显示到墨水屏
 * @param json_str JSON字符串内容
 * @return true 成功, false 失败
 */
//用到
bool loadAndDisplayFromJSON(const char* json_str);

/**
 * @brief 从文件读取JSON并显示
 * @param file_path 文件路径
 * @return true 成功, false 失败
 */
//用到
bool loadAndDisplayFromFile(const char* file_path);
//用到
// ==================== JSON布局全局变量（extern声明） ====================
extern RectInfo* g_json_rects;
extern int g_json_rect_count;
extern int g_json_status_rect_index;

// ==================== JSON布局的按键交互 ====================
/**
 * @brief 按键：向下移动焦点并重绘
 */
//用到
void jsonLayoutFocusNext();

/**
 * @brief 按键：向上移动焦点并重绘
 */
//用到
void jsonLayoutFocusPrev();

/**
 * @brief 按键：确认当前焦点矩形（触发回调）
 */
//用到
void jsonLayoutConfirm();

/**
 * @brief icon_roll 相关函数
 */
//用到
int getVariableIndex(const char* var_name);
int getIconRollCurrentIndex(const IconRollInRect* icon_roll);
const char* getTextRollCurrentText(const TextRollInRect* text_roll);
void updateIconRollIndices();
void processAutoRollAnimations();

// ==================== 界面缓存管理系统 ====================
//用到
#define MAX_CACHED_SCREENS 10  // 最多缓存10个界面
//用到
/**
 * @brief 单个界面缓存结构体
 */
typedef struct {
    char file_path[64];         // JSON文件路径
    char screen_name[32];       // 界面名称（从文件名提取）
    RectInfo* rects;            // 矩形数组
    int rect_count;             // 矩形数量
    int status_rect_index;      // 状态栏索引
    bool is_loaded;             // 是否已加载
    uint32_t last_access_time;  // 最后访问时间（用于LRU）
} ScreenCache;
//用到
/**
 * @brief 从文件加载界面但不显示（仅解析到内存）
 * @param file_path 文件路径
 * @param out_rects 输出矩形数组指针
 * @param out_rect_count 输出矩形数量
 * @param out_status_index 输出状态栏索引
 * @return true 成功, false 失败
 */
bool loadScreenToMemory(const char* file_path, RectInfo** out_rects, 
                        int* out_rect_count, int* out_status_index);
//用到
/**
 * @brief 扫描/spiffs目录下所有.json文件并预加载到缓存
 * @return 成功加载的界面数量
 */
int preloadAllScreens();
//用到
/**
 * @brief 根据索引切换到指定界面（从缓存中快速显示）
 * @param screen_index 界面索引（0-9）
 * @return true 成功, false 失败
 */
bool switchToScreen(int screen_index);
//用到
/**
 * @brief 根据文件名切换到指定界面
 * @param file_path 文件路径（如 "/spiffs/layout.json"）
 * @return true 成功, false 失败
 */
bool switchToScreenByPath(const char* file_path);
//用到
/**
 * @brief 获取已缓存的界面数量
 */
int getCachedScreenCount();
//用到
/**
 * @brief 获取指定索引的界面名称
 * @param screen_index 界面索引
 * @return 界面名称，失败返回NULL
 */
const char* getScreenName(int screen_index);
//用到
/**
 * @brief 释放所有界面缓存
 */
void freeAllScreenCache();
//用到
/**
 * @brief 获取当前显示的界面索引
 * @return 当前界面索引，-1表示无界面
 */
int getCurrentScreenIndex();

// ==================== 提示信息缓存 API ====================
/**
 * @brief 初始化提示信息缓存
 */
void initPromptCache();

/**
 * @brief 添加新的提示信息到缓存（循环队列）
 * @param prompt 提示信息文本
 */
void addPromptToCache(const char* prompt);

/**
 * @brief 释放提示信息缓存
 */
void freePromptCache();

/**
 * @brief 获取提示信息总数
 */
int getPromptCount();

/**
 * @brief 获取最新的提示信息
 */
const char* getLatestPrompt();
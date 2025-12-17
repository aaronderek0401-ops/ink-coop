#pragma once
#include "../../Grbl.h"
#include <FS.h>
#include <GxEPD2_BW.h>

// 颜色常量定义 - 仅在未定义时定义
#ifndef BLACK
#define BLACK GxEPD_BLACK
#endif
#ifndef WHITE
#define WHITE GxEPD_WHITE
#endif

// 屏幕尺寸定义 - GxEPD2 屏幕是 240x416，而旧 EPD.h 定义的是 152x152
// 使用新的名称以避免冲突，或者检查是否是 GXEPD2 屏幕
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 416

// 为了兼容旧代码中使用 EPD_H 和 EPD_W 的地方，这里定义为新屏幕尺寸
#ifndef EPD_W
#define EPD_W SCREEN_WIDTH
#endif
#ifndef EPD_H
#define EPD_H SCREEN_HEIGHT
#endif

void ink_screen_init();
void clearEntireScreen();

// 定义图片显示区域结构体
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    bool displayed;  // 标记是否正在显示
} display_area_t;

// 图片显示区域数组
static display_area_t picture_areas[] = {
    {60, 40, 62, 64, false},   // ZHONGJINGYUAN_3_7_ICON_1
    {180, 40, 64, 64, false},  // ZHONGJINGYUAN_3_7_ICON_2
    {300, 40, 86, 64, false},  // ZHONGJINGYUAN_3_7_ICON_3
    {60, 140, 71, 56, false},  // ZHONGJINGYUAN_3_7_ICON_4
    {180, 140, 76, 56, false}, // ZHONGJINGYUAN_3_7_ICON_5
    {300, 140, 94, 64, false}  // ZHONGJINGYUAN_3_7_ICON_6
};

typedef struct {
    int grid_cols;
    int grid_rows;
    int icon_width;
    int icon_height;
    int icon_spacing_x;
    int icon_spacing_y;
    int status_bar_height;
} LayoutConfig;

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
// 全局变量记录上次下划线信息
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t color;
    bool has_underline;  // 是否有下划线
} LastUnderlineInfo;

typedef struct {
    uint16_t screenWidth;
    uint16_t screenHeigt;
}InkScreenSize;

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

/*
 * @brief 矩形内图标位置定义
 */
typedef struct {
    float rel_x;    // 图标在矩形内的水平相对位置 (0.0-1.0, 0.5为中心)
    float rel_y;    // 图标在矩形内的垂直相对位置 (0.0-1.0, 0.5为中心)
    int icon_index; // 使用的图标索引
} IconPositionInRect;

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

/**
 * @brief 框架结构体（只定义位置和内容类型，不含图标）
 */
typedef struct {
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
    
    // ========== 文本内容列表（动态填充） ==========
    TextPositionInRect texts[4];  // 每个矩形最多支持4个文本内容
    int text_count;               // 实际文本数量
    bool custom_text_mode;        // Web自定义文本模式，true则禁用默认回退
} RectInfo;

/**
 * @brief 图标放置辅助函数
 * @param rects 矩形数组
 * @param rect_index 目标矩形索引
 * @param icon_index 图标索引（0-20）
 * @param rel_x 相对X位置（0.0-1.0，0.5为中心）
 * @param rel_y 相对Y位置（0.0-1.0，0.5为中心）
 * @return 是否成功添加
 */
bool addIconToRect(RectInfo* rects, int rect_index, int icon_index, float rel_x, float rel_y);

/**
 * @brief 清空矩形内所有图标
 */
void clearRectIcons(RectInfo* rect);

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

/**
 * @brief 初始化主界面图标布局
 */
void initMainScreenIcons();

/**
 * @brief 初始化单词界面图标布局
 */
void initVocabScreenIcons();

/**
 * @brief 初始化单词界面文本布局
 */
void initVocabScreenTexts();

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

#define PICTURE_AREA_COUNT (sizeof(picture_areas) / sizeof(picture_areas[0]))
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
void displayVocabularyScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
void displaySleepScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
void displayScreen(ScreenType screen_type);  // 统一屏幕显示函数
void readAndPrintRandomWord();
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
void drawFocusCursor(RectInfo *rects, int focus_index, float global_scale);
void startFocusTestMode();  // 焦点测试模式启动函数

extern uint8_t inkScreenTestFlag;
extern uint8_t inkScreenTestFlagTwo;
extern uint8_t* showPrompt;
extern RectInfo rects[MAX_MAIN_RECTS];
extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
extern int rect_count;
extern int g_global_icon_count;
extern IconPosition g_icon_positions[MAX_GLOBAL_ICONS];
extern IconInfo g_available_icons[13];
extern InkScreenSize setInkScreenSize;
extern ScreenManager g_screen_manager;
extern bool g_focus_mode_enabled;
extern int g_current_focus_rect;
// 屏幕管理函数
void updateVocabScreenRectCount(int new_rect_count);
void updateMainScreenRectCount(int new_rect_count);
int getVocabScreenRectCount();
int getMainScreenRectCount();
void drawPictureScaled(uint16_t orig_x, uint16_t orig_y, 
                           uint16_t orig_w, uint16_t orig_h,
                           const uint8_t* BMP, uint16_t color) ;
void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);

// ===== 外部声明 =====
extern GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display;
extern char lastDisplayedText[256];
extern uint16_t lastTextLength;
extern uint16_t lastX;
extern uint16_t lastY;
extern uint16_t lastSize;

// ===== 图标文件名映射表 =====
// 与 g_available_icons 数组保持同步
// 每个文件对应 components/resource/icon/ 文件夹中的文件
extern const char *g_icon_filenames[13];

// ===== 函数声明 =====

void showSimplePromptWithNail(uint8_t *tempPrompt, int bg_x, int bg_y);
void showPromptInfor(uint8_t *tempPrompt, bool isAllRefresh);
bool isChineseText(const uint8_t* text);
void showChineseString(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color);
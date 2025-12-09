#pragma once
#include "../../Grbl.h"
#include <FS.h>
void ink_screen_init();
// CSV文件结构体
struct WordEntry {
  String word;
  String phonetic;
  String definition;
  String translation;
  String pos;
};
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

/*
 * @brief 矩形内图标位置定义
 */
typedef struct {
    float rel_x;    // 图标在矩形内的水平相对位置 (0.0-1.0)
    float rel_y;    // 图标在矩形内的垂直相对位置 (0.0-1.0)
    int icon_index; // 使用的图标索引
} IconPositionInRect;

/**
 * @brief 矩形信息结构体
 */
typedef struct {
    int x;          // X坐标（原始坐标）
    int y;          // Y坐标（原始坐标）
    int width;      // 宽度（原始尺寸）
    int height;     // 高度（原始尺寸）
    
    // 矩形内的图标定义
    IconPositionInRect icons[4];  // 每个矩形最多支持4个图标
    int icon_count;               // 实际图标数量
} RectInfo;

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

void parseCSVLine(String line, WordEntry &entry);
int countLines(File &file);
void printWordEntry(WordEntry &entry, int lineNumber);
void assignField(int fieldCount, String &field, WordEntry &entry);
void update_activity_time();
void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);
void updateDisplayWithMain(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
void getStatusIconPositions(RectInfo *rects, int rect_count, int status_rect_index,
                           int* wifi_x, int* wifi_y, 
                           int* battery_x, int* battery_y);
void displayMainScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
void readAndPrintRandomWord();
void initIconPositions();
extern uint8_t inkScreenTestFlag;
extern uint8_t inkScreenTestFlagTwo;
extern uint8_t* showPrompt;
extern RectInfo rects[7];
extern int rect_count;
extern int g_global_icon_count;
extern IconPosition g_icon_positions[MAX_GLOBAL_ICONS];
extern IconInfo g_available_icons[21];
extern InkScreenSize setInkScreenSize;
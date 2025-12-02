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

// 全局变量记录选中的图标位置
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    bool selected;
} IconPosition;

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

#define PICTURE_AREA_COUNT (sizeof(picture_areas) / sizeof(picture_areas[0]))

void parseCSVLine(String line, WordEntry &entry);
int countLines(File &file);
void printWordEntry(WordEntry &entry, int lineNumber);
void assignField(int fieldCount, String &field, WordEntry &entry);
void update_activity_time();
void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);
extern uint8_t inkScreenTestFlag;
extern uint8_t inkScreenTestFlagTwo;
extern uint8_t* showPrompt;
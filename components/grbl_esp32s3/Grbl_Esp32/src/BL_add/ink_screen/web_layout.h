#pragma once
// Web界面数据结构
typedef struct {
    int x;          // 屏幕显示坐标x
    int y;          // 屏幕显示坐标y
    int width;      // 宽度
    int height;     // 高度
    int icon_count; // 图标数量
    char icons[256]; // 图标位置信息JSON字符串
} WebRectInfo;

typedef struct {
    int x;          // 图标在屏幕上的x坐标
    int y;          // 图标在屏幕上的y坐标
    int width;      // 图标宽度
    int height;     // 图标高度
    int icon_index; // 图标索引
    int rect_index; // 所属矩形索引
    int global_index; // 全局图标索引
} WebIconInfo;

bool parseAndApplyLayout(const char* layout_json);
bool getCurrentLayoutInfo(char *output, int output_size);
void initLayoutFromConfig();
String getCurrentLayoutJson() ;
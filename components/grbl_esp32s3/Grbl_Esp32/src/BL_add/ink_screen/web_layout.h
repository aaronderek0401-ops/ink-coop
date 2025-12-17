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
bool parseAndApplyVocabLayout(const char* layout_json);
bool getCurrentLayoutInfo(char *output, int output_size);
bool getVocabLayoutInfo(char *output, int output_size);
void initLayoutFromConfig();
bool loadVocabLayoutFromConfig();
void saveVocabLayoutToConfig();
String getCurrentLayoutJson();

// 焦点矩形配置相关（添加界面类型参数以区分主界面和单词界面）
void saveFocusableRectsToConfig(int* rect_indices, int count, const char* screen_type = "vocab");
bool loadFocusableRectsFromConfig(const char* screen_type = "vocab");
// 子数组配置（添加界面类型参数）
void saveSubArrayConfigToSD(const char* json_data, const char* screen_type = "vocab");
bool loadSubArrayConfigFromSD(char* output, int output_size, const char* screen_type = "vocab");
bool loadAndApplySubArrayConfig(const char* screen_type = "vocab");  // 加载并应用子数组配置到焦点系统

// 导出可用图标列表（包括索引和来自components/resource/icon/文件夹的文件名）
char* exportAvailableIcons();


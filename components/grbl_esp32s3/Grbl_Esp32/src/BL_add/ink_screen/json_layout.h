#pragma once

#include "ink_screen.h"
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"

// ==================== JSON布局全局变量声明 ====================
extern RectInfo* g_json_rects;
extern int g_json_rect_count;
extern int g_json_status_rect_index;

// ==================== JSON布局函数声明 ====================

/**
 * @brief 从JSON字符串解析布局并显示到墨水屏
 * @param json_str JSON字符串内容
 * @return true 成功, false 失败
 */
bool loadAndDisplayFromJSON(const char* json_str);

/**
 * @brief 从文件读取JSON并显示
 * @param file_path 文件路径 (SD卡路径，如 "/layout_main.json")
 * @return true 成功, false 失败
 */
bool loadAndDisplayFromFile(const char* file_path);

/**
 * @brief 从SD卡文件读取JSON并解析到内存（不显示）
 * @param file_path 文件路径
 * @param out_rects 输出参数 - 解析后的矩形数组
 * @param out_rect_count 输出参数 - 矩形数量
 * @param out_status_index 输出参数 - 状态栏矩形索引
 * @return true 成功, false 失败
 */
bool loadScreenToMemory(const char* file_path, RectInfo** out_rects, 
                        int* out_rect_count, int* out_status_index);

/**
 * @brief 保存JSON布局数据供按键交互使用
 * @param rects 矩形数组
 * @param rect_count 矩形数量
 * @param status_rect_index 状态栏矩形索引
 */
void saveJsonLayoutForInteraction(RectInfo* rects, int rect_count, int status_rect_index);

/**
 * @brief 重绘当前JSON布局（用于焦点变化后刷新显示）
 */
void redrawJsonLayout();

/**
 * @brief 按键：向下移动焦点（用于JSON布局）
 */
void jsonLayoutFocusNext();

/**
 * @brief 按键：向上移动焦点（用于JSON布局）
 */
void jsonLayoutFocusPrev();

/**
 * @brief 按键：确认当前焦点矩形（触发回调并处理子母数组切换）
 */
void jsonLayoutConfirm();

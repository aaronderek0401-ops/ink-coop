#include "json_layout.h"
#include "ink_screen.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static const char *TAG = "json_layout.cpp";

// ==================== 常量定义 ====================
#define MAX_RECTS 50
#define MAX_ICONS_PER_RECT 4

// ==================== 外部变量和函数声明 ====================
// 这些变量和函数在 ink_screen.cpp 中定义
extern InkScreenSize setInkScreenSize;
extern bool g_in_sub_array;

// 这些函数在 ink_screen.cpp 中实现
extern int getIconIndexByName(const char* name);
extern OnConfirmFn find_action_by_id(const char* action_id);
extern void initPomodoro();
extern void initFocusSystem(int total_rects);
extern void moveFocusNext();
extern void moveFocusPrev();
extern int getCurrentFocusRect();
extern bool enterSubArray();
extern void exitSubArray();
extern void updateDisplayWithMain(RectInfo *rects, int rect_count, int status_rect_index, int show_border);
extern void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);

// ==================== JSON布局全局变量定义 ====================
RectInfo* g_json_rects = nullptr;
int g_json_rect_count = 0;
int g_json_status_rect_index = -1;

// ==================== JSON布局函数实现 ====================

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
            redrawJsonLayout();
        }
    }
}

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

                if (icon_arr && cJSON_IsString(icon_arr) && idx && cJSON_IsString(idx)) {
                    
                    IconRollInRect* icon_roll = &rect->icon_rolls[icon_roll_count];
                    
                    // 复制字符串，确保不超出缓冲区
                    strncpy(icon_roll->icon_arr, icon_arr->valuestring, sizeof(icon_roll->icon_arr) - 1);
                    icon_roll->icon_arr[sizeof(icon_roll->icon_arr) - 1] = '\0';
                    
                    strncpy(icon_roll->idx, idx->valuestring, sizeof(icon_roll->idx) - 1);
                    icon_roll->idx[sizeof(icon_roll->idx) - 1] = '\0';
                    
                    // 解析rel_x（支持单个值或数组）
                    icon_roll->path_count = 0;
                    if (rel_x) {
                        if (cJSON_IsArray(rel_x)) {
                            int arr_size = cJSON_GetArraySize(rel_x);
                            for (int k = 0; k < arr_size && k < 8; k++) {
                                cJSON* x_item = cJSON_GetArrayItem(rel_x, k);
                                if (x_item && cJSON_IsNumber(x_item)) {
                                    icon_roll->rel_x[k] = (float)x_item->valuedouble;
                                    icon_roll->path_count = k + 1;
                                }
                            }
                        } else if (cJSON_IsNumber(rel_x)) {
                            icon_roll->rel_x[0] = (float)rel_x->valuedouble;
                            icon_roll->path_count = 1;
                        }
                    }
                    
                    // 解析rel_y（支持单个值或数组）
                    if (rel_y) {
                        if (cJSON_IsArray(rel_y)) {
                            int arr_size = cJSON_GetArraySize(rel_y);
                            for (int k = 0; k < arr_size && k < 8; k++) {
                                cJSON* y_item = cJSON_GetArrayItem(rel_y, k);
                                if (y_item && cJSON_IsNumber(y_item)) {
                                    icon_roll->rel_y[k] = (float)y_item->valuedouble;
                                }
                            }
                        } else if (cJSON_IsNumber(rel_y)) {
                            icon_roll->rel_y[0] = (float)rel_y->valuedouble;
                        }
                    }
                    
                    // 解析auto_roll字段，默认为false
                    icon_roll->auto_roll = false;
                    if (auto_roll && cJSON_IsBool(auto_roll)) {
                        icon_roll->auto_roll = cJSON_IsTrue(auto_roll);
                    }
                    
                    ESP_LOGI("JSON", "解析动态图标组%d: arr=%s, idx=%s, path_count=%d, auto_roll=%s", 
                            icon_roll_count, icon_roll->icon_arr, icon_roll->idx, 
                            icon_roll->path_count, icon_roll->auto_roll ? "true" : "false");
                    
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
                    
                    // 解析offset字段，默认为0
                    cJSON* offset_obj = cJSON_GetObjectItem(text_roll_item, "offset");
                    text_roll->offset = 0;
                    if (offset_obj && cJSON_IsNumber(offset_obj)) {
                        text_roll->offset = offset_obj->valueint;
                        ESP_LOGI("JSON_DEBUG", "✅ 读取到offset字段: %d", text_roll->offset);
                    } else {
                        ESP_LOGI("JSON_DEBUG", "⚠️  未找到offset字段或非数字，使用默认值0 (offset_obj=%p)", offset_obj);
                    }
                    
                    // 解析auto_roll字段，默认为false
                    text_roll->auto_roll = false;
                    if (auto_roll && cJSON_IsBool(auto_roll)) {
                        text_roll->auto_roll = cJSON_IsTrue(auto_roll);
                    }
                    
                    ESP_LOGI("JSON", "解析动态文本组%d: arr=%s, idx=%s, offset=%d, font=%s, pos=(%.2f,%.2f), auto_roll=%s", 
                            text_roll_count, text_roll->text_arr, text_roll->idx, text_roll->offset,
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
// 注意：这些函数已经在前面定义过了，这里是重复的，已删除

// 下面的代码块已经移除，因为函数在第43-100行已经定义过了

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
    bool in_icon_roll = false;  // 新增：标记是否在icon_roll数组中
    bool in_text_roll = false;
    bool in_group_array = false;  // 新增：标记是否在Group数组中
    int current_icon = 0;
    int current_icon_roll = 0;  // 新增：当前icon_roll索引
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
                // 检查是否是空数组 "icons": []
                if (!strstr(line_buffer, "[]")) {
                    in_icons = true;
                    current_icon = 0;
                }
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
            
            // 检测进入icon_roll数组
            if (strstr(line_buffer, "\"icon_roll\"") && strstr(line_buffer, "[")) {
                ESP_LOGI("CACHE", ">>> 检测到icon_roll行: %s", line_buffer);
                // 检查是否是空数组 "icon_roll": []
                if (!strstr(line_buffer, "[]")) {
                    ESP_LOGI("CACHE", ">>> 进入icon_roll解析状态");
                    in_icon_roll = true;
                    current_icon_roll = 0;
                } else {
                    ESP_LOGI("CACHE", ">>> icon_roll是空数组，跳过");
                }
            }
            else if (in_icon_roll && strstr(line_buffer, "]") && !strstr(line_buffer, "\"")) {
                ESP_LOGI("CACHE", ">>> 退出icon_roll解析状态");
                in_icon_roll = false;
            }
            else if (in_icon_roll) {
                if (strstr(line_buffer, "\"icon_arr\"")) {
                    if (current_icon_roll < 4) {
                        sscanf(line_buffer, " \"icon_arr\" : \"%31[^\"]\"", temp_rect.icon_rolls[current_icon_roll].icon_arr);
                        ESP_LOGI("CACHE", "  解析icon_arr='%s'", temp_rect.icon_rolls[current_icon_roll].icon_arr);
                    }
                }
                else if (strstr(line_buffer, "\"idx\"")) {
                    if (current_icon_roll < 4) {
                        sscanf(line_buffer, " \"idx\" : \"%31[^\"]\"", temp_rect.icon_rolls[current_icon_roll].idx);
                        ESP_LOGI("CACHE", "  解析idx='%s'", temp_rect.icon_rolls[current_icon_roll].idx);
                    }
                }
                else if (strstr(line_buffer, "\"rel_x\"")) {
                    if (current_icon_roll < 4) {
                        // 检查是数组还是单个值
                        if (strstr(line_buffer, "[")) {
                            // 数组格式：解析多个值
                            char* bracket_start = strchr(line_buffer, '[');
                            char* bracket_end = strchr(line_buffer, ']');
                            if (bracket_start && bracket_end) {
                                char values_str[128] = {0};
                                int len = bracket_end - bracket_start - 1;
                                if (len > 0 && len < 127) {
                                    strncpy(values_str, bracket_start + 1, len);
                                    int path_idx = 0;
                                    char* token = strtok(values_str, ", ");
                                    while (token && path_idx < 8) {
                                        temp_rect.icon_rolls[current_icon_roll].rel_x[path_idx] = atof(token);
                                        path_idx++;
                                        token = strtok(NULL, ", ");
                                    }
                                    temp_rect.icon_rolls[current_icon_roll].path_count = path_idx;
                                }
                            }
                        } else {
                            // 单个值格式
                            float rel_x;
                            sscanf(line_buffer, " \"rel_x\" : %f", &rel_x);
                            temp_rect.icon_rolls[current_icon_roll].rel_x[0] = rel_x;
                            temp_rect.icon_rolls[current_icon_roll].path_count = 1;
                        }
                    }
                }
                else if (strstr(line_buffer, "\"rel_y\"")) {
                    if (current_icon_roll < 4) {
                        // 检查是数组还是单个值
                        if (strstr(line_buffer, "[")) {
                            // 数组格式：解析多个值
                            char* bracket_start = strchr(line_buffer, '[');
                            char* bracket_end = strchr(line_buffer, ']');
                            if (bracket_start && bracket_end) {
                                char values_str[128] = {0};
                                int len = bracket_end - bracket_start - 1;
                                if (len > 0 && len < 127) {
                                    strncpy(values_str, bracket_start + 1, len);
                                    int path_idx = 0;
                                    char* token = strtok(values_str, ", ");
                                    while (token && path_idx < 8) {
                                        temp_rect.icon_rolls[current_icon_roll].rel_y[path_idx] = atof(token);
                                        path_idx++;
                                        token = strtok(NULL, ", ");
                                    }
                                }
                            }
                        } else {
                            // 单个值格式
                            float rel_y;
                            sscanf(line_buffer, " \"rel_y\" : %f", &rel_y);
                            temp_rect.icon_rolls[current_icon_roll].rel_y[0] = rel_y;
                        }
                    }
                }
                else if (strstr(line_buffer, "\"auto_roll\"")) {
                    if (current_icon_roll < 4) {
                        temp_rect.icon_rolls[current_icon_roll].auto_roll = strstr(line_buffer, "true") != NULL;
                        current_icon_roll++;
                        temp_rect.icon_roll_count = current_icon_roll;
                        ESP_LOGI("CACHE", "矩形%d icon_roll%d 解析完成，auto_roll=%d, path_count=%d, arr='%s', idx='%s'", 
                                 current_rect, current_icon_roll-1, 
                                 temp_rect.icon_rolls[current_icon_roll-1].auto_roll,
                                 temp_rect.icon_rolls[current_icon_roll-1].path_count,
                                 temp_rect.icon_rolls[current_icon_roll-1].icon_arr,
                                 temp_rect.icon_rolls[current_icon_roll-1].idx);
                    }
                }
            }
            
            // 检测进入text_roll数组
            if (strstr(line_buffer, "\"text_roll\"") && strstr(line_buffer, "[")) {
                // 检查是否是空数组 "text_roll": []
                if (!strstr(line_buffer, "[]")) {
                    in_text_roll = true;
                    current_text_roll = 0;
                }
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
                        sscanf(line_buffer, " \"idx\" : \"%31[^\"]\"", temp_rect.text_rolls[current_text_roll].idx);
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
                else if (strstr(line_buffer, "\"offset\"")) {
                    int offset_val = 0;
                    sscanf(line_buffer, " \"offset\" : %d", &offset_val);
                    if (current_text_roll < 4) {
                        temp_rect.text_rolls[current_text_roll].offset = offset_val;
                        ESP_LOGI("CACHE", "矩形%d text_roll%d offset=%d", current_rect, current_text_roll, offset_val);
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
            // 注意：只有在不处于 icons/icon_roll/text_roll 解析状态时才检测矩形结束
            if (parsing_rect && !in_icons && !in_icon_roll && !in_text_roll && strstr(line_buffer, "}")) {
                // 检查是否是矩形对象的结束括号（不是数组的结束）
                char* trimmed = line_buffer;
                while (*trimmed && isspace(*trimmed)) trimmed++;
                if (*trimmed == '}') {
                    rects[current_rect] = temp_rect;
                    ESP_LOGI("CACHE", "✅ 矩形[%d]解析完成", current_rect);
                    current_rect++;
                    parsing_rect = false;
                    in_icons = false;
                    in_icon_roll = false;
                    in_text_roll = false;
                    temp_rect = {};  // 重置temp_rect
                    current_icon = 0;
                    current_icon_roll = 0;
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
        "/spiffs/layout_main.json",
        "/spiffs/layout_wordbook.json",
        "/spiffs/layout_clock.json",
        "/spiffs/layout_clock_set.json",
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
    
    // clearDisplayArea(0, 0, 416, 240);

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
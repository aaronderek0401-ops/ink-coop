
#include "ink_screen.h"
#include "web_layout.h"

extern "C" {
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"
}
#define TAG "web_layout.cpp"
// 保存布局到配置文件
void saveLayoutToConfig() {
        SDState state = get_sd_state(true);
	if (state != SDState::Idle) {
		if (state == SDState::NotPresent) {
			ESP_LOGI(TAG,"No SD Card\r\n");
		} else {
			ESP_LOGI(TAG,"SD Card Busy\r\n");
		}
	}
// 保存到SD卡
String config_filename = "/layout_config.json";
// 先检查文件是否存在，如果存在则删除
if (SD.exists(config_filename.c_str())) {
    SD.remove(config_filename.c_str());
    ESP_LOGI(TAG, "删除旧的配置文件: %s", config_filename.c_str());
}
cJSON *root = cJSON_CreateObject();
if (!root) return;

// 保存矩形数量
cJSON_AddNumberToObject(root, "rect_count", rect_count);

// 保存矩形数组
cJSON *rects_array = cJSON_CreateArray();
if (rects_array) {
    for (int i = 0; i < rect_count; i++) {
        cJSON *rect_obj = cJSON_CreateObject();
        if (rect_obj) {
            RectInfo *rect = &rects[i];
            
            cJSON_AddNumberToObject(rect_obj, "x", rect->x);
            cJSON_AddNumberToObject(rect_obj, "y", rect->y);
            cJSON_AddNumberToObject(rect_obj, "width", rect->width);
            cJSON_AddNumberToObject(rect_obj, "height", rect->height);
            cJSON_AddNumberToObject(rect_obj, "icon_count", rect->icon_count);
            
            // 保存图标信息
            cJSON *icons_array = cJSON_CreateArray();
            if (icons_array) {
                for (int j = 0; j < rect->icon_count; j++) {
                    cJSON *icon_obj = cJSON_CreateObject();
                    if (icon_obj) {
                        IconPositionInRect *icon = &rect->icons[j];
                        cJSON_AddNumberToObject(icon_obj, "icon_index", icon->icon_index);
                        cJSON_AddNumberToObject(icon_obj, "rel_x", icon->rel_x);
                        cJSON_AddNumberToObject(icon_obj, "rel_y", icon->rel_y);
                        cJSON_AddItemToArray(icons_array, icon_obj);
                    }
                }
                cJSON_AddItemToObject(rect_obj, "icons", icons_array);
            }
            
            cJSON_AddItemToArray(rects_array, rect_obj);
        }
    }
    cJSON_AddItemToObject(root, "rectangles", rects_array);
}

char *json_str = cJSON_PrintUnformatted(root);
if (json_str) {
    File config_file = SD.open(config_filename.c_str(), FILE_WRITE);
    if (config_file) {
        config_file.write((const uint8_t *)json_str, strlen(json_str));
        config_file.close();
        ESP_LOGI(TAG, "Layout config saved to %s", config_filename.c_str());
    }
    free(json_str);
}

cJSON_Delete(root);
}
// 从配置文件加载布局
bool loadLayoutFromConfig() {
    String config_filename = "/layout_config.json";

    if (!SD.exists(config_filename)) {
        ESP_LOGI(TAG, "No layout config found, using default");
        return false;
    }

    File config_file = SD.open(config_filename.c_str(), FILE_READ);
    if (!config_file) {
        ESP_LOGI(TAG, "Failed to open layout config file");
        return false;
    }

    size_t file_size = config_file.size();
    if (file_size > 4096) {
        config_file.close();
        ESP_LOGI(TAG, "Layout config file too large");
        return false;
    }

    char *json_str = (char *)malloc(file_size + 1);
    if (!json_str) {
        config_file.close();
        ESP_LOGI(TAG, "Failed to allocate memory for layout config");
        return false;
    }

    size_t bytes_read = config_file.read((uint8_t *)json_str, file_size);
    config_file.close();
    json_str[bytes_read] = '\0';

    // 解析JSON
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        ESP_LOGI(TAG, "Failed to parse layout config JSON");
        return false;
    }

    // 解析矩形数量
    cJSON *rect_count_json = cJSON_GetObjectItem(root, "rect_count");
    if (rect_count_json) {
        int new_rect_count = rect_count_json->valueint;
        if (new_rect_count > 7) new_rect_count = 7;
        
        // 解析矩形数组
        cJSON *rects_array = cJSON_GetObjectItem(root, "rectangles");
        if (rects_array && cJSON_IsArray(rects_array)) {
            int array_size = cJSON_GetArraySize(rects_array);
            int rects_to_load = (array_size < new_rect_count) ? array_size : new_rect_count;
            
            for (int i = 0; i < rects_to_load; i++) {
                cJSON *rect_obj = cJSON_GetArrayItem(rects_array, i);
                if (rect_obj) {
                    cJSON *x = cJSON_GetObjectItem(rect_obj, "x");
                    cJSON *y = cJSON_GetObjectItem(rect_obj, "y");
                    cJSON *width = cJSON_GetObjectItem(rect_obj, "width");
                    cJSON *height = cJSON_GetObjectItem(rect_obj, "height");
                    cJSON *icon_count = cJSON_GetObjectItem(rect_obj, "icon_count");
                    
                    if (x && y && width && height && icon_count) {
                        rects[i].x = x->valueint;
                        rects[i].y = y->valueint;
                        rects[i].width = width->valueint;
                        rects[i].height = height->valueint;
                        rects[i].icon_count = 0;
                        
                        // 解析图标信息
                        cJSON *icons_array = cJSON_GetObjectItem(rect_obj, "icons");
                        if (icons_array && cJSON_IsArray(icons_array)) {
                            int icon_array_size = cJSON_GetArraySize(icons_array);
                            int icons_to_add = (icon_array_size < 4) ? icon_array_size : 4;
                            
                            for (int j = 0; j < icons_to_add; j++) {
                                cJSON *icon_obj = cJSON_GetArrayItem(icons_array, j);
                                if (icon_obj) {
                                    cJSON *icon_index = cJSON_GetObjectItem(icon_obj, "icon_index");
                                    cJSON *rel_x = cJSON_GetObjectItem(icon_obj, "rel_x");
                                    cJSON *rel_y = cJSON_GetObjectItem(icon_obj, "rel_y");
                                    
                                    if (icon_index && rel_x && rel_y) {
                                        rects[i].icons[j].icon_index = icon_index->valueint;
                                        rects[i].icons[j].rel_x = rel_x->valuedouble;
                                        rects[i].icons[j].rel_y = rel_y->valuedouble;
                                        rects[i].icon_count++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            ESP_LOGI(TAG, "Layout config loaded successfully: %d rectangles", rects_to_load);
            cJSON_Delete(root);
            return true;
        }
    }

    cJSON_Delete(root);
    return false;
}
// 初始化布局管理器
void initLayoutManager() {

}
// 初始化时加载布局
void initLayoutFromConfig() {
    if (loadLayoutFromConfig()) {
        ESP_LOGI(TAG, "Layout loaded from config file");
    } else {
        // 使用默认布局
        ESP_LOGI(TAG, "Using default layout");
        // 确保rect_count已正确设置
        rect_count = sizeof(rects) / sizeof(rects[0]);
    }
}


/**
 * @brief 获取主界面布局信息，用于Web界面显示
 * @param rects 矩形数组
 * @param rect_count 矩形数量
 * @param status_rect_index 状态栏矩形索引
 * @param output 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @return 成功返回true，失败返回false
 */
bool getMainLayoutInfo(RectInfo *rects, int rect_count, int status_rect_index, 
                       char *output, int output_size) {
    if (!rects || !output) {
        ESP_LOGE("WEB_LAYOUT", "rects or output is null");
        return false;
    }
    
    if (output_size < 1024) {  // 增加到 1024 或更大
        ESP_LOGE("WEB_LAYOUT", "output_size too small: %d, need at least 1024", output_size);
        return false;
    }
    
    // 检查 rect_count 是否合理
    if (rect_count <= 0 || rect_count > 100) {
        ESP_LOGE("WEB_LAYOUT", "invalid rect_count: %d", rect_count);
        return false;
    }
    
    ESP_LOGI("WEB_LAYOUT", "开始生成布局信息，rect_count=%d", rect_count);
    
    // 计算缩放比例
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    ESP_LOGI("WEB_LAYOUT", "屏幕尺寸: %dx%d, 缩放比例: %f", 
             setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt, global_scale);
    
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE("WEB_LAYOUT", "创建JSON根对象失败");
        return false;
    }
    
    // 添加屏幕信息
    cJSON_AddNumberToObject(root, "screen_width", setInkScreenSize.screenWidth);
    cJSON_AddNumberToObject(root, "screen_height", setInkScreenSize.screenHeigt);
    cJSON_AddNumberToObject(root, "scale", global_scale);
    cJSON_AddNumberToObject(root, "original_width", 416);
    cJSON_AddNumberToObject(root, "original_height", 240);
    
    // 创建矩形数组
    cJSON *rects_array = cJSON_CreateArray();
    if (!rects_array) {
        ESP_LOGE("WEB_LAYOUT", "创建rects_array失败");
        cJSON_Delete(root);
        return false;
    }
    
    ESP_LOGI("WEB_LAYOUT", "开始处理矩形，数量: %d", rect_count);
    
    for (int i = 0; i < rect_count; i++) {
        cJSON *rect_obj = cJSON_CreateObject();
        if (!rect_obj) {
            ESP_LOGE("WEB_LAYOUT", "创建rect_obj[%d]失败", i);
            continue;
        }
        
        RectInfo *rect = &rects[i];
        
        // 计算显示坐标
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        int display_width = (int)(rect->width * global_scale + 0.5f);
        int display_height = (int)(rect->height * global_scale + 0.5f);
        
        // 边界检查
        if (display_x < 0) display_x = 0;
        if (display_y < 0) display_y = 0;
        if (display_x + display_width > setInkScreenSize.screenWidth) {
            display_width = setInkScreenSize.screenWidth - display_x;
        }
        if (display_y + display_height > setInkScreenSize.screenHeigt) {
            display_height = setInkScreenSize.screenHeigt - display_y;
        }
        
        // 添加矩形信息
        cJSON_AddNumberToObject(rect_obj, "index", i);
        cJSON_AddNumberToObject(rect_obj, "x", display_x);
        cJSON_AddNumberToObject(rect_obj, "y", display_y);
        cJSON_AddNumberToObject(rect_obj, "width", display_width);
        cJSON_AddNumberToObject(rect_obj, "height", display_height);
        cJSON_AddNumberToObject(rect_obj, "original_x", rect->x);
        cJSON_AddNumberToObject(rect_obj, "original_y", rect->y);
        cJSON_AddNumberToObject(rect_obj, "original_width", rect->width);
        cJSON_AddNumberToObject(rect_obj, "original_height", rect->height);
        cJSON_AddNumberToObject(rect_obj, "icon_count", rect->icon_count);
        cJSON_AddBoolToObject(rect_obj, "is_status_bar", (i == status_rect_index));
        
        // 添加矩形内的图标信息
        cJSON *icons_array = cJSON_CreateArray();
        if (icons_array) {
            int valid_icon_count = 0;
            for (int j = 0; j < rect->icon_count; j++) {
                IconPositionInRect *icon_pos = &rect->icons[j];
                
                // 获取图标尺寸
                if (icon_pos->icon_index >= 0 && 
                    icon_pos->icon_index < (int)(sizeof(g_available_icons) / sizeof(IconInfo))) {
                    IconInfo *icon = &g_available_icons[icon_pos->icon_index];
                    
                    // 计算图标在矩形内的原始位置
                    int icon_orig_x = rect->x + (int)((rect->width - icon->width) * icon_pos->rel_x);
                    int icon_orig_y = rect->y + (int)((rect->height - icon->height) * icon_pos->rel_y);
                    
                    // 计算显示位置
                    int icon_display_x = (int)(icon_orig_x * global_scale + 0.5f);
                    int icon_display_y = (int)(icon_orig_y * global_scale + 0.5f);
                    int icon_display_width = (int)(icon->width * global_scale + 0.5f);
                    int icon_display_height = (int)(icon->height * global_scale + 0.5f);
                    
                    cJSON *icon_obj = cJSON_CreateObject();
                    if (icon_obj) {
                        // 只保留一位小数
                        double rel_x_rounded = round(icon_pos->rel_x * 10.0) / 10.0;
                        double rel_y_rounded = round(icon_pos->rel_y * 10.0) / 10.0;
                        
                        // 确保在0-1范围内
                        if (rel_x_rounded < 0.0f) rel_x_rounded = 0.0f;
                        if (rel_x_rounded > 1.0f) rel_x_rounded = 1.0f;
                        if (rel_y_rounded < 0.0f) rel_y_rounded = 0.0f;
                        if (rel_y_rounded > 1.0f) rel_y_rounded = 1.0f;
                        cJSON_AddNumberToObject(icon_obj, "rect_index", i);
                        cJSON_AddNumberToObject(icon_obj, "icon_index", icon_pos->icon_index);
                        
                        cJSON_AddNumberToObject(icon_obj, "rel_x", rel_x_rounded);
                        cJSON_AddNumberToObject(icon_obj, "rel_y", rel_y_rounded);
                        
                        cJSON_AddNumberToObject(icon_obj, "original_x", icon_orig_x);
                        cJSON_AddNumberToObject(icon_obj, "original_y", icon_orig_y);
                        cJSON_AddNumberToObject(icon_obj, "original_width", icon->width);
                        cJSON_AddNumberToObject(icon_obj, "original_height", icon->height);
                        cJSON_AddNumberToObject(icon_obj, "display_x", icon_display_x);
                        cJSON_AddNumberToObject(icon_obj, "display_y", icon_display_y);
                        cJSON_AddNumberToObject(icon_obj, "display_width", icon_display_width);
                        cJSON_AddNumberToObject(icon_obj, "display_height", icon_display_height);
                        
                        // 简化图标名称
                        char icon_name[16];
                        snprintf(icon_name, sizeof(icon_name), "icon_%d", icon_pos->icon_index);
                        cJSON_AddStringToObject(icon_obj, "icon_name", icon_name);
                        
                        cJSON_AddItemToArray(icons_array, icon_obj);
                        valid_icon_count++;
                    }
                }
            }
            cJSON_AddItemToObject(rect_obj, "icons", icons_array);
            ESP_LOGD("WEB_LAYOUT", "矩形[%d]有 %d 个有效图标", i, valid_icon_count);
        }
        
        cJSON_AddItemToArray(rects_array, rect_obj);
    }
    
    cJSON_AddItemToObject(root, "rectangles", rects_array);
    ESP_LOGI("WEB_LAYOUT", "矩形处理完成");
    
    // 添加当前显示的图标信息（如果定义了 g_global_icon_count）
    #ifdef g_global_icon_count
    cJSON *displayed_icons = cJSON_CreateArray();
    if (displayed_icons && g_global_icon_count > 0) {
        int valid_displayed_icons = 0;
        for (int i = 0; i < g_global_icon_count; i++) {
            if (g_icon_positions[i].width > 0 && g_icon_positions[i].height > 0) {
                cJSON *icon_obj = cJSON_CreateObject();
                if (icon_obj) {
                    // 计算显示坐标
                    int display_x = (int)(g_icon_positions[i].x * global_scale);
                    int display_y = (int)(g_icon_positions[i].y * global_scale);
                    int display_width = (int)(g_icon_positions[i].width * global_scale);
                    int display_height = (int)(g_icon_positions[i].height * global_scale);
                    
                    cJSON_AddNumberToObject(icon_obj, "global_index", i);
                    cJSON_AddNumberToObject(icon_obj, "icon_index", g_icon_positions[i].icon_index);
                    cJSON_AddNumberToObject(icon_obj, "original_x", g_icon_positions[i].x);
                    cJSON_AddNumberToObject(icon_obj, "original_y", g_icon_positions[i].y);
                    cJSON_AddNumberToObject(icon_obj, "original_width", g_icon_positions[i].width);
                    cJSON_AddNumberToObject(icon_obj, "original_height", g_icon_positions[i].height);
                    cJSON_AddNumberToObject(icon_obj, "display_x", display_x);
                    cJSON_AddNumberToObject(icon_obj, "display_y", display_y);
                    cJSON_AddNumberToObject(icon_obj, "display_width", display_width);
                    cJSON_AddNumberToObject(icon_obj, "display_height", display_height);
                    cJSON_AddBoolToObject(icon_obj, "selected", g_icon_positions[i].selected);
                    
                    cJSON_AddItemToArray(displayed_icons, icon_obj);
                    valid_displayed_icons++;
                }
            }
        }
        cJSON_AddItemToObject(root, "displayed_icons", displayed_icons);
        ESP_LOGD("WEB_LAYOUT", "显示图标: %d 个", valid_displayed_icons);
    }
    #endif
    
    // 添加状态栏图标位置
    int wifi_x, wifi_y, battery_x, battery_y;
    getStatusIconPositions(rects, rect_count, status_rect_index, 
                        &wifi_x, &wifi_y, &battery_x, &battery_y);

    // 对状态栏图标坐标进行缩放
    int wifi_display_x = (int)(wifi_x * global_scale + 0.5f);
    int wifi_display_y = (int)(wifi_y * global_scale + 0.5f);
    int battery_display_x = (int)(battery_x * global_scale + 0.5f);
    int battery_display_y = (int)(battery_y * global_scale + 0.5f);

    // 边界检查
    if (wifi_display_x < 0) wifi_display_x = 0;
    if (wifi_display_y < 0) wifi_display_y = 0;
    if (battery_display_x < 0) battery_display_x = 0;
    if (battery_display_y < 0) battery_display_y = 0;

    cJSON *status_icons = cJSON_CreateObject();
    if (status_icons) {
        cJSON *wifi = cJSON_CreateObject();
        cJSON_AddNumberToObject(wifi, "x", wifi_display_x);
        cJSON_AddNumberToObject(wifi, "y", wifi_display_y);
        cJSON_AddNumberToObject(wifi, "width", 32);
        cJSON_AddNumberToObject(wifi, "height", 32);
        cJSON_AddItemToObject(status_icons, "wifi", wifi);
        
        cJSON *battery = cJSON_CreateObject();
        cJSON_AddNumberToObject(battery, "x", battery_display_x);
        cJSON_AddNumberToObject(battery, "y", battery_display_y);
        cJSON_AddNumberToObject(battery, "width", 36);
        cJSON_AddNumberToObject(battery, "height", 24);
        cJSON_AddItemToObject(status_icons, "battery", battery);
        
        cJSON_AddItemToObject(root, "status_icons", status_icons);
    }
    
    // 转换为JSON字符串
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        ESP_LOGE("WEB_LAYOUT", "cJSON_PrintUnformatted 返回 NULL");
        cJSON_Delete(root);
        return false;
    }
    
    int len = strlen(json_str);
    ESP_LOGI("WEB_LAYOUT", "JSON长度: %d, 缓冲区大小: %d", len, output_size);
    
    if (len < output_size) {
        strcpy(output, json_str);
        ESP_LOGI("WEB_LAYOUT", "JSON数据生成成功");
        // 不要打印整个JSON，只打印前100个字符用于调试
        if (len > 100) {
            ESP_LOGD("WEB_LAYOUT", "JSON前100字符: %.100s", json_str);
        } else {
            ESP_LOGD("WEB_LAYOUT", "JSON: %s", json_str);
        }
        free(json_str);
        cJSON_Delete(root);
        return true;
    } else {
        ESP_LOGE("WEB_LAYOUT", "输出缓冲区太小: 需要 %d, 实际 %d", len, output_size);
        free(json_str);
        cJSON_Delete(root);
        return false;
    }
}



typedef struct {
    int rect_index;
    char type[10]; // "rect" 或 "icon"
    char change[20]; // "added", "removed", "modified", "moved"
    int x, y;
    int width, height;
    int icon_index; // 如果是图标
} LayoutChange;

// 比较两个布局的差异
bool compareLayouts(RectInfo *web_rects, int web_rect_count, 
                    RectInfo *current_rects, int current_rect_count,
                    LayoutChange *changes, int *change_count) {
    
    *change_count = 0;
    
    // 比较矩形数量
    if (web_rect_count != current_rect_count) {
        changes[*change_count].rect_index = -1;
        strcpy(changes[*change_count].type, "rect");
        strcpy(changes[*change_count].change, "count_changed");
        changes[*change_count].x = web_rect_count;
        changes[*change_count].y = current_rect_count;
        (*change_count)++;
    }
    
    // 比较每个矩形
    int max_rects = (web_rect_count > current_rect_count) ? web_rect_count : current_rect_count;
    
    for (int i = 0; i < max_rects; i++) {
        RectInfo web_rect = web_rects[i];
        RectInfo current_rect = current_rects[i];
        
        // 检查矩形位置和尺寸变化
        if (web_rect.x != current_rect.x || web_rect.y != current_rect.y ||
            web_rect.width != current_rect.width || web_rect.height != current_rect.height) {
            
            changes[*change_count].rect_index = i;
            strcpy(changes[*change_count].type, "rect");
            strcpy(changes[*change_count].change, "moved_or_resized");
            changes[*change_count].x = web_rect.x - current_rect.x;
            changes[*change_count].y = web_rect.y - current_rect.y;
            changes[*change_count].width = web_rect.width;
            changes[*change_count].height = web_rect.height;
            (*change_count)++;
            
            ESP_LOGI(TAG, "矩形 %d 发生变化: 位置(%d,%d)->(%d,%d), 尺寸(%dx%d)->(%dx%d)", 
                    i, current_rect.x, current_rect.y, web_rect.x, web_rect.y,
                    current_rect.width, current_rect.height, web_rect.width, web_rect.height);
        }
        
        // 比较图标数量
        if (web_rect.icon_count != current_rect.icon_count) {
            changes[*change_count].rect_index = i;
            strcpy(changes[*change_count].type, "icon");
            strcpy(changes[*change_count].change, "count_changed");
            changes[*change_count].x = web_rect.icon_count;
            changes[*change_count].y = current_rect.icon_count;
            (*change_count)++;
            
            ESP_LOGI(TAG, "矩形 %d 图标数量变化: %d -> %d", i, current_rect.icon_count, web_rect.icon_count);
        }
        
        // 比较图标位置
        int max_icons = (web_rect.icon_count > current_rect.icon_count) ? 
                       web_rect.icon_count : current_rect.icon_count;
        
        for (int j = 0; j < max_icons; j++) {
            bool web_icon_exists = (j < web_rect.icon_count);
            bool current_icon_exists = (j < current_rect.icon_count);
            
            if (web_icon_exists && !current_icon_exists) {
                // 新增图标
                changes[*change_count].rect_index = i;
                strcpy(changes[*change_count].type, "icon");
                strcpy(changes[*change_count].change, "added");
                changes[*change_count].icon_index = web_rect.icons[j].icon_index;
                changes[*change_count].x = (int)(web_rect.icons[j].rel_x * 100); // 百分比
                changes[*change_count].y = (int)(web_rect.icons[j].rel_y * 100);
                (*change_count)++;
                
                ESP_LOGI(TAG, "矩形 %d 新增图标 %d (索引: %d, 位置: %.2f,%.2f)", 
                        i, j, web_rect.icons[j].icon_index,
                        web_rect.icons[j].rel_x, web_rect.icons[j].rel_y);
            }
            else if (!web_icon_exists && current_icon_exists) {
                // 删除图标
                changes[*change_count].rect_index = i;
                strcpy(changes[*change_count].type, "icon");
                strcpy(changes[*change_count].change, "removed");
                changes[*change_count].icon_index = current_rect.icons[j].icon_index;
                (*change_count)++;
                
                ESP_LOGI(TAG, "矩形 %d 删除图标 %d (索引: %d)", i, j, current_rect.icons[j].icon_index);
            }
            else if (web_icon_exists && current_icon_exists) {
                // 检查图标是否修改
                IconPositionInRect web_icon = web_rect.icons[j];
                IconPositionInRect current_icon = current_rect.icons[j];
                
                float pos_threshold = 0.01f; // 位置变化阈值
                
                if (web_icon.icon_index != current_icon.icon_index ||
                    fabs(web_icon.rel_x - current_icon.rel_x) > pos_threshold ||
                    fabs(web_icon.rel_y - current_icon.rel_y) > pos_threshold) {
                    
                    changes[*change_count].rect_index = i;
                    strcpy(changes[*change_count].type, "icon");
                    strcpy(changes[*change_count].change, "modified");
                    changes[*change_count].icon_index = web_icon.icon_index;
                    changes[*change_count].x = (int)(web_icon.rel_x * 100);
                    changes[*change_count].y = (int)(web_icon.rel_y * 100);
                    (*change_count)++;
                    
                    ESP_LOGI(TAG, "矩形 %d 图标 %d 修改: 索引 %d->%d, 位置 (%.2f,%.2f)->(%.2f,%.2f)",
                            i, j, current_icon.icon_index, web_icon.icon_index,
                            current_icon.rel_x, current_icon.rel_y, web_icon.rel_x, web_icon.rel_y);
                }
            }
        }
    }
    
    return (*change_count > 0);
}

bool parseAndApplyLayout(const char* layout_json) {
    cJSON *root = cJSON_Parse(layout_json);
    if (!root) {
        ESP_LOGI(TAG, "Failed to parse layout JSON");
        return false;
    }
    
    bool any_real_changes = false;
    
    // 解析矩形信息
    cJSON *rects_array = cJSON_GetObjectItem(root, "rectangles");
    if (rects_array && cJSON_IsArray(rects_array)) {
        int web_rect_count = cJSON_GetArraySize(rects_array);
        ESP_LOGI(TAG, "Web布局包含 %d 个矩形", web_rect_count);
        
        for (int i = 0; i < web_rect_count && i < 7; i++) {
            cJSON *rect_obj = cJSON_GetArrayItem(rects_array, i);
            if (rect_obj) {
                // 处理图标
                cJSON *icons_array = cJSON_GetObjectItem(rect_obj, "icons");
                if (icons_array && cJSON_IsArray(icons_array)) {
                    int web_icon_count = cJSON_GetArraySize(icons_array);
                    
                    // 逐个处理Web图标
                    for (int j = 0; j < web_icon_count && j < 4; j++) {
                        cJSON *icon_obj = cJSON_GetArrayItem(icons_array, j);
                        if (icon_obj) {
                            cJSON *icon_index = cJSON_GetObjectItem(icon_obj, "icon_index");
                            cJSON *rel_x = cJSON_GetObjectItem(icon_obj, "rel_x");
                            cJSON *rel_y = cJSON_GetObjectItem(icon_obj, "rel_y");
                            
                            if (icon_index && rel_x && rel_y) {
                                float new_rel_x = rel_x->valuedouble;
                                float new_rel_y = rel_y->valuedouble;
                                int new_icon_index = icon_index->valueint;
                                
                                // 应用修正：如果Web使用中心点，设备也使用中心点
                                // 或者调整偏移量
                                
                                // 检查是否与当前配置不同
                                if (j < rects[i].icon_count) {
                                    float diff_x = fabs(rects[i].icons[j].rel_x - new_rel_x);
                                    float diff_y = fabs(rects[i].icons[j].rel_y - new_rel_y);
                                    
                                    // 设置阈值：只有大于5%的变化才认为是用户修改
                                    float threshold = 0.05f;
                                    
                                    if (diff_x > threshold || diff_y > threshold) {
                                        any_real_changes = true;
                                        ESP_LOGI(TAG, "矩形 %d 图标 %d 被用户修改:", i, j);
                                        ESP_LOGI(TAG, "  旧位置: (%.4f,%.4f)", 
                                                rects[i].icons[j].rel_x, rects[i].icons[j].rel_y);
                                        ESP_LOGI(TAG, "  新位置: (%.4f,%.4f)", 
                                                new_rel_x, new_rel_y);
                                        ESP_LOGI(TAG, "  变化量: dx=%+.4f, dy=%+.4f",
                                                new_rel_x - rects[i].icons[j].rel_x,
                                                new_rel_y - rects[i].icons[j].rel_y);
                                    } else if (diff_x > 0.001 || diff_y > 0.001) {
                                        // 微小变化，可能是计算误差
                                        ESP_LOGD(TAG, "矩形 %d 图标 %d 微小差异: dx=%.4f, dy=%.4f",
                                                i, j, diff_x, diff_y);
                                    }
                                } else {
                                    // 新增图标
                                    any_real_changes = true;
                                    ESP_LOGI(TAG, "矩形 %d 新增图标 %d", i, j);
                                }
                                
                                // 更新图标
                                rects[i].icons[j].icon_index = new_icon_index;
                                rects[i].icons[j].rel_x = new_rel_x;
                                rects[i].icons[j].rel_y = new_rel_y;
                            }
                        }
                    }
                    
                    // 更新图标数量
                    rects[i].icon_count = (web_icon_count < 4) ? web_icon_count : 4;
                }
            }
        }
        
        if (any_real_changes) {
            ESP_LOGI(TAG, "检测到用户修改的布局变化");
        } else {
            ESP_LOGI(TAG, "没有检测到显著的布局变化");
        }
        
        // 保存到配置文件
        saveLayoutToConfig();
        
        // 立即更新显示
        updateDisplayWithMain(rects, web_rect_count, 0, 1);
        
        ESP_LOGI(TAG, "布局已成功应用并显示");
    }
    
    cJSON_Delete(root);
    return true;
}



// // 解析并应用布局的辅助函数
// bool parseAndApplyLayout(const char* layout_json) {
//     cJSON *root = cJSON_Parse(layout_json);
//     if (!root) {
//         ESP_LOGI(TAG, "Failed to parse layout JSON");
//         return false;
//     }
    
//     // 解析屏幕信息
//     cJSON *screen = cJSON_GetObjectItem(root, "screen");
//     if (screen) {
//         cJSON *width = cJSON_GetObjectItem(screen, "width");
//         cJSON *height = cJSON_GetObjectItem(screen, "height");
//         if (width && height) {
//             // 可以更新屏幕设置
//             ESP_LOGI(TAG, "Screen size: %dx%d", width->valueint, height->valueint);
//         }
//     }
    
//     // 解析矩形信息
//     cJSON *rects_array = cJSON_GetObjectItem(root, "rectangles");
//     if (rects_array && cJSON_IsArray(rects_array)) {
//         int rect_count = cJSON_GetArraySize(rects_array);
//         ESP_LOGI(TAG, "Found %d rectangles in layout", rect_count);
        
//         // 这里可以更新全局的rects数组
//         // 注意：需要确保rects数组足够大或者动态分配
        
//         for (int i = 0; i < rect_count && i < 7; i++) { // 限制最多7个矩形
//             cJSON *rect_obj = cJSON_GetArrayItem(rects_array, i);
//             if (rect_obj) {
//                 // 解析矩形基本信息
//                 cJSON *x = cJSON_GetObjectItem(rect_obj, "original_x");
//                 cJSON *y = cJSON_GetObjectItem(rect_obj, "original_y");
//                 cJSON *width = cJSON_GetObjectItem(rect_obj, "original_width");
//                 cJSON *height = cJSON_GetObjectItem(rect_obj, "original_height");
//                 cJSON *icon_count = cJSON_GetObjectItem(rect_obj, "icon_count");
                
//                 if (x && y && width && height && icon_count) {
//                     // 更新矩形信息
//                     if (i < rect_count) {
//                         rects[i].x = x->valueint;
//                         rects[i].y = y->valueint;
//                         rects[i].width = width->valueint;
//                         rects[i].height = height->valueint;
//                         rects[i].icon_count = 0; // 先重置
                        
//                         // 解析图标信息
//                         cJSON *icons_array = cJSON_GetObjectItem(rect_obj, "icons");
//                         if (icons_array && cJSON_IsArray(icons_array)) {
//                             int icon_array_size = cJSON_GetArraySize(icons_array);
//                             int icons_to_add = (icon_array_size < 4) ? icon_array_size : 4;
                            
//                             for (int j = 0; j < icons_to_add; j++) {
//                                 cJSON *icon_obj = cJSON_GetArrayItem(icons_array, j);
//                                 if (icon_obj) {
//                                     cJSON *icon_index = cJSON_GetObjectItem(icon_obj, "icon_index");
//                                     cJSON *rel_x = cJSON_GetObjectItem(icon_obj, "rel_x");
//                                     cJSON *rel_y = cJSON_GetObjectItem(icon_obj, "rel_y");
                                    
//                                     if (icon_index && rel_x && rel_y) {
//                                         rects[i].icons[j].icon_index = icon_index->valueint;
//                                         rects[i].icons[j].rel_x = rel_x->valuedouble;
//                                         rects[i].icons[j].rel_y = rel_y->valuedouble;
//                                         rects[i].icon_count++;
//                                     }
//                                 }
//                             }
//                         }
//                     }
//                 }
//             }
//         }
        
//         // 保存到配置文件
//         saveLayoutToConfig();
//        // initIconPositions();
//         // 立即更新显示
//      //   displayMainScreen(rects, rect_count, 0, 1);
//         updateDisplayWithMain(rects, rect_count, 0, 1);
//     }
    
//     cJSON_Delete(root);
//     return true;
// }

// 在 handleGetLayout 函数中，确保返回的是当前实际位置
String getCurrentLayoutJson() {
    String json = "{";
    
    // 添加屏幕信息
    json += "\"screen\": {\"width\": 416, \"height\": 240},";
    
    // 添加矩形信息
    json += "\"rectangles\": [";
    
    for (int i = 0; i < 7; i++) {
        if (i > 0) json += ",";
        
        json += "{";
        json += "\"x\": " + String(rects[i].x) + ",";
        json += "\"y\": " + String(rects[i].y) + ",";
        json += "\"width\": " + String(rects[i].width) + ",";
        json += "\"height\": " + String(rects[i].height) + ",";
        json += "\"icon_count\": " + String(rects[i].icon_count) + ",";
        json += "\"is_status_bar\": " + String((i == 0) ? "true" : "false") + ",";
        
        // 添加图标信息 - 使用当前存储的位置
        json += "\"icons\": [";
        for (int j = 0; j < rects[i].icon_count; j++) {
            if (j > 0) json += ",";
            
            json += "{";
            json += "\"icon_index\": " + String(rects[i].icons[j].icon_index) + ",";
            // 使用当前存储的相对位置
            json += "\"rel_x\": " + String(rects[i].icons[j].rel_x, 4) + ",";
            json += "\"rel_y\": " + String(rects[i].icons[j].rel_y, 4);
            json += "}";
            
            ESP_LOGI(TAG, "getCurrentLayoutJson: 矩形 %d 图标 %d: rel_x=%.4f, rel_y=%.4f",
                    i, j, rects[i].icons[j].rel_x, rects[i].icons[j].rel_y);
        }
        json += "]";
        json += "}";
    }
    
    json += "],";
    json += "\"status_rect_index\": 0";
    json += "}";
    
    return json;
}


// 获取当前布局信息的辅助函数
bool getCurrentLayoutInfo(char *output, int output_size) {
    if (!rects || rect_count <= 0) {
        ESP_LOGE("WEB_LAYOUT", "rects is null or rect_count is 0");
        return false;
    }
    return getMainLayoutInfo(rects, rect_count, 0, output, output_size);
}

/**
 * @brief 获取所有矩形的基本信息（用于Web界面网格显示）
 * @param rects 矩形数组
 * @param rect_count 矩形数量
 * @param web_rects 输出Web矩形信息数组
 * @param max_rects 最大矩形数量
 * @return 实际获取的矩形数量
 */
int getAllRectInfo(RectInfo *rects, int rect_count, WebRectInfo *web_rects, int max_rects) {
    if (!rects || !web_rects || max_rects <= 0) return 0;
    
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    int count = (rect_count < max_rects) ? rect_count : max_rects;
    
    for (int i = 0; i < count; i++) {
        RectInfo *rect = &rects[i];
        WebRectInfo *web_rect = &web_rects[i];
        
        // 计算显示坐标
        web_rect->x = (int)(rect->x * global_scale + 0.5f);
        web_rect->y = (int)(rect->y * global_scale + 0.5f);
        web_rect->width = (int)(rect->width * global_scale + 0.5f);
        web_rect->height = (int)(rect->height * global_scale + 0.5f);
        web_rect->icon_count = rect->icon_count;
        
        // 生成图标信息JSON
        char icons_json[256] = "[";
        char temp[64];
        for (int j = 0; j < rect->icon_count; j++) {
            IconPositionInRect *icon_pos = &rect->icons[j];
            snprintf(temp, sizeof(temp), 
                    "{\"idx\":%d,\"rx\":%.2f,\"ry\":%.2f}%s",
                    icon_pos->icon_index, icon_pos->rel_x, icon_pos->rel_y,
                    (j < rect->icon_count - 1) ? "," : "");
            strcat(icons_json, temp);
        }
        strcat(icons_json, "]");
        strncpy(web_rect->icons, icons_json, sizeof(web_rect->icons) - 1);
        web_rect->icons[sizeof(web_rect->icons) - 1] = '\0';
    }
    
    return count;
}

/**
 * @brief 获取所有图标的位置信息
 * @param web_icons 输出Web图标信息数组
 * @param max_icons 最大图标数量
 * @return 实际获取的图标数量
 */
int getAllIconInfo(WebIconInfo *web_icons, int max_icons) {
    if (!web_icons || max_icons <= 0) return 0;
    
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    int count = (g_global_icon_count < max_icons) ? g_global_icon_count : max_icons;
    
    for (int i = 0; i < count; i++) {
        WebIconInfo *web_icon = &web_icons[i];
        
        web_icon->x = (int)(g_icon_positions[i].x * global_scale);
        web_icon->y = (int)(g_icon_positions[i].y * global_scale);
        web_icon->width = (int)(g_icon_positions[i].width * global_scale);
        web_icon->height = (int)(g_icon_positions[i].height * global_scale);
        web_icon->icon_index = g_icon_positions[i].icon_index;
        web_icon->global_index = i;
        
        // 需要确定所属矩形索引（这里简化处理，实际需要根据坐标判断）
        web_icon->rect_index = -1; // 可以在调用时通过其他方式确定
    }
    
    return count;
}

#include "ink_screen.h"
#include "web_layout.h"

// 依赖于ink_screen.cpp中的全局单词界面矩形数组
// extern声明已在ink_screen.h中

// 前置声明
void saveVocabLayoutToConfig();

extern "C" {
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"
}
#define TAG "web_layout.cpp"

// ========== 图标索引说明 ==========
// 所有图标操作中的 icon_index 都对应：
// - ink_screen.cpp 中的 g_available_icons 数组（0-12）
// - ink_screen.cpp 中的 g_icon_filenames 数组（0-12）
// - components/resource/icon/ 文件夹中的文件顺序（0-12）
// 这三者的顺序必须保持完全一致
// 详见 g_icon_filenames[] 的定义

// 保存布局到配置文件（主界面）
void saveLayoutToConfig() {
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        if (state == SDState::NotPresent) {
            ESP_LOGI(TAG, "No SD Card\r\n");
        } else {
            ESP_LOGI(TAG, "SD Card Busy\r\n");
        }
    }

    String config_filename = "/layout_config.json";

    // 删除旧文件，避免残留
    if (SD.exists(config_filename.c_str())) {
        SD.remove(config_filename.c_str());
        ESP_LOGI(TAG, "删除旧的配置文件: %s", config_filename.c_str());
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    // 从ScreenManager获取实际的主界面矩形数量，而不是使用全局变量
    int actual_rect_count = getMainScreenRectCount();
    ESP_LOGI(TAG, "保存主界面配置，矩形数量: %d (全局rect_count=%d)", actual_rect_count, rect_count);
    
    // 保存矩形数量
    cJSON_AddNumberToObject(root, "rect_count", actual_rect_count);

    // 保存矩形数组
    cJSON *rects_array = cJSON_CreateArray();
    if (rects_array) {
        for (int i = 0; i < actual_rect_count; i++) {
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
                    for (int j = 0; j < rect->icon_count && j < 4; j++) {
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

                // 保存文本信息
                cJSON_AddNumberToObject(rect_obj, "text_count", rect->text_count);
                cJSON *texts_array = cJSON_CreateArray();
                if (texts_array) {
                    for (int j = 0; j < rect->text_count && j < 4; j++) {
                        cJSON *text_obj = cJSON_CreateObject();
                        if (text_obj) {
                            TextPositionInRect *text = &rect->texts[j];
                            cJSON_AddNumberToObject(text_obj, "content_type", text->type);
                            cJSON_AddNumberToObject(text_obj, "rel_x", text->rel_x);
                            cJSON_AddNumberToObject(text_obj, "rel_y", text->rel_y);
                            cJSON_AddNumberToObject(text_obj, "font_size", text->font_size);
                            cJSON_AddNumberToObject(text_obj, "h_align", text->h_align);
                            cJSON_AddNumberToObject(text_obj, "v_align", text->v_align);
                            cJSON_AddItemToArray(texts_array, text_obj);
                        }
                    }
                    cJSON_AddItemToObject(rect_obj, "texts", texts_array);
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
        if (new_rect_count > MAX_MAIN_RECTS) new_rect_count = MAX_MAIN_RECTS;
        
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
                        
                        // 解析文本信息（支持数值型字段）
                        cJSON *text_count_obj = cJSON_GetObjectItem(rect_obj, "text_count");
                        if (text_count_obj) {
                            rects[i].text_count = text_count_obj->valueint;
                            
                            cJSON *texts_array = cJSON_GetObjectItem(rect_obj, "texts");
                            if (texts_array && cJSON_IsArray(texts_array)) {
                                int text_array_size = cJSON_GetArraySize(texts_array);
                                int texts_to_load = (text_array_size < rects[i].text_count && text_array_size < 4) ? text_array_size : (rects[i].text_count < 4 ? rects[i].text_count : 4);
                                
                                for (int j = 0; j < texts_to_load; j++) {
                                    cJSON *text_obj = cJSON_GetArrayItem(texts_array, j);
                                    if (text_obj) {
                                        cJSON *content_type = cJSON_GetObjectItem(text_obj, "content_type");
                                        cJSON *rel_x = cJSON_GetObjectItem(text_obj, "rel_x");
                                        cJSON *rel_y = cJSON_GetObjectItem(text_obj, "rel_y");
                                        cJSON *font_size = cJSON_GetObjectItem(text_obj, "font_size");
                                        cJSON *h_align = cJSON_GetObjectItem(text_obj, "h_align");
                                        cJSON *v_align = cJSON_GetObjectItem(text_obj, "v_align");
                                        
                                        // 调试：打印原始 JSON 数据
                                        char* text_json_str = cJSON_Print(text_obj);
                                        ESP_LOGI(TAG, "文本%d 原始JSON: %s", j, text_json_str);
                                        free(text_json_str);
                                        
                                        // 调试：检查 content_type 字段
                                        if (content_type) {
                                            ESP_LOGI(TAG, "  content_type 存在, 类型:%d, 值:%d", 
                                                    content_type->type, content_type->valueint);
                                        } else {
                                            ESP_LOGW(TAG, "  content_type 字段为 NULL!");
                                        }
                                        
                                        // 设置 content_type（如果不存在则使用默认值）
                                        if (content_type && cJSON_IsNumber(content_type)) {
                                            rects[i].texts[j].type = (RectContentType)content_type->valueint;
                                            ESP_LOGI(TAG, "  矩形%d 文本%d 设置 type = %d (来自 JSON 数字)", i, j, rects[i].texts[j].type);
                                        } else if (content_type && cJSON_IsString(content_type)) {
                                            // 兼容老的字符串形式
                                            const char *type_str = content_type->valuestring;
                                            if (strcmp(type_str, "WORD") == 0) {
                                                rects[i].texts[j].type = CONTENT_WORD;
                                            } else if (strcmp(type_str, "PHONETIC") == 0) {
                                                rects[i].texts[j].type = CONTENT_PHONETIC;
                                            } else if (strcmp(type_str, "DEFINITION") == 0) {
                                                rects[i].texts[j].type = CONTENT_DEFINITION;
                                            } else if (strcmp(type_str, "TRANSLATION") == 0) {
                                                rects[i].texts[j].type = CONTENT_TRANSLATION;
                                            } else {
                                                rects[i].texts[j].type = CONTENT_WORD;  // 默认为单词
                                            }
                                        }
                                        if (rel_x) {
                                            rects[i].texts[j].rel_x = rel_x->valuedouble;
                                        }
                                        if (rel_y) {
                                            rects[i].texts[j].rel_y = rel_y->valuedouble;
                                        }
                                        if (font_size) {
                                            rects[i].texts[j].font_size = font_size->valueint;
                                        }
                                        if (h_align && cJSON_IsNumber(h_align)) {
                                            rects[i].texts[j].h_align = (TextAlignment)h_align->valueint;
                                        } else if (h_align && cJSON_IsString(h_align)) {
                                            const char *align_str = h_align->valuestring;
                                            if (strcmp(align_str, "left") == 0) rects[i].texts[j].h_align = ALIGN_LEFT;
                                            else if (strcmp(align_str, "center") == 0) rects[i].texts[j].h_align = ALIGN_CENTER;
                                            else if (strcmp(align_str, "right") == 0) rects[i].texts[j].h_align = ALIGN_RIGHT;
                                            else rects[i].texts[j].h_align = ALIGN_LEFT;
                                        }
                                        if (v_align && cJSON_IsNumber(v_align)) {
                                            rects[i].texts[j].v_align = (TextAlignment)v_align->valueint;
                                        } else if (v_align && cJSON_IsString(v_align)) {
                                            const char *align_str = v_align->valuestring;
                                            if (strcmp(align_str, "top") == 0) rects[i].texts[j].v_align = ALIGN_TOP;
                                            else if (strcmp(align_str, "middle") == 0) rects[i].texts[j].v_align = ALIGN_MIDDLE;
                                            else if (strcmp(align_str, "bottom") == 0) rects[i].texts[j].v_align = ALIGN_BOTTOM;
                                            else rects[i].texts[j].v_align = ALIGN_TOP;
                                        }
                                    }
                                }
                                rects[i].text_count = texts_to_load; // 更新为实际加载的文本数量
                            }
                        } else {
                            rects[i].text_count = 0;
                        }
                    }
                }
            }
            
            ESP_LOGI(TAG, "Layout config loaded successfully: %d rectangles", rects_to_load);
            
            // 更新主界面矩形数量
            updateMainScreenRectCount(rects_to_load);
            
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
        ESP_LOGW(TAG, "主界面配置文件不存在或加载失败");
        ESP_LOGW(TAG, "请通过web端配置主界面布局，配置后会自动保存到SD卡");
        ESP_LOGW(TAG, "不设置默认矩形数量，避免覆盖用户的web端配置");
        // 不再设置 rect_count = sizeof(rects) / sizeof(rects[0]);
        // 保持矩形数量为0，等待用户通过web端配置
    }
    
    // 加载单词界面布局
    if (loadVocabLayoutFromConfig()) {
        ESP_LOGI(TAG, "Vocab layout loaded from config file");
    } else {
        ESP_LOGW(TAG, "单词界面配置文件不存在或加载失败");
        ESP_LOGW(TAG, "请通过web端配置单词界面布局，配置后会自动保存到SD卡");
        ESP_LOGW(TAG, "不设置默认矩形数量，避免覆盖用户的web端配置");
        // 不再检测默认矩形数量，不调用 updateVocabScreenRectCount()
        // 保持矩形数量为0，等待用户通过web端配置
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
    ESP_LOGI("WEB_LAYOUT", "========== getMainLayoutInfo 开始 ==========");
    ESP_LOGI("WEB_LAYOUT", "参数: rects=%p, rect_count=%d, output=%p, output_size=%d", 
             rects, rect_count, output, output_size);
    
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
    ESP_LOGD("WEB_LAYOUT", "JSON根对象创建成功");
    
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
    ESP_LOGD("WEB_LAYOUT", "rects_array创建成功");
    
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
                        
                        // 添加来自 components/resource/icon/ 文件夹的图标文件名
                        if (icon_pos->icon_index >= 0 && icon_pos->icon_index < 13) {
                            cJSON_AddStringToObject(icon_obj, "filename", g_icon_filenames[icon_pos->icon_index]);
                            cJSON_AddStringToObject(icon_obj, "resource_path", "/sd/resource/icon");
                        }
                        
                        cJSON_AddItemToArray(icons_array, icon_obj);
                        valid_icon_count++;
                    }
                }
            }
            cJSON_AddItemToObject(rect_obj, "icons", icons_array);
            ESP_LOGD("WEB_LAYOUT", "矩形[%d]有 %d 个有效图标", i, valid_icon_count);
        }
        
        // 添加矩形内的文本信息
        cJSON *texts_array = cJSON_CreateArray();
        if (texts_array) {
            for (int j = 0; j < rect->text_count; j++) {
                TextPositionInRect *text_pos = &rect->texts[j];
                
                cJSON *text_obj = cJSON_CreateObject();
                if (text_obj) {
                    // 保留两位小数
                    double rel_x_rounded = round(text_pos->rel_x * 100.0) / 100.0;
                    double rel_y_rounded = round(text_pos->rel_y * 100.0) / 100.0;
                    
                    // 确保在0-1范围内
                    if (rel_x_rounded < 0.0) rel_x_rounded = 0.0;
                    if (rel_x_rounded > 1.0) rel_x_rounded = 1.0;
                    if (rel_y_rounded < 0.0) rel_y_rounded = 0.0;
                    if (rel_y_rounded > 1.0) rel_y_rounded = 1.0;
                    
                    cJSON_AddNumberToObject(text_obj, "rel_x", rel_x_rounded);
                    cJSON_AddNumberToObject(text_obj, "rel_y", rel_y_rounded);
                    cJSON_AddNumberToObject(text_obj, "content_type", text_pos->type);
                    cJSON_AddStringToObject(text_obj, "content", ""); // 添加空的内容字段
                    cJSON_AddNumberToObject(text_obj, "font_size", text_pos->font_size);
                    cJSON_AddNumberToObject(text_obj, "h_align", text_pos->h_align);
                    cJSON_AddNumberToObject(text_obj, "v_align", text_pos->v_align);
                    
                    // 添加类型名称（便于调试）
                    const char* type_name = "UNKNOWN";
                    switch(text_pos->type) {
                        case CONTENT_WORD: type_name = "WORD"; break;
                        case CONTENT_PHONETIC: type_name = "PHONETIC"; break;
                        case CONTENT_DEFINITION: type_name = "DEFINITION"; break;
                        case CONTENT_TRANSLATION: type_name = "TRANSLATION"; break;
                        case CONTENT_NONE:
                        case CONTENT_STATUS_BAR:
                        case CONTENT_ICON_ONLY:
                        case CONTENT_SEPARATOR:
                        case CONTENT_CUSTOM:
                        default:
                            type_name = "UNKNOWN";
                            break;
                    }
                    cJSON_AddStringToObject(text_obj, "type_name", type_name);
                    
                    cJSON_AddItemToArray(texts_array, text_obj);
                }
            }
            cJSON_AddItemToObject(rect_obj, "texts", texts_array);
            ESP_LOGD("WEB_LAYOUT", "矩形[%d]有 %d 个文本元素", i, rect->text_count);
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
                    
                    // 添加来自 components/resource/icon/ 文件夹的图标文件名
                    if (g_icon_positions[i].icon_index >= 0 && g_icon_positions[i].icon_index < 13) {
                        cJSON_AddStringToObject(icon_obj, "filename", g_icon_filenames[g_icon_positions[i].icon_index]);
                        cJSON_AddStringToObject(icon_obj, "resource_path", "/sd/resource/icon");
                    }
                    
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
    ESP_LOGI("WEB_LAYOUT", "cJSON_PrintUnformatted 成功");
    
    int len = strlen(json_str);
    ESP_LOGI("WEB_LAYOUT", "JSON长度: %d, 缓冲区大小: %d", len, output_size);
    
    if (len < output_size) {
        strcpy(output, json_str);
        ESP_LOGI("WEB_LAYOUT", "JSON数据生成成功");
        ESP_LOGI("WEB_LAYOUT", "========== getMainLayoutInfo 结束 (成功) ==========");
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
        ESP_LOGI("WEB_LAYOUT", "========== getMainLayoutInfo 结束 (失败-缓冲区) ==========");
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

bool parseAndApplyVocabLayout(const char* layout_json) {
    extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
    cJSON *root = cJSON_Parse(layout_json);
    if (!root) {
        ESP_LOGI(TAG, "Failed to parse vocab layout JSON");
        return false;
    }
    
    bool any_real_changes = false;
    
    // 解析矩形信息
    cJSON *rects_array = cJSON_GetObjectItem(root, "rectangles");
    if (rects_array && cJSON_IsArray(rects_array)) {
        int web_rect_count = cJSON_GetArraySize(rects_array);
        ESP_LOGI(TAG, "单词界面Web布局包含 %d 个矩形", web_rect_count);
        
        for (int i = 0; i < web_rect_count && i < MAX_VOCAB_RECTS; i++) {
            cJSON *rect_obj = cJSON_GetArrayItem(rects_array, i);
            if (rect_obj) {
                // 更新矩形的坐标和尺寸
                cJSON *x_item = cJSON_GetObjectItem(rect_obj, "x");
                cJSON *y_item = cJSON_GetObjectItem(rect_obj, "y");
                cJSON *width_item = cJSON_GetObjectItem(rect_obj, "width");
                cJSON *height_item = cJSON_GetObjectItem(rect_obj, "height");
                
                if (x_item && y_item && width_item && height_item) {
                    int new_x = x_item->valueint;
                    int new_y = y_item->valueint;
                    int new_width = width_item->valueint;
                    int new_height = height_item->valueint;
                    
                    if (vocab_rects[i].x != new_x || vocab_rects[i].y != new_y || 
                        vocab_rects[i].width != new_width || vocab_rects[i].height != new_height) {
                        any_real_changes = true;
                        ESP_LOGI(TAG, "单词界面矩形 %d 尺寸/位置变化", i);
                    }
                    
                    vocab_rects[i].x = new_x;
                    vocab_rects[i].y = new_y;
                    vocab_rects[i].width = new_width;
                    vocab_rects[i].height = new_height;
                }
                
                // 处理图标（参照主界面逻辑）
                cJSON *icons_array = cJSON_GetObjectItem(rect_obj, "icons");
                if (icons_array && cJSON_IsArray(icons_array)) {
                    int web_icon_count = cJSON_GetArraySize(icons_array);
                    ESP_LOGI(TAG, "单词界面矩形 %d 包含 %d 个图标", i, web_icon_count);
                    
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
                                
                                // 检查是否与当前配置不同
                                if (j < vocab_rects[i].icon_count) {
                                    float diff_x = fabs(vocab_rects[i].icons[j].rel_x - new_rel_x);
                                    float diff_y = fabs(vocab_rects[i].icons[j].rel_y - new_rel_y);
                                    
                                    // 设置阈值：只有大于5%的变化才认为是用户修改
                                    float threshold = 0.05f;
                                    
                                    if (diff_x > threshold || diff_y > threshold) {
                                        any_real_changes = true;
                                        ESP_LOGI(TAG, "单词界面矩形 %d 图标 %d 被用户修改:", i, j);
                                        ESP_LOGI(TAG, "  旧位置: (%.4f,%.4f)", 
                                                vocab_rects[i].icons[j].rel_x, vocab_rects[i].icons[j].rel_y);
                                        ESP_LOGI(TAG, "  新位置: (%.4f,%.4f)", 
                                                new_rel_x, new_rel_y);
                                        ESP_LOGI(TAG, "  变化量: dx=%+.4f, dy=%+.4f",
                                                new_rel_x - vocab_rects[i].icons[j].rel_x,
                                                new_rel_y - vocab_rects[i].icons[j].rel_y);
                                    } else if (diff_x > 0.001 || diff_y > 0.001) {
                                        // 微小变化，可能是计算误差
                                        ESP_LOGD(TAG, "单词界面矩形 %d 图标 %d 微小差异: dx=%.4f, dy=%.4f",
                                                i, j, diff_x, diff_y);
                                    }
                                } else {
                                    // 新增图标
                                    any_real_changes = true;
                                    ESP_LOGI(TAG, "单词界面矩形 %d 新增图标 %d", i, j);
                                }
                                
                                // 更新图标
                                vocab_rects[i].icons[j].icon_index = new_icon_index;
                                vocab_rects[i].icons[j].rel_x = new_rel_x;
                                vocab_rects[i].icons[j].rel_y = new_rel_y;
                            }
                        }
                    }
                    
                    vocab_rects[i].icon_count = (web_icon_count < 4) ? web_icon_count : 4;
                }
                
                // 处理custom_text_mode字段
                cJSON *custom_text_mode = cJSON_GetObjectItem(rect_obj, "custom_text_mode");
                if (custom_text_mode && cJSON_IsBool(custom_text_mode)) {
                    bool is_custom_mode = cJSON_IsTrue(custom_text_mode);
                    bool old_custom_mode = vocab_rects[i].custom_text_mode;
                    vocab_rects[i].custom_text_mode = is_custom_mode;
                    
                    if (old_custom_mode != is_custom_mode) {
                        any_real_changes = true;
                        ESP_LOGI(TAG, "单词界面矩形 %d 自定义模式变化: %s -> %s", i, 
                                old_custom_mode ? "true" : "false", 
                                is_custom_mode ? "true" : "false");
                    }
                    
                    if (is_custom_mode) {
                        ESP_LOGI(TAG, "单词界面矩形 %d 设为自定义文本模式", i);
                    }
                }
                
                // 处理文本 - 重构以支持跨矩形移动和rect_index
                cJSON *texts_array = cJSON_GetObjectItem(rect_obj, "texts");
                if (texts_array && cJSON_IsArray(texts_array)) {
                    int web_text_count = cJSON_GetArraySize(texts_array);
                    ESP_LOGI(TAG, "单词界面矩形 %d 包含 %d 个文本元素", i, web_text_count);
                    
                    // 首先清空当前矩形的文本（以便重新分配）
                    int old_text_count = vocab_rects[i].text_count;
                    vocab_rects[i].text_count = 0;
                    memset(vocab_rects[i].texts, 0, sizeof(vocab_rects[i].texts));
                    
                    if (old_text_count > 0 && web_text_count == 0) {
                        any_real_changes = true;
                        ESP_LOGI(TAG, "单词界面矩形 %d 文本被清空 (%d -> 0)", i, old_text_count);
                    }
                    
                    // 处理每个文本元素
                    for (int j = 0; j < web_text_count && j < 4; j++) {
                        cJSON *text_obj = cJSON_GetArrayItem(texts_array, j);
                        if (text_obj) {
                            cJSON *rel_x = cJSON_GetObjectItem(text_obj, "rel_x");
                            cJSON *rel_y = cJSON_GetObjectItem(text_obj, "rel_y");
                            cJSON *content_type = cJSON_GetObjectItem(text_obj, "content_type");
                            cJSON *content = cJSON_GetObjectItem(text_obj, "content");
                            cJSON *font_size = cJSON_GetObjectItem(text_obj, "font_size");
                            cJSON *h_align = cJSON_GetObjectItem(text_obj, "h_align");
                            cJSON *v_align = cJSON_GetObjectItem(text_obj, "v_align");
                            cJSON *target_rect_index = cJSON_GetObjectItem(text_obj, "rect_index");
                            
                            // 调试：打印原始 JSON 数据
                            char* text_json_str = cJSON_Print(text_obj);
                            ESP_LOGI(TAG, "文本%d 原始JSON: %s", j, text_json_str);
                            free(text_json_str);
                            
                            if (rel_x && rel_y) {  // 只要求 rel_x 和 rel_y，content_type 可选
                                // 确定目标矩形索引（优先使用rect_index）
                                int target_rect = i; // 默认为当前矩形
                                if (target_rect_index && cJSON_IsNumber(target_rect_index)) {
                                    int requested_rect = target_rect_index->valueint;
                                    if (requested_rect >= 0 && requested_rect < 10) {
                                        target_rect = requested_rect;
                                        if (target_rect != i) {
                                            ESP_LOGI(TAG, "检测到跨矩形文本移动：从矩形%d移动到矩形%d", i, target_rect);
                                        }
                                    }
                                }
                                
                                // 确保目标矩形有空间
                                if (vocab_rects[target_rect].text_count < 4) {
                                    int target_slot = vocab_rects[target_rect].text_count;
                                    
                                    vocab_rects[target_rect].texts[target_slot].rel_x = rel_x->valuedouble;
                                    vocab_rects[target_rect].texts[target_slot].rel_y = rel_y->valuedouble;
                                    
                                    // 处理 content_type - 与 loadLayoutFromConfig() 一致
                                    if (content_type && cJSON_IsNumber(content_type)) {
                                        vocab_rects[target_rect].texts[target_slot].type = (RectContentType)content_type->valueint;
                                        ESP_LOGI(TAG, "  矩形%d 文本%d 设置 type = %d (来自 JSON 数字)", target_rect, target_slot, vocab_rects[target_rect].texts[target_slot].type);
                                    } else if (content_type && cJSON_IsString(content_type)) {
                                        // 兼容老的字符串形式
                                        const char *type_str = content_type->valuestring;
                                        if (strcmp(type_str, "WORD") == 0) {
                                            vocab_rects[target_rect].texts[target_slot].type = CONTENT_WORD;
                                        } else if (strcmp(type_str, "PHONETIC") == 0) {
                                            vocab_rects[target_rect].texts[target_slot].type = CONTENT_PHONETIC;
                                        } else if (strcmp(type_str, "DEFINITION") == 0) {
                                            vocab_rects[target_rect].texts[target_slot].type = CONTENT_DEFINITION;
                                        } else if (strcmp(type_str, "TRANSLATION") == 0) {
                                            vocab_rects[target_rect].texts[target_slot].type = CONTENT_TRANSLATION;
                                        } else {
                                            vocab_rects[target_rect].texts[target_slot].type = CONTENT_WORD;
                                        }
                                        ESP_LOGI(TAG, "  矩形%d 文本%d 设置 type = %d (来自 JSON 字符串 '%s')", target_rect, target_slot, vocab_rects[target_rect].texts[target_slot].type, type_str);
                                    } else {
                                        // JSON 中没有 content_type 字段，使用默认值 CONTENT_WORD
                                        vocab_rects[target_rect].texts[target_slot].type = CONTENT_WORD;
                                        ESP_LOGW(TAG, "  content_type 为空，设置默认 type = %d", CONTENT_WORD);
                                    }
                                    
                                    vocab_rects[target_rect].texts[target_slot].font_size = font_size ? font_size->valueint : 16;
                                    vocab_rects[target_rect].texts[target_slot].h_align = (TextAlignment)(h_align ? h_align->valueint : ALIGN_LEFT);
                                    vocab_rects[target_rect].texts[target_slot].v_align = (TextAlignment)(v_align ? v_align->valueint : ALIGN_TOP);
                                    vocab_rects[target_rect].texts[target_slot].max_width = 0;
                                    vocab_rects[target_rect].texts[target_slot].max_height = 0;
                                    
                                    vocab_rects[target_rect].text_count++;
                                    any_real_changes = true;
                                    
                                    ESP_LOGI(TAG, "文本元素已分配到矩形%d，位置槽%d", target_rect, target_slot);
                                } else {
                                    ESP_LOGW(TAG, "目标矩形%d的文本槽已满，跳过文本元素", target_rect);
                                }
                            } else {
                                ESP_LOGW(TAG, "文本元素缺少必要字段 rel_x 或 rel_y，跳过");
                            }
                        }
                    }
                } else {
                    // 如果没有文本数据但不在自定义模式，保持原有文本
                    if (!vocab_rects[i].custom_text_mode) {
                        ESP_LOGD(TAG, "矩形%d无Web文本数据，保持原有配置", i);
                    }
                }
            }
        }
        
        // 清理超出web布局范围的矩形（将它们重置为无效状态）
        for (int i = web_rect_count; i < MAX_VOCAB_RECTS; i++) {
            // 检查是否需要清理这个矩形
            if (vocab_rects[i].width > 0 || vocab_rects[i].height > 0 || 
                vocab_rects[i].text_count > 0 || vocab_rects[i].icon_count > 0) {
                
                ESP_LOGI(TAG, "清理超出范围的矩形%d", i);
                
                // 重置矩形为无效状态
                vocab_rects[i].x = 0;
                vocab_rects[i].y = 0;
                vocab_rects[i].width = 0;  // 宽度为0表示无效矩形
                vocab_rects[i].height = 0; // 高度为0表示无效矩形
                vocab_rects[i].content_type = CONTENT_CUSTOM;
                vocab_rects[i].custom_text_mode = true;
                vocab_rects[i].text_count = 0;
                vocab_rects[i].icon_count = 0;
                
                // 清理文本数组
                for (int j = 0; j < 4; j++) {
                    memset(&vocab_rects[i].texts[j], 0, sizeof(TextPositionInRect));
                }
                
                // 清理图标数组
                for (int j = 0; j < 4; j++) {
                    memset(&vocab_rects[i].icons[j], 0, sizeof(IconPositionInRect));
                }
                
                any_real_changes = true;
            }
        }
        
        // 更新屏幕管理器中的矩形数量
        updateVocabScreenRectCount(web_rect_count);
        
        if (any_real_changes) {
            ESP_LOGI(TAG, "检测到单词界面布局变化，开始保存到配置");
            saveVocabLayoutToConfig();
            ESP_LOGI(TAG, "单词界面布局已应用并保存");
        } else {
            ESP_LOGI(TAG, "无单词界面布局变化，跳过保存");
        }
    }
    
    cJSON_Delete(root);
    return true;
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
        
        for (int i = 0; i < web_rect_count && i < MAX_MAIN_RECTS; i++) {
            cJSON *rect_obj = cJSON_GetArrayItem(rects_array, i);
            if (rect_obj) {
                // 首先更新矩形的坐标和尺寸
                cJSON *x_item = cJSON_GetObjectItem(rect_obj, "x");
                cJSON *y_item = cJSON_GetObjectItem(rect_obj, "y");
                cJSON *width_item = cJSON_GetObjectItem(rect_obj, "width");
                cJSON *height_item = cJSON_GetObjectItem(rect_obj, "height");
                
                if (x_item && y_item && width_item && height_item) {
                    int new_x = x_item->valueint;
                    int new_y = y_item->valueint;
                    int new_width = width_item->valueint;
                    int new_height = height_item->valueint;
                    
                    // 检查矩形是否有变化
                    if (rects[i].x != new_x || rects[i].y != new_y || 
                        rects[i].width != new_width || rects[i].height != new_height) {
                        any_real_changes = true;
                        ESP_LOGI(TAG, "矩形 %d 尺寸/位置变化:", i);
                        ESP_LOGI(TAG, "  旧: (%d,%d) %dx%d", rects[i].x, rects[i].y, rects[i].width, rects[i].height);
                        ESP_LOGI(TAG, "  新: (%d,%d) %dx%d", new_x, new_y, new_width, new_height);
                    }
                    
                    // 更新矩形数据
                    rects[i].x = new_x;
                    rects[i].y = new_y;
                    rects[i].width = new_width;
                    rects[i].height = new_height;
                }
                
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
                
                // 处理文本
                cJSON *texts_array = cJSON_GetObjectItem(rect_obj, "texts");
                if (texts_array && cJSON_IsArray(texts_array)) {
                    int web_text_count = cJSON_GetArraySize(texts_array);
                    ESP_LOGI(TAG, "矩形 %d 包含 %d 个文本元素", i, web_text_count);
                    
                    // 清空现有文本
                    rects[i].text_count = 0;
                    
                    // 逐个解析文本
                    for (int j = 0; j < web_text_count && j < 4; j++) {
                        cJSON *text_obj = cJSON_GetArrayItem(texts_array, j);
                        if (text_obj) {
                            cJSON *rel_x = cJSON_GetObjectItem(text_obj, "rel_x");
                            cJSON *rel_y = cJSON_GetObjectItem(text_obj, "rel_y");
                            cJSON *type = cJSON_GetObjectItem(text_obj, "type");
                            cJSON *font_size = cJSON_GetObjectItem(text_obj, "font_size");
                            cJSON *h_align = cJSON_GetObjectItem(text_obj, "h_align");
                            cJSON *v_align = cJSON_GetObjectItem(text_obj, "v_align");
                            cJSON *max_width = cJSON_GetObjectItem(text_obj, "max_width");
                            cJSON *max_height = cJSON_GetObjectItem(text_obj, "max_height");
                            
                            if (rel_x && rel_y && type) {
                                rects[i].texts[j].rel_x = rel_x->valuedouble;
                                rects[i].texts[j].rel_y = rel_y->valuedouble;
                                rects[i].texts[j].type = (RectContentType)type->valueint;
                                rects[i].texts[j].font_size = font_size ? font_size->valueint : 16;
                                rects[i].texts[j].h_align = (TextAlignment)(h_align ? h_align->valueint : ALIGN_LEFT);
                                rects[i].texts[j].v_align = (TextAlignment)(v_align ? v_align->valueint : ALIGN_TOP);
                                rects[i].texts[j].max_width = max_width ? max_width->valueint : 0;
                                rects[i].texts[j].max_height = max_height ? max_height->valueint : 0;
                                
                                rects[i].text_count++;
                                any_real_changes = true;
                                
                                ESP_LOGI(TAG, "  文本 %d: type=%d, pos=(%.2f,%.2f), font=%d", 
                                        j, rects[i].texts[j].type, 
                                        rects[i].texts[j].rel_x, rects[i].texts[j].rel_y,
                                        rects[i].texts[j].font_size);
                            }
                        }
                    }
                }
            }
        }
        
        // 清理被删除的矩形（从web_rect_count到MAX_MAIN_RECTS-1）
        for (int i = web_rect_count; i < MAX_MAIN_RECTS; i++) {
            // 检查是否需要清理
            if (rects[i].width > 0 || rects[i].height > 0 || rects[i].icon_count > 0) {
                ESP_LOGI(TAG, "清理被删除的主界面矩形 %d", i);
                
                // 清空矩形基本信息
                rects[i].x = 0;
                rects[i].y = 0; 
                rects[i].width = 0;
                rects[i].height = 0;
                rects[i].icon_count = 0;
                rects[i].text_count = 0;
                
                // 清理图标数组
                for (int j = 0; j < 4; j++) {
                    memset(&rects[i].icons[j], 0, sizeof(IconPositionInRect));
                }
                
                // 清理文本数组
                for (int j = 0; j < 4; j++) {
                    memset(&rects[i].texts[j], 0, sizeof(TextPositionInRect));
                }
                
                any_real_changes = true;
            }
        }
        
        // 更新主界面矩形数量
        updateMainScreenRectCount(web_rect_count);
        
        if (any_real_changes) {
            ESP_LOGI(TAG, "检测到用户修改的布局变化");
        } else {
            ESP_LOGI(TAG, "没有检测到显著的布局变化");
        }
        
        // 保存到配置文件
        saveLayoutToConfig();
        
        // 先清空屏幕，避免重影
        ESP_LOGI(TAG, "清空屏幕以避免重影");
        clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
        
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
    
    for (int i = 0; i < rect_count; i++) {
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


// 获取当前布局信息的辅助函数（主界面）
bool getCurrentLayoutInfo(char *output, int output_size) {
    // 从ScreenManager获取主界面的矩形数据
    extern ScreenManager g_screen_manager;
    
    ESP_LOGI("WEB_LAYOUT", "========== getCurrentLayoutInfo 开始 ==========");
    ESP_LOGI("WEB_LAYOUT", "g_screen_manager地址: %p", &g_screen_manager);
    ESP_LOGI("WEB_LAYOUT", "g_screen_manager.screens地址: %p", g_screen_manager.screens);
    ESP_LOGI("WEB_LAYOUT", "g_screen_manager.screens[SCREEN_HOME]地址: %p", &g_screen_manager.screens[SCREEN_HOME]);
    
    // 获取主界面的实际矩形数量
    int main_rect_count = g_screen_manager.screens[SCREEN_HOME].rect_count;
    RectInfo* main_rects = g_screen_manager.screens[SCREEN_HOME].rects;
    
    ESP_LOGI("WEB_LAYOUT", "main_rect_count: %d", main_rect_count);
    ESP_LOGI("WEB_LAYOUT", "main_rects地址: %p", main_rects);
    
    if (!main_rects || main_rect_count <= 0) {
        ESP_LOGE("WEB_LAYOUT", "Main screen: rects is null or rect_count is 0 (count=%d)", main_rect_count);
        return false;
    }
    
    ESP_LOGI("WEB_LAYOUT", "获取主界面布局信息: %d个矩形", main_rect_count);
    bool result = getMainLayoutInfo(main_rects, main_rect_count, 0, output, output_size);
    ESP_LOGI("WEB_LAYOUT", "getMainLayoutInfo 返回: %s", result ? "true" : "false");
    ESP_LOGI("WEB_LAYOUT", "========== getCurrentLayoutInfo 结束 ==========");
    return result;
}

/**
 * @brief 获取单词界面布局信息
 */
bool getVocabLayoutInfo(char *output, int output_size) {
    extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
    int current_rect_count = getVocabScreenRectCount();
    ESP_LOGI("WEB_LAYOUT", "获取单词界面布局信息，当前矩形数量: %d", current_rect_count);
    return getMainLayoutInfo(vocab_rects, current_rect_count, 1, output, output_size);
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

// 保存单词界面布局到专用配置文件
void saveVocabLayoutToConfig() {
    extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
    
    ESP_LOGI(TAG, "开始保存单词界面布局配置");
    
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        if (state == SDState::NotPresent) {
            ESP_LOGE(TAG,"No SD Card - 无法保存配置\r\n");
        } else {
            ESP_LOGE(TAG,"SD Card Busy - 无法保存配置\r\n");
        }
        return;
    }
    
    ESP_LOGI(TAG, "SD卡状态正常，继续保存过程");

    // 保存到SD卡，使用专门的文件名（与load函数路径一致）
    String config_filename = "/vocab_layout_config.json";
    
    // 先检查文件是否存在，如果存在则删除
    if (SD.exists(config_filename.c_str())) {
        SD.remove(config_filename.c_str());
        ESP_LOGI(TAG, "删除旧的单词界面配置文件: %s", config_filename.c_str());
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    // 获取实际的矩形数量，而不是硬编码10个
    int actual_vocab_rect_count = getVocabScreenRectCount();
    ESP_LOGI(TAG, "保存单词界面配置，实际矩形数量: %d", actual_vocab_rect_count);
    
    // 保存实际的矩形数量
    cJSON_AddNumberToObject(root, "rect_count", actual_vocab_rect_count);

    // 保存矩形数组
    cJSON *rects_array = cJSON_CreateArray();
    if (rects_array) {
        // 只保存有效的矩形
        for (int i = 0; i < actual_vocab_rect_count && i < MAX_VOCAB_RECTS; i++) {
            cJSON *rect_obj = cJSON_CreateObject();
            if (rect_obj) {
                RectInfo *rect = &vocab_rects[i];
                
                cJSON_AddNumberToObject(rect_obj, "x", rect->x);
                cJSON_AddNumberToObject(rect_obj, "y", rect->y);
                cJSON_AddNumberToObject(rect_obj, "width", rect->width);
                cJSON_AddNumberToObject(rect_obj, "height", rect->height);
                cJSON_AddNumberToObject(rect_obj, "icon_count", rect->icon_count);
                cJSON_AddBoolToObject(rect_obj, "custom_text_mode", rect->custom_text_mode);
                
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
                
                // 保存文本信息
                cJSON_AddNumberToObject(rect_obj, "text_count", rect->text_count);
                cJSON *texts_array = cJSON_CreateArray();
                if (texts_array) {
                    for (int j = 0; j < rect->text_count; j++) {
                        cJSON *text_obj = cJSON_CreateObject();
                        if (text_obj) {
                            TextPositionInRect *text = &rect->texts[j];
                            cJSON_AddNumberToObject(text_obj, "content_type", text->type);
                            cJSON_AddStringToObject(text_obj, "content", ""); // 空内容，运行时填充
                            cJSON_AddNumberToObject(text_obj, "rel_x", text->rel_x);
                            cJSON_AddNumberToObject(text_obj, "rel_y", text->rel_y);
                            cJSON_AddNumberToObject(text_obj, "font_size", text->font_size);
                            cJSON_AddNumberToObject(text_obj, "h_align", text->h_align);
                            cJSON_AddNumberToObject(text_obj, "v_align", text->v_align);
                            cJSON_AddItemToArray(texts_array, text_obj);
                        }
                    }
                    cJSON_AddItemToObject(rect_obj, "texts", texts_array);
                }
                
                cJSON_AddItemToArray(rects_array, rect_obj);
            }
        }
        cJSON_AddItemToObject(root, "rectangles", rects_array);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        ESP_LOGI(TAG, "生成JSON字符串，长度: %d", strlen(json_str));
        ESP_LOGD(TAG, "JSON内容预览: %.100s...", json_str);
        
        File config_file = SD.open(config_filename.c_str(), FILE_WRITE);
        if (config_file) {
            size_t bytes_written = config_file.write((const uint8_t *)json_str, strlen(json_str));
            config_file.close();
            ESP_LOGI(TAG, "单词界面布局配置已保存到 %s，写入字节数: %d", 
                    config_filename.c_str(), bytes_written);
            
            // 验证文件是否真的创建了
            if (SD.exists(config_filename.c_str())) {
                ESP_LOGI(TAG, "配置文件创建成功，文件存在");
            } else {
                ESP_LOGE(TAG, "配置文件创建失败，文件不存在！");
            }
        } else {
            ESP_LOGE(TAG, "无法创建配置文件: %s", config_filename.c_str());
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "JSON生成失败");
    }

    cJSON_Delete(root);
}

// 从配置文件加载单词界面布局
bool loadVocabLayoutFromConfig() {
    extern RectInfo vocab_rects[MAX_VOCAB_RECTS];  // 声明外部单词界面矩形数组
    
    String config_filename = "/vocab_layout_config.json";
    
    if (!SD.exists(config_filename)) {
        ESP_LOGI(TAG, "单词界面布局配置文件不存在: %s", config_filename.c_str());
        return false;
    }

    File config_file = SD.open(config_filename, FILE_READ);
    if (!config_file) {
        ESP_LOGE(TAG, "无法打开单词界面布局配置文件: %s", config_filename.c_str());
        return false;
    }

    String json_content = config_file.readString();
    config_file.close();

    if (json_content.length() == 0) {
        ESP_LOGE(TAG, "单词界面布局配置文件为空");
        return false;
    }

    cJSON *root = cJSON_Parse(json_content.c_str());
    if (!root) {
        ESP_LOGE(TAG, "解析单词界面布局配置JSON失败");
        return false;
    }

    cJSON *rects_count_obj = cJSON_GetObjectItem(root, "rect_count");
    if (rects_count_obj) {
        int new_rect_count = rects_count_obj->valueint;
        ESP_LOGI(TAG, "配置文件中的单词界面矩形数量: %d", new_rect_count);
        
        // 限制最大矩形数量为10（数组大小限制）
        if (new_rect_count > MAX_VOCAB_RECTS) {
            ESP_LOGW(TAG, "矩形数量 %d 超过最大限制10，将限制为10", new_rect_count);
            new_rect_count = MAX_VOCAB_RECTS;
        }
        
        // 如果数量为0或负数，跳过加载
        if (new_rect_count <= 0) {
            ESP_LOGW(TAG, "无效的矩形数量: %d，跳过加载", new_rect_count);
            cJSON_Delete(root);
            return false;
        }

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
                    cJSON *custom_text_mode = cJSON_GetObjectItem(rect_obj, "custom_text_mode");
                    
                    if (x && y && width && height && icon_count) {
                        vocab_rects[i].x = x->valueint;
                        vocab_rects[i].y = y->valueint;
                        vocab_rects[i].width = width->valueint;
                        vocab_rects[i].height = height->valueint;
                        vocab_rects[i].icon_count = 0;
                        
                        // 加载 custom_text_mode 字段
                        if (custom_text_mode && cJSON_IsBool(custom_text_mode)) {
                            vocab_rects[i].custom_text_mode = cJSON_IsTrue(custom_text_mode);
                        } else {
                            // 如果配置文件中没有此字段，根据矩形索引设置默认值
                            vocab_rects[i].custom_text_mode = (i == 2 || i == 4 || i == 5 || i == 8);
                        }
                        
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
                                        vocab_rects[i].icons[j].icon_index = icon_index->valueint;
                                        vocab_rects[i].icons[j].rel_x = rel_x->valuedouble;
                                        vocab_rects[i].icons[j].rel_y = rel_y->valuedouble;
                                        vocab_rects[i].icon_count++;
                                    }
                                }
                            }
                        }
                        
                        // 解析文本信息
                        cJSON *text_count_obj = cJSON_GetObjectItem(rect_obj, "text_count");
                        if (text_count_obj && text_count_obj->valueint > 0) {
                            // 只有当配置文件中有文本信息时才覆盖默认值
                            vocab_rects[i].text_count = text_count_obj->valueint;
                            
                            cJSON *texts_array = cJSON_GetObjectItem(rect_obj, "texts");
                            if (texts_array && cJSON_IsArray(texts_array)) {
                                int text_array_size = cJSON_GetArraySize(texts_array);
                                int texts_to_load = (text_array_size < vocab_rects[i].text_count && text_array_size < 4) ? text_array_size : (vocab_rects[i].text_count < 4 ? vocab_rects[i].text_count : 4);
                                
                                for (int j = 0; j < texts_to_load; j++) {
                                    cJSON *text_obj = cJSON_GetArrayItem(texts_array, j);
                                    if (text_obj) {
                                        cJSON *content_type = cJSON_GetObjectItem(text_obj, "content_type");
                                        cJSON *rel_x = cJSON_GetObjectItem(text_obj, "rel_x");
                                        cJSON *rel_y = cJSON_GetObjectItem(text_obj, "rel_y");
                                        cJSON *font_size = cJSON_GetObjectItem(text_obj, "font_size");
                                        cJSON *h_align = cJSON_GetObjectItem(text_obj, "h_align");
                                        cJSON *v_align = cJSON_GetObjectItem(text_obj, "v_align");
                                        
                                        // 调试：打印原始 JSON 数据
                                        char* text_json_str = cJSON_Print(text_obj);
                                        ESP_LOGI(TAG, "[VOCAB LOAD] 文本%d 原始JSON: %s", j, text_json_str);
                                        free(text_json_str);
                                        
                                        // 调试：检查 content_type 字段
                                        if (content_type) {
                                            ESP_LOGI(TAG, "[VOCAB LOAD]   content_type 存在, 类型:%d, 值:%d", 
                                                    content_type->type, content_type->valueint);
                                        } else {
                                            ESP_LOGW(TAG, "[VOCAB LOAD]   content_type 字段为 NULL!");
                                        }
                                        
                                        // 设置 content_type（支持数字和字符串两种格式）
                                        if (content_type && cJSON_IsNumber(content_type)) {
                                            vocab_rects[i].texts[j].type = (RectContentType)content_type->valueint;
                                            ESP_LOGI(TAG, "[VOCAB LOAD]   矩形%d 文本%d 设置 type = %d (来自 JSON 数字)", i, j, vocab_rects[i].texts[j].type);
                                        } else if (content_type && cJSON_IsString(content_type)) {
                                            // 将字符串转换为枚举
                                            const char *type_str = content_type->valuestring;
                                            if (strcmp(type_str, "WORD") == 0) {
                                                vocab_rects[i].texts[j].type = CONTENT_WORD;
                                            } else if (strcmp(type_str, "PHONETIC") == 0) {
                                                vocab_rects[i].texts[j].type = CONTENT_PHONETIC;
                                            } else if (strcmp(type_str, "DEFINITION") == 0) {
                                                vocab_rects[i].texts[j].type = CONTENT_DEFINITION;
                                            } else if (strcmp(type_str, "TRANSLATION") == 0) {
                                                vocab_rects[i].texts[j].type = CONTENT_TRANSLATION;
                                            } else {
                                                vocab_rects[i].texts[j].type = CONTENT_NONE;
                                            }
                                            ESP_LOGI(TAG, "[VOCAB LOAD]   矩形%d 文本%d 设置 type = %d (来自 JSON 字符串 '%s')", i, j, vocab_rects[i].texts[j].type, type_str);
                                        } else {
                                            // JSON 中没有 content_type 字段，保持初始化时的值（已经是 CONTENT_WORD）
                                            ESP_LOGW(TAG, "[VOCAB LOAD]   content_type 为空，type 保持默认 = %d", vocab_rects[i].texts[j].type);
                                        }
                                        
                                        if (rel_x) {
                                            vocab_rects[i].texts[j].rel_x = rel_x->valuedouble;
                                        }
                                        if (rel_y) {
                                            vocab_rects[i].texts[j].rel_y = rel_y->valuedouble;
                                        }
                                        if (font_size) {
                                            vocab_rects[i].texts[j].font_size = font_size->valueint;
                                        }
                                        if (h_align && cJSON_IsNumber(h_align)) {
                                            vocab_rects[i].texts[j].h_align = (TextAlignment)h_align->valueint;
                                        } else if (h_align && cJSON_IsString(h_align)) {
                                            const char *align_str = h_align->valuestring;
                                            if (strcmp(align_str, "left") == 0) {
                                                vocab_rects[i].texts[j].h_align = ALIGN_LEFT;
                                            } else if (strcmp(align_str, "center") == 0) {
                                                vocab_rects[i].texts[j].h_align = ALIGN_CENTER;
                                            } else if (strcmp(align_str, "right") == 0) {
                                                vocab_rects[i].texts[j].h_align = ALIGN_RIGHT;
                                            } else {
                                                vocab_rects[i].texts[j].h_align = ALIGN_LEFT;
                                            }
                                        }
                                        if (v_align && cJSON_IsNumber(v_align)) {
                                            vocab_rects[i].texts[j].v_align = (TextAlignment)v_align->valueint;
                                        } else if (v_align && cJSON_IsString(v_align)) {
                                            const char *align_str = v_align->valuestring;
                                            if (strcmp(align_str, "top") == 0) {
                                                vocab_rects[i].texts[j].v_align = ALIGN_TOP;
                                            } else if (strcmp(align_str, "middle") == 0) {
                                                vocab_rects[i].texts[j].v_align = ALIGN_MIDDLE;
                                            } else if (strcmp(align_str, "bottom") == 0) {
                                                vocab_rects[i].texts[j].v_align = ALIGN_BOTTOM;
                                            } else {
                                                vocab_rects[i].texts[j].v_align = ALIGN_TOP;
                                            }
                                        }
                                    }
                                }
                                vocab_rects[i].text_count = texts_to_load; // 更新为实际加载的文本数量
                            }
                        }
                        // 如果配置文件中没有文本信息，保留vocab_rects的默认text_count值
                        // 不再强制设置为0
                    }
                }
            }
            
            ESP_LOGI(TAG, "单词界面布局配置加载成功: %d 个矩形", rects_to_load);
            
            // 更新屏幕管理器中的矩形数量
            updateVocabScreenRectCount(rects_to_load);
            
            cJSON_Delete(root);
            return true;
        }
    }

    cJSON_Delete(root);
    return false;
}
// ==================== 焦点矩形配置管理 ====================

/**
 * @brief 保存可焦点矩形列表到配置文件
 * @param rect_indices 矩形索引数组
 * @param count 数组长度
 * @param screen_type 界面类型："main" 或 "vocab"
 */
void saveFocusableRectsToConfig(int* rect_indices, int count, const char* screen_type) {
    if (!rect_indices || count <= 0 || count > MAX_FOCUSABLE_RECTS) {
        ESP_LOGW(TAG, "无效的焦点矩形列表参数");
        return;
    }
    
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        ESP_LOGE(TAG, "SD卡不可用，无法保存焦点矩形配置");
        return;
    }
    
    // 根据界面类型选择配置文件
    String config_filename;
    if (strcmp(screen_type, "main") == 0) {
        config_filename = "/main_focusable_rects_config.json";
    } else {
        config_filename = "/vocab_focusable_rects_config.json";
    }
    
    ESP_LOGI(TAG, "保存%s界面焦点配置到: %s", screen_type, config_filename.c_str());
    
    // 删除旧文件
    if (SD.exists(config_filename.c_str())) {
        SD.remove(config_filename.c_str());
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) return;
    
    // 保存矩形数量
    cJSON_AddNumberToObject(root, "count", count);
    
    // 保存矩形索引数组
    cJSON *indices_array = cJSON_CreateArray();
    if (indices_array) {
        for (int i = 0; i < count; i++) {
            cJSON_AddItemToArray(indices_array, cJSON_CreateNumber(rect_indices[i]));
        }
        cJSON_AddItemToObject(root, "focusable_indices", indices_array);
    }
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        File config_file = SD.open(config_filename.c_str(), FILE_WRITE);
        if (config_file) {
            config_file.write((const uint8_t *)json_str, strlen(json_str));
            config_file.close();
            ESP_LOGI(TAG, "焦点矩形配置已保存: %d个矩形", count);
        }
        free(json_str);
    }
    
    cJSON_Delete(root);
}

/**
 * @brief 从配置文件加载可焦点矩形列表
 * @return 是否成功加载
 */
/**
 * @brief 从 SD 卡加载焦点矩形配置
 * @param screen_type 界面类型："main" 或 "vocab"
 * @return true 成功，false 失败
 */
bool loadFocusableRectsFromConfig(const char* screen_type) {
    // 根据界面类型选择配置文件
    String config_filename;
    if (strcmp(screen_type, "main") == 0) {
        config_filename = "/main_focusable_rects_config.json";
    } else {
        config_filename = "/vocab_focusable_rects_config.json";
    }
    
    ESP_LOGI(TAG, "加载%s界面焦点配置从: %s", screen_type, config_filename.c_str());
    
    if (!SD.exists(config_filename)) {
        ESP_LOGI(TAG, "焦点矩形配置文件不存在，使用默认配置");
        return false;
    }
    
    File config_file = SD.open(config_filename.c_str(), FILE_READ);
    if (!config_file) {
        ESP_LOGE(TAG, "无法打开焦点矩形配置文件");
        return false;
    }
    
    String json_content = config_file.readString();
    config_file.close();
    
    if (json_content.length() == 0) {
        ESP_LOGE(TAG, "焦点矩形配置文件为空");
        return false;
    }
    
    cJSON *root = cJSON_Parse(json_content.c_str());
    if (!root) {
        ESP_LOGE(TAG, "解析焦点矩形配置JSON失败");
        return false;
    }
    
    cJSON *count_obj = cJSON_GetObjectItem(root, "count");
    cJSON *indices_array = cJSON_GetObjectItem(root, "focusable_indices");
    
    if (count_obj && indices_array && cJSON_IsArray(indices_array)) {
        int count = count_obj->valueint;
        int array_size = cJSON_GetArraySize(indices_array);
        
        if (count > 0 && count <= MAX_FOCUSABLE_RECTS && count == array_size) {
            int rect_indices[MAX_FOCUSABLE_RECTS];
            
            for (int i = 0; i < count; i++) {
                cJSON *index_obj = cJSON_GetArrayItem(indices_array, i);
                if (index_obj) {
                    rect_indices[i] = index_obj->valueint;
                }
            }
            
            // 调用焦点系统设置函数
            setFocusableRects(rect_indices, count);
            
            ESP_LOGI(TAG, "焦点矩形配置加载成功: %d个矩形", count);
            cJSON_Delete(root);
            return true;
        }
    }
    
    cJSON_Delete(root);
    return false;
}

/**
 * @brief 保存子数组配置到SD卡
 * @param json_data JSON字符串数据
 * @param screen_type 界面类型："main" 或 "vocab"
 */
void saveSubArrayConfigToSD(const char* json_data, const char* screen_type) {
    if (!json_data) {
        ESP_LOGE(TAG, "子数组配置数据为空");
        return;
    }
    
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        if (state == SDState::NotPresent) {
            ESP_LOGI(TAG, "No SD Card");
        } else {
            ESP_LOGI(TAG, "SD Card Busy");
        }
        return;
    }
    
    // 根据界面类型选择配置文件
    String config_filename;
    if (strcmp(screen_type, "main") == 0) {
        config_filename = "/main_subarray_config.json";
    } else {
        config_filename = "/vocab_subarray_config.json";
    }
    
    ESP_LOGI(TAG, "保存%s界面子数组配置到: %s", screen_type, config_filename.c_str());
    
    // 删除旧文件
    if (SD.exists(config_filename.c_str())) {
        SD.remove(config_filename.c_str());
        ESP_LOGI(TAG, "删除旧的子数组配置文件: %s", config_filename.c_str());
    }
    
    File config_file = SD.open(config_filename.c_str(), FILE_WRITE);
    if (config_file) {
        config_file.write((const uint8_t *)json_data, strlen(json_data));
        config_file.close();
        ESP_LOGI(TAG, "子数组配置已保存到SD卡");
    } else {
        ESP_LOGE(TAG, "无法创建子数组配置文件");
    }
    
    set_sd_state(SDState::Idle);
}

/**
 * @brief 从SD卡加载子数组配置
 * @param output 输出缓冲区
 * @param output_size 缓冲区大小
 * @param screen_type 界面类型："main" 或 "vocab"
 * @return 是否成功加载
 */
bool loadSubArrayConfigFromSD(char* output, int output_size, const char* screen_type) {
    if (!output || output_size <= 0) {
        ESP_LOGE(TAG, "无效的输出缓冲区");
        return false;
    }
    
    output[0] = '\0';
    
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        if (state == SDState::NotPresent) {
            ESP_LOGI(TAG, "No SD Card");
        } else {
            ESP_LOGI(TAG, "SD Card Busy");
        }
        return false;
    }
    
    // 根据界面类型选择配置文件
    String config_filename;
    if (strcmp(screen_type, "main") == 0) {
        config_filename = "/main_subarray_config.json";
    } else {
        config_filename = "/vocab_subarray_config.json";
    }
    
    ESP_LOGI(TAG, "加载%s界面子数组配置从: %s", screen_type, config_filename.c_str());
    
    if (!SD.exists(config_filename.c_str())) {
        ESP_LOGI(TAG, "子数组配置文件不存在");
        set_sd_state(SDState::Idle);
        return false;
    }
    
    File config_file = SD.open(config_filename.c_str(), FILE_READ);
    if (!config_file) {
        ESP_LOGE(TAG, "无法打开子数组配置文件");
        set_sd_state(SDState::Idle);
        return false;
    }
    
    String json_content = config_file.readString();
    config_file.close();
    set_sd_state(SDState::Idle);
    
    if (json_content.length() == 0 || json_content.length() >= output_size) {
        ESP_LOGE(TAG, "子数组配置文件为空或过大");
        return false;
    }
    
    strncpy(output, json_content.c_str(), output_size - 1);
    output[output_size - 1] = '\0';
    
    ESP_LOGI(TAG, "子数组配置已从SD卡加载，长度: %d", json_content.length());
    return true;
}

/**
 * @brief 从SD卡加载子数组配置并应用到焦点系统
 * @param screen_type 界面类型："main" 或 "vocab"
 * @return 是否成功加载并应用
 */
bool loadAndApplySubArrayConfig(const char* screen_type) {
    char config_buffer[2048];
    
    if (!loadSubArrayConfigFromSD(config_buffer, sizeof(config_buffer), screen_type)) {
        ESP_LOGI(TAG, "无法加载%s界面子数组配置", screen_type);
        return false;
    }
    
    if (strlen(config_buffer) == 0) {
        ESP_LOGI(TAG, "子数组配置为空");
        return false;
    }
    
    cJSON *root = cJSON_Parse(config_buffer);
    if (!root) {
        ESP_LOGE(TAG, "解析子数组配置JSON失败");
        return false;
    }
    
    cJSON *sub_arrays = cJSON_GetObjectItem(root, "sub_arrays");
    if (!sub_arrays || !cJSON_IsArray(sub_arrays)) {
        ESP_LOGE(TAG, "子数组配置格式错误");
        cJSON_Delete(root);
        return false;
    }
    
    int array_size = cJSON_GetArraySize(sub_arrays);
    ESP_LOGI(TAG, "开始应用子数组配置，共 %d 个条目", array_size);
    
    for (int i = 0; i < array_size; i++) {
        cJSON *item = cJSON_GetArrayItem(sub_arrays, i);
        if (!item) continue;
        
        cJSON *parent_idx_obj = cJSON_GetObjectItem(item, "parent_index");
        cJSON *sub_indices_array = cJSON_GetObjectItem(item, "sub_indices");
        cJSON *sub_count_obj = cJSON_GetObjectItem(item, "sub_count");
        
        if (!parent_idx_obj || !sub_indices_array || !sub_count_obj) continue;
        
        int parent_array_index = parent_idx_obj->valueint;  // 母数组中的位置索引（0,1,2...）
        int sub_count = sub_count_obj->valueint;
        
        ESP_LOGI(TAG, "处理子数组配置 - 母数组位置: %d, 子数组数量: %d", parent_array_index, sub_count);
        
        if (sub_count <= 0 || sub_count > MAX_FOCUSABLE_RECTS) {
            ESP_LOGW(TAG, "无效的子数组数量: %d", sub_count);
            continue;
        }
        
        int sub_indices[MAX_FOCUSABLE_RECTS];
        for (int j = 0; j < sub_count; j++) {
            cJSON *idx_obj = cJSON_GetArrayItem(sub_indices_array, j);
            if (idx_obj) {
                sub_indices[j] = idx_obj->valueint;
            }
        }
        
        // 调用函数设置子数组（使用母数组位置索引，直接调用setSubArrayForParent）
        setSubArrayForParent(parent_array_index, sub_indices, sub_count);
    }
    
    cJSON_Delete(root);
    ESP_LOGI(TAG, "✓ 子数组配置已应用到焦点系统");
    return true;
}

// ======== 导出可用图标列表 ========
/**
 * @brief 导出所有可用图标的列表（包括索引和文件名）
 * @return 返回JSON格式的图标列表字符串，调用者需要释放该指针
 */
char* exportAvailableIcons() {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "无法创建JSON根对象");
        return NULL;
    }
    
    // 创建图标数组
    cJSON *icons_array = cJSON_CreateArray();
    if (!icons_array) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "无法创建图标数组");
        return NULL;
    }
    
    // 遍历所有可用图标
    int icon_count = sizeof(g_available_icons) / sizeof(IconInfo);
    for (int i = 0; i < icon_count; i++) {
        cJSON *icon_obj = cJSON_CreateObject();
        if (icon_obj) {
            // 添加图标索引
            cJSON_AddNumberToObject(icon_obj, "index", i);
            
            // 添加文件名（来自components/resource/icon/文件夹）
            if (i < sizeof(g_icon_filenames) / sizeof(const char*)) {
                cJSON_AddStringToObject(icon_obj, "filename", g_icon_filenames[i]);
            } else {
                cJSON_AddStringToObject(icon_obj, "filename", "unknown.jpg");
            }
            
            // 添加图标尺寸信息
            cJSON_AddNumberToObject(icon_obj, "width", g_available_icons[i].width);
            cJSON_AddNumberToObject(icon_obj, "height", g_available_icons[i].height);
            
            // 添加图标资源路径
            cJSON_AddStringToObject(icon_obj, "resource_path", "/sd/resource/icon");
            
            cJSON_AddItemToArray(icons_array, icon_obj);
        }
    }
    
    cJSON_AddItemToObject(root, "icons", icons_array);
    cJSON_AddNumberToObject(root, "total_count", icon_count);
    
    // 转换为JSON字符串
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    ESP_LOGI(TAG, "✓ 导出了 %d 个可用图标", icon_count);
    return json_str;
}


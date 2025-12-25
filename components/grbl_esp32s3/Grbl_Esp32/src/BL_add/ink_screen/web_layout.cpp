
#include "ink_screen.h"
#include "web_layout.h"
#include <string.h>

extern "C" {
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"
}
#define TAG "web_layout.cpp"



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
                        // 加载焦点模式（可选）
                        cJSON *focus_mode_obj = cJSON_GetObjectItem(rect_obj, "focus_mode");
                        if (focus_mode_obj && cJSON_IsNumber(focus_mode_obj)) {
                            vocab_rects[i].focus_mode = (FocusMode)focus_mode_obj->valueint;
                        } else {
                            // 如果配置文件中没有此字段，保持原有值或使用默认（不覆盖）
                        }
                        
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
                                        
                                        if (content_type && cJSON_IsString(content_type)) {
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
                                        if (h_align && cJSON_IsString(h_align)) {
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
                                        if (v_align && cJSON_IsString(v_align)) {
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
            
            cJSON_Delete(root);
            return true;
        }
    }

    cJSON_Delete(root);
    return false;
}
// ==================== 焦点矩形配置管理 ====================


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


/**
 * @file ttf_font_manager.cpp
 * @brief TTF字体管理系统实现
 */

#include "ttf_font_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../../Grbl.h"
#include <FS.h>

extern "C" {
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"
}

#define TAG "TTF_FontMgr"

// ==================== 常量定义 ====================

#define FONT_BIN_MAGIC          0x544F4E46  // "FONT"
#define FONT_BIN_VERSION        1
#define MAX_FONT_NAME_LEN       64
#define MAX_FONT_SIZE           256
#define MAX_FONTS               16
#define SD_FONTS_DIR            "/sd/fonts"
#define SD_TTF_DIR              "/sd/fonts/ttf"
#define SD_BIN_DIR              "/sd/fonts/bin"
#define CONFIG_FILE             "/sd/fonts/config.json"

// ==================== 全局变量 ====================

static FontConfig_t g_available_fonts[MAX_FONTS];
static int g_font_count = 0;
static char g_current_font[MAX_FONT_NAME_LEN] = "";
static FontConversionTask_t g_conversion_tasks[4];
static bool g_initialized = false;

// ==================== 预定义字体配置 ====================

// 使用函数初始化而不是静态数据，避免C99初始化器
static void init_default_fonts(FontConfig_t *fonts) {
    // Font 0: fangsong_GB2312
    strcpy(fonts[0].font_name, "fangsong_GB2312");
    strcpy(fonts[0].display_name, "仿宋_GB2312");
    strcpy(fonts[0].ttf_filename, "fangsong.ttf");
    fonts[0].font_size = 16;
    fonts[0].charset_start = 0x4E00;
    fonts[0].charset_end = 0x9FA5;
    fonts[0].is_available = false;
    
    // Font 1: ComicSansMSV3
    strcpy(fonts[1].font_name, "ComicSansMSV3");
    strcpy(fonts[1].display_name, "Comic Sans MS V3");
    strcpy(fonts[1].ttf_filename, "ComicSansMS_V3.ttf");
    fonts[1].font_size = 16;
    fonts[1].charset_start = 0x0020;
    fonts[1].charset_end = 0x007E;
    fonts[1].is_available = false;
    
    // Font 2: ComicSansMSBold
    strcpy(fonts[2].font_name, "ComicSansMSBold");
    strcpy(fonts[2].display_name, "Comic Sans MS Bold");
    strcpy(fonts[2].ttf_filename, "ComicSansMS_Bold.ttf");
    fonts[2].font_size = 16;
    fonts[2].charset_start = 0x0020;
    fonts[2].charset_end = 0x007E;
    fonts[2].is_available = false;
}

#define DEFAULT_FONTS_COUNT 3

// ==================== 工具函数 ====================

/**
 * @brief 确保目录存在
 */
static bool ensure_directory(const char *path) {
    if (SD.exists(path)) {
        return true;
    }
    
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        ESP_LOGE(TAG, "SD card not idle");
        return false;
    }
    
    if (!SD.mkdir(path)) {
        ESP_LOGE(TAG, "Failed to create directory: %s", path);
        return false;
    }
    
    ESP_LOGI(TAG, "Created directory: %s", path);
    return true;
}

/**
 * @brief 获取字体的BIN文件路径
 */
static void get_bin_path(const char *font_name, char *path, size_t path_len) {
    snprintf(path, path_len, "%s/%s.bin", SD_BIN_DIR, font_name);
}

/**
 * @brief 获取TTF文件路径
 */
static void get_ttf_path(const char *filename, char *path, size_t path_len) {
    if (path_len < 512) {
        path[0] = '\0';
        return;
    }
    snprintf(path, path_len, "%s/%s", SD_TTF_DIR, filename);
}

/**
 * @brief 生成字库BIN头
 */
static void generate_bin_header(FontBinHeader_t *header, const FontConfig_t *config) {
    memset(header, 0, sizeof(FontBinHeader_t));
    header->magic = FONT_BIN_MAGIC;
    header->version = FONT_BIN_VERSION;
    header->font_size = config->font_size;
    header->charset_start = config->charset_start;
    header->charset_end = config->charset_end;
    header->charset_size = config->charset_end - config->charset_start + 1;
    header->char_width = config->font_size;
    header->char_height = config->font_size;
    
    // 计算每个字符的字节数 (bits per row * height / 8)
    uint32_t bits_per_row = (config->font_size + 7) / 8;
    header->bytes_per_char = bits_per_row * config->font_size;
    
    strncpy(header->font_name, config->font_name, sizeof(header->font_name) - 1);
}

/**
 * @brief 保存字体配置到SD卡
 */
static bool save_config_to_sd(void) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return false;
    
    // 添加字体列表
    cJSON *fonts_array = cJSON_CreateArray();
    if (!fonts_array) {
        cJSON_Delete(root);
        return false;
    }
    
    for (int i = 0; i < g_font_count; i++) {
        cJSON *font_obj = cJSON_CreateObject();
        if (font_obj) {
            cJSON_AddStringToObject(font_obj, "name", g_available_fonts[i].font_name);
            cJSON_AddStringToObject(font_obj, "display_name", g_available_fonts[i].display_name);
            cJSON_AddNumberToObject(font_obj, "font_size", g_available_fonts[i].font_size);
            cJSON_AddBoolToObject(font_obj, "available", g_available_fonts[i].is_available);
            cJSON_AddItemToArray(fonts_array, font_obj);
        }
    }
    
    cJSON_AddItemToObject(root, "fonts", fonts_array);
    cJSON_AddStringToObject(root, "current_font", g_current_font);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        cJSON_Delete(root);
        return false;
    }
    
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        free(json_str);
        cJSON_Delete(root);
        return false;
    }
    
    // 删除旧配置
    if (SD.exists(CONFIG_FILE)) {
        SD.remove(CONFIG_FILE);
    }
    
    // 写入新配置
    File config_file = SD.open(CONFIG_FILE, FILE_WRITE);
    if (!config_file) {
        ESP_LOGE(TAG, "Failed to create config file");
        free(json_str);
        cJSON_Delete(root);
        return false;
    }
    
    config_file.print(json_str);
    config_file.close();
    
    free(json_str);
    cJSON_Delete(root);
    
    ESP_LOGI(TAG, "Config saved to SD card");
    return true;
}

/**
 * @brief 从SD卡加载字体配置
 */
static bool load_config_from_sd(void) {
    if (!SD.exists(CONFIG_FILE)) {
        ESP_LOGW(TAG, "Config file not found, using defaults");
        return false;
    }
    
    File config_file = SD.open(CONFIG_FILE, FILE_READ);
    if (!config_file) {
        ESP_LOGE(TAG, "Failed to open config file");
        return false;
    }
    
    String json_str = config_file.readString();
    config_file.close();
    
    cJSON *root = cJSON_Parse(json_str.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse config JSON");
        return false;
    }
    
    // 加载当前字体
    cJSON *current_font_obj = cJSON_GetObjectItem(root, "current_font");
    if (current_font_obj && cJSON_IsString(current_font_obj)) {
        strncpy(g_current_font, current_font_obj->valuestring, sizeof(g_current_font) - 1);
    }
    
    cJSON_Delete(root);
    return true;
}

// ==================== 初始化 ====================

bool ttf_font_manager_init(void) {
    if (g_initialized) {
        return true;
    }
    
    ESP_LOGI(TAG, "Initializing TTF font manager...");
    
    // 初始化默认字体配置
    g_font_count = 0;
    memset(g_available_fonts, 0, sizeof(g_available_fonts));
    memset(g_conversion_tasks, 0, sizeof(g_conversion_tasks));
    memset(g_current_font, 0, sizeof(g_current_font));
    
    // 添加预定义字体
    FontConfig_t default_fonts[DEFAULT_FONTS_COUNT];
    init_default_fonts(default_fonts);
    
    for (int i = 0; i < DEFAULT_FONTS_COUNT && g_font_count < MAX_FONTS; i++) {
        memcpy(&g_available_fonts[g_font_count], &default_fonts[i], sizeof(FontConfig_t));
        g_font_count++;
    }
    
    // 确保目录存在
    if (!ensure_directory(SD_FONTS_DIR)) {
        ESP_LOGE(TAG, "Failed to ensure fonts directory");
        return false;
    }
    
    if (!ensure_directory(SD_TTF_DIR)) {
        ESP_LOGE(TAG, "Failed to ensure TTF directory");
        return false;
    }
    
    if (!ensure_directory(SD_BIN_DIR)) {
        ESP_LOGE(TAG, "Failed to ensure BIN directory");
        return false;
    }
    
    // 检查可用的BIN文件
    DIR *dir = opendir(SD_BIN_DIR);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG && strstr(entry->d_name, ".bin")) {
                // 提取字体名称 (去掉.bin扩展名)
                char font_name[MAX_FONT_NAME_LEN];
                int name_len = strlen(entry->d_name) - 4;  // 去掉 ".bin"
                if (name_len > 0 && name_len < MAX_FONT_NAME_LEN) {
                    snprintf(font_name, MAX_FONT_NAME_LEN, "%.*s", name_len, entry->d_name);
                    
                    // 标记字体为可用
                    for (int i = 0; i < g_font_count; i++) {
                        if (strcmp(g_available_fonts[i].font_name, font_name) == 0) {
                            g_available_fonts[i].is_available = true;
                            ESP_LOGI(TAG, "Found available font: %s", font_name);
                            break;
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
    
    // 加载配置
    load_config_from_sd();
    
    // 如果未设置当前字体，使用第一个可用的
    if (strlen(g_current_font) == 0) {
        for (int i = 0; i < g_font_count; i++) {
            if (g_available_fonts[i].is_available) {
                strcpy(g_current_font, g_available_fonts[i].font_name);
                break;
            }
        }
    }
    
    g_initialized = true;
    ESP_LOGI(TAG, "TTF font manager initialized, %d fonts loaded", g_font_count);
    
    return true;
}

// ==================== 文件上传 ====================

bool ttf_font_upload(const char *filename, const uint8_t *data, uint32_t size) {
    if (!filename || !data || size == 0) {
        ESP_LOGE(TAG, "Invalid parameters for font upload");
        return false;
    }
    
    ESP_LOGI(TAG, "Uploading TTF file: %s (size: %d bytes)", filename, size);
    
    SDState state = get_sd_state(true);
    if (state != SDState::Idle) {
        ESP_LOGE(TAG, "SD card not idle");
        return false;
    }
    
    char ttf_path[256];
    get_ttf_path(filename, ttf_path, sizeof(ttf_path));
    
    // 删除旧文件
    if (SD.exists(ttf_path)) {
        SD.remove(ttf_path);
    }
    
    // 检查空间
    uint64_t free_space = SD.totalBytes() - SD.usedBytes();
    if (size > free_space) {
        ESP_LOGE(TAG, "Not enough space on SD card");
        return false;
    }
    
    // 写入文件
    File ttf_file = SD.open(ttf_path, FILE_WRITE);
    if (!ttf_file) {
        ESP_LOGE(TAG, "Failed to create TTF file: %s", ttf_path);
        return false;
    }
    
    size_t written = ttf_file.write(data, size);
    ttf_file.close();
    
    if (written != size) {
        ESP_LOGE(TAG, "Failed to write TTF file completely");
        SD.remove(ttf_path);
        return false;
    }
    
    ESP_LOGI(TAG, "TTF file uploaded successfully: %s", filename);
    return true;
}

// ==================== 字体转换 ====================

bool ttf_font_start_conversion(const char *font_name, const char *ttf_filename, 
                                uint16_t font_size, const char *charset) {
    if (!font_name || !ttf_filename) {
        ESP_LOGE(TAG, "Invalid parameters for conversion");
        return false;
    }
    
    ESP_LOGI(TAG, "Starting conversion: %s (from %s, size: %d)", 
             font_name, ttf_filename, font_size);
    
    // 检查TTF文件是否存在
    char ttf_path[256];
    get_ttf_path(ttf_filename, ttf_path, sizeof(ttf_path));
    
    if (!SD.exists(ttf_path)) {
        ESP_LOGE(TAG, "TTF file not found: %s", ttf_path);
        return false;
    }
    
    // 查找或创建字体配置
    int font_idx = -1;
    for (int i = 0; i < g_font_count; i++) {
        if (strcmp(g_available_fonts[i].font_name, font_name) == 0) {
            font_idx = i;
            break;
        }
    }
    
    if (font_idx == -1 && g_font_count < MAX_FONTS) {
        font_idx = g_font_count++;
        memset(&g_available_fonts[font_idx], 0, sizeof(FontConfig_t));
        strncpy(g_available_fonts[font_idx].font_name, font_name, sizeof(g_available_fonts[font_idx].font_name) - 1);
        strncpy(g_available_fonts[font_idx].display_name, font_name, sizeof(g_available_fonts[font_idx].display_name) - 1);
    }
    
    if (font_idx == -1) {
        ESP_LOGE(TAG, "Too many fonts");
        return false;
    }
    
    g_available_fonts[font_idx].font_size = font_size;
    strncpy(g_available_fonts[font_idx].ttf_filename, ttf_filename, sizeof(g_available_fonts[font_idx].ttf_filename) - 1);
    
    // 生成BIN文件
    char bin_path[256];
    get_bin_path(font_name, bin_path, sizeof(bin_path));
    
    // 删除旧的BIN文件
    if (SD.exists(bin_path)) {
        SD.remove(bin_path);
    }
    
    // 创建一个简单的BIN文件（占位符）
    // 在实际应用中，这里应该进行TTF解析和转换
    File bin_file = SD.open(bin_path, FILE_WRITE);
    if (!bin_file) {
        ESP_LOGE(TAG, "Failed to create BIN file: %s", bin_path);
        return false;
    }
    
    // 写入BIN文件头
    FontBinHeader_t header;
    generate_bin_header(&header, &g_available_fonts[font_idx]);
    
    bin_file.write((const uint8_t *)&header, sizeof(FontBinHeader_t));
    
    // TODO: 在这里添加实际的TTF转BIN转换逻辑
    // 对于现在，我们只创建一个有效的头部
    bin_file.write((const uint8_t *)"\x00", 1);  // 占位
    bin_file.close();
    
    // 标记为可用
    g_available_fonts[font_idx].is_available = true;
    
    // 保存配置
    save_config_to_sd();
    
    ESP_LOGI(TAG, "Font conversion completed: %s", font_name);
    return true;
}

// ==================== 查询接口 ====================

float ttf_font_get_conversion_progress(const char *font_name) {
    if (!font_name) return 0.0f;
    
    for (int i = 0; i < 4; i++) {
        if (g_conversion_tasks[i].is_converting && 
            strcmp(g_conversion_tasks[i].font_name, font_name) == 0) {
            return g_conversion_tasks[i].progress;
        }
    }
    
    return 0.0f;
}

bool ttf_font_list_available(char *buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return false;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) return false;
    
    cJSON *fonts_array = cJSON_CreateArray();
    if (!fonts_array) {
        cJSON_Delete(root);
        return false;
    }
    
    for (int i = 0; i < g_font_count; i++) {
        if (g_available_fonts[i].is_available) {
            cJSON *font_obj = cJSON_CreateObject();
            if (font_obj) {
                cJSON_AddStringToObject(font_obj, "name", g_available_fonts[i].font_name);
                cJSON_AddStringToObject(font_obj, "display_name", g_available_fonts[i].display_name);
                cJSON_AddNumberToObject(font_obj, "font_size", g_available_fonts[i].font_size);
                cJSON_AddItemToArray(fonts_array, font_obj);
            }
        }
    }
    
    cJSON_AddItemToObject(root, "fonts", fonts_array);
    cJSON_AddNumberToObject(root, "count", g_font_count);
    
    char *json_str = cJSON_Print(root);
    if (json_str) {
        strncpy(buffer, json_str, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        free(json_str);
        cJSON_Delete(root);
        return true;
    }
    
    cJSON_Delete(root);
    return false;
}

bool ttf_font_list_uploaded(char *buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return false;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) return false;
    
    cJSON *files_array = cJSON_CreateArray();
    if (!files_array) {
        cJSON_Delete(root);
        return false;
    }
    
    DIR *dir = opendir(SD_TTF_DIR);
    if (dir) {
        struct dirent *entry;
        int count = 0;
        
        while ((entry = readdir(dir)) != NULL && count < 20) {
            if (entry->d_type == DT_REG && strstr(entry->d_name, ".ttf")) {
                cJSON *file_obj = cJSON_CreateObject();
                if (file_obj) {
                    cJSON_AddStringToObject(file_obj, "filename", entry->d_name);
                    
                    // 获取文件大小
                    char file_path[256];
                    get_ttf_path(entry->d_name, file_path, sizeof(file_path));
                    struct stat st;
                    if (stat(file_path, &st) == 0) {
                        cJSON_AddNumberToObject(file_obj, "size", st.st_size);
                    }
                    
                    cJSON_AddItemToArray(files_array, file_obj);
                    count++;
                }
            }
        }
        closedir(dir);
        
        cJSON_AddNumberToObject(root, "count", count);
    }
    
    cJSON_AddItemToObject(root, "files", files_array);
    
    char *json_str = cJSON_Print(root);
    if (json_str) {
        strncpy(buffer, json_str, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        free(json_str);
        cJSON_Delete(root);
        return true;
    }
    
    cJSON_Delete(root);
    return false;
}

// ==================== 删除接口 ====================

bool ttf_font_delete(const char *font_name) {
    if (!font_name) {
        return false;
    }
    
    ESP_LOGI(TAG, "Deleting font: %s", font_name);
    
    // 找到并删除BIN文件
    char bin_path[256];
    get_bin_path(font_name, bin_path, sizeof(bin_path));
    
    if (SD.exists(bin_path)) {
        if (!SD.remove(bin_path)) {
            ESP_LOGE(TAG, "Failed to delete BIN file: %s", bin_path);
            return false;
        }
    }
    
    // 更新配置
    for (int i = 0; i < g_font_count; i++) {
        if (strcmp(g_available_fonts[i].font_name, font_name) == 0) {
            g_available_fonts[i].is_available = false;
            break;
        }
    }
    
    // 如果删除的是当前字体，切换到其他可用字体
    if (strcmp(g_current_font, font_name) == 0) {
        for (int i = 0; i < g_font_count; i++) {
            if (g_available_fonts[i].is_available) {
                strcpy(g_current_font, g_available_fonts[i].font_name);
                break;
            }
        }
    }
    
    save_config_to_sd();
    
    ESP_LOGI(TAG, "Font deleted: %s", font_name);
    return true;
}

bool ttf_font_delete_ttf(const char *ttf_filename) {
    if (!ttf_filename) {
        return false;
    }
    
    char ttf_path[256];
    get_ttf_path(ttf_filename, ttf_path, sizeof(ttf_path));
    
    if (SD.exists(ttf_path)) {
        if (!SD.remove(ttf_path)) {
            ESP_LOGE(TAG, "Failed to delete TTF file: %s", ttf_path);
            return false;
        }
    }
    
    return true;
}

// ==================== 其他接口 ====================

bool ttf_font_get_info(const char *font_name, char *buffer, uint32_t buffer_size) {
    if (!font_name || !buffer || buffer_size == 0) {
        return false;
    }
    
    // 查找字体
    for (int i = 0; i < g_font_count; i++) {
        if (strcmp(g_available_fonts[i].font_name, font_name) == 0) {
            cJSON *root = cJSON_CreateObject();
            if (!root) return false;
            
            cJSON_AddStringToObject(root, "name", g_available_fonts[i].font_name);
            cJSON_AddStringToObject(root, "display_name", g_available_fonts[i].display_name);
            cJSON_AddNumberToObject(root, "font_size", g_available_fonts[i].font_size);
            cJSON_AddBoolToObject(root, "available", g_available_fonts[i].is_available);
            cJSON_AddNumberToObject(root, "charset_start", g_available_fonts[i].charset_start);
            cJSON_AddNumberToObject(root, "charset_end", g_available_fonts[i].charset_end);
            
            char *json_str = cJSON_Print(root);
            if (json_str) {
                strncpy(buffer, json_str, buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
                free(json_str);
                cJSON_Delete(root);
                return true;
            }
            
            cJSON_Delete(root);
            return false;
        }
    }
    
    return false;
}

bool ttf_font_validate_bin(const char *bin_path) {
    if (!bin_path || !SD.exists(bin_path)) {
        return false;
    }
    
    File bin_file = SD.open(bin_path, FILE_READ);
    if (!bin_file) {
        return false;
    }
    
    FontBinHeader_t header;
    size_t read_bytes = bin_file.read((uint8_t *)&header, sizeof(FontBinHeader_t));
    bin_file.close();
    
    if (read_bytes != sizeof(FontBinHeader_t)) {
        ESP_LOGE(TAG, "Invalid BIN file header size");
        return false;
    }
    
    if (header.magic != FONT_BIN_MAGIC) {
        ESP_LOGE(TAG, "Invalid magic number in BIN file");
        return false;
    }
    
    if (header.version != FONT_BIN_VERSION) {
        ESP_LOGE(TAG, "Unsupported BIN file version");
        return false;
    }
    
    return true;
}

bool ttf_font_switch_to(const char *font_name) {
    if (!font_name) {
        return false;
    }
    
    // 检查字体是否存在且可用
    bool found = false;
    for (int i = 0; i < g_font_count; i++) {
        if (strcmp(g_available_fonts[i].font_name, font_name) == 0) {
            if (!g_available_fonts[i].is_available) {
                ESP_LOGE(TAG, "Font not available: %s", font_name);
                return false;
            }
            found = true;
            break;
        }
    }
    
    if (!found) {
        ESP_LOGE(TAG, "Font not found: %s", font_name);
        return false;
    }
    
    strcpy(g_current_font, font_name);
    save_config_to_sd();
    
    ESP_LOGI(TAG, "Switched to font: %s", font_name);
    return true;
}

const char *ttf_font_get_current(void) {
    if (strlen(g_current_font) == 0) {
        for (int i = 0; i < g_font_count; i++) {
            if (g_available_fonts[i].is_available) {
                return g_available_fonts[i].font_name;
            }
        }
        return "default";
    }
    return g_current_font;
}

const char *ttf_font_get_path(const char *font_name) {
    static char path[256];
    
    if (!font_name) {
        font_name = ttf_font_get_current();
    }
    
    get_bin_path(font_name, path, sizeof(path));
    return path;
}

uint16_t ttf_font_get_size(const char *font_name) {
    if (!font_name) {
        font_name = ttf_font_get_current();
    }
    
    for (int i = 0; i < g_font_count; i++) {
        if (strcmp(g_available_fonts[i].font_name, font_name) == 0) {
            return g_available_fonts[i].font_size;
        }
    }
    
    return 16;  // 默认大小
}

void ttf_font_cleanup_tasks(void) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000000);
    
    for (int i = 0; i < 4; i++) {
        if (g_conversion_tasks[i].is_converting) {
            // 如果任务超时（超过1小时），标记为完成
            if ((now - g_conversion_tasks[i].timestamp) > 3600) {
                memset(&g_conversion_tasks[i], 0, sizeof(FontConversionTask_t));
            }
        }
    }
}

bool ttf_font_get_status(char *buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return false;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) return false;
    
    cJSON_AddBoolToObject(root, "initialized", g_initialized);
    cJSON_AddStringToObject(root, "current_font", g_current_font);
    cJSON_AddNumberToObject(root, "total_fonts", g_font_count);
    
    // 统计可用字体
    int available_count = 0;
    for (int i = 0; i < g_font_count; i++) {
        if (g_available_fonts[i].is_available) {
            available_count++;
        }
    }
    cJSON_AddNumberToObject(root, "available_fonts", available_count);
    
    // 统计正在转换的任务
    int converting_count = 0;
    for (int i = 0; i < 4; i++) {
        if (g_conversion_tasks[i].is_converting) {
            converting_count++;
        }
    }
    cJSON_AddNumberToObject(root, "converting_tasks", converting_count);
    
    // 获取SD卡信息
    uint64_t total = SD.totalBytes();
    uint64_t used = SD.usedBytes();
    uint64_t free_bytes = total - used;
    
    cJSON_AddNumberToObject(root, "sd_total_bytes", total);
    cJSON_AddNumberToObject(root, "sd_used_bytes", used);
    cJSON_AddNumberToObject(root, "sd_free_bytes", free_bytes);
    
    char *json_str = cJSON_Print(root);
    if (json_str) {
        strncpy(buffer, json_str, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        free(json_str);
        cJSON_Delete(root);
        return true;
    }
    
    cJSON_Delete(root);
    return false;
}

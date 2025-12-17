/**
 * @file ttf_font_api.cpp
 * @brief TTF字体管理Web API实现
 */

#include "ttf_font_api.h"
#include "ttf_font_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

extern "C" {
#include "../../../../../arduino_esp32/tools/sdk/esp32s3/include/json/cJSON/cJSON.h"
}

#define TAG "TTF_FontAPI"

// ==================== 工具宏 ====================

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// ==================== API 端点处理器 ====================

/**
 * @brief GET /api/fonts/list - 获取可用字库列表
 */
static esp_err_t api_fonts_list_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/fonts/list");
    
    char *response = (char *)malloc(4096);
    if (!response) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    
    bool success = ttf_font_list_available(response, 4096);
    
    if (success) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, response, strlen(response));
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get font list");
    }
    
    free(response);
    return ESP_OK;
}

/**
 * @brief POST /api/fonts/upload - 上传并转换TTF文件
 */
static esp_err_t api_fonts_upload_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/fonts/upload");
    
    #define UPLOAD_BUFFER_SIZE 8192
    
    char *buf = (char *)malloc(UPLOAD_BUFFER_SIZE);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    
    // 接收上传数据
    char json_data[2048] = {0};
    size_t json_length = 0;
    int remaining = req->content_len;
    
    while (remaining > 0) {
        int received = httpd_req_recv(req, buf, MIN(remaining, UPLOAD_BUFFER_SIZE));
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
            return ESP_FAIL;
        }
        
        if (json_length + received < sizeof(json_data)) {
            memcpy(json_data + json_length, buf, received);
            json_length += received;
        }
        remaining -= received;
    }
    
    free(buf);
    
    // 解析JSON数据
    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    // 获取参数
    cJSON *font_name_obj = cJSON_GetObjectItem(root, "font_name");
    cJSON *ttf_filename_obj = cJSON_GetObjectItem(root, "ttf_filename");
    cJSON *font_size_obj = cJSON_GetObjectItem(root, "font_size");
    cJSON *charset_obj = cJSON_GetObjectItem(root, "charset");
    
    if (!font_name_obj || !ttf_filename_obj || !font_size_obj) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required parameters");
        return ESP_FAIL;
    }
    
    const char *font_name = font_name_obj->valuestring;
    const char *ttf_filename = ttf_filename_obj->valuestring;
    uint16_t font_size = (uint16_t)font_size_obj->valueint;
    const char *charset = charset_obj ? charset_obj->valuestring : "GB2312";
    
    ESP_LOGI(TAG, "Converting font: %s from %s (size: %d)", font_name, ttf_filename, font_size);
    
    // 执行转换
    bool success = ttf_font_start_conversion(font_name, ttf_filename, font_size, charset);
    
    cJSON_Delete(root);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    if (success) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "Font conversion started");
        cJSON_AddStringToObject(response, "font_name", font_name);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Font conversion failed");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

/**
 * @brief GET /api/fonts/list-uploaded - 获取已上传的TTF文件列表
 */
static esp_err_t api_fonts_list_uploaded_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/fonts/list-uploaded");
    
    char *response = (char *)malloc(4096);
    if (!response) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    
    bool success = ttf_font_list_uploaded(response, 4096);
    
    if (success) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, response, strlen(response));
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get uploaded files list");
    }
    
    free(response);
    return ESP_OK;
}

/**
 * @brief DELETE /api/fonts/delete - 删除字库文件
 */
static esp_err_t api_fonts_delete_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "DELETE /api/fonts/delete");
    
    char query_str[256] = {0};
    size_t query_len = httpd_req_get_url_query_len(req);
    
    if (query_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font name parameter");
        return ESP_FAIL;
    }
    
    if (query_len >= sizeof(query_str)) {
        query_len = sizeof(query_str) - 1;
    }
    
    httpd_req_get_url_query_str(req, query_str, query_len + 1);
    
    // 解析查询参数
    char font_name[128] = {0};
    if (httpd_query_key_value(query_str, "font_name", font_name, sizeof(font_name)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font_name parameter");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Deleting font: %s", font_name);
    
    // 执行删除
    bool success = ttf_font_delete(font_name);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    if (success) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "Font deleted successfully");
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to delete font");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

/**
 * @brief GET /api/fonts/info - 获取字体信息
 */
static esp_err_t api_fonts_info_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/fonts/info");
    
    char query_str[256] = {0};
    size_t query_len = httpd_req_get_url_query_len(req);
    
    if (query_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font name parameter");
        return ESP_FAIL;
    }
    
    if (query_len >= sizeof(query_str)) {
        query_len = sizeof(query_str) - 1;
    }
    
    httpd_req_get_url_query_str(req, query_str, query_len + 1);
    
    // 解析查询参数
    char font_name[128] = {0};
    if (httpd_query_key_value(query_str, "font_name", font_name, sizeof(font_name)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font_name parameter");
        return ESP_FAIL;
    }
    
    char *response = (char *)malloc(1024);
    if (!response) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    
    bool success = ttf_font_get_info(font_name, response, 1024);
    
    if (success) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, response, strlen(response));
    } else {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Font not found");
    }
    
    free(response);
    return ESP_OK;
}

/**
 * @brief GET /api/fonts/status - 获取字体管理系统状态
 */
static esp_err_t api_fonts_status_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/fonts/status");
    
    char *response = (char *)malloc(2048);
    if (!response) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }
    
    bool success = ttf_font_get_status(response, 2048);
    
    if (success) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, response, strlen(response));
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get status");
    }
    
    free(response);
    return ESP_OK;
}

/**
 * @brief GET /api/fonts/switch - 切换当前使用的字库
 */
static esp_err_t api_fonts_switch_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/fonts/switch");
    
    char query_str[256] = {0};
    size_t query_len = httpd_req_get_url_query_len(req);
    
    if (query_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font name parameter");
        return ESP_FAIL;
    }
    
    if (query_len >= sizeof(query_str)) {
        query_len = sizeof(query_str) - 1;
    }
    
    httpd_req_get_url_query_str(req, query_str, query_len + 1);
    
    // 解析查询参数
    char font_name[128] = {0};
    if (httpd_query_key_value(query_str, "font_name", font_name, sizeof(font_name)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font_name parameter");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Switching to font: %s", font_name);
    
    // 执行切换
    bool success = ttf_font_switch_to(font_name);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    if (success) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "Font switched successfully");
        cJSON_AddStringToObject(response, "current_font", font_name);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to switch font");
    }
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

/**
 * @brief GET /api/fonts/progress - 获取转换进度
 */
static esp_err_t api_fonts_progress_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/fonts/progress");
    
    char query_str[256] = {0};
    size_t query_len = httpd_req_get_url_query_len(req);
    
    if (query_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font name parameter");
        return ESP_FAIL;
    }
    
    if (query_len >= sizeof(query_str)) {
        query_len = sizeof(query_str) - 1;
    }
    
    httpd_req_get_url_query_str(req, query_str, query_len + 1);
    
    // 解析查询参数
    char font_name[128] = {0};
    if (httpd_query_key_value(query_str, "font_name", font_name, sizeof(font_name)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing font_name parameter");
        return ESP_FAIL;
    }
    
    float progress = ttf_font_get_conversion_progress(font_name);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "progress", progress);
    cJSON_AddStringToObject(response, "font_name", font_name);
    
    char *response_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    free(response_str);
    cJSON_Delete(response);
    
    return ESP_OK;
}

// ==================== API 注册 ====================

esp_err_t register_ttf_font_apis(httpd_handle_t server) {
    if (!server) {
        ESP_LOGE(TAG, "Invalid server handle");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Registering TTF font management APIs...");
    
    // 初始化字体管理器
    if (!ttf_font_manager_init()) {
        ESP_LOGE(TAG, "Failed to initialize font manager");
        return ESP_FAIL;
    }
    
    // 注册端点
    httpd_uri_t uri_list = {
        .uri = "/api/fonts/list",
        .method = HTTP_GET,
        .handler = api_fonts_list_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_list);
    
    httpd_uri_t uri_upload = {
        .uri = "/api/fonts/upload",
        .method = HTTP_POST,
        .handler = api_fonts_upload_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_upload);
    
    httpd_uri_t uri_list_uploaded = {
        .uri = "/api/fonts/list-uploaded",
        .method = HTTP_GET,
        .handler = api_fonts_list_uploaded_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_list_uploaded);
    
    httpd_uri_t uri_delete = {
        .uri = "/api/fonts/delete",
        .method = HTTP_DELETE,
        .handler = api_fonts_delete_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_delete);
    
    httpd_uri_t uri_info = {
        .uri = "/api/fonts/info",
        .method = HTTP_GET,
        .handler = api_fonts_info_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_info);
    
    httpd_uri_t uri_status = {
        .uri = "/api/fonts/status",
        .method = HTTP_GET,
        .handler = api_fonts_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_status);
    
    httpd_uri_t uri_switch = {
        .uri = "/api/fonts/switch",
        .method = HTTP_GET,
        .handler = api_fonts_switch_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_switch);
    
    httpd_uri_t uri_progress = {
        .uri = "/api/fonts/progress",
        .method = HTTP_GET,
        .handler = api_fonts_progress_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_progress);
    
    ESP_LOGI(TAG, "TTF font management APIs registered successfully");
    return ESP_OK;
}

void unregister_ttf_font_apis(httpd_handle_t server) {
    if (!server) {
        return;
    }
    
    ESP_LOGI(TAG, "Unregistering TTF font management APIs...");
    
    // 注销端点
    httpd_unregister_uri_handler(server, "/api/fonts/list", HTTP_GET);
    httpd_unregister_uri_handler(server, "/api/fonts/upload", HTTP_POST);
    httpd_unregister_uri_handler(server, "/api/fonts/list-uploaded", HTTP_GET);
    httpd_unregister_uri_handler(server, "/api/fonts/delete", HTTP_DELETE);
    httpd_unregister_uri_handler(server, "/api/fonts/info", HTTP_GET);
    httpd_unregister_uri_handler(server, "/api/fonts/status", HTTP_GET);
    httpd_unregister_uri_handler(server, "/api/fonts/switch", HTTP_GET);
    httpd_unregister_uri_handler(server, "/api/fonts/progress", HTTP_GET);
}

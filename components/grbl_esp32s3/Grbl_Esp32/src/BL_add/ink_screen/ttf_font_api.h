/**
 * @file ttf_font_api.h
 * @brief TTF字体管理Web API接口
 */

#ifndef TTF_FONT_API_H
#define TTF_FONT_API_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册TTF字体管理API端点
 * @param server HTTP服务器指针
 * @return ESP_OK成功
 */
esp_err_t register_ttf_font_apis(httpd_handle_t server);

/**
 * @brief 注销TTF字体管理API端点
 * @param server HTTP服务器指针
 */
void unregister_ttf_font_apis(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif // TTF_FONT_API_H

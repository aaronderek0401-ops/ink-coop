/**
 * @file image_loader.h
 * @brief SD卡和SPIFFS图片加载器 - 从 .bin 文件加载图片到墨水屏
 * 
 * 支持的格式:
 * - .bin 单色位图文件 (1 bit per pixel)
 * - 文件结构: [宽度4字节][高度4字节][位图数据]
 * 
 * 支持的存储位置:
 * - SD 卡: /sd/image.bin
 * - SPIFFS: /image.bin
 * 
 * 使用示例:
 * ```cpp
 * // 从SD卡显示图片
 * displayImageFromSD("/sd/logo_240x416.bin", 0, 0, display);
 * 
 * // 从SPIFFS显示图片（需要先烧录SPIFFS分区）
 * displayImageFromSPIFFS("/logo.bin", 0, 0, display);
 * ```
 */

#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <Arduino.h>
#include <SD.h>
#include <GxEPD2_BW.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <FS.h>

// 文件源枚举（与 cpp 文件同步）
typedef enum {
    FILE_SOURCE_SD = 0,      // 从SD卡加载
    FILE_SOURCE_SPIFFS = 1   // 从SPIFFS加载
} FileSource;

/**
 * @brief 从SD卡加载并显示 .bin 格式的图片
 * 
 * @tparam DisplayType GxEPD2_BW 类型（支持任何尺寸的墨水屏）
 * @param filename SD卡上的文件路径 (如 "/sd/logo.bin")
 * @param x 显示位置 X 坐标
 * @param y 显示位置 Y 坐标
 * @param display GxEPD2 显示对象引用
 * @return true 成功, false 失败
 * 
 * .bin 文件格式:
 * - 前 4 字节: 图片宽度 (小端序 uint32_t)
 * - 第 5-8 字节: 图片高度 (小端序 uint32_t)
 * - 剩余字节: 位图数据 (每8个像素=1字节, 逐行扫描)
 * 
 * 示例：
 * ```cpp
 * // 3.7寸屏
 * GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display370(...);
 * displayImageFromSD("/sd/logo.bin", 0, 0, display370);
 * 
 * // 2.13寸屏
 * GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT> display213(...);
 * displayImageFromSD("/sd/logo.bin", 0, 0, display213);
 * ```
 */
template<typename GxEPD2_Type, const uint16_t page_height>
bool displayImageFromSD(const char* filename, int16_t x, int16_t y, 
                        GxEPD2_BW<GxEPD2_Type, page_height>& display);

template<typename GxEPD2_Type, const uint16_t page_height>
bool displayImageFromSource(const char* filename, int16_t x, int16_t y,
                            GxEPD2_BW<GxEPD2_Type, page_height>& display,
                            FileSource source);

/**
 * @brief 从SPIFFS加载并显示 .bin 格式的图片
 * 
 * @tparam DisplayType GxEPD2_BW 类型（支持任何尺寸的墨水屏）
 * @param filename SPIFFS上的文件路径 (如 "/logo.bin")
 * @param x 显示位置 X 坐标
 * @param y 显示位置 Y 坐标
 * @param display GxEPD2 显示对象引用
 * @return true 成功, false 失败
 * 
 * 注意: 需要先将图片文件烧录到SPIFFS分区
 */
template<typename GxEPD2_Type, const uint16_t page_height>
inline bool displayImageFromSPIFFS(const char* filename, int16_t x, int16_t y,
                                    GxEPD2_BW<GxEPD2_Type, page_height>& display) {
    return displayImageFromSource(filename, x, y, display, FILE_SOURCE_SPIFFS);
}

/**
 * @brief 从SD卡读取图片信息（不显示）
 * 
 * @param filename SD卡文件路径
 * @param width 输出参数 - 图片宽度
 * @param height 输出参数 - 图片高度
 * @return true 成功, false 失败
 */
bool getImageInfo(const char* filename, uint32_t* width, uint32_t* height);

/**
 * @brief 从指定源读取图片信息（不显示）
 */
bool getImageInfoFromSource(const char* filename, uint32_t* width, uint32_t* height,
                            FileSource source);

/**
 * @brief 从SD卡加载图片到内存缓冲区
 * 
 * @param filename SD卡文件路径
 * @param buffer 输出缓冲区（调用者负责分配和释放）
 * @param width 输出 - 图片宽度
 * @param height 输出 - 图片高度
 * @return true 成功, false 失败
 * 
 * 注意: buffer 需要预先分配足够大小:
 * size = ((width + 7) / 8) * height 字节
 */
bool loadImageToBuffer(const char* filename, uint8_t* buffer, 
                       uint32_t* width, uint32_t* height);

/**
 * @brief 从指定源加载图片到内存缓冲区（内部使用）
 */
bool loadImageToBufferFromSource(const char* filename, uint8_t* buffer,
                                 uint32_t* width, uint32_t* height,
                                 FileSource source);

// ==================== 模板函数实现 ====================
// 必须在头文件中实现，以支持任意 GxEPD2 屏幕类型

/**
 * @brief 从指定源显示图片（模板实现）
 * @tparam GxEPD2_Type 屏幕驱动类型（如 GxEPD2_370_GDEY037T03, GxEPD2_213_B73 等）
 * @tparam page_height 页面高度（通常是屏幕高度）
 */
template<typename GxEPD2_Type, const uint16_t page_height>
bool displayImageFromSource(const char* filename, int16_t x, int16_t y,
                            GxEPD2_BW<GxEPD2_Type, page_height>& display,
                            FileSource source) {
    static const char* TAG = "ImageLoader";
    const char* source_name = (source == FILE_SOURCE_SPIFFS) ? "SPIFFS" : "SD卡";
    
    ESP_LOGI(TAG, "开始从 %s 加载图片: %s", source_name, filename);
    
    uint32_t img_width, img_height;
    
    // 获取图片信息
    if (!getImageInfoFromSource(filename, &img_width, &img_height, source)) {
        return false;
    }
    
    // 计算需要的缓冲区大小
    uint32_t bytes_per_row = (img_width + 7) / 8;
    uint32_t buffer_size = bytes_per_row * img_height;
    
    ESP_LOGI(TAG, "分配缓冲区: %d 字节 (用于 %dx%d 图片)", 
             buffer_size, img_width, img_height);
    
    // 优先使用 PSRAM 分配缓冲区（如果启用了）
    uint8_t* bitmap_buffer = (uint8_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!bitmap_buffer) {
        ESP_LOGW(TAG, "PSRAM 不足或未启用，尝试使用内部 RAM");
        bitmap_buffer = (uint8_t*)malloc(buffer_size);
    }
    
    if (!bitmap_buffer) {
        ESP_LOGE(TAG, "内存分配失败! 需要 %d 字节", buffer_size);
        return false;
    }
    
    // 加载图片到缓冲区
    if (!loadImageToBufferFromSource(filename, bitmap_buffer, &img_width, &img_height, source)) {
        free(bitmap_buffer);
        return false;
    }
    
    // 显示图片
    ESP_LOGI(TAG, "在位置 (%d, %d) 显示图片...", x, y);
    display.drawBitmap(x, y, bitmap_buffer, img_width, img_height, GxEPD_BLACK);
    
    // 释放缓冲区
    free(bitmap_buffer);
    
    ESP_LOGI(TAG, "✅ 从 %s 成功显示图片", source_name);
    return true;
}

/**
 * @brief 从SD卡显示图片（模板实现）
 */
template<typename GxEPD2_Type, const uint16_t page_height>
bool displayImageFromSD(const char* filename, int16_t x, int16_t y,
                        GxEPD2_BW<GxEPD2_Type, page_height>& display) {
    return displayImageFromSource(filename, x, y, display, FILE_SOURCE_SD);
}

#endif // IMAGE_LOADER_H

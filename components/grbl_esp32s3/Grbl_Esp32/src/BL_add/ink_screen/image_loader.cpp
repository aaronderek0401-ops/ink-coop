/**
 * @file image_loader.cpp
 * @brief SD卡和SPIFFS图片加载器实现
 */

#include "image_loader.h"
#include <esp_log.h>
#include "../../Grbl.h"
#include <FS.h>
#include <SPIFFS.h>
#include <esp_heap_caps.h>

static const char* TAG = "ImageLoader";

/**
 * @brief 打开文件（支持 SD 和 SPIFFS）
 */
static File openFile(const char* filename, FileSource source) {
    if (source == FILE_SOURCE_SPIFFS) {
        return SPIFFS.open(filename, FILE_READ);
    } else {
        return SD.open(filename, FILE_READ);
    }
}

/**
 * @brief 获取文件源的字符串描述
 */
static const char* getSourceName(FileSource source) {
    return (source == FILE_SOURCE_SPIFFS) ? "SPIFFS" : "SD卡";
}

bool getImageInfo(const char* filename, uint32_t* width, uint32_t* height) {
    return getImageInfoFromSource(filename, width, height, FILE_SOURCE_SD);
}

/**
 * @brief 从指定源获取图片信息
 * @param filename 文件名（支持绝对路径）
 * @param width 输出参数：图片宽度
 * @param height 输出参数：图片高度
 * @param source 文件源：FILE_SOURCE_SD 或 FILE_SOURCE_SPIFFS
 */
bool getImageInfoFromSource(const char* filename, uint32_t* width, uint32_t* height, 
                            FileSource source) {
    // 如果是从 SD 卡加载，先检查 SD 卡状态
    if (source == FILE_SOURCE_SD) {
        SDState state = get_sd_state(true);
        if (state != SDState::Idle) {
            if (state == SDState::NotPresent) {
               ESP_LOGE(TAG,"SD卡不存在");
            } else {
                ESP_LOGE(TAG,"SD卡繁忙");
            }
            // 如果 SD 卡不可用，尝试从 SPIFFS 加载
            if (state == SDState::NotPresent) {
                ESP_LOGW(TAG, "SD卡不可用，尝试从 SPIFFS 加载: %s", filename);
                return getImageInfoFromSource(filename, width, height, FILE_SOURCE_SPIFFS);
            }
        }
    }
    
    File file = openFile(filename, source);
    if (!file) {
        ESP_LOGE(TAG, "无法从 %s 打开文件: %s", getSourceName(source), filename);
        return false;
    }
    
    // 读取文件头 (8字节)
    if (file.size() < 8) {
        ESP_LOGE(TAG, "文件太小,不是有效的 .bin 图片: %s (%s)", filename, getSourceName(source));
        file.close();
        return false;
    }
    
    // 读取宽度 (小端序)
    uint8_t width_bytes[4];
    file.read(width_bytes, 4);
    *width = width_bytes[0] | (width_bytes[1] << 8) | 
             (width_bytes[2] << 16) | (width_bytes[3] << 24);
    
    // 读取高度 (小端序)
    uint8_t height_bytes[4];
    file.read(height_bytes, 4);
    *height = height_bytes[0] | (height_bytes[1] << 8) | 
              (height_bytes[2] << 16) | (height_bytes[3] << 24);
    
    file.close();
    
    // 验证尺寸合理性
    if (*width == 0 || *height == 0 || *width > 1000 || *height > 1000) {
        ESP_LOGE(TAG, "图片尺寸异常: %dx%d", *width, *height);
        return false;
    }
    
    ESP_LOGI(TAG, "✅ 获取图片信息成功 (%s): %s, 尺寸 %dx%d", 
             getSourceName(source), filename, *width, *height);
    return true;
}

bool loadImageToBuffer(const char* filename, uint8_t* buffer, 
                       uint32_t* width, uint32_t* height) {
    return loadImageToBufferFromSource(filename, buffer, width, height, FILE_SOURCE_SD);
}

/**
 * @brief 从指定源加载图片到缓冲区
 * @param filename 文件名
 * @param buffer 输出缓冲区
 * @param width 输出参数：图片宽度
 * @param height 输出参数：图片高度
 * @param source 文件源
 */
bool loadImageToBufferFromSource(const char* filename, uint8_t* buffer, 
                                 uint32_t* width, uint32_t* height,
                                 FileSource source) {
    if (!buffer) {
        ESP_LOGE(TAG, "缓冲区指针为空");
        return false;
    }
    
    // 先获取图片信息
    if (!getImageInfoFromSource(filename, width, height, source)) {
        return false;
    }
    
    File file = openFile(filename, source);
    if (!file) {
        ESP_LOGE(TAG, "无法从 %s 打开文件: %s", getSourceName(source), filename);
        return false;
    }
    
    // 跳过文件头 (8字节)
    file.seek(8);
    
    // 计算位图数据大小
    uint32_t bytes_per_row = (*width + 7) / 8;
    uint32_t bitmap_size = bytes_per_row * (*height);
    
    // 读取位图数据
    size_t bytes_read = file.read(buffer, bitmap_size);
    file.close();
    
    if (bytes_read != bitmap_size) {
        ESP_LOGE(TAG, "读取数据不完整: 期望 %d 字节, 实际 %d 字节", 
                 bitmap_size, bytes_read);
        return false;
    }
    
    ESP_LOGI(TAG, "✅ 成功从 %s 加载图片到缓冲区: %dx%d, %d 字节", 
             getSourceName(source), *width, *height, bitmap_size);
    return true;
}

// 注意: displayImageFromSD 和 displayImageFromSource 已移至 image_loader.h 作为模板函数实现
// 以支持任意 GxEPD2 屏幕类型（3.7寸、2.13寸等）

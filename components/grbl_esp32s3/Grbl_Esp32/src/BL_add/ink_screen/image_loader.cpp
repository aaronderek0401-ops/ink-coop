/**
 * @file image_loader.cpp
 * @brief SD卡图片加载器实现
 */

#include "image_loader.h"
#include <esp_log.h>
#include "../../Grbl.h"
#include <FS.h>
static const char* TAG = "ImageLoader";

bool getImageInfo(const char* filename, uint32_t* width, uint32_t* height) {
            SDState state = get_sd_state(true);
        if (state != SDState::Idle) {
            if (state == SDState::NotPresent) {
               ESP_LOGE(TAG,"No SD Card");
            } else {
                ESP_LOGE(TAG,"SD Card Busy");
            }
        }
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        ESP_LOGE(TAG, "无法打开文件: %s", filename);
        return false;
    }
    
    // 读取文件头 (8字节)
    if (file.size() < 8) {
        ESP_LOGE(TAG, "文件太小,不是有效的 .bin 图片: %s", filename);
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
    
    ESP_LOGI(TAG, "图片信息: %s, 尺寸 %dx%d", filename, *width, *height);
    return true;
}

bool loadImageToBuffer(const char* filename, uint8_t* buffer, 
                       uint32_t* width, uint32_t* height) {
    if (!buffer) {
        ESP_LOGE(TAG, "缓冲区指针为空");
        return false;
    }
    
    // 先获取图片信息
    if (!getImageInfo(filename, width, height)) {
        return false;
    }
    
    File file = SD.open(filename, FILE_READ);
    if (!file) {
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
    
    ESP_LOGI(TAG, "成功加载图片到缓冲区: %dx%d, %d 字节", 
             *width, *height, bitmap_size);
    return true;
}

bool displayImageFromSD(const char* filename, int16_t x, int16_t y,
                        GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT>& display) {
    ESP_LOGI(TAG, "开始从SD卡加载图片: %s", filename);
    
    uint32_t img_width, img_height;
    
    // 获取图片信息
    if (!getImageInfo(filename, &img_width, &img_height)) {
        return false;
    }
    
    // 计算需要的缓冲区大小
    uint32_t bytes_per_row = (img_width + 7) / 8;
    uint32_t buffer_size = bytes_per_row * img_height;
    
    ESP_LOGI(TAG, "分配缓冲区: %d 字节 (用于 %dx%d 图片)", 
             buffer_size, img_width, img_height);
    
    // 分配缓冲区
    uint8_t* bitmap_buffer = (uint8_t*)malloc(buffer_size);
    if (!bitmap_buffer) {
        ESP_LOGE(TAG, "内存分配失败! 需要 %d 字节", buffer_size);
        return false;
    }
    
    // 加载图片到缓冲区
    if (!loadImageToBuffer(filename, bitmap_buffer, &img_width, &img_height)) {
        free(bitmap_buffer);
        return false;
    }
    
    // 显示图片
    ESP_LOGI(TAG, "在位置 (%d, %d) 显示图片...", x, y);
    display.drawBitmap(x, y, bitmap_buffer, img_width, img_height, GxEPD_BLACK);
    
    // 释放缓冲区
    free(bitmap_buffer);
    
    ESP_LOGI(TAG, "✅ 图片显示完成");
    return true;
}

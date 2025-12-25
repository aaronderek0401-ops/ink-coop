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

// 文件源枚举（与 cpp 文件同步）
typedef enum {
    FILE_SOURCE_SD = 0,      // 从SD卡加载
    FILE_SOURCE_SPIFFS = 1   // 从SPIFFS加载
} FileSource;

/**
 * @brief 从SD卡加载并显示 .bin 格式的图片
 * 
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
 */
bool displayImageFromSD(const char* filename, int16_t x, int16_t y, 
                        GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT>& display);
bool displayImageFromSource(const char* filename, int16_t x, int16_t y,
                            GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT>& display,
                            FileSource source);
/**
 * @brief 从SPIFFS加载并显示 .bin 格式的图片
 * 
 * @param filename SPIFFS上的文件路径 (如 "/logo.bin")
 * @param x 显示位置 X 坐标
 * @param y 显示位置 Y 坐标
 * @param display GxEPD2 显示对象引用
 * @return true 成功, false 失败
 * 
 * 注意: 需要先将图片文件烧录到SPIFFS分区
 */
inline bool displayImageFromSPIFFS(const char* filename, int16_t x, int16_t y,
                                    GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT>& display) {
    return displayImageFromSource(filename, x, y, display, FILE_SOURCE_SPIFFS);
}

/**
 * @brief 从指定源加载并显示图片（内部使用）
 */
bool displayImageFromSource(const char* filename, int16_t x, int16_t y,
                            GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT>& display,
                            FileSource source);

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

#endif // IMAGE_LOADER_H

/**
 * @file image_loader.h
 * @brief SD卡图片加载器 - 从 .bin 文件加载图片到墨水屏
 * 
 * 支持的格式:
 * - .bin 单色位图文件 (1 bit per pixel)
 * - 文件结构: [宽度4字节][高度4字节][位图数据]
 * 
 * 使用示例:
 * ```cpp
 * // 显示SD卡中的图片
 * displayImageFromSD("/sd/logo_240x416.bin", 0, 0);
 * 
 * // 显示指定尺寸的图片
 * displayImageFromSD("/sd/photo_200x200.bin", 20, 100);
 * ```
 * 
 * 配合 Python 工具生成 .bin 文件:
 * ```
 * python tools/ttf_to_gfx_webservice.py
 * # 在网页中上传图片 → 转换为 .bin → 上传到SD卡
 * ```
 */

#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <Arduino.h>
#include <SD.h>
#include <GxEPD2_BW.h>

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

#endif // IMAGE_LOADER_H

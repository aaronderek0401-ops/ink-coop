/**
 * @file chinese_text_display.h
 * @brief 中文文本显示助手 - 使用SD卡.bin字库在墨水屏上显示中文
 */

#ifndef CHINESE_TEXT_DISPLAY_H
#define CHINESE_TEXT_DISPLAY_H

#include <stdint.h>

// 前置声明
class GxEPD2_GFX;

/**
 * @brief 初始化中文字库 (从SD卡.bin文件)
 * @param bin_path .bin字库文件路径,如 "/sd/fangsong_gb2312_16x16.bin"
 * @param font_size 字号 (16, 24 或 32)
 * @return true 初始化成功, false 失败
 */
bool initChineseFontFromSD(const char* bin_path, int font_size = 16);

/**
 * @brief 在墨水屏上绘制单个中文字符
 * @param display GxEPD2显示对象引用
 * @param x X坐标
 * @param y Y坐标
 * @param unicode Unicode编码
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 */
template<typename T>
void drawChineseChar(T& display, int16_t x, int16_t y, uint16_t unicode, uint16_t color = 0x0000);

/**
 * @brief 在墨水屏上绘制中文文本 (支持自动换行)
 * @param display GxEPD2显示对象引用
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param text UTF-8编码的文本
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 * @return 绘制结束后的Y坐标
 */
template<typename T>
int16_t drawChineseText(T& display, int16_t x, int16_t y, const char* text, uint16_t color = 0x0000);

/**
 * @brief 在墨水屏上绘制居中的中文文本
 * @param display GxEPD2显示对象引用
 * @param y Y坐标
 * @param text UTF-8编码的文本
 * @param color 颜色 (GxEPD_BLACK 或 GxEPD_WHITE)
 * @return 绘制结束后的Y坐标
 */
template<typename T>
int16_t drawChineseTextCentered(T& display, int16_t y, const char* text, uint16_t color = 0x0000);

// 模板函数实现必须在头文件中
#include "chinese_text_display_impl.h"

#endif // CHINESE_TEXT_DISPLAY_H

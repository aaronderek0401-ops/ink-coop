/**
 * @file multi_font_manager.h
 * @brief 多字体管理器 - 支持在不同场景使用不同大小的字体
 * @details
 * 用途：
 * - 显示单词时使用 24x24 字体 (comic_sans_ms_bold_24x24.bin)
 * - 显示音标时使用 16x16 字体 (comic_sans_ms_bold_16x16.bin)
 * 
 * 使用方法：
 * 1. 调用 initMultiFontManager() 初始化
 * 2. 调用 drawEnglishWord() 显示单词 (使用24x24)
 * 3. 调用 drawEnglishPhonetic() 显示音标 (使用16x16)
 */

#ifndef MULTI_FONT_MANAGER_H
#define MULTI_FONT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// 前置声明
class GxEPD2_GFX;

/**
 * @brief 初始化多字体管理器
 * @param word_font_path 单词字体路径 (如: "/sd/comic_sans_ms_bold_24x24.bin")
 * @param phonetic_font_path 音标字体路径 (如: "/sd/comic_sans_ms_bold_16x16.bin")
 * @return true 初始化成功
 */
bool initMultiFontManager(const char* word_font_path, const char* phonetic_font_path);

/**
 * @brief 获取字体信息
 * @param font_type 字体类型: 0=单词字体, 1=音标字体
 * @return 字体大小 (16, 24 或 32)
 */
int getMultiFontSize(int font_type);

/**
 * @brief 检查字体是否已初始化
 * @return true 已初始化, false 未初始化
 */
bool isMultiFontManagerInitialized(void);

/**
 * @brief 获取单个英文字符的字模
 * @param font_type 字体类型: 0=单词字体, 1=音标字体
 * @param ascii_code ASCII码 (0-127)
 * @param out_buffer 输出缓冲区
 * @return true 成功, false 失败
 */
bool getEnglishCharGlyph(int font_type, uint8_t ascii_code, uint8_t* out_buffer);

/**
 * @brief 内部函数：使用全局 display 对象绘制英文单词
 * @note 仅供 wordbook_interface.cpp 使用
 */
void _drawEnglishWordInternal(int16_t x, int16_t y, const char* text, uint16_t color);

/**
 * @brief 内部函数：使用全局 display 对象绘制英文音标
 * @note 仅供 wordbook_interface.cpp 使用
 */
void _drawEnglishPhoneticInternal(int16_t x, int16_t y, const char* text, uint16_t color);

#ifdef __cplusplus
}
#endif

#endif // MULTI_FONT_MANAGER_H

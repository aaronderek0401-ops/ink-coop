/**
 * @file ttf_font_manager.h
 * @brief TTF字体管理系统 - 支持多种TTF字体转换为BIN格式
 * @details 
 * 支持的字体：
 * - fangsong_GB2312 (仿宋_GB2312)
 * - Comic-Sans-MS-V3
 * - Comic Sans MS Bold
 * - 其他自定义TTF字体
 * 
 * 功能：
 * 1. TTF文件上传
 * 2. TTF转BIN转换
 * 3. 字库列表查询
 * 4. 字库删除
 * 5. 字体管理和配置
 */

#ifndef TTF_FONT_MANAGER_H
#define TTF_FONT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 数据结构 ====================

/**
 * @brief 字体配置信息
 */
typedef struct {
    char font_name[64];         // 字体名称 (如: "fangsong_GB2312")
    char display_name[64];      // 显示名称 (如: "仿宋_GB2312")
    char ttf_filename[128];     // TTF文件名
    uint16_t font_size;         // 字体大小 (16, 24, 32等)
    uint32_t charset_start;     // 字符集开始编码
    uint32_t charset_end;       // 字符集结束编码
    bool is_available;          // 是否可用
} FontConfig_t;

/**
 * @brief 字库文件头
 */
typedef struct {
    uint32_t magic;             // 魔数: 0x544F4E46 ("FONT" in reverse)
    uint16_t version;           // 版本号
    uint16_t font_size;         // 字体大小
    uint32_t charset_start;     // 字符集起始编码
    uint32_t charset_end;       // 字符集结束编码
    uint32_t charset_size;      // 实际包含的字符数
    uint32_t char_width;        // 字符宽度
    uint32_t char_height;       // 字符高度
    uint32_t bytes_per_char;    // 每个字符的字节数
    char font_name[32];         // 字体名称
    uint32_t reserved[8];       // 预留
} FontBinHeader_t;

/**
 * @brief 字体转换任务
 */
typedef struct {
    char font_name[64];         // 字体识别名称
    char ttf_path[256];         // TTF文件路径
    char bin_path[256];         // 输出BIN路径
    uint16_t font_size;         // 字体大小
    float progress;             // 进度 (0.0 - 100.0)
    bool is_converting;         // 正在转换中
    uint32_t timestamp;         // 创建时间戳
} FontConversionTask_t;

// ==================== API函数 ====================

/**
 * @brief 初始化字体管理系统
 * @return 成功返回true
 */
bool ttf_font_manager_init(void);

/**
 * @brief 上传TTF文件（从Web接收）
 * @param filename TTF文件名
 * @param data TTF文件数据
 * @param size 数据大小
 * @return 成功返回true
 */
bool ttf_font_upload(const char *filename, const uint8_t *data, uint32_t size);

/**
 * @brief 启动字体转换任务
 * @param font_name 字体名称
 * @param ttf_filename TTF文件名
 * @param font_size 目标字体大小
 * @param charset 字符集 ("GB2312", "GBK", "UTF-8"等)
 * @return 成功返回true
 */
bool ttf_font_start_conversion(const char *font_name, const char *ttf_filename, 
                                uint16_t font_size, const char *charset);

/**
 * @brief 获取转换进度
 * @param font_name 字体名称
 * @return 进度百分比 (0-100)
 */
float ttf_font_get_conversion_progress(const char *font_name);

/**
 * @brief 查询可用的字库列表
 * @param buffer 输出缓冲区 (JSON格式)
 * @param buffer_size 缓冲区大小
 * @return 成功返回true
 */
bool ttf_font_list_available(char *buffer, uint32_t buffer_size);

/**
 * @brief 查询已上传的TTF文件列表
 * @param buffer 输出缓冲区 (JSON格式)
 * @param buffer_size 缓冲区大小
 * @return 成功返回true
 */
bool ttf_font_list_uploaded(char *buffer, uint32_t buffer_size);

/**
 * @brief 删除字库文件
 * @param font_name 字体名称
 * @return 成功返回true
 */
bool ttf_font_delete(const char *font_name);

/**
 * @brief 删除TTF文件
 * @param ttf_filename TTF文件名
 * @return 成功返回true
 */
bool ttf_font_delete_ttf(const char *ttf_filename);

/**
 * @brief 查询字体信息
 * @param font_name 字体名称
 * @param buffer 输出缓冲区 (JSON格式)
 * @param buffer_size 缓冲区大小
 * @return 成功返回true
 */
bool ttf_font_get_info(const char *font_name, char *buffer, uint32_t buffer_size);

/**
 * @brief 验证BIN字库文件
 * @param bin_path BIN文件路径
 * @return 有效返回true
 */
bool ttf_font_validate_bin(const char *bin_path);

/**
 * @brief 切换当前使用的字库
 * @param font_name 字体名称
 * @return 成功返回true
 */
bool ttf_font_switch_to(const char *font_name);

/**
 * @brief 获取当前使用的字库名称
 * @return 字体名称指针
 */
const char *ttf_font_get_current(void);

/**
 * @brief 获取字体文件路径
 * @param font_name 字体名称
 * @return 文件路径
 */
const char *ttf_font_get_path(const char *font_name);

/**
 * @brief 获取字体大小
 * @param font_name 字体名称
 * @return 字体大小
 */
uint16_t ttf_font_get_size(const char *font_name);

/**
 * @brief 清理过期的转换任务
 */
void ttf_font_cleanup_tasks(void);

/**
 * @brief 获取字体管理系统状态
 * @param buffer 输出缓冲区 (JSON格式)
 * @param buffer_size 缓冲区大小
 * @return 成功返回true
 */
bool ttf_font_get_status(char *buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // TTF_FONT_MANAGER_H

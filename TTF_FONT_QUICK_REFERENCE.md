# TTF 字体管理系统 - 快速参考

## 核心 API 端点 (8 个)

| 方法 | 端点 | 功能 |
|------|------|------|
| GET | `/api/fonts/list` | 列出所有可用字库 |
| GET | `/api/fonts/list-uploaded` | 列出已上传的 TTF 文件 |
| GET | `/api/fonts/info?font_name=NAME` | 获取字体详细信息 |
| GET | `/api/fonts/status` | 获取系统状态 |
| GET | `/api/fonts/progress?font_name=NAME` | 获取转换进度 |
| GET | `/api/fonts/switch?font_name=NAME` | 切换当前字体 |
| POST | `/api/fonts/upload` | 上传并转换 TTF |
| DELETE | `/api/fonts/delete?font_name=NAME` | 删除字体 |

## 快速命令

### 获取字体列表
```bash
curl http://192.168.1.100:8848/api/fonts/list
```

### 上传和转换字体
```bash
curl -X POST http://192.168.1.100:8848/api/fonts/upload \
  -H "Content-Type: application/json" \
  -d '{
    "font_name": "fangsong_GB2312",
    "ttf_filename": "fangsong.ttf",
    "font_size": 16,
    "charset": "GB2312"
  }'
```

### 切换字体
```bash
curl "http://192.168.1.100:8848/api/fonts/switch?font_name=fangsong_GB2312"
```

### 查看系统状态
```bash
curl http://192.168.1.100:8848/api/fonts/status
```

### 删除字体
```bash
curl -X DELETE "http://192.168.1.100:8848/api/fonts/delete?font_name=fangsong_GB2312"
```

## 文件系统

```
/sd/fonts/
├── ttf/          → 上传的 TTF 源文件
├── bin/          → 生成的 BIN 字库文件
└── config.json   → 系统配置
```

## 支持的字体

预设字体:
- `fangsong_GB2312` - 仿宋_GB2312
- `ComicSansMSV3` - Comic Sans MS V3
- `ComicSansMSBold` - Comic Sans MS Bold

自定义: 支持任何 TTF 文件上传

## 编译

```bash
idf.py fullclean
idf.py build
idf.py flash
idf.py monitor
```

## 主要类和函数

### `ttf_font_manager.h` - 核心 API

```c
// 初始化系统
bool ttf_font_manager_init(void);

// 字体转换
bool ttf_font_start_conversion(const char *font_name, 
                                const char *ttf_filename, 
                                uint16_t font_size, 
                                const char *charset);

// 字体查询
bool ttf_font_list_available(char *buffer, uint32_t buffer_size);

// 字体切换
bool ttf_font_switch_to(const char *font_name);

// 字体删除
bool ttf_font_delete(const char *font_name);
```

### `ttf_font_api.h` - Web API

```c
// 注册所有 API 端点
esp_err_t register_ttf_font_apis(httpd_handle_t server);

// 注销 API 端点
void unregister_ttf_font_apis(httpd_handle_t server);
```

## JSON 响应格式

### 成功响应
```json
{
  "status": "success",
  "message": "操作描述",
  "data": {...}
}
```

### 错误响应
```json
{
  "status": "error",
  "message": "错误描述"
}
```

## 状态码

- 200: 请求成功
- 400: 请求参数错误
- 404: 字体或资源未找到
- 500: 服务器内部错误

## 常用操作流程

### 1. 安装新字体

```
1. 获取 TTF 文件
2. 调用 POST /api/fonts/upload 转换
3. 使用 GET /api/fonts/list 验证
4. 调用 GET /api/fonts/switch 切换使用
```

### 2. 切换字体

```
1. 调用 GET /api/fonts/list 查看可用字体
2. 调用 GET /api/fonts/switch?font_name=X
```

### 3. 清理旧字体

```
1. 调用 GET /api/fonts/list 查看已有字体
2. 调用 DELETE /api/fonts/delete?font_name=X
```

## 默认配置

| 项目 | 默认值 |
|------|--------|
| 字体大小 | 16px |
| 字符集 | GB2312 |
| SD 路径 | /sd/fonts/ |
| 最大字体数 | 16 |

## 错误处理

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| "Failed to allocate memory" | 内存不足 | 增加 heap_size |
| "SD card not idle" | SD 卡繁忙 | 等待后重试 |
| "Not enough space" | 空间不足 | 删除旧文件 |
| "TTF file not found" | 文件不存在 | 检查上传 |
| "Invalid JSON" | JSON 格式错误 | 检查请求格式 |

## 注意事项

⚠️ **重要**:
1. TTF 文件必须是有效的字体文件
2. 大文件转换可能耗时较长
3. 转换中断会导致 BIN 文件损坏
4. 需要足够的 SD 卡空间 (约 GB 级)
5. 系统初始化时会扫描已有字库

## 开发信息

**代码位置**:
- `components/grbl_esp32s3/Grbl_Esp32/src/BL_add/ink_screen/ttf_font_*.{h,cpp}`

**集成位置**:
- WebServer 在 HTTP 启动时自动初始化

**日志标记**:
- TTF_FontMgr - 字体管理器
- TTF_FontAPI - Web API

## 性能指标

| 操作 | 耗时 |
|------|------|
| 初始化系统 | ~100ms |
| 列表查询 | ~10ms |
| 字体切换 | ~20ms |
| TTF 转 BIN | 1-5分钟 |

## 版本信息

**版本**: 2.0.0  
**发布日期**: 2024-12-15  
**状态**: 生产就绪 ✅

---

更多详情见 `TTF_FONT_SYSTEM_GUIDE.md`

# 双模式快速启动指南

## 概述

`web_layout.html` 现已支持 **PC 单机模式** 和 **ESP32 设备模式** 的自动切换。系统会优先尝试使用 PC 端接口，如果不可用则自动降级到 ESP32 或 Python 后端。

## 快速开始

### 方式 1：PC 单机模式（推荐开发时使用）

#### 步骤 1：启动 Flask 配置服务
```bash
cd g:\A_BL_Project\inkScree_fuben
python config_server.py
```

**输出示例**：
```
Starting Config Server...
Resource directory: g:\A_BL_Project\inkScree_fuben\components\resource
Serving on http://localhost:5001

=== PC 端专用 API ===
  GET  http://localhost:5001/api/layout?screen_type=main|vocab
  GET  http://localhost:5001/api/config/focus?screen_type=main|vocab
  ...
=== ESP32 兼容 API ===
  GET  http://localhost:5001/getlayout                      (主界面布局)
  POST http://localhost:5001/setlayout                      (保存主界面布局)
  ...
```

#### 步骤 2：在 VS Code 中打开 web_layout_standalone.html

```
文件 → 打开文件 → web_layout_standalone.html
右键 → 使用 Live Server 打开
或直接在浏览器中打开：file:///path/to/web_layout_standalone.html
```

#### 步骤 3：验证连接
- 打开浏览器控制台 (F12 → Console)
- 执行布局操作（如加载、保存）
- 查看日志输出：
  ```
  ✓ PC endpoint success: /getlayout
  ✓ PC endpoint success: /setlayout
  ```

### 方式 2：ESP32 设备模式

#### 步骤 1：编译并烧录固件
```bash
idf.py build
idf.py flash
idf.py monitor
```

#### 步骤 2：连接到设备
```
浏览器打开：http://192.168.0.2:8848/
或通过 USB 串口连接
```

#### 步骤 3：自动降级
- 系统会优先尝试连接 PC 服务 (localhost:5001)
- 5 秒超时后自动降级到 ESP32 本地接口
- 查看日志：
  ```
  ⚠ PC endpoint failed: PC endpoint timeout
  ✓ ESP32 endpoint success: /getlayout
  ```

## 功能对比

| 功能 | PC 单机模式 | ESP32 设备模式 |
|------|----------|------------|
| 布局编辑 | ✅ | ✅ |
| 焦点配置 | ✅ | ✅ |
| 子数组配置 | ✅ | ✅ |
| 字体转换* | ✅ | ⚠️ (需Python服务) |
| 图像转换* | ✅ | ⚠️ (需Python服务) |
| 文件管理 | ✅ (本地) | ✅ (SD卡) |
| 图标加载 | ✅ | ✅ |

\* 字体/图像转换需要额外的 Python 后端服务 (localhost:5000)

## 接口优先级

### 1. 布局相关接口
```
优先级链：PC (localhost:5001) → ESP32 (http://192.168.0.2:8848)
超时设置：5 秒
```

可用接口：
- `GET /getlayout` - 获取主界面布局
- `POST /setlayout` - 保存主界面布局
- `GET /getvocablayout` - 获取单词界面布局
- `POST /setvocablayout` - 保存单词界面布局

### 2. 焦点配置接口
```
优先级链：PC (localhost:5001) → ESP32
```

可用接口：
- `GET /getfocusconfig?screen_type=main|vocab`
- `POST /setfocusconfig`

### 3. 子数组配置接口
```
优先级链：PC (localhost:5001) → ESP32
```

可用接口：
- `GET /getsubarrayconfig?screen_type=main|vocab`
- `POST /setsubarrayconfig`

### 4. 图标加载接口
```
优先级链：PC (/api/icon/binary/<index>) → ESP32 (/api/icon/data/<index>)
```

### 5. Python 服务接口（字体/图像转换）
```
优先级链：PC (localhost:5001) → Python (localhost:5000)
```

可用接口：
- `/convert_ttf_to_bin` - TTF 字体转 BIN
- `/convert_image_to_bin` - 图像转 BIN
- `/download_bin_file` - 下载 BIN 文件
- `/save_font_file` - 保存字体
- `/save_bitmap_file` - 保存位图
- `/list_fonts` - 列出字体
- `/list_bitmaps` - 列出位图
- `/get_font_file` - 获取字体文件
- `/get_bitmap_file` - 获取位图文件

## 调试技巧

### 查看日志输出
打开浏览器开发者工具 (F12) → Console 标签页，可以看到所有 API 调用的详细日志：

```javascript
// PC 端成功
✓ PC endpoint success: /getlayout

// PC 端失败，自动降级
⚠ PC endpoint failed (/getlayout): PC endpoint timeout
✓ ESP32 endpoint success: /getlayout

// 两端都失败
✗ Both endpoints failed. PC: timeout, ESP32: 404 Not Found
```

### 强制使用 ESP32 模式
停止 Flask 服务：
```bash
# 关闭 Flask 服务
Ctrl+C (在 config_server.py 运行的终端)
```

然后刷新浏览器，系统会自动尝试 ESP32 接口。

### 强制使用 PC 模式
启动 Flask 服务：
```bash
python config_server.py
```

然后从 PC 本地文件打开 `web_layout_standalone.html`：
```
file:///g:/A_BL_Project/inkScree_fuben/web_layout_standalone.html
```

## 故障排查

### 问题 1：显示 "PC endpoint timeout"
**原因**：Flask 服务未运行或端口被占用
**解决**：
```bash
# 检查服务是否运行
netstat -ano | findstr :5001

# 如果端口被占用，找到对应的 PID 并结束进程
taskkill /PID <PID> /F

# 重新启动 Flask 服务
python config_server.py
```

### 问题 2：显示 "Both endpoints failed"
**原因**：PC 和 ESP32 都不可用
**解决**：
- 检查 Flask 服务是否运行 (localhost:5001)
- 检查 ESP32 设备是否在线
- 检查网络连接
- 查看浏览器控制台的详细错误信息

### 问题 3：文件夹路径不存在
**原因**：`components/resource/` 目录不存在
**解决**：
```bash
mkdir components\resource
mkdir components\resource\icon
mkdir components\resource\fonts
mkdir components\resource\bitmap
```

## 相关文件

- `web_layout_standalone.html` - PC 单机版本（含完整功能）
- `components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html` - ESP32 嵌入版本（5700+ 行）
- `config_server.py` - PC Flask 后端服务
- `API_FALLBACK_IMPLEMENTATION.md` - 详细的技术文档

## 性能指标

| 操作 | PC 模式 | ESP32 模式 |
|------|--------|----------|
| 加载布局 | < 100ms | 200-500ms |
| 保存布局 | < 100ms | 300-800ms |
| 加载图标 | 即时 | 50-200ms |
| 转换字体 | 2-10s | N/A |
| 转换图像 | 1-5s | N/A |

## 下一步

1. **完整测试**：在 PC 和 ESP32 两种环境下测试所有功能
2. **性能优化**：如需加快传输速度，可以考虑压缩算法
3. **功能扩展**：可以基于此框架添加更多 API 端点
4. **错误处理**：根据实际使用情况完善错误提示

## 支持

如有问题，请查看：
- 浏览器控制台日志（F12 → Console）
- Flask 服务输出日志
- `API_FALLBACK_IMPLEMENTATION.md` 技术文档

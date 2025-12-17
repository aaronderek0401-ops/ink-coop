# Web 配置文件系统实现总结

## 问题描述

原本的系统使用 ESP32 设备上的 SD 卡来存储焦点配置和子数组配置。用户需要修改为从本地电脑的 `components/resource/` 文件夹读写配置文件，以便在开发过程中更方便地编辑和管理配置。

## 解决方案

### 1. 创建配置文件服务器 (`config_server.py`)

一个 Python Flask 应用，运行在 `localhost:5001`，提供以下 API 端点：

- **GET `/api/config/focus`** - 读取焦点配置
- **POST `/api/config/focus`** - 保存焦点配置  
- **GET `/api/config/subarray`** - 读取子数组配置
- **POST `/api/config/subarray`** - 保存子数组配置

所有文件操作都在 `components/resource/` 目录中进行。

### 2. 修改 `web_layout.html`

更新以下四个函数，使用新的 Flask API 端点：

| 函数名 | 旧端点 | 新端点 |
|--------|--------|--------|
| `loadFocusConfig()` | `/getfocusconfig` | `http://localhost:5001/api/config/focus` |
| `saveFocusConfig()` | `/setfocusconfig` | `http://localhost:5001/api/config/focus` |
| `loadSubArrayConfig()` | `/getsubarrayconfig` | `http://localhost:5001/api/config/subarray` |
| `saveSubArrayConfig()` | `/setsubarrayconfig` | `http://localhost:5001/api/config/subarray` |

### 3. 创建辅助脚本

#### `init_resource.py`
初始化 `components/resource/` 文件夹和示例配置文件。

#### `compress_html.py`
压缩 `web_layout.html` 为 gzip 格式（`web_layout.html.gz`）。

#### `start_config_server.bat`
Windows 批处理脚本，用于快速启动配置服务器。

## 配置文件位置

```
components/resource/
├── main_focusable_rects_config.json      # 主界面焦点配置
├── vocab_focusable_rects_config.json     # 单词界面焦点配置
├── main_subarray_config.json             # 主界面子数组配置
└── vocab_subarray_config.json            # 单词界面子数组配置
```

## 使用工作流

### 开发阶段

1. **启动配置服务器**
   ```bash
   python config_server.py
   ```
   或在 Windows 中双击 `start_config_server.bat`

2. **打开 Web UI**
   - 在浏览器中打开 web_layout.html
   - 配置服务器需要正在运行

3. **加载配置**
   - 点击"从设备加载焦点配置"按钮
   - 配置从 `components/resource/` 加载到浏览器内存

4. **编辑配置**
   - 在 Web UI 中进行修改（复选框等）
   - 修改暂时保存在浏览器内存中

5. **保存配置**
   - 点击"保存焦点配置到设备"按钮
   - 配置写入 `components/resource/` 中的 JSON 文件

6. **重新编译（如果需要）**
   - 修改后的配置文件会被编译到 ESP32
   ```bash
   idf.py build
   ```

## 技术细节

### 跨域请求 (CORS)

`config_server.py` 启用了 CORS，允许从任何源访问 API（仅用于开发）。

### 错误处理

- 如果配置文件不存在，API 返回空配置
- 首次保存会自动创建文件
- 所有文件操作都包含错误处理和日志记录

### 文件格式

焦点配置 JSON：
```json
{
  "count": 15,
  "focusable_indices": [0, 1, 2, ...],
  "screen_type": "main"
}
```

子数组配置 JSON：
```json
{
  "sub_arrays": [
    {
      "parent_index": 0,
      "sub_indices": [5, 6, 7],
      "sub_count": 3
    }
  ],
  "screen_type": "main"
}
```

## 文件变更清单

### 新创建的文件
- `config_server.py` - Flask 配置服务器
- `init_resource.py` - 初始化脚本
- `compress_html.py` - HTML 压缩脚本
- `start_config_server.bat` - Windows 启动脚本
- `CONFIG_SERVER_README.md` - 详细文档

### 修改的文件
- `components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html`
  - 修改 `loadFocusConfig()` 函数
  - 修改 `saveFocusConfig()` 函数
  - 修改 `loadSubArrayConfig()` 函数
  - 修改 `saveSubArrayConfig()` 函数
  - 重新压缩为 `web_layout.html.gz`

### 现有的文件（保持不变）
- `components/grbl_esp32s3/Grbl_Esp32/src/WebUI/WebServer.cpp`
  - 继续包含原有的 `/getfocusconfig`, `/setfocusconfig`, `/getsubarrayconfig`, `/setsubarrayconfig` 端点
  - 这些端点在 ESP32 设备上仍然可用，但开发时使用本地配置服务器

## 后续考虑

### 可选：清理 WebServer.cpp

如果完全不需要 ESP32 上的配置端点，可以从 `WebServer.cpp` 中删除：
- `Web_focus_config_handler()` 函数
- `Web_subarray_config_handler()` 函数
- 相应的路由注册

这将节省设备上的代码空间，但对功能没有影响（开发时使用本地服务器）。

### 可选：生产部署

如果需要在设备上部署，可以：
1. 将 `config_server.py` 改为 FastAPI 或其他轻量级框架
2. 在 ESP32 上运行配置服务器（如果有足够的内存）
3. 或者使用 OTA 更新机制来更新配置文件

## 优势

✅ **开发效率高** - 无需每次编译烧录就能修改配置
✅ **文件在项目目录** - 配置文件版本控制友好
✅ **浏览器操作** - 熟悉的 Web UI 界面
✅ **快速迭代** - 即时保存和加载
✅ **易于备份** - 配置文件在本地目录，易于备份

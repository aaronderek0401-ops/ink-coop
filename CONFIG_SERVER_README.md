# 配置文件服务器使用说明

## 概述

`config_server.py` 是一个 Python Flask 服务器，用于在开发过程中，web UI 与本地项目目录 `components/resource/` 文件夹进行交互。

## 文件结构

配置文件存储在项目的 `components/resource/` 目录下：

```
components/resource/
├── main_focusable_rects_config.json      # 主界面焦点配置
├── vocab_focusable_rects_config.json     # 单词界面焦点配置
├── main_subarray_config.json             # 主界面子数组配置
└── vocab_subarray_config.json            # 单词界面子数组配置
```

## 安装依赖

首先安装所需的 Python 包：

```bash
pip install flask flask-cors
```

## 启动服务器

1. 在项目根目录运行：

```bash
python config_server.py
```

服务器将启动在 `http://localhost:5001`

2. 使用浏览器打开 web UI，确保在打开 web_layout.html 的设备中可以访问 `http://localhost:5001`

## API 端点

### 焦点配置 (Focus Config)

**GET `/api/config/focus`**
- 参数: `screen_type=main|vocab` (默认: vocab)
- 返回: JSON 配置
- 示例: `GET http://localhost:5001/api/config/focus?screen_type=main`

**POST `/api/config/focus`**
- 参数: `screen_type=main|vocab` (从请求体中的 `screen_type` 字段获取)
- 请求体: JSON 配置数据
- 返回: `{"status": "success"}`

### 子数组配置 (Subarray Config)

**GET `/api/config/subarray`**
- 参数: `screen_type=main|vocab` (默认: vocab)
- 返回: JSON 配置
- 示例: `GET http://localhost:5001/api/config/subarray?screen_type=main`

**POST `/api/config/subarray`**
- 参数: `screen_type=main|vocab` (从请求体中的 `screen_type` 字段获取)
- 请求体: JSON 配置数据
- 返回: `{"status": "success"}`

### 其他端点

**GET `/api/config/list`**
- 列出所有配置文件及其存在状态

**GET `/api/health`**
- 健康检查，返回服务器状态和 resource 目录信息

**GET `/`**
- 返回 API 文档和端点列表

## 配置文件格式

### 焦点配置 (Focusable Rects Config)

```json
{
  "count": 15,
  "focusable_indices": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14],
  "screen_type": "main"
}
```

### 子数组配置 (Subarray Config)

```json
{
  "sub_arrays": [
    {
      "parent_index": 0,
      "sub_indices": [5, 6, 7, 8],
      "sub_count": 4
    }
  ],
  "screen_type": "main"
}
```

## web_layout.html 集成

web_layout.html 已修改为使用以下 API 端点：

- `loadFocusConfig()` → GET `/api/config/focus`
- `saveFocusConfig()` → POST `/api/config/focus`
- `loadSubArrayConfig()` → GET `/api/config/subarray`
- `saveSubArrayConfig()` → POST `/api/config/subarray`

所有请求都指向 `http://localhost:5001`

## 使用工作流

1. **启动配置服务器**
   ```bash
   python config_server.py
   ```

2. **打开 web UI**
   - 在浏览器中打开 web_layout.html
   - 确保 ESP32 设备已连接并提供 web UI

3. **加载配置**
   - 点击"从设备加载焦点配置"按钮
   - 配置服务器会从 `components/resource/` 读取 JSON 文件

4. **修改配置**
   - 在 web UI 中进行修改
   - 修改会暂时存储在浏览器内存中

5. **保存配置**
   - 点击"保存焦点配置到设备"按钮
   - 配置会写入 `components/resource/` 文件夹

6. **重新编译**
   - 修改后的配置文件可以重新编译到 ESP32 中
   - 使用 `idf.py build` 编译项目

## 故障排除

### 连接失败

如果 web UI 无法连接到配置服务器，检查：
- 服务器是否正在运行 (`python config_server.py`)
- 防火墙是否阻止了 5001 端口
- web UI 是否在可以访问 localhost:5001 的地方打开

### 文件权限

确保 `components/resource/` 文件夹对当前用户有读写权限

### 创建配置文件

如果配置文件不存在，API 会返回空配置。首次保存时会创建文件。

## 安全性注意

此服务器仅用于开发目的，不应在生产环境中使用。它允许无认证访问本地文件系统。

## 跨域 (CORS)

配置服务器已启用 CORS，允许来自任何源的请求。这对开发很有用，但在生产环境中应该限制。

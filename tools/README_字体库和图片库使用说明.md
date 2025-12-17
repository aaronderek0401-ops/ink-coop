# 字体库和图片库管理功能使用说明

## 功能概述

本功能允许您通过 Web 界面管理本地的字体文件（.ttf/.otf）和位图文件（.h），并将它们上传到 ESP32 的 SD 卡。

## 目录结构

```
tools/
├── fonts/              # 存放 TTF/OTF 字体文件
├── bitmap/             # 存放转换后的 .h 位图文件
├── ttf_to_gfx_webservice.py  # Python Web 服务
└── README_字体库和图片库使用说明.md
```

## 使用步骤

### 1. 启动 Python Web 服务

```powershell
cd G:\A_BL_Project\inkScree_fuben
python tools\ttf_to_gfx_webservice.py
```

服务启动后会显示：
```
============================================================
TTF 转 GFX Web 服务启动中...
============================================================
访问: http://localhost:5000
API 端点:
  - 列出字体库: http://localhost:5000/list_fonts
  - 列出图片库: http://localhost:5000/list_bitmaps
  ...
============================================================
```

### 2. 访问 Web 界面

在浏览器中访问：
```
http://192.168.0.4:8848
```

（请确保 ESP32 设备已连接并运行）

### 3. 字体库管理

#### 3.1 准备字体文件

将 `.ttf` 或 `.otf` 字体文件复制到：
```
G:\A_BL_Project\inkScree_fuben\tools\fonts\
```

#### 3.2 查询字体库

1. 在 Web 界面中找到 **"📚 字体库管理"** 区域
2. 点击 **"🔍 查询字体库"** 按钮
3. 系统会列出 `tools/fonts/` 文件夹下的所有字体文件

#### 3.3 手动导入字体

1. 在字体列表中，点击某个字体旁边的 **"手动导入"** 按钮
2. 系统会将 `#include` 路径复制到剪贴板
3. 在 `ink_screen.cpp` 中手动添加 `#include` 语句和显示代码

#### 3.4 自动导入字体（推荐）

1. 点击 **"🚀 自动导入"** 按钮
2. 输入要显示的文字（例如："测试文字"）
3. 输入 X 坐标（例如：10）
4. 输入 Y 坐标（例如：300）
5. 系统会自动修改 `ink_screen.cpp`，添加 `#include` 和显示代码
6. 重新编译并上传程序

### 4. 图片库管理

#### 4.1 准备位图文件

有两种方式获取 `.h` 位图文件：

**方式一：通过 Web 界面转换**
1. 在 **"图片转位图工具"** 区域
2. 上传图片文件（JPG/PNG/BMP）
3. 设置目标宽高
4. 选择处理模式（抖动算法效果更好）
5. 点击 **"转换为位图"**
6. 点击 **"下载 .h 文件"** → **自动保存到 `tools/bitmap/` 文件夹**

**方式二：手动复制**
- 将现有的 `.h` 位图文件复制到 `tools/bitmap/` 文件夹

#### 4.2 查询图片库

1. 在 Web 界面中找到 **"🖼️ 图片库管理"** 区域
2. 点击 **"🔍 查询图片库"** 按钮
3. 系统会列出 `tools/bitmap/` 文件夹下的所有 `.h` 文件

#### 4.3 预览位图

1. 在位图列表中，点击 **"预览"** 按钮
2. 在预览区域查看位图的黑白效果
3. 显示位图的宽度、高度、文件大小等信息

#### 4.4 手动导入位图

1. 点击 **"手动导入"** 按钮
2. 系统会将 `#include` 路径复制到剪贴板
3. 在 `ink_screen.cpp` 中手动添加代码

#### 4.5 自动导入位图（推荐）

1. 点击 **"🚀 自动导入"** 按钮
2. 输入 X 坐标
3. 输入 Y 坐标
4. 系统自动修改代码
5. 重新编译并上传程序

### 5. 上传到 SD 卡（计划中）

> **注意**：此功能需要进一步开发，目前可以通过现有的 `/upload` 接口手动上传。

如需上传文件到 ESP32 SD 卡：

1. 使用 Web 界面的 **"SD卡文件管理"** 功能
2. 手动上传字体或位图文件

## API 接口说明

### 列出字体库

```
GET http://localhost:5000/list_fonts
```

返回：
```json
{
  "success": true,
  "fonts": [
    {
      "name": "font1.ttf",
      "size": 12345,
      "size_kb": 12.05,
      "path": "G:\\...\\tools\\fonts\\font1.ttf"
    }
  ],
  "count": 1,
  "directory": "G:\\...\\tools\\fonts"
}
```

### 列出图片库

```
GET http://localhost:5000/list_bitmaps
```

返回：
```json
{
  "success": true,
  "bitmaps": [
    {
      "name": "bitmap1.h",
      "size": 5678,
      "size_kb": 5.54,
      "width": 128,
      "height": 64,
      "path": "G:\\...\\tools\\bitmap\\bitmap1.h"
    }
  ],
  "count": 1,
  "directory": "G:\\...\\tools\\bitmap"
}
```

## 常见问题

### 1. 点击"查询字体库/图片库"没有反应

**原因**：Python 服务未启动

**解决方案**：
```powershell
cd G:\A_BL_Project\inkScree_fuben
python tools\ttf_to_gfx_webservice.py
```

### 2. 显示"❌ 查询失败: Failed to fetch"

**原因**：浏览器跨域限制或 Python 服务未运行

**解决方案**：
- 确保 Python 服务正在运行（端口 5000）
- 检查防火墙设置
- 使用 Chrome 浏览器，F12 查看控制台错误信息

### 3. 下载的 .h 文件没有自动保存到 bitmap 文件夹

**原因**：此功能需要在下载逻辑中添加自动保存代码

**临时方案**：
- 下载后手动复制 `.h` 文件到 `tools/bitmap/` 文件夹

## 开发者信息

- **Python 服务端口**：5000
- **ESP32 HTTP 服务端口**：8848
- **API 框架**：Flask + CORS
- **前端框架**：原生 JavaScript

## 更新日志

### 2025-12-13
- ✅ 修改 `list_fonts` API，扫描 `tools/fonts/` 文件夹
- ✅ 修改 `list_bitmaps` API，扫描 `tools/bitmap/` 文件夹
- ✅ 前端界面已存在，可直接使用
- ⏸️ SD卡上传功能（待实现）
- ⏸️ 下载.h文件自动保存到本地（待实现）

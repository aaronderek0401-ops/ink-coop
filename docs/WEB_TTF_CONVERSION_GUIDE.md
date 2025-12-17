# Web界面 TTF 转 GFX 使用指南

## 📋 概述

`web_layout.html` 现在支持通过 Python 后端服务将 TTF 字体转换为 Adafruit GFX 格式。

---

## 🚀 快速开始

### 方法 1: 使用 Python Web 服务（推荐）⭐⭐⭐⭐⭐

#### 步骤 1: 安装依赖

```powershell
pip install Flask flask-cors Pillow
```

#### 步骤 2: 启动服务

```powershell
cd G:\A_BL_Project\inkScree_fuben
python tools/ttf_to_gfx_webservice.py
```

你会看到：
```
============================================================
TTF 转 GFX Web 服务启动中...
============================================================
访问: http://localhost:5000
API 端点: http://localhost:5000/convert_ttf_to_gfx
...
```

#### 步骤 3: 在 Web 界面使用

1. 打开 `web_layout.html`（在浏览器中）
2. 找到 **"🔤 TTF 字体管理"** 区域
3. 上传 TTF 字体文件（如 `simhei.ttf`）
4. 输入要转换的文字（如 `爆裂`）
5. 选择字体大小（如 `16`）
6. 点击 **"转换为 GFX 格式"** 按钮
7. 自动下载生成的 `.h` 文件

---

### 方法 2: 使用本地 Python 脚本⭐⭐⭐⭐

如果不想启动 Web 服务，可以直接运行脚本：

```powershell
# 生成 "爆裂" 两个字
python tools/generate_bailie_font.py

# 生成自定义字符
# 编辑 tools/generate_chinese_font.py 修改 CHARS 变量
python tools/generate_chinese_font.py
```

---

### 方法 3: 使用在线工具⭐⭐⭐

访问：https://rop.nl/truetype2gfx/

1. 上传 TTF 字体
2. Font size: `16`
3. **Custom character list** 粘贴你的字符
4. 点击 "Get GFX font file"
5. 手动添加头文件（见 `docs/TTF_Font_Guide.md`）

---

## 📁 文件说明

### Python Web 服务

| 文件 | 用途 |
|------|------|
| `tools/ttf_to_gfx_webservice.py` | Flask Web 服务 |
| `tools/generate_bailie_font.py` | 生成 "爆裂" 字体 |
| `tools/generate_chinese_font.py` | 生成自定义中文字体 |

### 文档

| 文件 | 内容 |
|------|------|
| `docs/TTF_Font_Guide.md` | 完整 TTF 转换教程 |
| `docs/CHINESE_DISPLAY_SOLUTIONS.md` | 中文显示方案对比 |
| `docs/CHINESE_FONT_COMPATIBILITY.md` | 字体通用性说明 |

---

## 🔧 Web 服务 API

### 端点：`POST /convert_ttf_to_gfx`

#### 请求格式

```json
{
    "ttf_base64": "base64编码的TTF文件内容",
    "chars": "爆裂",
    "font_size": 16,
    "font_name": "bailie"
}
```

#### 响应格式

```json
{
    "success": true,
    "h_file": "#ifndef BAILIE16PT7B_H\n...",
    "char_count": 2,
    "file_size": 62
}
```

---

## 💡 使用示例

### 示例 1: 转换 "爆裂" 两个字

1. 启动服务：`python tools/ttf_to_gfx_webservice.py`
2. 打开 Web 界面
3. 上传 `simhei.ttf`
4. 输入文字：`爆裂`
5. 字体大小：`16`
6. 点击转换
7. 下载 `bailie16pt7b.h`

### 示例 2: 转换温湿度显示字符

1. 输入文字：`温度湿度电量0123456789°C%`
2. 字体大小：`16`
3. 点击转换
4. 下载 `sensor16pt7b.h`

### 示例 3: 转换菜单文字

1. 输入文字：`主菜单设置显示系统关于返回确定取消`
2. 字体大小：`20`
3. 点击转换
4. 下载 `menu20pt7b.h`

---

## ⚠️ 注意事项

### 1. 字符数量限制

- ✅ 推荐：< 100 个字符（文件 ~20KB）
- ⚠️ 警告：> 500 个字符（文件可能 >100KB）
- ❌ 避免：全部汉字（文件 >1MB，ESP32 无法承受）

### 2. 编码问题

- Web 界面会自动处理 UTF-8 编码
- 下载的 `.h` 文件已包含正确的 UTF-8 注释

### 3. 使用生成的字体

```cpp
// 1. 复制到 components/fonts/
// 2. 在 ink_screen.cpp 中包含
#include "../fonts/bailie16pt7b.h"

// 3. 使用位图方式显示
#include "../fonts/bailie_bitmap.h"
drawBaiLie(display, 10, 50, 2);  // 显示 "爆裂"
```

---

## 🐛 故障排除

### 问题 1: 无法连接到 Python 服务

**症状：** Web 界面显示 "❌ 无法连接到 Python 转换服务"

**解决：**
```powershell
# 1. 检查服务是否运行
# 应该看到类似输出：
#  * Running on http://0.0.0.0:5000

# 2. 检查端口是否被占用
netstat -ano | findstr :5000

# 3. 重启服务
python tools/ttf_to_gfx_webservice.py
```

### 问题 2: ModuleNotFoundError

**症状：** `ModuleNotFoundError: No module named 'flask'`

**解决：**
```powershell
pip install Flask flask-cors Pillow
```

### 问题 3: 下载的文件为空

**症状：** 下载的 `.h` 文件大小为 0

**解决：**
1. 检查输入的文字是否包含 TTF 字体支持的字符
2. 尝试使用英文字符测试（如 `ABC123`）
3. 检查 Python 服务控制台是否有错误信息

---

## 📚 相关资源

- [Adafruit GFX 库文档](https://learn.adafruit.com/adafruit-gfx-graphics-library)
- [GxEPD2 库文档](https://github.com/ZinggJM/GxEPD2)
- [truetype2gfx 在线工具](https://rop.nl/truetype2gfx/)
- [PIL/Pillow 文档](https://pillow.readthedocs.io/)

---

## 🎯 总结

### ✅ 推荐工作流程

1. **开发阶段**：使用 Python Web 服务快速转换测试
2. **确定字符集**：整理所有需要显示的字符
3. **生成最终字体**：一次性转换所有需要的字符
4. **集成到项目**：复制 `.h` 文件到 `components/fonts/`

### 📊 方案对比

| 方案 | 难度 | 速度 | 灵活性 | 推荐度 |
|------|------|------|--------|--------|
| Python Web 服务 | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| 本地 Python 脚本 | ⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| 在线工具 | ⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| letter&pic2doth.ipynb | ❌ | ❌ | ❌ | ❌ |

**`letter&pic2doth.ipynb` 不适合用于 TTF 转换！**

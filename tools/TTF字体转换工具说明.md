# 🔤 TTF 字体转换工具包

独立的 TTF 字体管理工具，支持图形界面和命令行两种方式。

## 📋 功能特点

✅ **图形化界面** - 无需命令行，直观易用
✅ **字体选择** - 支持 TTF、OTF 等格式
✅ **灵活参数** - 自定义字体大小（8-128pt）
✅ **字符集管理** - 预设 + 自定义字符集
✅ **多种格式** - 导出为 GFX (.h) 或 BIN 格式
✅ **实时预览** - 查看选择的字符集内容

## 🚀 快速开始

### 方式一：GUI（推荐）

**Windows 用户**：
```
双击：启动TTF字体转换工具.bat
```

或者：
```bash
python ttf_font_converter_gui.py
```

**使用步骤**：
1. 点击"📂 浏览..."选择 TTF/OTF 字体文件
2. 设置字体大小（默认 16pt）
3. 选择字符集或自定义字符
4. 选择输出格式（GFX 或 BIN）
5. **可选**：点击"📂 浏览..."指定输出路径（不指定时保存到字体文件所在目录）
6. 点击"🚀 开始转换"

### 方式二：命令行

```bash
# 使用中文字符集转换为 GFX（自动输出到字体文件目录）
python ttf_font_converter.py -i font.ttf -s 16 -c "中文字符" -f gfx

# 使用英文字符集转换为 GFX，指定输出路径
python ttf_font_converter.py -i font.ttf -s 16 -c "英文字符" -f gfx -o output.h

# 使用自定义字符集转换为 BIN，指定输出路径
python ttf_font_converter.py -i font.ttf -s 24 -c "温度湿度电量" -f bin -o output.bin

# 列出所有预设字符集
python ttf_font_converter.py --list-charsets
```

**完整命令示例**：
```bash
# 指定完整文件路径输出
python ttf_font_converter.py -i "C:\Fonts\simhei.ttf" -s 16 -c "中文字符" -f gfx -o "D:\output\myfont.h"

# 指定目录（自动生成文件名）
python ttf_font_converter.py -i "C:\Fonts\simhei.ttf" -s 16 -c "中文字符" -f gfx -o "D:\output"

# 使用相对路径
python ttf_font_converter.py -i font.ttf -s 16 -f gfx -o ./output/font.h
```

## 📖 详细说明

### 预设字符集

| 名称 | 字符数 | 用途 |
|------|--------|------|
| **中文字符** | ~21,056 个 | 完整 CJK 汉字库 + 符号 + 中文标点 |
| **英文字符** | 133 个 | 数字 (0-9) + 英文 (A-Z, a-z) + 符号 + 音标 |

### 命令行参数

| 参数 | 说明 | 默认值 | 示例 |
|------|------|-------|------|
| `-i, --input` | 输入 TTF 文件（必需） | - | `font.ttf` |
| `-s, --size` | 字体大小（像素） | 16 | `-s 24` |
| `-c, --charset` | 字符集名称或自定义 | 全组合 | `-c "常用汉字"` |
| `-f, --format` | 输出格式 | gfx | `-f bin` |
| `-o, --output` | 输出文件路径 | 自动生成 | `-o font.h` |
| `--list-charsets` | 列出所有预设 | - | - |

### 输出路径说明

**默认行为**（不指定 `-o` 参数）：
- 输出文件保存到 **字体文件所在的目录**
- 文件名自动生成：`字体名_字体大小.h` 或 `字体名_字体大小.bin`
- 例：转换 `C:\Windows\Fonts\simhei.ttf` 16pt GFX，输出为 `C:\Windows\Fonts\simhei_16pt.h`

**指定输出目录**（使用 `-o` 参数指定目录）：
- 工具自动在该目录生成文件名
- 文件名格式：`字体名_字体大小.h` 或 `字体名_字体大小.bin`
- 目录不存在时自动创建
- 例：`-o "D:\output"` 会生成 `D:\output\simhei_16pt.h`

**指定完整文件路径**（使用 `-o` 参数指定文件路径）：
- 使用自定义的文件路径和名称
- 支持绝对路径和相对路径
- 例：`-o "D:\output\myfont.h"` 或 `-o ./fonts/custom.h`

### 输出格式

#### GFX 格式 (.h 头文件)
- 用于 Adafruit GFX 库
- 包含字形数据和元数据
- 适合 Arduino/ESP32 项目
- 文件大小相对较小

#### BIN 格式 (二进制)
- 自定义二进制格式
- 包含文件头 + 字形表 + 位图数据
- 适合嵌入式系统存储
- 快速加载和使用

## 🔧 命令行示例

### 示例 1：转换中文字符为 GFX

```bash
python ttf_font_converter.py \
  -i "C:\Windows\Fonts\simhei.ttf" \
  -s 16 \
  -c "中文字符" \
  -f gfx \
  -o simhei16_chinese.h
```

### 示例 2：转换英文字符为 BIN

```bash
python ttf_font_converter.py \
  -i font.ttf \
  -s 24 \
  -c "英文字符" \
  -f bin \
  -o font_24pt.bin
```

### 示例 3：自定义字符集

```bash
python ttf_font_converter.py \
  -i font.ttf \
  -s 16 \
  -c "温度：25℃ 湿度：60%" \
  -f gfx
```

### 示例 4：列出预设字符集

```bash
python ttf_font_converter.py --list-charsets
```

输出：
```
📋 预设字符集:
============================================================

常用汉字 (95 个字符):
  测试中文字体显示正常温度湿度电量充电WiFi连接成功失败...

基础英文 (62 个字符):
  0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz

符号 (20 个字符):
  °C%:/-_.()[]{}<>@#$&*+=!?'"

扩展汉字 (155 个字符):
  ...

全组合 (300+ 个字符):
  ...
```

## 📊 文件结构

```
tools/
├── ttf_font_converter.py          # 核心库（命令行）
├── ttf_font_converter_gui.py      # GUI 应用
├── 启动TTF字体转换工具.bat       # Windows 启动脚本
└── TTF字体转换工具说明.md        # 本文档
```

## 🎯 使用场景

### 场景 1：为 ESP32 墨水屏生成中文字体

```bash
python ttf_font_converter.py \
  -i simhei.ttf \
  -s 16 \
  -c "温度 湿度 电量 WiFi 连接 成功 失败" \
  -f gfx \
  -o esp32_display_font.h
```

然后在 Arduino 代码中包含此文件。

### 场景 2：嵌入式系统字体库

```bash
python ttf_font_converter.py \
  -i font.ttf \
  -s 24 \
  -c "全组合" \
  -f bin \
  -o system_font.bin
```

将二进制文件刻录到 Flash 或 SD 卡。

### 场景 3：自定义显示内容

```bash
python ttf_font_converter.py \
  -i font.ttf \
  -s 12 \
  -c "2024年12月22日 温度：25℃ 湿度：60% 电量：85%" \
  -f gfx
```

只包含需要显示的字符，最小化文件大小。

## 💡 技术细节

### GFX 格式文件结构

```c
#ifndef simhei_16pt_H
#define simhei_16pt_H

#include <Adafruit_GFX.h>

// 字体: simhei.ttf
// 大小: 16pt
// 字符: 95

// 字形数据
const uint8_t simhei_16pt_glyphs[] = {
    0x00, 0x01, 0x02, ...  // 位图数据
};

// 字形元数据
struct Glyph {
    uint32_t code;          // Unicode 编码
    uint32_t bitmapOffset;  // 位图偏移
    uint16_t width;         // 字符宽度
    uint16_t height;        // 字符高度
    int8_t xAdvance;        // X 方向推进量
    int8_t dX, dY;          // 偏移量
};

const Glyph simhei_16pt_glyphTable[] = {
    {0x0041, 0, 16, 16, 17, 0, 0},  // 'A'
    {0x0042, 256, 16, 16, 17, 0, 0},  // 'B'
    ...
};

const uint16_t simhei_16pt_glyphCount = 95;

#endif
```

### BIN 格式文件结构

```
偏移    大小    说明
0       4       文件头: "TTFG"
4       2       字体大小
6       2       字形数量
8       4       字形表大小
12      N       字形表
        (字符编码 + 位图偏移 + 宽度 + 高度)
...     M       位图数据
```

## 🔐 常见问题

### Q1：为什么生成的文件这么大？

**A：** 位图字体本质上包含所有字符的逐像素数据。要减小文件大小：
- 降低字体大小（例如 16pt 改为 12pt）
- 减少字符数（只包含需要的字符）
- 使用 BIN 格式而不是 GFX

### Q2：可以用哪些字体？

**A：** 任何 TTF 或 OTF 格式的字体都可以，例如：
- 系统自带：simhei.ttf, songti.ttf, msyh.ttf
- 开源字体：思源黑体、文泉驿、冬青黑体
- 商业字体：微软雅黑、苹方等

### Q3：如何在 Arduino 中使用生成的字体？

**A：** 包含 .h 文件后使用 Adafruit_GFX 库：

```cpp
#include "simhei16pt7b_chinese.h"

void setup() {
    display.setFont(&simhei16pt7b_chinese);
    display.println("温度：25℃");
}
```

### Q4：支持多种语言吗？

**A：** 支持！任何 TTF 字体中的字符都可以：
- 中文、日文、韩文、阿拉伯文等
- 就是简单地在字符集中包含所需的字符

### Q5：如何添加自定义字符集？

**A：** 在 GUI 中或命令行使用 `-c` 参数：

```bash
# 直接指定字符串
python ttf_font_converter.py -i font.ttf -c "你好世界123ABC" -f gfx
```

## 📦 依赖

- Python 3.6+
- Pillow 8.0+ (用于图像处理)

安装依赖：
```bash
pip install Pillow
```

## 🎓 进阶用法

### 编程使用核心库

```python
from ttf_font_converter import TTFConverter, CHAR_SETS

# 创建转换器
converter = TTFConverter(
    font_path="simhei.ttf",
    font_size=16,
    charset=CHAR_SETS['常用汉字']
)

# 转换为 GFX
output_gfx = converter.convert_to_gfx("output.h")

# 或转换为 BIN
output_bin = converter.convert_to_bin("output.bin")

print(f"输出文件: {output_gfx}")
```

### 创建自定义字符集

```python
custom_charset = "⚡🔌📶💧☀️🌙"  # Emoji
converter = TTFConverter("font.ttf", 24, custom_charset)
output = converter.convert_to_gfx()
```

## 📞 获取帮助

### 查看命令行帮助

```bash
python ttf_font_converter.py -h
```

### 列出所有预设字符集

```bash
python ttf_font_converter.py --list-charsets
```

### GUI 中的提示

- 悬停在参数上查看工具提示
- 字符集预览实时更新
- 错误消息清晰提示问题所在

## ✅ 验证输出

### 检查 GFX 文件

```bash
# 文件应该是可读的 C 代码
type output.h
```

### 检查 BIN 文件

```bash
# 文件头应该是 "TTFG"
hexdump -C output.bin | head
```

## 📈 性能指标

| 参数 | 范围 | 建议 |
|------|------|------|
| 字体大小 | 8-128pt | 12-24pt |
| 字符数 | 1-1000+ | 50-300 |
| 文件大小 | 10KB-1MB | <100KB |
| 生成时间 | <1秒 | 通常 <0.5秒 |

## 🎉 总结

这个工具包让 TTF 字体转换变得简单方便：

✅ **GUI** - 点击即用，无需学习命令行
✅ **CLI** - 强大灵活，支持自动化脚本
✅ **多格式** - GFX 和 BIN 格式都支持
✅ **易定制** - 预设和自定义字符集随意搭配

现在开始使用吧！🚀

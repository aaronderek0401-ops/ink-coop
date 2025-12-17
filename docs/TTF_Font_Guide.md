# 墨水屏 TTF 字体显示完整教程

## 📚 目录
1. [准备工作](#准备工作)
2. [方案选择](#方案选择)
3. [Web 界面使用](#web-界面使用)
4. [字体转换详细步骤](#字体转换详细步骤)
5. [ESP32 代码集成](#esp32-代码集成)
6. [示例代码](#示例代码)
7. [常见问题](#常见问题)

---

## 🎯 准备工作

### 所需工具
- ✅ TTF/OTF 字体文件（如思源黑体、微软雅黑等）
- ✅ Web 浏览器（Chrome/Edge 推荐）
- ✅ [truetype2gfx](https://rop.nl/truetype2gfx/) 在线工具或命令行工具
- ✅ ESP32 开发环境（已配置 GXEPD2 库）

### 字体文件来源
- **免费中文字体**：
  - 思源黑体：https://github.com/adobe-fonts/source-han-sans
  - 文泉驿：https://wenq.org/
  - 站酷字体：https://www.zcool.com.cn/special/zcoolfonts/
  
- **系统字体**：
  - Windows: `C:\Windows\Fonts\`
  - macOS: `/System/Library/Fonts/`
  - Linux: `/usr/share/fonts/`

---

## 🔀 方案选择

### 方案 1: Adafruit GFX 字体（推荐）⭐⭐⭐⭐⭐

**优点：**
- ✅ GXEPD2 原生支持，无需额外库
- ✅ 内存占用小
- ✅ 渲染速度快
- ✅ 支持抗锯齿

**缺点：**
- ❌ 需要预先转换字体
- ❌ 中文字体文件较大

**适用场景：** 固定文字显示、菜单、标签等

### 方案 2: U8g2 字体 ⭐⭐⭐⭐

**优点：**
- ✅ 专为嵌入式优化
- ✅ 内置大量中文字体
- ✅ 支持多种编码

**缺点：**
- ❌ 需要额外库
- ❌ API 不同于 GFX

**适用场景：** 需要大量中文的场景

### 方案 3: FreeType 实时渲染 ⭐⭐

**优点：**
- ✅ 动态渲染，灵活性高
- ✅ 支持任意字号

**缺点：**
- ❌ 占用大量内存和 CPU
- ❌ 渲染速度慢
- ❌ 需要 PSRAM

**适用场景：** 需要动态字号或特殊字体效果

---

## 🌐 Web 界面使用

### 步骤 1: 打开布局编辑器
1. 连接到 ESP32 的 WiFi AP 或确保在同一局域网
2. 访问 `http://<ESP32_IP>/web_layout.html`
3. 找到 **🔤 TTF 字体管理** 部分

### 步骤 2: 上传字体
1. 点击"选择 TTF 字体文件"
2. 选择你的 TTF/OTF 文件
3. 系统会自动加载字体

### 步骤 3: 预览字体
1. 在"字体大小"下拉框选择字号（12/16/20/24/32pt）
2. 在"预览文字"输入框输入要测试的文字
3. 点击"预览字体"按钮
4. 查看下方的渲染效果

### 步骤 4: 获取转换指引
1. 点击"转换为 GFX 格式"按钮
2. 系统会显示推荐的转换工具链接
3. 点击"下载 .h 文件"获取模板

---

## 🔧 字体转换详细步骤

### 使用在线工具 truetype2gfx

#### 步骤 1: 访问网站
打开 https://rop.nl/truetype2gfx/

#### 步骤 2: 上传字体
- 点击 "Choose File" 选择你的 TTF 文件

#### 步骤 3: 配置参数
```
Font size: 16        # 字号（像素）
Characters: 常用字    # 要包含的字符

推荐配置:
- 英文数字: 0x20-0x7E (基本 ASCII)
- 常用中文: 使用字符列表文件
- 全部中文: 0x4E00-0x9FFF (20000+ 字符，不推荐)
```

#### 步骤 4: 生成字符列表
创建 `common_chinese.txt` 包含常用 3500 字：
```text
的一是不了人我在有他这为之大来以个中上们到说国和地也子时道出而要于就下得可你年生自会那后能对着事其里所去行过家十用发天如然作方成者多日都三小军二无同么经法当起与好看学进种将还分此心前面又定见只主没公从
（继续添加...）
```

#### 步骤 5: 下载 .h 文件
点击 "Convert" 后下载生成的头文件

### 使用命令行工具 fontconvert

#### 安装
```bash
# macOS/Linux
git clone https://github.com/adafruit/Adafruit-GFX-Library.git
cd Adafruit-GFX-Library/fontconvert
make

# Windows (需要 MinGW 或 WSL)
```

#### 使用
```bash
# 基本用法
./fontconvert YourFont.ttf 16 > MyFont16pt.h

# 指定字符范围（ASCII）
./fontconvert YourFont.ttf 16 0x20 0x7E > MyFont16pt_ASCII.h

# 中文字符范围（警告：文件会非常大！）
./fontconvert SimHei.ttf 16 0x4E00 0x4EFF > SimHei16pt_CJK1.h
```

#### 批量转换脚本
```bash
#!/bin/bash
# convert_fonts.sh

FONT_FILE="SourceHanSansCN-Regular.ttf"
SIZES=(12 16 20 24 32)

for size in "${SIZES[@]}"; do
    echo "Converting size $size..."
    ./fontconvert "$FONT_FILE" $size > "SourceHanSans${size}pt.h"
done
```

---

## 💻 ESP32 代码集成

### 步骤 1: 添加字体文件到项目
```
your_project/
├── components/
│   └── fonts/
│       ├── MyFont12pt.h
│       ├── MyFont16pt.h
│       └── MyFont24pt.h
```

### 步骤 2: 修改 ink_screen.cpp

在文件开头添加：
```cpp
#include "fonts/MyFont16pt.h"   // 包含生成的字体
```

### 步骤 3: 使用字体显示文字

```cpp
void displayTextWithCustomFont() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // 设置自定义字体
        display.setFont(&MyFont16pt);
        display.setTextColor(GxEPD_BLACK);
        
        // 显示文字
        display.setCursor(10, 30);
        display.print("Hello 你好");
        
        // 恢复默认字体
        display.setFont();  // 或 display.setFont(nullptr)
        display.setCursor(10, 60);
        display.print("Default Font");
        
    } while (display.nextPage());
}
```

---

## 📝 示例代码

### 完整示例：多字号文字显示

```cpp
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_370_GDEY037T03.h>
#include "fonts/SourceHanSans12pt.h"
#include "fonts/SourceHanSans16pt.h"
#include "fonts/SourceHanSans24pt.h"

GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(
    GxEPD2_370_GDEY037T03(14, 13, 12, 4)
);

void setup() {
    SPI.begin(48, -1, 47, -1);
    display.init(0);
    display.setRotation(1);
    
    showMultiFontDemo();
}

void showMultiFontDemo() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        int y = 20;
        
        // 12pt 字体
        display.setFont(&SourceHanSans12pt);
        display.setCursor(10, y);
        display.print("12pt: 小字体测试");
        y += 30;
        
        // 16pt 字体
        display.setFont(&SourceHanSans16pt);
        display.setCursor(10, y);
        display.print("16pt: 中等字体");
        y += 40;
        
        // 24pt 字体
        display.setFont(&SourceHanSans24pt);
        display.setCursor(10, y);
        display.print("24pt: 大字");
        y += 50;
        
        // 默认字体（英文）
        display.setFont();
        display.setCursor(10, y);
        display.print("Default: ABC123");
        
    } while (display.nextPage());
}

void loop() {
    delay(10000);
}
```

### 示例：动态切换字体

```cpp
void displayWithDifferentFonts(const char* text) {
    const GFXfont* fonts[] = {
        &SourceHanSans12pt,
        &SourceHanSans16pt,
        &SourceHanSans24pt
    };
    const char* labels[] = {"小", "中", "大"};
    
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        int y = 30;
        for (int i = 0; i < 3; i++) {
            display.setFont(fonts[i]);
            display.setCursor(10, y);
            display.print(labels[i]);
            display.print(": ");
            display.print(text);
            
            y += 20 + (12 * (i + 1));
        }
        
    } while (display.nextPage());
}
```

---

## ❓ 常见问题

### Q1: 字体文件太大，ESP32 内存不够？

**A:** 使用以下策略：
1. **只包含常用字** - 不要转换所有中文字符
2. **使用 PROGMEM** - 将字体存储在 Flash 中
3. **分文件存储** - 按场景分割字体
4. **使用外部 SPI Flash** - 存储大字体文件

```cpp
// 示例：只包含 100 个常用字
const char commonChars[] = "的一是不了人我在有他..."; // 100 字

// 使用 fontconvert 时指定
./fontconvert font.ttf 16 --include=commonChars.txt
```

### Q2: 中文显示乱码？

**A:** 检查编码问题：
```cpp
// 确保源文件使用 UTF-8 编码
// 使用字符串常量
const char* text = u8"中文测试";  // C++11 UTF-8 字符串

// 或使用 Unicode 码点
display.print("\xe4\xb8\xad");  // "中" 的 UTF-8 编码
```

### Q3: 字体显示不完整或位置不对？

**A:** 调整基线和边距：
```cpp
display.setFont(&MyFont16pt);

// 获取字体边界
int16_t x1, y1;
uint16_t w, h;
display.getTextBounds("测试", 0, 0, &x1, &y1, &w, &h);

// 调整位置
display.setCursor(10 - x1, 30 - y1);
display.print("测试");
```

### Q4: 如何优化刷新速度？

**A:** 使用局部刷新：
```cpp
// 只刷新文字区域
display.setPartialWindow(x, y, w, h);
display.firstPage();
do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&MyFont16pt);
    display.setCursor(x, y);
    display.print("更新");
} while (display.nextPage());
```

### Q5: 想要抗锯齿效果？

**A:** 使用灰度字体（需要灰度屏）或：
```cpp
// 使用更大字号 + 软件缩放
display.setFont(&MyFont24pt);
// 或使用 freetype 库实时渲染（需要 PSRAM）
```

---

## 🎓 进阶技巧

### 1. 字体回退机制
```cpp
void printWithFallback(const char* text) {
    // 尝试使用中文字体
    display.setFont(&ChineseFont16pt);
    
    // 如果字符不存在，回退到默认字体
    // (需要自己实现检测逻辑)
}
```

### 2. 动态加载字体
```cpp
// 从 SD 卡或 SPIFFS 加载字体
#include <FS.h>
#include <SPIFFS.h>

void loadFontFromSPIFFS() {
    File file = SPIFFS.open("/fonts/myfont.gfx", "r");
    // 读取并解析字体数据
}
```

### 3. 多语言支持
```cpp
enum Language {
    LANG_EN,
    LANG_CN,
    LANG_JP
};

const GFXfont* getFontForLanguage(Language lang) {
    switch(lang) {
        case LANG_CN: return &ChineseFont16pt;
        case LANG_JP: return &JapaneseFont16pt;
        default: return nullptr;  // 默认字体
    }
}
```

---

## 📖 参考资料

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [GXEPD2 Documentation](https://github.com/ZinggJM/GxEPD2)
- [truetype2gfx Tool](https://rop.nl/truetype2gfx/)
- [U8g2 Font List](https://github.com/olikraus/u8g2/wiki/fntlistall)
- [Free Chinese Fonts](https://github.com/adobe-fonts/source-han-sans/releases)

---

## 💡 总结

**推荐工作流：**
1. 使用 Web 界面预览和测试字体效果
2. 使用 truetype2gfx 在线工具转换字体（小于 1000 字符）
3. 或使用 fontconvert 命令行工具（大批量转换）
4. 将生成的 .h 文件添加到项目
5. 在代码中使用 `display.setFont()` 切换字体
6. 测试并优化显示效果

**注意事项：**
- ⚠️ 中文字体转换时只包含需要的字符
- ⚠️ 使用 PROGMEM 存储字体数据
- ⚠️ 大字号字体占用空间成倍增加
- ⚠️ 测试不同字体在墨水屏上的显示效果

祝你成功实现 TTF 字体显示！🎉

# 在线 TTF 字体转换详细步骤

## 🎯 目标
将 TTF 字体转换为 Adafruit GFX 格式的 .h 头文件，用于墨水屏显示

---

## 📝 步骤 1: 准备 TTF 字体文件

### 推荐字体（免费可商用）：

#### 中文字体：
1. **思源黑体 (Source Han Sans)**
   - 下载：https://github.com/adobe-fonts/source-han-sans/releases
   - 文件：SourceHanSansCN-Regular.ttf
   - 大小：~16MB（完整版）

2. **思源宋体 (Source Han Serif)**
   - 下载：https://github.com/adobe-fonts/source-han-serif/releases
   - 文件：SourceHanSerifCN-Regular.ttf

3. **文泉驿微米黑**
   - 下载：http://wenq.org/wqy2/index.cgi?FontGuide
   - 文件：wqy-microhei.ttc

#### 英文字体：
1. **Roboto**
   - 下载：https://fonts.google.com/specimen/Roboto
   
2. **Open Sans**
   - 下载：https://fonts.google.com/specimen/Open+Sans

#### 或使用系统自带字体：
- Windows: `C:\Windows\Fonts\simhei.ttf` (黑体)
- Windows: `C:\Windows\Fonts\simsun.ttc` (宋体)
- macOS: `/System/Library/Fonts/PingFang.ttc` (苹方)

---

## 🌐 步骤 2: 使用在线工具转换

### 工具选择

#### 🔥 推荐工具 1: truetype2gfx (最好用)
**网址：** https://rop.nl/truetype2gfx/

**特点：**
- ✅ 完全在线，无需安装
- ✅ 支持自定义字符集
- ✅ 生成标准 GFX 格式
- ✅ 可预览字形

#### 🔧 推荐工具 2: SquareLine Studio Font Converter
**网址：** https://squareline.io/

**特点：**
- ✅ 图形化界面
- ✅ 支持批量字号

---

## 📖 详细操作步骤（使用 truetype2gfx）

### 第 1 步：访问网站
在浏览器打开：https://rop.nl/truetype2gfx/

### 第 2 步：上传字体文件
点击 **"Browse..."** 按钮，选择你的 TTF 文件

### 第 3 步：配置参数

#### 基础配置：
```
Font size (px): 16          # 字体大小（像素）
```

#### 字符集配置（重要！）：

##### 选项 A：只包含 ASCII 字符（英文数字）
```
Start char: 0x20 (空格)
End char:   0x7E (~符号)
```
**生成文件大小：** ~2-3 KB

##### 选项 B：包含常用中文字符（推荐）
```
Start char: 0x4E00 (一)
End char:   0x4FFF (仿)
```
**说明：** 这只包含部分常用汉字（约 512 字）
**生成文件大小：** ~30-50 KB

##### 选项 C：使用自定义字符列表（最佳）
创建一个文本文件 `my_chars.txt`，只包含你需要的字符：
```text
测试显示温度湿度电量WiFi连接成功失败
0123456789:.-
ABCDEFGHIJKLMNOPQRSTUVWXYZ
```

然后在网站上粘贴这些字符到 "Custom character list" 框中

**生成文件大小：** ~5-10 KB（取决于字符数量）

### 第 4 步：高级选项（可选）

```
Bits per pixel: 1          # 墨水屏使用 1（黑白）
First char:     32         # ASCII 空格
```

### 第 5 步：生成并下载

1. 点击 **"Convert"** 按钮
2. 等待转换完成（可能需要几秒到几分钟）
3. 点击 **"Download"** 下载生成的 `.h` 文件
4. 文件名示例：`SourceHanSans16.h`

---

## 📂 步骤 3: 集成到项目

### 3.1 创建字体目录
```bash
mkdir -p components/fonts
```

### 3.2 放置字体文件
将下载的 `.h` 文件放入：
```
components/fonts/
├── SourceHanSans16.h
├── SourceHanSans24.h
└── SourceHanSans32.h
```

### 3.3 修改 CMakeLists.txt（如果需要）

打开 `components/grbl_esp32s3/CMakeLists.txt`，确保包含路径正确：
```cmake
set(COMPONENT_SRCS
    # ... 现有源文件 ...
)

set(COMPONENT_ADD_INCLUDEDIRS
    # ... 现有包含目录 ...
    "../fonts"  # 添加字体目录
)
```

---

## 💻 步骤 4: 在代码中使用

### 4.1 包含字体头文件

在 `ink_screen.cpp` 文件顶部添加：
```cpp
// 在其他 include 之后添加
#include "../fonts/SourceHanSans16.h"
```

### 4.2 使用字体显示文字

修改测试函数：
```cpp
void ink_screen_test_gxepd2_microsnow_213() {
    // ... 现有初始化代码 ...
    
    // 显示自定义字体
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        
        // 使用自定义字体
        display.setFont(&SourceHanSans16);  // 字体名称来自 .h 文件
        
        // 显示中文
        display.setCursor(20, 40);
        display.print("温度: 25°C");
        
        display.setCursor(20, 70);
        display.print("湿度: 60%");
        
        // 恢复默认字体
        display.setFont();
        display.setCursor(20, 100);
        display.print("Default Font");
        
    } while (display.nextPage());
}
```

---

## 🎨 步骤 5: 查看生成的 .h 文件内容

生成的头文件格式示例：
```cpp
const uint8_t SourceHanSans16Bitmaps[] PROGMEM = {
  0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0xF0, // ...字形位图数据
};

const GFXglyph SourceHanSans16Glyphs[] PROGMEM = {
  {     0,   0,   0,   8,    0,    1 },   // 0x20 ' '
  {     0,   2,  11,   4,    1,  -10 },   // 0x21 '!'
  // ... 每个字符的信息
};

const GFXfont SourceHanSans16 PROGMEM = {
  (uint8_t  *)SourceHanSans16Bitmaps,
  (GFXglyph *)SourceHanSans16Glyphs,
  0x20, 0x7E, 16  // 起始字符, 结束字符, 行高
};
```

---

## 📊 不同配置的文件大小对比

| 字符范围 | 字符数 | 16pt 大小 | 24pt 大小 | 32pt 大小 |
|---------|--------|----------|----------|----------|
| ASCII (0x20-0x7E) | 95 | ~2 KB | ~4 KB | ~7 KB |
| 常用中文 (500字) | 500 | ~25 KB | ~55 KB | ~95 KB |
| 常用中文 (1000字) | 1000 | ~50 KB | ~110 KB | ~190 KB |
| 常用中文 (3500字) | 3500 | ~175 KB | ~385 KB | ~665 KB |
| 全部中文 (20000+字) | 20000+ | ~1 MB | ~2.2 MB | ~3.8 MB |

**推荐：** 只包含实际需要的字符，控制在 100KB 以内

---

## 🔧 常见问题解决

### Q1: 转换后的文件太大？
**A:** 减少字符数量，只包含必需的字符
```
不要：0x4E00-0x9FFF (全部中文)
改为：自定义字符列表（只包含你会用到的字）
```

### Q2: 字体显示不完整？
**A:** 检查字符是否在转换范围内
```cpp
// 在 .h 文件中查找字符范围
// 例如：0x20, 0x7E, 16
// 表示只包含 ASCII 字符，不包含中文
```

### Q3: 编译时内存不足？
**A:** 使用 PROGMEM 存储（已自动生成）
```cpp
// .h 文件中应该有 PROGMEM 关键字
const uint8_t Bitmap[] PROGMEM = { ... };
```

### Q4: 需要多个字号？
**A:** 分别转换并命名区分
```
SourceHanSans12.h  (小字)
SourceHanSans16.h  (中字)
SourceHanSans24.h  (大字)
```

---

## 📝 实用字符集示例

### 温湿度显示
```text
温度湿度：0123456789.-°C%
```

### WiFi 状态
```text
WiFi连接成功失败已断开IP地址：0123456789.
```

### 电池电量
```text
电量充电中已充满低电量：0123456789%
```

### 时间日期
```text
年月日时分秒星期一二三四五六日0123456789:/-
```

### 完整示例（约 150 字符）
```text
温度湿度电量充电中已充满低电量WiFi连接成功失败已断开
IP地址年月日时分秒星期一二三四五六日
开关灯风扇空调模式自动手动设置返回确定取消
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
°C%:/-_.()[]
```

---

## ✅ 检查清单

在开始转换前，请确认：

- [ ] 已下载或准备好 TTF 字体文件
- [ ] 已列出需要显示的所有字符
- [ ] 已确定字号（建议 16pt 起步）
- [ ] 已访问转换网站 https://rop.nl/truetype2gfx/
- [ ] 已创建 `components/fonts/` 目录

转换完成后，请确认：

- [ ] 下载的 .h 文件已放入 fonts 目录
- [ ] 在代码中正确 include 字体文件
- [ ] 使用 `display.setFont(&FontName)` 设置字体
- [ ] 编译通过
- [ ] 在墨水屏上测试显示效果

---

## 🎓 下一步

完成字体转换后，你可以：

1. **创建字体库** - 转换多个字号和字重
2. **优化字符集** - 根据实际使用精简字符
3. **测试显示效果** - 在实际墨水屏上验证
4. **封装字体函数** - 创建便捷的显示函数

祝你成功！🎉

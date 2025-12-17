# 中文字体测试完整指南

## 🎯 目标
在墨水屏上显示中文字符

---

## 📋 步骤 1: 准备中文 TTF 字体

### 推荐中文字体（免费）：

1. **思源黑体** (推荐) ⭐⭐⭐⭐⭐
   - 下载：https://github.com/adobe-fonts/source-han-sans/releases
   - 文件：`SourceHanSansCN-Regular.otf` (简体中文)
   - 大小：~16MB
   - 特点：免费商用，字形优美

2. **文泉驿微米黑**
   - 下载：http://wenq.org/wqy2/index.cgi?FontGuide
   - 文件：`wqy-microhei.ttc`
   - 特点：开源免费

3. **系统自带字体**
   - Windows: `C:\Windows\Fonts\simhei.ttf` (黑体)
   - Windows: `C:\Windows\Fonts\msyh.ttc` (微软雅黑)

### 快速获取系统字体：
```powershell
# 复制黑体到桌面
Copy-Item "C:\Windows\Fonts\simhei.ttf" "$env:USERPROFILE\Desktop\simhei.ttf"
```

---

## 📝 步骤 2: 在线转换中文字体（重要！）

### 访问转换工具：
https://rop.nl/truetype2gfx/

### 配置参数（关键）：

#### ⚠️ 注意：完整转换中文字体会生成巨大文件！

**不推荐（文件太大）：**
```
Font size: 16
Start char: 0x4E00  (一)
End char:   0x9FFF  (鿿)
结果：20000+ 字符，文件 >1MB，ESP32 无法承受
```

**✅ 推荐方案：只转换需要的字符**

#### 方案 A：使用字符列表（推荐）⭐

1. 在转换页面找到 **"Custom character list"** 输入框
2. 粘贴你需要的中文字符（参考 `tools/common_chinese_chars.txt`）

**示例字符集（温湿度显示）：**
```
温度湿度电量充电中已充满低电量WiFi连接成功失败已断开IP地址年月日时分秒星期一二三四五六日开关0123456789.:°C%
```

3. 配置参数：
```
Font size: 16
Custom character list: [粘贴上面的字符]
Bits per pixel: 1
```

4. 点击 **"Convert"** 并等待（可能需要 1-2 分钟）

5. 下载生成的 `.h` 文件，重命名为 `ChineseFont16.h`

#### 方案 B：只转换少量常用字（快速测试）

**最小测试字符集（约 20 字）：**
```
测试中文字体显示正常0123456789
```

这样生成的文件只有几 KB，适合快速测试。

---

## 🔧 步骤 3: 修复字体文件

下载的 `ChineseFont16.h` 文件需要手动添加头文件：

### 3.1 打开文件，在最开头添加：

```cpp
#ifndef CHINESEFONT16_H
#define CHINESEFONT16_H

#include <Adafruit_GFX.h>

// ... 原有内容 ...
```

### 3.2 在文件末尾添加：

```cpp
#endif // CHINESEFONT16_H
```

### 3.3 找到字体结构体名称

在文件末尾找到类似这样的代码：
```cpp
const GFXfont ChineseFont16pt PROGMEM = {  // ⭐ 记住这个名称！
  (uint8_t  *)ChineseFont16Bitmaps,
  (GFXglyph *)ChineseFont16Glyphs,
  0x4E00, 0x9FFF, 16
};
```

**记住 `ChineseFont16pt` 这个名称，后面要用！**

---

## 💻 步骤 4: 集成到项目

### 4.1 复制字体文件到项目

```powershell
# 将下载的字体文件复制到项目
Copy-Item "$env:USERPROFILE\Downloads\ChineseFont16.h" "G:\A_BL_Project\inkScree_fuben\components\fonts\"
```

### 4.2 修改 ink_screen.cpp

打开 `components\grbl_esp32s3\Grbl_Esp32\src\BL_add\ink_screen\ink_screen.cpp`

#### 在文件顶部添加（约第 20 行）：

```cpp
// 包含中文字体
#include "../fonts/ChineseFont16.h"
```

#### 修改测试函数显示中文：

找到 `ink_screen_test_gxepd2_microsnow_213()` 函数，修改显示部分：

```cpp
display.setFullWindow();
display.firstPage();
do
{
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    // ⭐ 使用中文字体（使用你在 .h 文件中找到的字体名称）
    display.setFont(&ChineseFont16pt);
    
    // 显示中文
    display.setCursor(20, 50);
    display.print("测试中文");
    
    display.setCursor(20, 80);
    display.print("温度: 25°C");
    
    display.setCursor(20, 110);
    display.print("湿度: 60%");
    
} while (display.nextPage());
```

---

## 🎨 步骤 5: 实用中文显示示例

### 示例 1: 传感器数据显示

```cpp
void displayChineseSensorData() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        
        // 使用中文字体
        display.setFont(&ChineseFont16pt);
        
        // 标题
        display.setCursor(50, 30);
        display.print("环境监测");
        
        // 温度
        display.setCursor(20, 70);
        display.print("温度: ");
        display.print(25.6, 1);
        display.print("°C");
        
        // 湿度
        display.setCursor(20, 100);
        display.print("湿度: ");
        display.print(62.3, 1);
        display.print("%");
        
        // 状态
        display.setCursor(20, 130);
        display.print("状态: 正常");
        
    } while (display.nextPage());
}
```

### 示例 2: WiFi 状态显示

```cpp
void displayWiFiStatus(bool connected, const char* ip) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&ChineseFont16pt);
        
        display.setCursor(20, 40);
        display.print("WiFi状态");
        
        display.setCursor(20, 80);
        if (connected) {
            display.print("已连接");
            display.setCursor(20, 110);
            display.print("IP: ");
            display.print(ip);
        } else {
            display.print("未连接");
        }
        
    } while (display.nextPage());
}
```

### 示例 3: 菜单界面

```cpp
void displayChineseMenu() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&ChineseFont16pt);
        
        // 菜单标题
        display.setCursor(70, 30);
        display.print("主菜单");
        
        // 菜单项
        int y = 70;
        display.setCursor(30, y); y += 35;
        display.print("1. 设置");
        
        display.setCursor(30, y); y += 35;
        display.print("2. 显示");
        
        display.setCursor(30, y); y += 35;
        display.print("3. 系统");
        
        display.setCursor(30, y);
        display.print("4. 关于");
        
    } while (display.nextPage());
}
```

---

## 📊 字符集建议

### 最小测试集（~20 字符）：
```
测试中文字体显示正常0123456789
```
**文件大小：** ~5 KB

### 基础显示集（~100 字符）：
```
温度湿度电量充电中已充满低电量WiFi连接成功失败已断开IP地址年月日时分秒星期一二三四五六日开关灯风扇空调模式自动手动设置返回确定取消0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz°C%:/-_.()
```
**文件大小：** ~20 KB

### 完整应用集（~500 字符）：
包含上述字符 + 你应用中所有可能显示的中文
**文件大小：** ~100 KB

---

## ⚠️ 重要注意事项

### 1. 内存限制
- ESP32-S3 Flash 有限
- **推荐：** 字体文件 < 100KB
- **最大：** < 500KB

### 2. 编码问题
确保源代码文件使用 **UTF-8 编码**：

```cpp
// ✅ 正确（C++11）
const char* text = u8"中文";

// ✅ 或直接写（确保文件是 UTF-8）
display.print("中文");

// ❌ 错误（会乱码）
display.print("\xD6\xD0\xCE\xC4");  // GBK 编码
```

### 3. 字符范围检查

如果显示空白或乱码，检查字符是否在转换范围内：

```cpp
// 在 .h 文件末尾查看
const GFXfont ChineseFont16pt PROGMEM = {
  ...,
  0x4E00, 0x9FFF, 16  // 起始字符, 结束字符
  //      ^^^^^^  ^^^^ 只有这个范围内的字符能显示
};
```

---

## 🐛 常见问题及解决

### Q1: 显示空白/方框？
**A:** 字符不在转换范围内，重新转换并包含该字符

### Q2: 显示乱码？
**A:** 
1. 检查源文件编码（应为 UTF-8）
2. 使用 `u8"中文"` 前缀
3. 确认字体文件包含该字符

### Q3: 编译时内存不足？
**A:** 减少字符数量，只包含必需的字符

### Q4: 部分字符显示异常？
**A:** 字体文件可能损坏，重新转换

---

## ✅ 快速测试步骤

### 1️⃣ 准备字体（5 分钟）
```powershell
# 复制系统字体
Copy-Item "C:\Windows\Fonts\simhei.ttf" "$env:USERPROFILE\Desktop\"
```

### 2️⃣ 在线转换（3 分钟）
- 访问 https://rop.nl/truetype2gfx/
- 上传 simhei.ttf
- Font size: 16
- Custom character list: `测试中文显示0123456789`
- 点击 Convert 并下载

### 3️⃣ 修复文件（2 分钟）
```cpp
// 在文件开头添加
#ifndef CHINESEFONT16_H
#define CHINESEFONT16_H
#include <Adafruit_GFX.h>

// 在文件末尾添加
#endif
```

### 4️⃣ 集成代码（2 分钟）
```cpp
// ink_screen.cpp 顶部
#include "../fonts/ChineseFont16.h"

// 使用
display.setFont(&你的字体名称);
display.print("测试中文");
```

### 5️⃣ 编译测试（5 分钟）
```powershell
cd G:\A_BL_Project\inkScree_fuben
idf.py build
idf.py -p COM3 flash monitor
```

---

## 🎓 下一步优化

### 多字号支持：
```
ChineseFont12.h  - 小字（状态栏）
ChineseFont16.h  - 中字（正文）
ChineseFont24.h  - 大字（标题）
```

### 字体混用：
```cpp
// 标题用大字体
display.setFont(&ChineseFont24);
display.print("温度监测");

// 正文用中字体
display.setFont(&ChineseFont16);
display.print("当前: 25°C");
```

---

## 📁 相关文件

- 字符集模板：`tools/common_chinese_chars.txt`
- 完整教程：`docs/TTF_Font_Guide.md`
- 错误解决：`docs/FONT_ERROR_SOLUTIONS.md`

---

祝你成功显示中文！🎉

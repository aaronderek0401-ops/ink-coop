# TTF 字体转换常见错误及解决方案

## ❌ 错误 1: 'GFXglyph' does not name a type

### 错误原因
字体头文件缺少必要的包含文件 `#include <Adafruit_GFX.h>`

### 解决方案
在字体 .h 文件的**最开头**添加：

```cpp
#ifndef MYFONT16_H
#define MYFONT16_H

#include <Adafruit_GFX.h>  // ⭐ 必须添加这一行

// ... 字体数据 ...

#endif // MYFONT16_H
```

---

## ❌ 错误 2: 'GFXfont' does not name a type

### 错误原因
同错误 1，缺少 Adafruit_GFX.h 头文件

### 解决方案
添加 `#include <Adafruit_GFX.h>` 并使用头文件保护宏

---

## ❌ 错误 3: no matching function for call to 'setFont'

### 错误原因
**使用了错误的变量名**

转换工具生成的文件结构：
```cpp
const uint8_t MyFont16[] PROGMEM = { ... };          // 位图数据（数组）
const GFXglyph MyFont16Glyphs[] PROGMEM = { ... };   // 字形数据（数组）
const GFXfont FreeSerifBoldItalic16pt7b PROGMEM = {  // 字体结构体 ⭐
  (uint8_t  *)MyFont16,
  (GFXglyph *)MyFont16Glyphs,
  0x20, 0x7E, 55
};
```

### ❌ 错误用法：
```cpp
display.setFont(&MyFont16);  // MyFont16 是位图数组，不是 GFXfont！
```

### ✅ 正确用法：
```cpp
display.setFont(&FreeSerifBoldItalic16pt7b);  // 使用字体结构体
```

---

## 📝 完整的字体文件模板

### 正确的 .h 文件结构：

```cpp
// ========================================
// 文件头部（必须）
// ========================================
#ifndef MYFONT16_H
#define MYFONT16_H

#include <Adafruit_GFX.h>

// ========================================
// 1. 位图数据
// ========================================
const uint8_t MyFont16Bitmaps[] PROGMEM = {
  0x00, 0x03, 0x81, 0xF0, ...
};

// ========================================
// 2. 字形描述
// ========================================
const GFXglyph MyFont16Glyphs[] PROGMEM = {
  {     0,   1,   1,   8,    0,    0 },   // 0x20 ' '
  {     1,  10,  21,  12,    2,  -20 },   // 0x21 '!'
  ...
};

// ========================================
// 3. 字体结构（重要！使用这个变量）
// ========================================
const GFXfont MyFont16 PROGMEM = {
  (uint8_t  *)MyFont16Bitmaps,
  (GFXglyph *)MyFont16Glyphs,
  0x20, 0x7E, 16  // 起始字符, 结束字符, 行高
};

// ========================================
// 文件尾部（必须）
// ========================================
#endif // MYFONT16_H
```

---

## 💻 代码中的正确使用方式

### 在 ink_screen.cpp 中：

```cpp
// ========================================
// 1. 包含字体头文件
// ========================================
#include "../fonts/MyFont16.h"

// ========================================
// 2. 使用字体
// ========================================
void displayWithCustomFont() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        
        // ⭐ 使用字体结构体名称（不是数组名）
        display.setFont(&MyFont16);
        
        display.setCursor(10, 50);
        display.print("Hello World!");
        
    } while (display.nextPage());
}
```

---

## 🔍 如何找到正确的字体名称？

### 方法 1: 查看 .h 文件末尾

找到 `const GFXfont XXX PROGMEM = {` 这一行，`XXX` 就是字体名称

**示例：**
```cpp
const GFXfont FreeSerifBoldItalic16pt7b PROGMEM = {  // ⭐ 这是字体名
  ...
};
```

使用时：
```cpp
display.setFont(&FreeSerifBoldItalic16pt7b);  // ⭐ 使用这个名称
```

### 方法 2: 搜索文件中的 "GFXfont"

1. 打开 .h 文件
2. 按 `Ctrl+F` 搜索 `GFXfont`
3. 找到的变量名就是要使用的字体名

---

## 🛠️ 转换工具生成的文件修复脚本

如果你从 truetype2gfx 下载的文件缺少头文件包含，可以快速修复：

### PowerShell 脚本：

```powershell
# fix_font_header.ps1
param(
    [string]$fontFile = "MyFont16.h"
)

$content = Get-Content $fontFile -Raw

# 检查是否已经有 include
if ($content -notmatch "#include <Adafruit_GFX.h>") {
    # 在文件开头添加头文件
    $header = @"
#ifndef $(($fontFile -replace '\.h$','').ToUpper())_H
#define $(($fontFile -replace '\.h$','').ToUpper())_H

#include <Adafruit_GFX.h>

"@
    
    # 在文件末尾添加 endif
    $footer = "`n`n#endif // $(($fontFile -replace '\.h$','').ToUpper())_H"
    
    $newContent = $header + $content + $footer
    Set-Content $fontFile $newContent
    
    Write-Host "✅ 已修复 $fontFile" -ForegroundColor Green
} else {
    Write-Host "⚠️ $fontFile 已经包含必要的头文件" -ForegroundColor Yellow
}
```

### 使用方法：
```powershell
cd components/fonts
.\fix_font_header.ps1 MyFont16.h
```

---

## 📋 检查清单

在使用字体前，请确认：

- [ ] .h 文件开头有 `#ifndef` 和 `#define`
- [ ] 包含了 `#include <Adafruit_GFX.h>`
- [ ] 有三个部分：Bitmaps, Glyphs, Font
- [ ] 文件末尾有 `#endif`
- [ ] 在代码中 include 了字体文件
- [ ] 使用 `setFont(&FontName)`，其中 `FontName` 是 `GFXfont` 类型的变量名
- [ ] 编译无错误

---

## 🎓 快速修复指南

### 如果你遇到编译错误：

1. **打开字体 .h 文件**
2. **检查第一行** - 应该是 `#ifndef XXX_H`，如果不是，添加：
   ```cpp
   #ifndef MYFONT16_H
   #define MYFONT16_H
   #include <Adafruit_GFX.h>
   ```
3. **检查最后一行** - 应该是 `#endif`，如果不是，添加
4. **找到 `const GFXfont` 这一行** - 记住变量名
5. **在代码中使用该变量名**：
   ```cpp
   display.setFont(&该变量名);
   ```

---

## 💡 常见问题 FAQ

### Q: 为什么在线工具生成的文件缺少头文件？
**A:** truetype2gfx 假设你会手动添加，或者工具版本不同

### Q: 能不能直接使用 MyFont16Bitmaps？
**A:** 不能，必须使用 GFXfont 结构体，它包含了位图、字形和元数据

### Q: 如果字体名太长可以改吗？
**A:** 可以！修改 `const GFXfont XXX` 这一行的变量名即可

### Q: 字体文件太大怎么办？
**A:** 只转换需要的字符，减少字符数量

---

## ✅ 总结

**核心要点：**
1. 字体 .h 文件必须包含 `<Adafruit_GFX.h>`
2. 使用 `GFXfont` 结构体变量，不是 Bitmaps 数组
3. 查看文件末尾找到正确的字体变量名
4. 使用 `display.setFont(&字体变量名)`

**记住这个公式：**
```
正确的字体名 = const GFXfont 后面的变量名
```

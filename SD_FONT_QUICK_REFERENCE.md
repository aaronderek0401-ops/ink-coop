# SD卡字库快速参考

## 🎯 三步开始

```cpp
// 1. 初始化
initChineseFontFromSD("/sd/fangsong_gb2312_16x16.bin", 16);

// 2. 清屏
display.setFullWindow();
display.firstPage();
do {
    display.fillScreen(GxEPD_WHITE);
    
    // 3. 显示
    drawChineseText(display, 10, 10, "你好世界", GxEPD_BLACK);
    
} while (display.nextPage());
```

## 📚 常用API

| 函数 | 用途 | 示例 |
|------|------|------|
| `initChineseFontFromSD(path, size)` | 初始化字库 | `initChineseFontFromSD("/sd/font.bin", 16)` |
| `drawChineseText(display, x, y, text)` | 显示文本 | `drawChineseText(display, 10, 10, "文字")` |
| `drawChineseTextCentered(display, y, text)` | 居中显示 | `drawChineseTextCentered(display, 50, "标题")` |
| `drawChineseChar(display, x, y, unicode)` | 单个字符 | `drawChineseChar(display, 10, 10, 0x4F60)` |

## 💡 常见场景

### 标题 + 正文
```cpp
drawChineseTextCentered(display, 10, "标题", GxEPD_BLACK);
drawChineseText(display, 10, 40, "正文内容...", GxEPD_BLACK);
```

### 多行显示
```cpp
int16_t y = 10;
y = drawChineseText(display, 10, y, "第一行", GxEPD_BLACK) + 5;
y = drawChineseText(display, 10, y, "第二行", GxEPD_BLACK) + 5;
```

### 混合中英文
```cpp
drawChineseText(display, 10, 10, "温度: ", GxEPD_BLACK);
display.setCursor(80, 26);
display.print("25C");
```

## ⚠️ 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 初始化失败 | 文件不存在 | 检查路径 `/sd/xxx.bin` |
| 显示空白 | 字号不匹配 | 16x16用`size=16` |
| 乱码 | 编码问题 | 使用UTF-8编码 |

## 📁 文件位置

```
BL_add/ink_screen/
├── sd_font_loader.h          // SD字库加载器
├── sd_font_loader.cpp
├── chinese_text_display.h    // 显示助手
├── chinese_text_display.cpp
└── SD_FONT_USAGE_GUIDE.md    // 完整指南
```

## 🔗 相关文档

- 完整指南: `SD_FONT_USAGE_GUIDE.md`
- .bin生成: `BIN_FONT_GENERATION_GUIDE.md`
- 测试清单: `BIN_FONT_TEST_CHECKLIST.md`

---
**快速上手 · 开箱即用**

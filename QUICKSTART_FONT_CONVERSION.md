# 🚀 TTF 字体转换快速开始清单

## ✅ 第 1 步：准备字体文件（2 分钟）

### 选项 A：使用系统自带字体
- [ ] Windows 用户：复制 `C:\Windows\Fonts\simhei.ttf` 到桌面
- [ ] macOS 用户：复制 `/System/Library/Fonts/PingFang.ttc` 到桌面

### 选项 B：下载免费字体
- [ ] 访问 https://github.com/adobe-fonts/source-han-sans/releases
- [ ] 下载 `SourceHanSansCN-Regular.otf` (简体中文)

---

## ✅ 第 2 步：在线转换字体（5 分钟）

### 2.1 访问转换网站
- [ ] 打开浏览器访问：https://rop.nl/truetype2gfx/

### 2.2 上传字体
- [ ] 点击 "Browse..." 按钮
- [ ] 选择你的 TTF/OTF 文件

### 2.3 配置参数（重要！）

#### 对于初学者（推荐）：
```
Font size: 16
Start char: 0x20
End char: 0x7E
```
这会生成一个**只包含英文数字**的小文件（~2KB）

#### 对于中文显示：
```
Font size: 16
Custom character list: 
【粘贴 tools/common_chinese_chars.txt 的内容】
```
这会生成一个**包含常用中文**的文件（~20KB）

### 2.4 转换并下载
- [ ] 点击 "Convert" 按钮
- [ ] 等待转换完成（10-30秒）
- [ ] 点击 "Download" 下载 `.h` 文件
- [ ] 将文件重命名为有意义的名称，如 `MyFont16.h`

---

## ✅ 第 3 步：集成到项目（3 分钟）

### 3.1 放置字体文件
- [ ] 将下载的 `.h` 文件复制到：
      `G:\A_BL_Project\inkScree_fuben\components\fonts\`

### 3.2 修改代码
打开 `components/grbl_esp32s3/Grbl_Esp32/src/BL_add/ink_screen/ink_screen.cpp`

在文件顶部添加（大约第 20 行）：
```cpp
// 包含自定义字体
#include "../fonts/MyFont16.h"
```

### 3.3 使用字体
在 `ink_screen_test_gxepd2_microsnow_213()` 函数中修改：

找到这段代码：
```cpp
display.setFont();  // 使用默认字体
display.setTextSize(1);
```

替换为：
```cpp
display.setFont(&MyFont16);  // 使用自定义字体
```

---

## ✅ 第 4 步：编译和测试（5 分钟）

### 4.1 编译项目
```bash
# 在 PowerShell 中运行
cd G:\A_BL_Project\inkScree_fuben
idf.py build
```

### 4.2 烧录固件
```bash
idf.py -p COM3 flash  # 修改 COM3 为你的端口
```

### 4.3 查看效果
- [ ] 打开串口监视器
- [ ] 观察墨水屏显示
- [ ] 验证自定义字体是否正常显示

---

## 🎯 快速验证代码（复制粘贴即可）

如果你已经完成字体转换，可以直接使用这段代码测试：

```cpp
// 在 ink_screen.cpp 顶部添加
#include "../fonts/MyFont16.h"  // 改成你的字体文件名

// 在测试函数中使用
void testCustomFont() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        
        // 使用自定义字体
        display.setFont(&MyFont16);
        
        // 显示测试文字
        display.setCursor(20, 50);
        display.print("Test 测试");
        
        display.setCursor(20, 80);
        display.print("123 ABC");
        
    } while (display.nextPage());
}
```

---

## 📋 检查清单

### 转换前
- [ ] 已准备 TTF/OTF 字体文件
- [ ] 已创建字符列表（中文需要）
- [ ] 已访问 https://rop.nl/truetype2gfx/

### 转换后
- [ ] 已下载 `.h` 文件
- [ ] 文件大小合理（< 100KB 推荐）
- [ ] 已放入 `components/fonts/` 目录

### 代码集成
- [ ] 已添加 `#include` 语句
- [ ] 已使用 `display.setFont()` 设置字体
- [ ] 编译无错误

### 测试验证
- [ ] 固件成功烧录
- [ ] 墨水屏有显示
- [ ] 字体显示正常（无乱码）

---

## 🆘 遇到问题？

### 问题 1：转换网站无法访问
**解决：** 使用命令行工具 fontconvert 或换个时间访问

### 问题 2：生成的文件太大（> 500KB）
**解决：** 减少字符数量，只包含必需的字符

### 问题 3：编译错误 "font not found"
**解决：** 检查 `#include` 路径是否正确

### 问题 4：显示乱码
**解决：** 
1. 确保字符在转换范围内
2. 检查源码文件编码（应为 UTF-8）
3. 使用 `u8"中文"` 字符串前缀

### 问题 5：字体显示位置不对
**解决：** 使用 `getTextBounds()` 计算正确位置

---

## ⏱️ 预计总用时

- 准备字体：2 分钟
- 在线转换：5 分钟
- 集成代码：3 分钟
- 编译测试：5 分钟
- **总计：约 15 分钟**

---

## 📚 参考文件位置

- 转换指南：`tools/online_font_conversion_guide.md`
- 常用字符：`tools/common_chinese_chars.txt`
- 示例代码：`components/fonts/example_usage.cpp`
- 完整教程：`docs/TTF_Font_Guide.md`

---

## 🎉 完成标志

当你看到墨水屏上显示出**自定义字体的文字**（而不是默认的像素字体），恭喜你成功了！

---

**下一步：** 尝试转换多个字号（12pt, 16pt, 24pt），创建丰富的界面效果！

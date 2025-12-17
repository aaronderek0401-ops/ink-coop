# 📷 图片转 .bin 文件 - 使用说明

## ✅ 已完成的修改

### 1. **Web 界面更新** (web_layout.html)
在 "📷 图片转位图" 区域新增了以下内容：

```html
<!-- .bin 文件转换区域 -->
<div style="background: #e3f2fd; padding: 12px; margin: 15px 0;">
    <h4>📦 图片转 .bin 文件</h4>
    
    <button onclick="convertImageToBin()">🔄 转换为 .bin 文件</button>
    <button onclick="downloadImageBin()">💾 下载 .bin 文件</button>
    <button onclick="uploadImageBinToSD()">📤 上传到 SD 卡</button>
    
    <div id="imageBinStatus"></div>
</div>
```

### 2. **JavaScript 函数**
新增了三个核心函数：

#### `convertImageToBin()`
- 调用 Python API `/convert_image_to_bin`
- 将图片转换为 .bin 格式
- 显示转换结果和文件信息

#### `downloadImageBin()`
- 从服务器下载生成的 .bin 文件
- 保存到浏览器默认下载目录

#### `uploadImageBinToSD()`
- 将生成的 .bin 文件上传到 ESP32 的 SD 卡
- 直接可用，无需手动复制

### 3. **Python 后端更新** (ttf_to_gfx_webservice.py)
- API: `/convert_image_to_bin`
- **保存位置**: `components/bitmap/` 文件夹
- **文件格式**: 
  - 文件头：8字节 (宽度4字节 + 高度4字节)
  - 位图数据：逐行扫描，每8像素=1字节

---

## 🚀 完整使用流程

### 步骤 1: 启动 Python 服务
```bash
cd tools
python ttf_to_gfx_webservice.py
```

输出应显示：
```
============================================================
TTF 转 GFX Web 服务启动中...
============================================================
访问: http://localhost:5000
API 端点:
  - 图片转.bin: http://localhost:5000/convert_image_to_bin
  ...
============================================================
```

### 步骤 2: 在网页中操作

#### 2.1 打开 Web 界面
```
http://<ESP32_IP>/web_layout.html
```
或如果 ESP32 未连接：
```
直接打开本地文件: components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html
```

#### 2.2 上传图片
1. 滚动到 **"📷 图片转位图"** 区域
2. 点击 **"选择图片文件"**，上传你的图片 (JPG/PNG/BMP 等)

#### 2.3 设置参数
```
图片宽度: 240 (墨水屏宽度)
图片高度: 416 (墨水屏高度)
处理模式: 抖动算法 (推荐) / 阈值二值化
阈值: 128 (仅在二值化模式下有效)
```

#### 2.4 预览效果
点击 **"预览处理效果"** 查看黑白转换效果

#### 2.5 生成 .bin 文件
找到蓝色区域 **"📦 图片转 .bin 文件"**，点击 **"🔄 转换为 .bin 文件"**

等待 1-3 秒，会显示：
```
✅ .bin 文件生成成功！

📋 文件信息:
• 文件名: logo_240x416.bin
• 文件大小: 12.48 KB
• 图片尺寸: 240x416
• 处理模式: Floyd-Steinberg 抖动

💾 本地路径:
G:\A_BL_Project\inkScree_fuben\components\bitmap\logo_240x416.bin

📖 使用方法:
displayImageFromSD("/sd/logo_240x416.bin", 0, 0, display);
```

#### 2.6 下载或上传
- **下载**: 点击 **"💾 下载 .bin 文件"**，保存到本地
- **上传到 SD 卡**: 点击 **"📤 上传到 SD 卡"**，直接传到 ESP32

---

## 📁 文件组织

### 本地 PC 目录结构
```
inkScree_fuben/
├── components/
│   ├── bitmap/                    # 图片 .bin 文件存储 (新增)
│   │   ├── logo_240x416.bin
│   │   ├── photo_200x200.bin
│   │   └── icon_64x64.bin
│   └── fonts/                     # 字库 .bin 文件存储
│       └── fangsong_gb2312_16x16.bin
└── tools/
    └── ttf_to_gfx_webservice.py
```

### ESP32 SD 卡目录结构
```
/sd/
├── logo_240x416.bin         # 全屏 Logo
├── photo_200x200.bin        # 产品照片
├── icon_wifi_32x32.bin      # WiFi 图标
├── icon_battery_32x32.bin   # 电池图标
└── fangsong_gb2312_16x16.bin # 字库
```

---

## 💻 ESP32 代码示例

### 基础用法
```cpp
#include "image_loader.h"

void setup() {
    // 初始化墨水屏
    display.init();
    
    // 显示 SD 卡中的图片
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        displayImageFromSD("/sd/logo_240x416.bin", 0, 0, display);
    } while (display.nextPage());
}
```

### 多图组合
```cpp
void displayProductPage() {
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // 顶部 Logo (200x60)
        displayImageFromSD("/sd/company_logo_200x60.bin", 20, 10, display);
        
        // 中间产品照片 (200x200)
        displayImageFromSD("/sd/product_main_200x200.bin", 20, 90, display);
        
        // 底部图标 (32x32 each)
        displayImageFromSD("/sd/wifi_icon_32x32.bin", 10, 360, display);
        displayImageFromSD("/sd/battery_icon_32x32.bin", 60, 360, display);
        
        // 中文文字
        displayChineseText(display, "智能温控器", 20, 310);
        
    } while (display.nextPage());
}
```

---

## 🔧 常见问题

### Q1: Python 服务启动失败？
**A**: 检查依赖库：
```bash
pip install flask pillow flask-cors
```

### Q2: 转换时报错 "请确保 Python 服务已启动"？
**A**: 
1. 确认 `python tools/ttf_to_gfx_webservice.py` 正在运行
2. 检查端口 5000 是否被占用
3. 浏览器控制台查看详细错误 (F12)

### Q3: 上传到 SD 卡失败？
**A**:
1. 检查 ESP32 IP 地址是否正确 (代码中默认 `192.168.0.4:8848`)
2. 确认 ESP32 已连接 WiFi
3. 确认 SD 卡已插入且格式化为 FAT32
4. 检查 SD 卡空间是否充足

### Q4: 图片显示效果不好？
**A**: 尝试不同的处理模式：
- **抖动算法**: 适合照片、渐变图 (推荐)
- **阈值二值化**: 适合 Logo、图标、文字 (调整阈值 0-255)

### Q5: 文件找不到？
**A**: 生成的文件保存在：
```
本地: components/bitmap/你的文件名.bin
SD卡: /sd/你的文件名.bin (上传后)
```

---

## 📊 .bin 文件格式说明

### 文件结构
```
偏移    字节数   说明
0x00    4       宽度 (uint32_t, 小端序)
0x04    4       高度 (uint32_t, 小端序)
0x08    ...     位图数据 (每8像素=1字节, 逐行扫描)
```

### 示例: 240x416 图片
```
文件头: 8 字节
  - 宽度: 0xF0 0x00 0x00 0x00 (240)
  - 高度: 0xA0 0x01 0x00 0x00 (416)

位图数据:
  - 每行: (240+7)/8 = 30 字节
  - 总行数: 416
  - 数据大小: 30 × 416 = 12480 字节

总文件大小: 8 + 12480 = 12488 字节 ≈ 12.2 KB
```

### 像素编码
```
字节内像素顺序: 高位在左 (MSB first)

示例 (8像素):
像素: ■□■□■■■□
字节: 0b10101110 = 0xAE

白色 = 1 (位设置)
黑色 = 0 (位清除)
```

---

## 🎯 使用建议

### 1. 图片尺寸优化
```
全屏:  240x416  (~12 KB)   - Logo/启动画面
大图:  200x200  (~5 KB)    - 产品照片
中图:  128x128  (~2 KB)    - 头像/图片
小图:  64x64    (~0.5 KB)  - 图标
迷你:  32x32    (~0.15 KB) - 状态指示
```

### 2. 处理模式选择
| 图片类型 | 推荐模式 | 原因 |
|---------|---------|------|
| 照片 | 抖动算法 | 灰阶过渡更自然 |
| Logo | 阈值二值化 | 边缘清晰锐利 |
| 图标 | 阈值二值化 | 简洁明了 |
| 文字 | 阈值二值化 | 可读性强 |

### 3. 文件命名规范
```cpp
// ✅ 推荐: 描述性名称 + 尺寸
logo_company_240x60.bin
product_main_200x200.bin
icon_wifi_32x32.bin

// ❌ 避免: 无意义名称
image1.bin
pic.bin
test.bin
```

---

## ✅ 功能清单

- [x] Web 界面新增 .bin 转换区域
- [x] JavaScript 转换函数 `convertImageToBin()`
- [x] 下载功能 `downloadImageBin()`
- [x] 上传到 SD 卡功能 `uploadImageBinToSD()`
- [x] Python API `/convert_image_to_bin`
- [x] 保存到 `components/bitmap/` 文件夹
- [x] 支持抖动算法和阈值二值化
- [x] 生成带文件头的 .bin 格式
- [x] ESP32 C++ 加载函数 `displayImageFromSD()`

---

## 🎉 总结

现在您可以：
1. ✅ 在网页中上传任意图片
2. ✅ 一键转换为 .bin 格式
3. ✅ 直接上传到 ESP32 SD 卡
4. ✅ 在 ESP32 代码中一行显示图片

**完全无需手动操作！从图片到墨水屏显示，全流程自动化！** 🚀

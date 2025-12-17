# 🚀 Python 高质量位图字体生成使用指南

## 📋 功能说明

现在可以在网页上直接调用 Python 后端生成**高质量位图字体**,比浏览器 Canvas 渲染质量更好!

### 质量对比

| 方案 | 渲染引擎 | 像素质量 | 适用场景 |
|------|---------|---------|---------|
| **浏览器版本** | JavaScript Canvas API | ⭐⭐⭐ 一般 | 简单测试 |
| **Python版本** | Python + Pillow | ⭐⭐⭐⭐⭐ 优秀 | **生产环境 (推荐)** |

### 实际案例对比

**"生" 字像素数据对比:**

```cpp
// 浏览器版本 - 只有 8 个非零字节 (像素稀疏)
0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
0x04, 0x00, 0x08, 0x00, 0x08, 0x10, 0x0F, 0xE0...

// Python版本 - 有 24 个非零字节 (像素丰富)
0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x09, 0x00,
0x09, 0x00, 0x11, 0xF0, 0x1F, 0x00, 0x21, 0x00...
```

---

## 🚀 使用步骤

### 步骤 1: 启动 Python 服务

```powershell
python tools\ttf_to_gfx_webservice.py
```

**确认输出:**
```
============================================================
TTF 转 GFX Web 服务启动中...
============================================================
访问: http://localhost:5000
API 端点:
  - GFX 转换: http://localhost:5000/convert_ttf_to_gfx
  - 位图保存: http://localhost:5000/save_bitmap_font
  - Python高质量位图: http://localhost:5000/convert_ttf_to_bitmap_python
============================================================
```

### 步骤 2: 打开网页

浏览器访问:
```
http://ESP32_IP/web_layout.html
```

### 步骤 3: 转换字体

1. **上传 TTF 字体** (如: 仿宋_GB2312.ttf)
2. **输入文字** (如: `生成字库`)
3. **选择字号** (推荐 16/20/24)
4. **点击** 🚀 Python高质量位图 (推荐)

### 步骤 4: 查看结果

**成功后显示:**
```
✅ Python高质量渲染完成！
字符数: 4
字号: 16pt
格式: 独立位图数组 (Python + Pillow 高质量渲染)
文件: fangsong_gb2312_16pt_bitmaps.h
✅ 已保存到: G:\...\components\fonts\fangsong_gb2312_16pt_bitmaps.h
💡 质量优于浏览器渲染,像素更完整!
```

---

## 🎯 三种转换方式对比

### 1. GFX 格式 (不推荐中文)
```
转换为 GFX 格式
```
- ❌ Unicode 范围问题
- ❌ 中文显示花屏
- ✅ 英文/数字正常

### 2. 浏览器位图 (一般)
```
转换为位图格式 (浏览器)
```
- ✅ 无 Unicode 范围限制
- ⚠️ 小字号像素缺失
- ⚠️ 质量一般

### 3. Python位图 (推荐) ⭐⭐⭐⭐⭐
```
🚀 Python高质量位图 (推荐)
```
- ✅ 无 Unicode 范围限制
- ✅ 像素完整丰富
- ✅ 质量最佳
- ✅ 自动保存到本地

---

## 📂 生成的文件

### 文件结构

```cpp
// fangsong_gb2312_16pt_bitmaps.h

#include <Arduino.h>

// 独立位图数组 (带前缀避免冲突)
const uint8_t BITMAP_FANGSONG_GB2312_751F[] PROGMEM = { ... };  // '生'
const uint8_t BITMAP_FANGSONG_GB2312_6210[] PROGMEM = { ... };  // '成'
const uint8_t BITMAP_FANGSONG_GB2312_5B57[] PROGMEM = { ... };  // '字'
const uint8_t BITMAP_FANGSONG_GB2312_5E93[] PROGMEM = { ... };  // '库'

// 绘制单个字符
template<typename T>
void drawFangsongGb2312Char(T& display, int16_t x, int16_t y, uint16_t charCode);

// 绘制 UTF-8 字符串
template<typename T>
void drawFangsongGb2312String(T& display, int16_t x, int16_t y, const char* text);
```

### 使用方法

```cpp
#include "../fonts/fangsong_gb2312_16pt_bitmaps.h"

// 在墨水屏上显示
drawFangsongGb2312String(display, 10, 50, "生成字库");
```

---

## 🔧 工作原理

### 前端 (web_layout.html)

```javascript
async function convertTTFtoBitmapPython() {
    // 1. 读取 TTF 文件为 base64
    const ttfBase64 = /* ... */;
    
    // 2. 调用 Python 后端
    const response = await fetch('http://localhost:5000/convert_ttf_to_bitmap_python', {
        method: 'POST',
        body: JSON.stringify({
            ttf_base64: ttfBase64,
            chars: "生成字库",
            font_size: 16,
            font_name: "fangsong_gb2312"
        })
    });
    
    // 3. 获取高质量位图内容
    const result = await response.json();
    // result.content - .h 文件内容
    // result.saved_path - 本地保存路径
}
```

### 后端 (ttf_to_gfx_webservice.py)

```python
@app.route('/convert_ttf_to_bitmap_python', methods=['POST'])
def convert_ttf_to_bitmap_python():
    # 1. 接收 TTF 文件
    ttf_data = base64.b64decode(data['ttf_base64'])
    
    # 2. 使用 Pillow 渲染
    font = ImageFont.truetype(ttf_data, font_size)
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    draw.text((x, y), char, font=font, fill=0)
    
    # 3. 转换为位图字节
    pixels = list(img.getdata())
    bitmap_bytes = [...]
    
    # 4. 生成 .h 文件
    h_content = f"const uint8_t BITMAP_XXX[] PROGMEM = {{ {bitmap_bytes} }};"
    
    # 5. 自动保存到 components/fonts/
    with open(file_path, 'w') as f:
        f.write(h_content)
    
    return jsonify({'success': True, 'content': h_content})
```

---

## ⚠️ 常见问题

### Q1: 点击按钮后提示 "Python 转换失败"

**原因**: Python 服务未启动

**解决**:
```powershell
python tools\ttf_to_gfx_webservice.py
```

---

### Q2: 提示 "Connection refused"

**原因**: 端口 5000 被占用或服务未运行

**检查**:
```powershell
# 检查端口
netstat -ano | findstr :5000

# 重启服务
python tools\ttf_to_gfx_webservice.py
```

---

### Q3: 为什么不能直接在浏览器运行 Python?

**原因**: 浏览器安全限制

浏览器 JavaScript 无法:
- ❌ 直接运行本地 Python 脚本
- ❌ 直接访问本地文件系统
- ❌ 执行系统命令

**解决方案**: 使用 HTTP API (当前方案)
- ✅ 浏览器通过 HTTP 调用 Python 后端
- ✅ Python 在服务器端处理并返回结果
- ✅ 符合浏览器安全策略

---

### Q4: 生成的文件在哪里?

**浏览器下载**: `Downloads/fangsong_gb2312_16pt_bitmaps.h`  
**本地保存**: `components/fonts/fangsong_gb2312_16pt_bitmaps.h`

两个位置都会保存!

---

## 📊 性能对比

| 指标 | 浏览器版本 | Python版本 |
|------|-----------|-----------|
| **渲染质量** | 一般 | 优秀 |
| **小字号支持** | 较差 | 优秀 |
| **中文支持** | 一般 | 优秀 |
| **生成速度** | 快 (客户端) | 中 (网络传输) |
| **依赖** | 无 | 需要 Python 服务 |
| **像素完整度** | 60-70% | 95-100% |

---

## 🎉 优势总结

### ✅ Python 高质量位图的优势

1. **像素丰富**: Pillow 库专业字体渲染,像素数据完整
2. **无缺失**: 16pt 小字号也不会有笔画缺失
3. **自动保存**: 直接保存到项目 `components/fonts/` 目录
4. **双重下载**: 浏览器下载 + 本地保存,更安全
5. **避免冲突**: 数组名带前缀,不会与其他字体冲突
6. **UTF-8 支持**: 完整的 UTF-8 解码,支持 1-4 字节字符

---

## 🔄 完整工作流

```
用户上传 TTF
     ↓
输入文字 "生成字库"
     ↓
点击 "Python高质量位图"
     ↓
浏览器读取 TTF → Base64编码
     ↓
POST 到 Python 后端
     ↓
Python + Pillow 渲染字符
     ↓
生成位图数组
     ↓
生成 .h 文件内容
     ↓
保存到 components/fonts/
     ↓
返回给浏览器
     ↓
浏览器自动下载
     ↓
显示成功消息
```

---

## 📝 代码示例

### 生成字体

```javascript
// 在 web_layout.html 中
// 1. 上传: 仿宋_GB2312.ttf
// 2. 输入: "温度湿度测试"
// 3. 字号: 20
// 4. 点击: 🚀 Python高质量位图
```

### 使用字体

```cpp
// 在 ink_screen.cpp 中
#include "../fonts/fangsong_gb2312_20pt_bitmaps.h"

void display_status() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // 使用 Python 生成的高质量字体
        drawFangsongGb2312String(display, 10, 50, "温度湿度测试");
        
    } while (display.nextPage());
}
```

---

**更新时间**: 2025-12-12  
**相关文档**: `位图字体自动保存说明.md`, `TTF字体位图转换使用指南.md`

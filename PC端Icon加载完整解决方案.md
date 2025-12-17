# 🎉 PC 端 Icon 加载完整解决方案

## 📌 问题说明

在 PC 端运行 `web_layout_standalone.html` 时，原来的图标加载方式是通过 HTTP `/api/icon/data/` 接口获取 Base64 编码的数据。但在 PC 端使用时，这个接口路径会被转换为 `file:///G:/...` 导致 CORS 跨域错误。

## ✅ 解决方案

现在已经实现了**双模式自适应加载**：

### 🟢 PC 模式（绿色状态）
- ✅ 检测到 `localhost:5001` Flask 服务器
- ✅ 从 Flask 服务器的 `/api/icon/binary/<index>` 端点加载二进制文件
- ✅ 直接读取 `components/resource/icon/` 文件夹下的 `.bin` 文件
- ✅ 快速高效，无需 Base64 编码/解码

### 🟦 ESP32 模式（蓝色状态）
- ✅ 通过 HTTP 从 ESP32 设备读取 Base64 编码的 icon 数据
- ✅ 自动解码为二进制格式用于渲染
- ✅ 保持原有的 ESP32 设备通信方式

## 🚀 快速开始

### 步骤 1：启动 Flask 服务器

打开命令行，进入项目目录：

```bash
cd g:\A_BL_Project\inkScree_fuben
python config_server.py
```

输出应该显示：

```
 * Running on http://127.0.0.1:5001
 * Press CTRL+C to quit
```

### 步骤 2：打开网页编辑器

**方式 A - 直接双击启动脚本（推荐）**

双击文件：
```
启动web编辑器_PC模式.bat
```

**方式 B - 手动打开文件**

在浏览器中打开：
```
file:///G:/A_BL_Project/inkScree_fuben/web_layout_standalone.html
```

### 步骤 3：验证状态

打开网页后，在顶部应该看到 **🟢 绿色状态条**，表示：
```
✓ 在电脑上运行 (PC Mode) - 使用本地服务器 (localhost:5001)
```

## 📊 技术细节

### HTML 端修改

**文件：** `web_layout_standalone.html`

新增 PC 模式下的 icon 文件映射：

```javascript
const ICON_FILES_FOR_PC = {
    0: '0_icon1_62x64.bin',
    1: '1_icon2_64x64.bin',
    // ... 更多 icon 映射
    12: '12_lock_32x32.bin'
};
```

更新后的 `getIconBitmapData()` 函数：

```javascript
async function getIconBitmapData(iconIndex) {
    // 检查缓存
    if (iconIndex in ICON_BITMAP_DATA) {
        return ICON_BITMAP_DATA[iconIndex];
    }
    
    try {
        // PC 模式：直接从 Flask 服务器加载二进制文件
        if (g_env.isPC) {
            const response = await fetch(`http://localhost:5001/api/icon/binary/${iconIndex}`);
            const arrayBuffer = await response.arrayBuffer();
            const binaryData = new Uint8Array(arrayBuffer);
            ICON_BITMAP_DATA[iconIndex] = binaryData;
            return binaryData;
        }
        
        // ESP32 模式：从设备获取 Base64 数据
        else if (g_env.isESP32) {
            const response = await fetch(`http://${g_env.esp32Host}/api/icon/data/${iconIndex}`);
            const base64Data = await response.text();
            const binaryString = atob(base64Data);
            const binaryData = new Uint8Array(binaryString.length);
            for (let i = 0; i < binaryString.length; i++) {
                binaryData[i] = binaryString.charCodeAt(i);
            }
            ICON_BITMAP_DATA[iconIndex] = binaryData;
            return binaryData;
        }
    } catch (error) {
        console.error(`Error loading icon ${iconIndex}:`, error);
        return null;
    }
}
```

### Flask 服务器端修改

**文件：** `config_server.py`

新增端点 `/api/icon/binary/<icon_index>`：

```python
@app.route('/api/icon/binary/<int:icon_index>', methods=['GET'])
def get_icon_binary(icon_index):
    """获取二进制 icon 文件"""
    icon_files = {
        0: '0_icon1_62x64.bin',
        1: '1_icon2_64x64.bin',
        # ... 更多 icon 映射
        12: '12_lock_32x32.bin'
    }
    
    if icon_index not in icon_files:
        return jsonify({"status": "error", "message": f"Icon index not found"}), 404
    
    icon_filename = icon_files[icon_index]
    icon_path = RESOURCE_DIR / 'icon' / icon_filename
    
    with open(icon_path, 'rb') as f:
        binary_data = f.read()
    
    from flask import send_file
    from io import BytesIO
    return send_file(
        BytesIO(binary_data),
        mimetype='application/octet-stream',
        as_attachment=False,
        download_name=icon_filename
    )
```

## 📂 文件结构

```
components/resource/icon/
├── 0_icon1_62x64.bin        ✅
├── 1_icon2_64x64.bin        ✅
├── 2_icon3_86x64.bin        ✅
├── 3_icon4_71x56.bin        ✅
├── 4_icon5_76x56.bin        ✅
├── 5_icon6_94x64.bin        ✅
├── 6_separate_120x8.bin     ✅
├── 7_wifi_connect_32x32.bin ✅
├── 8_wifi_disconnect_32x32.bin ✅
├── 9_battery_1_36x24.bin    ✅
├── 10_horn_16x16.bin        ✅
├── 11_nail_15x16.bin        ✅
└── 12_lock_32x32.bin        ✅
```

## 🔄 运行流程

```
1. 用户打开 web_layout_standalone.html
                    ↓
2. 页面加载，执行 detectEnvironment()
                    ↓
3. 尝试连接 localhost:5001/api/health
                    ↓
    ┌─── 连接成功 ─────────────────────┐
    │                                   │
    ↓                                   ↓
设置 g_env.isPC = true         设置 g_env.isESP32 = true
显示绿色状态条                  显示蓝色状态条
    │                                   │
    ↓                                   ↓
获取布局配置                    获取布局配置
    │                                   │
    ↓                                   ↓
开始渲染布局                    开始渲染布局
    │                                   │
    ↓                                   ↓
加载每个 icon 图标              加载每个 icon 图标
    │                                   │
    ↓                                   ↓
fetch /api/icon/binary/N      fetch /api/icon/data/N
(Flask 服务器)                (ESP32 设备)
    │                                   │
    ↓                                   ↓
返回二进制数据                  返回 Base64 数据
                                       ↓
                            atob() 解码为二进制
                                       │
                    ┌──────────────────┘
                    ↓
            在 Canvas 上渲染图标
```

## 🐛 常见问题

### Q1: 为什么显示蓝色状态？

**A:** Flask 服务器没有启动或无法连接。检查：

```bash
# 1. 确认 Python 已安装
python --version

# 2. 检查 config_server.py 是否存在
ls config_server.py

# 3. 启动服务器
python config_server.py

# 4. 验证服务是否运行
curl http://localhost:5001/api/health
```

### Q2: 图标仍然加载失败？

**A:** 检查 icon 文件是否存在：

```bash
# 检查 icon 文件夹
ls components/resource/icon/

# 应该看到 13 个 .bin 文件
0_icon1_62x64.bin
1_icon2_64x64.bin
...
12_lock_32x32.bin
```

### Q3: 能否同时支持多个用户？

**A:** 可以！Flask 服务器可以处理多个并发请求。默认配置：

```python
app.run(host='0.0.0.0', port=5001, debug=True)
```

- `host='0.0.0.0'` - 接受来自任何 IP 的连接
- `port=5001` - 监听端口 5001
- `debug=True` - 开发模式，代码更改自动重载

### Q4: 如何在 ESP32 设备上使用？

**A:** 编译烧写固件后，直接访问设备 IP：

```
http://192.168.1.100:8848
```

设备会自动显示蓝色状态（ESP32 模式）。

## 📋 API 文档

### PC 模式下的新端点

#### GET /api/icon/binary/<index>

获取二进制 icon 文件

**参数：**
- `index` (integer): Icon 索引 (0-12)

**请求示例：**
```bash
curl http://localhost:5001/api/icon/binary/0
```

**响应：**
- Content-Type: `application/octet-stream`
- Body: 二进制 icon 数据

**示例 (使用 PowerShell)：**
```powershell
$response = Invoke-WebRequest -Uri 'http://localhost:5001/api/icon/binary/0' -UseBasicParsing
$response.Content | Format-Object -Property Length  # 显示二进制数据大小
```

## 🎯 完整端点列表

| 端点 | 方法 | 描述 |
|------|------|------|
| `/api/layout` | GET | 获取布局文件 (screen_type=main\|vocab) |
| `/api/config/focus` | GET/POST | 焦点配置 |
| `/api/config/subarray` | GET/POST | 子数组配置 |
| `/api/icon/binary/<index>` | GET | **[新]** 二进制 icon 文件 |
| `/api/health` | GET | 健康检查 |
| `/` | GET | 显示 API 信息 |

## ✨ 优势

✅ **无 CORS 错误** - 使用二进制传输而不是相对路径
✅ **快速加载** - 直接二进制传输，无需 Base64 编码
✅ **双模式支持** - 自动切换 PC 和 ESP32 模式
✅ **代码简洁** - 统一的 `getIconBitmapData()` API
✅ **易于扩展** - 可轻松添加更多 icon 索引

## 📞 需要帮助？

- 检查浏览器控制台 (F12) 的错误信息
- 查看 Flask 服务器的输出日志
- 确保防火墙允许 localhost:5001 连接
- 确保 icon 文件夹中的所有 .bin 文件完整

---

**最后更新：** 2025年12月16日
**版本：** 1.0
**支持：** PC 模式完全支持，ESP32 模式兼容

# web_layout_standalone.html 使用指南

## 概述

`web_layout_standalone.html` 是一个完全独立的HTML文件，可以在任何浏览器中直接打开使用，不需要ESP32设备的参与。它支持**双模式操作**：

- **PC模式**：连接到本地Python服务器（localhost:5001）获取数据
- **ESP32模式**：自动降级到ESP32设备的Web服务器

## 文件位置

```
g:\A_BL_Project\inkScree_fuben\web_layout_standalone.html
```

## 启动方式

### 方式1：直接在浏览器中打开
右键点击`web_layout_standalone.html`，选择"用浏览器打开"，或者直接拖拽到浏览器中。

### 方式2：本地服务器运行（推荐）
为了完整体验所有功能，建议使用本地HTTP服务器：

```bash
# Python 3 内置服务器
cd g:\A_BL_Project\inkScree_fuben
python -m http.server 8080
# 然后访问 http://localhost:8080/web_layout_standalone.html
```

## 核心功能

### 🎯 环境自动检测
- ✅ 启动时自动检测 localhost:5001 是否可用
- ✅ 如果PC服务可用，优先使用PC本地数据
- ✅ 如果PC服务不可用，自动降级到ESP32
- ✅ 界面顶部显示当前运行环境（绿色=PC模式，蓝色=ESP32模式）

### 📱 PC模式（推荐）

#### 前置要求
1. **启动Python服务器**（二选一）：

   方式A - 使用config_server.py：
   ```bash
   cd g:\A_BL_Project\inkScree_fuben
   python config_server.py
   # 服务运行在 http://localhost:5001
   ```

   方式B - 使用TTF转换服务：
   ```bash
   cd g:\A_BL_Project\inkScree_fuben\tools
   python ttf_to_gfx_webservice.py
   # 服务运行在 http://localhost:5000
   ```

2. **layout.json 文件存在**：
   ```
   g:\A_BL_Project\inkScree_fuben\components\resource\layout.json
   ```

#### 可用API端点
- `/api/layout?screen_type=main` - 获取主界面布局
- `/api/layout?screen_type=vocab` - 获取单词界面布局  
- `/api/config/focus?screen_type=main` - 获取焦点配置
- `/api/config/subarray?screen_type=main` - 获取子数组配置
- `/api/health` - 健康检查

#### 功能特性
- ✅ 完整的布局可视化编辑
- ✅ 拖拽式矩形和组件管理
- ✅ 智能对齐指南
- ✅ 图标位置精确控制
- ✅ 文本属性编辑
- ✅ TTF字体转换为.bin字库
- ✅ 图片转换为.bin文件
- ✅ 直接上传文件到设备SD卡
- ✅ 焦点配置和子数组导航设置

### 📡 ESP32模式（自动降级）

如果PC服务不可用，网页会自动切换到ESP32模式：
- 自动连接到 `http://ESP32_IP:8848`
- 可以编辑设备上的当前布局
- 有限的功能支持（不支持字体和图片转换）

## 使用步骤

### 步骤1：启动PC服务
```bash
# 打开一个新的终端窗口
cd g:\A_BL_Project\inkScree_fuben
python config_server.py
# 等待输出：Running on http://0.0.0.0:5001
```

### 步骤2：打开网页编辑器
```bash
# 方式A：直接打开文件
在浏览器中访问：file:///g:/A_BL_Project/inkScree_fuben/web_layout_standalone.html

# 方式B：使用本地服务器（更稳定）
访问：http://localhost:8080/web_layout_standalone.html
```

### 步骤3：验证环境
- 查看页面顶部状态栏
- 如果显示绿色 "✓ 在电脑上运行" → PC模式已激活
- 如果显示蓝色 "✓ 在ESP32上运行" → 自动降级为ESP32模式

### 步骤4：编辑布局
1. **编辑矩形布局**：
   - 拖拽矩形调整位置和大小
   - 使用右侧控制面板添加或删除矩形
   
2. **管理图标**：
   - 从 resource/icon/ 选择.bin格式图标
   - 拖拽图标调整位置
   - 或使用相对坐标精确定位

3. **添加文本**：
   - 支持单词、音标、释义、翻译四种类型
   - 自定义字体大小和对齐方式
   - 拖拽文本调整位置

### 步骤5：应用到设备
1. 点击"应用到设备"按钮
2. 布局数据会发送到设备（或仅保存在本地）
3. 刷新设备屏幕查看更新

## 高级功能

### 🔤 TTF字体转换
1. 上传 .ttf/.otf 字体文件
2. 选择字库尺寸（16x16, 24x24, 32x32）
3. 点击"生成.bin字库文件"
4. 下载或直接上传到SD卡

### 📷 图片转换
1. 上传 JPG/PNG/BMP 图片
2. 选择处理模式（阈值或抖动算法）
3. 点击"转换为.bin文件"
4. 下载或直接上传到SD卡

### 💾 文件管理
- 查询字体库
- 查询图片库
- SD卡文件列表
- 批量删除文件

## 常见问题

### Q: "正在检测运行环境..." 一直不消失
**A:** 说明PC服务器 (localhost:5001) 不可用。请检查：
1. Python服务是否启动：`python config_server.py`
2. 服务是否监听在正确的端口：5001
3. 防火墙是否阻止localhost连接

### Q: 如何在其他电脑上访问布局编辑器？
**A:** 使用本地HTTP服务器并配置网络访问：
```bash
# 使用 0.0.0.0 而不是 localhost
python -m http.server 0.0.0.0:8080
# 然后其他电脑访问 http://你的IP:8080/web_layout_standalone.html
```

### Q: 为什么文件很大（5MB）？
**A:** 文件包含完整的JavaScript代码、CSS样式和所有功能实现。这是一个完整的Web应用。

### Q: 如何恢复到ESP32上的默认布局？
**A:** 
1. 如果有局部备份的JSON文件，可以"从文件加载"
2. 点击"重置为默认布局"按钮
3. 点击"应用到设备"

## 环境架构图

```
┌─────────────────────────────────────────────────────────┐
│                   web_layout_standalone.html             │
│                      (5100 行纯HTML文件)                 │
└──────────────────────────┬──────────────────────────────┘
                           │
                    detectEnvironment()
                           │
        ┌──────────────────┴──────────────────┐
        │                                     │
        v                                     v
┌──────────────────┐              ┌──────────────────┐
│   PC Mode        │              │  ESP32 Mode      │
│ (localhost:5001) │              │ (Auto Fallback)  │
│                  │              │                  │
│ ✓ Full Feature   │              │ ✗ Limited Feat   │
│ ✓ All APIs       │              │ ✓ Basic Edit     │
│ ✓ Font Conv      │              │ ✗ No Conv        │
│ ✓ Image Conv     │              │ ✗ No Upload      │
└──────────────────┘              └──────────────────┘
```

## 技术细节

### 双模式检测逻辑
```javascript
// 1. 首先尝试 localhost:5001
fetch("http://localhost:5001/api/health", { timeout: 2000 })

// 2. 如果PC服务响应 → PC模式激活
// 3. 如果PC服务无响应 → 自动降级到ESP32模式
// 4. 所有API调用通过 getApiUrl() 智能路由
```

### 配置参数
在 `web_layout_standalone.html` 中可以修改：
```javascript
let g_env = {
    isPC: false,           // PC模式标志
    isESP32: false,        // ESP32模式标志
    esp32Host: '',         // 自动检测
    pcHost: 'localhost',   // 修改PC地址
    pcPort: 5001           // 修改PC端口
};
```

## 故障排查

| 问题 | 症状 | 解决方案 |
|------|------|--------|
| 环境检测卡住 | "正在检测..." 不消失 | 启动 Python 服务器 |
| 图标不显示 | 矩形中显示灰色方块 | 检查 resource/icon/ 目录和权限 |
| 文件上传失败 | 显示连接错误 | 检查 ESP32 连接和 SD 卡 |
| 字体转换超时 | 等待10秒后失败 | Python 服务可能未启动 |
| 中文显示为方块 | 界面乱码 | 确保文件编码为 UTF-8 |

## 支持的浏览器

- ✅ Chrome / Edge 90+
- ✅ Firefox 88+
- ✅ Safari 14+
- ✅ Opera 76+
- ⚠️ IE 11 (不支持)

## 更新日志

### v2.0 (2024-12-15)
- ✨ 实现双模式环境检测
- ✨ PC优先降级为ESP32架构
- ✨ 完整的焦点和子数组配置
- ✨ TTF字体转换系统
- ✨ 图片转.bin文件转换
- ✨ 直接上传到SD卡功能

### v1.0 (2024-12-01)
- 基础布局编辑功能
- 矩形、图标、文本管理
- ESP32集成

## 联系方式

如有问题或建议，请参考项目文档或提交Issue。

---

**最后更新**：2024-12-15  
**文件大小**：约 5.1 MB（包含完整HTML+CSS+JavaScript）  
**编码格式**：UTF-8  
**推荐浏览器**：Chrome/Edge 最新版本

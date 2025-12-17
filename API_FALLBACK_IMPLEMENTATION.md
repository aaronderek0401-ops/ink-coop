# web_layout.html 接口自动回退实现文档

## 概述

修改了 `components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html`，实现了 **优先使用 PC 端接口，自动降级到 ESP32/Python 接口** 的机制。

## 核心原理

### 1. CONFIG 对象扩展
在原有配置基础上，增加了：
```javascript
const CONFIG = {
    PC_SERVICE_URL: 'http://localhost:5001',  // PC Flask 服务
    PYTHON_SERVICE_URL: 'http://localhost:5000',  // Python 转换服务
    ESP32_BASE_URL: '...',  // ESP32 设备地址
    
    // 新增辅助函数
    pcUrl(path)  // 返回 PC 服务 URL
    pythonUrlWithFallback(path)  // Python 服务（含回退）
}
```

### 2. 两个回退函数

#### A. fetchWithFallback(pcPath, esp32Path, options)
**用途**：ESP32 相关接口的自动降级  
**优先级**：PC 端 → ESP32  
**超时**：PC 端 5 秒

**适用场景**：
- 布局获取/保存 (`/getlayout`, `/setlayout`, `/getvocablayout`, `/setvocablayout`)
- 焦点配置 (`/setfocusconfig`, `/getfocusconfig`)
- 子数组配置 (`/setsubarrayconfig`, `/getsubarrayconfig`)
- 屏幕控制命令 (`/command?commandText=...`)
- 文件管理 (`/files`, `/command?commandText=$SD/...`)
- 图标加载 (`/api/icon/data`)

#### B. fetchWithFallbackForPython(path, options)
**用途**：Python 服务相关接口的自动降级  
**优先级**：PC 端 → Python 后端  
**超时**：PC 端 5 秒

**适用场景**：
- 字体转换 (`/convert_ttf_to_bin`)
- 图像转换 (`/convert_image_to_bin`)
- 文件下载 (`/download_bin_file`)
- 文件保存 (`/save_font_file`, `/save_bitmap_file`)
- 文件列表 (`/list_fonts`, `/list_bitmaps`)
- 文件获取 (`/get_font_file`, `/get_bitmap_file`)

## 修改详情

### 修改的接口调用点（共 50+ 处）

#### 1. 图标加载接口
```javascript
// 修改前
const response = await fetch(`/api/icon/data/${iconIndex}`);

// 修改后
const response = await fetchWithFallback(
    `/api/icon/binary/${iconIndex}`,
    `/api/icon/data/${iconIndex}`
);
```

#### 2. 布局操作接口
```javascript
// 获取布局
const response = await fetchWithFallback(endpoints.get, endpoints.get);

// 保存布局
const response = await fetchWithFallback(endpoints.set, endpoints.set, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(layoutData)
});
```

#### 3. 屏幕控制接口
```javascript
// 修改前
const widthResponse = await fetch(
    `${currentOrigin}/command?commandText=$set/screenXSize=${width}&PAGE=0`
);

// 修改后
const widthCommandPath = `/command?commandText=$set/screenXSize=${width}&PAGE=0`;
const widthResponse = await fetchWithFallback(widthCommandPath, widthCommandPath);
```

#### 4. 焦点/子数组配置接口
```javascript
// setfocusconfig
const response = await fetchWithFallback('/setfocusconfig', '/setfocusconfig', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(config)
});

// getfocusconfig
const focusPath = '/getfocusconfig?screen_type=' + currentScreenMode;
const response = await fetchWithFallback(focusPath, focusPath);
```

#### 5. Python 服务接口（字体/图像转换）
```javascript
// 修改前
const response = await fetch(CONFIG.pythonUrl('/convert_ttf_to_bin'), {
    method: 'POST',
    ...
});

// 修改后
const response = await fetchWithFallbackForPython('/convert_ttf_to_bin', {
    method: 'POST',
    ...
});
```

#### 6. 文件管理接口
```javascript
// 刷新 SD 卡文件列表
const response = await fetchWithFallback('/files?path=/', '/files?path=/');

// 删除文件
const deleteCmd = `/command?commandText=$SD/Delete=${encodeURIComponent(filename)}`;
const response = await fetchWithFallback(deleteCmd, deleteCmd);
```

## 使用场景

### 场景 1：PC 单机运行（优先 PC 端）
```
用户启动 Flask 服务 (localhost:5001)
    ↓
打开 web_layout.html
    ↓
所有接口优先使用 localhost:5001
    ↓
如果 PC 服务不可用，自动降级到 ESP32/Python
```

### 场景 2：ESP32 网页界面（自动选择 ESP32）
```
浏览器打开 http://192.168.0.2:8848/
    ↓
加载 web_layout.html
    ↓
尝试连接 PC 服务 (localhost:5001) → 超时/失败
    ↓
自动降级到 ESP32 本地接口 (http://192.168.0.2:8848/)
    ↓
正常工作
```

## 日志输出

### 成功情况
```
✓ PC endpoint success: /getlayout
✓ ESP32 endpoint success: /getlayout
✓ PC endpoint success for Python API: /convert_ttf_to_bin
✓ Python endpoint success: /convert_ttf_to_bin
```

### 失败情况
```
⚠ PC endpoint failed (/getlayout): PC endpoint timeout
✗ Both endpoints failed. PC: PC endpoint timeout, ESP32: 404 Not Found
✗ Both endpoints failed. PC: PC endpoint timeout, Python: Connection refused
```

## 注意事项

1. **超时设置**：PC 端接口有 5 秒超时限制，避免长时间卡顿
2. **相对路径**：所有接口都使用相对路径（如 `/getlayout`），由回退函数处理完整 URL
3. **FormData 支持**：回退函数完全支持 FormData 和 POST 请求
4. **错误处理**：建议在 try-catch 中捕获错误，显示给用户

## 测试建议

1. **仅 ESP32 可用**：关闭 PC Flask 服务，验证自动降级
2. **仅 PC 可用**：断网或关闭 ESP32 设备，验证 PC 优先工作
3. **两者都可用**：验证 PC 端优先被使用（检查日志）
4. **两者都不可用**：验证提示清晰的错误信息

## 相关文件

- `web_layout_standalone.html` - PC 单机版本（已同步相同逻辑）
- `config_server.py` - PC Flask 后端服务
- `components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html` - ESP32 嵌入版本（本次修改）

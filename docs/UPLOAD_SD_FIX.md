# 🔧 上传到 SD 卡错误修复

## ❌ 问题描述

### 错误信息
```
POST http://192.168.0.4:8848/upload net::ERR_CONNECTION_REFUSED
```

### 原因分析
- **网页地址**: `http://192.168.0.2:8848/`
- **代码中硬编码的地址**: `http://192.168.0.4:8848/upload`
- **问题**: IP 地址不匹配导致连接失败

---

## ✅ 修复方案

### 修改内容
修改 `uploadFileToSD()` 函数，让它**自动检测当前网页的主机地址**，而不是使用硬编码的 IP。

### 修复后的代码
```javascript
async function uploadFileToSD(file, path = '/') {
    return new Promise((resolve, reject) => {
        // ... FormData 构建 ...
        
        // 自动获取当前网页的主机地址和端口
        let uploadUrl;
        if (window.location.protocol === 'file:') {
            // 本地文件打开时，使用默认地址
            uploadUrl = 'http://192.168.0.2:8848/upload';
            console.warn('检测到本地文件模式，使用默认地址:', uploadUrl);
        } else {
            // 从 ESP32 网页服务器打开时，使用当前主机地址
            uploadUrl = `${window.location.protocol}//${window.location.host}/upload`;
            console.log('使用当前网页地址:', uploadUrl);
        }
        
        xhr.open('POST', uploadUrl, true);
        xhr.send(formData);
    });
}
```

---

## 🎯 工作原理

### 场景 1: 从 ESP32 打开网页
```
网页地址: http://192.168.0.2:8848/web_layout.html
↓
window.location.protocol = "http:"
window.location.host = "192.168.0.2:8848"
↓
uploadUrl = "http://192.168.0.2:8848/upload" ✅ 正确
```

### 场景 2: 本地文件打开
```
网页地址: file:///G:/A_BL_Project/.../web_layout.html
↓
window.location.protocol = "file:"
↓
uploadUrl = "http://192.168.0.2:8848/upload" (默认值) ✅ 回退方案
```

### 场景 3: ESP32 IP 改变
```
旧地址: http://192.168.0.2:8848/
新地址: http://192.168.0.100:8848/ (路由器重新分配 IP)
↓
代码自动适配新地址 ✅ 无需修改代码
```

---

## 📋 测试步骤

### 1. 刷新网页
```
按 F5 或 Ctrl+R 刷新 http://192.168.0.2:8848/web_layout.html
```

### 2. 打开浏览器控制台
```
按 F12 → Console 标签
```

### 3. 尝试上传文件
点击 "📤 上传到 SD 卡" 按钮

### 4. 查看控制台输出
应该看到：
```
使用当前网页地址: http://192.168.0.2:8848/upload
上传进度: 25.00%
上传进度: 50.00%
上传进度: 75.00%
上传进度: 100.00%
上传成功: OK
```

---

## 🔍 调试技巧

### 检查当前网页地址
在浏览器控制台输入：
```javascript
console.log(window.location.href);
// 输出: http://192.168.0.2:8848/web_layout.html

console.log(`${window.location.protocol}//${window.location.host}/upload`);
// 输出: http://192.168.0.2:8848/upload
```

### 检查 ESP32 IP 地址
1. 查看 ESP32 串口输出
2. 查看路由器 DHCP 分配表
3. 使用工具扫描局域网:
   ```bash
   # Windows
   arp -a
   
   # 或使用网络扫描工具
   Advanced IP Scanner
   ```

---

## ⚙️ 配置说明

### 修改默认地址 (可选)
如果您的 ESP32 使用**固定 IP**，可以修改默认地址：

```javascript
// 在 web_layout.html 中找到:
if (window.location.protocol === 'file:') {
    uploadUrl = 'http://192.168.0.2:8848/upload';  // ← 改成您的 ESP32 IP
}
```

### 常见 ESP32 网络配置
```cpp
// 固定 IP 配置 (在 ESP32 代码中)
IPAddress local_IP(192, 168, 0, 100);    // 固定 IP
IPAddress gateway(192, 168, 0, 1);       // 网关
IPAddress subnet(255, 255, 255, 0);      // 子网掩码

WiFi.config(local_IP, gateway, subnet);
WiFi.begin(ssid, password);
```

---

## 🐛 常见错误及解决

### 错误 1: `ERR_CONNECTION_REFUSED`
**原因**: IP 地址不匹配  
**解决**: 
1. 确认网页地址和上传地址一致
2. 检查浏览器控制台输出的 `uploadUrl`
3. 确保 ESP32 已连接 WiFi

### 错误 2: `ERR_NETWORK_CHANGED`
**原因**: ESP32 IP 地址改变  
**解决**: 
1. 重新刷新网页 (使用新 IP 打开)
2. 或在 ESP32 中配置固定 IP

### 错误 3: `HTTP 500` 或 `HTTP 413`
**原因**: 
- 500: SD 卡未插入或损坏
- 413: 文件太大

**解决**:
1. 检查 SD 卡是否正常插入
2. 格式化 SD 卡为 FAT32
3. 减小文件大小 (如降低图片尺寸)

### 错误 4: 上传进度卡在 0%
**原因**: CORS 跨域问题  
**解决**: 必须从 ESP32 网页服务器打开网页，不要用本地文件

---

## 📊 上传流程图

```
用户操作
    ↓
点击 "上传到 SD 卡"
    ↓
调用 uploadImageBinToSD()
    ↓
创建 File 对象
    ↓
调用 uploadFileToSD(file, '/')
    ↓
检测网页打开方式
    ├─→ 从 ESP32 打开: 使用 window.location.host
    └─→ 本地文件打开: 使用默认地址 192.168.0.2:8848
    ↓
构建 FormData
    ├─ path: '/'
    ├─ size: 文件大小
    └─ file: 文件对象
    ↓
XMLHttpRequest POST
    ↓
ESP32 接收文件
    ↓
保存到 SD 卡 /sd/xxx.bin
    ↓
返回 200 OK
    ↓
显示 "✅ 已成功上传到 SD 卡"
```

---

## ✅ 验证修复

### 1. 上传字库 .bin 文件
```
1. 生成 fangsong_gb2312_16x16.bin
2. 点击 "📤 上传到 SD 卡"
3. 查看控制台: 使用当前网页地址: http://192.168.0.2:8848/upload
4. 等待上传完成: ✅ 已成功上传到 SD 卡
```

### 2. 上传图片 .bin 文件
```
1. 转换图片为 logo_240x416.bin
2. 点击 "📤 上传到 SD 卡"
3. 查看控制台输出
4. 验证上传成功
```

### 3. 在 ESP32 中验证
```cpp
void checkSDCard() {
    if (SD.exists("/sd/logo_240x416.bin")) {
        ESP_LOGI(TAG, "✅ 文件已成功上传到 SD 卡!");
        
        File file = SD.open("/sd/logo_240x416.bin");
        ESP_LOGI(TAG, "文件大小: %d 字节", file.size());
        file.close();
    } else {
        ESP_LOGE(TAG, "❌ SD 卡中未找到文件");
    }
}
```

---

## 🎯 总结

**修复内容**:
- ✅ 移除硬编码的 IP 地址 `192.168.0.4`
- ✅ 自动检测当前网页的主机地址
- ✅ 支持 ESP32 IP 动态变化
- ✅ 支持本地文件打开时的回退方案

**优势**:
- 🔄 **自动适配**: ESP32 IP 改变无需修改代码
- 🌐 **通用性强**: 适用于任何网络环境
- 🛡️ **健壮性**: 本地文件打开时有默认值
- 📝 **可调试**: 控制台输出详细日志

**现在上传功能应该正常工作了！** ✅

# 配置服务器测试指南

## 快速开始测试

### 步骤 1: 启动配置服务器

```bash
python config_server.py
```

你应该看到类似的输出：
```
Starting Config Server...
Resource directory: G:\A_BL_Project\inkScree_fuben\components\resource
Serving on http://localhost:5001

Endpoints:
  GET  http://localhost:5001/api/config/focus?screen_type=main|vocab
  POST http://localhost:5001/api/config/focus
  GET  http://localhost:5001/api/config/subarray?screen_type=main|vocab
  POST http://localhost:5001/api/config/subarray
```

### 步骤 2: 测试 API 端点

使用 curl 或 Postman 测试以下端点：

#### 2.1 获取焦点配置

```bash
curl http://localhost:5001/api/config/focus?screen_type=main
```

预期响应：
```json
{
  "count": 1,
  "focusable_indices": [0],
  "screen_type": "main"
}
```

#### 2.2 保存焦点配置

```bash
curl -X POST http://localhost:5001/api/config/focus \
  -H "Content-Type: application/json" \
  -d '{"count":5,"focusable_indices":[0,1,2,3,4],"screen_type":"main"}'
```

预期响应：
```json
{
  "status": "success",
  "message": "Focus config saved for main"
}
```

#### 2.3 获取子数组配置

```bash
curl http://localhost:5001/api/config/subarray?screen_type=vocab
```

预期响应：
```json
{
  "sub_arrays": [],
  "screen_type": "vocab"
}
```

#### 2.4 保存子数组配置

```bash
curl -X POST http://localhost:5001/api/config/subarray \
  -H "Content-Type: application/json" \
  -d '{"sub_arrays":[{"parent_index":0,"sub_indices":[1,2,3],"sub_count":3}],"screen_type":"vocab"}'
```

预期响应：
```json
{
  "status": "success",
  "message": "Subarray config saved for vocab"
}
```

#### 2.5 列出所有配置

```bash
curl http://localhost:5001/api/config/list
```

#### 2.6 健康检查

```bash
curl http://localhost:5001/api/health
```

### 步骤 3: 在浏览器中测试 Web UI

1. 在浏览器中打开 `web_layout.html`
2. 打开开发者工具（F12）查看控制台和网络标签
3. 点击"从设备加载焦点配置"按钮
4. 在浏览器控制台中观察：
   - 应该看到 `http://localhost:5001/api/config/focus?screen_type=...` 的请求
   - 应该看到成功加载的配置数据

### 步骤 4: 验证文件系统

检查 `components/resource/` 目录：

```bash
# 列出目录
dir components\resource\

# 查看主界面焦点配置文件
type components\resource\main_focusable_rects_config.json
```

保存配置后，文件内容应该被更新。

## 常见问题

### Q: "连接被拒绝"错误

**A:** 确保配置服务器正在运行。检查：
- 终端窗口是否还在运行 `python config_server.py`
- 是否有其他进程占用了 5001 端口

```bash
# 在 PowerShell 中检查端口
Get-NetTCPConnection -LocalPort 5001
```

### Q: "文件找不到"错误

**A:** 检查 `components/resource/` 目录是否存在，并包含所需的 JSON 文件：

```bash
# 初始化目录和文件
python init_resource.py
```

### Q: 修改不生效

**A:** 检查：
1. 配置服务器是否正在运行
2. 浏览器控制台中是否有错误消息
3. `components/resource/` 中的文件是否有读写权限
4. 防火墙是否阻止了 localhost:5001

### Q: 跨域请求失败

**A:** 这不应该发生，因为配置服务器启用了 CORS。但如果出现，检查：
- 浏览器是否支持 CORS（所有现代浏览器都支持）
- 请求头是否正确（应包含 `Content-Type: application/json`）

## 文件监控

在开发过程中，你可以监控配置文件的变化：

### PowerShell 监控脚本

创建 `watch_config.ps1`:

```powershell
$path = "G:\A_BL_Project\inkScree_fuben\components\resource"
$filter = "*.json"

$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = $path
$watcher.Filter = $filter
$watcher.IncludeSubdirectories = $false

$action = {
    $name = $Event.SourceEventArgs.Name
    $changeType = $Event.SourceEventArgs.ChangeType
    $timeStamp = Get-Date -Format "HH:mm:ss"
    Write-Host "$timeStamp - $changeType: $name"
}

Register-ObjectEvent -InputObject $watcher -EventName "Changed" -Action $action
Register-ObjectEvent -InputObject $watcher -EventName "Created" -Action $action

Write-Host "监控 $path 中的配置文件变化..."
Write-Host "按 Ctrl+C 停止监控"

while ($true) { Start-Sleep -Milliseconds 100 }
```

运行监控：
```bash
powershell -ExecutionPolicy Bypass -File watch_config.ps1
```

## 完整工作流测试

1. **启动服务器**
   ```bash
   python config_server.py
   ```

2. **在另一个终端中打开浏览器**
   ```bash
   start "" "file:///G:/A_BL_Project/inkScree_fuben/components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html"
   ```

3. **在 Web UI 中**
   - 打开浏览器开发者工具（F12）
   - 选择"网络"标签
   - 点击"从设备加载焦点配置"
   - 观察网络请求和响应

4. **修改配置**
   - 在 Web UI 中修改复选框
   - 点击"保存焦点配置到设备"

5. **验证保存**
   ```bash
   type components\resource\vocab_focusable_rects_config.json
   ```
   应该看到更新后的配置

## 服务器日志

运行配置服务器时，你会看到详细的日志：

```
 * Running on http://0.0.0.0:5001
 * Debug mode: on
Loading focus config from: G:\A_BL_Project\inkScree_fuben\components\resource\main_focusable_rects_config.json
Focus config saved for main: G:\A_BL_Project\inkScree_fuben\components\resource\main_focusable_rects_config.json
```

这对调试和验证文件操作很有帮助。

## 性能考虑

- 配置文件通常很小（< 10KB），所以没有性能问题
- API 响应应该在毫秒级
- 文件 I/O 是同步的，但对于小文件影响不大

## 下一步

配置服务器测试完成后，你可以：

1. 更新配置文件内容
2. 重新编译 ESP32 项目
3. 烧录到设备进行实际测试

```bash
idf.py build
idf.py -p COM3 flash
```

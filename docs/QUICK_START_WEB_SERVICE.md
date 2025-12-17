# 🚀 Web 字体转换快速启动指南

## ⚠️ 如果看到 "Failed to fetch" 错误

这表示 **Python 服务没有运行**。按以下步骤操作：

---

## 📝 步骤 1：打开新的命令行窗口

**Windows PowerShell**:
- 按 `Win + X`，选择 "Windows PowerShell"
- 或者在开始菜单搜索 "PowerShell"

---

## 📝 步骤 2：进入项目目录

```powershell
cd G:\A_BL_Project\inkScree_fuben
```

---

## 📝 步骤 3：检查依赖是否安装

```powershell
pip show Flask flask-cors Pillow
```

**如果显示 "WARNING: Package(s) not found"**，运行：

```powershell
pip install Flask flask-cors Pillow
```

---

## 📝 步骤 4：启动 Python 服务

```powershell
python tools/ttf_to_gfx_webservice.py
```

**成功标志**：看到以下输出

```
============================================================
TTF 转 GFX Web 服务启动中...
============================================================
访问: http://localhost:5000
API 端点: http://localhost:5000/convert_ttf_to_gfx

在 web_layout.html 中可以通过 fetch 调用此API
============================================================
 * Serving Flask app 'ttf_to_gfx_webservice'
 * Debug mode: on
 * Running on http://127.0.0.1:5000    <-- 看到这行表示成功！
 * Running on http://192.168.2.200:5000
Press CTRL+C to quit
```

---

## 📝 步骤 5：测试服务是否正常

**保持上面的窗口打开**，再打开一个新的 PowerShell 窗口：

```powershell
cd G:\A_BL_Project\inkScree_fuben
python tools/test_web_service.py
```

**应该看到**：
```
✅ 转换成功！
💾 文件已保存到本地: ...
```

---

## 📝 步骤 6：在 Web 界面使用

1. **刷新浏览器** 中的 `web_layout.html`
2. 上传 TTF 字体
3. 输入要转换的文字
4. 点击 **"转换为 GFX 格式"**

**成功标志**：
```
✅ 转换成功！
字符数: 10
位图大小: 200 bytes
文件已下载到浏览器

💾 本地已保存:
G:\A_BL_Project\inkScree_fuben\components\fonts\xxx16pt7b.h
📁 文件名: xxx16pt7b.h
✨ 可以直接在代码中使用了！
```

---

## 🔧 故障排除

### 问题 1: "python 不是内部或外部命令"

**解决**：
```powershell
# 使用完整路径
E:\Python\python.exe tools/ttf_to_gfx_webservice.py
```

### 问题 2: 端口 5000 被占用

**解决**：修改 `tools/ttf_to_gfx_webservice.py` 最后一行：

```python
# 原来
app.run(host='0.0.0.0', port=5000, debug=True)

# 改为
app.run(host='0.0.0.0', port=5001, debug=True)
```

同时修改 `web_layout.html` 中的 URL：
```javascript
// 原来
const pythonServiceUrl = 'http://localhost:5000/convert_ttf_to_gfx';

// 改为
const pythonServiceUrl = 'http://localhost:5001/convert_ttf_to_gfx';
```

### 问题 3: 转换后文件没保存

**检查**：
```powershell
dir components\fonts\*.h
```

应该能看到新生成的 `.h` 文件。

---

## ✅ 成功运行的标志

1. **PowerShell 窗口显示**：
   ```
   * Running on http://127.0.0.1:5000
   * Debugger is active!
   ```

2. **浏览器显示**：
   ```
   ✅ 转换成功！
   💾 本地已保存: ...
   ```

3. **文件存在**：
   ```powershell
   dir components\fonts\xxx16pt7b.h
   # 应该显示文件大小和时间
   ```

---

## 🎯 常用命令总结

```powershell
# 启动服务
cd G:\A_BL_Project\inkScree_fuben
python tools/ttf_to_gfx_webservice.py

# 测试服务（在另一个窗口）
python tools/test_web_service.py

# 直接转换（不用 Web 界面）
python tools/quick_convert.py

# 生成中文位图
python tools/generate_bitmap_arrays.py

# 查看已生成的字体
dir components\fonts\*.h
```

---

## 💡 提示

- **保持 PowerShell 窗口打开**，关闭窗口会停止服务
- **每次重启电脑后需要重新启动服务**
- 可以创建批处理文件 `.bat` 一键启动

### 创建一键启动脚本

创建文件 `启动字体服务.bat`：

```batch
@echo off
cd /d G:\A_BL_Project\inkScree_fuben
python tools/ttf_to_gfx_webservice.py
pause
```

双击运行即可！

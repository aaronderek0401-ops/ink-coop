@echo off
REM 墨水屏布局编辑器 - 快速启动脚本
REM 此脚本启动 Python 服务器，然后在浏览器中打开网页编辑器

echo.
echo ====================================================
echo   墨水屏布局编辑器 v2.0 - 快速启动工具
echo ====================================================
echo.

REM 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM 检查 Python 是否已安装
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ❌ 错误：未检测到 Python 环境
    echo.
    echo 请确保已安装 Python 3.8 或更高版本
    echo 下载地址: https://www.python.org/downloads/
    echo.
    pause
    exit /b 1
)

echo ✅ 已检测到 Python 环境
echo.

REM 检查是否安装了 Flask 和 python-cors
echo 正在检查依赖...
python -c "import flask" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ⚠️  缺少 Flask 依赖，正在安装...
    pip install flask python-cors -q
)

python -c "import flask" >nul 2>&1
if %ERRORLEVEL% EQ 0 (
    echo ✅ Flask 和 python-cors 已可用
) else (
    echo ❌ 依赖安装失败
    echo.
    echo 请手动运行以下命令：
    echo pip install flask python-cors
    echo.
    pause
    exit /b 1
)

echo.
echo ====================================================
echo   启动配置
echo ====================================================
echo.
echo 1️⃣  启动 Python 服务器 (localhost:5001)...
echo.

REM 启动 Python 服务器
start /MIN python config_server.py

REM 等待服务器启动
timeout /t 2 /nobreak

echo.
echo ====================================================
echo   打开网页编辑器
echo ====================================================
echo.

REM 检查浏览器
for /f "tokens=*" %%i in ('REG QUERY "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Desktop /s 2^>nul') do (
    if "%%i"=="Desktop" goto :found_desktop
)

:found_desktop
REM 尝试用默认浏览器打开文件
start "" "web_layout_standalone.html"

echo ✅ 网页编辑器已启动
echo.
echo 🌐 URL: file:///%SCRIPT_DIR%web_layout_standalone.html
echo.
echo 📋 功能检查清单：
echo    □ 页面加载完成
echo    □ 顶部显示环境状态（绿色=PC, 蓝色=ESP32）
echo    □ 可以加载和编辑布局
echo    □ 可以拖拽矩形和图标
echo.
echo 💡 提示：
echo    - 如果环境检测失败，请检查 Python 服务器是否正常运行
echo    - 关闭此窗口将结束 Python 服务
echo    - 要停止服务，请按 Ctrl+C
echo.
echo ====================================================
echo.

REM 保持窗口打开
echo 服务器正在运行... (按 Ctrl+C 停止)
timeout /t -1


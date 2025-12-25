@echo off
REM TTF 字体转换工具 - 启动脚本
REM 图形化选择字体、参数、导出格式

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   🔤 TTF 字体转换工具
echo ========================================
echo.

REM 获取脚本所在目录
for %%I in ("%~dp0.") do set "TOOLS_DIR=%%~fI"

REM 检查 Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ❌ 错误: 未找到 Python
    echo.
    echo 请确保已安装 Python 3.6+ 并添加到 PATH
    echo 访问: https://www.python.org/downloads/
    echo.
    pause
    exit /b 1
)

REM 检查 Pillow
python -c "import PIL" >nul 2>&1
if errorlevel 1 (
    echo ⚠️  Pillow 未安装，正在安装...
    pip install --upgrade Pillow
    if errorlevel 1 (
        echo ❌ Pillow 安装失败
        pause
        exit /b 1
    )
)

REM 检查 GUI 文件
set "GUI_FILE=%TOOLS_DIR%\ttf_font_converter_gui.py"
if not exist "%GUI_FILE%" (
    echo ❌ 错误: 找不到 GUI 文件: %GUI_FILE%
    echo.
    pause
    exit /b 1
)

echo ✅ 检查完成，正在启动 GUI...
echo.
echo 💡 功能:
echo   • 选择 TTF 或 OTF 字体文件
echo   • 设置字体大小（8-128pt）
echo   • 选择或自定义字符集
echo   • 导出为 GFX (.h) 或 BIN 格式
echo.

REM 启动 GUI
cd /d "%TOOLS_DIR%"
python ttf_font_converter_gui.py

if errorlevel 1 (
    echo.
    echo ❌ GUI 启动失败
    pause
    exit /b 1
)

exit /b 0

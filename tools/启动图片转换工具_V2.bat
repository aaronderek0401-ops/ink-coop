@echo off
REM ESP32 e-ink 图片转换工具 - GUI 启动脚本 (改进版)
REM 功能更完整的图形界面，包含快速预设按钮和更清晰的布局

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   🖼️  ESP32 e-ink 图片转换工具 V2
echo ========================================
echo.

REM 获取脚本所在目录
for %%I in ("%~dp0.") do set "TOOLS_DIR=%%~fI"

REM 检查 Python 是否安装
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

REM 检查 Pillow 是否安装
python -c "import PIL" >nul 2>&1
if errorlevel 1 (
    echo ⚠️  Pillow 未安装，正在安装...
    echo.
    pip install --upgrade Pillow
    if errorlevel 1 (
        echo ❌ Pillow 安装失败
        pause
        exit /b 1
    )
)

REM 检查 GUI 文件是否存在
set "GUI_FILE=%TOOLS_DIR%\image_converter_gui_v2.py"
if not exist "%GUI_FILE%" (
    echo ❌ 错误: 找不到 GUI 文件
    echo 路径: %GUI_FILE%
    echo.
    pause
    exit /b 1
)

REM 检查转换器文件是否存在
set "CONVERTER_FILE=%TOOLS_DIR%\image_converter_tool.py"
if not exist "%CONVERTER_FILE%" (
    echo ❌ 错误: 找不到转换器文件
    echo 路径: %CONVERTER_FILE%
    echo.
    pause
    exit /b 1
)

echo ✅ 检查完成，正在启动 GUI...
echo.
echo 💡 提示:
echo   • 选择单个图片或包含图片的文件夹
echo   • 设置输出宽度和高度（像素）
echo   • 点击"🚀 开始转换"开始处理
echo   • 支持 JPG, PNG, BMP, GIF 等格式
echo.

REM 启动 GUI
cd /d "%TOOLS_DIR%"
python image_converter_gui_v2.py

if errorlevel 1 (
    echo.
    echo ❌ GUI 启动失败
    pause
    exit /b 1
)

exit /b 0

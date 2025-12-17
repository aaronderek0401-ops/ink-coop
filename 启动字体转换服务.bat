@echo off
chcp 65001 > nul
echo ============================================================
echo TTF 转 GFX Web 服务 - 一键启动
echo ============================================================
echo.

cd /d "%~dp0"

echo [1/3] 检查 Python 是否可用...
python --version > nul 2>&1
if errorlevel 1 (
    echo ❌ 错误: 找不到 Python
    echo.
    echo 请确保 Python 已安装并添加到 PATH 环境变量
    echo.
    pause
    exit /b 1
)
echo ✅ Python 已安装

echo.
echo [2/3] 检查依赖包...
python -c "import flask, flask_cors, PIL" > nul 2>&1
if errorlevel 1 (
    echo ⚠️  依赖包未安装，正在安装...
    echo.
    pip install Flask flask-cors Pillow
    if errorlevel 1 (
        echo ❌ 安装失败
        pause
        exit /b 1
    )
    echo ✅ 依赖包安装成功
) else (
    echo ✅ 依赖包已安装
)

echo.
echo [3/3] 启动 Web 服务...
echo.
echo ============================================================
echo 🚀 服务启动中...
echo ============================================================
echo.
echo 访问地址: http://localhost:5000
echo 按 Ctrl+C 停止服务
echo.
echo ============================================================
echo.

python tools\ttf_to_gfx_webservice.py

pause

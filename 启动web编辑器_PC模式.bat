@echo off
chcp 65001 >nul
echo.
echo ═══════════════════════════════════════════════════════════
echo   启动 Web 编辑器 - PC 模式
echo ═══════════════════════════════════════════════════════════
echo.

REM 检查 Python 是否安装
python --version >nul 2>&1
if errorlevel 1 (
    echo ✗ 错误：未找到 Python，请先安装 Python
    pause
    exit /b 1
)

echo ✓ Python 已找到
echo.
echo 启动步骤：
echo   1. 启动 Flask 配置服务器...
echo   2. 打开网页编辑器...
echo.

REM 启动 Flask 服务器
start "Flask Config Server - localhost:5001" cmd /k python config_server.py

REM 等待服务器启动
echo ⏳ 等待 Flask 服务器启动 (3 秒)...
timeout /t 3 /nobreak

REM 打开网页（使用 web_layout.html）
start "" "components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html"

echo.
echo ✓ Web 编辑器已启动！
echo.
echo 访问地址：file:///G:/A_BL_Project/inkScree_fuben/components/grbl_esp32s3/Grbl_Esp32/data/web_layout.html
echo 后端服务：http://localhost:5001
echo.
echo 状态提示：
echo   � 绿色徽章 = PC 模式（已连接 localhost:5001）
echo   � 橙色徽章 = ESP32 模式（后备方案）
echo.
echo 功能说明：
echo   - 自动检测环境，优先使用 PC 服务
echo   - 若 PC 服务不可用，自动降级到 ESP32
echo   - 支持实时切换，无需重启
echo.
pause

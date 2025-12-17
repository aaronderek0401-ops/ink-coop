@echo off
chcp 65001 > nul
echo ================================================
echo  .bin 字库生成功能 - 快速测试
echo ================================================
echo.
echo 📋 测试准备:
echo   1. 确保有中文TTF字体文件 (如 SimHei.ttf)
echo   2. ESP32已连接 (可选)
echo   3. SD卡已插入 (可选)
echo.
echo ================================================
echo  正在启动 Python 服务...
echo ================================================
echo.

cd /d "%~dp0"
python tools\ttf_to_gfx_webservice.py

pause

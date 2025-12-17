@echo off
REM 启动配置服务器的 Windows 批处理文件
echo 启动 Flask 配置服务器...
echo.
echo 配置服务器将在 http://localhost:5001 上运行
echo.
echo 提示: 在使用 web UI 之前，请确保此服务器正在运行！
echo.
python config_server.py
pause
g:\A_BL_Project\inkScree_fuben\start_config_server.bat

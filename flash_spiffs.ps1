# ========================================
# SPIFFS分区烧录脚本
# 用于更新JSON布局文件到ESP32设备
# ========================================

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  SPIFFS分区烧录工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查data目录中的文件
Write-Host "[1/4] 检查data目录..." -ForegroundColor Yellow
$dataPath = "data"
if (-not (Test-Path $dataPath)) {
    Write-Host "❌ 错误: data目录不存在！" -ForegroundColor Red
    exit 1
}

$jsonFiles = Get-ChildItem -Path $dataPath -Filter "*.json"
Write-Host "✅ 找到 $($jsonFiles.Count) 个JSON文件:" -ForegroundColor Green
foreach ($file in $jsonFiles) {
    Write-Host "   - $($file.Name)" -ForegroundColor Gray
}
Write-Host ""

# 2. 创建SPIFFS镜像
Write-Host "[2/4] 创建SPIFFS镜像..." -ForegroundColor Yellow
$spiffsImage = "build\spiffs.bin"

# 使用ESP-IDF的mkspiffs工具（如果可用）或esptool
# 注意：分区大小必须与partitions.csv中定义的一致
$spiffsSize = 0x100000  # 1MB，根据你的partitions.csv调整

Write-Host "⚠️  需要手动创建SPIFFS镜像，步骤如下:" -ForegroundColor Yellow
Write-Host ""
Write-Host "方法1 - 使用idf.py (推荐):" -ForegroundColor Cyan
Write-Host "  idf.py spiffs-flash" -ForegroundColor White
Write-Host ""
Write-Host "方法2 - 使用mkspiffs工具:" -ForegroundColor Cyan
Write-Host "  mkspiffs -c data -b 4096 -p 256 -s 0x100000 build/spiffs.bin" -ForegroundColor White
Write-Host "  esptool.py --port COM3 write_flash 0x310000 build/spiffs.bin" -ForegroundColor White
Write-Host ""
Write-Host "方法3 - 重新编译并烧录全部:" -ForegroundColor Cyan
Write-Host "  idf.py build flash" -ForegroundColor White
Write-Host ""

# 等待用户确认
Write-Host "按任意键继续手动操作，或Ctrl+C退出..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

# ESP-IDF 编译脚本
$env:IDF_PATH = 'e:\Espressif\frameworks\esp-idf-v4.4.8'
$env:PATH = "$env:IDF_PATH\tools;$env:PATH"

cd "g:\A_BL_Project\inkScree_fuben"
python -m idf build

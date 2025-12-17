#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

# 读取源文件
source_file = r"g:\A_BL_Project\inkScree_fuben\components\grbl_esp32s3\Grbl_Esp32\data\web_layout.html"
output_file = r"g:\A_BL_Project\inkScree_fuben\web_layout_standalone.html"

try:
    with open(source_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # 去掉第一行（const char* WEB_LAYOUT_HTML = R"rawliteral(）
    # 去掉最后一行（)rawliteral(";）
    if len(lines) > 2:
        pure_html_lines = lines[1:-1]
    else:
        print("文件太短")
        sys.exit(1)
    
    # 写入新文件
    with open(output_file, 'w', encoding='utf-8') as f:
        f.writelines(pure_html_lines)
    
    print(f"✅ 成功提取纯HTML内容")
    print(f"   源文件: {source_file}")
    print(f"   输出文件: {output_file}")
    print(f"   总行数: {len(pure_html_lines)}")

except Exception as e:
    print(f"❌ 错误: {e}")
    sys.exit(1)

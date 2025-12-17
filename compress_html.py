#!/usr/bin/env python3
import gzip
import os

html_file = r"g:\A_BL_Project\inkScree_fuben\components\grbl_esp32s3\Grbl_Esp32\data\web_layout.html"
gz_file = html_file + ".gz"

with open(html_file, 'rb') as f:
    data = f.read()

with gzip.open(gz_file, 'wb') as f:
    f.write(data)

orig_size = len(data)
compressed_data = gzip.compress(data)
compressed_size = len(compressed_data)
ratio = (1 - compressed_size / orig_size) * 100

print(f"原始文件大小: {orig_size:,} 字节")
print(f"压缩文件大小: {compressed_size:,} 字节")
print(f"压缩比: {ratio:.1f}%")
print(f"已保存: {gz_file}")

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import gzip
import os

src = 'web_layout.html'
dst = 'web_layout.html.gz'

# 读取原始文件
with open(src, 'rb') as f:
    data = f.read()

# 压缩
with open(dst, 'wb') as f:
    f.write(gzip.compress(data, 9))

# 计算统计信息
orig = os.path.getsize(src)
comp = os.path.getsize(dst)
ratio = 100 * (1 - comp / orig)

print('✅ 压缩完成:')
print(f'  原始: {orig:,} 字节')
print(f'  压缩: {comp:,} 字节')
print(f'  比率: {ratio:.1f}% 减少')
print(f'  文件: {dst}')

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试仿宋_GB2312.ttf字体是否支持中文
"""

from PIL import ImageFont

TTF_PATH = "components/fonts/仿宋_GB2312.ttf"
TEST_CHARS = "测试文字ABC123"

print("=" * 60)
print("🔍 测试字体文件: 仿宋_GB2312.ttf")
print("=" * 60)

try:
    font = ImageFont.truetype(TTF_PATH, 16)
    print("✅ 字体加载成功")
    print()
    
    for char in TEST_CHARS:
        bbox = font.getbbox(char)
        width = bbox[2] - bbox[0]
        height = bbox[3] - bbox[1]
        
        status = "✅" if width > 10 else "❌"
        char_type = "中文" if ord(char) > 127 else "ASCII"
        
        print(f"{status} '{char}' ({char_type}): {width}x{height} pixels")
    
    print()
    print("=" * 60)
    print("结论:")
    
    # 测试中文字符
    chinese_chars = [c for c in TEST_CHARS if ord(c) > 127]
    if chinese_chars:
        bbox = font.getbbox(chinese_chars[0])
        width = bbox[2] - bbox[0]
        
        if width > 10:
            print("✅ 仿宋_GB2312.ttf 完美支持中文显示！")
            print("👍 可以使用此字体转换中文 GFX 格式")
        else:
            print("❌ 仿宋_GB2312.ttf 不支持中文")
            print("⚠️  需要使用其他中文字体（如黑体、宋体）")
    
    print("=" * 60)
    
except Exception as e:
    print(f"❌ 测试失败: {e}")

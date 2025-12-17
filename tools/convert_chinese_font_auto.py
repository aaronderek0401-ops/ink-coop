#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
使用 Windows 系统字体生成中文 GFX 字体
自动搜索系统中可用的中文字体
"""

from PIL import Image, ImageDraw, ImageFont
import os
import glob

# 配置
CHARS = "测试文字ABC123"
FONT_SIZE = 16
OUTPUT_NAME = "chinese_test"

# Windows 系统字体路径
FONT_PATHS = [
    r"C:\Windows\Fonts\simhei.ttf",      # 黑体
    r"C:\Windows\Fonts\msyh.ttc",        # 微软雅黑
    r"C:\Windows\Fonts\simsun.ttc",      # 宋体
    r"C:\Windows\Fonts\STZHONGS.TTF",    # 华文中宋
    r"C:\Windows\Fonts\simkai.ttf",      # 楷体
]

def find_chinese_font():
    """查找可用的中文字体"""
    for font_path in FONT_PATHS:
        if os.path.exists(font_path):
            return font_path
    
    # 如果都不存在，尝试搜索所有 TTF 文件
    print("⚠️  预定义字体未找到，正在搜索系统字体...")
    fonts_dir = r"C:\Windows\Fonts"
    ttf_files = glob.glob(os.path.join(fonts_dir, "*.ttf"))
    ttc_files = glob.glob(os.path.join(fonts_dir, "*.ttc"))
    
    all_fonts = ttf_files + ttc_files
    if all_fonts:
        print(f"找到 {len(all_fonts)} 个字体文件，尝试第一个...")
        return all_fonts[0]
    
    return None

def test_font_chinese_support(font, chars):
    """测试字体是否真正支持中文"""
    for char in chars:
        if ord(char) > 127:  # 非 ASCII 字符
            bbox = font.getbbox(char)
            width = bbox[2] - bbox[0]
            if width < 10:  # 如果中文字符宽度小于 10，可能不支持
                return False
    return True

def generate_gfx_font():
    """生成 GFX 格式字体"""
    
    print("=" * 60)
    print("🔍 正在查找支持中文的字体...")
    print("=" * 60)
    
    font_path = find_chinese_font()
    
    if not font_path:
        print("❌ 未找到可用的中文字体！")
        print("\n请手动下载中文字体，例如：")
        print("  - simhei.ttf (黑体)")
        print("  - 放到 C:\\Windows\\Fonts\\ 目录")
        return
    
    print(f"✅ 找到字体: {os.path.basename(font_path)}")
    print()
    
    try:
        font = ImageFont.truetype(font_path, FONT_SIZE)
        print(f"✅ 字体加载成功: {font_path}")
    except Exception as e:
        print(f"❌ 加载字体失败: {e}")
        return
    
    # 测试中文支持
    if not test_font_chinese_support(font, "测试"):
        print(f"⚠️  警告: 字体可能不支持中文，尝试使用...")
    
    print(f"📝 字符: {CHARS}")
    print(f"📏 字号: {FONT_SIZE}")
    print()
    
    # 按 Unicode 排序
    sorted_chars = sorted(set(CHARS), key=lambda c: ord(c))
    
    bitmaps = []
    glyphs = []
    bitmap_offset = 0
    
    # 为每个字符生成位图
    for char in sorted_chars:
        try:
            # 获取字符边界
            bbox = font.getbbox(char)
            width = bbox[2] - bbox[0]
            height = bbox[3] - bbox[1]
            x_offset = bbox[0]
            y_offset = -bbox[3]
            
            if width <= 0 or height <= 0:
                print(f"⚠️  字符 '{char}' (U+{ord(char):04X}) 尺寸为0，跳过")
                continue
            
            # 创建图像
            img = Image.new('1', (width, height), 1)
            draw = ImageDraw.Draw(img)
            draw.text((-bbox[0], -bbox[1]), char, font=font, fill=0)
            
            # 转换为字节数组
            pixels = list(img.getdata())
            bitmap_bytes = []
            
            for y in range(height):
                for x in range(0, width, 8):
                    byte = 0
                    for bit in range(8):
                        if x + bit < width:
                            pixel_index = y * width + x + bit
                            if pixels[pixel_index] == 0:
                                byte |= (0x80 >> bit)
                    bitmap_bytes.append(byte)
            
            bitmaps.extend(bitmap_bytes)
            
            x_advance = width + 1
            
            glyphs.append({
                'char': char,
                'unicode': ord(char),
                'bitmap_offset': bitmap_offset,
                'width': width,
                'height': height,
                'x_advance': x_advance,
                'x_offset': x_offset,
                'y_offset': y_offset,
                'bitmap_size': len(bitmap_bytes)
            })
            
            bitmap_offset += len(bitmap_bytes)
            
            char_display = char if ord(char) < 127 else f"'{char}'"
            print(f"✓ {char_display:8s} (U+{ord(char):04X}): {width:2d}x{height:2d}, {len(bitmap_bytes):3d} bytes")
            
        except Exception as e:
            print(f"❌ 处理字符 '{char}' 失败: {e}")
            continue
    
    if len(glyphs) == 0:
        print("❌ 没有成功转换任何字符")
        return
    
    print()
    print(f"✅ 成功转换 {len(glyphs)} 个字符")
    print(f"📊 总位图大小: {len(bitmaps)} bytes")
    print()
    
    # 生成 .h 文件
    first_char = glyphs[0]['unicode']
    last_char = glyphs[-1]['unicode']
    
    h_content = f"""// Font: {OUTPUT_NAME} {FONT_SIZE}pt
// Generated by convert_chinese_font_auto.py
// Source: {os.path.basename(font_path)}
// Characters: {CHARS}
// Character count: {len(glyphs)}
// Bitmap size: {len(bitmaps)} bytes

#ifndef _{OUTPUT_NAME.upper()}_{FONT_SIZE}PT7B_H_
#define _{OUTPUT_NAME.upper()}_{FONT_SIZE}PT7B_H_

#include <Adafruit_GFX.h>

const uint8_t {OUTPUT_NAME}{FONT_SIZE}ptBitmaps[] PROGMEM = {{
"""
    
    # 位图数据
    for i, byte in enumerate(bitmaps):
        if i % 16 == 0:
            h_content += "    "
        h_content += f"0x{byte:02X}"
        if i < len(bitmaps) - 1:
            h_content += ", "
        if (i + 1) % 16 == 0 and i < len(bitmaps) - 1:
            h_content += "\n"
    
    h_content += "\n};\n\n"
    
    # Glyphs 数据
    h_content += f"const GFXglyph {OUTPUT_NAME}{FONT_SIZE}ptGlyphs[] PROGMEM = {{\n"
    
    for g in glyphs:
        h_content += f"    {{ {g['bitmap_offset']:5d}, {g['width']:3d}, {g['height']:3d}, "
        h_content += f"{g['x_advance']:3d}, {g['x_offset']:4d}, {g['y_offset']:4d} }}"
        char_display = g['char'] if g['unicode'] < 127 else g['char']
        h_content += f",  // U+{g['unicode']:04X} '{char_display}'\n"
    
    h_content = h_content.rstrip(',\n') + '\n'
    h_content += "};\n\n"
    
    # Font 结构
    h_content += f"""const GFXfont {OUTPUT_NAME}{FONT_SIZE}pt7b PROGMEM = {{
    (uint8_t *){OUTPUT_NAME}{FONT_SIZE}ptBitmaps,
    (GFXglyph *){OUTPUT_NAME}{FONT_SIZE}ptGlyphs,
    0x{first_char:04X}, 0x{last_char:04X}, {FONT_SIZE}
}};

#endif // _{OUTPUT_NAME.upper()}_{FONT_SIZE}PT7B_H_
"""
    
    # 保存文件
    output_path = f"components/fonts/{OUTPUT_NAME}{FONT_SIZE}pt7b.h"
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(h_content)
    
    print(f"✅ 文件已生成: {output_path}")
    print()
    print("=" * 60)
    print("使用方法:")
    print("=" * 60)
    print(f"#include \"../fonts/{OUTPUT_NAME}{FONT_SIZE}pt7b.h\"")
    print(f"display.setFont(&{OUTPUT_NAME}{FONT_SIZE}pt7b);")
    print(f'display.print("{CHARS}");')
    print("=" * 60)

if __name__ == "__main__":
    generate_gfx_font()

# -*- coding: utf-8 -*-
"""
使用 PIL/Pillow 生成 Adafruit GFX 字体文件 - 修复版
支持非连续字符（使用空 Glyph 填充）
"""

from PIL import Image, ImageDraw, ImageFont
import struct

# 要转换的字符
CHARS = "测试中文字体显示正常温度湿度电量充电WiFi连接成功失败0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz°C%:/-_.()"

# 字体配置
TTF_PATH = r"C:\Windows\Fonts\simhei.ttf"
FONT_SIZE = 16
OUTPUT_NAME = "simhei16pt7b_chinese"

def generate_gfx_font():
    """生成 GFX 字体文件"""
    
    print(f"加载字体: {TTF_PATH}")
    print(f"字体大小: {FONT_SIZE}pt")
    
    try:
        font = ImageFont.truetype(TTF_PATH, FONT_SIZE)
    except Exception as e:
        print(f"❌ 加载字体失败: {e}")
        return
    
    # 按 Unicode 顺序排序并去重
    sorted_chars = sorted(set(CHARS), key=lambda c: ord(c))
    print(f"字符数量: {len(sorted_chars)}")
    print(f"字符范围: {sorted_chars[0]} (0x{ord(sorted_chars[0]):04X}) ~ {sorted_chars[-1]} (0x{ord(sorted_chars[-1]):04X})")
    
    # 创建字符映射表
    char_map = {}
    bitmaps = []
    bitmap_offset = 0
    
    for char in sorted_chars:
        # 获取字符边界框
        try:
            bbox = font.getbbox(char)
            width = bbox[2] - bbox[0]
            height = bbox[3] - bbox[1]
            
            if width == 0 or height == 0:
                print(f"⚠️ 跳过空字符: {char}")
                continue
            
            # 创建图像
            img = Image.new('1', (width, height), 0)
            draw = ImageDraw.Draw(img)
            draw.text((-bbox[0], -bbox[1]), char, font=font, fill=1)
            
            # 转换为字节
            bitmap_bytes = []
            for y in range(height):
                byte_val = 0
                bit_count = 0
                for x in range(width):
                    if img.getpixel((x, y)):
                        byte_val |= (1 << (7 - bit_count))
                    bit_count += 1
                    if bit_count == 8:
                        bitmap_bytes.append(byte_val)
                        byte_val = 0
                        bit_count = 0
                # 处理剩余位
                if bit_count > 0:
                    bitmap_bytes.append(byte_val)
            
            bitmaps.extend(bitmap_bytes)
            
            # Glyph 信息
            glyph = {
                'bitmapOffset': bitmap_offset,
                'width': width,
                'height': height,
                'xAdvance': width + 1,
                'xOffset': bbox[0],
                'yOffset': -bbox[3]
            }
            char_map[char] = glyph
            bitmap_offset += len(bitmap_bytes)
            
            print(f"✓ {char}: {width}x{height}px, offset={glyph['bitmapOffset']}")
            
        except Exception as e:
            print(f"❌ 处理字符 '{char}' 失败: {e}")
    
    # 生成 .h 文件
    generate_header_file(bitmaps, char_map, sorted_chars)

def generate_header_file(bitmaps, char_map, sorted_chars):
    """生成 C 头文件"""
    
    output_file = f"{OUTPUT_NAME}.h"
    
    first_char_code = ord(sorted_chars[0])
    last_char_code = ord(sorted_chars[-1])
    char_range = last_char_code - first_char_code + 1
    
    print(f"\n生成头文件...")
    print(f"字符范围: 0x{first_char_code:04X} - 0x{last_char_code:04X} ({char_range} 个槽位)")
    print(f"实际字符: {len(char_map)} 个")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(f"#ifndef {OUTPUT_NAME.upper()}_H\n")
        f.write(f"#define {OUTPUT_NAME.upper()}_H\n\n")
        f.write("#include <Adafruit_GFX.h>\n\n")
        
        # Bitmaps
        f.write(f"const uint8_t {OUTPUT_NAME}Bitmaps[] PROGMEM = {{\n  ")
        for i, byte in enumerate(bitmaps):
            f.write(f"0x{byte:02X}")
            if i < len(bitmaps) - 1:
                f.write(", ")
            if (i + 1) % 12 == 0:
                f.write("\n  ")
        f.write(" };\n\n")
        
        # Glyphs - 为每个字符位置创建条目
        f.write(f"const GFXglyph {OUTPUT_NAME}Glyphs[] PROGMEM = {{\n")
        
        for i in range(char_range):
            char_code = first_char_code + i
            char = chr(char_code)
            
            if char in char_map:
                glyph = char_map[char]
                f.write(f"  {{ {glyph['bitmapOffset']:5d}, ")
                f.write(f"{glyph['width']:3d}, {glyph['height']:3d}, ")
                f.write(f"{glyph['xAdvance']:3d}, ")
                f.write(f"{glyph['xOffset']:4d}, {glyph['yOffset']:4d} }}")
                f.write(f",  // 0x{char_code:04X} '{char}'\n")
            else:
                # 空 Glyph（不显示）
                f.write(f"  {{     0,   0,   0,   0,    0,    0 }}")
                f.write(f",  // 0x{char_code:04X} (未定义)\n")
        
        f.write(" };\n\n")
        
        # Font
        f.write(f"const GFXfont {OUTPUT_NAME} PROGMEM = {{\n")
        f.write(f"  (uint8_t  *){OUTPUT_NAME}Bitmaps,\n")
        f.write(f"  (GFXglyph *){OUTPUT_NAME}Glyphs,\n")
        f.write(f"  0x{first_char_code:04X}, 0x{last_char_code:04X}, {FONT_SIZE} }};\n\n")
        f.write(f"#endif // {OUTPUT_NAME.upper()}_H\n")
    
    print(f"\n✅ 生成成功: {output_file}")
    print(f"位图大小: {len(bitmaps)} bytes")
    print(f"Glyph 数量: {char_range} 个（包含 {char_range - len(char_map)} 个空槽位）")
    print(f"⚠️ 警告: 文件可能很大，包含了所有字符范围的空槽位！")

if __name__ == "__main__":
    print("=" * 60)
    print("TTF 转 Adafruit GFX 中文字体生成器 (修复版)")
    print("=" * 60)
    print()
    
    generate_gfx_font()

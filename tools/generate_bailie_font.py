# -*- coding: utf-8 -*-
"""
生成只包含"爆裂"两个汉字的字体文件
"""

from PIL import Image, ImageDraw, ImageFont

# 字体配置
TTF_PATH = r"C:\Windows\Fonts\simhei.ttf"
FONT_SIZE = 16
OUTPUT_NAME = "bailie16pt7b"

# 只包含这两个字
CHARS = "爆裂"

def generate_gfx_font():
    """生成 GFX 字体文件"""
    
    print(f"加载字体: {TTF_PATH}")
    print(f"字体大小: {FONT_SIZE}pt")
    print(f"要生成的字符: {CHARS}")
    
    try:
        font = ImageFont.truetype(TTF_PATH, FONT_SIZE)
    except Exception as e:
        print(f"❌ 加载字体失败: {e}")
        return
    
    # 按 Unicode 顺序排序
    sorted_chars = sorted(set(CHARS), key=lambda c: ord(c))
    print(f"\n字符 Unicode:")
    for char in sorted_chars:
        print(f"  '{char}' = 0x{ord(char):04X}")
    
    bitmaps = []
    glyphs = []
    bitmap_offset = 0
    
    for char in sorted_chars:
        try:
            bbox = font.getbbox(char)
            width = bbox[2] - bbox[0]
            height = bbox[3] - bbox[1]
            
            print(f"\n处理字符: '{char}'")
            print(f"  尺寸: {width}x{height}px")
            
            if width == 0 or height == 0:
                print(f"  ⚠️ 跳过空字符")
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
                if bit_count > 0:
                    bitmap_bytes.append(byte_val)
            
            bitmaps.extend(bitmap_bytes)
            
            glyph = {
                'bitmapOffset': bitmap_offset,
                'width': width,
                'height': height,
                'xAdvance': width + 1,
                'xOffset': bbox[0],
                'yOffset': -bbox[3]
            }
            glyphs.append((char, glyph))
            bitmap_offset += len(bitmap_bytes)
            
            print(f"  ✓ 位图大小: {len(bitmap_bytes)} bytes")
            
        except Exception as e:
            print(f"❌ 处理字符 '{char}' 失败: {e}")
    
    # 生成 .h 文件
    generate_header_file(bitmaps, glyphs)

def generate_header_file(bitmaps, glyphs):
    """生成 C 头文件"""
    
    output_file = f"{OUTPUT_NAME}.h"
    
    first_char = ord(glyphs[0][0])
    last_char = ord(glyphs[-1][0])
    char_range = last_char - first_char + 1
    
    print(f"\n生成头文件: {output_file}")
    print(f"字符范围: 0x{first_char:04X} - 0x{last_char:04X}")
    print(f"总槽位: {char_range} (包含 {char_range - len(glyphs)} 个空槽位)")
    
    # 创建字符映射
    char_map = {ord(char): glyph for char, glyph in glyphs}
    
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
        
        # Glyphs - 为所有字符位置创建条目
        f.write(f"const GFXglyph {OUTPUT_NAME}Glyphs[] PROGMEM = {{\n")
        
        for i in range(char_range):
            char_code = first_char + i
            
            if char_code in char_map:
                # 实际字符
                glyph = char_map[char_code]
                char = chr(char_code)
                f.write(f"  {{ {glyph['bitmapOffset']:5d}, ")
                f.write(f"{glyph['width']:3d}, {glyph['height']:3d}, ")
                f.write(f"{glyph['xAdvance']:3d}, ")
                f.write(f"{glyph['xOffset']:4d}, {glyph['yOffset']:4d} }},")
                f.write(f"  // 0x{char_code:04X} '{char}'\n")
            else:
                # 空 Glyph（不显示）
                f.write(f"  {{     0,   0,   0,   0,    0,    0 }},")
                f.write(f"  // 0x{char_code:04X} (空)\n")
        
        f.write(" };\n\n")
        
        # Font
        f.write(f"const GFXfont {OUTPUT_NAME} PROGMEM = {{\n")
        f.write(f"  (uint8_t  *){OUTPUT_NAME}Bitmaps,\n")
        f.write(f"  (GFXglyph *){OUTPUT_NAME}Glyphs,\n")
        f.write(f"  0x{first_char:04X}, 0x{last_char:04X}, {FONT_SIZE} }};\n\n")
        f.write(f"#endif // {OUTPUT_NAME.upper()}_H\n")
    
    print(f"\n✅ 生成成功!")
    print(f"位图大小: {len(bitmaps)} bytes")
    print(f"Glyph 条目: {char_range} 个（实际字符: {len(glyphs)} 个）")
    print(f"⚠️ 文件会包含 {char_range - len(glyphs)} 个空 Glyph 槽位")

if __name__ == "__main__":
    print("=" * 60)
    print("生成 '爆裂' 字体文件")
    print("=" * 60)
    print()
    
    generate_gfx_font()

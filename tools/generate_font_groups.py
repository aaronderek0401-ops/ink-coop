# -*- coding: utf-8 -*-
"""
生成多个独立的中文字体文件
每个文件包含一个连续的字符范围
"""

from PIL import Image, ImageDraw, ImageFont

# 字体配置
TTF_PATH = r"C:\Windows\Fonts\simhei.ttf"
FONT_SIZE = 16

# 定义字符集（分组）
CHAR_GROUPS = {
    "ascii": " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~°",
    "chinese1": "中体充功失字常度成接文显正测温湿电示试败连量",  # 常用字1（连续）
}

def generate_font_group(group_name, chars, ttf_path, font_size):
    """生成单个字体组"""
    
    output_name = f"simhei{font_size}pt_{group_name}"
    
    print(f"\n生成字体组: {group_name}")
    print(f"字符: {chars[:20]}...")
    
    try:
        font = ImageFont.truetype(ttf_path, font_size)
    except Exception as e:
        print(f"❌ 加载字体失败: {e}")
        return False
    
    # 排序字符
    sorted_chars = sorted(set(chars), key=lambda c: ord(c))
    
    bitmaps = []
    glyphs = []
    bitmap_offset = 0
    
    for char in sorted_chars:
        try:
            bbox = font.getbbox(char)
            width = bbox[2] - bbox[0]
            height = bbox[3] - bbox[1]
            
            if width == 0 or height == 0:
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
            
        except Exception as e:
            print(f"❌ 处理字符 '{char}' 失败: {e}")
    
    # 生成 .h 文件
    output_file = f"{output_name}.h"
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(f"#ifndef {output_name.upper()}_H\n")
        f.write(f"#define {output_name.upper()}_H\n\n")
        f.write("#include <Adafruit_GFX.h>\n\n")
        
        # Bitmaps
        f.write(f"const uint8_t {output_name}Bitmaps[] PROGMEM = {{\n  ")
        for i, byte in enumerate(bitmaps):
            f.write(f"0x{byte:02X}")
            if i < len(bitmaps) - 1:
                f.write(", ")
            if (i + 1) % 12 == 0:
                f.write("\n  ")
        f.write(" };\n\n")
        
        # Glyphs
        f.write(f"const GFXglyph {output_name}Glyphs[] PROGMEM = {{\n")
        for char, glyph in glyphs:
            f.write(f"  {{ {glyph['bitmapOffset']:5d}, ")
            f.write(f"{glyph['width']:3d}, {glyph['height']:3d}, ")
            f.write(f"{glyph['xAdvance']:3d}, ")
            f.write(f"{glyph['xOffset']:4d}, {glyph['yOffset']:4d} }},")
            f.write(f"  // '{char}'\n")
        f.write(" };\n\n")
        
        # Font
        first_char = ord(glyphs[0][0])
        last_char = ord(glyphs[-1][0])
        
        f.write(f"const GFXfont {output_name} PROGMEM = {{\n")
        f.write(f"  (uint8_t  *){output_name}Bitmaps,\n")
        f.write(f"  (GFXglyph *){output_name}Glyphs,\n")
        f.write(f"  0x{first_char:04X}, 0x{last_char:04X}, {font_size} }};\n\n")
        f.write(f"#endif // {output_name.upper()}_H\n")
    
    print(f"✅ 生成成功: {output_file}")
    print(f"   字符数: {len(glyphs)}, 范围: 0x{first_char:04X}-0x{last_char:04X}")
    return True

if __name__ == "__main__":
    print("=" * 60)
    print("生成多组中文字体")
    print("=" * 60)
    
    for group_name, chars in CHAR_GROUPS.items():
        generate_font_group(group_name, chars, TTF_PATH, FONT_SIZE)
    
    print("\n" + "=" * 60)
    print("全部完成！")

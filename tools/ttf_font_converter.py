#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTF 字体转换工具包 - 核心库
支持 TTF -> GFX / BIN 转换
自定义字体大小、字符集、输出格式
"""

from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import struct
import json
from typing import List, Tuple, Dict
import os

# 常用符号和标点符号
_symbols = "0123456789" + \
           ".,;:!?()[]{}\"'`~@#$%^&*-_+=/<>|\\。，；：！？（）【】《》「」『』～·、… —"

# 音标符号 - 包括英文发音音标常用符号
_phonetic = "ːˈˌəæŋθðʃʒtʃdʒɪʊɒʌɑɔɛæɪaʊ"

# 生成 1 万个常用汉字的字符集
# 使用 Unicode 范围内的常用汉字：CJK Unified Ideographs（4E00-9FFF）
_extended_hanzi = ""
for code in range(0x4E00, 0x9FFF + 1, 1):  # 完整的 CJK 统一表意文字范围
    _extended_hanzi += chr(code)

# 预设字符集 - 只保留中文和英文两个选项，都包含符号和音标
CHAR_SETS = {
    '中文字符': _extended_hanzi + _symbols,  # 汉字 + 符号
    '英文字符': "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" + _symbols + _phonetic,  # 英文字母 + 符号 + 音标
}

class TTFConverter:
    """TTF 字体转换器"""
    
    def __init__(self, font_path: str, font_size: int = 16, charset: str = None):
        """
        初始化转换器
        
        参数:
            font_path: TTF 字体文件路径
            font_size: 字体大小（像素）
            charset: 字符集（字符串）
        """
        self.font_path = Path(font_path)
        self.font_size = font_size
        self.charset = charset or CHAR_SETS['全组合']
        self.font = None
        self.glyphs = {}
        self.bitmaps = bytearray()
        
        if not self.font_path.exists():
            raise FileNotFoundError(f"找不到字体文件: {self.font_path}")
        
        print(f"📚 加载字体: {self.font_path.name}")
        print(f"   大小: {self.font_size}pt")
        print(f"   字符数: {len(set(self.charset))}")
        
        try:
            self.font = ImageFont.truetype(str(self.font_path), self.font_size)
            print(f"   ✅ 加载成功")
        except Exception as e:
            print(f"   ❌ 加载失败: {e}")
            raise
    
    def generate_bitmap(self, char: str) -> Tuple[bytes, int, int]:
        """
        生成单个字符的位图
        
        返回:
            (位图字节, 宽度, 高度)
        """
        try:
            bbox = self.font.getbbox(char)
            width = bbox[2] - bbox[0]
            height = bbox[3] - bbox[1]
            
            if width == 0 or height == 0:
                return bytes(), 0, 0
            
            # 创建图像并绘制字符
            img = Image.new('1', (width, height), 0)
            draw = ImageDraw.Draw(img)
            draw.text((-bbox[0], -bbox[1]), char, font=self.font, fill=1)
            
            # 转换为字节流
            bitmap_bytes = bytearray()
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
            
            return bytes(bitmap_bytes), width, height
        
        except Exception as e:
            print(f"⚠️  生成位图失败 ({char}): {e}")
            return bytes(), 0, 0
    
    def convert_to_gfx(self, output_path: str = None) -> str:
        """
        转换为 Adafruit GFX 格式 (.h 文件)
        
        参数:
            output_path: 输出文件路径。如果为 None，则保存到字体文件所在目录
        
        返回:
            输出文件路径
        """
        if output_path is None:
            # 默认保存到字体文件所在目录，使用字体名称 + 字体大小 + .h 后缀
            output_path = self.font_path.parent / (self.font_path.stem + f"_{self.font_size}pt.h")
        else:
            output_path = Path(output_path)
            # 如果指定的路径是目录，自动生成文件名
            if output_path.is_dir() or (not output_path.suffix and not str(output_path).endswith('.h')):
                output_path = output_path / (self.font_path.stem + f"_{self.font_size}pt.h")
        
        print(f"\n🎨 转换为 GFX 格式...")
        print(f"   输出: {output_path}")
        
        # 处理字符
        sorted_chars = sorted(set(self.charset), key=lambda c: ord(c))
        glyphs = []
        self.bitmaps = bytearray()
        
        for char in sorted_chars:
            bitmap, width, height = self.generate_bitmap(char)
            
            if width > 0 and height > 0:
                glyph = {
                    'char': char,
                    'code': ord(char),
                    'bitmapOffset': len(self.bitmaps),
                    'width': width,
                    'height': height,
                    'xAdvance': width + 1,
                    'dX': 0,
                    'dY': 0,
                }
                glyphs.append(glyph)
                self.bitmaps.extend(bitmap)
        
        # 生成 C 头文件
        header = self._generate_gfx_header(glyphs)
        
        # 确保输出目录存在
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(header)
        
        print(f"   ✅ 生成完成 ({len(glyphs)} 个字符)")
        print(f"   大小: {len(self.bitmaps)} 字节")
        
        return str(output_path)
    
    def convert_to_bin(self, output_path: str = None) -> str:
        """
        转换为二进制格式 (.bin 文件)
        
        参数:
            output_path: 输出文件路径。如果为 None，则保存到字体文件所在目录
        
        返回:
            输出文件路径
        """
        if output_path is None:
            # 默认保存到字体文件所在目录，使用字体名称 + 字体大小 + .bin 后缀
            output_path = self.font_path.parent / (self.font_path.stem + f"_{self.font_size}pt.bin")
        else:
            output_path = Path(output_path)
            # 如果指定的路径是目录，自动生成文件名
            if output_path.is_dir() or (not output_path.suffix and not str(output_path).endswith('.bin')):
                output_path = output_path / (self.font_path.stem + f"_{self.font_size}pt.bin")
        
        print(f"\n📦 转换为二进制格式...")
        print(f"   输出: {output_path}")
        
        # 处理字符
        sorted_chars = sorted(set(self.charset), key=lambda c: ord(c))
        glyphs = []
        self.bitmaps = bytearray()
        
        for char in sorted_chars:
            bitmap, width, height = self.generate_bitmap(char)
            
            if width > 0 and height > 0:
                glyph = {
                    'char': char,
                    'code': ord(char),
                    'bitmapOffset': len(self.bitmaps),
                    'width': width,
                    'height': height,
                }
                glyphs.append(glyph)
                self.bitmaps.extend(bitmap)
        
        # 生成二进制文件
        bin_data = bytearray()
        
        # 文件头
        bin_data.extend(b'TTFG')  # 魔数
        bin_data.extend(self.font_size.to_bytes(2, 'little'))  # 字体大小
        bin_data.extend(len(glyphs).to_bytes(2, 'little'))  # 字形数量
        
        # 字形表
        glyph_table_size = len(glyphs) * 12
        bin_data.extend(glyph_table_size.to_bytes(4, 'little'))  # 字形表大小
        
        for glyph in glyphs:
            bin_data.extend(glyph['code'].to_bytes(4, 'little'))
            bin_data.extend(glyph['bitmapOffset'].to_bytes(4, 'little'))
            bin_data.extend(glyph['width'].to_bytes(2, 'little'))
            bin_data.extend(glyph['height'].to_bytes(2, 'little'))
        
        # 位图数据
        bin_data.extend(self.bitmaps)
        
        # 确保输出目录存在
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_path, 'wb') as f:
            f.write(bin_data)
        
        print(f"   ✅ 生成完成 ({len(glyphs)} 个字符)")
        print(f"   大小: {len(bin_data)} 字节")
        
        return str(output_path)
    
    def _generate_gfx_header(self, glyphs: List[Dict]) -> str:
        """生成 GFX C 头文件"""
        font_name = self.font_path.stem.replace(' ', '_').replace('-', '_')
        size_name = f"{self.font_size}pt"
        
        lines = [
            "#ifndef " + f"{font_name}_{size_name}_H",
            "#define " + f"{font_name}_{size_name}_H",
            "",
            "#include <Adafruit_GFX.h>",
            "",
            f"// 字体: {self.font_path.name}",
            f"// 大小: {self.font_size}pt",
            f"// 字符: {len(glyphs)}",
            "",
            "// 字形数据",
            "const uint8_t " + f"{font_name}_{size_name}_glyphs[] = {{",
        ]
        
        # 添加位图数据
        hex_data = ', '.join(f"0x{b:02X}" for b in self.bitmaps)
        # 每行最多 16 个字节
        for i in range(0, len(self.bitmaps), 16):
            chunk = ', '.join(f"0x{b:02X}" for b in self.bitmaps[i:i+16])
            lines.append("    " + chunk + ",")
        
        lines.append("};")
        lines.append("")
        
        # 字形元数据
        lines.append("// 字形元数据")
        lines.append("struct Glyph {")
        lines.append("    uint32_t code;")
        lines.append("    uint32_t bitmapOffset;")
        lines.append("    uint16_t width;")
        lines.append("    uint16_t height;")
        lines.append("    int8_t xAdvance;")
        lines.append("    int8_t dX, dY;")
        lines.append("};")
        lines.append("")
        
        lines.append("const Glyph " + f"{font_name}_{size_name}_glyphTable[] = {{")
        for glyph in glyphs:
            char_display = repr(glyph['char'])[1:-1] if glyph['char'].isprintable() else f"U+{glyph['code']:04X}"
            lines.append(f"    {{{glyph['code']}, {glyph['bitmapOffset']}, {glyph['width']}, {glyph['height']}, {glyph['xAdvance']}, {glyph['dX']}, {glyph['dY']}}}  // {char_display}")
        
        lines.append("};")
        lines.append("")
        lines.append(f"const uint16_t {font_name}_{size_name}_glyphCount = {len(glyphs)};")
        lines.append("")
        lines.append("#endif")
        
        return "\n".join(lines)


def main():
    """命令行接口"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='TTF 字体转换工具包',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例:
  # 转换为 GFX 格式
  python ttf_font_converter.py -i font.ttf -s 16 -c "常用汉字" -f gfx
  
  # 转换为二进制格式
  python ttf_font_converter.py -i font.ttf -s 16 -c "全组合" -f bin -o output.bin
  
  # 列出所有预设字符集
  python ttf_font_converter.py --list-charsets
        '''
    )
    
    parser.add_argument('-i', '--input', required=False, help='TTF 字体文件路径')
    parser.add_argument('-s', '--size', type=int, default=16, help='字体大小（像素，默认: 16）')
    parser.add_argument('-c', '--charset', default=None, help='字符集名称或自定义字符串')
    parser.add_argument('-f', '--format', choices=['gfx', 'bin'], default='gfx', help='输出格式（默认: gfx）')
    parser.add_argument('-o', '--output', default=None, help='输出文件路径')
    parser.add_argument('--list-charsets', action='store_true', help='列出所有预设字符集')
    
    args = parser.parse_args()
    
    if args.list_charsets:
        print("\n📋 预设字符集:")
        print("=" * 60)
        for name, chars in CHAR_SETS.items():
            print(f"\n{name} ({len(set(chars))} 个字符):")
            print(f"  {chars[:60]}...")
        return
    
    # 转换时需要字体文件
    if not args.input:
        parser.error("需要指定 -i/--input 字体文件路径，或使用 --list-charsets 列出字符集")
    
    # 解析字符集
    if args.charset:
        if args.charset in CHAR_SETS:
            charset = CHAR_SETS[args.charset]
            print(f"📌 使用预设: {args.charset}")
        else:
            charset = args.charset
            print(f"📌 使用自定义字符集 ({len(set(charset))} 个字符)")
    else:
        charset = CHAR_SETS['全组合']
        print(f"📌 使用默认字符集: 全组合")
    
    # 创建转换器
    converter = TTFConverter(args.input, args.size, charset)
    
    # 转换
    if args.format == 'gfx':
        output = converter.convert_to_gfx(args.output)
        print(f"\n✅ 输出: {output}")
    else:
        output = converter.convert_to_bin(args.output)
        print(f"\n✅ 输出: {output}")


if __name__ == '__main__':
    main()

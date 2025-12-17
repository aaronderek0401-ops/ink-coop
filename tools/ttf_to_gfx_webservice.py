#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTF 转 GFX Web 服务
可以被 web_layout.html 调用的 HTTP API
"""

from flask import Flask, request, jsonify, send_file
from flask_cors import CORS
from PIL import Image, ImageDraw, ImageFont
import io
import base64
import os

app = Flask(__name__)
CORS(app)  # 允许跨域请求

@app.route('/', methods=['GET'])
def health_check():
    """健康检查端点"""
    return jsonify({
        'status': 'running',
        'service': 'TTF to GFX Converter',
        'version': '1.0',
        'endpoints': {
            'convert': '/convert_ttf_to_gfx'
        }
    })

@app.route('/convert_ttf_to_gfx', methods=['POST'])
def convert_ttf_to_gfx():
    """
    接收 TTF 文件和字符列表，返回 GFX .h 文件
    
    请求格式：
    {
        "ttf_base64": "base64编码的TTF文件内容",
        "chars": "要转换的字符列表",
        "font_size": 16,
        "font_name": "myfont"
    }
    """
    try:
        data = request.json
        
        # 解码 TTF 文件
        ttf_data = base64.b64decode(data['ttf_base64'])
        chars = data['chars']
        font_size = int(data.get('font_size', 16))
        font_name = data.get('font_name', 'customfont')
        
        # 创建临时字体对象
        font = ImageFont.truetype(io.BytesIO(ttf_data), font_size)
        
        # 按 Unicode 排序
        sorted_chars = sorted(set(chars), key=lambda c: ord(c))
        
        # 生成位图数据
        bitmaps = []
        glyphs = []
        bitmap_offset = 0
        
        for char in sorted_chars:
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
                'yOffset': -bbox[3],
                'char': char
            }
            glyphs.append(glyph)
            bitmap_offset += len(bitmap_bytes)
        
        # 生成 .h 文件内容
        h_content = generate_h_file(font_name, font_size, bitmaps, glyphs)
        
        # 保存到本地 components/fonts/ 目录
        output_filename = f"{font_name}{font_size}pt7b.h"
        output_dir = os.path.join(os.path.dirname(__file__), '..', 'components', 'fonts')
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, output_filename)
        
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(h_content)
        
        print(f"✅ 字体文件已保存: {output_path}")
        
        return jsonify({
            'success': True,
            'h_file': h_content,
            'char_count': len(glyphs),
            'file_size': len(bitmaps),
            'saved_path': output_path,
            'filename': output_filename
        })
        
    except Exception as e:
        print(f"❌ 转换失败: {e}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 400


def generate_h_file(font_name, font_size, bitmaps, glyphs):
    """生成 C 头文件内容"""
    
    output_name = f"{font_name}{font_size}pt7b"
    lines = []
    
    lines.append(f"#ifndef {output_name.upper()}_H")
    lines.append(f"#define {output_name.upper()}_H\n")
    lines.append("#include <Adafruit_GFX.h>\n")
    
    # Bitmaps
    lines.append(f"const uint8_t {output_name}Bitmaps[] PROGMEM = {{")
    bitmap_lines = []
    for i in range(0, len(bitmaps), 12):
        chunk = bitmaps[i:i+12]
        bitmap_lines.append("  " + ", ".join(f"0x{b:02X}" for b in chunk))
    lines.append(",\n".join(bitmap_lines))
    lines.append(" };\n")
    
    # Glyphs
    lines.append(f"const GFXglyph {output_name}Glyphs[] PROGMEM = {{")
    glyph_lines = []
    for g in glyphs:
        glyph_line = f"  {{ {g['bitmapOffset']:5d}, {g['width']:3d}, {g['height']:3d}, "
        glyph_line += f"{g['xAdvance']:3d}, {g['xOffset']:4d}, {g['yOffset']:4d} }},"
        glyph_line += f"  // 0x{ord(g['char']):04X} '{g['char']}'"
        glyph_lines.append(glyph_line)
    lines.append("\n".join(glyph_lines))
    lines.append(" };\n")
    
    # Font
    first_char = ord(glyphs[0]['char'])
    last_char = ord(glyphs[-1]['char'])
    
    lines.append(f"const GFXfont {output_name} PROGMEM = {{")
    lines.append(f"  (uint8_t  *){output_name}Bitmaps,")
    lines.append(f"  (GFXglyph *){output_name}Glyphs,")
    lines.append(f"  0x{first_char:04X}, 0x{last_char:04X}, {font_size} }};\n")
    lines.append(f"#endif // {output_name.upper()}_H")
    
    return "\n".join(lines)


@app.route('/health', methods=['GET'])
def health():
    """健康检查"""
    return jsonify({'status': 'ok'})


@app.route('/save_bitmap_font', methods=['POST'])
def save_bitmap_font():
    """
    保存位图字体文件到本地 components/resource/fonts/ 目录
    
    请求格式:
    {
        "content": "文件内容",
        "filename": "fangsong24pt_bitmaps.h"
    }
    """
    try:
        data = request.json
        content = data.get('content', '')
        filename = data.get('filename', 'bitmap_font.h')
        
        # 确定保存路径（相对于当前脚本所在目录）
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        fonts_dir = os.path.join(project_root, 'components', 'resource', 'fonts')
        
        # 确保目录存在
        os.makedirs(fonts_dir, exist_ok=True)
        
        # 保存文件
        file_path = os.path.join(fonts_dir, filename)
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        
        return jsonify({
            'success': True,
            'message': '文件保存成功',
            'saved_path': file_path,
            'filename': filename
        })
        
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/save_image_bitmap', methods=['POST'])
def save_image_bitmap():
    """
    保存图片位图文件到本地 components/resource/bitmap/ 目录
    
    请求格式:
    {
        "content": "文件内容",
        "filename": "my_image_bitmap.h"
    }
    """
    try:
        data = request.json
        content = data.get('content', '')
        filename = data.get('filename', 'image_bitmap.h')
        
        # 确定保存路径
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        bitmap_dir = os.path.join(project_root, 'components', 'resource', 'bitmap')
        
        # 确保目录存在
        os.makedirs(bitmap_dir, exist_ok=True)
        
        # 保存文件
        file_path = os.path.join(bitmap_dir, filename)
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        
        return jsonify({
            'success': True,
            'message': '位图文件保存成功',
            'saved_path': file_path,
            'filename': filename
        })
        
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/convert_ttf_to_bitmap_python', methods=['POST'])
def convert_ttf_to_bitmap_python():
    """
    使用 Python + Pillow 生成高质量位图（比浏览器 Canvas 质量更好）
    
    请求格式:
    {
        "ttf_base64": "base64编码的TTF文件内容",
        "chars": "要转换的字符",
        "font_size": 16,
        "font_name": "fangsong"
    }
    """
    try:
        data = request.json
        
        # 解码 TTF 文件
        ttf_data = base64.b64decode(data['ttf_base64'])
        chars = data['chars']
        font_size = int(data.get('font_size', 16))
        font_name = data.get('font_name', 'customfont')
        output_name = data.get('output_name', font_name)
        
        # 保存临时 TTF 文件
        temp_ttf = io.BytesIO(ttf_data)
        font = ImageFont.truetype(temp_ttf, font_size)
        
        # 生成头文件内容
        h_content = f"""// Font Bitmaps: {output_name} {font_size}pt
// Generated by Python + Pillow (High Quality)
// Characters: {chars}
// 每个字符一个独立的位图数组

#ifndef _{output_name.upper()}_{font_size}PT_H_
#define _{output_name.upper()}_{font_size}PT_H_

#include <Arduino.h>

"""
        
        char_info = []
        
        for char in chars:
            bbox = font.getbbox(char)
            width = bbox[2] - bbox[0]
            height = bbox[3] - bbox[1]
            
            if width <= 0 or height <= 0:
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
                            if pixels[y * width + x + bit] == 0:
                                byte |= (0x80 >> bit)
                    bitmap_bytes.append(byte)
            
            # 生成数组名（使用 Unicode 码点，添加前缀避免冲突）
            array_name = f"BITMAP_{output_name.upper()}_{ord(char):04X}"
            
            # 写入数组定义
            h_content += f"// '{char}' (U+{ord(char):04X}) - {width}x{height}\n"
            h_content += f"const uint8_t {array_name}[] PROGMEM = {{\n"
            h_content += "    "
            
            for i, byte in enumerate(bitmap_bytes):
                h_content += f"0x{byte:02X}"
                if i < len(bitmap_bytes) - 1:
                    h_content += ", "
                if (i + 1) % 16 == 0 and i < len(bitmap_bytes) - 1:
                    h_content += "\n    "
            
            h_content += "\n};\n\n"
            
            char_info.append({
                'char': char,
                'unicode': ord(char),
                'width': width,
                'height': height,
                'array_name': array_name,
                'size': len(bitmap_bytes)
            })
        
        # 生成单字符绘制函数
        func_name_base = ''.join([w.capitalize() for w in output_name.split('_')])
        
        h_content += f"""// 绘制单个字符
template<typename T>
void draw{func_name_base}Char(T& display, int16_t x, int16_t y, uint16_t charCode) {{
    const uint8_t* bitmap = nullptr;
    uint8_t width = 0, height = {font_size};

    switch(charCode) {{
"""
        
        for info in char_info:
            h_content += f"        case 0x{info['unicode']:04X}: bitmap = {info['array_name']}; width = {info['width']}; height = {info['height']}; break;\n"
        
        h_content += """        default: return; // 未知字符
    }

    if (bitmap) {
        display.drawBitmap(x, y, bitmap, width, height, GxEPD_BLACK);
    }
}

// 绘制 UTF-8 字符串
template<typename T>
void draw"""
        
        h_content += f"""{func_name_base}String(T& display, int16_t x, int16_t y, const char* text) {{
    int16_t currentX = x;
    const uint8_t* p = (const uint8_t*)text;

    while (*p) {{
        uint16_t charCode = 0;
        // UTF-8 解码
        if ((*p & 0x80) == 0) {{  // 1字节 ASCII
            charCode = *p++;
        }} else if ((*p & 0xE0) == 0xC0) {{  // 2字节
            charCode = ((*p++ & 0x1F) << 6);
            charCode |= (*p++ & 0x3F);
        }} else if ((*p & 0xF0) == 0xE0) {{  // 3字节（中文）
            charCode = ((*p++ & 0x0F) << 12);
            charCode |= ((*p++ & 0x3F) << 6);
            charCode |= (*p++ & 0x3F);
        }} else if ((*p & 0xF8) == 0xF0) {{  // 4字节
            charCode = ((*p++ & 0x07) << 18);
            charCode |= ((*p++ & 0x3F) << 12);
            charCode |= ((*p++ & 0x3F) << 6);
            charCode |= (*p++ & 0x3F);
        }} else {{
            p++;  // 跳过无效字节
            continue;
        }}

        // 绘制字符并移动光标
        draw{func_name_base}Char(display, currentX, y, charCode);
        
        // 根据实际字符宽度移动
        uint8_t charWidth = {font_size};  // 默认宽度
        switch(charCode) {{
"""
        
        for info in char_info:
            h_content += f"            case 0x{info['unicode']:04X}: charWidth = {info['width']}; break;\n"
        
        h_content += """        }
        currentX += charWidth + 1;  // 字符宽度 + 1像素间隔
    }
}

"""
        
        h_content += f"#endif // _{output_name.upper()}_{font_size}PT_H_\n"
        
        # 自动保存到本地
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        fonts_dir = os.path.join(project_root, 'components', 'resource', 'fonts')
        os.makedirs(fonts_dir, exist_ok=True)
        
        filename = f"{output_name}{font_size}pt_bitmaps.h"
        file_path = os.path.join(fonts_dir, filename)
        
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(h_content)
        
        return jsonify({
            'success': True,
            'content': h_content,
            'saved_path': file_path,
            'filename': filename,
            'char_count': len(char_info),
            'message': f'✅ Python高质量渲染完成! 已保存到: {file_path}'
        })
        
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/list_fonts', methods=['GET'])
def list_fonts():
    """列出 components/resource/fonts 目录下的所有字体文件（.h 和 .bin）"""
    try:
        # 获取项目根目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        fonts_dir = os.path.join(project_root, 'components', 'resource', 'fonts')
        
        # 如果目录不存在，创建它
        if not os.path.exists(fonts_dir):
            os.makedirs(fonts_dir)
            return jsonify({
                'success': True,
                'fonts': [],
                'count': 0,
                'directory': fonts_dir,
                'message': 'components/resource/fonts 目录已创建，当前为空'
            })
        
        # 列出所有 .h 和 .bin 文件
        font_files = []
        for filename in os.listdir(fonts_dir):
            if filename.endswith('.h') or filename.endswith('.bin'):
                file_path = os.path.join(fonts_dir, filename)
                file_size = os.path.getsize(file_path)
                file_type = 'binary' if filename.endswith('.bin') else 'header'
                
                # 尝试解析 .bin 文件的字库尺寸
                font_size = None
                if filename.endswith('.bin'):
                    try:
                        # 从文件名推断尺寸 (例如: fangsong_16x16.bin)
                        import re
                        size_match = re.search(r'(\d+)x(\d+)', filename)
                        if size_match:
                            font_size = f"{size_match.group(1)}x{size_match.group(2)}"
                    except:
                        pass
                
                font_files.append({
                    'name': filename,
                    'size': file_size,
                    'size_kb': round(file_size / 1024, 2),
                    'type': file_type,
                    'font_size': font_size,
                    'path': file_path
                })
        
        font_files.sort(key=lambda x: x['name'])
        
        return jsonify({
            'success': True,
            'fonts': font_files,
            'count': len(font_files),
            'directory': fonts_dir
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/list_bitmaps', methods=['GET'])
def list_bitmaps():
    """列出 components/resource/bitmap 目录下的所有位图文件（.h 和 .bin）"""
    try:
        # 获取项目根目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        bitmap_dir = os.path.join(project_root, 'components', 'resource', 'bitmap')
        
        # 如果目录不存在，创建它
        if not os.path.exists(bitmap_dir):
            os.makedirs(bitmap_dir)
            return jsonify({
                'success': True,
                'bitmaps': [],
                'count': 0,
                'directory': bitmap_dir,
                'message': 'components/resource/bitmap 目录已创建，当前为空'
            })
        
        # 列出所有 .h 和 .bin 文件
        bitmap_files = []
        for filename in os.listdir(bitmap_dir):
            if filename.endswith('.h') or filename.endswith('.bin'):
                file_path = os.path.join(bitmap_dir, filename)
                file_size = os.path.getsize(file_path)
                
                # 尝试读取宽度和高度信息
                width, height = None, None
                file_type = 'header' if filename.endswith('.h') else 'binary'
                
                try:
                    if filename.endswith('.h'):
                        # 解析 .h 文件
                        with open(file_path, 'r', encoding='utf-8') as f:
                            content = f.read(500)  # 只读前500字符
                            import re
                            width_match = re.search(r'#define\s+\w*WIDTH\s+(\d+)', content)
                            height_match = re.search(r'#define\s+\w*HEIGHT\s+(\d+)', content)
                            if width_match:
                                width = int(width_match.group(1))
                            if height_match:
                                height = int(height_match.group(1))
                    elif filename.endswith('.bin'):
                        # 解析 .bin 文件头 (前8字节: 宽度4字节 + 高度4字节)
                        with open(file_path, 'rb') as f:
                            header = f.read(8)
                            if len(header) >= 8:
                                width = int.from_bytes(header[0:4], 'little')
                                height = int.from_bytes(header[4:8], 'little')
                except:
                    pass
                
                bitmap_files.append({
                    'name': filename,
                    'size': file_size,
                    'size_kb': round(file_size / 1024, 2),
                    'width': width,
                    'height': height,
                    'type': file_type,
                    'path': file_path
                })
        
        bitmap_files.sort(key=lambda x: x['name'])
        
        return jsonify({
            'success': True,
            'bitmaps': bitmap_files,
            'count': len(bitmap_files),
            'directory': bitmap_dir
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/get_font_file', methods=['GET'])
def get_font_file():
    """获取字体文件内容，用于上传到 SD 卡"""
    try:
        filename = request.args.get('filename')
        if not filename:
            return jsonify({'success': False, 'error': '缺少 filename 参数'}), 400
        
        # 获取项目根目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        file_path = os.path.join(project_root, 'components', 'resource', 'fonts', filename)
        
        if not os.path.exists(file_path):
            return jsonify({'success': False, 'error': f'文件不存在: {filename}'}), 404
        
        # 返回文件内容
        return send_file(file_path, as_attachment=True, download_name=filename)
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/get_bitmap_file', methods=['GET'])
def get_bitmap_file():
    """获取位图文件内容，用于上传到 SD 卡"""
    try:
        filename = request.args.get('filename')
        if not filename:
            return jsonify({'success': False, 'error': '缺少 filename 参数'}), 400
        
        # 获取项目根目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        file_path = os.path.join(project_root, 'components', 'resource', 'bitmap', filename)
        
        if not os.path.exists(file_path):
            return jsonify({'success': False, 'error': f'文件不存在: {filename}'}), 404
        
        # 返回文件内容
        return send_file(file_path, as_attachment=True, download_name=filename)
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/save_font_file', methods=['POST'])
def save_font_file():
    """保存 .bin 字体文件到 components/resource/fonts 目录"""
    try:
        if 'file' not in request.files:
            return jsonify({'success': False, 'error': '未上传文件'}), 400
        
        file = request.files['file']
        filename = request.form.get('filename', file.filename)
        
        if not filename:
            return jsonify({'success': False, 'error': '缺少文件名'}), 400
        
        # 获取保存目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        fonts_dir = os.path.join(project_root, 'components', 'resource', 'fonts')
        
        # 创建目录（如果不存在）
        os.makedirs(fonts_dir, exist_ok=True)
        
        file_path = os.path.join(fonts_dir, filename)
        
        # 保存文件
        file.save(file_path)
        
        # 获取文件大小
        file_size = os.path.getsize(file_path)
        
        return jsonify({
            'success': True,
            'filename': filename,
            'saved_path': file_path,
            'file_size': file_size,
            'message': f'字体文件已保存: {filename}'
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/save_bitmap_file', methods=['POST'])
def save_bitmap_file():
    """保存 .bin 位图文件到 components/resource/bitmap 目录"""
    try:
        if 'file' not in request.files:
            return jsonify({'success': False, 'error': '未上传文件'}), 400
        
        file = request.files['file']
        filename = request.form.get('filename', file.filename)
        
        if not filename:
            return jsonify({'success': False, 'error': '缺少文件名'}), 400
        
        # 获取保存目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        bitmap_dir = os.path.join(project_root, 'components', 'resource', 'bitmap')
        
        # 创建目录（如果不存在）
        os.makedirs(bitmap_dir, exist_ok=True)
        
        file_path = os.path.join(bitmap_dir, filename)
        
        # 保存文件
        file.save(file_path)
        
        # 获取文件大小
        file_size = os.path.getsize(file_path)
        
        return jsonify({
            'success': True,
            'filename': filename,
            'saved_path': file_path,
            'file_size': file_size,
            'message': f'位图文件已保存: {filename}'
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/get_bitmap_preview', methods=['POST'])
def get_bitmap_preview():
    """获取位图文件的预览数据"""
    try:
        data = request.json
        filename = data.get('filename')
        
        if not filename:
            return jsonify({'success': False, 'error': '未提供文件名'}), 400
        
        # 获取文件路径
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        bitmap_dir = os.path.join(project_root, 'components', 'resource', 'bitmap')
        file_path = os.path.join(bitmap_dir, filename)
        
        if not os.path.exists(file_path):
            return jsonify({'success': False, 'error': '文件不存在'}), 404
        
        # 读取文件内容
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 提取宽度、高度和位图数据
        import re
        width_match = re.search(r'#define\s+\w*WIDTH\s+(\d+)', content)
        height_match = re.search(r'#define\s+\w*HEIGHT\s+(\d+)', content)
        bitmap_match = re.search(r'const\s+uint8_t\s+\w+\[\]\s+PROGMEM\s*=\s*\{([^}]+)\}', content, re.DOTALL)
        
        if not all([width_match, height_match, bitmap_match]):
            return jsonify({
                'success': False,
                'error': '无法解析位图文件格式'
            }), 400
        
        width = int(width_match.group(1))
        height = int(height_match.group(1))
        
        # 提取十六进制数据
        bitmap_str = bitmap_match.group(1)
        hex_values = re.findall(r'0x([0-9A-Fa-f]{2})', bitmap_str)
        bitmap_data = [int(h, 16) for h in hex_values]
        
        return jsonify({
            'success': True,
            'width': width,
            'height': height,
            'data': bitmap_data,
            'type': 'header'
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/get_bin_preview', methods=['POST'])
def get_bin_preview():
    """获取 .bin 格式图片文件的预览数据"""
    try:
        data = request.json
        filename = data.get('filename')
        
        if not filename:
            return jsonify({'success': False, 'error': '未提供文件名'}), 400
        
        # 获取文件路径
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        bitmap_dir = os.path.join(project_root, 'components', 'resource', 'bitmap')
        file_path = os.path.join(bitmap_dir, filename)
        
        if not os.path.exists(file_path):
            return jsonify({'success': False, 'error': '文件不存在'}), 404
        
        # 读取 .bin 文件
        with open(file_path, 'rb') as f:
            # 读取文件头 (8字节: 宽度4字节 + 高度4字节)
            header = f.read(8)
            if len(header) < 8:
                return jsonify({
                    'success': False,
                    'error': '文件太小,不是有效的 .bin 图片文件'
                }), 400
            
            # 解析宽度和高度 (小端序)
            width = int.from_bytes(header[0:4], 'little')
            height = int.from_bytes(header[4:8], 'little')
            
            # 验证尺寸
            if width <= 0 or height <= 0 or width > 1000 or height > 1000:
                return jsonify({
                    'success': False,
                    'error': f'图片尺寸异常: {width}x{height}'
                }), 400
            
            # 读取位图数据
            bytes_per_row = (width + 7) // 8
            expected_size = bytes_per_row * height
            bitmap_bytes = f.read(expected_size)
            
            if len(bitmap_bytes) < expected_size:
                return jsonify({
                    'success': False,
                    'error': f'位图数据不完整: 期望 {expected_size} 字节, 实际 {len(bitmap_bytes)} 字节'
                }), 400
            
            # 转换为列表
            bitmap_data = list(bitmap_bytes)
        
        return jsonify({
            'success': True,
            'width': width,
            'height': height,
            'data': bitmap_data,
            'type': 'binary'
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/auto_import_font', methods=['POST'])
def auto_import_font():
    """
    自动导入字体：在 ink_screen.cpp 中添加 include 和显示代码
    
    请求格式:
    {
        "filename": "fangsong16pt_bitmaps.h",
        "display_text": "测试文字",
        "x": 10,
        "y": 50
    }
    """
    try:
        data = request.json
        filename = data.get('filename')
        display_text = data.get('display_text', '测试')
        x = data.get('x', 10)
        y = data.get('y', 50)
        
        if not filename:
            return jsonify({'success': False, 'error': '未提供文件名'}), 400
        
        # 获取 ink_screen.cpp 路径
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        ink_screen_path = os.path.join(project_root, 'components', 'grbl_esp32s3', 'Grbl_Esp32', 'src', 'BL_add', 'ink_screen', 'ink_screen.cpp')
        
        if not os.path.exists(ink_screen_path):
            return jsonify({'success': False, 'error': f'找不到 ink_screen.cpp: {ink_screen_path}'}), 404
        
        # 读取文件内容
        with open(ink_screen_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 1. 添加 include（如果不存在）
        include_line = f'#include "../resource/fonts/{filename}"'
        if include_line not in content:
            # 找到最后一个 #include "../resource/fonts/ 的位置
            import re
            last_font_include = None
            for match in re.finditer(r'#include\s+"\.\.\/resource\/fonts\/[^"]+\.h"', content):
                last_font_include = match
            
            if last_font_include:
                # 在最后一个字体 include 后面添加
                pos = last_font_include.end()
                content = content[:pos] + '\n' + include_line + content[pos:]
            else:
                # 找到任意 #include 的位置后添加
                first_include = re.search(r'#include\s+[<"][^>"]+[>"]', content)
                if first_include:
                    pos = first_include.end()
                    content = content[:pos] + '\n' + include_line + content[pos:]
                else:
                    return jsonify({'success': False, 'error': '无法找到合适的位置添加 include'}), 500
        
        # 2. 读取字体文件，查找实际定义的函数名或数组名
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        font_file_path = os.path.join(project_root, 'components', 'resource', 'fonts', filename)
        
        func_name = None
        array_names = []
        generic_draw_func = None  # 通用绘制函数（如 drawBitmap16x16）
        
        if os.path.exists(font_file_path):
            with open(font_file_path, 'r', encoding='utf-8') as f:
                font_content = f.read()
                
                # 查找模板函数定义（例如：inline void drawXXX(T& display, ...)）
                import re
                
                # 优先查找通用位图绘制函数（如 drawBitmap16x16）
                generic_func_match = re.search(r'inline\s+void\s+(drawBitmap\d+x\d+)\s*\([^)]*const\s+uint8_t\*\s+bitmap', font_content)
                if generic_func_match:
                    generic_draw_func = generic_func_match.group(1)
                
                # 查找特定的绘制函数（如 drawBaiLie）
                func_match = re.search(r'inline\s+void\s+(draw\w+)\s*\([^)]*display[^)]*\)', font_content)
                if func_match:
                    func_name = func_match.group(1)
                
                # 查找位图数组名（例如：const uint8_t BITMAP_XXX[] PROGMEM）
                array_matches = re.findall(r'const\s+uint8_t\s+(BITMAP_\w+)\[\]\s+PROGMEM', font_content)
                if array_matches:
                    array_names = array_matches
        
        # 3. 生成显示代码（优先使用通用函数）
        if generic_draw_func and array_names:
            # 使用通用位图函数 + 第一个数组
            # 例如: drawBitmap16x16(display, x, y, BITMAP_BAO);
            first_array = array_names[0]
            display_code = f'        {generic_draw_func}(display, {x}, {y}, {first_array});  // 自动导入: {filename}'
            if len(array_names) > 1:
                display_code += f'\n        // 可用数组: {", ".join(array_names[:5])}'
            note = f'使用通用函数: {generic_draw_func}() 显示 {first_array}'
        elif func_name:
            # 如果找到了专用函数，使用函数调用
            display_code = f'        {func_name}(display, {x}, {y});  // 自动导入: {filename}'
            note = f'使用函数: {func_name}()'
        elif array_names:
            # 如果只找到数组，生成注释说明
            display_code = f'        // TODO: 请手动添加显示代码，使用数组: {", ".join(array_names[:3])}'
            if len(array_names) > 3:
                display_code += f' (共{len(array_names)}个)'
            display_code += f'\n        // 示例: display.drawBitmap({x}, {y}, {array_names[0]}, width, height, GxEPD_BLACK);'
            note = f'找到{len(array_names)}个位图数组，需要手动编写显示代码'
        else:
            # 都没找到，生成通用注释
            display_code = f'        // TODO: 请添加 {filename} 的显示代码\n        // 坐标: ({x}, {y}), 文字: "{display_text}"'
            note = '未找到函数或数组定义，需要手动编写显示代码'
        
        # 4. 添加显示代码（在特定标记处添加）
        # 4. 添加显示代码（在特定标记处添加）
        # 查找是否有自动导入标记区域
        auto_import_marker = '// === 自动导入的字体显示 ==='
        
        if auto_import_marker in content:
            # 在标记后添加
            marker_pos = content.find(auto_import_marker)
            next_line = content.find('\n', marker_pos) + 1
            content = content[:next_line] + display_code + '\n' + content[next_line:]
        else:
            # 查找 display.print("OK!"); 前面添加
            ok_marker = 'display.print("OK!");'
            if ok_marker in content:
                marker_pos = content.find(ok_marker)
                # 找到这一行的开始
                line_start = content.rfind('\n', 0, marker_pos) + 1
                # 添加标记和代码
                insert_code = f'\n        {auto_import_marker}\n{display_code}\n'
                content = content[:line_start] + insert_code + content[line_start:]
            else:
                return jsonify({'success': False, 'error': '无法找到合适的位置添加显示代码'}), 500
        
        # 保存文件
        with open(ink_screen_path, 'w', encoding='utf-8') as f:
            f.write(content)
        
        return jsonify({
            'success': True,
            'message': '字体已自动导入到 ink_screen.cpp',
            'include_added': include_line,
            'display_code_added': display_code,
            'function_name': generic_draw_func if generic_draw_func else (func_name if func_name else ', '.join(array_names[:5])),
            'generic_function': generic_draw_func,
            'file_path': ink_screen_path,
            'note': note,
            'arrays_found': array_names[:10] if array_names else []
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/auto_import_bitmap', methods=['POST'])
def auto_import_bitmap():
    """
    自动导入位图：在 ink_screen.cpp 中添加 include 和显示代码
    
    请求格式:
    {
        "filename": "logo_bitmap.h",
        "x": 10,
        "y": 100
    }
    """
    try:
        data = request.json
        filename = data.get('filename')
        x = data.get('x', 10)
        y = data.get('y', 100)
        
        if not filename:
            return jsonify({'success': False, 'error': '未提供文件名'}), 400
        
        # 获取 ink_screen.cpp 路径
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        ink_screen_path = os.path.join(project_root, 'components', 'grbl_esp32s3', 'Grbl_Esp32', 'src', 'BL_add', 'ink_screen', 'ink_screen.cpp')
        
        if not os.path.exists(ink_screen_path):
            return jsonify({'success': False, 'error': f'找不到 ink_screen.cpp: {ink_screen_path}'}), 404
        
        # 读取文件内容
        with open(ink_screen_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 1. 添加 include（如果不存在）
        include_line = f'#include "../resource/bitmap/{filename}"'
        if include_line not in content:
            # 找到最后一个 #include "../resource/bitmap/ 的位置，如果没有则在 fonts include 后添加
            import re
            last_bitmap_include = None
            for match in re.finditer(r'#include\s+"\.\.\/resource\/bitmap\/[^"]+\.h"', content):
                last_bitmap_include = match
            
            if last_bitmap_include:
                pos = last_bitmap_include.end()
                content = content[:pos] + '\n' + include_line + content[pos:]
            else:
                # 在最后一个 fonts include 后添加
                last_font_include = None
                for match in re.finditer(r'#include\s+"\.\.\/resource\/fonts\/[^"]+\.h"', content):
                    last_font_include = match
                
                if last_font_include:
                    pos = last_font_include.end()
                    content = content[:pos] + '\n' + include_line + content[pos:]
                else:
                    # 找到任意 #include 后添加
                    first_include = re.search(r'#include\s+[<"][^>"]+[>"]', content)
                    if first_include:
                        pos = first_include.end()
                        content = content[:pos] + '\n' + include_line + content[pos:]
                    else:
                        return jsonify({'success': False, 'error': '无法找到合适的位置添加 include'}), 500
        
        # 2. 添加显示代码
        auto_import_marker = '// === 自动导入的位图显示 ==='
        display_code = f'        display.drawBitmap({x}, {y}, IMAGE_BITMAP, IMAGE_WIDTH, IMAGE_HEIGHT, GxEPD_BLACK);'
        
        if auto_import_marker in content:
            marker_pos = content.find(auto_import_marker)
            next_line = content.find('\n', marker_pos) + 1
            content = content[:next_line] + display_code + '\n' + content[next_line:]
        else:
            # 查找 display.print("OK!"); 前面添加
            ok_marker = 'display.print("OK!");'
            if ok_marker in content:
                marker_pos = content.find(ok_marker)
                line_start = content.rfind('\n', 0, marker_pos) + 1
                insert_code = f'\n        {auto_import_marker}\n{display_code}\n'
                content = content[:line_start] + insert_code + content[line_start:]
            else:
                return jsonify({'success': False, 'error': '无法找到合适的位置添加显示代码'}), 500
        
        # 保存文件
        with open(ink_screen_path, 'w', encoding='utf-8') as f:
            f.write(content)
        
        return jsonify({
            'success': True,
            'message': '位图已自动导入到 ink_screen.cpp',
            'include_added': include_line,
            'display_code_added': display_code,
            'file_path': ink_screen_path,
            'note': '需要重新编译并上传程序才能看到效果'
        })
    
    except Exception as e:
        import traceback
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 500


@app.route('/convert_ttf_to_bin', methods=['POST'])
def convert_ttf_to_bin():
    """
    将TTF字体转换为混合缓存系统的.bin格式
    
    bin文件格式:
    - 按Unicode顺序存储
    - 每个字符固定字节数: 16x16=32字节, 24x24=72字节, 32x32=128字节
    - 偏移计算: offset = (unicode - start_unicode) * glyph_size
    
    请求格式:
    {
        "ttf_base64": "base64编码的TTF文件内容",
        "font_size": 16,  // 16, 24 或 32
        "font_name": "comic_bold",
        "char_range": "english",  // "gb2312"(中文), "english"(英文ASCII), 或自定义范围
        "charset_start": 0x0020,   // 可选: 自定义范围开始
        "charset_end": 0x007E     // 可选: 自定义范围结束
    }
    """
    try:
        data = request.json
        
        # 解码 TTF 文件
        ttf_data = base64.b64decode(data['ttf_base64'])
        font_size = int(data.get('font_size', 16))
        font_name = data.get('font_name', 'custom_font')
        char_range = data.get('char_range', 'gb2312')
        
        # 创建字体对象
        font = ImageFont.truetype(io.BytesIO(ttf_data), font_size)
        
        # 根据 char_range 设置字符范围
        if char_range == 'english':
            # ASCII英文字符范围: 0x0020 - 0x007E (95个字符)
            start_unicode = 0x0020
            end_unicode = 0x007E
        elif char_range == 'gb2312':
            # GB2312 汉字范围: 0x4E00 - 0x9FA5 (约20902个字符)
            start_unicode = 0x4E00
            end_unicode = 0x9FA5
        else:
            # 自定义范围
            start_unicode = int(data.get('charset_start', 0x0020), 0)
            end_unicode = int(data.get('charset_end', 0x007E), 0)
        total_chars = end_unicode - start_unicode + 1
        
        # 计算每个字符的字节数
        bytes_per_row = (font_size + 7) // 8  # 每行字节数
        glyph_size = bytes_per_row * font_size  # 总字节数
        
        print(f"生成 {font_size}x{font_size} .bin 文件, 每字符 {glyph_size} 字节, 共 {total_chars} 字符")
        
        # 创建二进制缓冲区
        bin_data = bytearray(total_chars * glyph_size)
        
        # 生成每个汉字的字模
        generated_count = 0
        for unicode_val in range(start_unicode, end_unicode + 1):
            try:
                char = chr(unicode_val)
                
                # 创建图像 (font_size x font_size)
                img = Image.new('1', (font_size, font_size), 0)
                draw = ImageDraw.Draw(img)
                
                # 获取字符边界框
                bbox = font.getbbox(char)
                char_width = bbox[2] - bbox[0]
                char_height = bbox[3] - bbox[1]
                
                # 居中绘制
                x_offset = (font_size - char_width) // 2 - bbox[0]
                y_offset = (font_size - char_height) // 2 - bbox[1]
                draw.text((x_offset, y_offset), char, font=font, fill=1)
                
                # 转换为字节数组 (逐行扫描)
                offset = (unicode_val - start_unicode) * glyph_size
                byte_index = 0
                
                for y in range(font_size):
                    for x_byte in range(bytes_per_row):
                        byte_val = 0
                        for bit in range(8):
                            x = x_byte * 8 + bit
                            if x < font_size:
                                if img.getpixel((x, y)):
                                    byte_val |= (1 << (7 - bit))
                        
                        bin_data[offset + byte_index] = byte_val
                        byte_index += 1
                
                generated_count += 1
                
                # 每1000个字符打印进度
                if generated_count % 1000 == 0:
                    print(f"  已生成 {generated_count}/{total_chars} 字符...")
                    
            except Exception as e:
                # 字体不包含某个字符时,保持为0
                pass
        
        print(f"✅ 生成完成: {generated_count}/{total_chars} 字符")
        
        # 保存到本地 components/resource/fonts/ 目录
        output_filename = f"{font_name}_{font_size}x{font_size}.bin"
        output_dir = os.path.join(os.path.dirname(__file__), '..', 'components', 'resource', 'fonts')
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, output_filename)
        
        with open(output_path, 'wb') as f:
            f.write(bin_data)
        
        file_size_kb = len(bin_data) / 1024
        print(f"✅ .bin文件已保存: {output_path} ({file_size_kb:.1f} KB)")
        
        return jsonify({
            'success': True,
            'filename': output_filename,
            'file_size_kb': round(file_size_kb, 2),
            'char_count': total_chars,
            'generated_count': generated_count,
            'glyph_size': glyph_size,
            'font_size': font_size,
            'saved_path': output_path,
            'format': {
                'unicode_range': f'0x{start_unicode:04X} - 0x{end_unicode:04X}',
                'bytes_per_char': glyph_size,
                'total_size': len(bin_data),
                'usage': f'offset = (unicode - 0x{start_unicode:04X}) * {glyph_size}'
            }
        })
        
    except Exception as e:
        import traceback
        print(f"❌ 转换失败: {e}")
        print(traceback.format_exc())
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 400


@app.route('/download_bin_file', methods=['GET'])
def download_bin_file():
    """
    下载生成的.bin文件
    
    参数: filename=chinese_font_16x16.bin
    支持从 components/resource/fonts 和 components/resource/bitmap 文件夹下载
    """
    try:
        filename = request.args.get('filename')
        if not filename:
            return jsonify({'error': 'Missing filename parameter'}), 400
        
        # 先尝试从 components/resource/fonts 文件夹查找
        file_path = os.path.join(os.path.dirname(__file__), '..', 'components', 'resource', 'fonts', filename)
        
        # 如果不存在，尝试从 components/resource/bitmap 文件夹查找
        if not os.path.exists(file_path):
            file_path = os.path.join(os.path.dirname(__file__), '..', 'components', 'resource', 'bitmap', filename)
        
        if not os.path.exists(file_path):
            return jsonify({'error': f'File not found: {filename}'}), 404
        
        return send_file(
            file_path,
            as_attachment=True,
            download_name=filename,
            mimetype='application/octet-stream'
        )
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/convert_image_to_bin', methods=['POST'])
def convert_image_to_bin():
    """
    将图片转换为 .bin 格式（用于墨水屏显示）
    
    bin文件格式:
    - 单色位图 (1 bit per pixel)
    - 逐行扫描,每8个像素打包为1字节
    - 文件头: 4字节宽度 + 4字节高度 + 位图数据
    
    请求格式:
    {
        "image_base64": "base64编码的图片文件",
        "width": 240,
        "height": 416,
        "mode": "dithering",  // "threshold" 或 "dithering"
        "threshold": 128,
        "image_name": "logo"
    }
    """
    try:
        data = request.json
        
        # 解码图片
        image_data = base64.b64decode(data['image_base64'])
        img = Image.open(io.BytesIO(image_data))
        
        # 获取参数
        target_width = int(data.get('width', 240))
        target_height = int(data.get('height', 416))
        mode = data.get('mode', 'dithering')
        threshold = int(data.get('threshold', 128))
        image_name = data.get('image_name', 'image').replace(' ', '_')
        
        print(f"转换图片为 {target_width}x{target_height} .bin 文件...")
        
        # 调整大小
        img = img.resize((target_width, target_height), Image.Resampling.LANCZOS)
        
        # 转换为灰度
        img = img.convert('L')
        
        # 二值化处理
        if mode == 'dithering':
            # Floyd-Steinberg 抖动
            img = img.convert('1', dither=Image.Dither.FLOYDSTEINBERG)
        else:
            # 简单阈值
            img = img.point(lambda x: 255 if x > threshold else 0, mode='1')
        
        # 转换为字节数组
        bytes_per_row = (target_width + 7) // 8
        total_bytes = bytes_per_row * target_height
        
        # 文件头: 宽度(4字节) + 高度(4字节) + 位图数据
        bin_data = bytearray()
        bin_data.extend(target_width.to_bytes(4, 'little'))
        bin_data.extend(target_height.to_bytes(4, 'little'))
        
        # 逐行扫描转换
        for y in range(target_height):
            for x_byte in range(bytes_per_row):
                byte_val = 0
                for bit in range(8):
                    x = x_byte * 8 + bit
                    if x < target_width:
                        pixel = img.getpixel((x, y))
                        # 墨水屏: 0=白色, 1=黑色
                        # PIL 二值图: 0=黑色, 255=白色
                        # 所以: 如果像素是黑色(0), 则设置位为1
                        if not pixel:  # 黑色像素 -> 位设为1
                            byte_val |= (1 << (7 - bit))
                bin_data.append(byte_val)
        
        # 保存文件到 bitmap 文件夹
        output_filename = f"{image_name}_{target_width}x{target_height}.bin"
        output_dir = os.path.join(os.path.dirname(__file__), '..', 'components', 'resource', 'bitmap')
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, output_filename)
        
        with open(output_path, 'wb') as f:
            f.write(bin_data)
        
        file_size_kb = len(bin_data) / 1024
        print(f"✅ 图片 .bin 文件已保存: {output_path} ({file_size_kb:.2f} KB)")
        
        return jsonify({
            'success': True,
            'filename': output_filename,
            'file_size_kb': round(file_size_kb, 2),
            'width': target_width,
            'height': target_height,
            'saved_path': output_path,
            'format': {
                'header': '8 bytes (4 bytes width + 4 bytes height)',
                'bitmap_size': total_bytes,
                'total_size': len(bin_data),
                'usage': f'File file = SD.open("/{output_filename}"); display.drawBitmap(...)'
            }
        })
        
    except Exception as e:
        import traceback
        print(f"❌ 图片转换失败: {e}")
        print(traceback.format_exc())
        return jsonify({
            'success': False,
            'error': str(e),
            'traceback': traceback.format_exc()
        }), 400


if __name__ == '__main__':
    print("=" * 60)
    print("TTF 转 GFX Web 服务启动中...")
    print("=" * 60)
    print("访问: http://localhost:5000")
    print("API 端点:")
    print("  - GFX 转换: http://localhost:5000/convert_ttf_to_gfx")
    print("  - 位图保存: http://localhost:5000/save_bitmap_font")
    print("  - Python高质量位图: http://localhost:5000/convert_ttf_to_bitmap_python")
    print("  - .bin字库生成: http://localhost:5000/convert_ttf_to_bin")
    print("  - 图片转.bin: http://localhost:5000/convert_image_to_bin")
    print("  - 下载.bin文件: http://localhost:5000/download_bin_file?filename=xxx.bin")
    print("  - 列出字体库: http://localhost:5000/list_fonts")
    print("  - 列出图片库: http://localhost:5000/list_bitmaps")
    print("  - 获取位图预览: http://localhost:5000/get_bitmap_preview")
    print("  - 自动导入字体: http://localhost:5000/auto_import_font")
    print("  - 自动导入位图: http://localhost:5000/auto_import_bitmap")
    print("  - 保存字体文件: http://localhost:5000/save_font_file (POST)")
    print("  - 保存位图文件: http://localhost:5000/save_bitmap_file (POST)")
    print()
    print("在 web_layout.html 中可以通过 fetch 调用此API")
    print("=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)

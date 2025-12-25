#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
墨水屏图片转换工具包
将图片批量转换为 ESP32 墨水屏格式的 .bin 文件
支持批量转换、自定义尺寸、统一处理模式
"""

import os
import sys
import argparse
import gzip
import shutil
from pathlib import Path
from PIL import Image
import io
from datetime import datetime

class ImageConverter:
    """图片转换器"""
    
    def __init__(self, width=416, height=240):
        """
        初始化转换器
        
        参数:
            width: 目标宽度（像素）
            height: 目标高度（像素）
        """
        self.width = width
        self.height = height
        self.mode = 'dithering'  # Floyd-Steinberg 抖动算法
        self.converted_count = 0
        self.failed_count = 0
        
    def convert_image_to_bin(self, image_path, output_path=None):
        """
        将单个图片转换为 .bin 格式
        
        参数:
            image_path: 输入图片路径
            output_path: 输出 .bin 文件路径（可选）
            
        返回:
            (成功标志, 输出文件路径, 文件大小)
        """
        try:
            image_path = Path(image_path)
            
            if not image_path.exists():
                print(f"❌ 错误: 找不到图片文件: {image_path}")
                self.failed_count += 1
                return False, None, 0
            
            # 打开并处理图片
            img = Image.open(image_path)
            original_size = img.size
            
            print(f"📷 处理图片: {image_path.name}")
            print(f"   原始尺寸: {original_size[0]}x{original_size[1]}")
            
            # 调整大小
            img = img.resize((self.width, self.height), Image.Resampling.LANCZOS)
            
            # 转换为灰度
            img = img.convert('L')
            
            # Floyd-Steinberg 抖动算法
            img = img.convert('1', dither=Image.Dither.FLOYDSTEINBERG)
            
            # 转换为字节数组
            bytes_per_row = (self.width + 7) // 8
            total_bytes = bytes_per_row * self.height
            
            # 文件头: 宽度(4字节) + 高度(4字节) + 位图数据
            bin_data = bytearray()
            bin_data.extend(self.width.to_bytes(4, 'little'))
            bin_data.extend(self.height.to_bytes(4, 'little'))
            
            # 逐行扫描转换
            for y in range(self.height):
                for x_byte in range(bytes_per_row):
                    byte_val = 0
                    for bit in range(8):
                        x = x_byte * 8 + bit
                        if x < self.width:
                            pixel = img.getpixel((x, y))
                            # 墨水屏: 0=白色, 1=黑色
                            # PIL 二值图: 0=黑色, 255=白色
                            if not pixel:  # 黑色像素 -> 位设为1
                                byte_val |= (1 << (7 - bit))
                    bin_data.append(byte_val)
            
            # 确定输出路径
            if output_path is None:
                output_path = image_path.with_suffix('.bin')
            else:
                output_path = Path(output_path)
            
            # 创建输出目录（如果需要）
            output_path.parent.mkdir(parents=True, exist_ok=True)
            
            # 保存 .bin 文件
            with open(output_path, 'wb') as f:
                f.write(bin_data)
            
            file_size_kb = len(bin_data) / 1024
            
            print(f"   ✅ 转换成功")
            print(f"   目标尺寸: {self.width}x{self.height}")
            print(f"   文件大小: {file_size_kb:.2f} KB")
            print(f"   输出文件: {output_path}")
            
            self.converted_count += 1
            return True, str(output_path), len(bin_data)
            
        except Exception as e:
            print(f"❌ 转换失败: {e}")
            self.failed_count += 1
            import traceback
            traceback.print_exc()
            return False, None, 0
    
    def convert_directory(self, input_dir, output_dir=None, pattern="*.jpg", backup=True, delete_originals=True):
        """
        批量转换目录中的图片
        
        参数:
            input_dir: 输入图片目录
            output_dir: 输出 .bin 文件目录
            pattern: 文件匹配模式
            backup: 转换完成后是否备份原始图片文件夹
            delete_originals: 转换完成后是否删除原始图片
            
        返回:
            (成功数, 失败数)
        """
        input_dir = Path(input_dir)
        
        if not input_dir.exists():
            print(f"❌ 错误: 找不到输入目录: {input_dir}")
            return 0, 0
        
        if output_dir is None:
            output_dir = input_dir
        else:
            output_dir = Path(output_dir)
            # 创建输出目录（如果不存在）
            output_dir.mkdir(parents=True, exist_ok=True)
        
        print(f"\n📁 批量转换: {input_dir}")
        print(f"   输出目录: {output_dir}")
        print(f"   目标尺寸: {self.width}x{self.height}")
        print(f"   处理模式: Floyd-Steinberg 抖动算法")
        print("=" * 60)
        
        # 支持的图片格式（使用大小写不敏感的方式）
        image_extensions = {'.jpg', '.jpeg', '.png', '.bmp', '.gif', '.webp'}
        
        image_files = []
        for item in input_dir.iterdir():
            if item.is_file() and item.suffix.lower() in image_extensions:
                image_files.append(item)
        
        if not image_files:
            print(f"⚠️  找不到图片文件")
            return 0, 0
        
        print(f"找到 {len(image_files)} 个图片文件\n")
        
        # 开始转换
        for image_file in sorted(image_files):
            output_file = output_dir / image_file.with_suffix('.bin').name
            self.convert_image_to_bin(str(image_file), str(output_file))
            print()
        
        # 转换完成后的后处理
        if self.failed_count == 0 and len(image_files) > 0:
            print("\n" + "=" * 60)
            print("🔄 转换完成，执行后处理...")
            print("=" * 60)
            
            # 创建备份
            if backup:
                self._backup_directory(input_dir)
            
            # 删除原始图片
            if delete_originals:
                self._delete_image_files(input_dir, image_files)
        
        return self.converted_count, self.failed_count
    
    def _backup_directory(self, source_dir):
        """创建目录备份"""
        try:
            source_dir = Path(source_dir)
            parent_dir = source_dir.parent
            
            # 生成备份目录名（带时间戳防止重复）
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            backup_name = f"{source_dir.name}_backup_{timestamp}"
            backup_path = parent_dir / backup_name
            
            print(f"\n💾 创建备份...")
            print(f"   源目录: {source_dir}")
            print(f"   备份路径: {backup_path}")
            
            # 复制整个目录
            shutil.copytree(source_dir, backup_path)
            
            print(f"   ✅ 备份创建成功!")
            return True
            
        except Exception as e:
            print(f"   ❌ 备份失败: {e}")
            return False
    
    def _delete_image_files(self, source_dir, image_files):
        """删除原始图片文件"""
        try:
            print(f"\n🗑️ 删除原始图片...")
            print(f"   目录: {source_dir}")
            
            deleted_count = 0
            for image_file in image_files:
                try:
                    image_file.unlink()  # 删除文件
                    deleted_count += 1
                except Exception as e:
                    print(f"   ⚠️  无法删除 {image_file.name}: {e}")
            
            print(f"   ✅ 已删除 {deleted_count} 个图片文件")
            return True
            
        except Exception as e:
            print(f"   ❌ 删除失败: {e}")
            return False
    
    def print_summary(self):
        """打印转换统计"""
        print("=" * 60)
        print("📊 转换统计:")
        print(f"   ✅ 成功: {self.converted_count}")
        print(f"   ❌ 失败: {self.failed_count}")
        print(f"   总计: {self.converted_count + self.failed_count}")
        print("=" * 60)

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='墨水屏图片转换工具包 - 批量转换图片为 .bin 格式',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例:
  # 转换单个图片
  python image_converter_tool.py --input image.jpg --width 416 --height 240
  
  # 批量转换目录中的图片
  python image_converter_tool.py --input ./pictures --output ./bin_files --width 416 --height 240
  
  # 转换并保存到指定目录
  python image_converter_tool.py -i ./source --o ./output -w 416 -h 240

支持的图片格式: JPG, JPEG, PNG, BMP, GIF, WebP
处理模式: Floyd-Steinberg 抖动算法（统一）
        '''
    )
    
    parser.add_argument('-i', '--input', required=True,
                        help='输入图片文件或目录路径')
    parser.add_argument('-o', '--output', default=None,
                        help='输出 .bin 文件目录（默认与输入同目录）')
    parser.add_argument('-w', '--width', type=int, default=416,
                        help='目标图片宽度，单位像素（默认: 416）')
    parser.add_argument('-hh', '--height', type=int, default=240,
                        help='目标图片高度，单位像素（默认: 240）')
    parser.add_argument('-b', '--backup', action='store_true', default=True,
                        help='转换完成后创建原始图片文件夹的备份（默认: 启用）')
    parser.add_argument('--no-backup', dest='backup', action='store_false',
                        help='转换完成后不创建备份')
    parser.add_argument('-d', '--delete', action='store_true', default=True,
                        help='转换完成后删除原始图片文件（默认: 启用）')
    parser.add_argument('--no-delete', dest='delete_originals', action='store_false',
                        help='转换完成后不删除原始图片')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='显示详细信息')
    
    args = parser.parse_args()
    
    # 创建转换器
    converter = ImageConverter(width=args.width, height=args.height)
    
    input_path = Path(args.input)
    
    print("=" * 60)
    print("🖼️  墨水屏图片转换工具包")
    print("=" * 60)
    print(f"输入: {input_path}")
    print(f"宽度: {args.width} px")
    print(f"高度: {args.height} px")
    print(f"模式: Floyd-Steinberg 抖动算法")
    print()
    
    # 判断是文件还是目录
    if input_path.is_file():
        # 单个文件
        print("📄 转换单个文件...\n")
        output_file = args.output if args.output else None
        success, output_path, file_size = converter.convert_image_to_bin(str(input_path), output_file)
        
        if success:
            print(f"\n✅ 转换成功!")
            print(f"输出文件: {output_path}")
        else:
            print(f"\n❌ 转换失败!")
            sys.exit(1)
    
    elif input_path.is_dir():
        # 目录
        print("📁 批量转换目录...\n")
        success_count, failed_count = converter.convert_directory(
            str(input_path), 
            args.output,
            backup=args.backup,
            delete_originals=args.delete_originals
        )
        converter.print_summary()
        
        if failed_count > 0:
            sys.exit(1)
    
    else:
        print(f"❌ 错误: {input_path} 既不是文件也不是目录")
        sys.exit(1)
    
    print("\n✨ 完成!")

if __name__ == '__main__':
    main()

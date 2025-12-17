#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
web_layout_standalone.html.gz 生成和替换工具
用于将新的独立HTML文件压缩并替换旧的web_layout.html.gz
"""

import gzip
import os
import shutil
from pathlib import Path

# 文件路径配置
BASE_DIR = Path(__file__).parent
STANDALONE_HTML = BASE_DIR / 'web_layout_standalone.html'
DATA_DIR = BASE_DIR / 'components/grbl_esp32s3/Grbl_Esp32/data'
OLD_GZ = DATA_DIR / 'web_layout.html.gz'
NEW_GZ = DATA_DIR / 'web_layout_standalone.html.gz'
BACKUP_GZ = OLD_GZ.with_stem(OLD_GZ.stem + '_backup')

def check_files():
    """检查必要的文件是否存在"""
    print("🔍 检查文件...")
    
    if not STANDALONE_HTML.exists():
        print(f"❌ 错误：找不到 {STANDALONE_HTML}")
        return False
    print(f"✅ 找到源文件: {STANDALONE_HTML.name} ({STANDALONE_HTML.stat().st_size / 1024:.1f} KB)")
    
    if not DATA_DIR.exists():
        print(f"❌ 错误：找不到数据目录 {DATA_DIR}")
        return False
    print(f"✅ 数据目录: {DATA_DIR}")
    
    if OLD_GZ.exists():
        print(f"✅ 找到旧文件: {OLD_GZ.name} ({OLD_GZ.stat().st_size / 1024:.1f} KB)")
    else:
        print(f"⚠️  旧文件不存在: {OLD_GZ.name}")
    
    return True

def compress_html():
    """压缩HTML文件"""
    print("\n🔨 压缩HTML文件...")
    
    try:
        # 读取源文件
        with open(STANDALONE_HTML, 'rb') as f:
            data = f.read()
        
        original_size = len(data)
        
        # 压缩
        with open(NEW_GZ, 'wb') as f:
            f.write(gzip.compress(data, 9))
        
        compressed_size = NEW_GZ.stat().st_size
        ratio = 100 * (1 - compressed_size / original_size)
        
        print(f"✅ 压缩完成:")
        print(f"   原始大小: {original_size:,} 字节 ({original_size / 1024:.1f} KB)")
        print(f"   压缩大小: {compressed_size:,} 字节 ({compressed_size / 1024:.1f} KB)")
        print(f"   压缩比率: {ratio:.1f}%")
        print(f"   输出文件: {NEW_GZ.name}")
        
        return True
    
    except Exception as e:
        print(f"❌ 压缩失败: {e}")
        return False

def backup_old_file():
    """备份旧文件"""
    if OLD_GZ.exists():
        print("\n💾 备份旧文件...")
        try:
            shutil.copy2(OLD_GZ, BACKUP_GZ)
            print(f"✅ 备份完成: {BACKUP_GZ.name}")
            return True
        except Exception as e:
            print(f"❌ 备份失败: {e}")
            return False
    return True

def replace_file():
    """替换旧文件"""
    print("\n🔄 替换文件...")
    
    try:
        # 删除旧文件
        if OLD_GZ.exists():
            OLD_GZ.unlink()
            print(f"✅ 删除旧文件: {OLD_GZ.name}")
        
        # 重命名新文件
        NEW_GZ.rename(OLD_GZ)
        print(f"✅ 重命名文件: {NEW_GZ.name} → {OLD_GZ.name}")
        
        return True
    
    except Exception as e:
        print(f"❌ 替换失败: {e}")
        return False

def update_cmakelists():
    """提示更新CMakeLists.txt（可选）"""
    print("\n📝 CMakeLists.txt 更新建议（可选）")
    print("-" * 60)
    print("如果要使用web_layout_standalone.html.gz的新名称，")
    print("需要更新 CMakeLists.txt 中的EMBED_FILES:")
    print()
    print("旧配置:")
    print('  EMBED_FILES "Grbl_Esp32/data/index.html.gz"')
    print('             "Grbl_Esp32/data/favicon.ico"')
    print('             "Grbl_Esp32/data/web_layout.html.gz"')
    print()
    print("新配置:")
    print('  EMBED_FILES "Grbl_Esp32/data/index.html.gz"')
    print('             "Grbl_Esp32/data/favicon.ico"')
    print('             "Grbl_Esp32/data/web_layout_standalone.html.gz"')
    print()
    print("然后在 WebServer.cpp 中同时更新：")
    print('  extern const char web_layout_start[] asm("_binary_web_layout_standalone_html_gz_start");')
    print('  extern const char web_layout_end[]   asm("_binary_web_layout_standalone_html_gz_end");')
    print("-" * 60)

def verify_compression():
    """验证压缩文件"""
    print("\n✓ 验证压缩文件...")
    
    try:
        with gzip.open(OLD_GZ, 'rb') as f:
            data = f.read()
        
        print(f"✅ 文件有效: {len(data):,} 字节解压后")
        return True
    
    except Exception as e:
        print(f"❌ 验证失败: {e}")
        return False

def main():
    """主函数"""
    print("=" * 60)
    print("  web_layout_standalone.html.gz 生成工具")
    print("=" * 60)
    print()
    
    # 检查文件
    if not check_files():
        return False
    
    # 压缩
    if not compress_html():
        return False
    
    # 备份
    if not backup_old_file():
        return False
    
    # 替换
    if not replace_file():
        return False
    
    # 验证
    if not verify_compression():
        print("⚠️  警告：文件可能损坏，请检查")
        return False
    
    print("\n" + "=" * 60)
    print("✅ 完成！")
    print("=" * 60)
    print()
    print("📋 下一步建议:")
    print("  1. 如果要使用新文件名，需要更新:")
    print("     - components/grbl_esp32s3/CMakeLists.txt")
    print("     - components/grbl_esp32s3/Grbl_Esp32/src/WebUI/WebServer.cpp")
    print()
    print("  2. 或者直接编译使用新的 web_layout.html.gz")
    print()
    print("🔍 文件信息:")
    print(f"  压缩文件: {OLD_GZ}")
    print(f"  文件大小: {OLD_GZ.stat().st_size / 1024:.1f} KB")
    print(f"  备份文件: {BACKUP_GZ.name}")
    print()
    
    return True

if __name__ == '__main__':
    import sys
    success = main()
    sys.exit(0 if success else 1)

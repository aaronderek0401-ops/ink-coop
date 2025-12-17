#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试 Web 服务是否正常工作
"""

import requests
import base64
import os

# 配置
FONT_PATH = "components/fonts/仿宋_GB2312.ttf"
TEST_CHARS = "测试"
FONT_SIZE = 16
FONT_NAME = "test_chinese"

def test_conversion():
    """测试字体转换功能"""
    
    print("=" * 60)
    print("🧪 测试 Web 服务字体转换功能")
    print("=" * 60)
    
    # 1. 检查字体文件是否存在
    if not os.path.exists(FONT_PATH):
        print(f"❌ 字体文件不存在: {FONT_PATH}")
        return
    
    print(f"✅ 字体文件存在: {FONT_PATH}")
    
    # 2. 读取字体文件并编码为 base64
    with open(FONT_PATH, 'rb') as f:
        ttf_data = f.read()
    
    ttf_base64 = base64.b64encode(ttf_data).decode('utf-8')
    print(f"✅ 字体文件已读取: {len(ttf_data)} bytes")
    
    # 3. 准备请求数据
    request_data = {
        'ttf_base64': ttf_base64,
        'chars': TEST_CHARS,
        'font_size': FONT_SIZE,
        'font_name': FONT_NAME
    }
    
    print(f"📝 转换字符: {TEST_CHARS}")
    print(f"📏 字体大小: {FONT_SIZE}")
    print()
    
    # 4. 发送请求到 Web 服务
    try:
        print("🔄 发送请求到 http://localhost:5000/convert_ttf_to_gfx ...")
        
        response = requests.post(
            'http://localhost:5000/convert_ttf_to_gfx',
            json=request_data,
            timeout=10
        )
        
        if response.status_code == 200:
            result = response.json()
            
            if result.get('success'):
                print("✅ 转换成功！")
                print()
                print(f"📊 字符数: {result.get('char_count')}")
                print(f"📊 位图大小: {result.get('file_size')} bytes")
                print(f"📁 文件名: {result.get('filename')}")
                print(f"💾 保存路径: {result.get('saved_path')}")
                print()
                
                # 验证文件是否真的保存了
                saved_path = result.get('saved_path')
                if saved_path and os.path.exists(saved_path):
                    file_size = os.path.getsize(saved_path)
                    print(f"✅ 文件已保存到本地: {saved_path}")
                    print(f"📊 文件大小: {file_size} bytes")
                    
                    # 读取文件前几行
                    with open(saved_path, 'r', encoding='utf-8') as f:
                        lines = f.readlines()[:10]
                    
                    print()
                    print("📄 文件内容预览:")
                    print("-" * 60)
                    for line in lines:
                        print(line.rstrip())
                    print("-" * 60)
                else:
                    print(f"⚠️  文件路径返回了，但文件不存在: {saved_path}")
                
                print()
                print("=" * 60)
                print("🎉 测试通过！Web 服务工作正常！")
                print("=" * 60)
                
            else:
                print(f"❌ 转换失败: {result.get('error')}")
        else:
            print(f"❌ HTTP 错误: {response.status_code}")
            print(f"响应: {response.text}")
    
    except requests.exceptions.ConnectionError:
        print("❌ 无法连接到 Web 服务")
        print()
        print("请先启动 Python 服务:")
        print("  python tools/ttf_to_gfx_webservice.py")
    
    except Exception as e:
        print(f"❌ 测试失败: {e}")

if __name__ == "__main__":
    test_conversion()

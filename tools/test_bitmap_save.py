#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试位图字体保存功能
"""

import requests
import json

def test_save_bitmap():
    """测试保存位图字体到本地"""
    
    # 模拟位图文件内容
    test_content = """// 测试位图字体文件
// 生成时间: 2025-12-12
#ifndef _TEST_BITMAP_H_
#define _TEST_BITMAP_H_

#include <Arduino.h>

// '测' (U+6D4B)
const uint8_t PROGMEM BITMAP_24PT_6D4B[] = {
    0x00, 0x00, 0x00,
    0x3F, 0xFF, 0xFC,
    0x20, 0x00, 0x04
};

#endif
"""
    
    test_filename = "test_bitmap_24pt.h"
    
    print("=" * 60)
    print("测试位图字体保存功能")
    print("=" * 60)
    
    try:
        # 1. 测试服务是否运行
        print("\n1. 检查服务状态...")
        health_response = requests.get('http://localhost:5000/')
        print(f"   ✅ 服务正常: {health_response.json()['status']}")
        
        # 2. 发送保存请求
        print(f"\n2. 保存文件: {test_filename}")
        save_response = requests.post(
            'http://localhost:5000/save_bitmap_font',
            json={
                'content': test_content,
                'filename': test_filename
            }
        )
        
        result = save_response.json()
        
        if result['success']:
            print(f"   ✅ 保存成功!")
            print(f"   路径: {result['saved_path']}")
            print(f"   文件名: {result['filename']}")
        else:
            print(f"   ❌ 保存失败: {result.get('error', '未知错误')}")
        
        print("\n" + "=" * 60)
        print("测试完成")
        print("=" * 60)
        
    except requests.exceptions.ConnectionError:
        print("\n❌ 错误: 无法连接到服务")
        print("请确保 Python 服务正在运行:")
        print("  python tools/ttf_to_gfx_webservice.py")
        
    except Exception as e:
        print(f"\n❌ 错误: {e}")


if __name__ == '__main__':
    test_save_bitmap()

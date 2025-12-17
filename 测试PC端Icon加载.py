#!/usr/bin/env python3
"""
PC 端 Icon 加载测试脚本
测试 Flask 服务器是否能正确提供所有 icon 二进制文件
"""

import requests
import os
from pathlib import Path

# Icon 文件列表
ICON_FILES = {
    0: '0_icon1_62x64.bin',
    1: '1_icon2_64x64.bin',
    2: '2_icon3_86x64.bin',
    3: '3_icon4_71x56.bin',
    4: '4_icon5_76x56.bin',
    5: '5_icon6_94x64.bin',
    6: '6_separate_120x8.bin',
    7: '7_wifi_connect_32x32.bin',
    8: '8_wifi_disconnect_32x32.bin',
    9: '9_battery_1_36x24.bin',
    10: '10_horn_16x16.bin',
    11: '11_nail_15x16.bin',
    12: '12_lock_32x32.bin'
}

def test_icon_loading():
    """测试所有 icon 是否能从 Flask 服务器加载"""
    
    print("=" * 70)
    print("🧪 PC 端 Icon 加载测试")
    print("=" * 70)
    print()
    
    # 检查 Flask 服务器是否运行
    try:
        response = requests.get('http://localhost:5001/api/health', timeout=2)
        if response.status_code == 200:
            print("✓ Flask 服务器在线 (localhost:5001)")
            print()
        else:
            print("✗ Flask 服务器响应异常")
            return False
    except Exception as e:
        print(f"✗ 无法连接到 Flask 服务器: {e}")
        print(f"  请先运行: python config_server.py")
        return False
    
    # 测试每个 icon
    success_count = 0
    total_size = 0
    
    print("测试 icon 加载状态:")
    print()
    
    for icon_index, icon_filename in ICON_FILES.items():
        try:
            url = f'http://localhost:5001/api/icon/binary/{icon_index}'
            response = requests.get(url, timeout=5)
            
            if response.status_code == 200:
                data_size = len(response.content)
                total_size += data_size
                success_count += 1
                
                # 获取本地文件大小用于对比
                icon_path = Path('components/resource/icon') / icon_filename
                if icon_path.exists():
                    local_size = icon_path.stat().st_size
                    match = "✓" if data_size == local_size else "⚠"
                    print(f"  {match} Icon {icon_index:2d} ({icon_filename:30s}): {data_size:5d} bytes")
                else:
                    print(f"  ✓ Icon {icon_index:2d} ({icon_filename:30s}): {data_size:5d} bytes (本地文件未找到)")
            else:
                print(f"  ✗ Icon {icon_index:2d} ({icon_filename:30s}): HTTP {response.status_code}")
        
        except requests.exceptions.Timeout:
            print(f"  ✗ Icon {icon_index:2d} ({icon_filename:30s}): 超时")
        except Exception as e:
            print(f"  ✗ Icon {icon_index:2d} ({icon_filename:30s}): {str(e)}")
    
    print()
    print("=" * 70)
    print(f"📊 测试结果: {success_count}/{len(ICON_FILES)} 成功")
    print(f"📦 总数据大小: {total_size} 字节 ({total_size/1024:.2f} KB)")
    print("=" * 70)
    
    if success_count == len(ICON_FILES):
        print("✅ 所有 icon 加载成功！")
        print()
        print("现在可以：")
        print("  1. 打开浏览器访问: file:///G:/A_BL_Project/inkScree_fuben/web_layout_standalone.html")
        print("  2. 应该看到绿色状态条（PC 模式）")
        print("  3. 所有 icon 图标应该正常显示")
        return True
    else:
        print(f"⚠️  有 {len(ICON_FILES) - success_count} 个 icon 加载失败")
        return False

if __name__ == '__main__':
    test_icon_loading()

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTF 转 Adafruit GFX 字体工具
使用 fontforge 库转换 TTF 字体为 .h 文件
"""

import sys
import os

# 需要的字符列表
CHINESE_CHARS = """测试中文字体显示正常温度湿度电量充电WiFi连接成功失败已断开IP地址年月日时分秒星期一二三四五六日开关0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz°C%:/-_.()\n"""

def check_dependencies():
    """检查依赖"""
    try:
        import fontforge
        print("✅ fontforge 已安装")
        return True
    except ImportError:
        print("❌ fontforge 未安装")
        print("\n安装方法：")
        print("1. 下载 FontForge: https://fontforge.org/en-US/downloads/")
        print("2. 安装后，pip install fontforge")
        return False

def convert_ttf_to_gfx(ttf_path, output_path, font_size, char_list):
    """
    转换 TTF 字体为 GFX 格式
    
    Args:
        ttf_path: TTF 字体文件路径
        output_path: 输出 .h 文件路径
        font_size: 字体大小（点）
        char_list: 要包含的字符列表
    """
    import fontforge
    
    print(f"正在加载字体: {ttf_path}")
    font = fontforge.open(ttf_path)
    
    print(f"字体名称: {font.fontname}")
    print(f"字体大小: {font_size}pt")
    print(f"字符数量: {len(set(char_list))}")
    
    # 这里需要使用 fontforge 的导出功能
    # 注意：fontforge 不直接支持 GFX 格式
    # 需要使用其他工具如 ttf2gfx
    
    print("\n⚠️ 此脚本需要配合 ttf2gfx 工具使用")
    print("推荐使用在线工具或下载 Adafruit-GFX-Library 的转换工具")

def main():
    """主函数"""
    print("=" * 60)
    print("TTF 转 Adafruit GFX 字体工具")
    print("=" * 60)
    
    if not check_dependencies():
        print("\n建议：使用在线工具 https://rop.nl/truetype2gfx/")
        print("或使用 Adafruit 官方工具")
        return
    
    # 示例用法
    ttf_path = r"C:\Windows\Fonts\simhei.ttf"
    output_path = "simhei16pt7b.h"
    font_size = 16
    
    print(f"\n字符列表预览：")
    print(CHINESE_CHARS[:50] + "...")
    print(f"\n总字符数: {len(set(CHINESE_CHARS))}")

if __name__ == "__main__":
    main()

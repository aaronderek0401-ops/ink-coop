#!/usr/bin/env python3
"""
验证 HTML 文件中的 ICON_METADATA 修改
"""

import re
import json

def check_icon_metadata():
    """检查 ICON_METADATA 定义"""
    
    print("=" * 70)
    print("🔍 检查 ICON_METADATA 修改")
    print("=" * 70)
    print()
    
    with open('web_layout_standalone.html', 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 提取 ICON_METADATA 定义
    pattern = r'const ICON_METADATA = \[(.*?)\];'
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        print("❌ 未找到 ICON_METADATA 定义")
        return False
    
    metadata_str = match.group(0)
    print("找到 ICON_METADATA 定义：")
    print()
    
    # 提取每个 icon 的定义
    icon_pattern = r'{index: (\d+),.*?width: (\d+),.*?height: (\d+).*?}'
    icons = re.findall(icon_pattern, metadata_str)
    
    # 预期的尺寸（根据文件名）
    expected_sizes = {
        0: (62, 64),
        1: (64, 64),
        2: (86, 64),
        3: (71, 56),
        4: (76, 56),
        5: (94, 64),
        6: (120, 8),
        7: (32, 32),
        8: (32, 32),
        9: (36, 24),
        10: (16, 16),
        11: (15, 16),
        12: (32, 32),
    }
    
    all_correct = True
    print("Icon 元数据检查结果：")
    print()
    print("索引 | 宽度 | 高度 | 文件名 | 状态")
    print("-" * 70)
    
    for idx, width, height in icons:
        idx = int(idx)
        width = int(width)
        height = int(height)
        
        if idx in expected_sizes:
            expected_w, expected_h = expected_sizes[idx]
            if width == expected_w and height == expected_h:
                status = "✅ 正确"
            else:
                status = f"❌ 错误（期望 {expected_w}x{expected_h}）"
                all_correct = False
            
            # 构造文件名
            if idx <= 5:
                filename = f"{idx}_icon{idx+1}_{width}x{height}.bin"
            elif idx == 6:
                filename = f"{idx}_separate_{width}x{height}.bin"
            elif idx == 7:
                filename = f"{idx}_wifi_connect_{width}x{height}.bin"
            elif idx == 8:
                filename = f"{idx}_wifi_disconnect_{width}x{height}.bin"
            elif idx == 9:
                filename = f"{idx}_battery_1_{width}x{height}.bin"
            elif idx == 10:
                filename = f"{idx}_horn_{width}x{height}.bin"
            elif idx == 11:
                filename = f"{idx}_nail_{width}x{height}.bin"
            elif idx == 12:
                filename = f"{idx}_lock_{width}x{height}.bin"
            else:
                filename = "unknown"
            
            print(f"{idx:2d}   | {width:4d} | {height:4d} | {filename:35s} | {status}")
    
    print()
    print("=" * 70)
    
    if all_correct:
        print("✅ 所有 ICON_METADATA 都正确！")
    else:
        print("❌ 存在不匹配的 ICON_METADATA")
    
    return all_correct


def check_drawing_function():
    """检查是否使用了简单的绘制函数"""
    
    print()
    print("=" * 70)
    print("🔍 检查绘制函数")
    print("=" * 70)
    print()
    
    with open('web_layout_standalone.html', 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 检查是否存在 drawBitmapToCanvasSimple 函数
    if 'function drawBitmapToCanvasSimple(' in content:
        print("✅ 找到 drawBitmapToCanvasSimple() 函数")
    else:
        print("❌ 未找到 drawBitmapToCanvasSimple() 函数")
        return False
    
    # 检查是否在图标绘制中调用了这个函数
    if 'drawBitmapToCanvasSimple(ctx, bitmapData,' in content:
        print("✅ 在图标绘制中使用了 drawBitmapToCanvasSimple()")
    else:
        print("❌ 未在图标绘制中调用 drawBitmapToCanvasSimple()")
        return False
    
    # 检查是否移除了旋转和镜像代码
    if 'drawBitmapToCanvasWithTransform' in content:
        print("⚠️  仍然存在 drawBitmapToCanvasWithTransform() 函数（可能是注释或备份）")
    else:
        print("✅ 已移除旋转和镜像的复杂逻辑")
    
    return True


def check_canvas_size():
    """检查 Canvas 大小是否使用实际尺寸"""
    
    print()
    print("=" * 70)
    print("🔍 检查 Canvas 尺寸获取")
    print("=" * 70)
    print()
    
    with open('web_layout_standalone.html', 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 检查是否从 ICON_METADATA 获取尺寸
    if 'ICON_METADATA[actualIconIndex]' in content and 'meta.width' in content and 'meta.height' in content:
        print("✅ Canvas 尺寸从 ICON_METADATA 获取")
    else:
        print("❌ Canvas 尺寸未从 ICON_METADATA 正确获取")
        return False
    
    # 检查是否使用了实际的尺寸
    if 'displayWidth = meta.width' in content and 'displayHeight = meta.height' in content:
        print("✅ 使用了 ICON_METADATA 中的实际宽高")
    else:
        print("⚠️  可能未完全使用 ICON_METADATA 中的尺寸")
    
    return True


def main():
    """主函数"""
    
    print()
    print("╔" + "=" * 68 + "╗")
    print("║  HTML 文件修改验证工具                                              ║")
    print("╚" + "=" * 68 + "╝")
    print()
    
    results = {
        'metadata': check_icon_metadata(),
        'drawing': check_drawing_function(),
        'canvas': check_canvas_size()
    }
    
    print()
    print("=" * 70)
    print("📊 验证总结")
    print("=" * 70)
    print()
    print(f"ICON_METADATA 检查: {'✅ 通过' if results['metadata'] else '❌ 失败'}")
    print(f"绘制函数检查:     {'✅ 通过' if results['drawing'] else '❌ 失败'}")
    print(f"Canvas 尺寸检查:  {'✅ 通过' if results['canvas'] else '❌ 失败'}")
    print()
    
    if all(results.values()):
        print("🎉 所有检查都通过！修改正确！")
        print()
        print("现在可以：")
        print("  1. 刷新浏览器查看效果")
        print("  2. 所有 icon 应该都能正确显示")
        print("  3. 没有乱码或错位")
        return 0
    else:
        print("⚠️  某些检查未通过，请检查修改")
        return 1


if __name__ == '__main__':
    exit(main())

#!/usr/bin/env python3
"""
ESP32 e-ink 图片转换工具 V2 - 诊断和测试脚本

此脚本用于：
1. 验证 Python 环境配置
2. 检查所需的依赖库
3. 诊断 GUI 启动问题
4. 直接启动 GUI 应用
"""

import sys
import os
from pathlib import Path

def print_header():
    """打印标题"""
    print("\n" + "=" * 70)
    print("  🖼️  ESP32 e-ink 图片转换工具 V2 - 诊断脚本")
    print("=" * 70 + "\n")

def check_python_version():
    """检查 Python 版本"""
    print("📌 检查 Python 版本...")
    version = sys.version_info
    print(f"   Python: {sys.version}")
    
    if version.major < 3 or (version.major == 3 and version.minor < 6):
        print("   ❌ 错误: 需要 Python 3.6 或更高版本")
        return False
    
    print("   ✅ Python 版本正确")
    return True

def check_pillow():
    """检查 Pillow 库"""
    print("\n📌 检查 Pillow 库...")
    try:
        import PIL
        from PIL import Image
        print(f"   ✅ Pillow 已安装，版本: {PIL.__version__}")
        return True
    except ImportError:
        print("   ❌ Pillow 未安装")
        print("\n   💡 解决方案:")
        print("      • 命令行执行:")
        print("        pip install --upgrade Pillow")
        print("      • 或使用镜像源（推荐国内用户）:")
        print("        pip install -i https://pypi.tsinghua.edu.cn/simple Pillow")
        return False

def check_tkinter():
    """检查 Tkinter 库"""
    print("\n📌 检查 Tkinter 库...")
    try:
        import tkinter as tk
        print(f"   ✅ Tkinter 已安装")
        
        # 尝试创建一个小窗口来验证显示能力
        try:
            root = tk.Tk()
            root.withdraw()  # 隐藏窗口
            root.destroy()
            print("   ✅ Tkinter 显示功能正常")
            return True
        except Exception as e:
            print(f"   ⚠️  Tkinter 显示可能有问题: {e}")
            return True  # 仍然继续，只是警告
    
    except ImportError:
        print("   ❌ Tkinter 未安装")
        print("\n   💡 解决方案:")
        print("      • Windows: 重新安装 Python，勾选 tcl/tk")
        print("      • macOS: brew install python-tk")
        print("      • Linux Debian/Ubuntu:")
        print("        sudo apt-get install python3-tk")
        print("      • Linux Fedora:")
        print("        sudo dnf install python3-tkinter")
        return False

def check_image_converter_tool():
    """检查转换器文件"""
    print("\n📌 检查转换器文件...")
    
    # 获取脚本所在目录
    script_dir = Path(__file__).parent
    
    converter_file = script_dir / "image_converter_tool.py"
    print(f"   寻找: {converter_file}")
    
    if not converter_file.exists():
        print(f"   ❌ 找不到转换器文件")
        return False
    
    print(f"   ✅ 找到转换器文件")
    
    # 尝试导入
    try:
        sys.path.insert(0, str(script_dir))
        from image_converter_tool import ImageConverter
        print(f"   ✅ 转换器可以导入")
        return True
    except ImportError as e:
        print(f"   ❌ 无法导入转换器: {e}")
        return False

def check_gui_file():
    """检查 GUI 文件"""
    print("\n📌 检查 GUI 文件...")
    
    script_dir = Path(__file__).parent
    gui_file = script_dir / "image_converter_gui_v2.py"
    
    print(f"   寻找: {gui_file}")
    
    if not gui_file.exists():
        print(f"   ❌ 找不到 GUI 文件")
        return False
    
    print(f"   ✅ 找到 GUI 文件")
    return True

def run_diagnostics():
    """运行所有诊断检查"""
    print_header()
    
    results = {
        "Python 版本": check_python_version(),
        "Pillow 库": check_pillow(),
        "Tkinter 库": check_tkinter(),
        "转换器文件": check_image_converter_tool(),
        "GUI 文件": check_gui_file(),
    }
    
    print("\n" + "=" * 70)
    print("📊 诊断结果摘要:")
    print("=" * 70)
    
    for check_name, result in results.items():
        status = "✅" if result else "❌"
        print(f"  {status} {check_name}")
    
    all_passed = all(results.values())
    
    print("\n" + "=" * 70)
    if all_passed:
        print("✅ 所有检查通过！系统准备就绪")
        print("=" * 70)
        return True
    else:
        print("❌ 部分检查失败，请查看上面的建议")
        print("=" * 70)
        return False

def launch_gui():
    """启动 GUI"""
    print("\n🚀 启动 GUI 应用...\n")
    
    script_dir = Path(__file__).parent
    gui_file = script_dir / "image_converter_gui_v2.py"
    
    try:
        # 改变工作目录
        os.chdir(script_dir)
        
        # 导入并运行 GUI
        sys.path.insert(0, str(script_dir))
        
        # 使用 exec 来运行 GUI，这样可以保持当前进程
        with open(gui_file, 'r', encoding='utf-8') as f:
            code = f.read()
        
        exec(code, {'__name__': '__main__'})
    
    except Exception as e:
        print(f"\n❌ 启动 GUI 失败: {e}")
        print("\n💡 故障排查:")
        print("   1. 检查上面的诊断结果")
        print("   2. 尝试手动运行: python image_converter_gui_v2.py")
        print("   3. 查看详细错误信息")
        import traceback
        traceback.print_exc()
        return False
    
    return True

def main():
    """主函数"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="ESP32 e-ink 图片转换工具 V2 - 诊断和启动脚本"
    )
    parser.add_argument(
        "--no-gui",
        action="store_true",
        help="仅运行诊断，不启动 GUI"
    )
    parser.add_argument(
        "--gui-only",
        action="store_true",
        help="直接启动 GUI，跳过诊断"
    )
    
    args = parser.parse_args()
    
    # 直接启动 GUI
    if args.gui_only:
        launch_gui()
        return
    
    # 运行诊断
    success = run_diagnostics()
    
    # 如果诊断通过，询问是否启动 GUI
    if success and not args.no_gui:
        print("\n💡 要启动 GUI，请选择:")
        print("   1. 按 Enter 键启动 GUI")
        print("   2. 按 Ctrl+C 退出")
        
        try:
            input("\n按 Enter 继续... ")
            launch_gui()
        except KeyboardInterrupt:
            print("\n👋 已退出")
    elif not success:
        print("\n❌ 由于诊断失败，无法启动 GUI")
        print("   请先解决上面报告的问题")
    else:
        print("\n✨ 诊断完成")

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
web_layout_standalone.html 环境检查工具
用于检查PC是否已正确配置以运行网页编辑器
"""

import os
import sys
import socket
import subprocess

def check_python_version():
    """检查 Python 版本"""
    print("1️⃣  检查 Python 版本...")
    version = sys.version_info
    if version.major >= 3 and version.minor >= 8:
        print(f"   ✅ Python {version.major}.{version.minor}.{version.micro} (满足要求)")
        return True
    else:
        print(f"   ❌ Python 版本过低: {version.major}.{version.minor}.{version.micro}")
        print(f"   建议升级到 Python 3.8 或以上")
        return False

def check_flask():
    """检查 Flask 是否已安装"""
    print("2️⃣  检查 Flask 依赖...")
    try:
        import flask
        print(f"   ✅ Flask {flask.__version__} 已安装")
        return True
    except ImportError:
        print("   ❌ Flask 未安装")
        print("   运行命令安装: pip install flask python-cors")
        return False

def check_cors():
    """检查 python-cors 是否已安装"""
    print("3️⃣  检查 python-cors 依赖...")
    try:
        import cors
        print(f"   ✅ python-cors 已安装")
        return True
    except ImportError:
        print("   ⚠️  python-cors 未安装")
        print("   运行命令安装: pip install python-cors")
        return False

def check_html_file():
    """检查 HTML 文件是否存在"""
    print("4️⃣  检查 HTML 文件...")
    file_path = "web_layout_standalone.html"
    if os.path.exists(file_path):
        size = os.path.getsize(file_path)
        size_mb = size / (1024 * 1024)
        print(f"   ✅ {file_path} 存在 ({size_mb:.1f} MB)")
        
        # 检查文件大小
        if size > 1000000:  # 大于1MB
            print(f"   ✓ 文件大小正常")
        else:
            print(f"   ⚠️  文件可能不完整")
            return False
        return True
    else:
        print(f"   ❌ {file_path} 不存在")
        print(f"   请确保在项目根目录运行此脚本")
        return False

def check_config_files():
    """检查配置文件"""
    print("5️⃣  检查配置文件...")
    required_files = [
        "config_server.py",
        "components/resource/layout.json",
    ]
    
    all_exist = True
    for file_path in required_files:
        if os.path.exists(file_path):
            print(f"   ✅ {file_path}")
        else:
            print(f"   ⚠️  {file_path} 不存在（可选）")
            all_exist = False
    
    return all_exist

def check_port_availability(port=5001):
    """检查端口是否可用"""
    print(f"6️⃣  检查 localhost:{port} 可用性...")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        result = sock.connect_ex(('127.0.0.1', port))
        if result == 0:
            print(f"   ⚠️  端口 {port} 已被占用（服务可能已在运行）")
            return "occupied"
        else:
            print(f"   ✅ 端口 {port} 可用")
            return "available"
    except Exception as e:
        print(f"   ❌ 检查失败: {e}")
        return "error"
    finally:
        sock.close()

def check_browsers():
    """检查常见浏览器"""
    print("7️⃣  检查浏览器...")
    
    browsers = {
        "Chrome": [
            r"C:\Program Files\Google\Chrome\Application\chrome.exe",
            r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
        ],
        "Firefox": [
            r"C:\Program Files\Mozilla Firefox\firefox.exe",
            r"C:\Program Files (x86)\Mozilla Firefox\firefox.exe",
        ],
        "Edge": [
            r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
        ],
    }
    
    found = False
    for browser_name, paths in browsers.items():
        for path in paths:
            if os.path.exists(path):
                print(f"   ✅ {browser_name} 已安装")
                found = True
                break
    
    if not found:
        print(f"   ⚠️  未找到常见浏览器")
        print(f"   请安装 Chrome、Firefox 或 Edge")
    
    return found

def main():
    """主检查流程"""
    print("\n" + "="*60)
    print("  web_layout_standalone.html 环境检查工具")
    print("="*60 + "\n")
    
    checks = {
        "Python版本": check_python_version(),
        "Flask依赖": check_flask(),
        "CORS依赖": check_cors(),
        "HTML文件": check_html_file(),
        "配置文件": check_config_files(),
        "端口可用性": check_port_availability() != "error",
        "浏览器": check_browsers(),
    }
    
    print("\n" + "="*60)
    print("  检查结果汇总")
    print("="*60 + "\n")
    
    passed = 0
    failed = 0
    
    for check_name, result in checks.items():
        status = "✅ 通过" if result else "❌ 失败"
        print(f"{status}: {check_name}")
        if result:
            passed += 1
        else:
            failed += 1
    
    print("\n" + "-"*60)
    print(f"总计: {passed} 项通过, {failed} 项失败\n")
    
    if failed == 0:
        print("🎉 所有检查通过！")
        print("\n✨ 建议的启动步骤：")
        print("   1. 启动 Python 服务器:")
        print("      python config_server.py")
        print("   2. 打开 web_layout_standalone.html")
        print("   3. 检查顶部状态栏显示环境信息")
        return 0
    else:
        print("⚠️  某些检查未通过")
        print("\n📋 按照上面的提示修复问题，然后重新运行此脚本")
        return 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n❌ 检查被中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ 检查出错: {e}")
        sys.exit(1)

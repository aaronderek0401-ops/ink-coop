#!/usr/bin/env python3
"""
初始化 components/resource 文件夹，创建示例配置文件
"""

import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent
RESOURCE_DIR = PROJECT_ROOT / "components" / "resource"

# 创建 resource 目录
RESOURCE_DIR.mkdir(parents=True, exist_ok=True)
print(f"✓ 创建/确保 resource 目录: {RESOURCE_DIR}")

# 焦点配置示例
focus_config_main = {
    "count": 15,
    "focusable_indices": list(range(15)),
    "screen_type": "main"
}

focus_config_vocab = {
    "count": 15,
    "focusable_indices": list(range(15)),
    "screen_type": "vocab"
}

# 子数组配置示例（可以为空）
subarray_config_main = {
    "sub_arrays": [],
    "screen_type": "main"
}

subarray_config_vocab = {
    "sub_arrays": [],
    "screen_type": "vocab"
}

# 配置文件映射
configs = {
    "main_focusable_rects_config.json": focus_config_main,
    "vocab_focusable_rects_config.json": focus_config_vocab,
    "main_subarray_config.json": subarray_config_main,
    "vocab_subarray_config.json": subarray_config_vocab,
}

# 创建或更新配置文件
for filename, config in configs.items():
    filepath = RESOURCE_DIR / filename
    
    if filepath.exists():
        print(f"⊘ 文件已存在（跳过）: {filepath.name}")
    else:
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2, ensure_ascii=False)
        print(f"✓ 创建配置文件: {filepath.name}")

print("\n✓ 初始化完成！")
print(f"配置文件位置: {RESOURCE_DIR}")
print("\n接下来的步骤:")
print("1. 运行 Python Flask 配置服务器:")
print("   python config_server.py")
print("\n2. 在浏览器中打开 web_layout.html")
print("\n3. 点击 '从设备加载焦点配置' 来加载配置")

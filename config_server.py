#!/usr/bin/env python3
"""
配置文件服务器 - 用于处理 web_layout.html 与本地 components/resource 文件夹的交互
在开发过程中运行此服务器，web UI 可以通过 HTTP 读写项目文件夹下的 JSON 配置文件
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import json
import os
from pathlib import Path

app = Flask(__name__)
CORS(app)

# 配置文件存储路径 - 相对于本脚本
PROJECT_ROOT = Path(__file__).parent
RESOURCE_DIR = PROJECT_ROOT / "components" / "resource"

# 确保 resource 目录存在
RESOURCE_DIR.mkdir(parents=True, exist_ok=True)

# 支持的配置文件列表
CONFIG_FILES = {
    'focus_main': 'main_focusable_rects_config.json',
    'focus_vocab': 'vocab_focusable_rects_config.json',
    'subarray_main': 'main_subarray_config.json',
    'subarray_vocab': 'vocab_subarray_config.json',
}


def get_config_file_path(config_type: str, screen_type: str) -> Path:
    """根据配置类型和屏幕类型获取文件路径"""
    if config_type == 'focus':
        filename = 'main_focusable_rects_config.json' if screen_type == 'main' else 'vocab_focusable_rects_config.json'
    elif config_type == 'subarray':
        filename = 'main_subarray_config.json' if screen_type == 'main' else 'vocab_subarray_config.json'
    else:
        return None
    
    return RESOURCE_DIR / filename


@app.route('/api/config/focus', methods=['GET', 'POST'])
def handle_focus_config():
    """处理焦点配置 - GET 读取，POST 保存"""
    screen_type = request.args.get('screen_type', 'vocab')
    
    if screen_type not in ['main', 'vocab']:
        screen_type = 'vocab'
    
    config_path = get_config_file_path('focus', screen_type)
    
    if request.method == 'GET':
        # 读取焦点配置
        if config_path.exists():
            try:
                with open(config_path, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                return jsonify(config), 200
            except Exception as e:
                print(f"Error reading focus config: {e}")
                return jsonify({"count": 0, "focusable_indices": []}), 200
        else:
            # 文件不存在，返回空配置
            return jsonify({"count": 0, "focusable_indices": []}), 200
    
    elif request.method == 'POST':
        # 保存焦点配置
        try:
            data = request.get_json()
            if not data:
                return jsonify({"status": "error", "message": "No JSON data"}), 400
            
            # 保存为 JSON 文件
            with open(config_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False)
            
            print(f"Focus config saved for {screen_type}: {config_path}")
            return jsonify({"status": "success", "message": f"Focus config saved for {screen_type}"}), 200
        except Exception as e:
            print(f"Error saving focus config: {e}")
            return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/config/subarray', methods=['GET', 'POST'])
def handle_subarray_config():
    """处理子数组配置 - GET 读取，POST 保存"""
    screen_type = request.args.get('screen_type', 'vocab')
    
    if screen_type not in ['main', 'vocab']:
        screen_type = 'vocab'
    
    config_path = get_config_file_path('subarray', screen_type)
    
    if request.method == 'GET':
        # 读取子数组配置
        if config_path.exists():
            try:
                with open(config_path, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                return jsonify(config), 200
            except Exception as e:
                print(f"Error reading subarray config: {e}")
                return jsonify({"sub_arrays": []}), 200
        else:
            # 文件不存在，返回空配置
            return jsonify({"sub_arrays": []}), 200
    
    elif request.method == 'POST':
        # 保存子数组配置
        try:
            data = request.get_json()
            if not data:
                return jsonify({"status": "error", "message": "No JSON data"}), 400
            
            # 保存为 JSON 文件
            with open(config_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False)
            
            print(f"Subarray config saved for {screen_type}: {config_path}")
            return jsonify({"status": "success", "message": f"Subarray config saved for {screen_type}"}), 200
        except Exception as e:
            print(f"Error saving subarray config: {e}")
            return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/config/list', methods=['GET'])
def list_configs():
    """列出所有可用的配置文件"""
    configs = {}
    for config_type, filename in CONFIG_FILES.items():
        filepath = RESOURCE_DIR / filename
        configs[config_type] = {
            'filename': filename,
            'path': str(filepath),
            'exists': filepath.exists()
        }
    
    return jsonify({
        'resource_dir': str(RESOURCE_DIR),
        'configs': configs
    }), 200


@app.route('/api/layout', methods=['GET'])
def get_layout():
    """获取布局文件 - GET 读取layout.json"""
    screen_type = request.args.get('screen_type', 'main')
    
    if screen_type not in ['main', 'vocab']:
        screen_type = 'main'
    
    # 根据screen_type选择不同的layout文件
    if screen_type == 'main':
        layout_file = RESOURCE_DIR / 'layout.json'
    else:
        layout_file = RESOURCE_DIR / 'vocab_layout_config.json'
    
    if layout_file.exists():
        try:
            with open(layout_file, 'r', encoding='utf-8') as f:
                layout_data = json.load(f)
            return jsonify(layout_data), 200
        except Exception as e:
            print(f"Error reading layout file: {e}")
            return jsonify({"status": "error", "message": f"Failed to read layout file: {str(e)}"}), 500
    else:
        # 文件不存在，返回错误
        return jsonify({
            "status": "error", 
            "message": f"Layout file not found: {screen_type}"
        }), 404


# ===== ESP32 原生接口映射（用于 PC 端兼容性）=====

@app.route('/getlayout', methods=['GET'])
def get_layout_esp32():
    """ESP32 兼容：获取主界面布局"""
    return get_layout()


@app.route('/getvocablayout', methods=['GET'])
def get_vocab_layout_esp32():
    """ESP32 兼容：获取单词界面布局"""
    return get_layout_with_type('vocab')


@app.route('/setlayout', methods=['POST'])
def set_layout():
    """ESP32 兼容：保存主界面布局"""
    try:
        layout_data = request.get_json()
        layout_file = RESOURCE_DIR / 'layout.json'
        
        with open(layout_file, 'w', encoding='utf-8') as f:
            json.dump(layout_data, f, ensure_ascii=False)
        
        return jsonify({"status": "ok", "message": "Layout saved"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": f"Failed to save layout: {str(e)}"}), 500


@app.route('/setvocablayout', methods=['POST'])
def set_vocab_layout():
    """ESP32 兼容：保存单词界面布局"""
    try:
        layout_data = request.get_json()
        layout_file = RESOURCE_DIR / 'vocab_layout_config.json'
        
        with open(layout_file, 'w', encoding='utf-8') as f:
            json.dump(layout_data, f, ensure_ascii=False)
        
        return jsonify({"status": "ok", "message": "Vocab layout saved"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": f"Failed to save vocab layout: {str(e)}"}), 500


def get_layout_with_type(screen_type):
    """获取指定类型的布局"""
    if screen_type not in ['main', 'vocab']:
        screen_type = 'main'
    
    if screen_type == 'main':
        layout_file = RESOURCE_DIR / 'layout.json'
    else:
        layout_file = RESOURCE_DIR / 'vocab_layout_config.json'
    
    if layout_file.exists():
        try:
            with open(layout_file, 'r', encoding='utf-8') as f:
                layout_data = json.load(f)
            return jsonify(layout_data), 200
        except Exception as e:
            return jsonify({"status": "error", "message": f"Failed to read layout file: {str(e)}"}), 500
    else:
        return jsonify({
            "status": "error", 
            "message": f"Layout file not found: {screen_type}"
        }), 404


@app.route('/getfocusconfig', methods=['GET'])
def get_focus_config_esp32():
    """ESP32 兼容：获取焦点配置"""
    screen_type = request.args.get('screen_type', 'main')
    return handle_focus_config()


@app.route('/setfocusconfig', methods=['POST'])
def set_focus_config_esp32():
    """ESP32 兼容：保存焦点配置"""
    return handle_focus_config()


@app.route('/getsubarrayconfig', methods=['GET'])
def get_subarray_config_esp32():
    """ESP32 兼容：获取子数组配置"""
    screen_type = request.args.get('screen_type', 'main')
    return handle_subarray_config()


@app.route('/setsubarrayconfig', methods=['POST'])
def set_subarray_config_esp32():
    """ESP32 兼容：保存子数组配置"""
    return handle_subarray_config()


@app.route('/api/icon/binary/<int:icon_index>', methods=['GET'])
def get_icon_binary(icon_index):
    """获取二进制 icon 文件"""
    # Icon 文件映射
    icon_files = {
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
    
    if icon_index not in icon_files:
        return jsonify({"status": "error", "message": f"Icon index {icon_index} not found"}), 404
    
    icon_filename = icon_files[icon_index]
    icon_path = RESOURCE_DIR / 'icon' / icon_filename
    
    print(f"[/api/icon/binary/{icon_index}] 请求图标文件: {icon_path}")
    print(f"[/api/icon/binary/{icon_index}] 文件存在: {icon_path.exists()}")
    
    if not icon_path.exists():
        return jsonify({
            "status": "error", 
            "message": f"Icon file not found: {icon_filename}",
            "path": str(icon_path)
        }), 404
    
    try:
        with open(icon_path, 'rb') as f:
            binary_data = f.read()
        
        print(f"[/api/icon/binary/{icon_index}] 成功读取图标文件, 原始大小: {len(binary_data)} bytes")
        
        # 跳过 8 字节文件头 (宽度 + 高度)
        HEADER_SIZE = 8
        if len(binary_data) > HEADER_SIZE:
            binary_data = binary_data[HEADER_SIZE:]
            print(f"[/api/icon/binary/{icon_index}] 已跳过 {HEADER_SIZE} 字节头部，返回纯位图数据: {len(binary_data)} bytes")
        
        # 返回二进制数据
        from flask import send_file
        from io import BytesIO
        return send_file(
            BytesIO(binary_data),
            mimetype='application/octet-stream',
            as_attachment=False,
            download_name=icon_filename
        )
    except Exception as e:
        print(f"[/api/icon/binary/{icon_index}] 读取文件失败: {e}")
        return jsonify({
            "status": "error", 
            "message": f"Failed to read icon file: {str(e)}"
        }), 500


@app.route('/api/health', methods=['GET'])
def health_check():
    """健康检查端点"""
    return jsonify({
        "status": "ok",
        "resource_dir": str(RESOURCE_DIR),
        "resource_dir_exists": RESOURCE_DIR.exists()
    }), 200


@app.route('/', methods=['GET'])
def index():
    """根路径 - 显示信息"""
    return jsonify({
        "name": "Ink Screen Config Server",
        "version": "1.0",
        "description": "Web UI configuration file server for ink screen project",
        "endpoints": {
            "GET /api/layout": "Get layout file (screen_type=main|vocab)",
            "GET /api/config/focus": "Get focus config (screen_type=main|vocab)",
            "POST /api/config/focus": "Save focus config",
            "GET /api/config/subarray": "Get subarray config (screen_type=main|vocab)",
            "POST /api/config/subarray": "Save subarray config",
            "GET /api/config/list": "List all config files",
            "GET /api/icon/binary/<index>": "Get binary icon file (index=0-12)",
            "GET /api/health": "Health check"
        },
        "resource_directory": str(RESOURCE_DIR)
    }), 200


if __name__ == '__main__':
    print(f"Starting Config Server...")
    print(f"Resource directory: {RESOURCE_DIR}")
    print(f"Serving on http://localhost:5001")
    print(f"\n=== PC 端专用 API ===")
    print(f"  GET  http://localhost:5001/api/layout?screen_type=main|vocab")
    print(f"  GET  http://localhost:5001/api/config/focus?screen_type=main|vocab")
    print(f"  POST http://localhost:5001/api/config/focus")
    print(f"  GET  http://localhost:5001/api/config/subarray?screen_type=main|vocab")
    print(f"  POST http://localhost:5001/api/config/subarray")
    print(f"  GET  http://localhost:5001/api/icon/binary/<index>")
    print(f"\n=== ESP32 兼容 API（优先 PC 端则使用下列接口）===")
    print(f"  GET  http://localhost:5001/getlayout                      (主界面布局)")
    print(f"  POST http://localhost:5001/setlayout                      (保存主界面布局)")
    print(f"  GET  http://localhost:5001/getvocablayout                 (单词界面布局)")
    print(f"  POST http://localhost:5001/setvocablayout                 (保存单词界面布局)")
    print(f"  GET  http://localhost:5001/getfocusconfig?screen_type=... (获取焦点配置)")
    print(f"  POST http://localhost:5001/setfocusconfig                 (保存焦点配置)")
    print(f"  GET  http://localhost:5001/getsubarrayconfig?screen_type=... (获取子数组配置)")
    print(f"  POST http://localhost:5001/setsubarrayconfig              (保存子数组配置)")
    print(f"\n  Health check: http://localhost:5001/api/health")
    
    app.run(host='0.0.0.0', port=5001, debug=True)

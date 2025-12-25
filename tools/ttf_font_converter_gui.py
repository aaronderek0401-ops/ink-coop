#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTF 字体转换工具 - GUI 版本
支持图形化选择字体、设置参数、导出格式
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path
import threading
from ttf_font_converter import TTFConverter, CHAR_SETS

class TTFConverterGUI:
    """TTF 字体转换 GUI"""
    
    def __init__(self, root):
        self.root = root
        self.root.title("🔤 TTF 字体转换工具")
        self.root.geometry("1000x900")
        self.root.minsize(900, 800)
        self.root.resizable(True, True)
        self.root.configure(bg='#f5f5f5')
        
        # 变量
        self.font_path = tk.StringVar()
        self.font_size = tk.StringVar(value="16")
        self.charset_var = tk.StringVar(value="全组合")
        self.custom_charset = tk.StringVar()
        self.output_format = tk.StringVar(value="gfx")
        self.output_path = tk.StringVar()
        
        self.create_widgets()
        self.center_window()
        
        # 绑定变量变化，自动更新输出路径
        self.font_path.trace('w', self.on_font_or_format_change)
        self.font_size.trace('w', self.on_font_or_format_change)
        self.output_format.trace('w', self.on_font_or_format_change)
    
    def center_window(self):
        """居中窗口"""
        self.root.update_idletasks()
        w = self.root.winfo_width()
        h = self.root.winfo_height()
        x = (self.root.winfo_screenwidth() // 2) - (w // 2)
        y = (self.root.winfo_screenheight() // 2) - (h // 2)
        self.root.geometry(f'{w}x{h}+{x}+{y}')
    
    def create_widgets(self):
        """创建 UI"""
        
        # 标题
        title_frame = tk.Frame(self.root, bg='#2196F3', height=60)
        title_frame.pack(fill=tk.X, padx=0, pady=0)
        title_frame.pack_propagate(False)
        
        title_label = tk.Label(
            title_frame,
            text="🔤 TTF 字体转换工具",
            font=("Arial", 18, "bold"),
            bg='#2196F3',
            fg='white',
            pady=10
        )
        title_label.pack()
        
        # 主容器 - 使用 scrollable 框架以适应较小的窗口
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=(5, 80))
        
        # ===== 字体文件选择 =====
        font_section = ttk.LabelFrame(main_frame, text="📁 选择字体文件", padding="8")
        font_section.pack(fill=tk.X, pady=(0, 8))
        
        btn_frame = tk.Frame(font_section, bg='#f5f5f5')
        btn_frame.pack(fill=tk.X, pady=(0, 5))
        
        tk.Button(
            btn_frame,
            text="📂 浏览...",
            command=self.select_font,
            width=15,
            height=1,
            font=("Arial", 10),
            bg='#2196F3',
            fg='white'
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            btn_frame,
            text="🗑️ 清除",
            command=lambda: self.font_path.set(""),
            width=10,
            height=1,
            font=("Arial", 10),
            bg='#999999',
            fg='white'
        ).pack(side=tk.LEFT, padx=5)
        
        font_display = tk.Entry(
            font_section,
            textvariable=self.font_path,
            state='readonly',
            width=80,
            font=("Arial", 9),
            bg='#f9f9f9'
        )
        font_display.pack(fill=tk.X, pady=(0, 8))
        
        self.font_info_label = tk.Label(
            font_section,
            text="未选择",
            font=("Arial", 9),
            fg='#666666',
            bg='#f5f5f5'
        )
        self.font_info_label.pack(fill=tk.X)
        
        # ===== 参数设置 =====
        param_section = ttk.LabelFrame(main_frame, text="⚙️ 参数设置", padding="8")
        param_section.pack(fill=tk.X, pady=(0, 8))
        
        # 字体大小
        size_frame = tk.Frame(param_section, bg='#f5f5f5')
        size_frame.pack(fill=tk.X, pady=5)
        
        tk.Label(size_frame, text="📏 字体大小 (pt):", width=15, anchor='w', bg='#f5f5f5', font=("Arial", 10)).pack(side=tk.LEFT)
        size_spin = tk.Spinbox(size_frame, from_=8, to=128, textvariable=self.font_size, width=8, font=("Arial", 11))
        size_spin.pack(side=tk.LEFT, padx=8)
        
        # 预设大小按钮
        for size in [12, 16, 24, 32]:
            tk.Button(size_frame, text=str(size), command=lambda s=size: self.font_size.set(str(s)), width=4, height=1, font=("Arial", 9)).pack(side=tk.LEFT, padx=2)
        
        # 字符集选择
        charset_frame = tk.Frame(param_section, bg='#f5f5f5')
        charset_frame.pack(fill=tk.X, pady=5)
        
        tk.Label(charset_frame, text="🔤 字符集:", width=15, anchor='w', bg='#f5f5f5', font=("Arial", 10)).pack(side=tk.LEFT)
        
        charset_combo = ttk.Combobox(
            charset_frame,
            textvariable=self.charset_var,
            values=list(CHAR_SETS.keys()),
            state='readonly',
            width=30
        )
        charset_combo.pack(side=tk.LEFT, padx=8)
        
        # 自定义字符集
        custom_frame = tk.Frame(param_section, bg='#f5f5f5')
        custom_frame.pack(fill=tk.X, pady=5)
        
        tk.Label(custom_frame, text="✏️ 自定义字符:", width=15, anchor='w', bg='#f5f5f5', font=("Arial", 10)).pack(side=tk.LEFT)
        
        custom_entry = tk.Entry(
            custom_frame,
            textvariable=self.custom_charset,
            width=50,
            font=("Arial", 10)
        )
        custom_entry.pack(side=tk.LEFT, padx=8, fill=tk.X, expand=True)
        
        self.char_count_label = tk.Label(
            custom_frame,
            text="0 个字符",
            font=("Arial", 9),
            fg='#666666',
            bg='#f5f5f5'
        )
        self.char_count_label.pack(side=tk.LEFT, padx=5)
        
        # 绑定自定义字符集的变化
        self.custom_charset.trace('w', self.update_char_count)
        
        # 输出格式
        format_frame = tk.Frame(param_section, bg='#f5f5f5')
        format_frame.pack(fill=tk.X, pady=5)
        
        tk.Label(format_frame, text="📦 输出格式:", width=15, anchor='w', bg='#f5f5f5', font=("Arial", 10)).pack(side=tk.LEFT)
        
        for fmt in ['GFX (.h 头文件)', 'BIN (二进制)']:
            value = fmt.split()[0].lower()
            tk.Radiobutton(
                format_frame,
                text=fmt,
                variable=self.output_format,
                value=value,
                font=("Arial", 10),
                bg='#f5f5f5'
            ).pack(side=tk.LEFT, padx=10)
        
        # 输出路径
        output_frame = tk.Frame(param_section, bg='#f5f5f5')
        output_frame.pack(fill=tk.X, pady=5)
        
        tk.Label(output_frame, text="💾 输出路径:", width=15, anchor='w', bg='#f5f5f5', font=("Arial", 10)).pack(side=tk.LEFT)
        
        tk.Button(
            output_frame,
            text="📂 浏览...",
            command=self.select_output,
            width=12,
            height=1,
            font=("Arial", 9),
            bg='#FF9800',
            fg='white'
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            output_frame,
            text="🗑️ 清除",
            command=lambda: self.output_path.set(""),
            width=8,
            height=1,
            font=("Arial", 9),
            bg='#999999',
            fg='white'
        ).pack(side=tk.LEFT, padx=2)
        
        output_display = tk.Entry(
            output_frame,
            textvariable=self.output_path,
            state='readonly',
            width=50,
            font=("Arial", 9),
            bg='#f9f9f9'
        )
        output_display.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        
        self.output_info_label = tk.Label(
            param_section,
            text="💡 未指定时，将保存到字体文件所在的文件夹",
            font=("Arial", 8),
            fg='#999999',
            bg='#f5f5f5'
        )
        self.output_info_label.pack(fill=tk.X, padx=10)
        
        # ===== 字符集预览 =====
        preview_section = ttk.LabelFrame(main_frame, text="👀 字符集预览", padding="10")
        preview_section.pack(fill=tk.X, pady=(0, 10))
        
        self.preview_text = tk.Text(
            preview_section,
            height=2,
            width=80,
            font=("Arial", 8),
            bg='#ffffff',
            relief=tk.SUNKEN,
            bd=1
        )
        self.preview_text.pack(fill=tk.X)
        
        # 绑定字符集变化
        self.charset_var.trace('w', self.update_preview)
        self.custom_charset.trace('w', self.update_preview)
        
        # ===== 进度 =====
        progress_section = ttk.LabelFrame(main_frame, text="📊 转换进度", padding="8")
        progress_section.pack(fill=tk.X, pady=(0, 8))
        
        self.progress_bar = ttk.Progressbar(
            progress_section,
            mode='indeterminate',
            length=500
        )
        self.progress_bar.pack(fill=tk.X, pady=5)
        
        self.status_label = tk.Label(
            progress_section,
            text="就绪",
            font=("Arial", 10),
            fg='#4CAF50'
        )
        self.status_label.pack(fill=tk.X)
        
        # ===== 按钮 =====
        button_frame = tk.Frame(self.root, bg='#f5f5f5', height=90)
        button_frame.pack(fill=tk.X, padx=10, pady=15, side=tk.BOTTOM)
        button_frame.pack_propagate(False)
        
        self.convert_button = tk.Button(
            button_frame,
            text="🚀 开始转换",
            command=self.start_conversion,
            width=30,
            height=2,
            font=("Arial", 13, "bold"),
            bg='#4CAF50',
            fg='white',
            relief=tk.RAISED,
            bd=5,
            cursor="hand2"
        )
        self.convert_button.pack(side=tk.LEFT, padx=15, pady=8)
        
        tk.Button(
            button_frame,
            text="❌ 关闭",
            command=self.root.quit,
            width=12,
            height=2,
            font=("Arial", 11),
            bg='#f44336',
            fg='white',
            relief=tk.RAISED,
            bd=2,
            cursor="hand2"
        ).pack(side=tk.LEFT, padx=5, pady=8)
    
    def select_font(self):
        """选择字体文件"""
        file = filedialog.askopenfilename(
            title="选择 TTF 字体文件",
            filetypes=[("TrueType 字体", "*.ttf"), ("OpenType 字体", "*.otf"), ("所有文件", "*.*")]
        )
        
        if file:
            self.font_path.set(file)
            font_file = Path(file)
            self.font_info_label.config(text=f"✅ {font_file.name} ({font_file.stat().st_size / 1024:.1f} KB)")
            
            # 自动生成输出路径：字体文件名 + 字体大小 + 输出格式后缀
            font_size = self.font_size.get()
            file_ext = ".h" if self.output_format.get() == "gfx" else ".bin"
            output_filename = f"{font_file.stem}_{font_size}pt{file_ext}"
            output_full_path = font_file.parent / output_filename
            
            self.output_path.set(str(output_full_path))
            self.output_info_label.config(
                text=f"✅ 输出到: {output_full_path.parent.name}/{output_filename}",
                fg='#4CAF50'
            )
    
    def on_font_or_format_change(self, *args):
        """当字体、字体大小或输出格式改变时，自动更新输出路径"""
        if self.font_path.get():  # 只有选择了字体才更新
            font_path = Path(self.font_path.get())
            font_size = self.font_size.get()
            file_ext = ".h" if self.output_format.get() == "gfx" else ".bin"
            output_filename = f"{font_path.stem}_{font_size}pt{file_ext}"
            output_full_path = font_path.parent / output_filename
            
            self.output_path.set(str(output_full_path))
            self.output_info_label.config(
                text=f"💾 输出到: {output_full_path.parent.name}/{output_filename}",
                fg='#4CAF50'
            )
    
    def select_output(self):
        """选择输出文件路径"""
        file_types = [("C 头文件", "*.h"), ("二进制文件", "*.bin"), ("所有文件", "*.*")]
        
        # 生成默认文件名（与当前自动生成的输出路径一致）
        default_filename = ""
        if self.font_path.get():
            font_path = Path(self.font_path.get())
            font_size = self.font_size.get()
            file_ext = ".h" if self.output_format.get() == "gfx" else ".bin"
            default_filename = f"{font_path.stem}_{font_size}pt{file_ext}"
        
        file = filedialog.asksaveasfilename(
            title="选择输出文件路径",
            initialfile=default_filename,  # 设置默认文件名
            defaultextension=".h" if self.output_format.get() == "gfx" else ".bin",
            filetypes=file_types
        )
        
        if file:
            self.output_path.set(file)
            output_file = Path(file)
            self.output_info_label.config(
                text=f"✅ 自定义输出: {output_file.parent.name}/{output_file.name}",
                fg='#4CAF50'
            )
    
    def update_preview(self, *args):
        """更新字符集预览"""
        self.preview_text.config(state=tk.NORMAL)
        self.preview_text.delete(1.0, tk.END)
        
        # 获取当前字符集
        if self.custom_charset.get():
            charset = self.custom_charset.get()
        else:
            charset_name = self.charset_var.get()
            charset = CHAR_SETS.get(charset_name, "")
        
        # 显示预览
        if charset:
            self.preview_text.insert(1.0, f"预览 ({len(set(charset))} 个字符):\n\n{charset}")
        else:
            self.preview_text.insert(1.0, "无字符")
        
        self.preview_text.config(state=tk.DISABLED)
    
    def update_char_count(self, *args):
        """更新字符数量显示"""
        charset = self.custom_charset.get()
        if charset:
            count = len(set(charset))
            self.char_count_label.config(text=f"{count} 个字符")
    
    def start_conversion(self):
        """开始转换"""
        if not self.font_path.get():
            messagebox.showerror("错误", "请先选择字体文件！")
            return
        
        try:
            font_size = int(self.font_size.get())
            if font_size < 8 or font_size > 128:
                raise ValueError("字体大小必须在 8-128 之间")
        except ValueError as e:
            messagebox.showerror("错误", f"字体大小错误: {e}")
            return
        
        # 获取字符集
        if self.custom_charset.get():
            charset = self.custom_charset.get()
        else:
            charset_name = self.charset_var.get()
            charset = CHAR_SETS.get(charset_name, "")
        
        if not charset:
            messagebox.showerror("错误", "请选择或输入字符集！")
            return
        
        # 在后台线程中执行转换
        thread = threading.Thread(
            target=self.convert_worker,
            args=(self.font_path.get(), font_size, charset, self.output_path.get()),
            daemon=True
        )
        thread.start()
    
    def convert_worker(self, font_path, font_size, charset, output_path):
        """转换工作线程"""
        try:
            self.convert_button.config(state=tk.DISABLED)
            self.progress_bar.start()
            self.update_status("正在初始化转换器...", "#2196F3")
            
            # 创建转换器
            converter = TTFConverter(font_path, font_size, charset)
            
            # 转换 (如果指定了输出路径，则使用指定路径)
            output_file = output_path if output_path else None
            if self.output_format.get() == 'gfx':
                self.update_status("正在生成 GFX 格式...", "#2196F3")
                output = converter.convert_to_gfx(output_file)
            else:
                self.update_status("正在生成二进制格式...", "#2196F3")
                output = converter.convert_to_bin(output_file)
            
            self.update_status(f"✅ 转换成功！输出: {Path(output).name}", "#4CAF50")
            messagebox.showinfo(
                "成功",
                f"✅ 转换完成！\n\n输出文件: {output}"
            )
        
        except Exception as e:
            self.update_status(f"❌ 转换失败: {e}", "#f44336")
            messagebox.showerror("错误", f"转换失败:\n{e}")
        
        finally:
            self.progress_bar.stop()
            self.convert_button.config(state=tk.NORMAL)
    
    def update_status(self, message, color='#666666'):
        """更新状态"""
        self.status_label.config(text=message, fg=color)
        self.root.update()


def main():
    """主函数"""
    root = tk.Tk()
    app = TTFConverterGUI(root)
    root.mainloop()


if __name__ == '__main__':
    main()

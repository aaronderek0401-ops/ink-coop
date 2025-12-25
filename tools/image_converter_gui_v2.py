#!/usr/bin/env python3
"""
ESP32 e-ink 图片转换工具 - GUI 版本（改进版）

点击运行的图形界面工具，无需命令行
支持：
  • 选择输入文件或文件夹
  • 输出目录自选
  • 对话框设置宽高
  • 实时进度显示
  • 转换完成提示
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path
import threading
from image_converter_tool import ImageConverter
import sys

class ImageConverterGUI:
    """图片转换工具 GUI"""
    
    def __init__(self, root):
        self.root = root
        self.root.title("🖼️ ESP32 e-ink 图片转换工具")
        self.root.geometry("1000x700")
        self.root.minsize(900, 650)
        self.root.resizable(True, True)
        
        # 设置主题色
        self.root.configure(bg='#f5f5f5')
        
        # 存储用户选择
        self.input_path = tk.StringVar()
        self.output_path = tk.StringVar()
        self.width_var = tk.StringVar(value="416")
        self.height_var = tk.StringVar(value="240")
        self.is_directory = tk.BooleanVar(value=False)
        
        # 创建 UI
        self.create_widgets()
        self.center_window()
    
    def center_window(self):
        """居中窗口"""
        self.root.update_idletasks()
        width = self.root.winfo_width()
        height = self.root.winfo_height()
        x = (self.root.winfo_screenwidth() // 2) - (width // 2)
        y = (self.root.winfo_screenheight() // 2) - (height // 2)
        self.root.geometry(f'{width}x{height}+{x}+{y}')
    
    def create_widgets(self):
        """创建 UI 组件"""
        
        # ===== 标题 =====
        title_frame = tk.Frame(self.root, bg='#2196F3', height=70)
        title_frame.pack(fill=tk.X, padx=0, pady=0)
        title_frame.pack_propagate(False)
        
        title_label = tk.Label(
            title_frame,
            text="🖼️  ESP32 e-ink 图片转换工具",
            font=("Arial", 20, "bold"),
            bg='#2196F3',
            fg='white',
            pady=15
        )
        title_label.pack()
        
        # ===== 主容器 =====
        main_frame = ttk.Frame(self.root, padding="15")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # ===== 输入文件/文件夹 =====
        input_section = ttk.LabelFrame(main_frame, text="📂 选择输入", padding="12")
        input_section.pack(fill=tk.X, pady=(0, 12))
        
        input_button_frame = tk.Frame(input_section, bg='#f5f5f5')
        input_button_frame.pack(fill=tk.X, pady=(0, 10))
        
        tk.Button(
            input_button_frame,
            text="📁 选择图片文件",
            command=self.select_input_file,
            width=18,
            height=1,
            font=("Arial", 10),
            bg='#2196F3',
            fg='white',
            activebackground='#0b7dda',
            relief=tk.RAISED,
            bd=2
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            input_button_frame,
            text="📂 选择图片文件夹",
            command=self.select_input_folder,
            width=18,
            height=1,
            font=("Arial", 10),
            bg='#2196F3',
            fg='white',
            activebackground='#0b7dda',
            relief=tk.RAISED,
            bd=2
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            input_button_frame,
            text="🗑️ 清除",
            command=lambda: self.input_path.set(""),
            width=10,
            height=1,
            font=("Arial", 10),
            bg='#999999',
            fg='white',
            relief=tk.RAISED,
            bd=2
        ).pack(side=tk.LEFT, padx=5)
        
        input_display = tk.Entry(
            input_section,
            textvariable=self.input_path,
            state='readonly',
            width=70,
            font=("Arial", 9),
            bg='#f9f9f9',
            relief=tk.SUNKEN,
            bd=2
        )
        input_display.pack(fill=tk.X, pady=(0, 8))
        
        self.input_type_label = tk.Label(
            input_section,
            text="",
            font=("Arial", 9),
            fg='#666666',
            bg='#f5f5f5'
        )
        self.input_type_label.pack(fill=tk.X)
        
        # ===== 输出文件夹 =====
        output_section = ttk.LabelFrame(main_frame, text="💾 选择输出（可选）", padding="12")
        output_section.pack(fill=tk.X, pady=(0, 12))
        
        output_button_frame = tk.Frame(output_section, bg='#f5f5f5')
        output_button_frame.pack(fill=tk.X, pady=(0, 10))
        
        tk.Button(
            output_button_frame,
            text="📂 选择输出文件夹",
            command=self.select_output_folder,
            width=18,
            height=1,
            font=("Arial", 10),
            bg='#4CAF50',
            fg='white',
            activebackground='#45a049',
            relief=tk.RAISED,
            bd=2
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            output_button_frame,
            text="🗑️ 清除",
            command=lambda: self.output_path.set(""),
            width=10,
            height=1,
            font=("Arial", 10),
            bg='#999999',
            fg='white',
            relief=tk.RAISED,
            bd=2
        ).pack(side=tk.LEFT, padx=5)
        
        output_display = tk.Entry(
            output_section,
            textvariable=self.output_path,
            state='readonly',
            width=70,
            font=("Arial", 9),
            bg='#f9f9f9',
            relief=tk.SUNKEN,
            bd=2
        )
        output_display.pack(fill=tk.X, pady=(0, 8))
        
        self.output_status_label = tk.Label(
            output_section,
            text="💡 提示：不指定则保存到输入目录",
            font=("Arial", 9),
            fg='#666666',
            bg='#f5f5f5'
        )
        self.output_status_label.pack(fill=tk.X)
        
        # ===== 参数设置 =====
        param_section = ttk.LabelFrame(main_frame, text="⚙️ 参数设置", padding="12")
        param_section.pack(fill=tk.X, pady=(0, 12))
        
        # 宽度
        width_frame = tk.Frame(param_section, bg='#f5f5f5')
        width_frame.pack(fill=tk.X, pady=8)
        
        tk.Label(width_frame, text="📏 图片宽度:", width=15, anchor='w', bg='#f5f5f5', font=("Arial", 10)).pack(side=tk.LEFT)
        tk.Entry(width_frame, textvariable=self.width_var, width=12, font=("Arial", 11), relief=tk.SUNKEN, bd=2).pack(side=tk.LEFT, padx=8)
        
        # 快速预设按钮
        tk.Button(width_frame, text="小\n(200)", command=lambda: self.width_var.set("200"), width=6, height=2, font=("Arial", 8), bg='#FF9800', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        tk.Button(width_frame, text="标准\n(416)", command=lambda: self.width_var.set("416"), width=6, height=2, font=("Arial", 8), bg='#2196F3', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        tk.Button(width_frame, text="大\n(800)", command=lambda: self.width_var.set("800"), width=6, height=2, font=("Arial", 8), bg='#4CAF50', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        tk.Button(width_frame, text="高清\n(1024)", command=lambda: self.width_var.set("1024"), width=6, height=2, font=("Arial", 8), bg='#9C27B0', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        
        # 高度
        height_frame = tk.Frame(param_section, bg='#f5f5f5')
        height_frame.pack(fill=tk.X, pady=8)
        
        tk.Label(height_frame, text="📏 图片高度:", width=15, anchor='w', bg='#f5f5f5', font=("Arial", 10)).pack(side=tk.LEFT)
        tk.Entry(height_frame, textvariable=self.height_var, width=12, font=("Arial", 11), relief=tk.SUNKEN, bd=2).pack(side=tk.LEFT, padx=8)
        
        tk.Button(height_frame, text="小\n(200)", command=lambda: self.height_var.set("200"), width=6, height=2, font=("Arial", 8), bg='#FF9800', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        tk.Button(height_frame, text="标准\n(240)", command=lambda: self.height_var.set("240"), width=6, height=2, font=("Arial", 8), bg='#2196F3', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        tk.Button(height_frame, text="大\n(480)", command=lambda: self.height_var.set("480"), width=6, height=2, font=("Arial", 8), bg='#4CAF50', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        tk.Button(height_frame, text="高清\n(768)", command=lambda: self.height_var.set("768"), width=6, height=2, font=("Arial", 8), bg='#9C27B0', fg='white', relief=tk.RAISED, bd=1).pack(side=tk.LEFT, padx=2)
        
        # 信息文本
        info_text = "💡 建议: 标准屏幕用 416×240，小屏幕用 200×200"
        info_label = tk.Label(
            param_section,
            text=info_text,
            font=("Arial", 9),
            fg='#666666',
            bg='#f5f5f5'
        )
        info_label.pack(fill=tk.X, pady=(10, 0))
        
        # ===== 进度 =====
        progress_section = ttk.LabelFrame(main_frame, text="📊 转换进度", padding="12")
        progress_section.pack(fill=tk.X, pady=(0, 12))
        
        self.progress_bar = ttk.Progressbar(
            progress_section,
            mode='indeterminate',
            length=500
        )
        self.progress_bar.pack(fill=tk.X, pady=10)
        
        self.status_label = tk.Label(
            progress_section,
            text="就绪，选择图片后点击开始转换",
            font=("Arial", 10),
            fg="#4CAF50"
        )
        self.status_label.pack(fill=tk.X)
        
        # 日志文本（隐藏，但保留功能）
        self.log_text = tk.Text(
            self.root,
            height=0,
            width=0,
            state=tk.DISABLED
        )
        
        # ===== 按钮 =====
        button_frame = tk.Frame(self.root, bg='#f5f5f5', height=110)
        button_frame.pack(fill=tk.X, padx=10, pady=15)
        button_frame.pack_propagate(False)
        
        # 开始转换按钮（特别突出）
        self.convert_button = tk.Button(
            button_frame,
            text="🚀 开始转换",
            command=self.start_conversion,
            width=35,
            height=3,
            font=("Arial", 15, "bold"),
            bg='#4CAF50',
            fg='white',
            activebackground='#45a049',
            activeforeground='white',
            relief=tk.RAISED,
            bd=5,
            cursor="hand2"
        )
        self.convert_button.pack(side=tk.LEFT, padx=15, pady=10)
        
        # 关闭按钮
        tk.Button(
            button_frame,
            text="❌ 关闭",
            command=self.root.quit,
            width=15,
            height=3,
            font=("Arial", 12),
            bg='#f44336',
            fg='white',
            activebackground='#da190b',
            activeforeground='white',
            relief=tk.RAISED,
            bd=2,
            cursor="hand2"
        ).pack(side=tk.LEFT, padx=5, pady=10)
    
    def select_input_file(self):
        """选择输入文件"""
        file_path = filedialog.askopenfilename(
            title="选择图片文件",
            filetypes=[
                ("所有支持格式", "*.jpg *.jpeg *.png *.bmp *.gif *.webp"),
                ("JPEG", "*.jpg *.jpeg"),
                ("PNG", "*.png"),
                ("所有文件", "*.*")
            ]
        )
        
        if file_path:
            self.input_path.set(file_path)
            self.input_type_label.config(text="✅ 单个文件", fg='#4CAF50')
    
    def select_input_folder(self):
        """选择输入文件夹"""
        folder_path = filedialog.askdirectory(title="选择包含图片的文件夹")
        
        if folder_path:
            self.input_path.set(folder_path)
            self.input_type_label.config(text="✅ 文件夹（批量转换）", fg='#4CAF50')
    
    def select_output_folder(self):
        """选择输出文件夹"""
        folder_path = filedialog.askdirectory(title="选择输出文件夹")
        
        if folder_path:
            self.output_path.set(folder_path)
            self.output_status_label.config(
                text=f"✅ 已选择输出文件夹",
                fg='#4CAF50'
            )
    
    def log_message(self, message, level='info'):
        """记录消息到状态标签"""
        if level == 'info':
            prefix = "ℹ️ "
            color = "#666666"
        elif level == 'success':
            prefix = "✅ "
            color = "#4CAF50"
        elif level == 'error':
            prefix = "❌ "
            color = "#f44336"
        elif level == 'warning':
            prefix = "⚠️  "
            color = "#FF9800"
        else:
            prefix = ""
            color = "#666666"
        
        # 更新状态标签
        self.status_label.config(text=f"{prefix}{message}", fg=color)
        self.root.update()
    
    def start_conversion(self):
        """开始转换"""
        # 验证输入
        if not self.input_path.get():
            messagebox.showerror("错误", "请先选择输入文件或文件夹！")
            return
        
        # 验证宽度和高度
        try:
            width = int(self.width_var.get())
            height = int(self.height_var.get())
            
            if width <= 0 or height <= 0:
                raise ValueError()
        except ValueError:
            messagebox.showerror("错误", "宽度和高度必须是正整数！")
            return
        
        # 在后台线程中执行转换
        thread = threading.Thread(
            target=self.convert_worker,
            args=(width, height),
            daemon=True
        )
        thread.start()
    
    def convert_worker(self, width, height):
        """转换工作线程"""
        try:
            self.convert_button.config(state=tk.DISABLED)
            self.progress_bar.start()
            self.log_text.config(state=tk.NORMAL)
            self.log_text.delete(1.0, tk.END)
            self.log_text.config(state=tk.DISABLED)
            
            input_path = Path(self.input_path.get())
            output_path = Path(self.output_path.get()) if self.output_path.get() else None
            
            self.log_message("=" * 60)
            self.log_message("🖼️  ESP32 e-ink 图片转换工具")
            self.log_message("=" * 60)
            self.log_message(f"输入: {input_path}")
            self.log_message(f"宽度: {width} px")
            self.log_message(f"高度: {height} px")
            self.log_message(f"模式: Floyd-Steinberg 抖动算法")
            self.log_message("")
            
            converter = ImageConverter(width=width, height=height)
            
            # 判断是文件还是目录
            if input_path.is_file():
                self.log_message("📄 转换单个文件...")
                self.log_message("")
                
                if output_path is None:
                    output_file = input_path.with_suffix('.bin')
                else:
                    output_file = output_path / input_path.with_suffix('.bin').name
                
                success, output, size = converter.convert_image_to_bin(
                    str(input_path),
                    str(output_file)
                )
                
                if success:
                    self.log_message("")
                    self.log_message(f"✅ 转换成功!", level='success')
                    self.log_message(f"输出文件: {output_file}")
                    self.log_message("")
                    self.log_message("✨ 完成!")
                    messagebox.showinfo(
                        "成功",
                        f"✅ 图片转换成功！\n\n输出文件: {output_file}"
                    )
                else:
                    self.log_message("❌ 转换失败！", level='error')
                    messagebox.showerror("失败", "图片转换失败！请检查输入文件。")
            
            elif input_path.is_dir():
                self.log_message("📁 批量转换目录...")
                self.log_message("")
                
                success_count, failed_count = converter.convert_directory(
                    str(input_path),
                    str(output_path) if output_path else None,
                    backup=True,
                    delete_originals=True
                )
                
                self.log_message("")
                self.log_message("=" * 60)
                self.log_message("📊 转换统计:")
                self.log_message(f"   ✅ 成功: {success_count}")
                self.log_message(f"   ❌ 失败: {failed_count}")
                self.log_message(f"   总计: {success_count + failed_count}")
                self.log_message("=" * 60)
                self.log_message("")
                self.log_message("✨ 完成!")
                
                messagebox.showinfo(
                    "完成",
                    f"✅ 转换完成！\n\n成功: {success_count}\n失败: {failed_count}"
                )
            
            else:
                messagebox.showerror("错误", "输入路径既不是文件也不是目录！")
        
        except Exception as e:
            self.log_message(f"❌ 错误: {e}", level='error')
            messagebox.showerror("错误", f"转换过程中出错：\n{e}")
        
        finally:
            self.progress_bar.stop()
            self.convert_button.config(state=tk.NORMAL)


def main():
    """主函数"""
    root = tk.Tk()
    app = ImageConverterGUI(root)
    root.mainloop()


if __name__ == '__main__':
    main()

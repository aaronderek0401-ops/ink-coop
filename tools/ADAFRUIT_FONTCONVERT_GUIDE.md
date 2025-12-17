# 使用 Adafruit fontconvert 工具

## 下载工具

```powershell
# 克隆 Adafruit GFX Library
git clone https://github.com/adafruit/Adafruit-GFX-Library.git
cd Adafruit-GFX-Library/fontconvert

# 编译工具（需要 MinGW 或 Visual Studio）
gcc fontconvert.c -o fontconvert.exe -lfreetype
```

## 使用方法

```powershell
# 转换字体（指定字符范围）
.\fontconvert.exe C:\Windows\Fonts\simhei.ttf 16 > simhei16pt7b.h
```

## 指定自定义字符

需要修改 fontconvert.c 源代码来支持自定义字符列表。

---

## 更简单的方法：使用 Python pillow

我可以为你创建一个 Python 脚本，使用 PIL/Pillow 库生成字体位图。

需要我创建吗？

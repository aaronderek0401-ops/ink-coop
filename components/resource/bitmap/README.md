# Bitmap 图片库

此文件夹用于存储转换后的位图文件。

## 用途

存储通过 `web_layout.html` 图片转位图功能生成的 `.h` 文件。

## 使用方法

1. 在 `web_layout.html` 中上传图片
2. 处理并转换为位图
3. 点击"💾 保存到服务器"按钮，文件会自动保存到此文件夹
4. 点击"🔍 查询图片库"可以查看所有已保存的位图
5. 点击"预览"按钮可以查看位图效果
6. 点击"导入"按钮将位图应用到程序中

## 文件格式

所有文件都是 C 头文件格式 (`.h`)，包含：
- `IMAGE_WIDTH` - 图片宽度定义
- `IMAGE_HEIGHT` - 图片高度定义
- `IMAGE_BITMAP[]` - 位图数据数组

## 在代码中使用

```cpp
#include "../bitmap/your_image_bitmap.h"

void displayImage() {
    display.drawBitmap(x, y, IMAGE_BITMAP, IMAGE_WIDTH, IMAGE_HEIGHT, GxEPD_BLACK);
}
```

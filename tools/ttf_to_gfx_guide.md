# TTF 字体转换为 Adafruit GFX 格式指南

## 方法 1: 使用在线工具（最简单）

### truetype2gfx 在线转换器
网址：https://rop.nl/truetype2gfx/

**步骤：**
1. 上传你的 TTF 字体文件
2. 选择字符集（如中文：GB2312、GBK）
3. 选择字号（建议 12, 16, 24, 32）
4. 点击 "Convert" 下载 .h 文件

## 方法 2: 使用 fontconvert 工具

### 安装
```bash
# 克隆 Adafruit GFX 仓库
git clone https://github.com/adafruit/Adafruit-GFX-Library.git
cd Adafruit-GFX-Library/fontconvert

# 编译工具
make
```

### 使用
```bash
# 转换 TTF 字体（指定字号）
./fontconvert YourFont.ttf 16 > MyFont16pt.h
./fontconvert YourFont.ttf 24 > MyFont24pt.h

# 转换指定字符范围（中文）
./fontconvert SimHei.ttf 16 0x4E00 0x9FFF > SimHei16pt_CN.h
```

## 方法 3: 使用 Python 脚本

```python
# 需要安装 PIL 和 freetype
pip install pillow freetype-py

# 使用脚本生成字体数组
# （这里可以创建自定义脚本）
```

## 生成的文件格式示例

```c
const uint8_t MyFont16ptBitmaps[] PROGMEM = {
  0xFF, 0xF0, 0xFF, 0xF0, ...  // 字形位图数据
};

const GFXglyph MyFont16ptGlyphs[] PROGMEM = {
  {     0,   8,  16,   9,    0,  -15 },   // 字符 '0'
  {    16,   5,  16,   9,    2,  -15 },   // 字符 '1'
  ...
};

const GFXfont MyFont16pt PROGMEM = {
  (uint8_t  *)MyFont16ptBitmaps,
  (GFXglyph *)MyFont16ptGlyphs,
  0x20, 0x7E, 16
};
```

## 中文字体特殊处理

由于中文字符数量巨大（20000+），建议：

1. **只转换常用字** - 选择 3500 个常用汉字
2. **分批加载** - 按使用场景分类
3. **使用 u8g2 字体** - 专为中文优化

## 注意事项

⚠️ **内存限制**：
- 每个 16×16 中文字占用 32 字节
- 3500 字 ≈ 110KB
- ESP32-S3 Flash 足够，但建议使用外部 PSRAM

⚠️ **性能考虑**：
- 大字号（32×32）渲染较慢
- 建议预渲染常用文字

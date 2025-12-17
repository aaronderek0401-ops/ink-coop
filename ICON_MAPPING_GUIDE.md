# 图标映射指南

## 概述

本文档说明了web界面上图标操作（添加、选择、显示）与 `components/resource/icon/` 文件夹中的图标文件之间的关系和同步机制。

## 核心概念

### 三个同步对象

所有图标操作都涉及以下三个必须保持同步的对象：

| 对象 | 位置 | 说明 |
|------|------|------|
| `g_available_icons` | `ink_screen.cpp` 行 135-148 | 图标数据数组，包含位图数据指针和尺寸 |
| `g_icon_filenames` | `ink_screen.cpp` 行 118-131 | 图标文件名映射表，指向 `components/resource/icon/` 中的文件 |
| 实际文件 | `components/resource/icon/` | 磁盘上存储的图标文件 |

### 索引对应关系

索引 0-12 必须以**完全相同的顺序**对应三个对象：

```
索引 → g_available_icons[index] → g_icon_filenames[index] → components/resource/icon/{filename}
```

## 当前映射表

```
索引 | 数组信息 | 文件名 | 文件路径
-----|---------|---------|----------
 0   | ICON_1 (62x64) | icon1.jpg | /sd/resource/icon/icon1.jpg
 1   | ICON_2 (64x64) | icon2.jpg | /sd/resource/icon/icon2.jpg
 2   | ICON_3 (86x64) | icon3.jpg | /sd/resource/icon/icon3.jpg
 3   | ICON_4 (71x56) | icon4.jpg | /sd/resource/icon/icon4.jpg
 4   | ICON_5 (76x56) | icon5.jpg | /sd/resource/icon/icon5.jpg
 5   | ICON_6 (94x64) | icon6.jpg | /sd/resource/icon/icon6.jpg
 6   | separate (120x8) | 12.jpg | /sd/resource/icon/12.jpg
 7   | WIFI_CONNECT (32x32) | wifi_connect.jpg | /sd/resource/icon/wifi_connect.jpg
 8   | WIFI_DISCONNECT (32x32) | wifi_disconnect.jpg | /sd/resource/icon/wifi_disconnect.jpg
 9   | BATTERY_1 (36x24) | battery_1.jpg | /sd/resource/icon/battery_1.jpg
10   | HORN (16x16) | horn.jpg | /sd/resource/icon/horn.jpg
11   | NAIL (15x16) | nail.jpg | /sd/resource/icon/nail.jpg
12   | LOCK (32x32) | lock.jpg | /sd/resource/icon/lock.jpg
```

## 代码修改说明

### 1. ink_screen.cpp 中的修改

#### 添加了 `g_icon_filenames` 数组

```cpp
const char *g_icon_filenames[13] = {
    "icon1.jpg",             // 0: ICON_1
    "icon2.jpg",             // 1: ICON_2
    // ... 其他图标 ...
};
```

**用途**：
- 在web界面导出layout配置时，同时返回icon对应的文件名
- 使web界面能够知道每个icon_index对应的实际文件

#### 更新了 `g_available_icons` 的注释

添加了详细的文件名映射注释，明确每个索引对应的文件名：

```cpp
// 图标索引对应的文件在 components/resource/icon/ 文件夹中
IconInfo g_available_icons[13] = {
    {ZHONGJINGYUAN_3_7_ICON_1, 62, 64},  // 0: ICON_1 -> icon1.jpg
    // ...
};
```

### 2. ink_screen.h 中的修改

添加了外部声明：

```cpp
// ===== 图标文件名映射表 =====
extern const char *g_icon_filenames[13];
```

这允许其他文件（如 web_layout.cpp）访问和使用这个映射表。

### 3. web_layout.cpp 中的修改

#### 添加了文档注释

在文件开头添加了清晰的说明：

```cpp
// ========== 图标索引说明 ==========
// 所有图标操作中的 icon_index 都对应：
// - ink_screen.cpp 中的 g_available_icons 数组（0-12）
// - ink_screen.cpp 中的 g_icon_filenames 数组（0-12）
// - components/resource/icon/ 文件夹中的文件顺序（0-12）
```

#### 导出layout时增加了文件名字段

在 `getCurrentLayoutInfo()` 函数中，每个icon对象现在包含：

```json
{
    "icon_index": 0,
    "filename": "icon1.jpg",           // 新增
    "resource_path": "/sd/resource/icon",  // 新增
    "rel_x": 0.5,
    "rel_y": 0.5,
    // ... 其他字段 ...
}
```

#### 添加了 `exportAvailableIcons()` 函数

这个新函数导出所有可用图标的列表及其对应的文件名：

```json
{
    "icons": [
        {
            "index": 0,
            "filename": "icon1.jpg",
            "width": 62,
            "height": 64,
            "resource_path": "/sd/resource/icon"
        },
        // ... 其他图标 ...
    ],
    "total_count": 13
}
```

**用途**：web界面可以调用此API了解所有可用的图标及其文件信息。

### 4. web_layout.h 中的修改

添加了函数声明：

```cpp
// 导出可用图标列表（包括索引和来自components/resource/icon/文件夹的文件名）
char* exportAvailableIcons();
```

## Web界面集成说明

### 获取可用图标列表

Web界面现在可以通过调用后端API获取所有可用图标的完整信息：

```
GET /api/icons/available  或相似端点
↓
exportAvailableIcons() 函数返回JSON格式的列表
↓
Web界面获得icon_index和filename的完整映射
```

### 添加图标到选中矩形

流程如下：

1. Web界面用户选择一个矩形和一个图标（指定icon_index，0-12）
2. Web界面发送请求到后端，包含icon_index
3. 后端函数处理icon_index，使用 `g_icon_filenames[icon_index]` 确定文件名
4. 屏幕显示时，从 `/sd/resource/icon/{filename}` 加载图标

### layout配置保存与恢复

当用户创建或修改layout时：

1. **保存**：调用 `saveLayoutToConfig()`
   - 保存icon_index到配置文件
   - 同时记录对应的filename和resource_path（用于参考）

2. **恢复**：调用 `parseAndApplyLayout()`
   - 读取配置文件中的icon_index
   - 使用 `g_available_icons[icon_index]` 获取位图数据
   - 使用 `g_icon_filenames[icon_index]` 了解来源文件

## 维护说明

### 添加新图标

如果要添加新的图标，必须**同时修改三个地方**：

1. **components/resource/icon/** 中添加新文件
   - 文件名例如：`icon13.jpg`
   - 位置：第13个位置

2. **ink_screen.cpp 中的 g_icon_filenames[13]**
   - 扩展数组大小（如果需要）
   - 添加文件名条目

3. **ink_screen.cpp 中的 g_available_icons[13]**
   - 扩展数组大小
   - 添加新图标的位图数据指针和尺寸

4. **相关函数和常量**
   - 如果数组大小改变，更新 `sizeof()` 的计算
   - 更新边界检查代码（如果有）
   - 更新本文档的映射表

### 顺序变更

**严禁改变现有图标的顺序！** 因为：
- 用户已保存的layout文件中包含icon_index
- 改变顺序会导致加载时显示错误的图标

如果必须调整顺序，必须：
1. 添加迁移逻辑来转换旧的icon_index
2. 更新所有用户的配置文件
3. 更新此文档

## 相关文件位置

- **数组定义**：`components/grbl_esp32s3/Grbl_Esp32/src/BL_add/ink_screen/ink_screen.cpp` 行 118-148
- **头文件声明**：`components/grbl_esp32s3/Grbl_Esp32/src/BL_add/ink_screen/ink_screen.h`
- **Web集成代码**：`components/grbl_esp32s3/Grbl_Esp32/src/BL_add/ink_screen/web_layout.cpp`，web_layout.h
- **实际图标文件**：`components/resource/icon/`

## 调试技巧

### 验证同步状态

检查以下条件以确保三个对象保持同步：

1. `sizeof(g_icon_filenames)/sizeof(char*)` == `sizeof(g_available_icons)/sizeof(IconInfo)`
2. 每个g_available_icons条目的注释中文件名与g_icon_filenames数组一致
3. components/resource/icon/中的文件数量与数组大小一致
4. 文件按字母排序或其他明确的顺序排列

### 测试icon_index映射

在C代码中添加日志输出：

```cpp
ESP_LOGI(TAG, "Icon index %d -> filename: %s", index, g_icon_filenames[index]);
```

在Web界面中验证返回的filename与期望相符。

## 常见问题

**Q: 为什么需要三个同步的对象？**
A: 因为不同的应用层需要不同的表示：
- C代码需要位图数据（g_available_icons）
- 配置保存需要文件名（g_icon_filenames）
- Web界面需要了解文件来源（/sd/resource/icon/）

**Q: 如果某个索引的文件删除了会怎样？**
A: 会导致屏幕显示出错或崩溃。必须确保：
- components/resource/icon/中的所有文件都存在且可读
- 文件与数组条目完全对应

**Q: 能否动态加载icon吗？**
A: 当前设计假设icon列表是固定的。要支持动态加载需要：
- 修改g_available_icons为动态数组
- 添加运行时加载函数
- 更新web_layout.cpp的相关代码


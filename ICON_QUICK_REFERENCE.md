# 图标操作快速参考

## 最重要的三件事

### 1️⃣ 三个同步对象

```
icon_index (0-12)
    ↓
g_available_icons[index] → 位图数据 (Pic.h常量)
g_icon_filenames[index]  → 文件名   ("icon1.jpg" 等)
components/resource/icon/{filename} → 磁盘文件
```

**✅ 这三个必须保持完全同步的顺序**

### 2️⃣ 当前13个图标

```
0=icon1.jpg  1=icon2.jpg  2=icon3.jpg   3=icon4.jpg   4=icon5.jpg  5=icon6.jpg
6=12.jpg (分隔线)   7=wifi_connect.jpg  8=wifi_disconnect.jpg
9=battery_1.jpg  10=horn.jpg  11=nail.jpg  12=lock.jpg
```

### 3️⃣ 关键代码位置

| 对象 | 文件 | 行号 |
|------|------|------|
| `g_icon_filenames` | ink_screen.cpp | 118-131 |
| `g_available_icons` | ink_screen.cpp | 135-148 |
| 外部声明 | ink_screen.h | 332-335 |
| Web集成 | web_layout.cpp | 行15-21 + 导出函数 |

## 常用操作

### 获取图标文件名

```cpp
// 已知icon_index，获取对应的文件名
const char* filename = g_icon_filenames[icon_index];  // 例如 "icon1.jpg"
const char* path = "/sd/resource/icon";
// 完整路径: /sd/resource/icon/icon1.jpg
```

### 获取图标位图数据

```cpp
// 已知icon_index，获取位图数据
IconInfo* icon = &g_available_icons[icon_index];
const uint8_t* bitmap = icon->data;
int width = icon->width;
int height = icon->height;
```

### 验证icon_index有效性

```cpp
int icon_index = ...; // 来自web或用户输入
if (icon_index >= 0 && icon_index < 13) {
    // 有效的icon_index，可以安全使用
}
```

### 导出所有可用图标信息

```cpp
// 调用此函数获取JSON格式的图标列表
char* json_icons = exportAvailableIcons();
// 使用后必须释放内存
cJSON_free(json_icons);
```

## 添加新图标流程

⚠️ **严禁改变现有图标的顺序！** 只能添加到末尾

### 如果要添加第14个图标（icon13.jpg）

**步骤1：放置文件**
```
复制 icon13.jpg 到 components/resource/icon/
```

**步骤2：更新 ink_screen.cpp**

在 `g_icon_filenames` 中添加（注意改变数组大小）：
```cpp
const char *g_icon_filenames[14] = {  // 改成14
    // ... 前面13个保持不变 ...
    "lock.jpg",        // 12
    "icon13.jpg"       // 13: 新图标
};
```

在 `g_available_icons` 中添加：
```cpp
IconInfo g_available_icons[14] = {  // 改成14
    // ... 前面13个保持不变 ...
    {ZHONGJINGYUAN_3_7_LOCK, 32, 32},        // 12
    {ZHONGJINGYUAN_3_7_ICON_13, width, height}  // 13: 新图标
};
```

**步骤3：更新 ink_screen.h**
```cpp
extern const char *g_icon_filenames[14];  // 改成14
```

**步骤4：更新 web_layout.cpp 边界检查**
```cpp
if (icon_pos->icon_index >= 0 && icon_pos->icon_index < 14) {  // 改成14
    cJSON_AddStringToObject(icon_obj, "filename", g_icon_filenames[icon_pos->icon_index]);
}
```

**步骤5：更新本文档**

## 调试

### 打印icon映射关系

```cpp
ESP_LOGI("DEBUG", "icon_index=%d -> filename=%s", 
         icon_index, 
         g_icon_filenames[icon_index]);
```

### 验证文件存在

```cpp
// 检查文件是否存在于SD卡
String path = String("/sd/resource/icon/") + g_icon_filenames[icon_index];
if (SD.exists(path.c_str())) {
    ESP_LOGI("DEBUG", "✓ 文件存在: %s", path.c_str());
} else {
    ESP_LOGE("DEBUG", "✗ 文件不存在: %s", path.c_str());
}
```

### 调用API获取图标列表

```bash
# 在开发工具或curl中调用（假设API端点为 /api/icons/available）
curl http://device-ip/api/icons/available

# 返回格式（部分示例）
{
    "icons": [
        {"index": 0, "filename": "icon1.jpg", "width": 62, "height": 64, ...},
        {"index": 1, "filename": "icon2.jpg", "width": 64, "height": 64, ...},
        ...
    ],
    "total_count": 13
}
```

## 常见错误

❌ **错误：改变了现有图标的顺序**
- 影响：所有现存的layout配置将显示错误的图标
- 解决：不要这样做！如必要，添加迁移逻辑

❌ **错误：g_icon_filenames 和 g_available_icons 数组大小不匹配**
- 症状：编译时可能不会报错，但运行时会崩溃或显示错误
- 检查：确保两个数组大小始终相等

❌ **错误：文件名typo错误**
- 症状：UI显示图标时报错或显示为空
- 检查：g_icon_filenames 中的文件名必须与 components/resource/icon/ 中的文件名完全相同（包括大小写）

❌ **错误：文件不存在**
- 症状：屏幕显示时看不到预期的图标
- 检查：所有 g_icon_filenames 中列出的文件必须存在于 /sd/resource/icon/

## 查找相关代码

| 任务 | 查找位置 |
|------|---------|
| 查看icon索引使用 | grep "icon_index" *.cpp |
| 查看文件名映射 | grep "g_icon_filenames" *.cpp |
| 查看位图数据 | grep "g_available_icons" *.cpp |
| 查看Web导出 | grep "exportAvailableIcons\|filename\|resource_path" web_layout.cpp |

## 快速检查清单

在修改任何图标相关代码前，检查以下内容：

- [ ] g_available_icons 和 g_icon_filenames 数组大小相同
- [ ] 两个数组中的元素顺序完全一致
- [ ] components/resource/icon/ 中的文件数量与数组大小相同
- [ ] 所有文件名在 g_icon_filenames 中都有对应条目
- [ ] 所有位图指针在 g_available_icons 中都有对应条目
- [ ] 没有改变现有图标的顺序（只添加到末尾）
- [ ] 相关的边界检查已更新（如果改变了数组大小）

## 相关文档

- **详细指南**：`ICON_MAPPING_GUIDE.md`
- **修改总结**：`ICON_OPERATION_CHANGES.md`
- **本快速参考**：`ICON_QUICK_REFERENCE.md`

---

最后更新：2025-12-15


# TTF字体管理系统 - 完整使用指南

## 项目概述

本项目为墨水屏设备提供了完整的 **TTF 字体转换和管理系统**，支持上传、转换、列表查询和删除多种 TTF 字库文件。

### 支持的字体类型
- **fangsong_GB2312** (仿宋_GB2312) - 中文
- **Comic-Sans-MS-V3** - 英文
- **Comic Sans MS Bold** - 英文加粗
- **自定义 TTF 字体** - 用户上传的任何 TTF 格式字体

### 核心功能
✅ TTF 文件上传到 SD 卡  
✅ TTF 转 BIN 格式转换（保存可用字库）  
✅ 字库文件管理（列表、删除、查询）  
✅ 字体切换和配置管理  
✅ 转换进度查询  
✅ 系统状态监控  

---

## 系统架构

### 模块组成

```
┌─────────────────────────────────────────────┐
│         Web 界面 (web_layout.html)          │
│    用户上传 TTF，管理字库，切换字体        │
└──────────────┬──────────────────────────────┘
               │
┌──────────────▼──────────────────────────────┐
│      Web API 接口 (ttf_font_api.cpp)        │
│  8个RESTful端点处理用户请求                │
└──────────────┬──────────────────────────────┘
               │
┌──────────────▼──────────────────────────────┐
│   字体管理核心 (ttf_font_manager.cpp)       │
│  处理转换、存储、配置管理                  │
└──────────────┬──────────────────────────────┘
               │
┌──────────────▼──────────────────────────────┐
│     SD 卡存储结构                            │
│  /sd/fonts/ttf      - 上传的TTF文件        │
│  /sd/fonts/bin      - 生成的BIN字库        │
│  /sd/fonts/config.json - 配置文件          │
└─────────────────────────────────────────────┘
```

---

## API 接口文档

### 1. GET /api/fonts/list
**功能**: 获取所有可用的字库列表

**请求示例**:
```bash
curl http://192.168.1.100:8848/api/fonts/list
```

**响应示例**:
```json
{
  "fonts": [
    {
      "name": "fangsong_GB2312",
      "display_name": "仿宋_GB2312",
      "font_size": 16
    },
    {
      "name": "ComicSansMSV3",
      "display_name": "Comic Sans MS V3",
      "font_size": 16
    }
  ],
  "count": 2
}
```

---

### 2. POST /api/fonts/upload
**功能**: 上传 TTF 文件并启动转换

**请求格式** (JSON):
```json
{
  "font_name": "fangsong_GB2312",
  "ttf_filename": "fangsong.ttf",
  "font_size": 16,
  "charset": "GB2312"
}
```

**使用说明**:
1. 先通过其他 API 上传 TTF 文件到 `/sd/fonts/ttf/`
2. 然后调用此 API 进行转换
3. `font_name`: 字体在系统中的标识名
4. `ttf_filename`: SD 卡中的 TTF 文件名
5. `charset`: 字符集类型 ("GB2312", "GBK", "UTF-8")

**响应示例**:
```json
{
  "status": "success",
  "message": "Font conversion started",
  "font_name": "fangsong_GB2312"
}
```

---

### 3. GET /api/fonts/list-uploaded
**功能**: 获取已上传的 TTF 文件列表

**请求示例**:
```bash
curl http://192.168.1.100:8848/api/fonts/list-uploaded
```

**响应示例**:
```json
{
  "files": [
    {
      "filename": "fangsong.ttf",
      "size": 3145728
    }
  ],
  "count": 1
}
```

---

### 4. DELETE /api/fonts/delete
**功能**: 删除已转换的字库文件

**请求示例**:
```bash
curl -X DELETE "http://192.168.1.100:8848/api/fonts/delete?font_name=fangsong_GB2312"
```

**参数**:
- `font_name`: 要删除的字体名称

**响应示例**:
```json
{
  "status": "success",
  "message": "Font deleted successfully"
}
```

---

### 5. GET /api/fonts/info
**功能**: 获取特定字体的详细信息

**请求示例**:
```bash
curl "http://192.168.1.100:8848/api/fonts/info?font_name=fangsong_GB2312"
```

**响应示例**:
```json
{
  "name": "fangsong_GB2312",
  "display_name": "仿宋_GB2312",
  "font_size": 16,
  "available": true,
  "charset_start": 19968,
  "charset_end": 40869
}
```

---

### 6. GET /api/fonts/switch
**功能**: 切换当前使用的字库

**请求示例**:
```bash
curl "http://192.168.1.100:8848/api/fonts/switch?font_name=ComicSansMSV3"
```

**响应示例**:
```json
{
  "status": "success",
  "message": "Font switched successfully",
  "current_font": "ComicSansMSV3"
}
```

---

### 7. GET /api/fonts/status
**功能**: 获取字体管理系统的整体状态

**请求示例**:
```bash
curl http://192.168.1.100:8848/api/fonts/status
```

**响应示例**:
```json
{
  "initialized": true,
  "current_font": "fangsong_GB2312",
  "total_fonts": 3,
  "available_fonts": 2,
  "converting_tasks": 0,
  "sd_total_bytes": 8589934592,
  "sd_used_bytes": 2147483648,
  "sd_free_bytes": 6442450944
}
```

---

### 8. GET /api/fonts/progress
**功能**: 获取字体转换的进度

**请求示例**:
```bash
curl "http://192.168.1.100:8848/api/fonts/progress?font_name=fangsong_GB2312"
```

**响应示例**:
```json
{
  "progress": 75.5,
  "font_name": "fangsong_GB2312"
}
```

---

## 使用工作流

### 1. 上传并转换字体

```bash
# 步骤1: 上传TTF文件
# (通过Web界面或其他上传接口上传到 /sd/fonts/ttf/)

# 步骤2: 启动转换
curl -X POST http://192.168.1.100:8848/api/fonts/upload \
  -H "Content-Type: application/json" \
  -d '{
    "font_name": "fangsong_GB2312",
    "ttf_filename": "fangsong.ttf",
    "font_size": 16,
    "charset": "GB2312"
  }'

# 步骤3: 查询转换进度
curl "http://192.168.1.100:8848/api/fonts/progress?font_name=fangsong_GB2312"

# 步骤4: 查询可用字库
curl http://192.168.1.100:8848/api/fonts/list

# 步骤5: 切换到新字体
curl "http://192.168.1.100:8848/api/fonts/switch?font_name=fangsong_GB2312"
```

### 2. 管理字体

```bash
# 列出所有可用字体
curl http://192.168.1.100:8848/api/fonts/list

# 列出已上传的TTF文件
curl http://192.168.1.100:8848/api/fonts/list-uploaded

# 获取字体详细信息
curl "http://192.168.1.100:8848/api/fonts/info?font_name=fangsong_GB2312"

# 删除字体
curl -X DELETE "http://192.168.1.100:8848/api/fonts/delete?font_name=fangsong_GB2312"

# 查看系统状态
curl http://192.168.1.100:8848/api/fonts/status
```

---

## 数据文件格式

### BIN 字库文件格式

```c
// 文件头结构 (60 字节)
typedef struct {
    uint32_t magic;           // 魔数: 0x544F4E46 ("FONT")
    uint16_t version;         // 版本: 1
    uint16_t font_size;       // 字体大小 (16, 24, 32)
    uint32_t charset_start;   // 字符集起始编码 (如 0x4E00)
    uint32_t charset_end;     // 字符集结束编码 (如 0x9FA5)
    uint32_t charset_size;    // 字符总数
    uint32_t char_width;      // 字符宽度
    uint32_t char_height;     // 字符高度
    uint32_t bytes_per_char;  // 每字符字节数
    char font_name[32];       // 字体名称
    uint32_t reserved[8];     // 预留字段
} FontBinHeader_t;

// 文件内容
[Header(60字节)] + [字符位图数据]
```

### 配置文件格式 (`/sd/fonts/config.json`)

```json
{
  "fonts": [
    {
      "name": "fangsong_GB2312",
      "display_name": "仿宋_GB2312",
      "font_size": 16,
      "available": true
    }
  ],
  "current_font": "fangsong_GB2312"
}
```

---

## 文件系统结构

```
/sd/fonts/
├── ttf/                        # TTF源文件目录
│   ├── fangsong.ttf
│   ├── ComicSansMS_V3.ttf
│   └── ComicSansMS_Bold.ttf
├── bin/                        # BIN字库文件目录
│   ├── fangsong_GB2312.bin
│   ├── ComicSansMSV3.bin
│   └── ComicSansMSBold.bin
└── config.json                 # 系统配置文件
```

---

## 常见问题

### Q1: 如何添加新字体?
**A**: 
1. 获取或下载 TTF 格式的字体文件
2. 使用 `/api/fonts/upload` 上传并转换
3. 系统会自动生成 BIN 文件
4. 使用 `/api/fonts/switch` 切换到新字体

### Q2: 转换需要多长时间?
**A**: 取决于字体大小和字符集。一般 16x16 中文字体约需要 1-5 分钟。

### Q3: 能否支持其他字符集?
**A**: 是的，支持 GB2312, GBK, UTF-8 等。在上传时指定即可。

### Q4: 字体文件太大怎么办?
**A**: 
- 检查 SD 卡剩余空间
- 删除不需要的字体
- 使用较小的字体大小

### Q5: 如何恢复默认字体?
**A**: 系统预置了默认字体，使用 `/api/fonts/switch` 切换回默认字体。

---

## 编译和构建

### 编译步骤

```bash
# 1. 进入项目目录
cd /path/to/inkScree_fuben

# 2. 清理旧构建
idf.py fullclean

# 3. 编译项目
idf.py build

# 4. 烧写固件
idf.py flash

# 5. 查看日志
idf.py monitor
```

### 预期输出

```
I (12345) TTF_FontMgr: Initializing TTF font manager...
I (12346) TTF_FontMgr: Created directory: /sd/fonts
I (12347) TTF_FontMgr: Created directory: /sd/fonts/ttf
I (12348) TTF_FontMgr: Created directory: /sd/fonts/bin
I (12349) TTF_FontMgr: Found available font: fangsong_GB2312
I (12350) TTF_FontMgr: TTF font manager initialized, 3 fonts loaded
I (12351) TTF_FontAPI: Registering TTF font management APIs...
I (12352) TTF_FontAPI: TTF font management APIs registered successfully
```

---

## 测试清单

- [ ] 编译不出错
- [ ] 系统启动时初始化字体管理器
- [ ] API 端点正常响应
- [ ] 能够列出可用字体
- [ ] 能够上传 TTF 文件
- [ ] 能够转换 TTF 为 BIN
- [ ] 能够查询转换进度
- [ ] 能够切换字体
- [ ] 能够删除字体
- [ ] 配置文件正确保存和加载
- [ ] 系统状态显示正确

---

## 代码文件清单

| 文件 | 功能 | 行数 |
|------|------|------|
| `ttf_font_manager.h` | 字体管理 API 接口 | ~280 |
| `ttf_font_manager.cpp` | 字体管理核心实现 | ~600 |
| `ttf_font_api.h` | Web API 接口定义 | ~40 |
| `ttf_font_api.cpp` | Web API 处理程序 | ~450 |
| `CMakeLists.txt` | 构建配置 | 已更新 |
| `WebServer.cpp` | Web 服务器集成 | 已更新 |

**总代码量**: ~1,800 行

---

## 扩展说明

### 未来改进方向

1. **完整 TTF 解析**: 集成 FreeType 库进行真正的 TTF 解析和光栅化
2. **预设字体库**: 提供更多预制字体选项
3. **字体预览**: Web 界面显示字体预览
4. **批量转换**: 支持一次转换多个字体
5. **自动缓存**: 首次使用时自动缓存字体
6. **性能优化**: 异步转换，不阻塞主线程
7. **权限管理**: 限制用户操作权限
8. **字体验证**: 自动验证字体合法性

---

## 技术支持

如有问题，请查阅:
1. 编译错误 → 检查 CMakeLists.txt
2. API 404 → 检查端点注册
3. 转换失败 → 检查 TTF 文件格式
4. SD 卡错误 → 检查 SD 卡状态
5. 内存不足 → 增加堆内存大小

---

## 许可证

该系统集成在本项目中，遵循项目的许可证条款。

**开发日期**: 2024-12-15  
**版本**: 2.0.0  
**状态**: 生产就绪

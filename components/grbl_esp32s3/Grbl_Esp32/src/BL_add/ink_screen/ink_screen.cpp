#include "ink_screen.h"
#include "./SPI_Init.h"
#include "esp_timer.h"

extern "C" {
	#include "./EPD.h"
#include "./EPD_GUI.h"
#include "./EPD_Font.h"
#include "./Pic.h"
}

// 全局变量：当前选中的图标索引
int g_selected_icon_index = -1;
uint8_t inkScreenTestFlag = 0; 
uint8_t inkScreenTestFlagTwo = 0;
uint8_t interfaceIndex=1;
IconPosition g_icon_positions[6] = {0};
// const unsigned char *gamePeople[] = {game_1, game_2, game_3, game_4, game_5, game_6, game_7};
#define GAME_PEOPLE_COUNT (sizeof(gamePeople) / sizeof(gamePeople[0]))
LastUnderlineInfo g_last_underline = {0, 0, 0, 0, BLACK, false};
const char *TAG = "ink_screen.cpp";
static TaskHandle_t _eventTaskHandle = NULL;
uint8_t ImageBW[12480];
WordEntry entry;
uint8_t *showPrompt=nullptr;
IconPosition selected_icon = {0, 0, 0, 0, false};
// 全局变量记录上次显示信息
static char lastDisplayedText[256] = {0};
static uint16_t lastTextLength = 0;
static uint16_t lastX = 65;
static uint16_t lastY = 120;
static uint16_t lastSize = 0;  // 添加字体大小记录

static esp_timer_handle_t sleep_timer;
static bool is_sleep_mode = false;
static uint32_t last_activity_time = 0;
static const uint32_t SLEEP_TIMEOUT_MS = 25000; // 15秒
// 休眠模式显示的数据
static WordEntry sleep_mode_entry;
static bool has_sleep_data = false;
InkScreenSize setInkScreenSize;
TimerHandle_t inkScreenDebounceTimer = NULL;
void showChineseString(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
    uint16_t currentX = x;
    uint8_t* p = s;
    
    // 直接计算字符数（假设全是UTF-8中文，每个3字节）
    uint16_t char_count = strlen((char*)s) / 3;
    
    ESP_LOGI(TAG, "显示 %d 个汉字，屏幕高度: %d", char_count, EPD_H);
    
    for(uint16_t i = 0; i < char_count; i++)
    {
        // 检查是否超出屏幕高度
        if (y + sizey > EPD_W) {
            ESP_LOGW(TAG, "超出屏幕高度，停止显示。已显示 %d 个汉字", i);
            break;
        }
        
        // 显示单个汉字
        EPD_ShowChinese_UTF8_Single(currentX, y, p, sizey, color);
        
        // 移动到下一个字符
        p += 3;
        currentX += sizey + 3;
        
        // 换行检查
        if (currentX + sizey > EPD_H) {
            currentX = x;
            y += sizey + 3;
            ESP_LOGI(TAG,"#######换行");
            // 换行后再次检查高度
            if (y + sizey > EPD_W) {
                ESP_LOGW(TAG, "换行后超出屏幕高度，停止显示。已显示 %d 个汉字", i + 1);
                break;
            }
        }
    }
    
    ESP_LOGI(TAG, "显示完成");
}

// 清除上次显示的下划线
void clearLastDisplay() {
    if (!g_last_underline.has_underline) {
        return;  // 没有下划线需要清除
    }
    
    ESP_LOGI(TAG, "清除上次下划线: 位置(%d,%d), 尺寸(%dx%d)", 
            g_last_underline.x, g_last_underline.y, 
            g_last_underline.width, g_last_underline.height);
    
    // 计算下划线位置
    uint16_t underline_y = g_last_underline.y + g_last_underline.height + 3;
    uint16_t underline_length = g_last_underline.width;
    uint16_t underline_thickness = 2;
    
    // 使用白色覆盖下划线（清除）
    for (int i = 0; i < underline_thickness; i++) {
        EPD_DrawLine(g_last_underline.x, 
                    underline_y + i, 
                    g_last_underline.x + underline_length - 1, 
                    underline_y + i, 
                    WHITE);  // 使用白色覆盖
    }
    
    // 重置记录
    g_last_underline.has_underline = false;
    
    ESP_LOGI(TAG, "下划线清除完成");
}

// 记录下划线信息
void recordUnderlineInfo(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    g_last_underline.x = x;
    g_last_underline.y = y;
    g_last_underline.width = width;
    g_last_underline.height = height;
    g_last_underline.color = color;
    g_last_underline.has_underline = true;
    
    ESP_LOGI(TAG, "记录下划线信息: 位置(%d,%d), 尺寸(%dx%d)", x, y, width, height);
}

// 绘制下划线（使用实际图片尺寸）
void drawUnderlineForIcon(uint16_t x, uint16_t y, uint16_t icon_width, uint16_t icon_height, uint16_t color = BLACK) {
    // 下划线参数：使用实际图标宽度
    uint16_t underline_y = y + icon_height + 3;  // 图标下方3像素
    uint16_t underline_length = icon_width;      // 使用图标实际宽度
    uint16_t underline_thickness = 2;            // 线粗2像素
    
    // 绘制下划线
    for (int i = 0; i < underline_thickness; i++) {
        EPD_DrawLine(x, 
                    underline_y + i, 
                    x + underline_length - 1, 
                    underline_y + i, 
                    color);
    }
    
    ESP_LOGI("UNDERLINE", "绘制下划线: 图标位置(%d,%d), 实际尺寸(%dx%d), 下划线长度:%d", 
            x, y, icon_width, icon_height, underline_length);
}

// 扩展版本：自动记录和清除
void drawUnderlineForIconEx(uint16_t x, uint16_t y, uint16_t icon_width, uint16_t icon_height, uint16_t color) {
    // 先清除上次的下划线
    clearLastDisplay();
    
    // 绘制下划线（使用实际图片尺寸）
    drawUnderlineForIcon(x, y, icon_width, icon_height, color);
    
    // 记录这次的下划线信息
    recordUnderlineInfo(x, y, icon_width, icon_height, color);
}

// 检查电池图标是否有效的函数
bool isValidBatteryIcon(const uint8_t* icon_data) {
    if (icon_data == NULL) {
        ESP_LOGE("ICON_CHECK", "电池图标数据为空");
        return false;
    }
    
    // 简单检查：确保不是WiFi图标
    if (icon_data == ZHONGJINGYUAN_3_7_WIFI_DISCONNECT) {
        ESP_LOGE("ICON_CHECK", "电池图标指向了WiFi图标");
        return false;
    }
    
    // 可以添加更详细的检查
    // 例如检查图标数据的前几个字节
    return true;
}
// 改进的状态栏绘制函数
void drawStatusBar() {
    #define STATUS_BAR_MARGIN 1
    #define STATUS_BAR_HEIGHT 40
    
    int status_y = STATUS_BAR_MARGIN;
    
    // 先清除整个状态栏区域
    EPD_DrawRectangle(STATUS_BAR_MARGIN, STATUS_BAR_MARGIN, 
                 setInkScreenSize.screenWidth * STATUS_BAR_MARGIN, STATUS_BAR_HEIGHT, WHITE,1);
    
    // 最右侧：WiFi图标（32x32）
    int wifi_x = setInkScreenSize.screenWidth - STATUS_BAR_MARGIN - 32;
    EPD_ShowPicture(wifi_x, status_y, 32, 32, ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK);
    
    // WiFi左侧：电池图标（36x24）
    int battery_x = wifi_x - 36 - 5;  // 5像素间距
    
    // 检查电池图标数据是否有效
    if (isValidBatteryIcon(ZHONGJINGYUAN_3_7_BATTERY_1)) {
        EPD_ShowPicture(battery_x, status_y, 36, 24, ZHONGJINGYUAN_3_7_BATTERY_1, BLACK);
    } else {
        // 如果图标无效，绘制一个简单的矩形作为占位符
        EPD_DrawRectangle(battery_x, status_y, battery_x + 36, status_y + 24, BLACK,1);
        EPD_DrawLine(battery_x + 34, status_y + 6, battery_x + 34, status_y + 18, BLACK); // 电池正极
        ESP_LOGW("STATUS", "使用占位符绘制电池图标");
    }
    
    ESP_LOGI("STATUS_BAR", "绘制状态栏: WiFi图标(%d,%d), 电池图标(%d,%d)", 
            wifi_x, status_y, battery_x, status_y);
}

// 在指定图标位置绘制下划线
void drawUnderlineAtIcon(int icon_index, uint16_t color = BLACK) {
    if (icon_index < 0 || icon_index >= 6) {
        ESP_LOGE(TAG, "无效的图标索引: %d", icon_index);
        return;
    }
    
    IconPosition* icon = &g_icon_positions[icon_index];
    
    // 检查图标位置是否有效
    if (icon->width == 0 || icon->height == 0) {
        ESP_LOGE(TAG, "图标%d位置未初始化或未显示", icon_index);
        return;
    }
    
    ESP_LOGI(TAG, "在图标%d位置绘制下划线: 位置(%d,%d), 实际尺寸(%dx%d), 颜色:%d", 
            icon_index, icon->x, icon->y, icon->width, icon->height, color);
    
    // 初始化显示
    EPD_FastInit();
    EPD_Display_Clear();  // 全屏清除
    EPD_Update();
    delay_ms(100);
    EPD_PartInit();
    
    // 清除上次显示的内容
    clearLastDisplay();
    
    // ==================== 重新显示状态栏图标 ====================
    drawStatusBar();
    
    // 使用扩展函数绘制下划线（会自动清除上次的）
    drawUnderlineForIconEx(icon->x, icon->y, icon->width, icon->height, color);
    
    // 更新显示
    EPD_Display(ImageBW);
    EPD_Update();
    EPD_DeepSleep();
    delay_ms(1000);
    
    ESP_LOGI(TAG, "图标%d下划线绘制完成", icon_index);
}


// 清除所有下划线（完全清除）
void clearAllUnderlines() {
    ESP_LOGI(TAG, "清除所有下划线");
    
    // 如果没有下划线记录，直接返回
    if (!g_last_underline.has_underline) {
        return;
    }
    
    // 清除当前记录的下划线
    clearLastDisplay();
    
    // 额外清除可能存在的其他下划线
    // 这里可以根据需要扩展，清除多个可能的位置
    
    ESP_LOGI(TAG, "所有下划线已清除");
}

// 检查是否需要清除下划线的辅助函数
bool shouldClearUnderline(uint16_t new_x, uint16_t new_y, uint16_t new_width) {
    if (!g_last_underline.has_underline) {
        return false;
    }
    
    // 如果新的下划线位置和上次不同，需要清除上次的
    bool different_position = (new_x != g_last_underline.x) || 
                             (new_y != g_last_underline.y) ||
                             (new_width != g_last_underline.width);
    
    if (different_position) {
        ESP_LOGI(TAG, "下划线位置变化，需要清除上次的下划线");
        return true;
    }
    
    return false;
}

void updateDisplayWithWifiIcon()
{
    static bool lastWifiStatus = false;
    bool currentWifiStatus = WebUI::wifi_config.isConnectWifi();
    static bool isFrist = 0;

	if(isFrist == 0 && lastWifiStatus==currentWifiStatus) {
		if(currentWifiStatus) {
			EPD_ShowPicture(340, 1, 32, 32, ZHONGJINGYUAN_3_7_WIFI_CONNECT , BLACK);
		} else {
			 EPD_ShowPicture(340, 1, 32, 32, ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK); 
		}
		isFrist = 1;
	}
    // 只在状态改变时更新显示
    if(lastWifiStatus != currentWifiStatus) {
		EPD_FastInit();
		EPD_Display_Clear();
		EPD_Update();  //局刷之前先对E-Paper进行清屏操作
		delay_ms(100);
		EPD_PartInit();
        clearDisplayArea(5,5,340,30);
	    clearDisplayArea(60,40,EPD_H,EPD_W);
		EPD_ShowPicture(380,2,36,24,ZHONGJINGYUAN_3_7_BATTERY_1,BLACK);
		EPD_ShowPicture(60,40,62,64,ZHONGJINGYUAN_3_7_ICON_1,BLACK);
		EPD_ShowPicture(180,40,64,64,ZHONGJINGYUAN_3_7_ICON_2,BLACK);
		EPD_ShowPicture(300,40,86,64,ZHONGJINGYUAN_3_7_ICON_3,BLACK);
		EPD_ShowPicture(60,140,71,56,ZHONGJINGYUAN_3_7_ICON_4,BLACK);
		EPD_ShowPicture(180,140,76,56,ZHONGJINGYUAN_3_7_ICON_5,BLACK);
		EPD_ShowPicture(300,140,94,64,ZHONGJINGYUAN_3_7_ICON_6,BLACK);
        // 清除原图标区域
        EPD_DrawRectangle(340, 1, 340+32, 1+32, WHITE, 1);
        
        // 根据 WiFi 连接状态选择图标
        if(currentWifiStatus) {
           EPD_ShowPicture(340, 1, 32, 32, ZHONGJINGYUAN_3_7_WIFI_CONNECT , BLACK);  // WiFi 已连接
            ESP_LOGI("WIFI222222222222", "WiFi状态: 已连接");
        } else {
           EPD_ShowPicture(340, 1, 32, 32, ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK); // WiFi 未连接
            ESP_LOGI("WIFI111111111111111", "WiFi状态: 未连接");
        }
        
        lastWifiStatus = currentWifiStatus;

		EPD_Display(ImageBW);
		EPD_Update();
		EPD_DeepSleep();
	//	delay_ms(1000);
    }
}

void clearAllPictureAreas() {
    for (int i = 0; i < PICTURE_AREA_COUNT; i++) {
        if (picture_areas[i].displayed) {
            EPD_DrawRectangle(picture_areas[i].x, 
                            picture_areas[i].y, 
                            picture_areas[i].x + picture_areas[i].width, 
                            picture_areas[i].y + picture_areas[i].height, 
                            WHITE, 1);
            picture_areas[i].displayed = false;
            
            ESP_LOGI(TAG, "清除图片区域%d: 位置(%d,%d), 大小(%dx%d)", 
                     i, picture_areas[i].x, picture_areas[i].y, 
                     picture_areas[i].width, picture_areas[i].height);
        }
    }
}

// 清除指定图片区域
void clearPictureArea(uint8_t area_index) {
    if (area_index < PICTURE_AREA_COUNT && picture_areas[area_index].displayed) {
        EPD_DrawRectangle(picture_areas[area_index].x, 
                        picture_areas[area_index].y, 
                        picture_areas[area_index].x + picture_areas[area_index].width, 
                        picture_areas[area_index].y + picture_areas[area_index].height, 
                        WHITE, 1);
        picture_areas[area_index].displayed = false;
        
        ESP_LOGI(TAG, "清除指定图片区域%d", area_index);
    }
}

// 标记图片区域为已显示
void markPictureAreaDisplayed(uint8_t area_index) {
    if (area_index < PICTURE_AREA_COUNT) {
        picture_areas[area_index].displayed = true;
    }
}

// 显示图片并自动标记区域
void EPD_ShowPictureWithMark(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *image, uint16_t color) {
    EPD_ShowPicture(x, y, width, height, image, color);
    
    // 查找并标记对应的显示区域
    for (int i = 0; i < PICTURE_AREA_COUNT; i++) {
        if (picture_areas[i].x == x && picture_areas[i].y == y &&
            picture_areas[i].width == width && picture_areas[i].height == height) {
            picture_areas[i].displayed = true;
            break;
        }
    }
}

// 清除指定坐标范围的区域（通用方法）
void clearDisplayArea(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    // 确保坐标在屏幕范围内
    if (start_x > EPD_H) start_x = EPD_H;
    if (start_y > EPD_W) start_y = EPD_W;
    if (end_x > EPD_H) end_x = EPD_H;
    if (end_y > EPD_W) end_y = EPD_W;
    
    EPD_DrawRectangle(start_x, start_y, end_x, end_y, WHITE, 1);
    
    ESP_LOGI(TAG, "清除显示区域: (%d,%d)到(%d,%d)", start_x, start_y, end_x, end_y);
}

// 清除特定图标区域
void clearSpecificIcons(uint8_t *icon_indices, uint8_t count) {
    for (int i = 0; i < count; i++) {
        clearPictureArea(icon_indices[i]);
    }
    
    // 立即刷新显示
    EPD_Display(ImageBW);
    EPD_Update();
}

void updateDisplayWithMain() {
    // 图标数据
    typedef struct {
        const uint8_t* data;
        int width;
        int height;
    } IconInfo;
    
    // 所有图标信息
    IconInfo icons[] = {
        {ZHONGJINGYUAN_3_7_ICON_1, 62, 64},
        {ZHONGJINGYUAN_3_7_ICON_2, 64, 64},
        {ZHONGJINGYUAN_3_7_ICON_3, 86, 64},
        {ZHONGJINGYUAN_3_7_ICON_4, 71, 56},
        {ZHONGJINGYUAN_3_7_ICON_5, 76, 56},
        {ZHONGJINGYUAN_3_7_ICON_6, 94, 64}
    };
    
    int icon_count = sizeof(icons) / sizeof(IconInfo);
    
    // 布局配置
    typedef struct {
        int grid_cols;           // 每行图标个数（自动计算）
        int grid_rows;           // 行数（自动计算）
        int cell_width;          // 单元格宽度
        int cell_height;         // 单元格高度
        int icon_spacing_x;      // 图标水平间距
        int icon_spacing_y;      // 图标垂直间距
        int status_bar_height;   // 状态栏高度
        int margin;              // 边距
    } LayoutConfig;
    
    LayoutConfig layout = {
        .grid_cols = 0,          // 自动计算
        .grid_rows = 0,          // 自动计算
        .cell_width = 0,         // 自动计算
        .cell_height = 80,       // 固定单元格高度
        .icon_spacing_x = 10,    // 最小水平间距
        .icon_spacing_y = 15,    // 最小垂直间距
        .status_bar_height = 35,
        .margin = 5
    };
    
    // 初始化显示
    EPD_FastInit();
    EPD_Display_Clear();
    EPD_Update();
    delay_ms(100);
    EPD_PartInit();
    
    // ==================== 自适应布局计算 ====================
    
    // 计算可用区域
    int available_width = setInkScreenSize.screenWidth - 2 * layout.margin;
    int available_height = setInkScreenSize.screenHeigt - 2 * layout.margin - layout.status_bar_height;
    
    // 自动计算每行最大图标数量
    // 基于平均图标宽度和最小间距
    int avg_icon_width = 0;
    for (int i = 0; i < icon_count; i++) {
        avg_icon_width += icons[i].width;
    }
    avg_icon_width /= icon_count;
    
    // 计算每行最大可能的图标数量
    int max_cols = (available_width + layout.icon_spacing_x) / 
                   (avg_icon_width + layout.icon_spacing_x);
    max_cols = (max_cols < 1) ? 1 : max_cols;
    max_cols = (max_cols > 6) ? 6 : max_cols;  // 最多6个
    
    // 计算实际每行图标数量（尽量接近正方形布局）
    layout.grid_cols = max_cols;
    layout.grid_rows = (icon_count + layout.grid_cols - 1) / layout.grid_cols;
    
    // 如果行数太多，增加每行图标数量
    while (layout.grid_rows > 3 && layout.grid_cols < 6) {
        layout.grid_cols++;
        layout.grid_rows = (icon_count + layout.grid_cols - 1) / layout.grid_cols;
    }
    
    // 计算单元格尺寸（基于可用空间）
    layout.cell_width = (available_width - (layout.grid_cols + 1) * layout.icon_spacing_x) / 
                       layout.grid_cols;
    
    // 调整垂直间距，使图标垂直居中
    layout.cell_height = (available_height - (layout.grid_rows + 1) * layout.icon_spacing_y) / 
                        layout.grid_rows;
    
    // 确保单元格尺寸合理
    if (layout.cell_width < 40) layout.cell_width = 40;
    if (layout.cell_height < 40) layout.cell_height = 40;
    
    // 如果单元格太小，减少列数
    while ((layout.cell_width < 50 || layout.cell_height < 50) && layout.grid_cols > 1) {
        layout.grid_cols--;
        layout.grid_rows = (icon_count + layout.grid_cols - 1) / layout.grid_cols;
        
        layout.cell_width = (available_width - (layout.grid_cols + 1) * layout.icon_spacing_x) / 
                           layout.grid_cols;
        layout.cell_height = (available_height - (layout.grid_rows + 1) * layout.icon_spacing_y) / 
                            layout.grid_rows;
    }
    
    // 最终计算间距
    layout.icon_spacing_x = (available_width - layout.grid_cols * layout.cell_width) / 
                           (layout.grid_cols + 1);
    layout.icon_spacing_y = (available_height - layout.grid_rows * layout.cell_height) / 
                           (layout.grid_rows + 1);
    
    // 确保最小间距
    if (layout.icon_spacing_x < 5) layout.icon_spacing_x = 5;
    if (layout.icon_spacing_y < 5) layout.icon_spacing_y = 5;
    
    // ==================== 显示布局信息 ====================
    ESP_LOGI("LAYOUT", "屏幕尺寸: %dx%d", setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
    ESP_LOGI("LAYOUT", "布局: %d列 x %d行", layout.grid_cols, layout.grid_rows);
    ESP_LOGI("LAYOUT", "单元格: %dx%d, 间距: %dx%d", 
            layout.cell_width, layout.cell_height, 
            layout.icon_spacing_x, layout.icon_spacing_y);
    
    // 清除区域
    clearDisplayArea(layout.margin, layout.margin, 
                     setInkScreenSize.screenWidth - layout.margin, layout.status_bar_height);
    
    clearDisplayArea(layout.margin, layout.status_bar_height + layout.margin, 
                     setInkScreenSize.screenWidth - layout.margin, setInkScreenSize.screenHeigt - layout.margin);

    // ==================== 状态栏图标（修正） ====================
    int status_y = layout.margin;
    
    // 最右侧：WiFi图标（32x32）
    int wifi_x = setInkScreenSize.screenWidth - layout.margin - 32;
    EPD_ShowPicture(wifi_x, status_y, 32, 32, ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK);
    
    // WiFi左侧：电池图标（36x24）
    int battery_x = wifi_x - 36 - 5;  // 5像素间距
    // 根据实际电量选择合适的电池图标
    #ifdef BATTERY_LEVEL
        // 根据电量选择不同等级的电池图标
        uint8_t* battery_icon = NULL;
        if (BATTERY_LEVEL >= 80) {
            battery_icon = ZHONGJINGYUAN_3_7_BATTERY_4;  // 满电
        } else if (BATTERY_LEVEL >= 60) {
            battery_icon = ZHONGJINGYUAN_3_7_BATTERY_3;  // 高电量
        } else if (BATTERY_LEVEL >= 40) {
            battery_icon = ZHONGJINGYUAN_3_7_BATTERY_2;  // 中电量
        } else if (BATTERY_LEVEL >= 20) {
            battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;  // 低电量
        } else {
            battery_icon = ZHONGJINGYUAN_3_7_BATTERY_0;  // 空电量
        }
        EPD_ShowPicture(battery_x, status_y, 36, 24, battery_icon, BLACK);
    #else
        // 如果没有电量信息，使用默认电池图标
        EPD_ShowPicture(battery_x, status_y, 36, 24, ZHONGJINGYUAN_3_7_BATTERY_1, BLACK);
    #endif
    
    ESP_LOGI("STATUS_BAR", "状态栏: WiFi图标(%d,%d), 电池图标(%d,%d)", 
            wifi_x, status_y, battery_x, status_y);
    
    // ==================== 初始化图标位置数组 ====================
    for (int i = 0; i < 6; i++) {
        g_icon_positions[i].x = 0;
        g_icon_positions[i].y = 0;
        g_icon_positions[i].width = 0;
        g_icon_positions[i].height = 0;
        g_icon_positions[i].selected = false;
    }
    
    // ==================== 显示网格图标并记录位置 ====================
    
    for (int i = 0; i < icon_count; i++) {
        // 计算图标在网格中的位置
        int row = i / layout.grid_cols;
        int col = i % layout.grid_cols;
        
        // 如果超过最大行数，停止显示
        if (row >= layout.grid_rows) {
            ESP_LOGW("LAYOUT", "图标%d超出网格范围，停止显示", i);
            break;
        }
        
        // 计算单元格位置
        int cell_x = layout.margin + layout.icon_spacing_x + 
                    col * (layout.cell_width + layout.icon_spacing_x);
        int cell_y = layout.status_bar_height + layout.margin + layout.icon_spacing_y + 
                    row * (layout.cell_height + layout.icon_spacing_y);
        
        // 计算图标居中位置
        int icon_x = cell_x + (layout.cell_width - icons[i].width) / 2;
        int icon_y = cell_y + (layout.cell_height - icons[i].height) / 2;
        
        // 边界检查
        if (icon_x < 0 || icon_x + icons[i].width > setInkScreenSize.screenWidth ||
            icon_y < 0 || icon_y + icons[i].height > setInkScreenSize.screenHeigt) {
            ESP_LOGE("LAYOUT", "图标%d位置超出屏幕范围: (%d,%d)", i, icon_x, icon_y);
            continue;
        }
        
        // 记录图标位置信息到全局数组
        g_icon_positions[i].x = icon_x;
        g_icon_positions[i].y = icon_y;
        g_icon_positions[i].width = icons[i].width;    // 记录实际图片宽度
        g_icon_positions[i].height = icons[i].height;  // 记录实际图片高度
        g_icon_positions[i].selected = false;  // 默认未选中
        
        // 显示图标
        EPD_ShowPicture(icon_x, icon_y, icons[i].width, icons[i].height, icons[i].data, BLACK);
        
        ESP_LOGI("ICON_POS", "图标%d位置已记录: 坐标(%d,%d), 实际尺寸(%dx%d)", 
                i, icon_x, icon_y, icons[i].width, icons[i].height);
    }
    
    // ==================== 显示选中的图标下划线（如果有） ====================
    if (g_selected_icon_index >= 0 && g_selected_icon_index < icon_count) {
        IconPosition* selected = &g_icon_positions[g_selected_icon_index];
        if (selected->width > 0 && selected->height > 0) {
            // 使用实际图片尺寸绘制下划线
            drawUnderlineForIconEx(selected->x, selected->y, 
                                 selected->width, selected->height, BLACK);
            
            ESP_LOGI("ICON_POS", "显示选中图标%d的下划线: 使用实际图片尺寸(%dx%d)", 
                    g_selected_icon_index, selected->width, selected->height);
        }
    }
    
    #define DEBUG_LAYOUT 1
    // ==================== 显示网格线（调试用，可选） ====================
    #ifdef DEBUG_LAYOUT
    for (int row = 0; row <= layout.grid_rows; row++) {
        int y = layout.status_bar_height + layout.margin + layout.icon_spacing_y + 
               row * (layout.cell_height + layout.icon_spacing_y);
        if (row < layout.grid_rows) y -= 1;  // 单元格之间的线
        
        EPD_DrawLine(layout.margin, y, setInkScreenSize.screenWidth - layout.margin, y, BLACK);
    }
    
    for (int col = 0; col <= layout.grid_cols; col++) {
        int x = layout.margin + layout.icon_spacing_x + 
               col * (layout.cell_width + layout.icon_spacing_x);
        if (col < layout.grid_cols) x -= 1;  // 单元格之间的线
        
        EPD_DrawLine(x, layout.status_bar_height + layout.margin, 
                    x, setInkScreenSize.screenHeigt - layout.margin, BLACK);
    }
    #endif
    
    // 更新显示
    EPD_Display(ImageBW);
    EPD_Update();
    EPD_DeepSleep();
    vTaskDelay(1000);
    
    ESP_LOGI("ICON_POS", "所有图标位置记录完成，共%d个图标", icon_count);
}

// 辅助函数：估算文本显示宽度
uint16_t calculateTextWidth(const char* text, uint8_t font_size) {
    if (text == NULL) return 0;
    
    uint16_t width = 0;
    for (int i = 0; text[i] != '\0'; ) {
        if ((text[i] & 0xE0) == 0xE0) {
            // 中文字符
            width += font_size;
            i += 3;
        } else if (isprint(text[i])) {
            // 英文字符
            width += font_size / 2;
            i++;
        } else {
            i++;
        }
    }
    return width;
}
// 判断是否为中文字符串
bool isChineseText(uint8_t *text) {
    if (text == NULL) return false;
    
    for (int i = 0; text[i] != '\0'; i++) {
        // UTF-8 中文字符判断：首字节在 0xE4-0xE9 范围内
        if ((text[i] & 0xE0) == 0xE0) {
            return true;
        }
    }
    return false;
}

void updateDisplayWithString(uint16_t x, uint16_t y, uint8_t *text, uint8_t size, uint16_t color) {
   // clearLastDisplay();
    
    if (text == NULL || strlen((char*)text) == 0) {
        ESP_LOGE(TAG, "显示文本为空");
        return;
    }
    
    // 判断文本类型并调用相应的显示函数
    if (isChineseText(text)) {
        // 中文文本使用 EPD_ShowChinese_UTF8
        ESP_LOGI(TAG, "显示中文文本: %s", text);
        showChineseString(x, y, text, size, color);
    } else {
        ESP_LOGI(TAG, "显示英文文本: %s", text);
        
        EPD_ShowString(x, y,text,size,color);
    }
    
    // 记录最后显示的文本信息
    size_t textLen = strlen((char*)text);
    size_t copyLen = (textLen < sizeof(lastDisplayedText)) ? textLen : (sizeof(lastDisplayedText) - 1);
    memcpy(lastDisplayedText, text, copyLen);
    lastDisplayedText[copyLen] = '\0';
    lastTextLength = copyLen;
    
    lastX = x;
    lastY = y;
    lastSize = size;
    
    ESP_LOGI(TAG, "更新显示: 文本长度=%d, 内容=%s", lastTextLength, lastDisplayedText);
}

String formatPhonetic(const String& phonetic) {
    if (phonetic.length() == 0) {
        return phonetic;
    }
    
    String result = phonetic;
    
    // 移除可能已经存在的方括号
    if (result.startsWith("[")) {
        result = result.substring(1);
    }
    if (result.endsWith("]")) {
        result = result.substring(0, result.length() - 1);
    }
    
    // 添加新的方括号
    return "[" + result + "]";
}

uint16_t displayEnglishWrapped(uint16_t x, uint16_t y, const char* text, uint8_t fontSize, 
                              uint16_t color, uint16_t max_width, uint16_t line_height) {
    if (text == nullptr || strlen(text) == 0) {
        return y;
    }
    
    uint16_t current_x = x;
    uint16_t current_y = y;
    uint16_t char_width = fontSize / 2;
    
    ESP_LOGI("WRAP", "开始换行显示英语句子，最大宽度: %d", max_width);
    
    for (int i = 0; text[i] != '\0'; i++) {
        // 检查换行符
        if (text[i] == '\\' && text[i+1] == 'n') {
            // 遇到 \n，强制换行
            current_x = x;
            current_y += line_height;
            i++; // 跳过 n，下一次循环会跳过 
            ESP_LOGD("WRAP", "英语句子 \\n 强制换行到 Y: %d", current_y);
            continue;
        }
        else if (text[i] == '\n') {
            // 遇到真正的换行符
            current_x = x;
            current_y += line_height;
            ESP_LOGD("WRAP", "英语句子换行符强制换行到 Y: %d", current_y);
            continue;
        }
        else if (text[i] == '\r') {
            // 跳过回车符
            continue;
        }
        
        // 检查当前字符是否会超出边界
        if (current_x + char_width > x + max_width) {
            // 自动换行
            current_x = x;
            current_y += line_height;
            ESP_LOGD("WRAP", "英语句子自动换行到 Y: %d", current_y);
        }
        
        // 显示当前字符
        EPD_ShowChar(current_x, current_y, text[i], fontSize, color);
        current_x += char_width;
        
        ESP_LOGD("WRAP", "显示字符 '%c' 在 (%d,%d)", text[i], current_x - char_width, current_y);
    }
    
    // 返回下一行的起始Y坐标
    return current_y + line_height;
}

// 检查是否是中文标点符号
bool isChinesePunctuation(const char* str) {
    if (strlen(str) < 3) return false;
    
    // 常见中文标点符号的UTF-8编码
    if (str[0] == 0xEF && str[1] == 0xBC) {
        // EF BC 开头的标点
        if (str[2] == 0x8C ||  // ，
            str[2] == 0x9B ||  // ；
            str[2] == 0x9A ||  // ：
            str[2] == 0x9F ||  // ？
            str[2] == 0x81 ||  // ！
            str[2] == 0x88 ||  // （
            str[2] == 0x89) {  // ）
            return true;
        }
    } else if (str[0] == 0xE3 && str[1] == 0x80) {
        // E3 80 开头的标点
        if (str[2] == 0x82 ||  // 、
            str[2] == 0x81) {  // 。
            return true;
        }
    }
    return false;
}

// 将中文标点转换为英文标点
char convertChinesePunctuation(const char* chinese_punct) {
    if (strlen(chinese_punct) < 3) return '\0';
    
    if (chinese_punct[0] == 0xEF && chinese_punct[1] == 0xBC) {
        switch (chinese_punct[2]) {
            case 0x8C: return ',';  // ， -> ,
            case 0x9B: return ';';  // ； -> ;
            case 0x9A: return ':';  // ： -> :
            case 0x9F: return '?';  // ？ -> ?
            case 0x81: return '!';  // ！ -> !
            case 0x88: return '(';  // （ -> (
            case 0x89: return ')';  // ） -> )
        }
    } else if (chinese_punct[0] == 0xE3 && chinese_punct[1] == 0x80) {
        switch (chinese_punct[2]) {
            case 0x82: return '/';  // 、 -> /
            case 0x81: return '.';  // 。 -> .
        }
    }
    
    return '\0';
}


// 预处理函数：将字符串中的中文标点全部转换为英文标点
String convertChinesePunctuationsInString(const String& input) {
    String result = "";
    
    for (int i = 0; i < input.length(); ) {
        if (isChinesePunctuation(input.c_str() + i)) {
            char english_punct = convertChinesePunctuation(input.c_str() + i);
            if (english_punct != '\0') {
                result += english_punct;
            }
            i += 3; // 跳过3字节UTF-8编码
        } else {
            result += input[i];
            i++;
        }
    }
    
    return result;
}
uint16_t displayWrappedText(uint16_t x, uint16_t y, const char* text, uint8_t fontSize, 
                           uint16_t color, uint16_t max_width, uint16_t line_height) {
    if (text == nullptr || strlen(text) == 0) {
        return y;
    }
    
    String processedText = convertChinesePunctuationsInString(String(text));
    const char* processedStr = processedText.c_str();
    
    ESP_LOGI("WRAP", "原始文本: %s", text);
    ESP_LOGI("WRAP", "处理后文本: %s", processedStr);
    
    uint16_t current_x = x;
    uint16_t current_y = y;
    uint16_t english_width = fontSize / 2;
    uint16_t chinese_width = fontSize;
    
    for (int i = 0; processedStr[i] != '\0'; ) {
        // 处理换行符
        if (processedStr[i] == '\\' && processedStr[i+1] == 'n') {
            // 在 \n 换行的下方画线
            uint16_t line_y = current_y + line_height - 2;
            ESP_LOGI("WRAP", "\\n 换行画线 at Y=%d", line_y);
            EPD_DrawLine(x, line_y, x + max_width, line_y, BLACK);
            
            current_x = x;
            current_y += line_height;
            i += 2;
            continue;
        }
        else if (processedStr[i] == '\n') {
            // 在 \n 换行的下方画线
            uint16_t line_y = current_y + line_height - 2;
            ESP_LOGI("WRAP", "换行符画线 at Y=%d", line_y);
            EPD_DrawLine(x, line_y, x + max_width, line_y, BLACK);
            
            current_x = x;
            current_y += line_height;
            i++;
            continue;
        }
        else if (processedStr[i] == '\r') {
            i++;
            continue;
        }
        
        if ((processedStr[i] & 0x80) == 0) {
            // ASCII字符 - 自动换行不画线
            if (current_x + english_width > x + max_width) {
                current_x = x;
                current_y += line_height;
            }
            
            EPD_ShowChar(current_x, current_y, processedStr[i], fontSize, color);
            current_x += english_width;
            i++;
            
        } else if ((processedStr[i] & 0xE0) == 0xE0) {
            // 中文字符 - 自动换行不画线
            if (current_x + chinese_width > x + max_width) {
                current_x = x;
                current_y += line_height;
            }
            
            if (processedStr[i+1] != '\0' && processedStr[i+2] != '\0') {
                uint8_t chinese_char[4] = {processedStr[i], processedStr[i+1], processedStr[i+2], '\0'};
                EPD_ShowChinese_UTF8_Single(current_x, current_y, chinese_char, fontSize, color);
                current_x += chinese_width;
                i += 3;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
    
    ESP_LOGI("WRAP", "换行显示完成，最终Y坐标: %d", current_y + line_height);
    return current_y + line_height;
}

void safeDisplayWordEntry(const WordEntry& entry, uint16_t x, uint16_t y) {
    ESP_LOGI("DISPLAY", "开始显示单词条目...");
    
    if(entry.word.length() == 0) {
        ESP_LOGE("DISPLAY", "单词字段为空");
        return;
    }
    
    uint16_t current_y = y;
    uint16_t line_height_word = 30;    // 单词和音标行高
    uint16_t line_height_text = 20;    // 句子和翻译行高
    uint16_t screen_width = 416;
    uint16_t right_margin = 10;
    uint16_t max_word_width = screen_width - x - right_margin;

    // 第一行：单词（支持自动换行）
    ESP_LOGI("DISPLAY", "显示单词: %s", entry.word.c_str());
    
    // 计算单词是否需要换行
    uint16_t word_width = calculateTextWidth(entry.word.c_str(), 24);
    esp_task_wdt_reset();
    
    if (word_width <= max_word_width) {
        // 单词可以在一行显示
        updateDisplayWithString(x, current_y, (uint8_t*)entry.word.c_str(), 24, BLACK);
        current_y += line_height_word;  // 移动到下一行
    } else {
        // 单词需要换行显示
        current_y = displayEnglishWrapped(x, current_y, entry.word.c_str(), 24, BLACK, 
                                        max_word_width, line_height_word);
        // displayEnglishWrapped 返回最后一行文本的底部Y坐标
        // 不需要再额外增加行高
    }
    
    // 音标处理
    if(entry.phonetic.length() > 0) {
        esp_task_wdt_reset();
        String formattedPhonetic = formatPhonetic(entry.phonetic);
        uint16_t phonetic_width = calculateTextWidth(formattedPhonetic.c_str(), 24);
        uint16_t icon_width = 16;
        uint16_t icon_height = 16;
        uint16_t icon_offset_y = -16;  // 图标向上偏移16像素
        
        // 计算可用空间和动态间距
        uint16_t available_space = screen_width - right_margin - (x + word_width);
        uint16_t min_spacing = 15;   // 最小间距
        uint16_t max_spacing = 80;  // 最大间距
        uint16_t spacing = min_spacing;
        
        // 如果有充足空间，增加间距
        if (available_space > phonetic_width + icon_width + min_spacing + 20) {
            // 计算动态间距：可用空间的1/3，但不超出最大间距
            spacing = (available_space - phonetic_width - icon_width) / 3;
            if (spacing > max_spacing) {
                spacing = max_spacing;
            }
        }
        
        ESP_LOGI("DISPLAY", "单词宽度: %d, 音标宽度: %d, 可用空间: %d, 使用间距: %d", 
                word_width, phonetic_width, available_space, spacing);
        
        // 检查音标是否可以与单词同行显示（仅当单词没有换行时）
        bool word_fits_one_line = (word_width <= max_word_width);
        bool canDisplayInline = word_fits_one_line && 
                               (x + word_width + spacing + phonetic_width + icon_width) <= (screen_width - right_margin);
        
        if (canDisplayInline) {
            // 同行显示音标（在单词右侧，使用动态间距）
            uint16_t phonetic_x = x + word_width + spacing;
            
            // 图标显示在音标正上方偏右
            uint16_t icon_x = phonetic_x + phonetic_width - icon_width + 2; // 偏右2像素
            uint16_t icon_y = y + icon_offset_y; // 向上偏移
            
            EPD_ShowPicture(icon_x, icon_y, icon_width, icon_height, (uint8_t *)ZHONGJINGYUAN_3_7_HORN, BLACK);
            updateDisplayWithString(phonetic_x, y, (uint8_t*)formattedPhonetic.c_str(), 24, BLACK);
            // 音标与单词同行，Y坐标保持不变
            
            ESP_LOGI("DISPLAY", "音标同行显示，间距: %d", spacing);
        } else {
            // 换行显示音标 - 靠右显示，图标在音标正上方偏右
            uint16_t phonetic_x = screen_width - right_margin - phonetic_width;
            
            // 图标显示在音标正上方偏右
            uint16_t icon_x = phonetic_x + phonetic_width - icon_width + 2; // 偏右2像素
            uint16_t icon_y = current_y + icon_offset_y; // 向上偏移
            
            EPD_ShowPicture(icon_x, icon_y, icon_width, icon_height, (uint8_t *)ZHONGJINGYUAN_3_7_HORN, BLACK);
            updateDisplayWithString(phonetic_x, current_y, (uint8_t*)formattedPhonetic.c_str(), 24, BLACK);
            current_y += line_height_word;  // 移动到音标下方
            
            ESP_LOGI("DISPLAY", "音标换行显示");
        }
    } else {
        // 没有音标，确保Y坐标正确推进
        if (word_width <= max_word_width) {
            // 单词单行显示，Y坐标已经在上面推进过了
        } else {
            // 单词多行显示，current_y 已经在 displayEnglishWrapped 中正确设置
            // 但可能需要增加一些间距
            current_y += 5; // 增加小间距
        }
    }
    
    // 句子和翻译显示 - 确保有足够的间距
    bool hasDefinition = entry.definition.length() > 0;
    bool hasTranslation = entry.translation.length() > 0;
    
    // 在显示句子和翻译前增加间距
    if (hasDefinition || hasTranslation) {
        current_y += 10; // 增加段落间距
    }
    
    esp_task_wdt_reset();
    
    if(hasDefinition) {
        ESP_LOGI("DISPLAY", "显示句子: %s", entry.definition.c_str());
        
        // 英语句子按字符自动换行
        current_y = displayEnglishWrapped(x, current_y, entry.definition.c_str(), 16, BLACK, 
                                        screen_width - x - right_margin, line_height_text);
        
        // 在句子和翻译之间增加间距
        if(hasTranslation) {
            current_y += 8;
            
            ESP_LOGI("DISPLAY", "显示翻译: %s", entry.translation.c_str());
            // 预处理翻译文本，转换中文标点
            String processedTranslation = convertChinesePunctuationsInString(entry.translation);
            // 翻译也支持自动换行
            current_y = displayWrappedText(x, current_y, processedTranslation.c_str(), 16, BLACK, 
                                     screen_width - x - right_margin, line_height_text);
        }
    } else if(hasTranslation) {
        ESP_LOGI("DISPLAY", "显示翻译: %s", entry.translation.c_str());
        // 预处理翻译文本，转换中文标点
        String processedTranslation = convertChinesePunctuationsInString(entry.translation);
        // 翻译支持自动换行
        current_y = displayWrappedText(x, current_y, processedTranslation.c_str(), 16, BLACK, 
                                 screen_width - x - right_margin, line_height_text);
    }
    
    esp_task_wdt_reset();
    ESP_LOGI("DISPLAY", "单词条目显示完成，最终Y坐标: %d", current_y);
}

int countLines(File &file) {
  int count = 0;
  file.seek(0);
  while (file.available()) {
    if (file.read() == '\n') count++;
  }
  file.seek(0);
  return count;
}

WordEntry readLineAtPosition(File &file, int lineNumber) {
  file.seek(0);
  int currentLine = 0;
  WordEntry entry;
  
  while (file.available() && currentLine <= lineNumber) {
    String line = file.readStringUntil('\n');
    if (currentLine == lineNumber) {
      parseCSVLine(line, entry);
      break;
    }
    currentLine++;
  }
  
  return entry;
}

void parseCSVLine(String line, WordEntry &entry) {
  int fieldCount = 0;
  String field = "";
  bool inQuotes = false;
  char lastChar = 0;
  
  for (int i = 0; i < line.length(); i++) {
    char c = line[i];
    
    if (lastChar != '\\' && c == '"') {
      inQuotes = !inQuotes;
    } else if (c == ',' && !inQuotes) {
      // 字段结束
      assignField(fieldCount, field, entry);
      field = "";
      fieldCount++;
    } else {
      field += c;
    }
    lastChar = c;
  }
  
  // 处理最后一个字段
  if (fieldCount < 5) {
    assignField(fieldCount, field, entry);
  }
}

void assignField(int fieldCount, String &field, WordEntry &entry) {
  // 移除字段两端的引号（如果存在）
  if (field.length() >= 2 && field[0] == '"' && field[field.length()-1] == '"') {
    field = field.substring(1, field.length()-1);
  }
  
  switch (fieldCount) {
    case 0: entry.word = field; break;
    case 1: entry.phonetic = field; break;
    case 2: entry.definition = field; break;
    case 3: entry.translation = field; break;
    case 4: entry.pos = field; break;
  }
}

void printWordEntry(WordEntry &entry, int lineNumber) {
  ESP_LOGI(TAG, "line number: %d", lineNumber);
  ESP_LOGI(TAG, "word: %s", entry.word.c_str());
  if (entry.phonetic.length() > 0) {
    ESP_LOGI(TAG, "symbol: %s", entry.phonetic.c_str());
  }
  
  if (entry.definition.length() > 0) {
    ESP_LOGI(TAG, "English Definition: %s", entry.definition.c_str());
  }
  
  if (entry.translation.length() > 0) {
    ESP_LOGI(TAG, "chinese Definition: %s", entry.translation.c_str());
  }
  
  if (entry.pos.length() > 0) {
    ESP_LOGI(TAG, "part of speech: %s", entry.pos.c_str());
  }
  
  ESP_LOGI(TAG, "----------------------------------------");
}

void readAndPrintWords() {
  File file = SD.open("/ecdict.mini.csv");
  if (!file) {
    ESP_LOGE(TAG, "无法打开CSV文件");
    return;
  }
  
  // 读取前几行作为示例
  ESP_LOGI(TAG, "显示前5个单词");
  int lineCount = 0;
  
  while (file.available() && lineCount < 5) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      WordEntry entry;
      parseCSVLine(line, entry);
      printWordEntry(entry, lineCount);
      lineCount++;
    }
  }
  
  file.close();
  
  ESP_LOGI(TAG, "开始随机显示单词");
}

void readAndPrintRandomWord() {
  File file = SD.open("/ecdict.mini.csv");
  if (!file) {
    ESP_LOGE(TAG, "无法打开CSV文件");
    return;
  }
  
  // 计算总行数
  int totalLines = countLines(file);
  if (totalLines <= 1) {
    ESP_LOGE(TAG, "文件内容不足");
    file.close();
    return;
  }
  
  // 随机选择一行（跳过标题行）
  int randomLine = random(1, totalLines);
  entry = readLineAtPosition(file, randomLine);
  file.close();
  
  ESP_LOGI(TAG, "随机单词");
  printWordEntry(entry, randomLine);
}

void EPD_part_Show(uint16_t Xstart,uint16_t Ystart,uint16_t Xend,uint16_t Yend,uint16_t Color)
{
	EPD_PartInit();
	EPD_DrawLine(Xstart,Ystart,Xend,Yend,BLACK);
	EPD_Display(ImageBW);
	ESP_LOGI(TAG,"EPD_DrawLine");
	EPD_Update();
	vTaskDelay(100);
	EPD_DeepSleep();
}

void showPromptInfor(uint8_t *tempPrompt,bool isAllRefresh) {
    static uint8_t *lastPrompt = nullptr;
    static char lastPromptContent[256] = {0};
    
    // 检查输入有效性
    if (tempPrompt == nullptr) {
     //   ESP_LOGW("PROMPT", "接收到空指针");
        return;
    }
    
    const char* currentPrompt = (const char*)tempPrompt;
    if(interfaceIndex !=1) {
    
        // 检查内容是否变化（比较字符串内容而不是指针）
        if (lastPrompt != nullptr && strcmp(currentPrompt, lastPromptContent) == 0) {
            return;  // 内容相同，无需更新
        }
        ESP_LOGI("PROMPT", "更新提示信息: %s", currentPrompt);
        
        // 保存当前内容用于下次比较
        strncpy(lastPromptContent, currentPrompt, sizeof(lastPromptContent) - 1);
        lastPromptContent[sizeof(lastPromptContent) - 1] = '\0';
        lastPrompt = tempPrompt;

        if(isAllRefresh) {
            EPD_FastInit();
            EPD_Display_Clear();
            EPD_Update();  //局刷之前先对E-Paper进行清屏操作
            EPD_PartInit();
            EPD_ShowPicture(5,5,15,16,ZHONGJINGYUAN_3_7_NAIL,BLACK);
            clearDisplayArea(30,10,340,30);
            updateDisplayWithString(30,10, tempPrompt,16,BLACK);
            EPD_Display(ImageBW);
            EPD_Update();
            EPD_DeepSleep();
            vTaskDelay(1000);
        } else {
            EPD_ShowPicture(5,5,15,16,ZHONGJINGYUAN_3_7_NAIL,BLACK);
            clearDisplayArea(30,10,340,30);
            updateDisplayWithString(30,10, tempPrompt,16,BLACK);
        }

    }
}

// 定时器回调函数
void sleep_timer_callback(void* arg) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (current_time - last_activity_time >= SLEEP_TIMEOUT_MS) {
        if (!is_sleep_mode) {
            ESP_LOGI("SLEEP", "进入休眠模式");
            is_sleep_mode = true;
            
            // 保存当前显示的数据用于休眠模式显示
            if (entry.word.length() > 0) {
                sleep_mode_entry = entry;
                has_sleep_data = true;
            }
            
            // 触发休眠模式显示
            inkScreenTestFlag = 99; // 使用特殊值表示休眠模式
        }
    }
}
void update_activity_time() {
    last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (is_sleep_mode) {
        ESP_LOGI("SLEEP", "退出休眠模式");
        is_sleep_mode = false;
        has_sleep_data = false;
    }
}
// 居中显示单行（自动换行）
uint16_t display_centered_line(uint16_t start_x, uint16_t y, const char* text, 
                              uint8_t fontSize, uint16_t color, 
                              uint16_t max_width, uint16_t line_height) {
    uint16_t current_x = start_x;
    uint16_t current_y = y;
    uint16_t english_width = fontSize / 2;
    uint16_t chinese_width = fontSize;
    
    for (int i = 0; text[i] != '\0'; ) {
        if ((text[i] & 0x80) == 0) {
            // ASCII字符
            if (current_x + english_width > start_x + max_width) {
                current_x = start_x;
                current_y += line_height;
            }
            
            EPD_ShowChar(current_x, current_y, text[i], fontSize, color);
            current_x += english_width;
            i++;
            
        } else if ((text[i] & 0xE0) == 0xE0) {
            // 中文字符
            if (current_x + chinese_width > start_x + max_width) {
                current_x = start_x;
                current_y += line_height;
            }
            
            if (text[i+1] != '\0' && text[i+2] != '\0') {
                uint8_t chinese_char[4] = {text[i], text[i+1], text[i+2], '\0'};
                EPD_ShowChinese_UTF8_Single(current_x, current_y, chinese_char, fontSize, color);
                current_x += chinese_width;
                i += 3;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
    
    return current_y + line_height;
}
// 居中换行文本显示
uint16_t display_centered_wrapped_text(uint16_t center_x, uint16_t y, const char* text, 
                                      uint8_t fontSize, uint16_t color, 
                                      uint16_t max_width, uint16_t line_height) {
    if (text == nullptr || strlen(text) == 0) return y;
    
    String processedText = convertChinesePunctuationsInString(String(text));
    const char* processedStr = processedText.c_str();
    
    uint16_t current_y = y;
    uint16_t english_width = fontSize / 2;
    uint16_t chinese_width = fontSize;
    
    // 按行分割文本
    char* text_copy = strdup(processedStr);
    char* line = strtok(text_copy, "\n");
    
    while (line != NULL) {
        // 计算当前行的宽度
        uint16_t line_width = calculateTextWidth(line, fontSize);
        uint16_t line_x = center_x - line_width / 2;
        
        // 显示当前行
        uint16_t line_end_y = display_centered_line(line_x, current_y, line, fontSize, color, max_width, line_height);
        current_y = line_end_y;
        
        line = strtok(NULL, "\n");
    }
    
    free(text_copy);
    return current_y;
}
// 计算文本在指定宽度内会显示多少行
uint16_t calculateTextLines(const char* text, uint8_t fontSize, uint16_t max_width) {
    if (text == nullptr || strlen(text) == 0) return 0;
    
    uint16_t lines = 1;
    uint16_t current_width = 0;
    uint16_t english_width = fontSize / 2;
    uint16_t chinese_width = fontSize;
    
    for (int i = 0; text[i] != '\0'; ) {
        if ((text[i] & 0x80) == 0) {
            // ASCII字符
            if (text[i] == '\n') {
                lines++;
                current_width = 0;
                i++;
                continue;
            }
            
            current_width += english_width;
            if (current_width > max_width) {
                lines++;
                current_width = english_width;
            }
            i++;
            
        } else if ((text[i] & 0xE0) == 0xE0) {
            // 中文字符
            current_width += chinese_width;
            if (current_width > max_width) {
                lines++;
                current_width = chinese_width;
            }
            i += 3;
        } else {
            i++;
        }
    }
    
    return lines;
}
void display_sleep_mode() {
    if (!has_sleep_data) {
        ESP_LOGW("SLEEP", "没有数据可显示");
        return;
    }
    
    EPD_FastInit();
    EPD_Display_Clear();
    EPD_Update();
    EPD_PartInit();
    clearDisplayArea(10,40,EPD_H,EPD_W);
    EPD_ShowPicture(380,40,32,32,ZHONGJINGYUAN_3_7_LOCK,BLACK);
    uint16_t screen_width = 416;
    uint16_t screen_height = 240;
    uint16_t center_x = screen_width / 2;
    
    // 计算总内容高度并确定起始Y坐标
    uint16_t total_height = 0;
    uint16_t line_spacing = 16; // 行间距
    
    // 估算各部分高度
    uint16_t word_height = 24 + line_spacing;
    uint16_t phonetic_height = 20 + line_spacing;
    
    // 计算翻译和句子的行数
    uint16_t definition_lines = calculateTextLines(sleep_mode_entry.definition.c_str(), 16, screen_width - 40);
    uint16_t translation_lines = calculateTextLines(sleep_mode_entry.translation.c_str(), 16, screen_width - 40);
    
    uint16_t definition_height = definition_lines * (16 + 5) + line_spacing; // 16字体+5行间距
    uint16_t translation_height = translation_lines * (16 + 5) + line_spacing;
    
    total_height = word_height + phonetic_height + definition_height + translation_height;
    
    // 计算起始Y坐标（垂直居中）
    uint16_t start_y = (screen_height - total_height) / 2;
    uint16_t current_y = start_y;
    
    // 1. 居中显示单词
    if (sleep_mode_entry.word.length() > 0) {
        uint16_t word_width = calculateTextWidth(sleep_mode_entry.word.c_str(), 24);
        uint16_t word_x = center_x - word_width / 2;
        updateDisplayWithString(word_x, current_y, (uint8_t*)sleep_mode_entry.word.c_str(), 24, BLACK);
        current_y += 24 + line_spacing;
    }
    
    // 2. 居中显示音标
    if (sleep_mode_entry.phonetic.length() > 0) {
        String formattedPhonetic = formatPhonetic(sleep_mode_entry.phonetic);
        uint16_t phonetic_width = calculateTextWidth(formattedPhonetic.c_str(), 24);
        uint16_t phonetic_x = center_x - phonetic_width / 2;
        updateDisplayWithString(phonetic_x, current_y, (uint8_t*)formattedPhonetic.c_str(), 24, BLACK);
        current_y += 20 + line_spacing;
        ESP_LOGI("SLEEP", "显示音标: x=%d, y=%d, 内容: %s", phonetic_x, current_y, formattedPhonetic.c_str());
    }
    
    // 3. 居中显示翻译（多行）
    if (sleep_mode_entry.translation.length() > 0) {
        String processedTranslation = convertChinesePunctuationsInString(sleep_mode_entry.translation);
        current_y = display_centered_wrapped_text(center_x, current_y, processedTranslation.c_str(), 16, BLACK, 
                                                screen_width - 40, 21); // 16字体+5行间距
        current_y += line_spacing;
    }
    
    // 4. 居中显示句子（多行）
    if (sleep_mode_entry.definition.length() > 0) {
        String processedDefinition = convertChinesePunctuationsInString(sleep_mode_entry.definition);
        display_centered_wrapped_text(center_x, current_y, processedDefinition.c_str(), 16, BLACK, 
                                    screen_width - 40, 21); // 16字体+5行间距
    }
    
    EPD_Display(ImageBW);
    EPD_Update();
    EPD_DeepSleep();
}

void init_sleep_timer() {
    esp_timer_create_args_t timer_args = {
        .callback = &sleep_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sleep_timer"
    };
    
    esp_timer_create(&timer_args, &sleep_timer);
    esp_timer_start_periodic(sleep_timer, 1000); // 每秒检查一次
    last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    ESP_LOGI("SLEEP", "休眠定时器初始化完成");
}

void changeInkSreenSize() {
    uint16_t newWidth = WebUI::inkScreenXSizeSet->get();
    uint16_t newHeight = WebUI::inkScreenYSizeSet->get();
    
    if(setInkScreenSize.screenWidth != newWidth && 
       setInkScreenSize.screenHeigt != newHeight) {
        setInkScreenSize.screenHeigt = newHeight;
        setInkScreenSize.screenWidth = newWidth;
        ESP_LOGI(TAG,"screenHeigt:%d,screenWidth:%d\r\n", setInkScreenSize.screenHeigt, setInkScreenSize.screenWidth);
        inkScreenTestFlag = 7;

    }
}

void ink_screen_show(void *args)
{
    float num=12.05;
    uint8_t dat=0;
	grbl_msg_sendf(CLIENT_SERIAL, MsgLevel::Info,"ink_screen_show");
	Uart0.printf("ink_screen_show\r\n");
   init_sleep_timer();
	while(1)
	{
        if (inkScreenTestFlag == 99) {
            ESP_LOGI("SLEEP", "显示休眠模式界面");
            display_sleep_mode();
            inkScreenTestFlag = 0;
            vTaskDelay(1000);
            continue;
        }
		switch(inkScreenTestFlag)
		{
			case 0:
				switch(inkScreenTestFlagTwo)
				{
					case 0:
					break;
					case 11:
                        ESP_LOGI(TAG,"############inkScreenTestFlagTwo\r\n");
                        update_activity_time(); // 更新活动时间 
						EPD_FastInit();
						EPD_Display_Clear();
						EPD_Update();  //局刷之前先对E-Paper进行清屏操作
						vTaskDelay(100);
                        EPD_PartInit();
						readAndPrintRandomWord();
                        showPromptInfor(showPrompt,false);
						if(entry.word.length() > 0) {
							clearDisplayArea(10,40,EPD_H,EPD_W);
							safeDisplayWordEntry(entry, 20, 60);
						} else {
							ESP_LOGW("DISPLAY", "跳过空单词显示");
						}

						EPD_Display(ImageBW);
						EPD_Update();
						EPD_DeepSleep();
                        vTaskDelay(1000);
						inkScreenTestFlagTwo = 0;
                        inkScreenTestFlag = 0;
                        interfaceIndex =2;
					break;
					case 22:
					break;
					case 33:
					break;
					case 44:
					break;
					case 55:
                        update_activity_time(); // 更新活动时间
					 	EPD_FastInit();
						EPD_Display_Clear();
						EPD_Update();  //局刷之前先对E-Paper进行清屏操作
						EPD_PartInit();
						ESP_LOGI(TAG,"inkScreenTestFlagTwo\r\n");
						// 遍历所有图像
						// for(int i = 0; i < GAME_PEOPLE_COUNT; i++) {
						// 	EPD_ShowPicture(60,40,128,128,gamePeople[i],BLACK);
						// 	EPD_Display(ImageBW);
						// 	EPD_Update();
						// 	vTaskDelay(1000);
						// 	ESP_LOGI(TAG,"gamePeople[%d]\r\n",i);
						// }
						EPD_DeepSleep();
						inkScreenTestFlagTwo = 0;
                        inkScreenTestFlag = 0;
                        interfaceIndex =2;
					break;
					case 66:
					break;
                    default:
                        break;

				}
			break;
			case 1:
                update_activity_time(); 
                drawUnderlineAtIcon(0);
				inkScreenTestFlag = 0;
			break;
			case 2:
                update_activity_time(); 
                drawUnderlineAtIcon(1);
				inkScreenTestFlag = 0;
			break;
			case 3:
                update_activity_time(); 
				drawUnderlineAtIcon(2);
				inkScreenTestFlag = 0;
			break;
			case 4:
                update_activity_time(); 
                drawUnderlineAtIcon(3);
				inkScreenTestFlag = 0;
			break;
			case 5:
                update_activity_time(); 
				drawUnderlineAtIcon(4);
				inkScreenTestFlag = 0;
			break;
			case 6:
                update_activity_time(); 
                drawUnderlineAtIcon(5);
				inkScreenTestFlag = 0;
			break;
			case 7:
                update_activity_time(); 
				updateDisplayWithMain();
                interfaceIndex =1;
				inkScreenTestFlag = 0;
			break;
            default:
                break;
		}
        showPromptInfor(showPrompt,true);
		updateDisplayWithWifiIcon();
        changeInkSreenSize();
       // ESP_LOGI(TAG,"ink_screen_show\r\n");
        vTaskDelay(100);
	}
}

void ink_screen_init()
{
    Uart0.printf("ink_screen_init\r\n");
    WebUI::inkScreenXSizeSet->load();
    WebUI::inkScreenYSizeSet->load();
	setInkScreenSize.screenWidth = WebUI::inkScreenXSizeSet->get();
    setInkScreenSize.screenHeigt = WebUI::inkScreenYSizeSet->get();
    EPD_GPIOInit();	
    Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);//create  Canvas 
    Paint_Clear(WHITE);
	Uart0.printf("Paint_Clear\r\n");

	updateDisplayWithMain();

	Uart0.printf("fast refresh\r\n");

	SDState state = get_sd_state(true);
	if (state != SDState::Idle) {
		if (state == SDState::NotPresent) {
			ESP_LOGI(TAG,"No SD Card\r\n");
		} else {
			ESP_LOGI(TAG,"SD Card Busy\r\n");
		}
	}

    BaseType_t task_created = xTaskCreatePinnedToCore(ink_screen_show, 
                                                        "ink_screen_show", 
                                                        8192, 
                                                        NULL, 
                                                        4, 
                                                        &_eventTaskHandle, 
                                                        0);
}
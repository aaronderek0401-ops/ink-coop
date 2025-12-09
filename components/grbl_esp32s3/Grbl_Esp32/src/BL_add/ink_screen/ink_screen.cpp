#include "ink_screen.h"
#include "./SPI_Init.h"
#include "esp_timer.h"

extern "C" {
	#include "./EPD.h"
#include "./EPD_GUI.h"
#include "./EPD_Font.h"
#include "./Pic.h"
}
// 全局变量定义
int g_last_underline_x = 0;
int g_last_underline_y = 0;
int g_last_underline_width = 0;
// 全局变量：当前选中的图标索引
int g_selected_icon_index = -1;
uint8_t inkScreenTestFlag = 0; 
uint8_t inkScreenTestFlagTwo = 0;
uint8_t interfaceIndex=1;
int g_global_icon_count = 0;     // 已分配的全局图标计数

IconPosition g_icon_positions[MAX_GLOBAL_ICONS];
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
// 全局图标数组
IconInfo g_available_icons[21] = {
    {ZHONGJINGYUAN_3_7_ICON_1, 62, 64},//1
    {ZHONGJINGYUAN_3_7_ICON_2, 64, 64},//2
    {ZHONGJINGYUAN_3_7_ICON_3, 86, 64},//3
    {ZHONGJINGYUAN_3_7_ICON_4, 71, 56},//4
    {ZHONGJINGYUAN_3_7_ICON_5, 76, 56},//5
    {ZHONGJINGYUAN_3_7_ICON_6, 94, 64},//6
    {ZHONGJINGYUAN_3_7_NAIL,15,16},//7
    {ZHONGJINGYUAN_3_7_LOCK,32,32},//8
    {ZHONGJINGYUAN_3_7_HORN,16,16},//9
    {ZHONGJINGYUAN_3_7_BATTERY_1,36,24},//10
    {ZHONGJINGYUAN_3_7_WIFI_DISCONNECT,32,32},//11
    {ZHONGJINGYUAN_3_7_WIFI_CONNECT,32,32},//12
    {ZHONGJINGYUAN_3_7_UNDERLINE,60,16},//13
    {ZHONGJINGYUAN_3_7_promt,320,36},//14
    {ZHONGJINGYUAN_3_7_wifi_battry,80,36},//15
    {ZHONGJINGYUAN_3_7_word,336,48},//16
    {ZHONGJINGYUAN_3_7_Translation1,416,24},//17
    {ZHONGJINGYUAN_3_7_separate,416,16},//18
    {ZHONGJINGYUAN_3_7_horn,80,16},//19
    {ZHONGJINGYUAN_3_7_pon,80,32},//20
    {ZHONGJINGYUAN_3_7_definition,416,72}//21

};

// 全局界面管理器实例
static ScreenManager g_screen_manager;

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

// 清除上次绘制的下划线
void clearLastUnderline() {
    if (g_last_underline_width > 0) {
        ESP_LOGI(TAG, "开始清除上次下划线: 位置(%d,%d), 宽度%d", 
                g_last_underline_x, g_last_underline_y, g_last_underline_width);
        
        // 用白色覆盖上次的下划线
        for (int i = 0; i < 3; i++) {  // 清除3像素高度的区域
            EPD_DrawLine(g_last_underline_x, 
                        g_last_underline_y + i, 
                        g_last_underline_x + g_last_underline_width, 
                        g_last_underline_y + i, 
                        WHITE);
        }
        
        ESP_LOGI(TAG, "清除上次下划线完成");
        // 重置记录
        g_last_underline_width = 0;
    }
}

void drawUnderlineForIconEx(int icon_index) {
    if (icon_index < 0 || icon_index >= MAX_GLOBAL_ICONS) {
        ESP_LOGE(TAG, "无效的图标索引: %d", icon_index);
        return;
    }
    
    // 获取图标位置信息
    IconPosition* icon = &g_icon_positions[icon_index];
    
    if (icon->width == 0 || icon->height == 0) {
        ESP_LOGE(TAG, "图标%d位置未初始化", icon_index);
        return;
    }
    
    // 调试：显示图标信息
    ESP_LOGI(TAG, "图标%d信息: 原始坐标(%d,%d), 原始尺寸(%dx%d)", 
            icon_index, icon->x, icon->y, icon->width, icon->height);
    
    // 初始化显示（确保可以绘制）
    EPD_FastInit();
    EPD_Display_Clear();
    EPD_Update();  //局刷之前先对E-Paper进行清屏操作
    vTaskDelay(100 / portTICK_PERIOD_MS);
    EPD_PartInit();
      // 清除上次的下划线（如果有）
    clearLastUnderline();
    // 计算缩放比例
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    ESP_LOGI(TAG, "屏幕尺寸: %dx%d, 缩放比例: X=%.2f, Y=%.2f, 使用: %.2f", 
            setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt, 
            scale_x, scale_y, scale);
    
    // 计算实际显示位置
    int x = (int)(icon->x * scale);
    int y = (int)(icon->y * scale);
    int width = (int)(icon->width * scale);
    int height = (int)(icon->height * scale);
    
    // 调试：显示计算后的位置
    ESP_LOGI(TAG, "计算后位置: X=%d, Y=%d, 宽度=%d, 高度=%d", x, y, width, height);
    
    // 绘制下划线（在图标下方，跟图标同宽）
    // 方法1：在图标底部绘制
    int underline_y = y + height + 3;  // 图标下方3像素
    
    // 方法2：如果方法1不行，尝试固定位置测试
    // int underline_y = 150;  // 临时固定位置测试
    
    // 绘制2像素粗的线
    EPD_DrawLine(x, underline_y, x + width, underline_y, BLACK);
    EPD_DrawLine(x, underline_y + 1, x + width, underline_y + 1, BLACK);
    
    // 绘制一个测试矩形，确认绘制功能正常
    // EPD_DrawRectangle(x, y, x + width, y + height, BLACK, 1);
    
    // 记录这次的下划线位置（用于下次清除）
    g_last_underline_x = x;
    g_last_underline_y = underline_y;
    g_last_underline_width = width;
    
    // 更新显示
    EPD_Display(ImageBW);
    EPD_Update();
    EPD_DeepSleep();

    ESP_LOGI(TAG, "在图标%d下方绘制下划线完成: 位置(%d,%d), 宽度%d, 图标底部Y=%d", 
            icon_index, x, underline_y, width, y + height);
    
    // 短暂延迟，确保显示更新
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

// 调试函数：显示所有图标位置信息
void debugIconPositions() {
    ESP_LOGI(TAG, "=== 图标位置调试信息 ===");
    ESP_LOGI(TAG, "屏幕尺寸: %dx%d", setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
    
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    ESP_LOGI(TAG, "缩放比例: %.2f", scale);
    
    for (int i = 0; i < 6; i++) {
        IconPosition* icon = &g_icon_positions[i];
        if (icon->width > 0 && icon->height > 0) {
            int display_x = (int)(icon->x * scale);
            int display_y = (int)(icon->y * scale);
            int display_width = (int)(icon->width * scale);
            int display_height = (int)(icon->height * scale);
            
            ESP_LOGI(TAG, "图标%d: 原始(%d,%d,%dx%d) -> 显示(%d,%d,%dx%d)", 
                    i, 
                    icon->x, icon->y, icon->width, icon->height,
                    display_x, display_y, display_width, display_height);
        } else {
            ESP_LOGI(TAG, "图标%d: 未初始化", i);
        }
    }
    ESP_LOGI(TAG, "=== 调试结束 ===");
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

 //清空整个屏幕（显示白色）
void clearEntireScreen() {
    ESP_LOGI(TAG, "清空整个屏幕");
    
    EPD_FastInit();
    EPD_Display_Clear();
    EPD_Update();
    delay_ms(50);
    EPD_PartInit();
    
    // 清除整个屏幕区域
    for (int y = 0; y < setInkScreenSize.screenHeigt; y += 8) {
        for (int x = 0; x < setInkScreenSize.screenWidth; x++) {
            Paint_SetPixel(x, y, WHITE);
        }
    }
    
    // 更新显示
    EPD_Display(ImageBW);
    EPD_Update();
    delay_ms(50);
    ESP_LOGI(TAG, "屏幕已清空");
}
// 清除所有矩形区域（包括边框）
void clearAllRectAreas(RectInfo *rects, int rect_count) {
    ESP_LOGI(TAG, "清除所有矩形区域，共%d个", rect_count);
    
    // 计算缩放比例
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    for (int i = 0; i < rect_count; i++) {
        RectInfo* rect = &rects[i];
        
        // 计算缩放后的位置和尺寸
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        int display_width = (int)(rect->width * global_scale + 0.5f);
        int display_height = (int)(rect->height * global_scale + 0.5f);
        
        // 边界检查
        if (display_x < 0) display_x = 0;
        if (display_y < 0) display_y = 0;
        if (display_x + display_width > setInkScreenSize.screenWidth) {
            display_width = setInkScreenSize.screenWidth - display_x;
        }
        if (display_y + display_height > setInkScreenSize.screenHeigt) {
            display_height = setInkScreenSize.screenHeigt - display_y;
        }
        
        if (display_width > 0 && display_height > 0) {
            // 清除矩形区域（用白色填充）
            for (int y = display_y; y < display_y + display_height; y++) {
                for (int x = display_x; x < display_x + display_width; x++) {
                    Paint_SetPixel(x, y, WHITE);
                }
            }
            
            ESP_LOGD(TAG, "清除矩形%d区域: (%d,%d) %dx%d", 
                    i, display_x, display_y, display_width, display_height);
        }
    }
}
//清除之前显示的所有图标区域
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
void EPD_ShowPictureScaled(uint16_t orig_x, uint16_t orig_y, 
                           uint16_t orig_w, uint16_t orig_h,
                           const uint8_t* BMP, uint16_t color) {
    // 使用统一的缩放因子保持长宽比（按最小缩放因子适配到屏幕）
    float scale_x = (float) setInkScreenSize.screenWidth / (float)416;
    float scale_y = (float)setInkScreenSize.screenHeigt / (float)240;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
                            
    // 计算缩放后的位置和尺寸（统一缩放，保持比例）
    uint16_t new_x = (uint16_t)(orig_x * scale);
    uint16_t new_y = (uint16_t)(orig_y * scale);
    uint16_t new_w = (uint16_t)(orig_w * scale);
    uint16_t new_h = (uint16_t)(orig_h * scale);

    // 确保最小尺寸
    if (new_w < 4) new_w = 4;
    if (new_h < 4) new_h = 4;

    // 源图每列的字节数（源图按列存储，每字节8个垂直像素）
    uint16_t src_bytes_per_col = (orig_h + 7) / 8;

    // 对目标区域每个像素进行最近邻采样
    for (uint16_t dx = 0; dx < new_w; dx++) {
        for (uint16_t dy = 0; dy < new_h; dy++) {
            // 映射回源图坐标
            uint16_t sx = dx * orig_w / new_w;
            uint16_t sy = dy * orig_h / new_h;

            // 计算源图中该像素的字节和位
            uint32_t src_byte_idx = (uint32_t)sx * src_bytes_per_col + sy / 8;
            uint8_t src_bit_pos = sy % 8;
            uint8_t src_byte = BMP[src_byte_idx];

            // 读取源像素（MSB first：bit 7 对应第 0 行）
            bool pixel_on = (src_byte & (0x80 >> src_bit_pos)) != 0;

            // 设置目标像素
            Paint_SetPixel(new_x + dx, new_y + dy, pixel_on ? !color : color);
        }
    }

    ESP_LOGD(TAG, "Scaled icon: (%d,%d) %dx%d -> (%d,%d) %dx%d",
             orig_x, orig_y, orig_w, orig_h, new_x, new_y, new_w, new_h);
}

// 定义7个矩形，每个矩形可以显示多个图标
RectInfo rects[7] = {
    // ==================== 矩形0：状态栏 ====================
    // 位于屏幕顶部，显示状态信息
    {
        0, 0, 416, 36,  // x, y, width, height - 横跨整个顶部
        {                // icons数组
            {0.0f, 0.0f, 0},
            {0.0f, 0.0f, 0},
            {0.0f, 0.0f, 0},
            {0.0f, 0.0f, 0}
        },
        0                // icon_count - 状态栏不显示应用图标
    },
    
    // ==================== 矩形1：左上区域 ====================
    // 占据左上角，显示3个图标
    {
        0, 40, 138, 100,  // x, y, width, height
        {
            {0.1f, 0.1f, 0},  // 左上方，图标0
            {0.4f, 0.1f, 1},  // 右上方，图标1
            {0.2f, 0.3f, 2},   // 下方居中，图标2
            {0.0f, 0.0f, 0}     // 填充
        },
        3
    },
    
    // ==================== 矩形2：中上区域 ====================
    // 位于矩形1右侧
    {
        142, 40, 132, 100,  // x, y, width, height
        {
            {0.1f, 0.1f, 3},  // 图标3
            {0.3f, 0.3f, 4},  // 图标4
            {0.0f, 0.0f, 0},  // 填充
            {0.0f, 0.0f, 0}   // 填充
        },
        2
    },
    
    // ==================== 矩形3：右上区域 ====================
    // 位于矩形2右侧
    {
        278, 40, 138, 100,  // x, y, width, height
        {
            {0.1f, 0.2f, 5},  // 左侧居中，图标5
            {0.4f, 0.2f, 0},  // 右侧居中，图标0（循环使用）
            {0.0f, 0.0f, 0},   // 填充
            {0.0f, 0.0f, 0}    // 填充
        },
        2
    },
    
    // ==================== 矩形4：左下区域 ====================
    // 位于屏幕左下角
    {
        0, 144, 138, 96,  // x, y, width, height
        {
            {0.1f, 0.1f, 1},  // 图标1
            {0.2f, 0.1f, 2},  // 图标2
            {0.4f, 0.1f, 3},  // 图标3
            {0.2f, 0.3f, 4}   // 底部居中，图标4
        },
        4
    },
    
    // ==================== 矩形5：中下区域 ====================
    // 位于矩形4右侧
    {
        142, 144, 132, 96,  // x, y, width, height
        {
            {0.1f, 0.1f, 5},  // 顶部居中，图标5
            {0.2f, 0.3f, 0},  // 左下，图标0
            {0.4f, 0.3f, 1},  // 右下，图标1
            {0.0f, 0.0f, 0}    // 填充
        },
        3
    },
    
    // ==================== 矩形6：右下区域 ====================
    // 位于屏幕右下角
    {
        278, 144, 138, 96,  // x, y, width, height
        {
            {0.2f, 0.2f, 2},  // 中心，图标2
            {0.0f, 0.0f, 0},  // 填充
            {0.0f, 0.0f, 0},  // 填充
            {0.0f, 0.0f, 0}   // 填充
        },
        1
    }
};

// 矩形总数
int rect_count = sizeof(rects) / sizeof(rects[0]);

// 网格布局示意图：
//  ┌─────────────────────────────────────────────────┐
//  │                   状态栏 (0)                     │
//  ├────────────┬────────────┬───────────────┤
//  │  矩形1     │   矩形2    │    矩形3      │
//  │  (3图标)   │  (2图标)   │   (2图标)     │
//  │            │            │               │
//  ├────────────┼────────────┼───────────────┤
//  │  矩形4     │   矩形5    │    矩形6      │
//  │  (4图标)   │  (3图标)   │   (1图标)     │
//  │            │            │               │
//  └────────────┴────────────┴───────────────┘


   // 定义10个矩形，每个矩形可以显示多个图标
    RectInfo vocab_rects[10] = {
        // ==================== 矩形0：状态栏 ====================
        // 位于屏幕顶部左侧，显示提示信息
        {
            0, 0, 336, 36,  // x, y, width, height - 占据屏幕整个顶部4/5区域
            {                // icons数组
                {0.0f, 0.0f, 7}, // 左上方，图标7
                {0.1f, 0.1f, 14},
                {0.0f, 0.0f, 0},
                {0.0f, 0.0f, 0}
            },
            2               // icon_count - 状态栏不显示应用图标
        },
        
        // ==================== 矩形1：右上区域 ====================
        // 占据右上角，显示1个图标
        {
            336, 0, 416, 36,  // x, y, width, height
            {
                {0.2f, 0.2f, 15},  // 左上方，图标15
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0}     // 填充
            },
            1
        },
        
        // ==================== 矩形2：中左区域 ====================
        // 位于矩形2中左侧
        {
            0, 40, 336, 90,  // x, y, width, height
            {
                {0.1f, 0.1f, 16},  // 图标16
                {0.0f, 0.0f, 0},  
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0}   // 填充
            },
            1
        },
        
        // ==================== 矩形3：中右区域 ====================
        // 位于矩形2右侧
        {
            336, 40, 416, 56,  // x, y, width, height
            {
                {0.1f, 0.2f, 19},  // 左侧居中，图标5
                {0.0f, 0.0f, 0},  //
                {0.0f, 0.0f, 0},   // 填充
                {0.0f, 0.0f, 0}    // 填充
            },
            1
        },
        
        // ==================== 矩形4：左下区域 ====================
        // 位于屏幕左下角
        {
            336, 56, 416, 90,  // x, y, width, height
            {
                {0.1f, 0.1f, 20},  // 图标20
                {0.0f, 0.0f, 0},  
                {00.0f, 0.0f, 0},  
                {0.0f, 0.0f, 0}   
            },
            1
        },
        
        // ==================== 矩形5：中下区域 ====================
        // 位于矩形4右侧
        {
            0, 90, 416, 140,  // x, y, width, height
            {
                {0.1f, 0.1f, 5},  // 16
                {0.0f, 0.0f, 0},  // 左下，图标0
                {0.0f, 0.0f, 0},  // 右下，图标1
                {0.0f, 0.0f, 0}    // 填充
            },
            1
        },
        
        // ==================== 矩形6：右下区域 ====================
        // 位于屏幕右下角
        {
            0, 140, 416, 166,  // x, y, width, height
            {
                {0.2f, 0.2f, 2},  // 17
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0}   // 填充
            },
            1
        },

        {
            0, 184, 416, 200,  // x, y, width, height
            {
                {0.2f, 0.2f, 2},  // 18
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0}   // 填充
            },
            1
        },
        
        {
            0, 200, 416, 224,  // x, y, width, height
            {
                {0.2f, 0.2f, 2},  // 17
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0}   // 填充
            },
            1
        },
        {
            0, 224, 416, 240,  // x, y, width, height
            {
                {0.2f, 0.2f, 2},  // 18
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0},  // 填充
                {0.0f, 0.0f, 0}   // 填充
            },
            1
        }
    };

/**
 * @brief 四舍五入浮点数到整数
 */
int round_float(float x) {
    return (int)(x + 0.5f);
}

/**
 * @brief 求最大值
 */
int max_int(int a, int b) {
    return (a > b) ? a : b;
}

/**
 * @brief 求最小值
 */
int min_int(int a, int b) {
    return (a < b) ? a : b;
}


int icon_totalCount = sizeof(g_available_icons) / sizeof(g_available_icons[0]);
// 
void initIconPositions() {
    for (int i = 0; i < 6; i++) {
        g_icon_positions[i].x = 0;
        g_icon_positions[i].y = 0;
        g_icon_positions[i].width = 0;
        g_icon_positions[i].height = 0;
        g_icon_positions[i].selected = false;
        g_icon_positions[i].icon_index = -1;
        g_icon_positions[i].data = NULL;
    }
    g_selected_icon_index = -1;
    g_global_icon_count = 0;
}
// 获取状态栏图标位置（独立函数）
void getStatusIconPositions(RectInfo *rects, int rect_count, int status_rect_index,
                           int* wifi_x, int* wifi_y, 
                           int* battery_x, int* battery_y) {
    const int wifi_orig_width = 32;
    const int wifi_orig_height = 32;
    const int battery_orig_width = 36;
    const int battery_orig_height = 24;
    const int status_spacing = 5;
    
    // 默认位置（右上角）
    int default_margin = 5;
    *wifi_x = 416 - default_margin - wifi_orig_width;
    *wifi_y = default_margin;
    *battery_x = *wifi_x - battery_orig_width - status_spacing;
    *battery_y = default_margin + (wifi_orig_height - battery_orig_height) / 2;
    
    // 如果指定了状态栏矩形，计算在矩形内的位置
    if (status_rect_index >= 0 && status_rect_index < rect_count) {
        RectInfo* status_rect = &rects[status_rect_index];
        
        if (status_rect->width >= (wifi_orig_width + battery_orig_width + status_spacing) &&
            status_rect->height >= (wifi_orig_height > battery_orig_height ? wifi_orig_height : battery_orig_height)) {
            
            *battery_x = status_rect->x + status_rect->width - battery_orig_width;
            *battery_y = status_rect->y + (status_rect->height - battery_orig_height) / 2;
            *wifi_x = *battery_x - wifi_orig_width - status_spacing;
            *wifi_y = status_rect->y + (status_rect->height - wifi_orig_height) / 2;
            
            if (*wifi_x < status_rect->x) {
                *wifi_x = status_rect->x;
                *battery_x = *wifi_x + wifi_orig_width + status_spacing;
            }
        }
    }
    
    // 边界检查
    if (*wifi_x < 0) *wifi_x = 0;
    if (*wifi_y < 0) *wifi_y = 0;
    if (*battery_x < 0) *battery_x = 0;
    if (*battery_y < 0) *battery_y = 0;
}

// 为单个矩形写入指定图标到指定位置
// rect: 目标矩形
// icon_index: 图标索引（0-5对应6个图标）
// rel_x: 相对x位置 (0.0-1.0)
// rel_y: 相对y位置 (0.0-1.0)
// 返回: 成功返回图标在全局数组中的索引，失败返回-1
int populateRectWithIcon(RectInfo* rect, int icon_index, float rel_x, float rel_y) {
    // 检查参数有效性
    if (rect == NULL) {
        ESP_LOGE("POPULATE_RECT", "矩形指针为空");
        return -1;
    }
    
    if (icon_index < 0 || icon_index >= (int)(sizeof(g_available_icons) / sizeof(IconInfo))) {
        ESP_LOGE("POPULATE_RECT", "图标索引无效: %d", icon_index);
        return -1;
    }
    
    if (rel_x < 0.0f || rel_x > 1.0f || rel_y < 0.0f || rel_y > 1.0f) {
        ESP_LOGE("POPULATE_RECT", "相对位置无效: (%.2f, %.2f)", rel_x, rel_y);
        return -1;
    }
    
    // 检查是否还有空间存储图标
    if (g_global_icon_count >= MAX_GLOBAL_ICONS) {
        ESP_LOGE("POPULATE_RECT", "已达到最大图标数量(20个)");
        return -1;
    }
    
    // 获取图标信息
    IconInfo* icon = &g_available_icons[icon_index];
    
    // 计算图标在矩形内的具体位置
    // int icon_x = rect->x + (int)((rect->width - icon->width) * rel_x);
    // int icon_y = rect->y + (int)((rect->height - icon->height) * rel_y);
       // 新代码（左上角对齐，匹配前端）：
    int icon_x = rect->x + (int)(rect->width * rel_x);
    int icon_y = rect->y + (int)(rect->height * rel_y);
    // 边界检查
    // if (icon_x < 0) {
    //     ESP_LOGW("POPULATE_RECT", "图标x坐标调整: %d -> 0", icon_x);
    //     icon_x = 0;
    // }
    
    // if (icon_y < 0) {
    //     ESP_LOGW("POPULATE_RECT", "图标y坐标调整: %d -> 0", icon_y);
    //     icon_y = 0;
    // }
    
    // if (icon_x + icon->width > 416) {
    //     int new_x = 416 - icon->width;
    //     ESP_LOGW("POPULATE_RECT", "图标x坐标超出屏幕右边界: %d -> %d", icon_x, new_x);
    //     icon_x = (new_x > 0) ? new_x : 0;
    // }
    
    // if (icon_y + icon->height > 240) {
    //     int new_y = 240 - icon->height;
    //     ESP_LOGW("POPULATE_RECT", "图标y坐标超出屏幕下边界: %d -> %d", icon_y, new_y);
    //     icon_y = (new_y > 0) ? new_y : 0;
    // }
    // 修改边界检查：
    if (icon_x < 0) {
        ESP_LOGW("POPULATE_RECT", "图标x坐标调整: %d -> 0", icon_x);
        icon_x = 0;
    }

    if (icon_y < 0) {
        ESP_LOGW("POPULATE_RECT", "图标y坐标调整: %d -> 0", icon_y);
        icon_y = 0;
    }

    if (icon_x + icon->width > 416) {
        int new_x = 416 - icon->width;
        ESP_LOGW("POPULATE_RECT", "图标x坐标超出屏幕右边界: %d -> %d", icon_x, new_x);
        icon_x = (new_x > 0) ? new_x : 0;
    }

    if (icon_y + icon->height > 240) {
        int new_y = 240 - icon->height;
        ESP_LOGW("POPULATE_RECT", "图标y坐标超出屏幕下边界: %d -> %d", icon_y, new_y);
        icon_y = (new_y > 0) ? new_y : 0;
    }
    // 记录到全局图标位置数组
    int global_index = g_global_icon_count;
    g_icon_positions[global_index].x = (uint16_t)icon_x;
    g_icon_positions[global_index].y = (uint16_t)icon_y;
    g_icon_positions[global_index].width = (uint16_t)icon->width;
    g_icon_positions[global_index].height = (uint16_t)icon->height;
    g_icon_positions[global_index].selected = false;
    g_icon_positions[global_index].icon_index = icon_index;
    g_icon_positions[global_index].data = icon->data;
    
    g_global_icon_count++;
    
ESP_LOGI("POPULATE_RECT", 
        "矩形[%d,%d %dx%d] 图标尺寸[%dx%d] rel=(%.2f,%.2f) 计算: x=%d+(%d*%.2f)=%d, y=%d+(%d*%.2f)=%d",
        rect->x, rect->y, rect->width, rect->height,
        icon->width, icon->height,
        rel_x, rel_y,
        rect->x, rect->width, rel_x, icon_x,
        rect->y, rect->height, rel_y, icon_y);
    
    return global_index;
}

// 选择指定索引的图标
bool selectIcon(int icon_index) {
    if (icon_index < 0 || icon_index >= g_global_icon_count) {
        ESP_LOGE("SELECT_ICON", "图标索引无效: %d", icon_index);
        return false;
    }
    
    // 取消之前选中的图标
    if (g_selected_icon_index >= 0 && g_selected_icon_index < g_global_icon_count) {
        g_icon_positions[g_selected_icon_index].selected = false;
    }
    
    // 选中新的图标
    g_selected_icon_index = icon_index;
    g_icon_positions[icon_index].selected = true;
    
    ESP_LOGI("SELECT_ICON", "图标%d被选中，位置(%d,%d)", 
            icon_index, 
            g_icon_positions[icon_index].x, 
            g_icon_positions[icon_index].y);
    
    return true;
}
// 新增：为特定矩形添加图标（最灵活的API）
int addIconToRect(RectInfo *rects, int rect_index, int rect_count,
                  int icon_index, float rel_x, float rel_y) {
    // 检查矩形索引是否有效
    if (rect_index < 0 || rect_index >= rect_count) {
        ESP_LOGE("ADD_ICON", "矩形索引无效: %d", rect_index);
        return -1;
    }
    
    // 检查是否还有空间
    if (g_global_icon_count >= 6) {
        ESP_LOGE("ADD_ICON", "已达到最大图标数量(6个)");
        return -1;
    }
    
    // 填充图标
    return populateRectWithIcon(&rects[rect_index], icon_index, rel_x, rel_y);
}

// 批量添加多个图标
void addIconsToRects(RectInfo *rects, int rect_count, 
                     int rect_indices[], int icon_indices[],
                     float rel_xs[], float rel_ys[], int icon_count) {
    for (int i = 0; i < icon_count && g_global_icon_count < 6; i++) {
        int result = addIconToRect(rects, rect_indices[i], rect_count,
                                 icon_indices[i], rel_xs[i], rel_ys[i]);
        
        if (result < 0) {
            ESP_LOGW("ADD_ICONS", "添加图标%d失败", i);
        }
    }
}

void populateRectsWithCustomIcons(RectInfo *rects, int rect_count, 
                                  IconConfig* icon_configs, int config_count) {
    // 重置全局图标计数
    initIconPositions();
    
    for (int i = 0; i < config_count && g_global_icon_count < 6; i++) {
        IconConfig* config = &icon_configs[i];
        
        // 检查矩形索引是否有效
        if (config->rect_index < 0 || config->rect_index >= rect_count) {
            ESP_LOGE("POPULATE_CUSTOM", "矩形索引无效: %d", config->rect_index);
            continue;
        }
        
        // 填充图标
        int result = populateRectWithIcon(&rects[config->rect_index], 
                                        config->icon_index, 
                                        config->rel_x, 
                                        config->rel_y);
        
        if (result < 0) {
            ESP_LOGE("POPULATE_CUSTOM", "填充图标失败: 矩形%d, 图标%d", 
                    config->rect_index, config->icon_index);
        }
    }
}



// 判断点是否在图标区域内
int findIconAtPoint(int x, int y) {
    for (int i = 0; i < g_global_icon_count; i++) {
        if (g_icon_positions[i].width > 0 && g_icon_positions[i].height > 0) {
            if (x >= g_icon_positions[i].x && 
                x <= g_icon_positions[i].x + g_icon_positions[i].width &&
                y >= g_icon_positions[i].y && 
                y <= g_icon_positions[i].y + g_icon_positions[i].height) {
                return i;  // 返回图标索引
            }
        }
    }
    return -1;  // 未找到
}

// 显示主界面（使用已填充的图标数据）
void displayMainScreen(RectInfo *rects, int rect_count, int status_rect_index, int show_border) {
    // 计算缩放比例
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    ESP_LOGI("DISPLAY", "屏幕尺寸: %dx%d, 缩放比例: %.4f, 图标数量: %d", 
            setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt, 
            global_scale, g_global_icon_count);
    
    // 初始化显示
    EPD_FastInit();
    EPD_Display_Clear();
    EPD_Update();
    delay_ms(50);
    EPD_PartInit();
    
    // ==================== 显示状态栏图标 ====================
    int wifi_x, wifi_y, battery_x, battery_y;
    getStatusIconPositions(rects, rect_count, status_rect_index, 
                          &wifi_x, &wifi_y, &battery_x, &battery_y);

    EPD_ShowPictureScaled(wifi_x, wifi_y, 32, 32, 
                         ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK);
    
    #ifdef BATTERY_LEVEL
        const uint8_t* battery_icon = NULL;
        if (BATTERY_LEVEL >= 80) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_4;
        else if (BATTERY_LEVEL >= 60) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_3;
        else if (BATTERY_LEVEL >= 40) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_2;
        else if (BATTERY_LEVEL >= 20) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
        else battery_icon = ZHONGJINGYUAN_3_7_BATTERY_0;
        
        EPD_ShowPictureScaled(battery_x, battery_y, 36, 24, battery_icon, BLACK);
    #else
        EPD_ShowPictureScaled(battery_x, battery_y, 36, 24, 
                             ZHONGJINGYUAN_3_7_BATTERY_1, BLACK);
    #endif
    
    // ==================== 显示所有图标 ====================
    int displayed_icon_count = 0;
    for (int i = 0; i < g_global_icon_count; i++) {
        if (g_icon_positions[i].width > 0 && g_icon_positions[i].height > 0) {
            // 使用存储的图标数据直接显示
            if (g_icon_positions[i].data != NULL) {
                EPD_ShowPictureScaled(g_icon_positions[i].x, g_icon_positions[i].y, 
                                     g_icon_positions[i].width, g_icon_positions[i].height, 
                                     g_icon_positions[i].data, BLACK);
                displayed_icon_count++;
                
                ESP_LOGI("DISPLAY_ICON", "显示图标%d: 位置(%d,%d), 尺寸(%dx%d), 图标索引%d",
                        i, g_icon_positions[i].x, g_icon_positions[i].y,
                        g_icon_positions[i].width, g_icon_positions[i].height,
                        g_icon_positions[i].icon_index);
            }
            
            // 如果图标被选中，显示选中标记
            if (g_icon_positions[i].selected) {
                // 绘制选中框
                int border_x = (int)(g_icon_positions[i].x * global_scale);
                int border_y = (int)(g_icon_positions[i].y * global_scale);
                int border_width = (int)(g_icon_positions[i].width * global_scale);
                int border_height = (int)(g_icon_positions[i].height * global_scale);
                
                EPD_DrawRectangle(border_x, border_y, 
                                 border_x + border_width - 1, 
                                 border_y + border_height - 1, 
                                 BLACK, 0);
            }
        }
    }
    
    // ==================== 显示矩形边框 ====================
    if (show_border) {
        ESP_LOGI("BORDER", "开始绘制矩形边框，共%d个矩形", rect_count);
        
        for (int i = 0; i < rect_count; i++) {
            RectInfo* rect = &rects[i];
            
            // 计算缩放后的边框位置
            int border_display_x = (int)(rect->x * global_scale + 0.5f);
            int border_display_y = (int)(rect->y * global_scale + 0.5f);
            int border_display_width = (int)(rect->width * global_scale + 0.5f);
            int border_display_height = (int)(rect->height * global_scale + 0.5f);
            
            // 边界检查
            if (border_display_x < 0) border_display_x = 0;
            if (border_display_y < 0) border_display_y = 0;
            if (border_display_x + border_display_width > setInkScreenSize.screenWidth) {
                border_display_width = setInkScreenSize.screenWidth - border_display_x;
            }
            if (border_display_y + border_display_height > setInkScreenSize.screenHeigt) {
                border_display_height = setInkScreenSize.screenHeigt - border_display_y;
            }
            
            if (border_display_width > 0 && border_display_height > 0) {
                uint16_t border_color = BLACK;
                int border_thickness = (i == status_rect_index) ? 2 : 1;
                
                // 绘制矩形边框
                EPD_DrawRectangle(border_display_x, border_display_y, 
                                 border_display_x + border_display_width - 1, 
                                 border_display_y + border_display_height - 1, 
                                 border_color, 0);
            }
        }
    }
    
    // ==================== 显示选中的图标下划线 ====================
    if (g_selected_icon_index >= 0 && g_selected_icon_index < g_global_icon_count) {
        IconPosition* selected = &g_icon_positions[g_selected_icon_index];
        if (selected->width > 0 && selected->height > 0) {
            int underline_orig_x = selected->x;
            int underline_orig_y = selected->y + selected->height + 2;  // 加2像素间隔
            int underline_orig_width = selected->width;
            
            int underline_display_x = (int)(underline_orig_x * global_scale + 0.5f);
            int underline_display_y = (int)(underline_orig_y * global_scale + 0.5f);
            int underline_display_width = (int)(underline_orig_width * global_scale + 0.5f);
            
            if (underline_display_width < 3) underline_display_width = 3;
            
            if (underline_display_x >= 0 && 
                underline_display_x + underline_display_width <= setInkScreenSize.screenWidth &&
                underline_display_y >= 0 && 
                underline_display_y < setInkScreenSize.screenHeigt) {
                
                // 绘制下划线
                EPD_DrawLine(underline_display_x, underline_display_y, 
                            underline_display_x + underline_display_width, underline_display_y, 
                            BLACK);
                
                // 加粗下划线（绘制两条线）
                EPD_DrawLine(underline_display_x, underline_display_y + 1, 
                            underline_display_x + underline_display_width, underline_display_y + 1, 
                            BLACK);
            }
        }
    }
    
    // 更新显示
    EPD_Display(ImageBW);
    EPD_Update();
    EPD_DeepSleep();
    delay_ms(100);
    
    ESP_LOGI("DISPLAY", "显示完成: 共显示%d个图标", displayed_icon_count);
}

// 原来的主函数，保持兼容性
void updateDisplayWithMain(RectInfo *rects, int rect_count, int status_rect_index, int show_border) {

    // 1. 重置图标
    initIconPositions();
   // clearEntireScreen();
    // 2. 直接使用rects数组中的图标配置
    // 遍历所有矩形（除了状态栏）
    for (int i = 0; i < rect_count; i++) {
        // 跳过状态栏矩形
        if (i == status_rect_index) {
            continue;
        }
        
        RectInfo *rect = &rects[i];
        
        // 如果有图标配置，就设置
        for (int j = 0; j < rect->icon_count; j++) {
            IconPositionInRect *icon = &rect->icons[j];
            
            // 直接使用存储的相对位置
            float rel_x = icon->rel_x;
            float rel_y = icon->rel_y;
            int icon_index = icon->icon_index;
            
            // 边界检查
            if (rel_x < 0.0f) rel_x = 0.0f;
            if (rel_x > 1.0f) rel_x = 1.0f;
            if (rel_y < 0.0f) rel_y = 0.0f;
            if (rel_y > 1.0f) rel_y = 1.0f;
            if (icon_index < 0) icon_index = 0;
            if (icon_index >= 6) icon_index = 5;
            
            // 设置图标
            populateRectWithIcon(rect, icon_index, rel_x, rel_y);
        }
    }
    // 4. 显示
    displayMainScreen(rects, rect_count, status_rect_index, show_border);
}
#define DEBUG_LAYOUT 1



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
            clearDisplayArea(30,10,340,30);
            updateDisplayWithString(30,10, tempPrompt,16,BLACK);
            EPD_Display(ImageBW);
            EPD_Update();
            EPD_DeepSleep();
            vTaskDelay(1000);
        } else {
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









// 新增函数：在指定矩形区域内显示图标
void displayIconsInRectangle(int rect_x, int rect_y, int rect_width, int rect_height) {
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
    
    // 获取全局缩放比例
    float global_scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float global_scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (global_scale_x < global_scale_y) ? global_scale_x : global_scale_y;
    
    // 将矩形坐标转换为原始坐标（416×240系统）
    int rect_orig_x = (int)(rect_x / global_scale);
    int rect_orig_y = (int)(rect_y / global_scale);
    int rect_orig_width = (int)(rect_width / global_scale);
    int rect_orig_height = (int)(rect_height / global_scale);
    
    // 边界检查
    if (rect_orig_x < 0) rect_orig_x = 0;
    if (rect_orig_y < 0) rect_orig_y = 0;
    if (rect_orig_x + rect_orig_width > 416) rect_orig_width = 416 - rect_orig_x;
    if (rect_orig_y + rect_orig_height > 240) rect_orig_height = 240 - rect_orig_y;
    
    ESP_LOGI("RECT_ICONS", "矩形区域: 实际(%d,%d,%dx%d) -> 原始(%d,%d,%dx%d)", 
            rect_x, rect_y, rect_width, rect_height,
            rect_orig_x, rect_orig_y, rect_orig_width, rect_orig_height);
    
    // ==================== 计算图标在矩形内的布局 ====================
    
    // 计算所有图标的最大尺寸
    int max_icon_width = 0;
    int max_icon_height = 0;
    for (int i = 0; i < icon_count; i++) {
        if (icons[i].width > max_icon_width) max_icon_width = icons[i].width;
        if (icons[i].height > max_icon_height) max_icon_height = icons[i].height;
    }
    
    // 计算可用的矩形区域（考虑内边距）
    int margin = 5;
    int available_width = rect_orig_width - 2 * margin;
    int available_height = rect_orig_height - 2 * margin;
    
    // 计算布局
    int icon_spacing_x = 10;
    int icon_spacing_y = 10;
    
    // 计算每行最多能放几个图标
    int max_cols = (available_width + icon_spacing_x) / (max_icon_width + icon_spacing_x);
    max_cols = (max_cols < 1) ? 1 : max_cols;
    max_cols = (max_cols > icon_count) ? icon_count : max_cols;
    
    int grid_cols = max_cols;
    int grid_rows = (icon_count + grid_cols - 1) / grid_cols;
    
    // 如果高度不够，尝试增加列数减少行数
    while (grid_rows > 1 && 
           (max_icon_height * grid_rows + icon_spacing_y * (grid_rows + 1)) > available_height &&
           grid_cols < icon_count) {
        grid_cols++;
        grid_rows = (icon_count + grid_cols - 1) / grid_cols;
    }
    
    // 重新计算单元格尺寸
    int cell_width = (available_width - (grid_cols + 1) * icon_spacing_x) / grid_cols;
    int cell_height = (available_height - (grid_rows + 1) * icon_spacing_y) / grid_rows;
    
    // 确保单元格足够大
    if (cell_width < max_icon_width) cell_width = max_icon_width + 10;
    if (cell_height < max_icon_height) cell_height = max_icon_height + 10;
    
    // 重新计算间距
    icon_spacing_x = (available_width - grid_cols * cell_width) / (grid_cols + 1);
    icon_spacing_y = (available_height - grid_rows * cell_height) / (grid_rows + 1);
    
    // 确保最小间距
    if (icon_spacing_x < 5) icon_spacing_x = 5;
    if (icon_spacing_y < 5) icon_spacing_y = 5;
    
    ESP_LOGI("RECT_LAYOUT", "矩形内布局: %d列 x %d行", grid_cols, grid_rows);
    ESP_LOGI("RECT_LAYOUT", "单元格: %dx%d, 间距: %dx%d", 
            cell_width, cell_height, icon_spacing_x, icon_spacing_y);
    
    // ==================== 在矩形内显示图标 ====================
    
    for (int i = 0; i < icon_count; i++) {
        // 计算图标在网格中的位置
        int row = i / grid_cols;
        int col = i % grid_cols;
        
        // 如果超过最大行数，停止显示
        if (row >= grid_rows) {
            ESP_LOGW("RECT_ICONS", "图标%d超出矩形范围，停止显示", i);
            break;
        }
        
        // 计算在矩形内的原始坐标
        int icon_in_rect_x = rect_orig_x + margin + icon_spacing_x + 
                           col * (cell_width + icon_spacing_x);
        int icon_in_rect_y = rect_orig_y + margin + icon_spacing_y + 
                           row * (cell_height + icon_spacing_y);
        
        // 计算图标在单元格内的居中位置
        int icon_orig_x = icon_in_rect_x + (cell_width - icons[i].width) / 2;
        int icon_orig_y = icon_in_rect_y + (cell_height - icons[i].height) / 2;
        
        // 边界检查
        if (icon_orig_x < rect_orig_x || icon_orig_x + icons[i].width > rect_orig_x + rect_orig_width ||
            icon_orig_y < rect_orig_y || icon_orig_y + icons[i].height > rect_orig_y + rect_orig_height) {
            ESP_LOGE("RECT_ICONS", "图标%d在矩形内位置超出范围", i);
            continue;
        }
        
        // 使用缩放函数显示图标
        EPD_ShowPictureScaled(icon_orig_x, icon_orig_y, 
                             icons[i].width, icons[i].height,
                             icons[i].data, BLACK);
        
        // 调试信息
        int display_x = (int)(icon_orig_x * global_scale);
        int display_y = (int)(icon_orig_y * global_scale);
        ESP_LOGI("RECT_ICONS", "图标%d: 矩形内原始位置(%d,%d) -> 显示位置(%d,%d)", 
                i, icon_orig_x, icon_orig_y, display_x, display_y);
    }
}

// 修改后的 drawSmartRectangle 函数
bool drawSmartRectangle(int x, int y, int width, int height, uint16_t color, int fill) {
    // 获取当前屏幕的缩放比例
    float global_scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float global_scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (global_scale_x < global_scale_y) ? global_scale_x : global_scale_y;
    
    // 计算缩放后的实际显示坐标和尺寸
    int display_x = (int)(x * global_scale);
    int display_y = (int)(y * global_scale);
    int display_width = (int)(width * global_scale);
    int display_height = (int)(height * global_scale);
    
    // 确保最小尺寸
    if (display_width < 1) display_width = 1;
    if (display_height < 1) display_height = 1;
    
    // 边界检查：矩形是否完全在屏幕内
    if (display_x < 0 || display_y < 0 || 
        display_x >= setInkScreenSize.screenWidth || 
        display_y >= setInkScreenSize.screenHeigt) {
        ESP_LOGE(TAG, "矩形左上角超出屏幕范围: (%d,%d), 屏幕尺寸: %dx%d", 
                display_x, display_y, 
                setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
        return false;
    }
    
    // 计算矩形右下角坐标
    int right = display_x + display_width - 1;
    int bottom = display_y + display_height - 1;
    
    // 边界检查：矩形右下角是否超出屏幕
    if (right >= setInkScreenSize.screenWidth || bottom >= setInkScreenSize.screenHeigt) {
        ESP_LOGW(TAG, "矩形部分超出屏幕，进行裁剪调整");
        
        // 裁剪矩形到屏幕范围内
        if (right >= setInkScreenSize.screenWidth) {
            right = setInkScreenSize.screenWidth - 1;
            display_width = right - display_x + 1;
        }
        if (bottom >= setInkScreenSize.screenHeigt) {
            bottom = setInkScreenSize.screenHeigt - 1;
            display_height = bottom - display_y + 1;
        }
        
        // 如果裁剪后矩形尺寸为0，则不绘制
        if (display_width <= 0 || display_height <= 0) {
            ESP_LOGE(TAG, "裁剪后矩形尺寸无效: %dx%d", display_width, display_height);
            return false;
        }
        
        ESP_LOGI(TAG, "矩形已裁剪: 新尺寸(%d,%d,%dx%d)", 
                display_x, display_y, display_width, display_height);
    }
    
    // 绘制矩形
    if (fill) {
        // 绘制填充矩形
        for (int i = 0; i < display_height; i++) {
            EPD_DrawLine(display_x, display_y + i, 
                        display_x + display_width - 1, display_y + i, color);
        }
    } else {
        // 绘制空心矩形
        EPD_DrawRectangle(display_x, display_y, 
                         display_x + display_width - 1, 
                         display_y + display_height - 1, 
                         color, 0);
    }
    
    // 记录绘制信息
    ESP_LOGI(TAG, "绘制矩形: 原始(%d,%d,%dx%d) -> 显示(%d,%d,%dx%d), 颜色:%d, 填充:%d", 
            x, y, width, height,
            display_x, display_y, display_width, display_height,
            color, fill);
    
    // ==================== 在矩形内显示图标 ====================
    // 注意：这里需要根据需求决定是否显示图标
    // 如果矩形足够大，就在里面显示图标
    if (display_width > 100 && display_height > 80) {
        // 显示图标（传入原始坐标，函数内部会处理缩放）
        displayIconsInRectangle(x, y, width, height);
        
        ESP_LOGI(TAG, "在矩形(%d,%d,%dx%d)内显示图标", 
                display_x, display_y, display_width, display_height);
    }
    
    return true;
}

/**
 * @brief 批量绘制多个矩形
 * @param rects 矩形数组，每个矩形包含{x, y, width, height}
 * @param count 矩形数量
 * @param color 矩形颜色
 * @param fill 是否填充
 * @return 成功绘制的矩形数量
 */
int drawMultipleRectangles(RectInfo* rects, int count, uint16_t color, int fill) {
    int success_count = 0;
    
    for (int i = 0; i < count; i++) {
        if (drawSmartRectangle(rects[i].x, rects[i].y, 
                              rects[i].width, rects[i].height, 
                              color, fill)) {
            success_count++;
        } else {
            ESP_LOGE(TAG, "矩形%d绘制失败: (%d,%d,%dx%d)", 
                    i, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
        }
    }
    
    ESP_LOGI(TAG, "批量绘制完成: 总数%d, 成功%d", count, success_count);
    return success_count;
}

void drawGridRectangles(int grid_cols, int grid_rows, 
                       int cell_width, int cell_height, 
                       int margin, int spacing) {
    // 计算缩放比例
    float global_scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float global_scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (global_scale_x < global_scale_y) ? global_scale_x : global_scale_y;
    
    // 计算显示尺寸
    int display_cell_width = (int)(cell_width * global_scale);
    int display_cell_height = (int)(cell_height * global_scale);
    int display_margin = (int)(margin * global_scale);
    int display_spacing = (int)(spacing * global_scale);
    
    // 绘制网格线
    for (int row = 0; row <= grid_rows; row++) {
        int y = display_margin + row * (display_cell_height + display_spacing);
        EPD_DrawLine(display_margin, y, 
                    display_margin + grid_cols * (display_cell_width + display_spacing), 
                    y, BLACK);
    }
    
    for (int col = 0; col <= grid_cols; col++) {
        int x = display_margin + col * (display_cell_width + display_spacing);
        EPD_DrawLine(x, display_margin, 
                    x, display_margin + grid_rows * (display_cell_height + display_spacing), 
                    BLACK);
    }
    
    // 绘制每个单元格的矩形
    for (int row = 0; row < grid_rows; row++) {
        for (int col = 0; col < grid_cols; col++) {
            int cell_x = display_margin + col * (display_cell_width + display_spacing);
            int cell_y = display_margin + row * (display_cell_height + display_spacing);
            
            // 绘制空心矩形
            EPD_DrawRectangle(cell_x, cell_y, 
                            cell_x + display_cell_width - 1, 
                            cell_y + display_cell_height - 1, 
                            BLACK, 1);
        }
    }
    
    ESP_LOGI(TAG, "网格矩形绘制完成: %d列×%d行, 单元格%d×%d", 
            grid_cols, grid_rows, display_cell_width, display_cell_height);
}

/**
 * @brief 绘制带边框的图标区域（用于可视化图标位置）
 * @param icon_index 图标索引
 * @param border_color 边框颜色
 */
void drawIconBorder(int icon_index, uint16_t border_color) {
    if (icon_index < 0 || icon_index >= 6) {
        ESP_LOGE(TAG, "无效的图标索引: %d", icon_index);
        return;
    }
    
    IconPosition* icon = &g_icon_positions[icon_index];
    
    if (icon->width == 0 || icon->height == 0) {
        ESP_LOGE(TAG, "图标%d位置未初始化", icon_index);
        return;
    }
    
    // 在图标周围绘制一个边框
    int border_thickness = 2;
    
    // 计算缩放比例
    float global_scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float global_scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (global_scale_x < global_scale_y) ? global_scale_x : global_scale_y;
    
    // 计算显示位置
    int x = (int)(icon->x * global_scale);
    int y = (int)(icon->y * global_scale);
    int width = (int)(icon->width * global_scale);
    int height = (int)(icon->height * global_scale);
    
    // 绘制外边框
    for (int i = 0; i < border_thickness; i++) {
        // 上边框
        EPD_DrawLine(x - border_thickness + i, y - border_thickness, 
                    x + width + border_thickness - i, y - border_thickness, 
                    border_color);
        // 下边框
        EPD_DrawLine(x - border_thickness + i, y + height + border_thickness, 
                    x + width + border_thickness - i, y + height + border_thickness, 
                    border_color);
        // 左边框
        EPD_DrawLine(x - border_thickness, y - border_thickness + i, 
                    x - border_thickness, y + height + border_thickness - i, 
                    border_color);
        // 右边框
        EPD_DrawLine(x + width + border_thickness, y - border_thickness + i, 
                    x + width + border_thickness, y + height + border_thickness - i, 
                    border_color);
    }
    
    ESP_LOGI(TAG, "图标%d边框绘制完成: 位置(%d,%d), 尺寸(%dx%d), 边框厚度%d", 
            icon_index, x, y, width, height, border_thickness);
}

void showSimplePromptWithNail(uint8_t *tempPrompt, int bg_x, int bg_y) {
    if (tempPrompt == nullptr) {
        return;
    }
    
    const char* promptText = (const char*)tempPrompt;
    
    // 固定参数
    int bg_width = 320;     // 背景框宽度
    int bg_height = 36;     // 背景框高度
    int border_margin = 6;  // 边框边距
    int icon_width = 15;    // 图标宽度
    int icon_height = 16;   // 图标高度
    int icon_margin = 10;   // 图标左边距
    int spacing = 8;        // 图标文字间距
    int font_size = 16;     // 字体大小
    
    // 1. 清空区域
    clearDisplayArea(bg_x, bg_y, bg_width, bg_height);
    
    // 2. 显示背景框
    EPD_ShowPictureScaled(bg_x, bg_y, bg_width, bg_height,
                         ZHONGJINGYUAN_3_7_promt, BLACK);
    
    // 3. 计算内部区域
    int inner_x = bg_x + border_margin;
    int inner_y = bg_y + border_margin;
    int inner_width = bg_width - 2 * border_margin;
    int inner_height = bg_height - 2 * border_margin;
    
    // 4. 显示图标（左侧固定位置）
    int icon_x = inner_x + icon_margin;
    int icon_y = inner_y + (inner_height - icon_height) / 2;
    
    EPD_ShowPictureScaled(icon_x, icon_y, icon_width, icon_height,
                         ZHONGJINGYUAN_3_7_NAIL, BLACK);
    
    // 5. 计算文本位置（图标右侧）
    int text_x = icon_x + icon_width + spacing;
    int text_y = inner_y + (inner_height - font_size) / 2;
    
    // 6. 检查文本宽度
    int text_width = calculateTextWidth(promptText, font_size);
    int max_width = inner_x + inner_width - text_x;
    
    if (text_width > max_width) {
        // 文本太长，截断
        int max_chars = max_width / (font_size / 2);
        if (max_chars > 3) {
            char truncated[256];
            strncpy(truncated, promptText, max_chars - 3);
            truncated[max_chars - 3] = '\0';
            strcat(truncated, "...");
            
            promptText = truncated;
            text_width = calculateTextWidth(truncated, font_size);
        }
    }
    
    // 7. 显示文本
    updateDisplayWithString(text_x, text_y, 
                          (uint8_t*)promptText, font_size, BLACK);
    
    ESP_LOGI("PROMPT", "显示提示: 图标(%d,%d) 文本(%d,%d) '%s'",
            icon_x, icon_y, text_x, text_y, promptText);
}
//加单词本显示函数
// 显示单词屏幕
void displayVocabularyScreen(RectInfo *rects, int rect_count, 
                           int status_rect_index, int show_border) {
    

    // 计算缩放比例
    float scale_x = (float)setInkScreenSize.screenWidth / 416.0f;
    float scale_y = (float)setInkScreenSize.screenHeigt / 240.0f;
    float global_scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    ESP_LOGI("VOCAB", "屏幕尺寸: %dx%d, 缩放比例: %.4f", 
            setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt, global_scale);
    
    // 初始化显示
    EPD_FastInit();
    EPD_Display_Clear();
    EPD_Update();
    delay_ms(50);
    EPD_PartInit();
    
    // ==================== 显示状态栏图标 ====================
    if (status_rect_index >= 0 && rect_count > 0) {
        int wifi_x, wifi_y, battery_x, battery_y;
        getStatusIconPositions(rects, rect_count, status_rect_index,
                              &wifi_x, &wifi_y, &battery_x, &battery_y);
        
        // 计算缩放后的位置
        int wifi_display_x = (int)(wifi_x * global_scale + 0.5f);
        int wifi_display_y = (int)(wifi_y * global_scale + 0.5f);
        int battery_display_x = (int)(battery_x * global_scale + 0.5f);
        int battery_display_y = (int)(battery_y * global_scale + 0.5f);
        
        // 显示WiFi图标
        EPD_ShowPictureScaled(wifi_display_x, wifi_display_y, 32, 32, 
                             ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK);
        
        #ifdef BATTERY_LEVEL
            const uint8_t* battery_icon = NULL;
            if (BATTERY_LEVEL >= 80) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;  // 使用BATTERY_1
            else if (BATTERY_LEVEL >= 60) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
            else if (BATTERY_LEVEL >= 40) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
            else if (BATTERY_LEVEL >= 20) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
            else battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
            
            EPD_ShowPictureScaled(battery_display_x, battery_display_y, 36, 24, battery_icon, BLACK);
        #else
            EPD_ShowPictureScaled(battery_display_x, battery_display_y, 36, 24, 
                                 ZHONGJINGYUAN_3_7_BATTERY_1, BLACK);
        #endif
    }
    
    // ==================== 矩形0：状态栏区域 - 显示提示信息 ====================
    if (rect_count > 0) {
        uint8_t *tempPrompt = showPrompt;
        // 检查输入有效性
        if (tempPrompt != nullptr) {
        //   ESP_LOGW("PROMPT", "接收到非空指针");
        
            RectInfo* rect = &rects[0];
            int display_x = (int)(rect->x * global_scale + 0.5f);
            int display_y = (int)(rect->y * global_scale + 0.5f);
            int display_width = (int)(rect->width * global_scale + 0.5f);
            int display_height = (int)(rect->height * global_scale + 0.5f);
            
            // 显示提示图标 (图标14) 
            if (rect->icon_count > 0) {

                showSimplePromptWithNail(tempPrompt, display_x, display_y);
            }
        }
    }
    
    // ==================== 矩形1：右上区域 - 显示WiFi电池组合图标 ====================
    if (rect_count > 1) {
        RectInfo* rect = &rects[1];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        
        // 显示WiFi电池组合图标 (图标15)
        if (rect->icon_count > 0) {
            EPD_ShowPictureScaled(display_x, display_y, 80, 36, 
                                 ZHONGJINGYUAN_3_7_wifi_battry, BLACK);
        }
    }

    readAndPrintRandomWord();
    if(entry.word.length() > 0) {
        safeDisplayWordEntry(entry, 20, 60);
        ESP_LOGI("VOCAB", "显示单词: %s", entry.word.c_str());
     } else {
     	ESP_LOGW("DISPLAY", "跳过空单词显示");
     }

    // ==================== 矩形2：中左区域 - 显示英文单词 ====================
    if (rect_count > 2) {
        RectInfo* rect = &rects[2];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        int display_width = (int)(rect->width * global_scale + 0.5f);
        int display_height = (int)(rect->height * global_scale + 0.5f);
        
        // 1. 显示背景图标并清空区域
        if (rect->icon_count > 0) {
            EPD_ShowPictureScaled(display_x, display_y, 336, 48, 
                                ZHONGJINGYUAN_3_7_word, BLACK);
            
            // 注意：clearDisplayArea的参数顺序通常是(x, y, width, height)
            clearDisplayArea(display_x, display_y, 336, 48);
        }
        
        // 2. 准备要显示的单词
        String word_to_display = entry.word;
        if (word_to_display.length() == 0) {
            word_to_display = "No Word";
        }
        
        // 3. 尝试不同的显示方案
        bool word_displayed = false;
        uint8_t font_sizes[] = {24, 20, 16, 14, 12}; // 尝试的字体大小
        int text_x = 0, text_y = 0;
        
        for (int i = 0; i < sizeof(font_sizes)/sizeof(font_sizes[0]) && !word_displayed; i++) {
            uint8_t font_size = font_sizes[i];
            
            // 计算字体高度和垂直位置
            int font_height = font_size;
            text_y = display_y + (display_height - font_height) / 2;
            
            // 计算单词宽度
            uint16_t word_width = calculateTextWidth(word_to_display.c_str(), font_size);
            
            // 如果宽度合适，居中显示
            if (word_width <= display_width - 10) { // 留10像素边距
                text_x = display_x + (display_width - word_width) / 2;
                updateDisplayWithString(text_x, text_y, (uint8_t*)word_to_display.c_str(), font_size, BLACK);
                word_displayed = true;
                
                ESP_LOGI("VOCAB", "使用字体大小%d显示单词: %s", font_size, word_to_display.c_str());
                break;
            }
        }
        
        // 4. 如果所有字体都太大，截断显示
        if (!word_displayed) {
            uint8_t font_size = 12;
            int font_height = font_size;
            text_y = display_y + (display_height - font_height) / 2;
            
            // 计算可以显示的字符数
            int avg_char_width = 6; // 12px字体下平均字符宽度
            int max_chars = (display_width - 10) / avg_char_width;
            
            if (max_chars > 3) {
                // 保留末尾3个字符用于"..."
                int keep_chars = max_chars - 3;
                String truncated_word = word_to_display.substring(0, keep_chars) + "...";
                
                uint16_t truncated_width = calculateTextWidth(truncated_word.c_str(), font_size);
                text_x = display_x + (display_width - truncated_width) / 2;
                
                updateDisplayWithString(text_x, text_y, (uint8_t*)truncated_word.c_str(), font_size, BLACK);
                
                ESP_LOGI("VOCAB", "截断显示单词: %s", truncated_word.c_str());
            } else {
                // 空间太小，只显示省略号
                text_x = display_x + (display_width - 20) / 2; // 省略号宽度大约20像素
                updateDisplayWithString(text_x, text_y, (uint8_t*)"...", font_size, BLACK);
                ESP_LOGW("VOCAB", "空间不足，只显示省略号");
            }
            word_displayed = true;
        }
        
        // 5. 记录显示位置（用于调试）
        ESP_LOGI("VOCAB", "显示英文单词: %s 在位置(%d,%d)", 
                word_to_display.c_str(), text_x, text_y);
    }
    
    // ==================== 矩形4：左下区域 - 显示音标 ====================
    if (rect_count > 4) {
        RectInfo* rect = &rects[4];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        
        // 先清空音标显示区域
        clearDisplayArea(display_x, display_y, 80, 32);
        
        // 显示音标背景图标 (图标20)
        if (rect->icon_count > 0) {
            EPD_ShowPictureScaled(display_x, display_y, 80, 32, 
                                ZHONGJINGYUAN_3_7_pon, BLACK);
        }
        
        // 音标文字的位置变量
        int phonetic_text_x = 0;
        int phonetic_text_y = 0;
        uint8_t phonetic_font_size = 0;
        
        if(entry.phonetic.length() > 0) {
            String formattedPhonetic = formatPhonetic(entry.phonetic);
            
            // 先尝试24px字体
            uint16_t phonetic_width_24 = calculateTextWidth(formattedPhonetic.c_str(), 24);
            if(phonetic_width_24 < 80 - 10) { // 留10像素边距
                phonetic_font_size = 24;
                phonetic_text_x = display_x + (80 - phonetic_width_24) / 2;
                phonetic_text_y = display_y + 4; // 24px字体位置
            } 
            // 再尝试16px字体
            else {
                uint16_t phonetic_width_16 = calculateTextWidth(formattedPhonetic.c_str(), 16);
                if(phonetic_width_16 < 80 - 10) {
                    phonetic_font_size = 16;
                    phonetic_text_x = display_x + (80 - phonetic_width_16) / 2;
                    phonetic_text_y = display_y + 8; // 16px字体位置
                } 
                // 最后尝试12px字体
                else {
                    uint16_t phonetic_width_12 = calculateTextWidth(formattedPhonetic.c_str(), 12);
                    if(phonetic_width_12 < 80 - 10) {
                        phonetic_font_size = 12;
                        phonetic_text_x = display_x + (80 - phonetic_width_12) / 2;
                        phonetic_text_y = display_y + 10; // 12px字体位置
                    } 
                    else {
                        // 音标太长，截断显示
                        int max_chars = 8; // 12px字体下大约显示8个字符
                        if (formattedPhonetic.length() > max_chars) {
                            formattedPhonetic = formattedPhonetic.substring(0, max_chars) + "...";
                        }
                        phonetic_font_size = 12;
                        uint16_t truncated_width = calculateTextWidth(formattedPhonetic.c_str(), 12);
                        phonetic_text_x = display_x + (80 - truncated_width) / 2;
                        phonetic_text_y = display_y + 10;
                    }
                }
            }
            
            // 显示音标文字
            if (phonetic_font_size > 0) {
                updateDisplayWithString(phonetic_text_x, phonetic_text_y, 
                                    (uint8_t*)formattedPhonetic.c_str(), 
                                    phonetic_font_size, BLACK);
                ESP_LOGI("VOCAB", "显示音标: %s 在位置(%d,%d) 字体大小:%d", 
                        formattedPhonetic.c_str(), phonetic_text_x, phonetic_text_y, phonetic_font_size);
                
                // ==================== 显示喇叭图标在音标正上方偏右 ====================
                // 计算喇叭图标位置
                int horn_width = 15;
                int horn_height = 16;
                
                // 喇叭位置在音标文字上方偏右
                // 1. 计算音标文字的右边界
                uint16_t phonetic_width = calculateTextWidth(formattedPhonetic.c_str(), phonetic_font_size);
                int phonetic_right_x = phonetic_text_x + phonetic_width;
                
                // 2. 喇叭位置：音标文字右侧 - 喇叭宽度的一半（偏右对齐）
                int horn_x = phonetic_right_x - horn_width;
                
                // 3. 喇叭Y位置：音标文字上方 - 喇叭高度
                int horn_y = phonetic_text_y - horn_height - 2; // 上方2像素间距
                
                // 4. 确保喇叭图标不会超出背景边界
                if (horn_x < display_x) {
                    horn_x = display_x; // 不超过左边界
                }
                if (horn_x + horn_width > display_x + 80) {
                    horn_x = display_x + 80 - horn_width; // 不超过右边界
                }
                if (horn_y < display_y - horn_height) {
                    horn_y = display_y; // 如果上方空间不足，放在音标同一行
                }
                
                // 5. 显示喇叭图标
                EPD_ShowPictureScaled(horn_x, horn_y, horn_width, horn_height,
                                    ZHONGJINGYUAN_3_7_horn, BLACK);
                
                ESP_LOGI("VOCAB", "喇叭图标位置: (%d,%d) 音标文字区域: [%d-%d]", 
                        horn_x, horn_y, phonetic_text_x, phonetic_right_x);
            }
        }
    }

            // 检查是否有释义和翻译
    bool hasDefinition = entry.definition.length() > 0;      // 有英文句子
    bool hasTranslation = entry.translation.length() > 0;    // 英文句子有翻译
    // ==================== 矩形5：中下区域 - 显示英文释义 ====================
    if (rect_count > 5) {
        RectInfo* rect = &rects[5];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        int display_width = (int)(rect->width * global_scale + 0.5f);
        int display_height = (int)(rect->height * global_scale + 0.5f);
        
        // 清空显示区域
        clearDisplayArea(display_x, display_y, display_width, display_height);
        
        // 显示翻译背景 (图标5)
        if (rect->icon_count > 0) {
            int icon_x = display_x + (display_width - 416) / 2;
            int icon_y = display_y + (display_height - 72) / 2;
            EPD_ShowPictureScaled(icon_x, icon_y, 416, 72, 
                                ZHONGJINGYUAN_3_7_definition, BLACK);
        }
        
        // 计算显示区域
        int content_x = display_x + 8;      // 左右边距8像素
        int content_y = display_y + 8;      // 上边距8像素
        int content_width = display_width - 16;  // 总宽度减去边距
        int line_height = 20;               // 行高20像素
        
        if (hasDefinition) {
            ESP_LOGI("DISPLAY", "显示英文句子: %s", entry.definition.c_str());
            
            // 英语句子自动换行显示
            content_y = displayEnglishWrapped(content_x, content_y, 
                                            entry.definition.c_str(), 16, BLACK, 
                                            content_width, line_height);
            
            // 如果有翻译，在句子和翻译之间增加间距
            if (hasTranslation) {
                content_y += 8;  // 增加间距
                
                ESP_LOGI("DISPLAY", "显示中文翻译: %s", entry.translation.c_str());
                
                // 预处理翻译文本，转换中文标点
                String processedTranslation = convertChinesePunctuationsInString(entry.translation);
                
                // 翻译自动换行显示
                content_y = displayWrappedText(content_x, content_y, 
                                            processedTranslation.c_str(), 16, BLACK, 
                                            content_width, line_height);
                ESP_LOGI("VOCAB", "释义显示完成，区域(%d,%d)-%dx%d", 
                display_x, display_y, display_width, display_height);
            }
        } 
    }
    
    // ==================== 矩形6：右下区域 - 显示单词翻译 ====================
    if (rect_count > 6) {
        RectInfo* rect = &rects[6];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        int display_width = (int)(rect->width * global_scale + 0.5f);
        int display_height = (int)(rect->height * global_scale + 0.5f);
        
        clearDisplayArea(display_x, display_y, display_width, display_height);
        
        // 显示矩形框图标
        if (rect->icon_count > 0) {
            int icon_x = display_x + (display_width - 416) / 2;
            int icon_y = display_y;
            EPD_ShowPictureScaled(icon_x, icon_y, 416, 24, 
                                ZHONGJINGYUAN_3_7_Translation1, BLACK);
        }
        
        // 情况1：如果没有英文例句，显示完整的单词翻译
        if (!hasDefinition && hasTranslation) {
            ESP_LOGI("VOCAB", "矩形6显示完整单词翻译（无例句情况）");
            
            String processedTranslation = convertChinesePunctuationsInString(entry.translation);
            
            int content_x = display_x + 15;
            int content_y = display_y + 30;
            int content_width = display_width - 30;
            int line_height = 22;
            
            displayWrappedText(content_x, content_y, 
                            processedTranslation.c_str(), 18, BLACK, 
                            content_width, line_height);
        }
    }
    // ==================== 矩形7：显示分隔线====================
    // 矩形7：显示分隔线
    if (rect_count > 7) {
        RectInfo* rect = &rects[7];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        
        int text_x = display_x + 10;
        int text_y = display_y + 8;
        //如果有单词翻译,显示分割线
        if(hasTranslation) {
            EPD_ShowPictureScaled(display_x, display_y, 416, 16, 
                                 ZHONGJINGYUAN_3_7_separate, BLACK);
        }
    }
    
    // 矩形8：显示单词翻译
    if (rect_count > 8) {
        RectInfo* rect = &rects[8];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        
        EPD_ShowPictureScaled(display_x, display_y, 416, 24, 
                             ZHONGJINGYUAN_3_7_Translation1, BLACK);
    }
    
    // 矩形9：显示分隔线
    if (rect_count > 9) {
        RectInfo* rect = &rects[9];
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        
        // 显示分隔线 (图标18)
        EPD_ShowPictureScaled(display_x, display_y, 416, 16, 
                             ZHONGJINGYUAN_3_7_separate, BLACK);
    }
    
    
    // ==================== 显示矩形边框 ====================
    if (show_border) {
        ESP_LOGI("BORDER", "开始绘制矩形边框，共%d个矩形", rect_count);
        
        for (int i = 0; i < rect_count; i++) {
            RectInfo* rect = &rects[i];
            
            int border_display_x = (int)(rect->x * global_scale + 0.5f);
            int border_display_y = (int)(rect->y * global_scale + 0.5f);
            int border_display_width = (int)(rect->width * global_scale + 0.5f);
            int border_display_height = (int)(rect->height * global_scale + 0.5f);
            
            // 边界检查
            if (border_display_x < 0) border_display_x = 0;
            if (border_display_y < 0) border_display_y = 0;
            if (border_display_x + border_display_width > setInkScreenSize.screenWidth) {
                border_display_width = setInkScreenSize.screenWidth - border_display_x;
            }
            if (border_display_y + border_display_height > setInkScreenSize.screenHeigt) {
                border_display_height = setInkScreenSize.screenHeigt - border_display_y;
            }
            
            if (border_display_width > 0 && border_display_height > 0) {
                uint16_t border_color = BLACK;
                int border_thickness = (i == status_rect_index) ? 2 : 1;
                
                // 绘制矩形边框
                EPD_DrawRectangle(border_display_x, border_display_y, 
                                 border_display_x + border_display_width - 1, 
                                 border_display_y + border_display_height - 1, 
                                 border_color, 0);
            }
        }
    }
    
    // 更新显示
    EPD_Display(ImageBW);
    EPD_Update();
    EPD_DeepSleep();
    delay_ms(100);
    
    ESP_LOGI("VOCAB", "单词显示完成");
}


// 添加休眠界面显示函数
void displaySleepScreen(RectInfo *rects, int rect_count,
                       int status_rect_index, int show_border) {
    // 类似上面的显示逻辑，但更简洁
    // 主要显示时间和唤醒提示
}
// 休眠界面矩形定义（3个矩形）
static RectInfo sleep_rects[3] = {
    // 矩形0：时间显示（居中大字体）
    {100, 60, 216, 80, {{0.5,0.5,0},{0,0,0},{0,0,0},{0,0,0}}, 1},
    
    // 矩形1：日期显示
    {150, 150, 116, 30, {{0.5,0.5,1},{0,0,0},{0,0,0},{0,0,0}}, 1},
    
    // 矩形2：唤醒提示
    {130, 190, 156, 40, {{0.5,0.5,2},{0,0,0},{0,0,0},{0,0,0}}, 1}
};
// 修改主显示函数，支持不同的界面
void displayScreen(ScreenType screen_type) {
    switch(screen_type) {
        case SCREEN_HOME:
            displayMainScreen(g_screen_manager.screens[screen_type].rects,
                            g_screen_manager.screens[screen_type].rect_count,
                            g_screen_manager.screens[screen_type].status_rect_index,
                            g_screen_manager.screens[screen_type].show_border);
            break;
            
        case SCREEN_VOCABULARY:
            displayVocabularyScreen(g_screen_manager.screens[screen_type].rects,
                            g_screen_manager.screens[screen_type].rect_count,
                            g_screen_manager.screens[screen_type].status_rect_index,
                            g_screen_manager.screens[screen_type].show_border);
            break;
            
        case SCREEN_SLEEP:
            displaySleepScreen(sleep_rects, 3, -1, 0);
            break;
            
        default:
            ESP_LOGE("DISPLAY", "未知的界面类型: %d", screen_type);
            break;
    }
}

// 单词本界面初始化函数
void initVocabularyScreen() {
    // 初始化单词本界面特定的图标
    // initIconPositions();
    
    // // 定义10个矩形，每个矩形可以显示多个图标
    // RectInfo vocab_rects[10] = {
    //     // ==================== 矩形0：状态栏 ====================
    //     // 位于屏幕顶部左侧，显示提示信息
    //     {
    //         0, 0, 336, 36,  // x, y, width, height - 占据屏幕整个顶部4/5区域
    //         {                // icons数组
    //             {0.0f, 0.0f, 7}, // 左上方，图标7
    //             {0.1f, 0.1f, 14},
    //             {0.0f, 0.0f, 0},
    //             {0.0f, 0.0f, 0}
    //         },
    //         2               // icon_count - 状态栏不显示应用图标
    //     },
        
    //     // ==================== 矩形1：右上区域 ====================
    //     // 占据右上角，显示1个图标
    //     {
    //         336, 0, 416, 36,  // x, y, width, height
    //         {
    //             {0.2f, 0.2f, 15},  // 左上方，图标15
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0}     // 填充
    //         },
    //         1
    //     },
        
    //     // ==================== 矩形2：中左区域 ====================
    //     // 位于矩形2中左侧
    //     {
    //         0, 40, 336, 90,  // x, y, width, height
    //         {
    //             {0.1f, 0.1f, 16},  // 图标16
    //             {0.0f, 0.0f, 0},  
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0}   // 填充
    //         },
    //         1
    //     },
        
    //     // ==================== 矩形3：中右区域 ====================
    //     // 位于矩形2右侧
    //     {
    //         336, 40, 416, 56,  // x, y, width, height
    //         {
    //             {0.1f, 0.2f, 19},  // 左侧居中，图标5
    //             {0.0f, 0.0f, 0},  //
    //             {0.0f, 0.0f, 0},   // 填充
    //             {0.0f, 0.0f, 0}    // 填充
    //         },
    //         1
    //     },
        
    //     // ==================== 矩形4：左下区域 ====================
    //     // 位于屏幕左下角
    //     {
    //         336, 56, 416, 90,  // x, y, width, height
    //         {
    //             {0.1f, 0.1f, 20},  // 图标20
    //             {0.0f, 0.0f, 0},  
    //             {00.0f, 0.0f, 0},  
    //             {0.0f, 0.0f, 0}   
    //         },
    //         1
    //     },
        
    //     // ==================== 矩形5：中下区域 ====================
    //     // 位于矩形4右侧
    //     {
    //         0, 90, 416, 140,  // x, y, width, height
    //         {
    //             {0.1f, 0.1f, 5},  // 16
    //             {0.0f, 0.0f, 0},  // 左下，图标0
    //             {0.0f, 0.0f, 0},  // 右下，图标1
    //             {0.0f, 0.0f, 0}    // 填充
    //         },
    //         1
    //     },
        
    //     // ==================== 矩形6：右下区域 ====================
    //     // 位于屏幕右下角
    //     {
    //         0, 140, 416, 166,  // x, y, width, height
    //         {
    //             {0.2f, 0.2f, 2},  // 17
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0}   // 填充
    //         },
    //         1
    //     },

    //     {
    //         0, 184, 416, 200,  // x, y, width, height
    //         {
    //             {0.2f, 0.2f, 2},  // 18
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0}   // 填充
    //         },
    //         1
    //     },
        
    //     {
    //         0, 200, 416, 224,  // x, y, width, height
    //         {
    //             {0.2f, 0.2f, 2},  // 17
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0}   // 填充
    //         },
    //         1
    //     },
    //     {
    //         0, 224, 416, 240,  // x, y, width, height
    //         {
    //             {0.2f, 0.2f, 2},  // 18
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0},  // 填充
    //             {0.0f, 0.0f, 0}   // 填充
    //         },
    //         1
    //     }
    // };
    
    // populateRectsWithCustomIcons(vocabulary_rects, 10, vocab_icons, 9);
}
// 单词本界面更新函数
void updateVocabularyScreen() {
    // 获取当前单词数据
    WordEntry current_word;
    // 这里从文件或内存获取当前单词
    
    // 更新显示
   // displayVocabularyScreen(vocabulary_rects, 10, 0, 1);
}

// 休眠界面初始化函数
void initSleepScreen() {
    initIconPositions();
    
    // 配置休眠界面的图标
    IconConfig sleep_icons[] = {
        {0, 0, 0.5f, 0.5f},  // 时间显示
        {1, 1, 0.5f, 0.5f},  // 日期显示
        {2, 2, 0.5f, 0.5f}   // 唤醒提示
    };
    
    populateRectsWithCustomIcons(sleep_rects, 3, sleep_icons, 3);
}

// 休眠界面更新函数
void updateSleepScreen() {
    // 获取当前时间
    // 这里获取系统时间
    
    // 更新显示
    displaySleepScreen(sleep_rects, 3, -1, 0); // 休眠界面不显示状态栏
}
// 初始化界面管理器
void initScreenManager() {
    g_screen_manager.current_screen = SCREEN_HOME;
    memset(g_screen_manager.screen_initialized, 0, sizeof(g_screen_manager.screen_initialized));
    
    // 配置主界面
    g_screen_manager.screens[SCREEN_HOME] = (ScreenConfig){
        .type = SCREEN_HOME,
        .name = "主界面",
        .rects = rects,
        .rect_count = sizeof(rects) / sizeof(rects[0]),
        .status_rect_index = 1,
        .show_border = 1,
        .init_func = NULL,
        .update_func = NULL
    };
}
// 更新当前界面
// 更新当前界面
void updateCurrentScreen() {
    ScreenConfig* config = &g_screen_manager.screens[g_screen_manager.current_screen];
    
    if (config->update_func) {
        // 使用界面的自定义更新函数
        config->update_func();
    } else {
        // 使用通用的显示函数
        displayScreen(g_screen_manager.current_screen);
    }
}
// 切换界面
bool switchScreen(ScreenType new_screen, bool force_refresh = false) {
    if (new_screen >= SCREEN_COUNT) {
        ESP_LOGE("SCREEN", "无效的界面类型: %d", new_screen);
        return false;
    }
    
    ScreenType old_screen = g_screen_manager.current_screen;
    
    // 如果已经是当前界面且不强制刷新，则跳过
    if (new_screen == old_screen && !force_refresh) {
        ESP_LOGI("SCREEN", "界面%d已经是当前界面，跳过切换", new_screen);
        return true;
    }
    
    ESP_LOGI("SCREEN", "从界面%d切换到界面%d", old_screen, new_screen);
    
    // 执行界面切换逻辑
    g_screen_manager.current_screen = new_screen;
    
    // 初始化新界面（如果未初始化）
    ScreenConfig* new_config = &g_screen_manager.screens[new_screen];
    if (!g_screen_manager.screen_initialized[new_screen]) {
        if (new_config->init_func) {
            new_config->init_func();
        }
        g_screen_manager.screen_initialized[new_screen] = true;
    }
    
    // 设置新界面的图标
    if (new_config->setup_icons_func) {
        new_config->setup_icons_func();
    }
    
    // 更新显示
    updateCurrentScreen();
    return true;
}


// 主界面图标设置函数
void setupHomeScreenIcons() {
    ESP_LOGI("SETUP_ICONS", "设置主界面图标");
    
    // 重置图标
    initIconPositions();
    
    // 遍历主界面的所有矩形（除了状态栏）
    for (int i = 0; i < g_screen_manager.screens[SCREEN_HOME].rect_count; i++) {
        // 跳过状态栏矩形
        if (i == g_screen_manager.screens[SCREEN_HOME].status_rect_index) {
            continue;
        }
        
        RectInfo *rect = &rects[i];
        
        // 如果有图标配置，就设置
        for (int j = 0; j < rect->icon_count; j++) {
            IconPositionInRect *icon = &rect->icons[j];
            
            // 直接使用存储的相对位置
            float rel_x = icon->rel_x;
            float rel_y = icon->rel_y;
            int icon_index = icon->icon_index;
            
            // 边界检查
            if (rel_x < 0.0f) rel_x = 0.0f;
            if (rel_x > 1.0f) rel_x = 1.0f;
            if (rel_y < 0.0f) rel_y = 0.0f;
            if (rel_y > 1.0f) rel_y = 1.0f;
            if (icon_index < 0) icon_index = 0;
            if (icon_index >= 6) icon_index = 5;
            
            // 设置图标（主界面）
            populateRectWithIcon(rect, icon_index, rel_x, rel_y);
        }
    }
}

// 单词本界面图标设置函数
void setupVocabularyScreenIcons() {
    ESP_LOGI("SETUP_ICONS", "设置单词本界面图标");
    
    // 重置图标
    initIconPositions();
    
    // 遍历主界面的所有矩形（除了状态栏）
    for (int i = 0; i < g_screen_manager.screens[SCREEN_VOCABULARY].rect_count; i++) {
        // 跳过状态栏矩形
        if (i == g_screen_manager.screens[SCREEN_VOCABULARY].status_rect_index) {
            continue;
        }
        
        RectInfo *rect = &vocab_rects[i];
        
        // 如果有图标配置，就设置
        for (int j = 0; j < rect->icon_count; j++) {
            IconPositionInRect *icon = &rect->icons[j];
            
            // 直接使用存储的相对位置
            float rel_x = icon->rel_x;
            float rel_y = icon->rel_y;
            int icon_index = icon->icon_index;
            
            // 边界检查
            if (rel_x < 0.0f) rel_x = 0.0f;
            if (rel_x > 1.0f) rel_x = 1.0f;
            if (rel_y < 0.0f) rel_y = 0.0f;
            if (rel_y > 1.0f) rel_y = 1.0f;
            if (icon_index < 0) icon_index = 0;
            if (icon_index >= 6) icon_index = 5;
            
            // 设置图标（主界面）
            populateRectWithIcon(rect, icon_index, rel_x, rel_y);
        }
    }
}

// 休眠界面图标设置函数
void setupSleepScreenIcons() {
    ESP_LOGI("SETUP_ICONS", "设置休眠界面图标");
    
    // 重置图标
    initIconPositions();
    
    // 休眠界面的图标配置
    RectInfo* sleep_rects = g_screen_manager.screens[SCREEN_SLEEP].rects;
    int rect_count = g_screen_manager.screens[SCREEN_SLEEP].rect_count;
    
    // 休眠界面图标通常较少，主要用于显示时间、日期等
    IconConfig sleep_icons[] = {
        // 矩形0：时间显示（使用图标0，精确居中）
        {0, 0, 0.5f, 0.5f},
        
        // 矩形1：日期显示（使用图标1，精确居中）
        {1, 1, 0.5f, 0.5f},
        
        // 矩形2：唤醒提示（使用图标2，居中）
        {2, 2, 0.5f, 0.5f}
    };
    
    // 设置休眠界面图标
    populateRectsWithCustomIcons(sleep_rects, rect_count, 
                                sleep_icons, sizeof(sleep_icons) / sizeof(sleep_icons[0]));
}

// 初始化所有界面配置
void initAllScreens() {
    
    // 配置主界面
    g_screen_manager.screens[SCREEN_HOME] = (ScreenConfig){
        .type = SCREEN_HOME,
        .name = "主界面",
        .rects = rects,  // 使用全局的rects数组
        .rect_count = sizeof(rects) / sizeof(rects[0]),
        .status_rect_index = 1,
        .show_border = 1,
        .init_func = NULL,
        .setup_icons_func = setupHomeScreenIcons,
        .update_func = NULL
    };
    
    // 配置单词本界面
    g_screen_manager.screens[SCREEN_VOCABULARY] = (ScreenConfig){
        .type = SCREEN_VOCABULARY,
        .name = "单词本",
        .rects = vocab_rects,
        .rect_count = sizeof(vocab_rects) / sizeof(vocab_rects[0]),
        .status_rect_index = 1,
        .show_border = 1,
        .init_func = NULL,
        .setup_icons_func = setupVocabularyScreenIcons,
        .update_func = updateVocabularyScreen
    };
    
    // 配置休眠界面
    g_screen_manager.screens[SCREEN_SLEEP] = (ScreenConfig){
        .type = SCREEN_SLEEP,
        .name = "休眠界面",
        .rects = sleep_rects,
        .rect_count = 3,
        .status_rect_index = -1,
        .show_border = 0,
        .init_func = initSleepScreen,
        .setup_icons_func = setupSleepScreenIcons,
        .update_func = updateSleepScreen
    };
}


void ink_screen_show(void *args)
{
    float num=12.05;
    uint8_t dat=0;
	grbl_msg_sendf(CLIENT_SERIAL, MsgLevel::Info,"ink_screen_show");
	Uart0.printf("ink_screen_show\r\n");
   //init_sleep_timer();
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
                        // update_activity_time(); // 更新活动时间 
						// EPD_FastInit();
						// EPD_Display_Clear();
						// EPD_Update();  //局刷之前先对E-Paper进行清屏操作
						// vTaskDelay(100);
                        // EPD_PartInit();
						// readAndPrintRandomWord();
                        // showPromptInfor(showPrompt,false);
						// if(entry.word.length() > 0) {
						// 	clearDisplayArea(10,40,EPD_H,EPD_W);
						// 	safeDisplayWordEntry(entry, 20, 60);
						// } else {
						// 	ESP_LOGW("DISPLAY", "跳过空单词显示");
						// }

						// EPD_Display(ImageBW);
						// EPD_Update();
						// EPD_DeepSleep();
                        // vTaskDelay(1000);
                        switchScreen(SCREEN_VOCABULARY);
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
                drawUnderlineForIconEx(0);
				inkScreenTestFlag = 0;
			break;
			case 2:
                update_activity_time(); 
                drawUnderlineForIconEx(1);
				inkScreenTestFlag = 0;
			break;
			case 3:
                update_activity_time(); 
				drawUnderlineForIconEx(2);
				inkScreenTestFlag = 0;
			break;
			case 4:
                update_activity_time(); 
                drawUnderlineForIconEx(3);
				inkScreenTestFlag = 0;
			break;
			case 5:
                update_activity_time(); 
				drawUnderlineForIconEx(4);
                debugIconPositions();
				inkScreenTestFlag = 0;
			break;
			case 6:
                update_activity_time(); 
                drawUnderlineForIconEx(5);
				inkScreenTestFlag = 0;
			break;
			case 7:
                update_activity_time(); 
				//updateDisplayWithMain();
                	updateDisplayWithMain(rects,rect_count, 0, 1);
                interfaceIndex =1;
				inkScreenTestFlag = 0;
			break;
            default:
                break;
		}
        showPromptInfor(showPrompt,true);
	//	updateDisplayWithWifiIcon();
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

    SDState state = get_sd_state(true);
	if (state != SDState::Idle) {
		if (state == SDState::NotPresent) {
			ESP_LOGI(TAG,"No SD Card\r\n");
		} else {
			ESP_LOGI(TAG,"SD Card Busy\r\n");
		}
	}
    initAllScreens();
   initLayoutFromConfig();
   setupHomeScreenIcons();
   displayScreen(SCREEN_HOME);
	//updateDisplayWithMain(rects,rect_count, 0, 1);  // 最后一个参数为1表示显示边框
	Uart0.printf("fast refresh\r\n");

	

    BaseType_t task_created = xTaskCreatePinnedToCore(ink_screen_show, 
                                                        "ink_screen_show", 
                                                        8192, 
                                                        NULL, 
                                                        4, 
                                                        &_eventTaskHandle, 
                                                        0);
}
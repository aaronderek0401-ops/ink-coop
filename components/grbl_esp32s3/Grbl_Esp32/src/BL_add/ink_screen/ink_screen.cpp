#include "ink_screen.h"
#include "./SPI_Init.h"
#include "esp_timer.h"
#include "word_book.h"
#include "wordbook_interface.h"
#include "sleep_interface.h"

// 前向声明 wordbook_interface.cpp 中的函数
extern void showPromptInfor(uint8_t *tempPrompt, bool isAllRefresh);

extern "C" {
	#include "./EPD.h"
#include "./EPD_GUI.h"
#include "./EPD_Font.h"
#include "./Pic.h"
}

// ===== GXEPD2 集成 - 支持多种墨水屏 =====
// 必须先包含 GxEPD2 和 Arduino 库,它们会提供 Adafruit_GFX.h
#include <GxEPD2_BW.h>

// 选择使用的屏幕驱动 (取消注释其中一个)
#include <gdey/GxEPD2_370_GDEY037T03.h>  // 中景园 3.7" UC8253 (240x416) - 当前使用
// #include <epd/GxEPD2_213_BN.h>        // 微雪 2.13" V4 SSD1680 (122x250)

#include <Fonts/FreeMonoBold9pt7b.h>      // Adafruit GFX 字体
#include <SPI.h>                          // Arduino SPI 库
// ===== SD卡字库支持 (.bin 字库系统) =====
#include "chinese_text_display.h"  // SD卡.bin字库显示助手
#include "image_loader.h"          // SD卡图片加载器
#include "multi_font_manager.h"    // 多字体管理器 (English 24x24 + 16x16)
// ===== GXEPD2 全局显示对象 =====
// 引脚定义（根据 inkScreen.h 硬件配置）
#define EPD_CS_PIN     14  // BSP_SPI_CS_GPIO_PIN
#define EPD_DC_PIN     13  // EPD_DC_GPIO_PIN
#define EPD_RST_PIN    12  // EPD_RES_GPIO_PIN
#define EPD_BUSY_PIN   4   // EPD_BUSY_GPIO_PIN

// ===== 使用两个独立的 SPI 总线 =====
// SPI2 (FSPI) - 用于 SD 卡（Arduino 默认 SPI 对象）
//   在 system_ini() 中由 SPI.begin() 初始化
//   引脚: SCK=37, MOSI=38, MISO=39, SS=36

// SPI3 (HSPI) - 用于墨水屏（新建独立 SPI 对象）
//   引脚: SCK=48, MOSI=47, MISO=-1, SS=14
SPIClass EPD_SPI(HSPI);  // 创建独立的 SPI3 实例用于墨水屏 (ESP32-S3 使用 HSPI 代表 SPI3)
#define EPD_SPI_SCK   48
#define EPD_SPI_MOSI  47
#define EPD_SPI_MISO  -1   // 墨水屏不需要 MISO

// 选择显示对象 (取消注释其中一个)
// 1. 中景园 3.7" UC8253 (240x416) - 当前使用
GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(
    GxEPD2_370_GDEY037T03(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
);

// 2. 微雪 2.13" V4 SSD1680 (122x250)
// GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(
//     GxEPD2_213_BN(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
// ); 

// 全局变量定义
int g_last_underline_x = 0;
int g_last_underline_y = 0;
int g_last_underline_width = 0;
// 全局变量：当前选中的图标索引
int g_selected_icon_index = -1;

// ======== 焦点系统变量 ========
int g_current_focus_rect = 0;  // 当前焦点所在的矩形索引
static int g_total_focusable_rects = 0;  // 可获得焦点的矩形总数
bool g_focus_mode_enabled = false;  // 是否启用焦点模式

// 可配置的焦点矩形列表
static int g_focusable_rect_indices[MAX_FOCUSABLE_RECTS];  // 可焦点矩形的索引数组（母数组）
static int g_focusable_rect_count = 0;  // 实际可焦点矩形数量
static int g_current_focus_index = 0;  // 当前焦点在g_focusable_rect_indices中的位置

// ======== 子母数组系统变量 ========
static bool g_in_sub_array = false;  // 是否在子数组模式
static int g_sub_array_indices[MAX_FOCUSABLE_RECTS][MAX_FOCUSABLE_RECTS];  // 每个母数组元素对应的子数组
static int g_sub_array_counts[MAX_FOCUSABLE_RECTS];  // 每个母数组元素对应的子数组长度
static int g_current_sub_focus_index = 0;  // 当前在子数组中的焦点位置
static int g_parent_focus_index_backup = 0;  // 进入子数组前的母数组焦点位置备份

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

uint8_t *showPrompt=nullptr;
IconPosition selected_icon = {0, 0, 0, 0, false, 0, nullptr};
// 全局变量记录上次显示信息
char lastDisplayedText[256] = {0};
uint16_t lastTextLength = 0;
uint16_t lastX = 65;
uint16_t lastY = 120;
uint16_t lastSize = 0;  // 添加字体大小记录

static esp_timer_handle_t sleep_timer;
static bool is_sleep_mode = false;
static uint32_t last_activity_time = 0;
static const uint32_t SLEEP_TIMEOUT_MS = 25000; // 15秒

InkScreenSize setInkScreenSize;
TimerHandle_t inkScreenDebounceTimer = NULL;

// ======== 图标文件名映射表 ========
// 与 g_available_icons 数组保持同步
// 每个文件对应 components/resource/icon/ 文件夹中的文件
const char *g_icon_filenames[13] = {
    "icon1.jpg",             // 0: ICON_1
    "icon2.jpg",             // 1: ICON_2
    "icon3.jpg",             // 2: ICON_3
    "icon4.jpg",             // 3: ICON_4
    "icon5.jpg",             // 4: ICON_5
    "icon6.jpg",             // 5: ICON_6
    "12.jpg",                // 6: separate (分隔线)
    "wifi_connect.jpg",      // 7: WIFI_CONNECT
    "wifi_disconnect.jpg",   // 8: WIFI_DISCONNECT
    "battery_1.jpg",         // 9: BATTERY_1
    "horn.jpg",              // 10: HORN (喇叭)
    "nail.jpg",              // 11: NAIL (钉子)
    "lock.jpg"               // 12: LOCK (锁)
};

// 全局图标数组 - 仅使用在 Pic.h 中定义的有效图标
// 注：部分位图未在 Pic.h 中定义（ZHONGJINGYUAN_3_7_promt, wifi_battry, word, Translation1, pon, definition）
// 图标索引对应的文件在 components/resource/icon/ 文件夹中
IconInfo g_available_icons[13] = {
    {ZHONGJINGYUAN_3_7_ICON_1, 62, 64},           // 0: ICON_1 -> icon1.jpg
    {ZHONGJINGYUAN_3_7_ICON_2, 64, 64},           // 1: ICON_2 -> icon2.jpg
    {ZHONGJINGYUAN_3_7_ICON_3, 86, 64},           // 2: ICON_3 -> icon3.jpg
    {ZHONGJINGYUAN_3_7_ICON_4, 71, 56},           // 3: ICON_4 -> icon4.jpg
    {ZHONGJINGYUAN_3_7_ICON_5, 76, 56},           // 4: ICON_5 -> icon5.jpg
    {ZHONGJINGYUAN_3_7_ICON_6, 94, 64},           // 5: ICON_6 -> icon6.jpg
    {ZHONGJINGYUAN_3_7_separate, 120, 8},         // 6: separate (分隔线) -> 12.jpg
    {ZHONGJINGYUAN_3_7_WIFI_CONNECT, 32, 32},     // 7: WIFI_CONNECT -> wifi_connect.jpg
    {ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, 32, 32},  // 8: WIFI_DISCONNECT -> wifi_disconnect.jpg
    {ZHONGJINGYUAN_3_7_BATTERY_1, 36, 24},        // 9: BATTERY_1 -> battery_1.jpg
    {ZHONGJINGYUAN_3_7_HORN, 16, 16},             // 10: HORN (喇叭) -> horn.jpg
    {ZHONGJINGYUAN_3_7_NAIL, 15, 16},             // 11: NAIL (钉子) -> nail.jpg
    {ZHONGJINGYUAN_3_7_LOCK, 32, 32}              // 12: LOCK (锁) -> lock.jpg
};

// 全局界面管理器实例
ScreenManager g_screen_manager;

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
        // display.drawBitmap(中文显示);
        
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
            display.drawLine(g_last_underline_x, 
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
    
    // 初始化显示（使用 GXEPD2）
    display.init(0, true, 2, true);  // 完全初始化
    display.clearScreen();            // 清空屏幕
    display.display(true);            // 完全刷新
    vTaskDelay(100 / portTICK_PERIOD_MS);
    display.setPartialWindow(0, 0, display.width(), display.height());  // 设置局部更新区域
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
    display.drawLine(x, underline_y, x + width, underline_y, BLACK);
    display.drawLine(x, underline_y + 1, x + width, underline_y + 1, BLACK);
    
    // 绘制一个测试矩形，确认绘制功能正常
    // display.drawRect(x, y, x + width - x, y + height - y, BLACK);
    
    // 记录这次的下划线位置（用于下次清除）
    g_last_underline_x = x;
    g_last_underline_y = underline_y;
    g_last_underline_width = width;
    
    // 更新显示
    display.display();
    display.display(true);
    display.powerOff();

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
			display.drawImage(ZHONGJINGYUAN_3_7_WIFI_CONNECT, 340, 1, 32, 32, true);
		} else {
			 display.drawImage(ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, 340, 1, 32, 32, true); 
		}
		isFrist = 1;
	}
    // 只在状态改变时更新显示
    if(lastWifiStatus != currentWifiStatus) {
		display.init(0, true, 2, true);
		display.clearScreen();
		display.display(true);  //局刷之前先对E-Paper进行清屏操作
		delay_ms(100);
		display.setPartialWindow(0, 0, display.width(), display.height());
        display.fillRect(5, 5, 340, 30, GxEPD_WHITE);
	    display.fillRect(60, 40, EPD_H, EPD_W, GxEPD_WHITE);
		display.drawImage(ZHONGJINGYUAN_3_7_BATTERY_1, 380, 2, 36, 24, true);
		display.drawImage(ZHONGJINGYUAN_3_7_ICON_1, 60, 40, 62, 64, true);
		display.drawImage(ZHONGJINGYUAN_3_7_ICON_2, 180, 40, 64, 64, true);
		display.drawImage(ZHONGJINGYUAN_3_7_ICON_3, 300, 40, 86, 64, true);
		display.drawImage(ZHONGJINGYUAN_3_7_ICON_4, 60, 140, 71, 56, true);
		display.drawImage(ZHONGJINGYUAN_3_7_ICON_5, 180, 140, 76, 56, true);
		display.drawImage(ZHONGJINGYUAN_3_7_ICON_6, 300, 140, 94, 64, true);
        // 清除原图标区域
        display.drawRect(340, 1, 32, 32, WHITE);
        
        // 根据 WiFi 连接状态选择图标
        if(currentWifiStatus) {
           display.drawImage(ZHONGJINGYUAN_3_7_WIFI_CONNECT, 340, 1, 32, 32, true);  // WiFi 已连接
            ESP_LOGI("WIFI222222222222", "WiFi状态: 已连接");
        } else {
           display.drawImage(ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, 340, 1, 32, 32, true); // WiFi 未连接
            ESP_LOGI("WIFI111111111111111", "WiFi状态: 未连接");
        }
        
        lastWifiStatus = currentWifiStatus;

		display.display();
		display.display(true);
		display.powerOff();
	//	delay_ms(1000);
    }
}

 //清空整个屏幕（显示白色）
void clearEntireScreen() {
    ESP_LOGI(TAG, "清空整个屏幕");
    
    display.init(0, true, 2, true);
    display.clearScreen();
    display.display(true);
    delay_ms(50);
    display.setPartialWindow(0, 0, display.width(), display.height());
    
    // 清除整个屏幕区域
    display.fillRect(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt, GxEPD_WHITE);
    
    // 更新显示
    display.display();
    display.display(true);
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
            display.fillRect(display_x, display_y, display_width, display_height, GxEPD_WHITE);
            
            ESP_LOGD(TAG, "清除矩形%d区域: (%d,%d) %dx%d", 
                    i, display_x, display_y, display_width, display_height);
        }
    }
}
//清除之前显示的所有图标区域
void clearAllPictureAreas() {
    for (int i = 0; i < PICTURE_AREA_COUNT; i++) {
        if (picture_areas[i].displayed) {
            display.drawRect(picture_areas[i].x, 
                            picture_areas[i].y, 
                            picture_areas[i].width, 
                            picture_areas[i].height, 
                            WHITE);
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
        display.drawRect(picture_areas[area_index].x, 
                        picture_areas[area_index].y, 
                        picture_areas[area_index].width, 
                        picture_areas[area_index].height, 
                        WHITE);
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
    display.drawImage(image, x, y, width, height, true);
    
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
    
    display.fillRect(start_x, start_y, end_x - start_x, end_y - start_y, GxEPD_WHITE);
    
    ESP_LOGI(TAG, "清除显示区域: (%d,%d)到(%d,%d)", start_x, start_y, end_x, end_y);
}

// 清除特定图标区域
void clearSpecificIcons(uint8_t *icon_indices, uint8_t count) {
    for (int i = 0; i < count; i++) {
        clearPictureArea(icon_indices[i]);
    }
    
    // 立即刷新显示
    display.display();
    display.display(true);
}
void drawPictureScaled(uint16_t orig_x, uint16_t orig_y, 
                           uint16_t orig_w, uint16_t orig_h,
                           const uint8_t* BMP, uint16_t color) {
    // 安全检查：图片数据不能为空
    if (BMP == nullptr) {
        ESP_LOGE(TAG, "EPD_ShowPictureScaled: BMP数据为空指针！位置(%d,%d) 尺寸%dx%d", 
                orig_x, orig_y, orig_w, orig_h);
        return;
    }
    
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

    // 边界裁剪，避免越界写入缓冲区
    const uint16_t screen_w = setInkScreenSize.screenWidth;
    const uint16_t screen_h = setInkScreenSize.screenHeigt;
    if (new_x >= screen_w || new_y >= screen_h) {
        ESP_LOGW(TAG, "EPD_ShowPictureScaled: 位置(%d,%d)超出屏幕(%d,%d)，跳过绘制", new_x, new_y, screen_w, screen_h);
        return;
    }

    if (new_x + new_w > screen_w) {
        uint16_t clipped_w = screen_w - new_x;
        ESP_LOGW(TAG, "EPD_ShowPictureScaled: 宽度裁剪 %d -> %d (屏幕宽度=%d)", new_w, clipped_w, screen_w);
        new_w = clipped_w;
    }

    if (new_y + new_h > screen_h) {
        uint16_t clipped_h = screen_h - new_y;
        ESP_LOGW(TAG, "EPD_ShowPictureScaled: 高度裁剪 %d -> %d (屏幕高度=%d)", new_h, clipped_h, screen_h);
        new_h = clipped_h;
    }

    if (new_w == 0 || new_h == 0) {
        ESP_LOGW(TAG, "EPD_ShowPictureScaled: 裁剪后尺寸为0，跳过绘制");
        return;
    }

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
            display.drawPixel(new_x + dx, new_y + dy, pixel_on ? !color : color);
        }
    }

    ESP_LOGD(TAG, "Scaled icon: (%d,%d) %dx%d -> (%d,%d) %dx%d",
             orig_x, orig_y, orig_w, orig_h, new_x, new_y, new_w, new_h);
}

// ==================== 框架辅助函数 ====================

/**
 * @brief 添加图标到矩形
 * @param rects 矩形数组
 * @param rect_index 目标矩形索引
 * @param icon_index 图标索引（0-20）
 * @param rel_x 相对X位置（0.0-1.0，0.5为中心）
 * @param rel_y 相对Y位置（0.0-1.0，0.5为中心）
 * @return 是否成功添加
 */
bool addIconToRect(RectInfo* rects, int rect_index, int icon_index, float rel_x, float rel_y) {
    if (rects == nullptr) {
        ESP_LOGE(TAG, "addIconToRect: rects为空");
        return false;
    }
    
    RectInfo* rect = &rects[rect_index];
    
    // 检查是否还有空间
    if (rect->icon_count >= 2) {
        ESP_LOGW(TAG, "矩形%d已满，无法添加图标%d", rect_index, icon_index);
        return false;
    }
    
    // 添加图标
    rect->icons[rect->icon_count].rel_x = rel_x;
    rect->icons[rect->icon_count].rel_y = rel_y;
    rect->icons[rect->icon_count].icon_index = icon_index;
    rect->icon_count++;
    
    ESP_LOGI(TAG, "添加图标%d到矩形%d, 位置(%.2f, %.2f), 当前图标数:%d", 
            icon_index, rect_index, rel_x, rel_y, rect->icon_count);
    
    return true;
}

/**
 * @brief 清空矩形内所有图标
 */
void clearRectIcons(RectInfo* rect) {
    if (rect == nullptr) return;
    
    for (int i = 0; i < 4; i++) {
        rect->icons[i].rel_x = 0.0f;
        rect->icons[i].rel_y = 0.0f;
        rect->icons[i].icon_index = 0;
    }
    rect->icon_count = 0;
}

// ==================== 框架定义（主界面7个框） ====================

// 定义7个框架，只定义位置和内容类型，图标后续添加
RectInfo rects[MAX_MAIN_RECTS] = {
    // 矩形0：状态栏
    // {0, 0, 416, 36, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0, 
    //  {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形1：左上区域
    // {0, 40, 136, 100, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0, 
    //  {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形2：中上区域
    // {142, 40, 136, 100, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0, 
    //  {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形3：右上区域
    // {278, 40, 136, 100, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0, 
    //  {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形4：左下区域
    // {0, 144, 138, 96, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0, 
    //  {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形5：中下区域
    // {142, 144, 132, 96, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0, 
    //  {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形6：右下区域
    // {278, 144, 138, 96, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0, 
    //  {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false}
};

// 矩形总数 - 从屏幕管理器中获取实际配置的数量
int rect_count = 0;  // 将在ink_screen_init()中设置为实际值

// ==================== 框架定义（单词界面，最多MAX_VOCAB_RECTS个框） ====================

// 定义矩形框架数组，只定义位置和内容类型，图标后续添加
RectInfo vocab_rects[MAX_VOCAB_RECTS] = {
    // 矩形0：状态栏左侧（提示信息区）
    // {0, 0, 336, 36, CONTENT_STATUS_BAR, 16, ALIGN_LEFT, ALIGN_MIDDLE, 
    //  10, 10, 4, 4, 20, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形1：状态栏右侧（WiFi/电池）
    // {336, 0, 80, 36, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 
    //  0, 0, 0, 0, 0, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形2：英文单词显示区 - 不预设文本内容
    // {0, 40, 416, 48, CONTENT_WORD, 24, ALIGN_LEFT, ALIGN_MIDDLE, 
    //  10, 10, 4, 4, 28, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, true},
    
    // // 矩形3：音标喇叭图标区
    // {336, 40, 80, 16, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 
    //  0, 0, 0, 0, 0, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形4：音标文字显示区 - 不预设文本内容
    // {336, 56, 80, 32, CONTENT_PHONETIC, 16, ALIGN_CENTER, ALIGN_MIDDLE, 
    //  5, 5, 4, 4, 20, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, true},
    
    // // 矩形5：英文例句/释义区 - 不预设文本内容
    // {0, 90, 416, 72, CONTENT_DEFINITION, 16, ALIGN_LEFT, ALIGN_TOP, 
    //  8, 8, 8, 8, 20, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, true},
    
    // // 矩形6：分隔线1
    // {0, 162, 416, 16, CONTENT_SEPARATOR, 0, ALIGN_CENTER, ALIGN_MIDDLE, 
    //  0, 0, 0, 0, 0, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形7：翻译标题
    // {0, 178, 416, 24, CONTENT_ICON_ONLY, 0, ALIGN_CENTER, ALIGN_MIDDLE, 
    //  0, 0, 0, 0, 0, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // // 矩形8：中文翻译区 - 不预设文本内容
    // {0, 202, 416, 22, CONTENT_TRANSLATION, 16, ALIGN_LEFT, ALIGN_TOP, 
    //  8, 8, 4, 4, 20, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, true},
    
    // // 矩形9：底部分隔线
    // {0, 224, 416, 16, CONTENT_SEPARATOR, 0, ALIGN_CENTER, ALIGN_MIDDLE, 
    //  0, 0, 0, 0, 0, {{0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 0,
    //  {{0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 16, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false}
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

// ==================== 图标布局初始化函数 ====================

/**
 * @brief 初始化主界面的图标布局
 * 
 * 使用示例：
 * - addIconToRect(rects, 1, 0, 0.2, 0.2);  // 矩形1，图标0，位置(20%, 20%)
 * - addIconToRect(rects, 1, 1, 0.5, 0.5);  // 矩形1，图标1，中心位置
 */
void initMainScreenIcons() {
    ESP_LOGI(TAG, "========== 初始化主界面图标布局 ==========");

    // 矩形0：状态栏 - 1个图标
    // addIconToRect(rects, 0, 11, 0.5, 0.0);   // 图标0 (ICON_1: 62x64)，左上


    // 矩形1：左上区域
    // addIconToRect(rects, 1, 1, 0.3, 0.3);   // 图标1 (ICON_2: 64x64)，右上
    // addIconToRect(rects, 1, 2, 0.4, 0.6);
    
    // 矩形2：中上区域
    // addIconToRect(rects, 2, 2, 0.3, 0.3);   // 图标2
    // addIconToRect(rects, 2, 4, 0.6, 0.5);
    
    // 矩形3：右上区域
    // addIconToRect(rects, 3, 3, 0.3, 0.3);   // 图标3 (ICON_6: 94x64)
    // addIconToRect(rects, 3, 0, 0.6, 0.4);
    
    // 矩形4：左下区域
    // addIconToRect(rects, 4, 4, 0.3, 0.3);  // 图标4
    // addIconToRect(rects, 4, 2, 0.35, 0.2);
    // addIconToRect(rects, 4, 3, 0.6, 0.2);
    // addIconToRect(rects, 4, 4, 0.4, 0.6);
    
    // 矩形5：中下区域
    // addIconToRect(rects, 5, 5, 0.3, 0.3);   // 图标5
    // addIconToRect(rects, 5, 0, 0.3, 0.6);
    // addIconToRect(rects, 5, 1, 0.6, 0.6);
    
    // 矩形6：右下区域
    // addIconToRect(rects, 6, 6, 0.3, 0.3);   // 图标6
    
    ESP_LOGI(TAG, "主界面图标布局初始化完成");
}

/**
 * @brief 初始化单词界面的图标布局
 */
void initVocabScreenIcons() {
    ESP_LOGI(TAG, "========== 初始化单词界面图标布局 ==========");
    
    // 矩形0：状态栏左侧 - 2个图标
    // addIconToRect(vocab_rects, 0, 7, 0.0, 0.0);    // 钉子图标，左上角
    // addIconToRect(vocab_rects, 0, 14, 0.1, 0.1);   // 提示背景，稍偏右
    
    // 矩形1：状态栏右侧 - 1个图标（WiFi/电池组合）
    // addIconToRect(vocab_rects, 1, 15, 0.2, 0.2);   // WiFi/电池组合图标
    
    // 矩形2：英文单词区 - 1个背景图标
    // addIconToRect(vocab_rects, 2, 16, 0.0, 0.0);   // 单词背景
    
    // 矩形3：音标喇叭区 - 1个图标
    // addIconToRect(vocab_rects, 3, 19, 0.0, 0.0);   // 喇叭图标
    
    // 矩形4：音标文字区 - 1个背景图标
    // addIconToRect(vocab_rects, 4, 20, 0.0, 0.0);   // 音标背景
    
    // 矩形5：释义区 - 1个背景图标
    // addIconToRect(vocab_rects, 5, 21, 0.0, 0.0);   // 释义背景
    
    // 矩形6：分隔线1
    // addIconToRect(vocab_rects, 6, 18, 0.0, 0.0);   // 分隔线
    
    // 矩形7：翻译标题
    // addIconToRect(vocab_rects, 7, 17, 0.0, 0.0);   // Translation图标
    
    // 矩形9：底部分隔线
    // addIconToRect(vocab_rects, 9, 18, 0.0, 0.0);   // 分隔线
    
    ESP_LOGI(TAG, "单词界面图标布局初始化完成");
}

/**
 * @brief 初始化单词界面的文本布局
 */
void initVocabScreenTexts() {
    ESP_LOGI(TAG, "========== 初始化单词界面文本布局 ==========");
    
    // 矩形2：英文单词显示区 - 添加单词文本
    // addTextToRect(vocab_rects, 2, CONTENT_WORD, 0.05, 0.5, 24, ALIGN_LEFT, ALIGN_MIDDLE);
    
    // 矩形4：音标文字显示区 - 添加音标文本
    // addTextToRect(vocab_rects, 4, CONTENT_PHONETIC, 0.5, 0.5, 16, ALIGN_CENTER, ALIGN_MIDDLE);
    
    // 矩形5：英文释义区 - 添加释义文本
    // addTextToRect(vocab_rects, 5, CONTENT_DEFINITION, 0.05, 0.05, 16, ALIGN_LEFT, ALIGN_TOP);
    
    // 矩形8：中文翻译区 - 添加翻译文本
    // addTextToRect(vocab_rects, 8, CONTENT_TRANSLATION, 0.05, 0.05, 16, ALIGN_LEFT, ALIGN_TOP);
    
    ESP_LOGI(TAG, "单词界面文本布局初始化完成");
}

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
    
    // 计算图标在矩形内的具体位置（基于左上角对齐）
    // rel_x, rel_y 表示图标左上角在矩形中的相对位置
    // 例如：rel_x=0.0, rel_y=0.0 表示图标左上角在矩形左上角
    int icon_x = rect->x + (int)(rel_x * rect->width);
    int icon_y = rect->y + (int)(rel_y * rect->height);
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



/**
 * @brief 添加文本内容到矩形
 */
bool addTextToRect(RectInfo* rects, int rect_index, RectContentType content_type,
                   float rel_x, float rel_y, uint8_t font_size,
                   TextAlignment h_align, TextAlignment v_align) {
    if (rects == nullptr || rect_index < 0) {
        ESP_LOGE("TEXT", "参数无效");
        return false;
    }
    
    RectInfo* rect = &rects[rect_index];
    
    if (rect->text_count >= 4) {
        ESP_LOGE("TEXT", "矩形%d已达到最大文本数量(4个)", rect_index);
        return false;
    }
    
    TextPositionInRect* text = &rect->texts[rect->text_count];
    text->rel_x = rel_x;
    text->rel_y = rel_y;
    text->type = content_type;
    text->font_size = font_size;
    text->h_align = h_align;
    text->v_align = v_align;
    text->max_width = 0;  // 0表示使用矩形剩余宽度
    text->max_height = 0; // 0表示使用矩形剩余高度
    
    rect->text_count++;
    
    ESP_LOGI("TEXT", "矩形%d添加文本类型%d，位置(%.2f, %.2f), 字体%d",
             rect_index, content_type, rel_x, rel_y, font_size);
    
    return true;
}

/**
 * @brief 清空矩形内所有文本
 */
void clearRectTexts(RectInfo* rect) {
    if (rect == nullptr) return;
    rect->text_count = 0;
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
    display.init(0, true, 2, true);
    display.clearScreen();
    display.display(true);
    delay_ms(50);
    display.setPartialWindow(0, 0, display.width(), display.height());
    
    // ==================== 显示状态栏图标 ====================
    int wifi_x, wifi_y, battery_x, battery_y;
    getStatusIconPositions(rects, rect_count, status_rect_index, 
                          &wifi_x, &wifi_y, &battery_x, &battery_y);

    // drawPictureScaled(wifi_x, wifi_y, 32, 32, 
    //                      ZHONGJINGYUAN_3_7_WIFI_DISCONNECT, BLACK);
    
    #ifdef BATTERY_LEVEL
        const uint8_t* battery_icon = NULL;
        if (BATTERY_LEVEL >= 80) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_4;
        else if (BATTERY_LEVEL >= 60) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_3;
        else if (BATTERY_LEVEL >= 40) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_2;
        else if (BATTERY_LEVEL >= 20) battery_icon = ZHONGJINGYUAN_3_7_BATTERY_1;
        else battery_icon = ZHONGJINGYUAN_3_7_BATTERY_0;
        
        drawPictureScaled(battery_x, battery_y, 36, 24, battery_icon, BLACK);
    #else
        // drawPictureScaled(battery_x, battery_y, 36, 24, 
        //                      ZHONGJINGYUAN_3_7_BATTERY_1, BLACK);
    #endif
    
    // ==================== 遍历所有矩形，显示图标 ====================
    ESP_LOGI("MAIN", "开始显示主界面，共%d个矩形", rect_count);
    
    for (int i = 0; i < rect_count; i++) {
        RectInfo* rect = &rects[i];
        
        // 计算缩放后的矩形位置和尺寸
        int display_x = (int)(rect->x * global_scale + 0.5f);
        int display_y = (int)(rect->y * global_scale + 0.5f);
        int display_width = (int)(rect->width * global_scale + 0.5f);
        int display_height = (int)(rect->height * global_scale + 0.5f);
        
        ESP_LOGI("MAIN", "--- 矩形%d: (%d,%d) %dx%d, 图标数:%d ---", 
                i, display_x, display_y, display_width, display_height, rect->icon_count);
        
        // 显示该矩形内的所有图标
        if (rect->icon_count > 0) {
            for (int j = 0; j < rect->icon_count; j++) {
                IconPositionInRect* icon = &rect->icons[j];
                int icon_index = icon->icon_index;
                
                ESP_LOGI("MAIN", "  处理图标%d, index=%d", j, icon_index);
                
                if (icon_index >= 0 && icon_index < 21) {
                    IconInfo* icon_info = &g_available_icons[icon_index];
                    
                    // 安全检查图标数据
                    if (icon_info->data == nullptr) {
                        ESP_LOGW("MAIN", "  图标%d数据为空，跳过", icon_index);
                        continue;
                    }
                    
                    // 计算图标位置（基于左上角对齐）
                    // rel_x, rel_y 表示图标左上角在矩形中的相对位置
                    // 0.0, 0.0 = 图标左上角在矩形左上角
                    // 使用原始坐标（未缩放），EPD_ShowPictureScaled内部会处理缩放
                    int icon_x = rect->x + (int)(icon->rel_x * rect->width);
                    int icon_y = rect->y + (int)(icon->rel_y * rect->height);
                    
                    ESP_LOGI("MAIN", "  显示图标%d: 原始位置(%d,%d) 尺寸%dx%d", 
                            icon_index, icon_x, icon_y, icon_info->width, icon_info->height);
                    
                    drawPictureScaled(icon_x, icon_y, 
                                        icon_info->width, icon_info->height,
                                        icon_info->data, BLACK);
                } else {
                    ESP_LOGW("MAIN", "  图标索引%d超出范围[0-20]，跳过", icon_index);
                }
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
                display.drawRect(border_display_x, border_display_y, 
                                 border_display_width, 
                                 border_display_height, 
                                 border_color);
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
                display.drawLine(underline_display_x, underline_display_y, 
                            underline_display_x + underline_display_width, underline_display_y, 
                            BLACK);
                
                // 加粗下划线（绘制两条线）
                display.drawLine(underline_display_x, underline_display_y + 1, 
                            underline_display_x + underline_display_width, underline_display_y + 1, 
                            BLACK);
            }
        }
    }
    
    // ==================== 绘制焦点光标 ====================
    if (g_focus_mode_enabled && g_current_focus_rect >= 0 && g_current_focus_rect < rect_count) {
        drawFocusCursor(rects, g_current_focus_rect, global_scale);
        ESP_LOGI("FOCUS", "主界面绘制焦点光标在矩形%d", g_current_focus_rect);
    }
    
    // 更新显示
    display.display();
    display.display(true);
    display.powerOff();
    delay_ms(100);
    
    ESP_LOGI("MAIN", "主界面显示完成");
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

// 判断是否为中文字符串
bool isChineseText(const uint8_t *text) {
    if (text == NULL) return false;
    
    for (int i = 0; text[i] != '\0'; i++) {
        // UTF-8 中文字符判断：首字节在 0xE4-0xE9 范围内
        if ((text[i] & 0xE0) == 0xE0) {
            return true;
        }
    }
    return false;
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


void init_sleep_timer() {
    esp_timer_create_args_t timer_args = {
        .callback = &sleep_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sleep_timer",
        .skip_unhandled_events = false
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
        drawPictureScaled(icon_orig_x, icon_orig_y, 
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
            display.drawLine(display_x, display_y + i, 
                        display_x + display_width - 1, display_y + i, color);
        }
    } else {
        // 绘制空心矩形
        display.drawRect(display_x, display_y, 
                         display_width, 
                         display_height, 
                         color);
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
        display.drawLine(display_margin, y, 
                    display_margin + grid_cols * (display_cell_width + display_spacing), 
                    y, BLACK);
    }
    
    for (int col = 0; col <= grid_cols; col++) {
        int x = display_margin + col * (display_cell_width + display_spacing);
        display.drawLine(x, display_margin, 
                    x, display_margin + grid_rows * (display_cell_height + display_spacing), 
                    BLACK);
    }
    
    // 绘制每个单元格的矩形
    for (int row = 0; row < grid_rows; row++) {
        for (int col = 0; col < grid_cols; col++) {
            int cell_x = display_margin + col * (display_cell_width + display_spacing);
            int cell_y = display_margin + row * (display_cell_height + display_spacing);
            
            // 绘制空心矩形
            display.drawRect(cell_x, cell_y, 
                            display_cell_width, 
                            display_cell_height, 
                            BLACK);
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
        display.drawLine(x - border_thickness + i, y - border_thickness, 
                    x + width + border_thickness - i, y - border_thickness, 
                    border_color);
        // 下边框
        display.drawLine(x - border_thickness + i, y + height + border_thickness, 
                    x + width + border_thickness - i, y + height + border_thickness, 
                    border_color);
        // 左边框
        display.drawLine(x - border_thickness, y - border_thickness + i, 
                    x - border_thickness, y + height + border_thickness - i, 
                    border_color);
        // 右边框
        display.drawLine(x + width + border_thickness, y - border_thickness + i, 
                    x + width + border_thickness, y + height + border_thickness - i, 
                    border_color);
    }
    
    ESP_LOGI(TAG, "图标%d边框绘制完成: 位置(%d,%d), 尺寸(%dx%d), 边框厚度%d", 
            icon_index, x, y, width, height, border_thickness);
}


/**
 * @brief 设置可焦点矩形列表
 * @param rect_indices 矩形索引数组
 * @param count 数组长度
 */
void setFocusableRects(int* rect_indices, int count) {
    if (!rect_indices || count <= 0 || count > MAX_FOCUSABLE_RECTS) {
        ESP_LOGW("FOCUS", "无效的焦点矩形列表参数: count=%d", count);
        return;
    }
    
    g_focusable_rect_count = count;
    for (int i = 0; i < count; i++) {
        g_focusable_rect_indices[i] = rect_indices[i];
    }
    
    g_current_focus_index = 0;
    g_current_focus_rect = g_focusable_rect_indices[0];
    
    // 清空所有子数组配置
    for (int i = 0; i < MAX_FOCUSABLE_RECTS; i++) {
        g_sub_array_counts[i] = 0;
    }
    
    ESP_LOGI("FOCUS", "已设置可焦点矩形列表，共%d个矩形:", count);
    for (int i = 0; i < count; i++) {
        ESP_LOGI("FOCUS", "  [%d] 矩形索引: %d", i, g_focusable_rect_indices[i]);
    }
}

/**
 * @brief 为母数组中的某个元素设置子数组
 * @param parent_index 母数组中的索引位置（0-based）
 * @param sub_indices 子数组的矩形索引数组
 * @param sub_count 子数组长度
 */
void setSubArrayForParent(int parent_index, int* sub_indices, int sub_count) {
    if (parent_index < 0 || parent_index >= g_focusable_rect_count) {
        ESP_LOGW("FOCUS", "无效的母数组索引: %d (母数组共%d个元素)", parent_index, g_focusable_rect_count);
        return;
    }
    
    if (!sub_indices || sub_count <= 0 || sub_count > MAX_FOCUSABLE_RECTS) {
        ESP_LOGW("FOCUS", "无效的子数组参数: count=%d", sub_count);
        return;
    }
    
    g_sub_array_counts[parent_index] = sub_count;
    for (int i = 0; i < sub_count; i++) {
        g_sub_array_indices[parent_index][i] = sub_indices[i];
    }
    
    ESP_LOGI("FOCUS", "已为母数组索引%d（矩形%d）设置子数组，共%d个元素:", 
             parent_index, g_focusable_rect_indices[parent_index], sub_count);
    for (int i = 0; i < sub_count; i++) {
        ESP_LOGI("FOCUS", "  [%d] 子数组矩形索引: %d", i, sub_indices[i]);
    }
}

/**
 * @brief 为指定矩形索引设置子数组（自动查找该矩形在母数组中的位置）
 * @param rect_index 矩形的实际索引（例如：矩形0, 矩形1）
 * @param sub_indices 子数组的矩形索引数组
 * @param sub_count 子数组长度
 */
void setSubArrayForRect(int rect_index, int* sub_indices, int sub_count) {
    // 查找rect_index在母数组g_focusable_rect_indices中的位置
    int parent_pos = -1;
    for (int i = 0; i < g_focusable_rect_count; i++) {
        if (g_focusable_rect_indices[i] == rect_index) {
            parent_pos = i;
            break;
        }
    }
    
    if (parent_pos == -1) {
        ESP_LOGW("FOCUS", "矩形%d不在母数组中，无法设置子数组", rect_index);
        ESP_LOGI("FOCUS", "当前母数组包含的矩形:");
        for (int i = 0; i < g_focusable_rect_count; i++) {
            ESP_LOGI("FOCUS", "  [%d] 矩形%d", i, g_focusable_rect_indices[i]);
        }
        return;
    }
    
    ESP_LOGI("FOCUS", "矩形%d在母数组中的位置: %d", rect_index, parent_pos);
    setSubArrayForParent(parent_pos, sub_indices, sub_count);
}

/**
 * @brief 初始化焦点系统（默认所有矩形都可焦点）
 * @param total_rects 总矩形数量
 */
void initFocusSystem(int total_rects) {
    g_total_focusable_rects = total_rects;
    
    // 默认情况：所有矩形都可焦点
    g_focusable_rect_count = (total_rects < MAX_FOCUSABLE_RECTS) ? total_rects : MAX_FOCUSABLE_RECTS;
    for (int i = 0; i < g_focusable_rect_count; i++) {
        g_focusable_rect_indices[i] = i;
    }
    
    g_current_focus_index = 0;
    g_current_focus_rect = 0;
    g_focus_mode_enabled = true;
    
    ESP_LOGI("FOCUS", "焦点系统已初始化，共%d个矩形，默认全部可焦点", total_rects);
}

/**
 * @brief 移动焦点到下一个矩形（在可焦点列表中）
 */
void moveFocusNext() {
    if (g_in_sub_array) {
        // 子数组模式：在子数组中移动
        int sub_count = g_sub_array_counts[g_parent_focus_index_backup];
        if (sub_count <= 0) {
            ESP_LOGW("FOCUS", "子数组为空");
            return;
        }
        
        g_current_sub_focus_index = (g_current_sub_focus_index + 1) % sub_count;
        g_current_focus_rect = g_sub_array_indices[g_parent_focus_index_backup][g_current_sub_focus_index];
        
        ESP_LOGI("FOCUS", "【子数组】焦点向下移动: 子索引%d -> 矩形%d", g_current_sub_focus_index, g_current_focus_rect);
    } else {
        // 母数组模式：在母数组中移动
        if (g_focusable_rect_count <= 0) {
            ESP_LOGW("FOCUS", "没有可焦点矩形");
            return;
        }
        
        g_current_focus_index = (g_current_focus_index + 1) % g_focusable_rect_count;
        g_current_focus_rect = g_focusable_rect_indices[g_current_focus_index];
        
        ESP_LOGI("FOCUS", "【母数组】焦点向下移动: 列表索引%d -> 矩形%d", g_current_focus_index, g_current_focus_rect);
    }
}

/**
 * @brief 移动焦点到前一个矩形（在可焦点列表中）
 */
void moveFocusPrev() {
    if (g_in_sub_array) {
        // 子数组模式：在子数组中移动
        int sub_count = g_sub_array_counts[g_parent_focus_index_backup];
        if (sub_count <= 0) {
            ESP_LOGW("FOCUS", "子数组为空");
            return;
        }
        
        g_current_sub_focus_index = (g_current_sub_focus_index - 1 + sub_count) % sub_count;
        g_current_focus_rect = g_sub_array_indices[g_parent_focus_index_backup][g_current_sub_focus_index];
        
        ESP_LOGI("FOCUS", "【子数组】焦点向上移动: 子索引%d -> 矩形%d", g_current_sub_focus_index, g_current_focus_rect);
    } else {
        // 母数组模式：在母数组中移动
        if (g_focusable_rect_count <= 0) {
            ESP_LOGW("FOCUS", "没有可焦点矩形");
            return;
        }
        
        g_current_focus_index = (g_current_focus_index - 1 + g_focusable_rect_count) % g_focusable_rect_count;
        g_current_focus_rect = g_focusable_rect_indices[g_current_focus_index];
        
        ESP_LOGI("FOCUS", "【母数组】焦点向上移动: 列表索引%d -> 矩形%d", g_current_focus_index, g_current_focus_rect);
    }
}

/**
 * @brief 获取当前焦点矩形索引
 */
int getCurrentFocusRect() {
    return g_current_focus_rect;
}

/**
 * @brief 进入子数组模式
 * @return 是否成功进入（如果当前母数组元素没有子数组则返回false）
 */
bool enterSubArray() {
    if (g_in_sub_array) {
        ESP_LOGW("FOCUS", "已经在子数组模式中");
        return false;
    }
    
    // 检查当前母数组焦点位置是否有子数组
    if (g_sub_array_counts[g_current_focus_index] <= 0) {
        ESP_LOGI("FOCUS", "当前矩形%d没有配置子数组", g_current_focus_rect);
        return false;
    }
    
    // 备份母数组焦点位置
    g_parent_focus_index_backup = g_current_focus_index;
    
    // 进入子数组模式
    g_in_sub_array = true;
    g_current_sub_focus_index = 0;
    g_current_focus_rect = g_sub_array_indices[g_parent_focus_index_backup][0];
    
    ESP_LOGI("FOCUS", "✓ 进入子数组模式：母数组索引%d -> 子数组（共%d个元素），初始焦点在矩形%d",
             g_parent_focus_index_backup, 
             g_sub_array_counts[g_parent_focus_index_backup],
             g_current_focus_rect);
    
    return true;
}

/**
 * @brief 退出子数组模式，返回母数组
 */
void exitSubArray() {
    if (!g_in_sub_array) {
        ESP_LOGW("FOCUS", "当前不在子数组模式中");
        return;
    }
    
    // 恢复母数组焦点位置
    g_current_focus_index = g_parent_focus_index_backup;
    g_current_focus_rect = g_focusable_rect_indices[g_current_focus_index];
    
    // 退出子数组模式
    g_in_sub_array = false;
    g_current_sub_focus_index = 0;
    
    ESP_LOGI("FOCUS", "✓ 退出子数组模式，返回母数组索引%d，焦点在矩形%d",
             g_current_focus_index, g_current_focus_rect);
}

/**
 * @brief 处理焦点确认动作
 */
void handleFocusConfirm() {
    if (g_in_sub_array) {
        // 在子数组中：按确认键返回母数组
        ESP_LOGI("FOCUS", "【子数组】确认操作：退出子数组，返回母数组");
        exitSubArray();
    } else {
        // 在母数组中：尝试进入子数组
        ESP_LOGI("FOCUS", "【母数组】确认操作：尝试进入子数组（当前焦点在矩形%d）", g_current_focus_rect);
        if (!enterSubArray()) {
            ESP_LOGI("FOCUS", "当前矩形没有子数组，执行默认确认操作");
            // 这里可以添加其他确认逻辑
        }
    }
}

/**
 * @brief 绘制焦点光标
 * @param rects 矩形数组
 * @param focus_index 焦点矩形索引
 * @param global_scale 全局缩放比例
 */
void drawFocusCursor(RectInfo *rects, int focus_index, float global_scale) {
    if (!rects || focus_index < 0 || focus_index >= g_total_focusable_rects) {
        ESP_LOGW("FOCUS", "无效的焦点索引: %d (总数: %d)", focus_index, g_total_focusable_rects);
        return;
    }
    
    RectInfo* rect = &rects[focus_index];
    
    // 检查矩形是否有效
    if (rect->width <= 0 || rect->height <= 0) {
        ESP_LOGW("FOCUS", "焦点矩形%d无效: (%d,%d) %dx%d", focus_index, 
                rect->x, rect->y, rect->width, rect->height);
        return;
    }
    
    // 计算缩放后的位置和尺寸
    int display_x = (int)(rect->x * global_scale + 0.5f);
    int display_y = (int)(rect->y * global_scale + 0.5f);
    int display_width = (int)(rect->width * global_scale + 0.5f);
    int display_height = (int)(rect->height * global_scale + 0.5f);
    
    // 绘制焦点光标（在矩形四角绘制小三角形或方块）
    int cursor_size = 6;  // 光标大小
    
    // 左上角
    display.drawRect(display_x, display_y, 
                     cursor_size, cursor_size, BLACK);
    
    // 右上角
    display.drawRect(display_x + display_width - cursor_size, display_y, 
                     cursor_size, cursor_size, BLACK);
    
    // 左下角
    display.drawRect(display_x, display_y + display_height - cursor_size, 
                     cursor_size, cursor_size, BLACK);
    
    // 右下角
    display.drawRect(display_x + display_width - cursor_size, display_y + display_height - cursor_size, 
                     cursor_size, cursor_size, BLACK);
    
    ESP_LOGI("FOCUS", "已绘制焦点光标，位置：(%d,%d) 尺寸：%dx%d", 
            display_x, display_y, display_width, display_height);
}

/**
 * @brief 启动焦点测试模式
 * 直接进入单词界面并启用焦点功能，用于调试
 */
void startFocusTestMode() {
    ESP_LOGI("FOCUS", "========== 启动焦点测试模式 ==========");
    
    // 清屏
    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
    
    // 重新加载单词界面配置
    if (loadVocabLayoutFromConfig()) {
        ESP_LOGI("FOCUS", "单词界面布局已重新加载");
    }
    
    // 直接从矩形数据计算有效矩形数量，不依赖配置值
    extern RectInfo vocab_rects[MAX_VOCAB_RECTS];
    int valid_rect_count = 0;
    for (int i = 0; i < MAX_VOCAB_RECTS; i++) {
        if (vocab_rects[i].width > 0 && vocab_rects[i].height > 0) {
            valid_rect_count++;
        } else {
            break; // 遇到第一个无效矩形就停止计数
        }
    }
    
    initFocusSystem(valid_rect_count);
    ESP_LOGI("FOCUS", "焦点系统已初始化，共%d个有效矩形", valid_rect_count);
    
    // 显示单词界面
    displayScreen(SCREEN_VOCABULARY);
    
    // 设置为界面2（单词界面）
    interfaceIndex = 2;
    inkScreenTestFlag = 0;
    inkScreenTestFlagTwo = 0;
    
    ESP_LOGI("FOCUS", "========== 焦点测试模式已启动 ==========");
    ESP_LOGI("FOCUS", "使用按键控制：");
    ESP_LOGI("FOCUS", "  按键1 - 向上选择（前一个矩形）");
    ESP_LOGI("FOCUS", "  按键2 - 确认选择");
    ESP_LOGI("FOCUS", "  按键3 - 向下选择（下一个矩形）");
}



/**
 * @brief 显示休眠界面
 */
void displaySleepScreen(RectInfo *rects, int rect_count,
                       int status_rect_index, int show_border) {
    // 简单的休眠界面显示
    display.init(0, true, 2, true);
    display.clearScreen();
    display.display(true);
    delay_ms(50);
    display.setPartialWindow(0, 0, display.width(), display.height());
    
    // 显示时间和提示信息
    // TODO: 实现休眠界面的具体显示逻辑
    
    display.display();
    display.display(true);
    display.powerOff();
    delay_ms(100);
}
// 休眠界面矩形定义（3个矩形）
static RectInfo sleep_rects[3] = {
    // 矩形0：时间显示（居中大字体）
    {100, 60, 216, 80, CONTENT_CUSTOM, 24, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0,
     {{0.5f, 0.5f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 1,
     {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // 矩形1：日期显示
    {150, 150, 116, 30, CONTENT_CUSTOM, 16, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0,
     {{0.5f, 0.5f, 1}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 1,
     {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false},
    
    // 矩形2：唤醒提示
    {130, 190, 156, 40, CONTENT_CUSTOM, 16, ALIGN_CENTER, ALIGN_MIDDLE, 0, 0, 0, 0, 0,
     {{0.5f, 0.5f, 2}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}, {0.0f, 0.0f, 0}}, 1,
     {{0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}, {0.0f, 0.0f, CONTENT_NONE, 0, ALIGN_LEFT, ALIGN_TOP, 0, 0}}, 0, false}
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
            ESP_LOGI(TAG, "准备显示单词界面，矩形数量: %d", g_screen_manager.screens[screen_type].rect_count);
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
        .setup_icons_func = NULL,
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
        .rect_count = 0,  // 初始化为0，稍后从配置文件设置
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
        .rect_count = 0,  // 初始化为0，稍后从配置文件设置
        .status_rect_index = 1,
        .show_border = 1,
        .init_func = NULL,
        .setup_icons_func = NULL,  // 新系统不需要单独的图标设置函数
        .update_func = NULL  // 使用通用的displayVocabularyScreen
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

/**
 * @brief 更新主界面的矩形数量
 * @param new_rect_count 新的矩形数量
 */
void updateMainScreenRectCount(int new_rect_count) {
    if (new_rect_count >= 0 && new_rect_count <= MAX_MAIN_RECTS) {
        g_screen_manager.screens[SCREEN_HOME].rect_count = new_rect_count;
        ESP_LOGI(TAG, "已更新主界面矩形数量为: %d", new_rect_count);
    } else {
        ESP_LOGW(TAG, "无效的主界面矩形数量: %d，保持原值", new_rect_count);
    }
}

/**
 * @brief 更新单词界面的矩形数量
 * @param new_rect_count 新的矩形数量
 */
void updateVocabScreenRectCount(int new_rect_count) {
    if (new_rect_count >= 0 && new_rect_count <= MAX_VOCAB_RECTS) {
        g_screen_manager.screens[SCREEN_VOCABULARY].rect_count = new_rect_count;
        ESP_LOGI(TAG, "已更新单词界面矩形数量为: %d", new_rect_count);
    } else {
        ESP_LOGW(TAG, "无效的矩形数量: %d，保持原值", new_rect_count);
    }
}

/**
 * @brief 获取主界面的矩形数量
 * @return 主界面的矩形数量
 */
int getMainScreenRectCount() {
    return g_screen_manager.screens[SCREEN_HOME].rect_count;
}

/**
 * @brief 获取单词界面的矩形数量
 * @return 单词界面的矩形数量
 */
int getVocabScreenRectCount() {
    return g_screen_manager.screens[SCREEN_VOCABULARY].rect_count;
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
                        enterVocabularyScreen(); // 进入单词界面
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
					 	display.init(0, true, 2, true);
						display.clearScreen();
						display.display(true);  //局刷之前先对E-Paper进行清屏操作
						display.setPartialWindow(0, 0, display.width(), display.height());
						ESP_LOGI(TAG,"inkScreenTestFlagTwo\r\n");
						// 遍历所有图像
						// for(int i = 0; i < GAME_PEOPLE_COUNT; i++) {
						// 	EPD_ShowPicture(60,40,128,128,gamePeople[i],BLACK);
						// 	display.display();
						// 	display.display(true);
						// 	vTaskDelay(1000);
						// 	ESP_LOGI(TAG,"gamePeople[%d]\r\n",i);
						// }
						display.powerOff();
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
			case 1:  // 按键1：上一个焦点/向上选择
                update_activity_time(); 
                
                if (g_focus_mode_enabled) {
                    // 焦点模式：向上移动焦点
                    moveFocusPrev();
                    ESP_LOGI("FOCUS", "按键1：焦点向上移动到矩形%d", g_current_focus_rect);
                    
                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                    // 根据当前界面类型重新显示
                    displayScreen(g_screen_manager.current_screen);
                } else {
                    // 原有逻辑：绘制下划线
                    drawUnderlineForIconEx(0);
                }
                
				inkScreenTestFlag = 0;
			break;
			case 2:  // 按键2：确认/选择
                update_activity_time(); 
                
                if (g_focus_mode_enabled) {
                    // 焦点模式：确认操作
                    handleFocusConfirm();
                    ESP_LOGI("FOCUS", "按键2：确认操作，当前焦点在矩形%d", g_current_focus_rect);

                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                    // 根据当前界面类型重新显示
                    displayScreen(g_screen_manager.current_screen);
                } else {
                    // 原有逻辑：绘制下划线
                    drawUnderlineForIconEx(1);
                }
                
				inkScreenTestFlag = 0;
			break;
			case 3:  // 按键3：下一个焦点/向下选择
                update_activity_time(); 
                
                if (g_focus_mode_enabled) {
                    // 焦点模式：向下移动焦点
                    moveFocusNext();
                    ESP_LOGI("FOCUS", "按键3：焦点向下移动到矩形%d", g_current_focus_rect);
                        
                    clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
                    
                    // 根据当前界面类型重新显示
                    displayScreen(g_screen_manager.current_screen);
                } else {
                    // 原有逻辑：绘制下划线
                    drawUnderlineForIconEx(2);
                }
                
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
            {
                clearDisplayArea(0, 0, setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);

                ESP_LOGI(TAG,"############显示主界面（使用新图标布局系统）\r\n");
                update_activity_time(); 
                
                // 直接从矩形数据计算有效矩形数量
                extern RectInfo rects[MAX_MAIN_RECTS];
                int valid_rect_count = 0;
                for (int i = 0; i < MAX_MAIN_RECTS; i++) {
                    if (rects[i].width > 0 && rects[i].height > 0) {
                        valid_rect_count++;
                        ESP_LOGI("FOCUS", "有效矩形%d: (%d,%d) %dx%d", i, 
                                rects[i].x, rects[i].y, 
                                rects[i].width, rects[i].height);
                    } else {
                        ESP_LOGI("FOCUS", "无效矩形%d: (%d,%d) %dx%d", i, 
                                rects[i].x, rects[i].y, 
                                rects[i].width, rects[i].height);
                        break; // 遇到第一个无效矩形就停止计数
                    }
                }
                
                ESP_LOGI("FOCUS", "检测到有效矩形数量: %d", valid_rect_count);
                
                // 使用实际检测到的有效矩形数量初始化焦点系统
                initFocusSystem(valid_rect_count);
                
                // 尝试从配置文件加载自定义的可焦点矩形列表（母数组）
                if (loadFocusableRectsFromConfig("main")) {
                    ESP_LOGI("FOCUS", "已从配置文件加载主界面焦点矩形列表");
                } else {
                    ESP_LOGI("FOCUS", "使用默认焦点配置（所有矩形都可焦点）");
                }
                
                // 加载子数组配置
                if (loadAndApplySubArrayConfig("main")) {
                    ESP_LOGI("FOCUS", "已从配置文件加载并应用主界面子数组配置");
                } else {
                    ESP_LOGI("FOCUS", "未加载主界面子数组配置或配置为空");
                }
                
                // 设置当前界面为主界面
                g_screen_manager.current_screen = SCREEN_HOME;
                
                // 显示主界面
                displayScreen(SCREEN_HOME);
                
                interfaceIndex = 1;
				inkScreenTestFlag = 0;
            }
			break;
			case 8:  // 新增：启动焦点测试模式
                ESP_LOGI("FOCUS", "========== 按键8：启动焦点测试模式 ==========");
                startFocusTestMode();
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

// ================= GXEPD2 中景园 3.7" 测试函数 =================
/**
 * @brief 使用 GXEPD2 库在中景园 3.7" 墨水屏上显示文字和图片
 * 
 * 屏幕参数:
 * - 分辨率: 240x416 (宽x高)
 * - 控制器: UC8253 (GDEY037T03)
 */
void ink_screen_test_gxepd2_370()
{
    ESP_LOGI(TAG, "=== 开始 GXEPD2 中景园 3.7\" 测试（UC8253）===");
    
    // ===== 关键修复：延迟启动，避免与 WiFi 初始化冲突 =====
    ESP_LOGI(TAG, "等待 WiFi 稳定后再初始化墨水屏...");
    vTaskDelay(3000 / portTICK_PERIOD_MS);  // 延迟 3 秒，等待 WiFi 和系统稳定
    
    // 更新全局屏幕尺寸（供其他函数使用）
    setInkScreenSize.screenWidth = GxEPD2_370_GDEY037T03::WIDTH;   // 240
    setInkScreenSize.screenHeigt = GxEPD2_370_GDEY037T03::HEIGHT;  // 416
    
    ESP_LOGI(TAG, "屏幕尺寸: %dx%d (UC8253 控制器)", setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
    
    // ===== 步骤 0: 初始化墨水屏专用的 SPI3 (VSPI) =====
    ESP_LOGI(TAG, "初始化墨水屏专用 SPI3 (SCK=48, MOSI=47)...");
    EPD_SPI.begin(EPD_SPI_SCK, EPD_SPI_MISO, EPD_SPI_MOSI, EPD_CS_PIN);
    ESP_LOGI(TAG, "墨水屏 SPI3 初始化完成");
    ESP_LOGI(TAG, "注意: SD 卡使用 SPI2 (默认 SPI), 墨水屏使用 SPI3 (EPD_SPI), 两者独立工作");
    
    // ===== 步骤 1: 初始化显示 (使用 EPD_SPI) =====
    ESP_LOGI(TAG, "初始化 GXEPD2 显示驱动...");
    display.epd2.selectSPI(EPD_SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));  // 设置使用 EPD_SPI
    display.init(0);  // 初始化（0=不启用调试输出）
    vTaskDelay(200 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "GXEPD2 初始化完成");
    
    // ===== 步骤 2: 设置旋转（可选）=====
    display.setRotation(1);  // 竖屏模式
    ESP_LOGI(TAG, "设置旋转: 竖屏模式 (rotation=1)");
    
    // ===== 步骤 3: 设置文本颜色 =====
    display.setTextColor(GxEPD_BLACK);
    
    // ===== 步骤 4: 首次全屏刷新（清白）=====
    ESP_LOGI(TAG, "准备首次全屏刷新（清白）...");
    display.setFullWindow();
    ESP_LOGI(TAG, "setFullWindow() 完成");
    
    ESP_LOGI(TAG, "调用 firstPage()...");
    display.firstPage();
    ESP_LOGI(TAG, "firstPage() 返回");
    // do
    // {
    //     display.fillScreen(GxEPD_WHITE);
    // }
    // while (display.nextPage());
     display.fillScreen(GxEPD_WHITE);
    ESP_LOGI(TAG, "屏幕已清白");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    // ===== 步骤 5: 显示内容 =====
    ESP_LOGI(TAG, "开始测试 GXEPD2 文字显示（3.7寸屏）...");
    
    // ===== 初始化SD卡字库 =====
    ESP_LOGI(TAG, "初始化SD卡字库...");
    // SD 卡使用 SPI2 (默认 SPI 对象), 与墨水屏的 SPI3 独立，无需切换
    if (initChineseFontFromSD("/sd/fangsong_gb2312_16x16.bin", 16)) {
        ESP_LOGI(TAG, "SD卡字库初始化成功!");
    } else {
        ESP_LOGW(TAG, "SD卡字库初始化失败,将使用内置字体");
    }
    
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        
        // // ===== 使用SD卡字库显示中文文本 =====
        // ESP_LOGI(TAG, "使用SD卡字库显示中文...");
        
        // // 示例1: 在指定位置显示中文
         drawChineseText(display, 10, 10, "你好世界!", GxEPD_BLACK);
        
        // // 示例2: 显示多行中文
        // drawChineseText(display, 10, 40, "这是一段测试文本", GxEPD_BLACK);
        // drawChineseText(display, 10, 70, "使用SD卡字库文件", GxEPD_BLACK);
        
        // // 示例3: 居中显示
        // drawChineseTextCentered(display, 120, "居中显示的文字", GxEPD_BLACK);
        
        // // 示例4: 混合中英文
        // drawChineseText(display, 10, 150, "混合ABC测试123", GxEPD_BLACK);
        
        // ===== 以下是原有代码(已注释) =====
        // 绘制边框
    //     display.drawRect(0, 0, 240, 416, GxEPD_BLACK);
    //     display.drawRect(2, 2, 236, 412, GxEPD_BLACK);
    //     ESP_LOGI(TAG, "绘制测试边框完成");
        
    //     ESP_LOGI(TAG, "绘制中文位图和文字...");
        
    //     // ===== 显示三种字号的中文位图 =====
    //     // 16pt 仿宋 "测试文字"
    //     drawFangsongBitmaps(display, 10, 50, 1);
    //     ESP_LOGI(TAG, "16pt 中文位图完成");
        
    //     // 英文
    //     display.setFont(NULL);
    //     display.setTextSize(2);
    //     display.setCursor(100, 50);
    //     display.print("ABC");
    //     ESP_LOGI(TAG, "英文 ABC 完成");
        
    //     // 20pt 仿宋 "字文测试"
    //    // drawFangsong20ptBitmaps(display, 10, 100, 1);
    //     ESP_LOGI(TAG, "20pt 中文位图完成");
        
    //     // 24pt 仿宋 "这是一个测试"
    //     drawFangsong24ptString(display, 10, 80, "这是一个测试");
    //     ESP_LOGI(TAG, "24pt 中文字符串完成");
        
    //     // 16pt 仿宋 "生成字库" - Python高质量版本
    //     drawFangsongShengchengString(display, 10, 120, "生成字库");
    //     ESP_LOGI(TAG, "16pt 高质量中文字符串完成 (生成字库)");
        
    //     // 16pt 仿宋 "美式咖啡" - Python高质量版本
    //     drawFangsongGb2312String(display, 10, 180, "下班");
    //     ESP_LOGI(TAG, "16pt 高质量中文字符串完成 (美式咖啡)");
        
        // ===== 废弃: 显示用户上传的图片位图 (.h 文件) =====
        // 现已改用 SD 卡 .bin 图片系统
        // display.drawBitmap(0, 0, IMAGE_BITMAP, IMAGE_WIDTH, IMAGE_HEIGHT, GxEPD_BLACK);
        
        // 从SD卡显示图片 (路径相对于SD卡根目录)
        //displayImageFromSD("/test2_400x216.bin", 0, 0, display);
        
        // 状态文本
        // display.setFont(NULL);
        // display.setTextSize(2);
        // display.setCursor(10, 200);
        // display.print("Status: OK");
        // ESP_LOGI(TAG, "状态文本完成");
        
        // display.setFont(NULL);
        // display.setTextSize(1);
    }
    while (display.nextPage());
    
    ESP_LOGI(TAG, "等待 5 秒后进入休眠...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    // ===== 步骤 6: 进入休眠模式 =====
    display.hibernate();
    ESP_LOGI(TAG, "=== GXEPD2 测试完成，显示已进入休眠 ===");
}

// ================= GXEPD2 微雪 2.13" V4 测试函数 =================
/**
 * @brief 使用 GXEPD2 库在微雪 2.13" V4 墨水屏上显示文字和图片
 * 
 * 功能说明:
 * 1. 初始化 GXEPD2 显示对象
 * 2. 清屏并绘制文字（使用 Adafruit GFX 字体）
 * 3. 绘制简单图形（矩形、圆形、线条）
 * 4. 显示位图图像（如果有图片数据）
 * 5. 刷新显示并进入休眠模式
 * 
 * 屏幕参数:
 * - 分辨率: 122x250 (宽x高，可视区域)
 * - 完整宽度: 128 (实际控制器宽度)
 * - 控制器: SSD1675B (GDEH0213B73)
 */
void ink_screen_test_gxepd2_microsnow_213()
{
    ESP_LOGI(TAG, "=== 开始 GXEPD2 微雪 2.13\" V4 测试（SSD1680）===");
    
    // ===== 关键修复：延迟启动，避免与 WiFi 初始化冲突 =====
    ESP_LOGI(TAG, "等待 WiFi 稳定后再初始化墨水屏...");
    vTaskDelay(3000 / portTICK_PERIOD_MS);  // 延迟 3 秒，等待 WiFi 和系统稳定
    
    // 更新全局屏幕尺寸（供其他函数使用）
    setInkScreenSize.screenWidth = GxEPD2_213_BN::WIDTH_VISIBLE;  // 122 (可见宽度)
    setInkScreenSize.screenHeigt = GxEPD2_213_BN::HEIGHT;         // 250
    
    ESP_LOGI(TAG, "屏幕尺寸: %dx%d (SSD1680 控制器)", setInkScreenSize.screenWidth, setInkScreenSize.screenHeigt);
    
    // ===== 步骤 0: 初始化 Arduino SPI（使用默认 SPI 对象）=====
    // 指定引脚：SCK=48, MISO=-1（不需要）, MOSI=47
    ESP_LOGI(TAG, "初始化 Arduino SPI (SCK=48, MOSI=47)...");
    SPI.begin(48, -1, 47, -1);
    ESP_LOGI(TAG, "Arduino SPI 初始化完成");
    
    // ===== 步骤 1: 初始化显示 =====
    ESP_LOGI(TAG, "初始化 GXEPD2 显示驱动...");
    display.init(0);  // 初始化（0=不启用调试输出）
    // 如果需要调试输出，使用: display.init(115200);
    
    vTaskDelay(200 / portTICK_PERIOD_MS);  // 增加延迟，确保初始化稳定
    
    ESP_LOGI(TAG, "GXEPD2 初始化完成");
    
    // ===== 步骤 2: 设置旋转（可选）=====
    // 2.13寸屏（122x250）横屏模式：rotation=1 或 3
    // 如果显示方向不对，可尝试 0, 1, 2, 3 四个值
    display.setRotation(1);  // 横屏模式，如果不对请改为 0, 2 或 3
    ESP_LOGI(TAG, "设置旋转: 横屏模式 (rotation=1)");
    
    // ===== 步骤 3: 设置文本颜色 =====
    display.setTextColor(GxEPD_BLACK);  // 黑色文字
    
    // ===== 步骤 4: 首次全屏刷新（清白）=====
    ESP_LOGI(TAG, "准备首次全屏刷新（清白）...");
    display.setFullWindow();
    ESP_LOGI(TAG, "setFullWindow() 完成");
    
    ESP_LOGI(TAG, "调用 firstPage()...");
    display.firstPage();
    ESP_LOGI(TAG, "firstPage() 返回");
    do
    {
        display.fillScreen(GxEPD_WHITE);  // 填充白色背景
    }
    while (display.nextPage());
    
    ESP_LOGI(TAG, "屏幕已清白");
    vTaskDelay(500 / portTICK_PERIOD_MS);  // 延长等待时间
    
    // ===== 步骤 5: 使用 GXEPD2 实现文字缩放（纯英文/数字）=====
    ESP_LOGI(TAG, "开始测试 GXEPD2 文字缩放功能（2.13寸屏）...");
    
    // 使用 GXEPD2 绘制不同缩放倍数的文字
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);  // 清白屏
        display.setTextColor(GxEPD_BLACK);
        
        // 先画一个测试边框,确认屏幕在工作
        display.drawRect(0, 0, 122, 250, GxEPD_BLACK);  // 外边框
        display.drawRect(2, 2, 118, 246, GxEPD_BLACK);  // 内边框
        ESP_LOGI(TAG, "绘制测试边框完成");
        
        ESP_LOGI(TAG, "绘制中文位图和文字...");
        
        // ===== 废弃: 使用 .h 字体文件的代码 =====
        // 现已改用 SD 卡 .bin 字库系统
        /*
        // ===== 混合显示：英文字体 + 中文位图（调整布局适应2.13寸屏幕）=====
        
        // 标题区域 - 使用位图方式显示中文（避免GFX字体Unicode问题）
        drawFangsongBitmaps(display, 5, 30, 1);  // "测试文字" 16pt 1x大小
        ESP_LOGI(TAG, "16pt 中文位图完成");
        
        // 英文使用默认字体测试
        display.setFont(NULL);
        display.setTextSize(2);
        display.setCursor(70, 30);
        display.print("AB");
        ESP_LOGI(TAG, "英文 AB 完成");
        
        // ===== 使用仿宋20pt位图显示中文 (避免GFX字体花屏问题) =====
        drawFangsong20ptBitmaps(display, 5, 70, 1);  // "字文测试" 20pt 1x大小
        ESP_LOGI(TAG, "20pt 中文位图完成");
        
        // ===== 使用仿宋24pt位图显示中文字符串 (缩短以适应屏幕宽度) =====
        drawFangsong24ptString(display, 5, 120, "这是测试");  // 24pt UTF-8字符串（4个字）
        ESP_LOGI(TAG, "24pt 中文字符串完成");
        */
        
        // 显示英文和数字 (使用默认字体)
        display.setFont(NULL);
        display.setTextSize(3);
        display.setCursor(5, 180);
        display.print("OK!");
        ESP_LOGI(TAG, "状态文本完成");
        
        // 恢复默认字体
        display.setFont(NULL);
        display.setTextSize(1);
        
        // // 温度 (用 T: 表示温度)
        // display.setCursor(10, 60);
        // display.print("Temp: 25");
        // display.print((char)247);  // ° 符号
        // display.print("C");
        
        // // 湿度 (用 H: 表示湿度)
        // display.setCursor(10, 100);
        // display.print("Humi: 60%");
        
        // // 电量 (用 Bat: 表示电池)
        // display.setCursor(10, 140);
        // display.print("Bat: 85%");
        
        // // WiFi 状态
        // display.setCursor(10, 180);
        // display.print("WiFi: OK");
        
        // // ===== 显示中文 "爆裂" - 使用位图方式 =====
        // drawBaiLie(display, 10, 200, 2);  // 在 (10, 230) 显示，放大 2 倍
        
        // 底部提示
        // display.setFont(NULL);
        // display.setTextSize(1);
        // display.setCursor(10, 300);
        // display.print("Last update: 12:34:56");
        
        // // 4. 缩放 4x
        // display.setTextSize(4);
        // display.setCursor(20, 180);
        // display.print("TEST");
        // display.setTextSize(1);
        // display.setCursor(180, 210);
        // display.print(" x4.0");
        
        // // 5. 缩放 5x（3.7寸屏幕更大，可以显示更多内容）
        // display.setTextSize(5);
        // display.setCursor(20, 260);
        // display.print("BIG");
        // display.setTextSize(1);
        // display.setCursor(180, 290);
        // display.print(" x5.0");
        // display.setTextSize(1);
        // display.setCursor(5, 0);
        // display.print("Font Size Test:");
    }
    while (display.nextPage());
    
    // ESP_LOGI(TAG, "文字缩放测试完成（3.7寸屏）！");
    // ESP_LOGI(TAG, "屏幕显示文字的 5 种缩放:");
    // ESP_LOGI(TAG, "  - 1x (默认)");
    // ESP_LOGI(TAG, "  - 2x (双倍)");
    // ESP_LOGI(TAG, "  - 3x (三倍)");
    // ESP_LOGI(TAG, "  - 4x (四倍)");
    // ESP_LOGI(TAG, "  - 5x (五倍 - BIG)");
    
    // ===== 步骤 6: 等待观察 =====
    ESP_LOGI(TAG, "等待 5 秒后进入休眠...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    /* 局部刷新测试（暂时禁用，先确认全屏刷新工作正常）
    // 定义局部更新区域（左上角坐标 + 宽高）
    uint16_t partial_x = 5;
    uint16_t partial_y = 200;
    uint16_t partial_w = 100;
    uint16_t partial_h = 30;
    
    display.setPartialWindow(partial_x, partial_y, partial_w, partial_h);
    display.firstPage();
    do
    {
        display.fillRect(partial_x, partial_y, partial_w, partial_h, GxEPD_WHITE);
        display.setCursor(partial_x + 5, partial_y + 10);
        display.setTextSize(1);
        display.print("Partial OK");
    }
    while (display.nextPage());
    
    ESP_LOGI(TAG, "局部刷新完成");
    */
    
    // ===== 步骤 7: 进入休眠模式 =====
    display.hibernate();
    
    ESP_LOGI(TAG, "=== GXEPD2 测试完成，显示已进入休眠 ===");
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
        // ===== 步骤 0: 初始化墨水屏专用的 SPI3 (VSPI) =====
    ESP_LOGI(TAG, "初始化墨水屏专用 SPI3 (SCK=48, MOSI=47)...");
    EPD_SPI.begin(EPD_SPI_SCK, EPD_SPI_MISO, EPD_SPI_MOSI, EPD_CS_PIN);
    ESP_LOGI(TAG, "墨水屏 SPI3 初始化完成");
    ESP_LOGI(TAG, "注意: SD 卡使用 SPI2 (默认 SPI), 墨水屏使用 SPI3 (EPD_SPI), 两者独立工作");
    
    // ===== 步骤 1: 初始化显示 (使用 EPD_SPI) =====
    ESP_LOGI(TAG, "初始化 GXEPD2 显示驱动...");
    display.epd2.selectSPI(EPD_SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));  // 设置使用 EPD_SPI
    display.init(0);  // 初始化（0=不启用调试输出）
    vTaskDelay(200 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "GXEPD2 初始化完成");
    
    // ===== 步骤 2: 设置旋转（可选）=====
    display.setRotation(1);  // 竖屏模式
    ESP_LOGI(TAG, "设置旋转: 竖屏模式 (rotation=1)");

    // ===== 步骤 3: 设置文本颜色 =====
    display.setTextColor(GxEPD_BLACK);
    
    // ===== 步骤 4: 首次全屏刷新（清白）=====
    ESP_LOGI(TAG, "准备首次全屏刷新（清白）...");
    display.setFullWindow();
    ESP_LOGI(TAG, "setFullWindow() 完成");
    ESP_LOGI(TAG, "调用 firstPage()...");
    display.firstPage();
    ESP_LOGI(TAG, "firstPage() 返回");
       // ===== 初始化SD卡字库 =====
    ESP_LOGI(TAG, "初始化SD卡字库...");
    // SD 卡使用 SPI2 (默认 SPI 对象), 与墨水屏的 SPI3 独立，无需切换
    if (initChineseFontFromSD("/sd/fangsong_gb2312_16x16.bin", 16)) {
        ESP_LOGI(TAG, "SD卡字库初始化成功!");
    } else {
        ESP_LOGW(TAG, "SD卡字库初始化失败,将使用内置字体");
    }
    
    // ===== 初始化英文字体系统 =====
    ESP_LOGI(TAG, "正在初始化英文字体系统...");
    if (initEnglishFontSystem()) {
        ESP_LOGI(TAG, "✅ 英文字体系统初始化成功");
    } else {
        ESP_LOGW(TAG, "❌ 英文字体系统初始化失败，请检查SD卡文件 /sd/comic_sans_ms_bold_*.bin");
    }
   // drawChineseText(display, 10, 10, "沉默的时光", GxEPD_BLACK);

    // 初始化图标布局
    initMainScreenIcons();   // 主界面图标
    // 注意：不要在加载配置前初始化单词界面图标，避免覆盖配置文件
    
    // 初始化文本布局
    // initVocabScreenTexts();  // 单词界面文本位置
    
    initAllScreens();  // 先初始化屏幕配置（rect_count为0）
    initLayoutFromConfig();  // 加载配置文件并更新rect_count
    
    // 强制更新矩形数量，确保从配置文件读取的值生效
    int main_count = getMainScreenRectCount();
    int vocab_count = getVocabScreenRectCount();
    
    ESP_LOGI(TAG, "配置加载后 - 主界面矩形数量: %d, 单词界面矩形数量: %d", main_count, vocab_count);
    
    // 如果配置文件没有正确设置，使用默认值并重新设置
    if (main_count <= 0) {
        // updateMainScreenRectCount(sizeof(rects) / sizeof(rects[0]));
        // main_count = getMainScreenRectCount();
        // ESP_LOGI(TAG, "使用默认主界面矩形数量: %d", main_count);
        ESP_LOGW(TAG, "主界面配置文件未加载或矩形数量为0，请通过web端配置主界面布局");
        ESP_LOGW(TAG, "注意：不要使用数组大小作为默认值，避免覆盖用户配置");
    }
    
    // 单词界面矩形数量：不再使用数组大小作为默认值
    // 如果配置文件加载失败或矩形数量为0，保持为0，等待用户通过web端配置
    if (vocab_count <= 0) {
        ESP_LOGW(TAG, "单词界面配置文件未加载或矩形数量为0，请通过web端配置单词界面布局");
        ESP_LOGW(TAG, "注意：不要使用数组大小作为默认值，避免覆盖用户配置");
        // 不再调用 updateVocabScreenRectCount(sizeof(vocab_rects) / sizeof(vocab_rects[0]));
        // 保持 vocab_count 为 0，等待用户配置
    }
    
    // 设置主界面的rect_count为从配置加载的实际值
    rect_count = main_count;
    ESP_LOGI(TAG, "全局rect_count已设置为: %d", rect_count);
    
    // 由于initLayoutFromConfig()已经加载了单词界面配置，这里不需要重复调用
    // 直接初始化默认图标（如果配置文件中没有图标信息）
    ESP_LOGI(TAG, "单词界面配置已通过initLayoutFromConfig()加载，当前矩形数量: %d", getVocabScreenRectCount());
    // initVocabScreenIcons();  // 单词界面图标（如果配置文件中没有则使用默认）
    // setupHomeScreenIcons();
    displayScreen(SCREEN_HOME);
	//updateDisplayWithMain(rects,rect_count, 0, 1);  // 最后一个参数为1表示显示边框
	Uart0.printf("ink_screen_test_gxepd2_370fast refresh\r\n");

	    // 1. 中景园 3.7" UC8253 (240x416) - 当前使用
     //();
    
    // 2. 微雪 2.13" V4 SSD1680 (122x250)
    // ink_screen_test_gxepd2_microsnow_213();

    BaseType_t task_created = xTaskCreatePinnedToCore(ink_screen_show, 
                                                        "ink_screen_show", 
                                                        8192, 
                                                        NULL, 
                                                        4, 
                                                        &_eventTaskHandle, 
                                                        0);
}


#include "src/Grbl.h"
#include "../components/grbl_esp32s3/Grbl_Esp32/src/BL_add/ink_screen/ink_screen.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"

static const char* TAG = "MAIN";

void setup() {
     ESP_LOGI(TAG, "========== [SETUP] 1. 开始执行setup() ==========");
     
     // 初始化SPIFFS文件系统
     ESP_LOGI(TAG, "[SETUP] 1.0 初始化SPIFFS文件系统...");
     esp_vfs_spiffs_conf_t conf = {
         .base_path = "/spiffs",
         .partition_label = "storage",
         .max_files = 5,
         .format_if_mount_failed = true
     };
     esp_err_t ret = esp_vfs_spiffs_register(&conf);
     if (ret != ESP_OK) {
         ESP_LOGE(TAG, "SPIFFS挂载失败: %s", esp_err_to_name(ret));
         return;
     }
     ESP_LOGI(TAG, "[SETUP] 1.1 SPIFFS文件系统挂载成功");
     
     ESP_LOGI(TAG, "========== [SETUP] 1.2 准备调用grbl_init() ==========");
     ESP_LOGI(TAG, "[SETUP] 1.2.1 当前堆内存状态:");
     ESP_LOGI(TAG, "[SETUP] 1.2.2 - 空闲堆内存: %d bytes", esp_get_free_heap_size());
     ESP_LOGI(TAG, "[SETUP] 1.2.3 - 最小空闲堆: %d bytes", esp_get_minimum_free_heap_size());
     
     // PSRAM内存检测
     ESP_LOGI(TAG, "[SETUP] 1.2.4 内存详细信息:");
     ESP_LOGI(TAG, "  ├─ 内部RAM可用: %d bytes (%.1f KB)", 
              heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
              heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024.0f);
     ESP_LOGI(TAG, "  ├─ PSRAM可用: %d bytes (%.1f MB)", 
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024.0f / 1024.0f);
     ESP_LOGI(TAG, "  └─ 总可用内存: %d bytes (%.1f MB)", 
              heap_caps_get_free_size(MALLOC_CAP_8BIT),
              heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024.0f / 1024.0f);
     
     ESP_LOGI(TAG, "🔥🔥🔥 即将调用 grbl_init() - 如果程序卡住，说明问题在grbl_init()内部 🔥🔥🔥");
      grbl_init();
     ESP_LOGI(TAG, "🎉🎉🎉 grbl_init() 成功完成！继续执行... 🎉🎉🎉");
     
     // 等待墨水屏初始化完成
     ESP_LOGI(TAG, "[SETUP] 2.1 开始3秒延迟...");
     delay(3000);
     ESP_LOGI(TAG, "[SETUP] 3. 延迟3秒完成");
     
     // ==================== 预加载所有界面到PSRAM ====================
     ESP_LOGI(TAG, "========== 开始预加载所有界面到PSRAM ==========");
     ESP_LOGI(TAG, "[SETUP] 4.1 预加载前内存状态:");
     ESP_LOGI(TAG, "  ├─ PSRAM可用: %d bytes (%.1f MB)", 
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024.0f / 1024.0f);
     
     int loaded_count = preloadAllScreens();
     
     ESP_LOGI(TAG, "[SETUP] 4.2 预加载后内存状态:");
     ESP_LOGI(TAG, "  ├─ PSRAM可用: %d bytes (%.1f MB)", 
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024.0f / 1024.0f);
     ESP_LOGI(TAG, "  └─ 成功加载: %d 个界面", loaded_count);
     
     if (loaded_count == 0) {
         ESP_LOGE(TAG, "❌ 没有加载任何界面！");
         ESP_LOGI(TAG, "✅ Setup完成（无界面）！");
         return;
     }
     
     // 显示所有已加载的界面列表
     ESP_LOGI(TAG, "[SETUP] 4.3 已加载的界面列表:");
     for (int i = 0; i < loaded_count; i++) {
         ESP_LOGI(TAG, "  [%d] %s", i, getScreenName(i));
     }

     // 切换到第一个界面（索引0）
     ESP_LOGI(TAG, "========== 显示默认界面 ==========");
     display.setFullWindow();

     bool success = switchToScreen(0);  // 显示第一个界面
     
     // 手动触发一次iconroll测试
     ESP_LOGI(TAG, "========== 手动测试iconroll ==========");
     extern RectInfo* g_json_rects;
     extern int g_json_rect_count;
     if (g_json_rects && g_json_rect_count > 6) {
         RectInfo* test_rect = &g_json_rects[6];
         ESP_LOGI(TAG, "矩形6: icon_count=%d, icon_roll_count=%d", 
                  test_rect->icon_count, test_rect->icon_roll_count);
         if (test_rect->icon_roll_count > 0) {
             ESP_LOGI(TAG, "  icon_roll[0]: arr='%s', idx='%s', auto_roll=%d",
                      test_rect->icon_rolls[0].icon_arr,
                      test_rect->icon_rolls[0].idx,
                      test_rect->icon_rolls[0].auto_roll);
             ESP_LOGI(TAG, "  位置: rel_x=%.3f, rel_y=%.3f",
                      test_rect->icon_rolls[0].rel_x,
                      test_rect->icon_rolls[0].rel_y);
         }
     }

     // ==================== 启动自动滚动定时器 ====================
     ESP_LOGI(TAG, "========== 初始化自动滚动定时器 ==========");
     const esp_timer_create_args_t auto_roll_timer_args = {
         .callback = &processAutoRollAnimations,
         .arg = NULL,
         .dispatch_method = ESP_TIMER_TASK,
         .name = "auto_roll",
         .skip_unhandled_events = false
     };
     esp_timer_handle_t auto_roll_timer;
     esp_err_t timer_err = esp_timer_create(&auto_roll_timer_args, &auto_roll_timer);
     if (timer_err == ESP_OK) {
         timer_err = esp_timer_start_periodic(auto_roll_timer, 2000000); // 2秒 (微秒)
         if (timer_err == ESP_OK) {
             ESP_LOGI(TAG, "✅ 自动滚动定时器启动成功 (间隔: 2000ms)");
         } else {
             ESP_LOGE(TAG, "❌ 定时器启动失败: %s", esp_err_to_name(timer_err));
         }
     } else {
         ESP_LOGE(TAG, "❌ 定时器创建失败: %s", esp_err_to_name(timer_err));
     }

     //$inkScreen/Test=3

}

static uint32_t loop_count = 0;
static bool first_loop = true;

void loop() {
     // 自动滚动动画已迁移到独立的ESP32硬件定时器（2秒间隔）
     // 不再占用loop，避免影响grbl实时性
     
     // 添加心跳调试，确认loop正常运行
     static uint32_t last_heartbeat = 0;
     uint32_t now = millis();
     if (now - last_heartbeat > 5000) {  // 每5秒一次心跳
         ESP_LOGI(TAG, "💓 Main loop heartbeat - millis: %lu", now);
         last_heartbeat = now;
     }
     
     // 简化loop，让grbl正常运行即可
     run_once();
}

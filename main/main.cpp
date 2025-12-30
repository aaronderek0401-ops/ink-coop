#include "src/Grbl.h"
#include "../components/grbl_esp32s3/Grbl_Esp32/src/BL_add/ink_screen/ink_screen.h"
#include "esp_log.h"
#include "esp_spiffs.h"

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
     bool success = switchToScreen(1);  // 显示第一个界面
     
    //  if (success) {
    //      ESP_LOGI(TAG, "✅ 默认界面显示成功！");
    //      ESP_LOGI(TAG, "现在可以使用物理按键测试：");
    //      ESP_LOGI(TAG, "  $inkScreen/Test=1 - 焦点向上（Prev）");
    //      ESP_LOGI(TAG, "  $inkScreen/Test=2 - 确认（Confirm）"); 
    //      ESP_LOGI(TAG, "  $inkScreen/Test=3 - 焦点向下（Next）");
    //      ESP_LOGI(TAG, "  当前焦点矩形索引: %d", getCurrentFocusRect());
    //      ESP_LOGI(TAG, "");
    //      ESP_LOGI(TAG, "💡 切换界面示例：");
    //      ESP_LOGI(TAG, "  switchToScreen(0) - 切换到第一个界面");
    //      ESP_LOGI(TAG, "  switchToScreen(1) - 切换到第二个界面");
    //  } else {
    //      ESP_LOGE(TAG, "❌ 默认界面显示失败");
    //  }
     
    //  ESP_LOGI(TAG, "✅ Setup完成！");
     
    //  // 🧪 测试界面切换功能
    //  if (getCachedScreenCount() > 1) {
    //      ESP_LOGI(TAG, "");
    //      ESP_LOGI(TAG, "========== 开始测试界面切换 ==========");
         
    //      delay(3000);
    //      ESP_LOGI(TAG, "🔄 3秒后切换到界面[1]: %s", getScreenName(1));
    //      switchToScreen(1);
         
    //      delay(3000);
    //      ESP_LOGI(TAG, "🔄 3秒后切换回界面[0]: %s", getScreenName(0));
    //      switchToScreen(0);
         
    //      ESP_LOGI(TAG, "✅ 界面切换测试完成！");
    //      ESP_LOGI(TAG, "==========================================");
    //  }
}

static uint32_t loop_count = 0;
static bool first_loop = true;

void loop() {
     // 处理自动滚动动画（使用更长间隔减少内存压力）
    //  processAutoRollAnimations();
     
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


#include "prompt.h"
#include <string.h>
#include <string.h>
#include "esp_log.h"     // 添加 ESP_LOGI 的头文件
#include "ink_screen.h"  // 提供全局 display 对象

// 兜底声明，防止编译单元未正确感知 ink_screen.cpp 中的全局 display 定义
extern GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display;
#define TAG "prompt.cpp"

uint8_t *showPrompt=nullptr;

void showPromptInfor(uint8_t *tempPrompt,bool isAllRefresh) {
    static uint8_t *lastPrompt = nullptr;
    static char lastPromptContent[256] = {0};
    
    // 检查输入有效性
    if (tempPrompt == nullptr) {
     //   ESP_LOGW("PROMPT", "接收到空指针");
        return;
    }
    
    const char* currentPrompt = (const char*)tempPrompt;
    
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
        display.init(0, true, 2, true);
        display.clearScreen();
        display.display(true);  //局刷之前先对E-Paper进行清屏操作
        display.setPartialWindow(0, 0, display.width(), display.height());
        display.fillRect(30, 10, 340, 30, GxEPD_WHITE);
      //  updateDisplayWithString(30,10, tempPrompt,16,BLACK);
        display.display();
        display.display(true);
        display.powerOff();
        vTaskDelay(1000);
    } else {
        display.fillRect(30, 10, 340, 30, GxEPD_WHITE);
    //    updateDisplayWithString(30,10, tempPrompt,16,BLACK);
    }

}
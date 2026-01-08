#include "onconfirm.h"
#include "word_book.h"
#include "esp_log.h"

// 外部函数声明（来自 ink_screen.cpp）
extern void toggleDecordStatus(int rect_index);
extern void nextDecordPage();

// 示例回调1：打开菜单（示例，实际实现可替换）
void onConfirmOpenMenu(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "示例回调：打开菜单，矩形 %d", idx);
}

// 示例回调2：播放提示音（示例）
void onConfirmPlaySound(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "示例回调：播放提示音，矩形 %d", idx);
}

// 单词本：下一个单词
void onConfirmNextWord(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到下一个单词，矩形 %d", idx);
    if (!g_wordbook_text_initialized) {
        ESP_LOGW("WORDBOOK", "单词本文本缓存未初始化");
        return;
    }
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(g_text_arrays[i].var_name, "$wordbook_idx") == 0) {
            int old_index = g_text_animation_indices[i];
            int new_index = (old_index + 1) % g_text_arrays[i].count;
            g_text_animation_indices[i] = new_index;
            ESP_LOGI("WORDBOOK", "单词本索引已更新: %d -> %d (共%d个单词)", 
                     old_index, new_index, g_text_arrays[i].count);
            ESP_LOGI("WORDBOOK", "  当前单词: %s", getWordBookWord(new_index));
            ESP_LOGI("WORDBOOK", "  音标: %s", getWordBookPhonetic(new_index));
            ESP_LOGI("WORDBOOK", "  翻译: %s", getWordBookTranslation(new_index));
            
            // 刷新屏幕显示新单词
            redrawJsonLayout();
            return;
        }
    }
    ESP_LOGW("WORDBOOK", "未找到$wordbook_idx变量");
}

// 界面切换：切换到第一个界面（layout_main.json）
void onConfirmSwitchToLayoutMain(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面0 (layout_main.json)，矩形 %d", idx);
    display.setFullWindow();

    if (switchToScreen(0)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面0: %s", getScreenName(0));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面0失败");
    }
}

// 界面切换：切换到第二个界面（layout_wordbook.json）
void onConfirmSwitchToLayoutWordbook(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面1 (layout_wordbook.json)，矩形 %d", idx);
    display.setFullWindow();

    if (switchToScreen(1)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面1: %s", getScreenName(1));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面1失败");
    }
}

// 界面切换：切换到第三个界面（layout_clock.json）
void onConfirmSwitchToLayoutClock(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面2 (layout_clock.json)，矩形 %d", idx);
    display.setFullWindow();

    if (switchToScreen(2)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面2: %s", getScreenName(2));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面2失败");
    }
}

// 界面切换：切换到番茄钟设置界面（layout_clock_set.json）
void onConfirmSwitchToLayoutClockSet(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面番茄钟设置 (layout_clock_set.json)，矩形 %d", idx);
    display.setPartialWindow(200, 60, setInkScreenSize.screenWidth - 200, setInkScreenSize.screenHeigt - 60);

    if (switchToScreen(3)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面3: %s", getScreenName(3));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面3失败");
    }
}

// 界面切换：切换到第四个界面（layout_decord.json）
void onConfirmSwitchToLayoutDecord(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面4 (layout_decord.json)，矩形 %d", idx);
    display.setFullWindow();

    if (switchToScreen(4)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面4: %s", getScreenName(4));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面4失败");
    }
}

// 番茄钟回调：开始/暂停
void onConfirmPomodoroStartPause(RectInfo* rect, int idx) {
    ESP_LOGI("POMODORO", "番茄钟开始/暂停按钮被按下");
    pomodoroStartPause();
}

// 番茄钟回调：重置
void onConfirmPomodoroReset(RectInfo* rect, int idx) {
    ESP_LOGI("POMODORO", "番茄钟重置按钮被按下");
    pomodoroReset();
}

// 番茄钟回调：设置
void onConfirmPomodoroSettings(RectInfo* rect, int idx) {
    ESP_LOGI("POMODORO", "番茄钟设置按钮被按下");
    pomodoroSettings();
}

// 番茄钟回调：切换预设时长（循环 3/5/10/25/50 分钟）
void onConfirmPomodoroChangeDuration(RectInfo* rect, int idx) {
    ESP_LOGI("POMODORO", "切换番茄钟时长，矩形 %d", idx);
    // 先切换到番茄钟界面（如果需要），再设置时长以避免 initPomodoro 覆盖设置
    display.setFullWindow();
    switchToScreen(2); // 刷新当前界面显示

    // 直接将番茄钟时长设置为600秒（10分钟）
    setPomodoroDurationSeconds(600);
    ESP_LOGI("POMODORO", "已设置番茄钟时长为 %d 分钟", 600 / 60);

    // 如有使用到动态文本索引（显示时长），重置对应索引以触发刷新
    for (int i = 0; i < g_text_arrays_count; i++) {
        if (strcmp(g_text_arrays[i].var_name, "$pomodoro_time_idx") == 0) {
            g_text_animation_indices[i] = 0;
            break;
        }
    }
}

// 打卡回调：切换指定矩形框的打卡状态
void onConfirmToggleDecordStatus(RectInfo* rect, int idx) {
    ESP_LOGI("DECORD", "切换矩形%d的打卡状态", idx);
    
    if (!rect) {
        ESP_LOGW("DECORD", "矩形指针为空");
        return;
    }
    
    // 从矩形的icon_roll中提取正确的索引号
    // 查找包含"isfinished_idx_"的变量名，从中提取数字后缀
    int decord_idx = -1;
    
    for (int i = 0; i < rect->icon_roll_count; i++) {
        const char* var_name = rect->icon_rolls[i].idx;
        ESP_LOGI("DECORD", "检查icon_roll[%d]: idx='%s'", i, var_name);
        
        // 查找"isfinished_idx_"字符串
        const char* pos = strstr(var_name, "isfinished_idx_");
        if (pos) {
            // 跳过"isfinished_idx_"部分，提取数字
            pos += strlen("isfinished_idx_");
            decord_idx = atoi(pos);
            ESP_LOGI("DECORD", "✅ 从变量名'%s'中提取到索引: %d", var_name, decord_idx);
            break;
        }
    }
    
    if (decord_idx < 0 || decord_idx > 5) {
        ESP_LOGW("DECORD", "无效的打卡索引: %d (从矩形%d提取)", decord_idx, idx);
        return;
    }
    
    // 调用切换函数
    toggleDecordStatus(decord_idx);
    ESP_LOGI("DECORD", "矩形%d（打卡索引%d）打卡状态已切换", idx, decord_idx);
}

// 单词本：点击任意选项（统一回调）
void onConfirmWordOption(RectInfo* rect, int idx) {
    ESP_LOGI("WORDBOOK", "点击单词选项，矩形 %d", idx);
    
    if (!rect) {
        ESP_LOGE("WORDBOOK", "矩形指针为空");
        return;
    }
    
    // 通过检查 text_roll 变量名来判断是哪个选项
    int option_num = 0;
    
    if (rect->text_roll_count > 0 && rect->text_rolls[0].text_arr[0] != '\0') {
        const char* text_var = rect->text_rolls[0].text_arr;
        ESP_LOGI("WORDBOOK", "矩形文本变量: %s", text_var);
        
        // 匹配 $wordbook_option_N 或 wordbook_option_N 格式
        if (strstr(text_var, "wordbook_option") != NULL) {
            // 提取选项编号（假设格式为 wordbook_option_1, wordbook_option_2, ...）
            const char* num_ptr = strstr(text_var, "option");
            if (num_ptr) {
                num_ptr += 6; // 跳过 "option"
                // 跳过可能的下划线
                if (*num_ptr == '_') {
                    num_ptr++;
                }
                option_num = atoi(num_ptr);
            }
        }
    }
    
    if (option_num <= 0) {
        ESP_LOGE("WORDBOOK", "无法确定选项编号");
        return;
    }
    
    ESP_LOGI("WORDBOOK", "识别为选项%d", option_num);
    
    int word_index = getCurrentWordIndex();
    bool is_correct = checkWordAnswer(word_index, option_num);
    
    if (is_correct) {
        ESP_LOGI("WORDBOOK", "✓ 答案正确！切换到下一个单词");
        if (moveToNextWord()) {
            // 成功移动到下一个单词，更新显示
            for (int i = 0; i < g_text_arrays_count; i++) {
                if (strcmp(g_text_arrays[i].var_name, "$wordbook_idx") == 0) {
                    g_text_animation_indices[i] = getCurrentWordIndex();
                    break;
                }
            }
            redrawJsonLayout();
        } else {
            ESP_LOGW("WORDBOOK", "已经是最后一个单词了");
        }
    } else {
        ESP_LOGI("WORDBOOK", "✗ 答案错误！在旁边显示×标记");
        
        // 在选项矩形右侧显示×标记
        if (rect) {
            ESP_LOGI("WORDBOOK", "矩形位置: (%d, %d, %d, %d)", 
                     rect->x, rect->y, rect->width, rect->height);
            
            // 计算×标记的位置（矩形右侧，留一些间距）
            int16_t mark_x = rect->x + rect->width - 10;  // 右侧10像素间距
            int16_t mark_y = rect->y + rect->height / 2;  // 垂直居中
            
            // 设置部分刷新窗口（只刷新×标记区域）
            uint16_t mark_width = 30;   // ×标记宽度
            uint16_t mark_height = 30;  // ×标记高度
            display.setPartialWindow(mark_x - 5, mark_y - mark_height/2 - 5, 
                                    mark_width + 10, mark_height + 10);
            
            // 开始绘制
            display.firstPage();
            do {
                display.fillScreen(GxEPD_WHITE);  // 白色背景
                
                // 绘制×标记（使用两条对角线）
                int16_t size = 20;  // ×的大小
                // 左上到右下的线
                display.drawLine(mark_x - size/2, mark_y - size/2, 
                               mark_x + size/2, mark_y + size/2, GxEPD_BLACK);
                // 右上到左下的线
                display.drawLine(mark_x + size/2, mark_y - size/2, 
                               mark_x - size/2, mark_y + size/2, GxEPD_BLACK);
                
                // 加粗效果（绘制多次）
                display.drawLine(mark_x - size/2 + 1, mark_y - size/2, 
                               mark_x + size/2 + 1, mark_y + size/2, GxEPD_BLACK);
                display.drawLine(mark_x + size/2 + 1, mark_y - size/2, 
                               mark_x - size/2 + 1, mark_y + size/2, GxEPD_BLACK);
                
            } while (display.nextPage());
            
            ESP_LOGI("WORDBOOK", "已在位置 (%d, %d) 显示×标记", mark_x, mark_y);
        }
        
        // 不需要重新绘制整个布局
        display.setFullWindow();

        redrawJsonLayout();
    }
}

// 动作注册表
ActionEntry g_action_registry[] = {
    {"open_menu", "打开菜单", onConfirmOpenMenu},
    {"play_sound", "播放提示音", onConfirmPlaySound},
    {"next_word", "下一个单词", onConfirmNextWord},
    {"word_option", "单词本选项", onConfirmWordOption},
    {"switch_to_layout_main", "切换到界面0", onConfirmSwitchToLayoutMain},
    {"switch_to_layout_wordbook", "切换到界面1", onConfirmSwitchToLayoutWordbook},
    {"switch_to_layout_clock", "切换到界面2", onConfirmSwitchToLayoutClock},
    {"switch_to_layout_decord", "切换到界面4", onConfirmSwitchToLayoutDecord},
    {"switch_to_layout_clock_set", "切换到番茄钟设置界面", onConfirmSwitchToLayoutClockSet},
    {"pomodoro_start_pause", "番茄钟开始/暂停", onConfirmPomodoroStartPause},
    {"pomodoro_reset", "番茄钟重置", onConfirmPomodoroReset},
    {"pomodoro_settings", "番茄钟设置", onConfirmPomodoroSettings},
    {"pomodoro_change_duration", "切换番茄钟时长", onConfirmPomodoroChangeDuration},
    {"toggle_decord_status", "切换打卡状态", onConfirmToggleDecordStatus}
};
int g_action_registry_count = sizeof(g_action_registry) / sizeof(g_action_registry[0]);

// 通过动作ID查找函数指针（返回NULL表示未找到）
OnConfirmFn find_action_by_id(const char* id) {
    if (!id) return NULL;
    for (int i = 0; i < g_action_registry_count; i++) {
        if (g_action_registry[i].id && strcmp(g_action_registry[i].id, id) == 0) {
            return g_action_registry[i].fn;
        }
    }
    return NULL;
}

#include "onconfirm.h"
#include "esp_log.h"

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
            return;
        }
    }
    ESP_LOGW("WORDBOOK", "未找到$wordbook_idx变量");
}

// 界面切换：切换到第一个界面（layout.json）
void onConfirmSwitchToLayout0(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面0 (layout.json)，矩形 %d", idx);
    if (switchToScreen(0)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面0: %s", getScreenName(0));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面0失败");
    }
}

// 界面切换：切换到第二个界面（layout_1.json）
void onConfirmSwitchToLayout1(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面1 (layout_1.json)，矩形 %d", idx);
    if (switchToScreen(1)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面1: %s", getScreenName(1));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面1失败");
    }
}

// 界面切换：切换到番茄钟设置界面（layout_clock_set.json）
void onConfirmSwitchToLayoutClockSet(RectInfo* rect, int idx) {
    ESP_LOGI("ONCONFIRM", "切换到界面番茄钟设置 (layout_clock_set.json)，矩形 %d", idx);
    if (switchToScreen(3)) {
        ESP_LOGI("SCREEN_SWITCH", "✅ 成功切换到界面3: %s", getScreenName(3));
    } else {
        ESP_LOGE("SCREEN_SWITCH", "❌ 切换到界面3失败");
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

// 动作注册表
ActionEntry g_action_registry[] = {
    {"open_menu", "打开菜单", onConfirmOpenMenu},
    {"play_sound", "播放提示音", onConfirmPlaySound},
    {"next_word", "下一个单词", onConfirmNextWord},
    {"switch_to_layout_0", "切换到界面0", onConfirmSwitchToLayout0},
    {"switch_to_layout_1", "切换到界面1", onConfirmSwitchToLayout1},
    {"switch_to_layout_clock_set", "切换到番茄钟设置界面", onConfirmSwitchToLayoutClockSet},
    {"pomodoro_start_pause", "番茄钟开始/暂停", onConfirmPomodoroStartPause},
    {"pomodoro_reset", "番茄钟重置", onConfirmPomodoroReset},
    {"pomodoro_settings", "番茄钟设置", onConfirmPomodoroSettings}
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

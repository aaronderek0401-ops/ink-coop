#pragma once
#include "ink_screen.h"

// onConfirm 回调函数声明
void onConfirmOpenMenu(RectInfo* rect, int idx);
void onConfirmPlaySound(RectInfo* rect, int idx);
void onConfirmNextWord(RectInfo* rect, int idx);
void onConfirmSwitchToLayout0(RectInfo* rect, int idx);
void onConfirmSwitchToLayout1(RectInfo* rect, int idx);
void onConfirmSwitchToLayoutClockSet(RectInfo* rect, int idx);
void onConfirmPomodoroStartPause(RectInfo* rect, int idx);
void onConfirmPomodoroReset(RectInfo* rect, int idx);
void onConfirmPomodoroSettings(RectInfo* rect, int idx);

// 动作注册表（定义在 onconfirm.cpp 中）
extern ActionEntry g_action_registry[];
extern int g_action_registry_count;

// 根据动作ID查找回调
OnConfirmFn find_action_by_id(const char* id);

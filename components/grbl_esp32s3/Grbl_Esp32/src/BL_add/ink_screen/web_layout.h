#pragma once

bool loadVocabLayoutFromConfig();

// 焦点矩形配置相关（添加界面类型参数以区分主界面和单词界面）
bool loadAndApplySubArrayConfig(const char* screen_type = "vocab");  // 加载并应用子数组配置到焦点系统
extern bool loadFocusableRectsFromConfig(const char* screen_type);
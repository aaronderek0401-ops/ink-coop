#pragma once

#include <Arduino.h>
#include <SD.h>
#include <string>

// 单词条目结构
struct WordEntry {
    String word;          // 单词
    String phonetic;      // 音标
    String definition;    // 英文释义
    String translation;   // 中文翻译
    String pos;           // 词性
};

// 全局变量声明
extern WordEntry entry;
extern WordEntry sleep_mode_entry;
extern bool has_sleep_data;

// 函数声明
void safeDisplayWordEntry(const WordEntry& entry, uint16_t x, uint16_t y);
int countLines(File &file);
WordEntry readLineAtPosition(File &file, int lineNumber);
void parseCSVLine(String line, WordEntry &entry);
void assignField(int fieldCount, String &field, WordEntry &entry);
void printWordEntry(WordEntry &entry, int lineNumber);
void readAndPrintWords();
void readAndPrintRandomWord();


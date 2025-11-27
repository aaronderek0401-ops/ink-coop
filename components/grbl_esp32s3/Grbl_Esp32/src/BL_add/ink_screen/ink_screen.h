#pragma once
#include "../../Grbl.h"
#include <FS.h>
void ink_screen_init();
// CSV文件结构体
struct WordEntry {
  String word;
  String phonetic;
  String definition;
  String translation;
  String pos;
};
void parseCSVLine(String line, WordEntry &entry);
int countLines(File &file);
void printWordEntry(WordEntry &entry, int lineNumber);
void assignField(int fieldCount, String &field, WordEntry &entry);
extern uint8_t inkScreenTestFlag;
extern uint8_t inkScreenTestFlagTwo;
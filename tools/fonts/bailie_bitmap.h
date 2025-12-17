// 简单的中文字符显示 - 只显示 "爆裂"
// 使用直接位图绘制，不依赖 GFX 字体系统

#ifndef BAILIE_BITMAP_H
#define BAILIE_BITMAP_H

#include <Arduino.h>

// "爆" 字位图 (16x16)
const uint8_t BITMAP_BAO[] PROGMEM = {
  0x11, 0xFC, 0x11, 0x04, 0x11, 0xFC, 0x55, 0x04, 
  0x59, 0xFC, 0x50, 0x88, 0x53, 0xFE, 0x50, 0x88, 
  0x13, 0xFE, 0x1D, 0xAC, 0x33, 0x26, 0x24, 0xB8, 
  0x61, 0xE8, 0x43, 0x24, 0x00, 0x40, 0x00, 0x00
};

// "裂" 字位图 (16x16, 底部留空)
const uint8_t BITMAP_LIE[] PROGMEM = {
  0x00, 0x04, 0x7F, 0xA4, 0x08, 0x24, 0x1F, 0xA4, 
  0x39, 0x24, 0x0E, 0x24, 0x0C, 0x04, 0x31, 0x98, 
  0x7F, 0xFE, 0x03, 0x00, 0x06, 0xC8, 0x3C, 0x70, 
  0x65, 0x98, 0x07, 0x0E, 0x04, 0x00, 0x00, 0x00
};

// 绘制 16x16 位图（使用模板支持任何显示类）
template<typename T>
inline void drawBitmap16x16(T& display, int16_t x, int16_t y, const uint8_t* bitmap, uint16_t scale = 1) {
    for (int16_t j = 0; j < 16; j++) {
        for (int16_t i = 0; i < 16; i++) {
            int byteIndex = (j * 16 + i) / 8;
            int bitIndex = 7 - ((j * 16 + i) % 8);
            
            if (pgm_read_byte(&bitmap[byteIndex]) & (1 << bitIndex)) {
                if (scale == 1) {
                    display.drawPixel(x + i, y + j, GxEPD_BLACK);
                } else {
                    // 绘制缩放后的像素块
                    display.fillRect(x + i * scale, y + j * scale, scale, scale, GxEPD_BLACK);
                }
            }
        }
    }
}

#endif // BAILIE_BITMAP_H

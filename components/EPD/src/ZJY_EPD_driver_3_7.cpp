#include "ZJY_EPD_driver_3_7.h"
#include "EPD.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <algorithm>
#include "esp_log.h"

static const char* TAG = "EPDDriver_3_7";

EPDDriver_3_7::EPDDriver_3_7(int width, int height)
    : _width(width), _height(height)
{
    _widthBytes = (_width % 8 == 0) ? (_width / 8) : (_width / 8 + 1);
    _oldImage.assign(_widthBytes * _height, 0xFF);
    _spiHandle = EPD_Handle; // use existing global handle; user must call SPI init before GPIOInit
}

EPDDriver_3_7::~EPDDriver_3_7() {
}

void EPDDriver_3_7::READBUSY() {
    // Poll the busy pin
    while (true) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (EPD_ReadBUSY == 1) break;
    }
}

void EPDDriver_3_7::HW_RESET() {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    EPD_RES_Clr();
    vTaskDelay(20 / portTICK_PERIOD_MS);
    EPD_RES_Set();
    vTaskDelay(20 / portTICK_PERIOD_MS);
    READBUSY();
}

void EPDDriver_3_7::GPIOInit() {
    // reuse existing function in C if desired; keep minimal here
    EPD_GPIOInit();
}

void EPDDriver_3_7::Init() {
    HW_RESET();
    READBUSY();
    EPD_WR_REG(0x00);
    EPD_WR_DATA8(EPD_Handle, 0x1B, 1);
}

void EPDDriver_3_7::FastInit() {
    HW_RESET();
    READBUSY();
    EPD_WR_REG(0x00);
    EPD_WR_DATA8(EPD_Handle, 0x1B, 1);
    EPD_WR_REG(0xE0);
    EPD_WR_DATA8(EPD_Handle, 0x02, 1);
    EPD_WR_REG(0xE5);
    EPD_WR_DATA8(EPD_Handle, 0x5F, 1);
}

void EPDDriver_3_7::PartInit() {
    HW_RESET();
    READBUSY();
    EPD_WR_REG(0x00);
    EPD_WR_DATA8(EPD_Handle, 0x1B, 1);
    EPD_WR_REG(0xE0);
    EPD_WR_DATA8(EPD_Handle, 0x02, 1);
    EPD_WR_REG(0xE5);
    EPD_WR_DATA8(EPD_Handle, 0x6E, 1);
}

void EPDDriver_3_7::Update() {
    EPD_WR_REG(0x04); // Power ON
    READBUSY();
    EPD_WR_REG(0x12);
    READBUSY();
}

void EPDDriver_3_7::DeepSleep() {
    EPD_WR_REG(0x02); // Power OFF
    READBUSY();
    EPD_WR_REG(0x07);
    EPD_WR_DATA8(EPD_Handle, 0xA5, 1);
}

void EPDDriver_3_7::DisplayClear() {
    size_t Width = _widthBytes;
    size_t Height = _height;

    EPD_WR_REG(0x10);
    for (size_t j = 0; j < Height; j++) {
        for (size_t i = 0; i < Width; i++) {
            EPD_WR_DATA8(EPD_Handle, _oldImage[i + j * Width], 1);
        }
    }

    EPD_WR_REG(0x13);
    for (size_t j = 0; j < Height; j++) {
        for (size_t i = 0; i < Width; i++) {
            EPD_WR_DATA8(EPD_Handle, 0xFF, 1);
            _oldImage[i + j * Width] = 0xFF;
        }
    }
}

void EPDDriver_3_7::Display(const uint8_t* image) {
    size_t Width = _widthBytes;
    size_t Height = _height;

    EPD_WR_REG(0x10);
    for (size_t j = 0; j < Height; j++) {
        for (size_t i = 0; i < Width; i++) {
            EPD_WR_DATA8(EPD_Handle, _oldImage[i + j * Width], 1);
        }
    }

    EPD_WR_REG(0x13);
    for (size_t j = 0; j < Height; j++) {
        for (size_t i = 0; i < Width; i++) {
            uint8_t v = image[i + j * Width];
            EPD_WR_DATA8(EPD_Handle, v, 1);
            _oldImage[i + j * Width] = v;
        }
    }
}

EPDDriver_3_7 myEPDDriver_3_7;
#include "ZJY_EPD_driver_1_5_4.h"
#include "EPD.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SPI_Init.h"
#include "esp_log.h"
#include <cstring>
#include "SPI_Init.h"
#include "esp_task_wdt.h"
static const char* TAG = "EPDDriver_1_5_4";

EPDDriver_1_5_4::EPDDriver_1_5_4(int width, int height)
    : _width(width), _height(height) {
    _widthBytes = (_width % 8 == 0) ? (_width / 8) : (_width / 8 + 1);
    _oldImage.assign(_widthBytes * _height, 0xFF);
    // Do not capture EPD_Handle at static-construction time: it may be
    // initialized later by EPD_GPIOInit(). Initialize to nullptr and set
    // the real handle in GPIOInit().
    _spiHandle = nullptr;
}

EPDDriver_1_5_4::~EPDDriver_1_5_4() {}

void EPDDriver_1_5_4::READBUSY() {
    ESP_LOGI(TAG, "Waiting for BUSY...");
    int timeout = 0;
    // SSD1680: BUSY=0 means busy, BUSY=1 means idle
    while (EPD_ReadBUSY == 0) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        timeout++;
        if (timeout > 300) { // ~3s
            ESP_LOGW(TAG, "BUSY timeout");
            break;
        }
    }
    ESP_LOGI(TAG, "BUSY released, timeout_count=%d", timeout);
}

void EPDDriver_1_5_4::HW_RESET() {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    EPD_RES_Clr();
    vTaskDelay(20 / portTICK_PERIOD_MS);
    EPD_RES_Set();
    vTaskDelay(20 / portTICK_PERIOD_MS);
    READBUSY();
}

void EPDDriver_1_5_4::GPIOInit() {
    ESP_LOGI(TAG, "GPIOInit");
    // Initialize the low-level SPI/GPIOs first, then refresh our cached handle.
    EPD_GPIOInit();
    _spiHandle = EPD_Handle;
}

void EPDDriver_1_5_4::Init() {
    ESP_LOGI(TAG, "Init start");
    HW_RESET();
    READBUSY();
    EPD_WR_REG(0x12); // SWRESET
    READBUSY();
    ESP_LOGI(TAG, "Init done");
}

void EPDDriver_1_5_4::Update() {
    ESP_LOGI(TAG, "Update start");
    EPD_WR_REG(0x22);
    EPD_WR_DATA8(_spiHandle, 0xF7, 1);
    EPD_WR_REG(0x20);
    READBUSY();
    ESP_LOGI(TAG, "Update done");
}

void EPDDriver_1_5_4::PartInit() {
    ESP_LOGI(TAG, "PartInit start");
    HW_RESET();
    READBUSY();

    EPD_WR_REG(0x12); // SW Reset
    READBUSY();

    EPD_WR_REG(0x01); // Driver output control
    EPD_WR_DATA8(_spiHandle, (uint8_t)((_height - 1) & 0xFF), 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)(((_height - 1) >> 8) & 0xFF), 1);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);

    EPD_WR_REG(0x11); // Data entry mode
    EPD_WR_DATA8(_spiHandle, 0x03, 1);

    EPD_WR_REG(0x44); // Set RAM X address
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)((_width / 8) - 1), 1);

    EPD_WR_REG(0x45); // Set RAM Y address
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)((_height - 1) & 0xFF), 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)(((_height - 1) >> 8) & 0xFF), 1);

    EPD_WR_REG(0x3C); // Border waveform
    EPD_WR_DATA8(_spiHandle, 0x05, 1);

    EPD_WR_REG(0x18); // Temperature sensor
    EPD_WR_DATA8(_spiHandle, 0x80, 1);

    EPD_WR_REG(0x4E); // Set RAM X counter
    EPD_WR_DATA8(_spiHandle, 0x00, 1);

    EPD_WR_REG(0x4F); // Set RAM Y counter
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);

    READBUSY();
    ESP_LOGI(TAG, "PartInit done");
}

void EPDDriver_1_5_4::FastInit() {
    ESP_LOGI(TAG, "FastInit start");
    HW_RESET();
    READBUSY();

    EPD_WR_REG(0x12); // SW Reset
    READBUSY();

    EPD_WR_REG(0x01); // Driver output control
    EPD_WR_DATA8(_spiHandle, (uint8_t)((_height - 1) & 0xFF), 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)(((_height - 1) >> 8) & 0xFF), 1);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);

    EPD_WR_REG(0x11); // Data entry mode
    EPD_WR_DATA8(_spiHandle, 0x03, 1);

    EPD_WR_REG(0x44); // Set RAM X address
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)((_width / 8) - 1), 1);

    EPD_WR_REG(0x45); // Set RAM Y address
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)((_height - 1) & 0xFF), 1);
    EPD_WR_DATA8(_spiHandle, (uint8_t)(((_height - 1) >> 8) & 0xFF), 1);

    EPD_WR_REG(0x3C); // Border waveform
    EPD_WR_DATA8(_spiHandle, 0x05, 1);

    EPD_WR_REG(0x18); // Temperature sensor
    EPD_WR_DATA8(_spiHandle, 0x80, 1);

    EPD_WR_REG(0x4E); // Set RAM X counter
    EPD_WR_DATA8(_spiHandle, 0x00, 1);

    EPD_WR_REG(0x4F); // Set RAM Y counter
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);

    READBUSY();
    ESP_LOGI(TAG, "FastInit done");
}

void EPDDriver_1_5_4::DeepSleep() {
    ESP_LOGI(TAG, "DeepSleep");
    EPD_WR_REG(0x10);
    EPD_WR_DATA8(_spiHandle, 0x01, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void EPDDriver_1_5_4::DisplayClear() {
    ESP_LOGI(TAG, "DisplayClear start");
    size_t buf_size = _width * _height / 8;
    EPD_WR_REG(0x3C);
    EPD_WR_DATA8(_spiHandle, 0x05, 1);
    EPD_WR_REG(0x24);
    for (size_t i = 0; i < buf_size; ++i) {
        EPD_WR_DATA8(_spiHandle, 0xFF, 1);
    }
    READBUSY();
    EPD_WR_REG(0x26);
    for (size_t i = 0; i < buf_size; ++i) {
        EPD_WR_DATA8(_spiHandle, 0x00, 1);
    }
    ESP_LOGI(TAG, "DisplayClear done");
}

void EPDDriver_1_5_4::Display(const uint8_t* image) {
    ESP_LOGI(TAG, "Display start");
    size_t Width = _widthBytes;
    size_t Height = _height;

    // Set RAM X/Y counter before writing
    EPD_WR_REG(0x4E);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_REG(0x4F);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);
    EPD_WR_DATA8(_spiHandle, 0x00, 1);

    EPD_WR_REG(0x24); // Write RAM (Black/White)
    for (size_t j = 0; j < Height; ++j) {
        for (size_t i = 0; i < Width; ++i) {
            EPD_WR_DATA8(_spiHandle, image[i + j * Width], 1);
        }
    }
    ESP_LOGI(TAG, "Display done, %d bytes sent", Width * Height);
}
EPDDriver_1_5_4 myEPDDriver_1_5_4;



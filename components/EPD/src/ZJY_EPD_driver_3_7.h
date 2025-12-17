#pragma once

#include "EPD.h"
#include "IEPDDriver.h"
#include <vector>
#include <cstdint>
#include "SPI_Init.h"

class EPDDriver_3_7 : public IEPDDriver {
public:
    // width, height in pixels (logical EPD orientation)
    explicit EPDDriver_3_7(int width = 240, int height = 416);
    ~EPDDriver_3_7() override;

    // Hardware / lifecycle
    void GPIOInit() override;
    void Init() override;
    void FastInit() override;
    void PartInit() override;
    void Update() override;
    void DeepSleep() override;

    // Drawing
    void DisplayClear() override;
    void Display(const uint8_t* image) override;

    // Accessors
    int width() const override { return _width; }
    int height() const override { return _height; }
    uint8_t* getImageBuffer() override { return ImageBW; }
    uint16_t rotated() override { return 0; }
private:
    int _width;
    int _height;
    int _widthBytes;
    uint8_t ImageBW[240*416/8];
    std::vector<uint8_t> _oldImage;
    spi_device_handle_t _spiHandle;

    // internal helpers (wrappers around existing macros/functions)
    void HW_RESET();
    void READBUSY();
};

extern EPDDriver_3_7 myEPDDriver_3_7;

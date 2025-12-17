#pragma once

#include "EPD.h"
#include "IEPDDriver.h"
#include "SPI_Init.h"
#include <vector>
#include <cstdint>

// C++ wrapper driver for the 1.5" 152x152 SSD1680-like panel.
// Implements IEPDDriver and preserves the original register sequences.
class EPDDriver_1_5_4 : public IEPDDriver {
public:
	explicit EPDDriver_1_5_4(int width = 152, int height = 152);
	~EPDDriver_1_5_4() override;

	// lifecycle
	void GPIOInit() override;
	void Init() override;
	void FastInit() override;
	void PartInit() override;
	void Update() override;
	void DeepSleep() override;

	// drawing
	void DisplayClear() override;
	void Display(const uint8_t* image) override;

	int width() const override { return _width; }
	int height() const override { return _height; }
    uint8_t* getImageBuffer() override { return ImageBW; }
    uint16_t rotated() override { return 0; }
private:
	int _width;
	int _height;
	int _widthBytes;
    uint8_t ImageBW[152 * 152 / 8];
	std::vector<uint8_t> _oldImage;
	spi_device_handle_t _spiHandle;

	// low level helpers (ported from original C functions)
	void READBUSY();
	void HW_RESET();
};
extern EPDDriver_1_5_4 myEPDDriver_1_5_4;





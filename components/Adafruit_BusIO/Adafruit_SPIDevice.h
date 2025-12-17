#ifndef ADAFRUIT_SPI_DEVICE_H
#define ADAFRUIT_SPI_DEVICE_H

#include <Arduino.h>
#include <SPI.h>

// Compatibility: some Adafruit code uses SPI_BITORDER_MSBFIRST / _LSBFIRST
#ifndef SPI_BITORDER_MSBFIRST
#define SPI_BITORDER_MSBFIRST SPI_MSBFIRST
#define SPI_BITORDER_LSBFIRST SPI_LSBFIRST
#endif

class Adafruit_SPIDevice {
public:
  // Hardware SPI constructor
  Adafruit_SPIDevice(int8_t csPin, SPIClass *spi = &SPI, uint32_t bitrate = 8000000);
  // Soft SPI constructor: cs, sck, miso, mosi, bitrate
  Adafruit_SPIDevice(int8_t csPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin,
                    uint32_t bitrate = 8000000);
  // Alternative constructor used by some Adafruit code: cs, bitrate, bitorder, mode, spi
  Adafruit_SPIDevice(int8_t csPin, uint32_t bitrate, int bitOrder, int mode,
                    SPIClass *spi = &SPI);

  bool begin();
  bool write(const uint8_t *data, size_t len);
  bool read(uint8_t *buffer, size_t len);
  void beginTransaction();
  void endTransaction();
  int8_t csPin() const { return _cs; }

private:
  int8_t _cs;
  // For hardware SPI
  SPIClass *_spi;
  SPISettings _settings;
  // For software SPI
  bool _isSoft = false;
  int8_t _sck, _mosi, _miso;
  uint32_t _bitrate = 8000000;
};

#endif // ADAFRUIT_SPI_DEVICE_H

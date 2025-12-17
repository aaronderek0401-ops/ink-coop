#include "Adafruit_SPIDevice.h"

// Hardware SPI constructor
Adafruit_SPIDevice::Adafruit_SPIDevice(int8_t csPin, SPIClass *spi, uint32_t bitrate)
    : _cs(csPin), _spi(spi), _settings(bitrate, MSBFIRST, SPI_MODE0) {
  _isSoft = false;
  _bitrate = bitrate;
}

// Soft SPI constructor (bitbang)
Adafruit_SPIDevice::Adafruit_SPIDevice(int8_t csPin, int8_t sckPin, int8_t misoPin,
                                     int8_t mosiPin, uint32_t bitrate)
    : _cs(csPin), _spi(nullptr), _settings(1000000, MSBFIRST, SPI_MODE0) {
  _isSoft = true;
  _sck = sckPin;
  _miso = misoPin;
  _mosi = mosiPin;
  _bitrate = bitrate;
}

// Alternative constructor (cs, bitrate, bitOrder, mode, spi)
Adafruit_SPIDevice::Adafruit_SPIDevice(int8_t csPin, uint32_t bitrate, int bitOrder, int mode,
                                     SPIClass *spi)
    : _cs(csPin), _spi(spi), _settings(bitrate, bitOrder == SPI_BITORDER_MSBFIRST ? MSBFIRST : LSBFIRST, mode) {
  _isSoft = false;
  _bitrate = bitrate;
}

bool Adafruit_SPIDevice::begin() {
  if (_cs >= 0) {
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
  }
  if (_isSoft) {
    pinMode(_sck, OUTPUT);
    pinMode(_mosi, OUTPUT);
    if (_miso >= 0) pinMode(_miso, INPUT);
  } else if (_spi) {
    _spi->begin();
  }
  return true;
}

void Adafruit_SPIDevice::beginTransaction() {
  if (!_isSoft && _spi) {
    _spi->beginTransaction(_settings);
    if (_cs >= 0) digitalWrite(_cs, LOW);
  } else {
    if (_cs >= 0) digitalWrite(_cs, LOW);
  }
}

void Adafruit_SPIDevice::endTransaction() {
  if (!_isSoft && _spi) {
    if (_cs >= 0) digitalWrite(_cs, HIGH);
    _spi->endTransaction();
  } else {
    if (_cs >= 0) digitalWrite(_cs, HIGH);
  }
}

bool Adafruit_SPIDevice::write(const uint8_t *data, size_t len) {
  if (_isSoft) {
    beginTransaction();
    // bit-bang MSB-first
    for (size_t i = 0; i < len; ++i) {
      uint8_t b = data[i];
      for (int k = 7; k >= 0; --k) {
        digitalWrite(_mosi, (b >> k) & 1);
        digitalWrite(_sck, HIGH);
        // crude timing
        digitalWrite(_sck, LOW);
      }
    }
    endTransaction();
    return true;
  }
  if (!_spi) return false;
  beginTransaction();
  for (size_t i = 0; i < len; ++i) {
    _spi->transfer(data[i]);
  }
  endTransaction();
  return true;
}

bool Adafruit_SPIDevice::read(uint8_t *buffer, size_t len) {
  if (_isSoft) {
    beginTransaction();
    for (size_t i = 0; i < len; ++i) {
      uint8_t val = 0;
      for (int k = 7; k >= 0; --k) {
        digitalWrite(_sck, HIGH);
        val |= (digitalRead(_miso) & 1) << k;
        digitalWrite(_sck, LOW);
      }
      buffer[i] = val;
    }
    endTransaction();
    return true;
  }
  if (!_spi) return false;
  beginTransaction();
  for (size_t i = 0; i < len; ++i) {
    buffer[i] = _spi->transfer(0x00);
  }
  endTransaction();
  return true;
}

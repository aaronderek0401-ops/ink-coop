#include "Adafruit_I2CDevice.h"

Adafruit_I2CDevice::Adafruit_I2CDevice(uint8_t addr, TwoWire *theWire)
    : _addr(addr), _wire(theWire) {}

bool Adafruit_I2CDevice::begin() {
  if (!_wire) return false;
  _wire->begin();
  return true;
}

bool Adafruit_I2CDevice::write(const uint8_t *data, size_t len) {
  return write(data, len, true, nullptr, 0);
}

bool Adafruit_I2CDevice::write(uint8_t b) {
  return write(&b, 1);
}

bool Adafruit_I2CDevice::read(uint8_t *buffer, size_t len) {
  if (!_wire) return false;
  size_t got = _wire->requestFrom((int)_addr, (int)len);
  if (got == 0) return false;
  size_t i = 0;
  while (_wire->available() && i < len) {
    buffer[i++] = (uint8_t)_wire->read();
  }
  return (i == len);
}

bool Adafruit_I2CDevice::write(const uint8_t *data, size_t len, bool stop,
                               const uint8_t *prefix, size_t prefix_len) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  // write prefix bytes first if provided
  if (prefix && prefix_len) {
    _wire->write(prefix, (size_t)prefix_len);
  }
  size_t written = 0;
  if (data && len) written = _wire->write(data, (size_t)len);
  int ok = _wire->endTransmission(stop);
  return (ok == 0) && (written == len || len == 0);
}

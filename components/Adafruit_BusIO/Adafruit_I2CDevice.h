#ifndef ADAFRUIT_I2C_DEVICE_H
#define ADAFRUIT_I2C_DEVICE_H

#include <Arduino.h>
#include <Wire.h>

class Adafruit_I2CDevice {
public:
  Adafruit_I2CDevice(uint8_t addr, TwoWire *theWire = &Wire);
  bool begin();
  bool write(const uint8_t *data, size_t len);
  // Extended write: optionally supply a prefix buffer and control stop flag
  bool write(const uint8_t *data, size_t len, bool stop,
             const uint8_t *prefix = nullptr, size_t prefix_len = 0);
  bool write(uint8_t b);
  bool read(uint8_t *buffer, size_t len);
  uint8_t address() const { return _addr; }

private:
  uint8_t _addr;
  TwoWire *_wire;
};

#endif // ADAFRUIT_I2C_DEVICE_H

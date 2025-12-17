#pragma once

#include <cstdint>

class IEPDDriver {
public:
    virtual ~IEPDDriver() = default;

    virtual void GPIOInit() = 0;
    virtual void Init() = 0;
    virtual void FastInit() = 0;
    virtual void PartInit() = 0;
    virtual void Update() = 0;
    virtual void DeepSleep() = 0;

    virtual void DisplayClear() = 0;
    virtual void Display(const uint8_t* image) = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual uint8_t* getImageBuffer() = 0;

    virtual uint16_t rotated() = 0;
};

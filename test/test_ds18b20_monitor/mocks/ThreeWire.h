#pragma once

#include <Arduino.h>

class ThreeWire {
public:
  uint8_t dataPin;
  uint8_t clockPin;
  uint8_t enablePin;
  ThreeWire(uint8_t data, uint8_t clock, uint8_t enable)
      : dataPin(data), clockPin(clock), enablePin(enable) {}
};
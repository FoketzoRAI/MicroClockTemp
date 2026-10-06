#pragma once

#include <OneWire.h>

constexpr float DEVICE_DISCONNECTED_C = -127.0f;

class DallasTemperature {
public:
  bool present = true;
  bool discovered = false;
  bool waitForConversion = false;
  uint8_t resolution = 0;
  unsigned int beginCount = 0;
  unsigned int requestCount = 0;
  mutable unsigned int readCount = 0;
  unsigned long conversionMs = 0;
  float temperatureC = 25.0f;

  explicit DallasTemperature(OneWire*) {}
  void begin() {
    ++beginCount;
    discovered = present;
  }
  uint8_t getDeviceCount() const { return discovered ? 1 : 0; }
  void setResolution(uint8_t bits) { resolution = bits; }
  void setWaitForConversion(bool wait) { waitForConversion = wait; }
  void requestTemperatures() {
    ++requestCount;
    if (waitForConversion) mockMillis += conversionMs;
  }
  float getTempCByIndex(uint8_t) const {
    ++readCount;
    return present && discovered ? temperatureC : DEVICE_DISCONNECTED_C;
  }
};
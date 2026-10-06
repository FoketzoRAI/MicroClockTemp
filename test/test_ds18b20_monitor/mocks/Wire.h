#pragma once

#include <Arduino.h>
#include <cstddef>

class TwoWire {
public:
  bool present = true;
  bool timedOut = false;
  bool dataTimeout = false;
  uint8_t address = 0x3C;
  uint8_t dataResult = 0;
  uint8_t currentAddress = 0;
  size_t writeLimit = 32;
  size_t bytesWritten = 0;
  unsigned int dataTransactionCount = 0;
  unsigned int probeCount = 0;
  uint32_t timeoutUs = 0;

  void begin() { bytesWritten = 0; }
  void setClock(uint32_t) {}
  void setWireTimeout(uint32_t timeout, bool) {
    timeoutUs = timeout;
    timedOut = false;
  }
  bool getWireTimeoutFlag() const { return timedOut; }
  void clearWireTimeoutFlag() { timedOut = false; }
  void beginTransmission(uint8_t target) {
    currentAddress = target;
    bytesWritten = 0;
  }
  size_t write(const uint8_t*, size_t count) {
    const size_t accepted = count < writeLimit ? count : writeLimit;
    bytesWritten += accepted;
    return accepted;
  }
  uint8_t endTransmission() {
    if (bytesWritten == 0) {
      ++probeCount;
    } else {
      ++dataTransactionCount;
      if (dataTimeout) {
        timedOut = true;
        return 5;
      }
      if (dataResult != 0) {
        return dataResult;
      }
    }
    return present && currentAddress == address ? 0 : 2;
  }
};

extern TwoWire Wire;
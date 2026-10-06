#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#define F(text) text

extern unsigned long mockMillis;
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;
constexpr uint8_t INPUT_PULLUP = 2;
extern uint8_t mockPinValues[20];
extern uint8_t mockPinModes[20];
extern unsigned int mockPinReads[20];
inline void pinMode(uint8_t pin, uint8_t mode) { mockPinModes[pin] = mode; }
inline int digitalRead(uint8_t pin) {
  ++mockPinReads[pin];
  return mockPinValues[pin];
}

inline unsigned long millis() {
  return mockMillis;
}

inline char* dtostrf(double value, signed char width, unsigned char precision,
                     char* buffer) {
  std::snprintf(buffer, 8, "%*.*f", static_cast<int>(width),
                static_cast<int>(precision), value);
  return buffer;
}

class MockSerial {
public:
  unsigned long baud = 0;
  std::string output;
  std::string input;
  int available() const { return input.size(); }
  int read() {
    const unsigned char character = input.front();
    input.erase(0, 1);
    return character;
  }

  void begin(unsigned long speed) { baud = speed; }
  void print(const char* text) { output += text; }
  void print(float value, int precision) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.*f", precision, value);
    output += text;
  }
  void println(const char* text) {
    output += text;
    output += '\n';
  }
};

extern MockSerial Serial;
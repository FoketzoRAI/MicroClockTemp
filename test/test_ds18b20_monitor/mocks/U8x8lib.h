#pragma once

#include <Wire.h>
#include <vector>

constexpr uint8_t U8X8_PIN_NONE = 255;
constexpr uint8_t U8X8_MSG_BYTE_INIT = 1;
constexpr uint8_t U8X8_MSG_BYTE_START_TRANSFER = 2;
constexpr uint8_t U8X8_MSG_BYTE_SEND = 3;
constexpr uint8_t U8X8_MSG_BYTE_END_TRANSFER = 4;
extern const uint8_t u8x8_font_chroma48medium8_r[];

struct u8x8_t {
  uint8_t (*byte_cb)(u8x8_t*, uint8_t, uint8_t, void*) = nullptr;
  uint8_t i2cAddress = 0;
};

inline uint8_t u8x8_byte_arduino_hw_i2c(u8x8_t* state, uint8_t message,
                                     uint8_t count, void* data) {
  switch (message) {
    case U8X8_MSG_BYTE_INIT:
      Wire.begin();
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      Wire.beginTransmission(state->i2cAddress >> 1);
      break;
    case U8X8_MSG_BYTE_SEND:
      Wire.write(static_cast<const uint8_t*>(data), count);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      Wire.endTransmission();
      break;
    default:
      return 0;
  }
  return 1;
}

class U8X8_SSD1306_128X64_NONAME_HW_I2C {
public:
  struct Draw {
    uint8_t column;
    uint8_t row;
    bool large;
    std::string text;
  };
  u8x8_t state;
  std::vector<Draw> draws;

  explicit U8X8_SSD1306_128X64_NONAME_HW_I2C(uint8_t) {
    state.byte_cb = u8x8_byte_arduino_hw_i2c;
  }
  u8x8_t* getU8x8() { return &state; }
  void setI2CAddress(uint8_t address) { state.i2cAddress = address; }
  void begin() {
    state.byte_cb(&state, U8X8_MSG_BYTE_INIT, 0, nullptr);
    transfer();
  }
  void setFont(const uint8_t*) {}
  void clearDisplay() {
    for (unsigned int transaction = 0; transaction < 4; ++transaction) {
      transfer();
    }
  }
  void drawString(uint8_t column, uint8_t row, const char* text) {
    draws.push_back({column, row, false, text});
    transfer();
  }
  void draw2x2String(uint8_t column, uint8_t row, const char* text) {
    draws.push_back({column, row, true, text});
    transfer();
  }

private:
  void transfer() {
    uint8_t data[] = {0x40, 0x00};
    state.byte_cb(&state, U8X8_MSG_BYTE_START_TRANSFER, 0, nullptr);
    state.byte_cb(&state, U8X8_MSG_BYTE_SEND, sizeof(data), data);
    state.byte_cb(&state, U8X8_MSG_BYTE_END_TRANSFER, 0, nullptr);
  }
};
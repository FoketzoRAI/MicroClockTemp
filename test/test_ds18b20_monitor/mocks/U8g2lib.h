#pragma once

#include <U8x8lib.h>
#include <cstring>

extern const uint8_t U8G2_R0[];
extern const uint8_t u8g2_font_7x13B_mr[];
extern const uint8_t u8g2_font_5x7_tr[];
extern const uint8_t u8g2_font_logisoso24_tn[];

class U8G2_SSD1306_128X64_NONAME_1_HW_I2C {
public:
  struct Draw {
    uint16_t column;
    uint16_t row;
    const uint8_t* font;
    std::string text;
  };
  u8x8_t state;
  std::vector<Draw> draws;
  struct Line { uint16_t column; uint16_t row; uint16_t width; };
  std::vector<Line> lines;
  unsigned int frameCount = 0;
  unsigned int page = 0;
  const uint8_t* currentFont = u8g2_font_5x7_tr;
  bool geometryValid = true;

  U8G2_SSD1306_128X64_NONAME_1_HW_I2C(const uint8_t*, uint8_t) {
    state.byte_cb = u8x8_byte_arduino_hw_i2c;
  }
  u8x8_t* getU8x8() { return &state; }
  void setI2CAddress(uint8_t address) { state.i2cAddress = address; }
  void begin() {
    state.byte_cb(&state, U8X8_MSG_BYTE_INIT, 0, nullptr);
    transfer();
  }
  void setFont(const uint8_t* font) { currentFont = font; }
  uint16_t getStrWidth(const char* text) const {
    return std::strlen(text) * currentFont[0];
  }
  void firstPage() {
    page = 0;
    ++frameCount;
    draws.clear();
    lines.clear();
  }
  bool nextPage() {
    transfer();
    return ++page < 8;
  }
  void drawStr(uint16_t column, uint16_t row, const char* text) {
    geometryValid = geometryValid && column + getStrWidth(text) <= 128 && row < 64;
    draws.push_back({column, row, currentFont, text});
  }
  void drawHLine(uint16_t column, uint16_t row, uint16_t width) {
    lines.push_back({column, row, width});
    geometryValid = geometryValid && column + width <= 128 && row < 64;
  }
  void drawCircle(uint16_t column, uint16_t row, uint16_t radius) {
    geometryValid = geometryValid && column >= radius && row >= radius &&
                    column + radius < 128 && row + radius < 64;
  }

private:
  void transfer() {
    uint8_t data[] = {0x40, 0x00};
    state.byte_cb(&state, U8X8_MSG_BYTE_START_TRANSFER, 0, nullptr);
    state.byte_cb(&state, U8X8_MSG_BYTE_SEND, sizeof(data), data);
    state.byte_cb(&state, U8X8_MSG_BYTE_END_TRANSFER, 0, nullptr);
  }
};
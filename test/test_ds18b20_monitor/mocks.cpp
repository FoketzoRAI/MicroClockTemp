#include <Arduino.h>
#include <Wire.h>
#include <U8x8lib.h>
#include <U8g2lib.h>

unsigned long mockMillis = 0;
uint8_t mockPinValues[20] = {};
uint8_t mockPinModes[20] = {};
unsigned int mockPinReads[20] = {};
MockSerial Serial;
TwoWire Wire;

const uint8_t U8G2_R0[] = {0};
const uint8_t u8g2_font_7x13B_mr[] = {7};
const uint8_t u8g2_font_5x7_tr[] = {5};
const uint8_t u8g2_font_logisoso24_tn[] = {14};
const uint8_t u8x8_font_chroma48medium8_r[] = {0};

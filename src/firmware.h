#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>
#include <U8g2lib.h>

constexpr uint8_t SENSOR_PIN = 2;
constexpr uint8_t RTC_CLOCK_PIN = 3;
constexpr uint8_t RTC_DATA_PIN = 4;
constexpr uint8_t RTC_ENABLE_PIN = 5;
constexpr uint8_t SENSOR_RESOLUTION_BITS = 12;
constexpr unsigned long SERIAL_BAUD = 9600;
constexpr uint32_t SAMPLE_INTERVAL_MS = 1000;
constexpr uint32_t RECOVERY_INTERVAL_MS = 5000;
constexpr uint32_t I2C_TIMEOUT_US = 25000;
constexpr uint8_t BUTTON_PINS[] = {6, 7, 8};
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t BUTTON_HOLD_MS = 1000;
constexpr uint32_t EDIT_TIMEOUT_MS = 30000;
constexpr uint32_t CONVERSION_INTERVAL_MS = 750;

enum EditorField : uint8_t {
	EDIT_HOUR,
	EDIT_MINUTE,
	EDIT_YEAR,
	EDIT_MONTH,
	EDIT_DAY,
	EDIT_FIELD_COUNT
};

struct ButtonState {
	bool rawPressed = false;
	bool pressed = false;
	bool longHandled = false;
	uint32_t changedMs = 0;
	uint32_t pressedMs = 0;
};

enum class ButtonEvent : uint8_t { NONE, PRESS, RELEASE, HOLD };

extern OneWire oneWire;
extern DallasTemperature sensors;
extern ThreeWire rtcWire;
extern RtcDS1302<ThreeWire> rtc;
extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C display;
extern uint32_t previousSampleMs;
extern uint32_t previousDisplayRetryMs;
extern uint32_t previousSensorRetryMs;
extern uint8_t displayAddress;
extern bool displayAvailable;
extern bool displayWriteFailed;
extern bool sensorNeedsDiscovery;
extern bool hasValidReading;
extern float minimumTemperatureC;
extern float maximumTemperatureC;
extern char displayedTemperature[8];
extern char displayedMinimum[8];
extern char displayedMaximum[8];
extern int8_t displayedSensorState;
extern int8_t displayedClockState;
extern bool clockValid;
extern char clockText[9];
extern char dateText[11];
extern char displayedClock[9];
extern char displayedDate[11];
extern char serialCommand[32];
extern uint8_t serialCommandLength;
extern bool serialCommandOverflow;
extern ButtonState buttons[3];
extern bool editorActive;
extern bool editorSaveError;
extern bool editorDirty;
extern EditorField selectedField;
extern uint32_t editorActivityMs;
extern RtcDateTime editDraft;
extern float lastTemperatureC;
extern bool lastReadingValid;
extern bool conversionPending;
extern uint32_t conversionStartedMs;

void configureRtcSafety();
bool validClockDate(const RtcDateTime& dateTime);
void writeTwoDigits(char* text, uint8_t value);
void formatClock(const RtcDateTime& dateTime, bool valid);
void readClock();
uint16_t decimalValue(const char* text, uint8_t count);
bool parseTimeCommand(const char* command, RtcDateTime& dateTime);
bool writeClock(const RtcDateTime& requestedTime);

ButtonEvent updateButton(ButtonState& button, bool pressed, uint32_t currentMs,
							 bool recognizeHold);
void handleTimeCommand();
uint8_t daysInMonth(uint16_t year, uint8_t month);
void adjustDraft(bool increase);
void handleSetHold();
void serviceButtons();
void serviceSerial();

uint8_t checkedDisplayByte(u8x8_t* displayState, uint8_t message,
								uint8_t byteCount, void* data);
bool displayResponds();
void disableDisplay();
void initializeDisplay();
void formatTemperature(float temperatureC, bool valid, uint8_t width, char* text);
void drawScreen(bool readingValid, const char* temperatureText,
				 const char* minimumText, const char* maximumText);
void updateDisplay(bool readingValid, float temperatureC);

void setup();
void loop();
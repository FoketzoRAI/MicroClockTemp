#include "firmware.h"

#include <math.h>

OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
ThreeWire rtcWire(RTC_DATA_PIN, RTC_CLOCK_PIN, RTC_ENABLE_PIN);
RtcDS1302<ThreeWire> rtc(rtcWire);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
uint32_t previousSampleMs = 0;
uint32_t previousDisplayRetryMs = 0;
uint32_t previousSensorRetryMs = 0;
uint8_t displayAddress = 0;
bool displayAvailable = false;
bool displayWriteFailed = false;
bool sensorNeedsDiscovery = false;
bool hasValidReading = false;
float minimumTemperatureC = 0.0f;
float maximumTemperatureC = 0.0f;
char displayedTemperature[8] = "";
char displayedMinimum[8] = "";
char displayedMaximum[8] = "";
int8_t displayedSensorState = -1;
int8_t displayedClockState = -1;
bool clockValid = false;
char clockText[9] = "--:--:--";
char dateText[11] = "---- -- --";
char displayedClock[9] = "";
char displayedDate[11] = "";
char serialCommand[32] = "";
uint8_t serialCommandLength = 0;
bool serialCommandOverflow = false;
ButtonState buttons[3];
bool editorActive = false;
bool editorSaveError = false;
bool editorDirty = false;
EditorField selectedField = EDIT_HOUR;
uint32_t editorActivityMs = 0;
RtcDateTime editDraft;
float lastTemperatureC = 0.0f;
bool lastReadingValid = false;
bool conversionPending = false;
uint32_t conversionStartedMs = 0;

void initializeSensor() {
	sensors.begin();
	sensors.setResolution(SENSOR_RESOLUTION_BITS);
	sensors.setWaitForConversion(false);
}

void setup() {
	Serial.begin(SERIAL_BAUD);
	for (const uint8_t pin : BUTTON_PINS) pinMode(pin, INPUT_PULLUP);
	rtc.Begin();
	configureRtcSafety();
	initializeSensor();
	sensorNeedsDiscovery = sensors.getDeviceCount() == 0;
	previousSensorRetryMs = millis();
	initializeDisplay();
	previousSampleMs = millis() - SAMPLE_INTERVAL_MS;
}

void loop() {
	serviceButtons();
	serviceSerial();
	if (editorDirty && displayAvailable) {
		readClock();
		updateDisplay(lastReadingValid, lastTemperatureC);
	}
	const uint32_t currentMs = millis();
	if (!conversionPending) {
		if (currentMs - previousSampleMs < SAMPLE_INTERVAL_MS) return;
		previousSampleMs = currentMs;
		if (!editorActive && sensorNeedsDiscovery &&
				currentMs - previousSensorRetryMs >= RECOVERY_INTERVAL_MS) {
			previousSensorRetryMs = currentMs;
			initializeSensor();
		}
		if (!editorActive && !displayAvailable &&
				currentMs - previousDisplayRetryMs >= RECOVERY_INTERVAL_MS) {
			initializeDisplay();
		}
		sensors.requestTemperatures();
		conversionStartedMs = millis();
		conversionPending = true;
		return;
	}
	if (currentMs - conversionStartedMs < CONVERSION_INTERVAL_MS) return;
	conversionPending = false;
	const float temperatureC = sensors.getTempCByIndex(0);
	lastTemperatureC = temperatureC;
	lastReadingValid = isfinite(temperatureC) && temperatureC != DEVICE_DISCONNECTED_C &&
							 temperatureC >= -55.0f && temperatureC <= 125.0f;
	readClock();
	Serial.print(editorActive ? F("Draft: ") : F("Time: "));
	if (clockValid || editorActive) {
		Serial.print(dateText);
		Serial.print(F(" "));
		Serial.println(clockText);
	} else {
		Serial.println(F("unavailable"));
	}
	if (!lastReadingValid) {
		sensorNeedsDiscovery = true;
		Serial.println(F("Error: sensor unavailable"));
		updateDisplay(false, 0.0f);
		return;
	}
	sensorNeedsDiscovery = false;

	if (!hasValidReading) {
		minimumTemperatureC = temperatureC;
		maximumTemperatureC = temperatureC;
		hasValidReading = true;
	} else {
		if (temperatureC < minimumTemperatureC) {
			minimumTemperatureC = temperatureC;
		}
		if (temperatureC > maximumTemperatureC) {
			maximumTemperatureC = temperatureC;
		}
	}

	Serial.print(F("Temperature: "));
	Serial.print(temperatureC, 2);
	Serial.println(F(" C"));
	updateDisplay(true, temperatureC);
}

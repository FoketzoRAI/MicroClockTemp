#include "firmware.h"

#include <Wire.h>
#include <stdlib.h>
#include <string.h>

uint8_t checkedDisplayByte(u8x8_t* displayState, uint8_t message,
								uint8_t byteCount, void* data) {
	if (displayWriteFailed) {
		return 0;
	}
	if (message == U8X8_MSG_BYTE_SEND) {
		displayWriteFailed =
				Wire.write(static_cast<const uint8_t*>(data), byteCount) != byteCount;
	} else if (message == U8X8_MSG_BYTE_END_TRANSFER) {
		displayWriteFailed = Wire.endTransmission() != 0 || Wire.getWireTimeoutFlag();
	} else {
		return u8x8_byte_arduino_hw_i2c(displayState, message, byteCount, data);
	}
	return displayWriteFailed ? 0 : 1;
}

bool displayResponds() {
	Wire.clearWireTimeoutFlag();
	Wire.beginTransmission(displayAddress);
	const uint8_t result = Wire.endTransmission();
	return result == 0 && !Wire.getWireTimeoutFlag();
}

void disableDisplay() {
	displayAvailable = false;
	editorActive = false;
	editorSaveError = false;
	editorDirty = true;
	Serial.println(F("OLED unavailable; serial only"));
}

void initializeDisplay() {
	previousDisplayRetryMs = millis();
	Wire.begin();
	Wire.setWireTimeout(I2C_TIMEOUT_US, true);
	const uint8_t candidateAddresses[] = {0x3C, 0x3D};
	for (const uint8_t candidateAddress : candidateAddresses) {
		displayAddress = candidateAddress;
		if (displayResponds()) {
			displayAvailable = true;
			break;
		}
	}
	if (!displayAvailable) {
		disableDisplay();
		return;
	}

	display.setI2CAddress(displayAddress * 2);
	displayWriteFailed = false;
	display.getU8x8()->byte_cb = checkedDisplayByte;
	display.begin();
	if (displayWriteFailed) {
		disableDisplay();
		return;
	}
	displayedTemperature[0] = '\0';
	displayedMinimum[0] = '\0';
	displayedMaximum[0] = '\0';
	displayedSensorState = -1;
	displayedClockState = -1;
	displayedClock[0] = '\0';
	displayedDate[0] = '\0';
}

void formatTemperature(float temperatureC, bool valid, uint8_t width, char* text) {
	if (valid) {
		dtostrf(temperatureC, width, 1, text);
	} else {
		strcpy(text, width == 5 ? " --.-" : "  --.-");
	}
}

void drawScreen(bool readingValid, const char* temperatureText,
				const char* minimumText, const char* maximumText) {
	display.setFont(u8g2_font_7x13B_mr);
	display.drawStr(0, 12, clockText);
	display.setFont(u8g2_font_5x7_tr);
	display.drawStr(128 - display.getStrWidth(dateText), 11, dateText);
	display.drawHLine(0, 16, 128);
	if (editorActive) {
		const uint8_t fieldX[] = {0, 21, 78, 103, 118};
		const uint8_t fieldWidth[] = {14, 14, 20, 10, 10};
		display.drawHLine(fieldX[selectedField], 14, fieldWidth[selectedField]);
	}

	display.setFont(u8g2_font_logisoso24_tn);
	const uint16_t temperatureWidth = display.getStrWidth(temperatureText);
	const uint8_t temperatureX = (128 - temperatureWidth - 18) / 2;
	display.drawStr(temperatureX, 43, temperatureText);
	const uint8_t unitX = temperatureX + temperatureWidth;
	display.drawCircle(unitX + 3, 27, 2);
	display.setFont(u8g2_font_7x13B_mr);
	display.drawStr(unitX + 8, 37, "C");

	display.setFont(u8g2_font_5x7_tr);
	display.drawStr(0, 53, "MIN ");
	display.drawStr(display.getStrWidth("MIN "), 53, minimumText);
	const uint16_t maximumLabelWidth = display.getStrWidth("MAX ");
	const uint16_t maximumValueX = 128 - display.getStrWidth(maximumText);
	display.drawStr(maximumValueX - maximumLabelWidth, 53, "MAX ");
	display.drawStr(maximumValueX, 53, maximumText);
	const char* const editLabels[] = {"EDIT HOUR", "EDIT MIN", "EDIT YEAR", "EDIT MONTH", "EDIT DAY"};
	display.drawStr(0, 63, editorActive ? (editorSaveError ? "SAVE ERR" : editLabels[selectedField]) :
								 readingValid ? "SENSOR OK" : "SENSOR ERROR");
	display.drawStr(128 - display.getStrWidth(clockValid ? "RTC OK" : "RTC ERR"),
					63, clockValid ? "RTC OK" : "RTC ERR");
}

void updateDisplay(bool readingValid, float temperatureC) {
	if (!displayAvailable) {
		return;
	}
	if (!displayResponds()) {
		disableDisplay();
		return;
	}

	char temperatureText[8];
	char minimumText[8];
	char maximumText[8];
	formatTemperature(temperatureC, readingValid, 5, temperatureText);
	formatTemperature(minimumTemperatureC, hasValidReading, 6, minimumText);
	formatTemperature(maximumTemperatureC, hasValidReading, 6, maximumText);
	if (!editorDirty && strcmp(clockText, displayedClock) == 0 &&
			strcmp(dateText, displayedDate) == 0 &&
			strcmp(temperatureText, displayedTemperature) == 0 &&
			strcmp(minimumText, displayedMinimum) == 0 &&
			strcmp(maximumText, displayedMaximum) == 0 &&
			displayedSensorState == readingValid && displayedClockState == clockValid) {
		return;
	}
	display.firstPage();
	do {
		drawScreen(readingValid, temperatureText, minimumText, maximumText);
	} while (!displayWriteFailed && display.nextPage());
	if (displayWriteFailed) {
		disableDisplay();
		return;
	}
	strcpy(displayedClock, clockText);
	strcpy(displayedDate, dateText);
	strcpy(displayedTemperature, temperatureText);
	strcpy(displayedMinimum, minimumText);
	strcpy(displayedMaximum, maximumText);
	displayedSensorState = readingValid;
	displayedClockState = clockValid;
	editorDirty = false;
}
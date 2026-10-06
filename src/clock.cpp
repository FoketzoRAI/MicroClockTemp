#include "firmware.h"

#include <string.h>

void configureRtcSafety() {
	rtc.SetIsWriteProtected(false);
	rtc.SetTrickleChargeSettings(DS1302Tcr_Disabled);
	rtc.SetIsWriteProtected(true);
}

bool validClockDate(const RtcDateTime& dateTime) {
	return dateTime.Year() >= 2000 && dateTime.Year() <= 2099 && dateTime.IsValid();
}

void writeTwoDigits(char* text, uint8_t value) {
	text[0] = '0' + value / 10;
	text[1] = '0' + value % 10;
}

void formatClock(const RtcDateTime& dateTime, bool valid) {
	if (!valid) {
		strcpy(clockText, "--:--:--");
		strcpy(dateText, "---- -- --");
		return;
	}
	writeTwoDigits(clockText, dateTime.Hour());
	clockText[2] = ':';
	writeTwoDigits(clockText + 3, dateTime.Minute());
	clockText[5] = ':';
	writeTwoDigits(clockText + 6, dateTime.Second());
	clockText[8] = '\0';
	dateText[0] = '2';
	dateText[1] = '0';
	writeTwoDigits(dateText + 2, static_cast<uint8_t>(dateTime.Year() - 2000));
	dateText[4] = '-';
	writeTwoDigits(dateText + 5, dateTime.Month());
	dateText[7] = '-';
	writeTwoDigits(dateText + 8, dateTime.Day());
	dateText[10] = '\0';
}

void readClock() {
	uint8_t chargeSettings = rtc.GetTrickleChargeSettings();
	if (chargeSettings != DS1302Tcr_Disabled) {
		configureRtcSafety();
		chargeSettings = rtc.GetTrickleChargeSettings();
	}
	const RtcDateTime dateTime = rtc.GetDateTime();
	clockValid = chargeSettings == DS1302Tcr_Disabled &&
							 rtc.GetIsRunning() && validClockDate(dateTime);
	formatClock(editorActive ? editDraft : dateTime, editorActive || clockValid);
}

uint16_t decimalValue(const char* text, uint8_t count) {
	uint16_t value = 0;
	for (uint8_t index = 0; index < count; ++index) {
		value = value * 10 + (text[index] - '0');
	}
	return value;
}

bool parseTimeCommand(const char* command, RtcDateTime& dateTime) {
	if (strlen(command) != 24 || strncmp(command, "TIME ", 5) != 0) {
		return false;
	}
	for (uint8_t index = 5; index < 24; ++index) {
		if (index == 9 || index == 12) {
			if (command[index] != '-') return false;
		} else if (index == 15) {
			if (command[index] != ' ') return false;
		} else if (index == 18 || index == 21) {
			if (command[index] != ':') return false;
		} else if (command[index] < '0' || command[index] > '9') {
			return false;
		}
	}
	dateTime = RtcDateTime(decimalValue(command + 5, 4),
							 decimalValue(command + 10, 2),
							 decimalValue(command + 13, 2),
							 decimalValue(command + 16, 2),
							 decimalValue(command + 19, 2),
							 decimalValue(command + 22, 2));
	return validClockDate(dateTime);
}

bool writeClock(const RtcDateTime& requestedTime) {
	if (!validClockDate(requestedTime)) {
		return false;
	}
	rtc.SetIsWriteProtected(false);
	rtc.SetTrickleChargeSettings(DS1302Tcr_Disabled);
	rtc.SetDateTime(requestedTime);
	rtc.SetIsRunning(true);
	rtc.SetIsWriteProtected(true);
	const RtcDateTime verifiedTime = rtc.GetDateTime();
	return validClockDate(verifiedTime) && rtc.GetIsRunning() &&
							 rtc.GetIsWriteProtected() &&
							 rtc.GetTrickleChargeSettings() == DS1302Tcr_Disabled &&
							 verifiedTime.TotalSeconds() - requestedTime.TotalSeconds() <= 1;
}
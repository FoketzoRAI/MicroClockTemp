#include "firmware.h"

ButtonEvent updateButton(ButtonState& button, bool pressed, uint32_t currentMs,
							 bool recognizeHold) {
	if (pressed != button.rawPressed) {
		button.rawPressed = pressed;
		button.changedMs = currentMs;
	}
	if (currentMs - button.changedMs >= BUTTON_DEBOUNCE_MS &&
			button.pressed != button.rawPressed) {
		button.pressed = button.rawPressed;
		if (button.pressed) {
			button.pressedMs = currentMs;
			button.longHandled = false;
			return ButtonEvent::PRESS;
		}
		return button.longHandled ? ButtonEvent::NONE : ButtonEvent::RELEASE;
	}
	if (recognizeHold && button.pressed && button.rawPressed && !button.longHandled &&
			currentMs - button.pressedMs >= BUTTON_HOLD_MS) {
		button.longHandled = true;
		return ButtonEvent::HOLD;
	}
	return ButtonEvent::NONE;
}

void handleTimeCommand() {
	if (editorActive) {
		Serial.println(F("Error: clock edit active"));
		return;
	}
	RtcDateTime requestedTime;
	if (serialCommandOverflow || !parseTimeCommand(serialCommand, requestedTime)) {
		Serial.println(F("Error: use TIME YYYY-MM-DD HH:MM:SS (2000-2099)"));
		return;
	}
	Serial.println(writeClock(requestedTime) ? F("RTC time set") : F("Error: RTC write failed"));
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
	if (month == 2) return year % 4 == 0 ? 29 : 28;
	return month == 4 || month == 6 || month == 9 || month == 11 ? 30 : 31;
}

void adjustDraft(bool increase) {
	uint16_t values[] = {editDraft.Hour(), editDraft.Minute(), editDraft.Year(),
							 editDraft.Month(), editDraft.Day()};
	uint16_t lower = 0;
	uint16_t upper = 0;
	switch (selectedField) {
		case EDIT_HOUR: upper = 23; break;
		case EDIT_MINUTE: upper = 59; break;
		case EDIT_YEAR: lower = 2000; upper = 2099; break;
		case EDIT_MONTH: lower = 1; upper = 12; break;
		case EDIT_DAY: lower = 1; upper = daysInMonth(values[EDIT_YEAR], values[EDIT_MONTH]); break;
		case EDIT_FIELD_COUNT: return;
	}
	uint16_t& value = values[selectedField];
	value = increase ? (value < upper ? value + 1 : lower) :
							 (value > lower ? value - 1 : upper);
	const uint8_t maximumDay = daysInMonth(values[EDIT_YEAR], values[EDIT_MONTH]);
	if (values[EDIT_DAY] > maximumDay) values[EDIT_DAY] = maximumDay;
	editDraft = RtcDateTime(values[EDIT_YEAR], values[EDIT_MONTH], values[EDIT_DAY],
							 values[EDIT_HOUR], values[EDIT_MINUTE], 0);
	editorSaveError = false;
	editorDirty = true;
}

void handleSetHold() {
	if (!displayAvailable) return;
	if (!displayResponds()) {
		disableDisplay();
		return;
	}
	if (!editorActive) {
		editDraft = rtc.GetDateTime();
		if (!validClockDate(editDraft)) editDraft = RtcDateTime(2000, 1, 1, 0, 0, 0);
		selectedField = EDIT_HOUR;
		editorActive = true;
		editorSaveError = false;
	} else {
		const RtcDateTime requestedTime(editDraft.Year(), editDraft.Month(), editDraft.Day(),
											editDraft.Hour(), editDraft.Minute(), 0);
		editorSaveError = !writeClock(requestedTime);
		if (!editorSaveError) editorActive = false;
		Serial.println(editorSaveError ? F("Error: RTC write failed") : F("RTC time set"));
	}
	editorDirty = true;
}

void serviceButtons() {
	const uint32_t currentMs = millis();
	bool pressedStates[3];
	for (uint8_t index = 0; index < 3; ++index) {
		pressedStates[index] = digitalRead(BUTTON_PINS[index]) == LOW;
	}
	for (uint8_t index = 0; index < 3; ++index) {
		const ButtonEvent event = updateButton(buttons[index], pressedStates[index], currentMs,
												 index == 0);
		if (index == 0) {
			if (event == ButtonEvent::HOLD) {
				handleSetHold();
				editorActivityMs = currentMs;
			} else if (event == ButtonEvent::RELEASE && editorActive) {
				selectedField = static_cast<EditorField>((selectedField + 1) % EDIT_FIELD_COUNT);
				editorActivityMs = currentMs;
				editorSaveError = false;
				editorDirty = true;
			}
		} else if (event == ButtonEvent::PRESS && editorActive && !pressedStates[0] &&
					 !pressedStates[index == 1 ? 2 : 1]) {
			adjustDraft(index == 1);
			editorActivityMs = currentMs;
		}
	}
	if (editorActive && !buttons[0].pressed &&
			currentMs - editorActivityMs >= EDIT_TIMEOUT_MS) {
		editorActive = false;
		editorSaveError = false;
		editorDirty = true;
	}
}

void serviceSerial() {
	for (uint8_t processed = 0; processed < sizeof(serialCommand) && Serial.available();
			 ++processed) {
		const char character = Serial.read();
		if (character == '\r' || character == '\n') {
			if (serialCommandLength != 0 || serialCommandOverflow) {
				serialCommand[serialCommandLength] = '\0';
				handleTimeCommand();
			}
			serialCommandLength = 0;
			serialCommandOverflow = false;
		} else if (serialCommandLength < sizeof(serialCommand) - 1) {
			serialCommand[serialCommandLength++] = character;
		} else {
			serialCommandOverflow = true;
		}
	}
}
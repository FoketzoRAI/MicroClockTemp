#include <unity.h>
#include <cmath>
#include <cstring>
#include "../../src/firmware.h"

void setUp() {
  mockMillis = 0;
  for (uint8_t pin = 0; pin < 20; ++pin) {
    mockPinValues[pin] = HIGH;
    mockPinModes[pin] = 0;
    mockPinReads[pin] = 0;
  }
  for (auto& button : buttons) button = ButtonState();
  editorActive = false;
  editorSaveError = false;
  editorDirty = false;
  selectedField = EDIT_HOUR;
  editorActivityMs = 0;
  editDraft = RtcDateTime();
  lastTemperatureC = 0.0f;
  lastReadingValid = false;
  conversionPending = false;
  conversionStartedMs = 0;
  Serial = MockSerial();
  Wire = TwoWire();
  sensors = DallasTemperature(&oneWire);
  rtc = RtcDS1302<ThreeWire>(rtcWire);
  display = U8G2_SSD1306_128X64_NONAME_1_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
  previousSampleMs = 0;
  previousDisplayRetryMs = 0;
  previousSensorRetryMs = 0;
  displayAddress = 0;
  displayAvailable = false;
  displayWriteFailed = false;
  sensorNeedsDiscovery = false;
  hasValidReading = false;
  minimumTemperatureC = 0.0f;
  maximumTemperatureC = 0.0f;
  displayedTemperature[0] = '\0';
  displayedMinimum[0] = '\0';
  displayedMaximum[0] = '\0';
  displayedSensorState = -1;
  displayedClockState = -1;
  clockValid = false;
  strcpy(clockText, "--:--:--");
  strcpy(dateText, "---- -- --");
  displayedClock[0] = '\0';
  displayedDate[0] = '\0';
  serialCommandLength = 0;
  serialCommandOverflow = false;
  serialCommand[0] = '\0';
}

void tearDown() {}

void sampleAt(unsigned long timestamp, float temperatureC) {
  mockMillis = timestamp;
  sensors.temperatureC = temperatureC;
  loop();
  if (conversionPending) {
    mockMillis = static_cast<uint32_t>(conversionStartedMs + CONVERSION_INTERVAL_MS);
    loop();
  }
}

const char* textAt(uint8_t row) {
  for (auto draw = display.draws.rbegin(); draw != display.draws.rend(); ++draw) {
    if (row == 2 && draw->font == u8g2_font_logisoso24_tn) {
      return draw->text.c_str();
    }
    if (row == 5 && draw->row == 53 && draw->column == 20) {
      return draw->text.c_str();
    }
    if (row == 6 && draw->row == 53 && draw->column == 98) {
      return draw->text.c_str();
    }
    if (row == 7 && draw->column == 0 && draw->row == 63) {
      return draw->text.c_str();
    }
  }
  return "";
}

void test_valid_read_updates_extremes_and_serial() {
  setup();
  sampleAt(0, 23.5f);
  TEST_ASSERT_TRUE(hasValidReading);
  TEST_ASSERT_EQUAL_FLOAT(23.5f, minimumTemperatureC);
  TEST_ASSERT_EQUAL_FLOAT(23.5f, maximumTemperatureC);
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "Temperature: 23.50 C"));
  TEST_ASSERT_EQUAL_STRING(" 23.5", textAt(2));
  TEST_ASSERT_EQUAL_STRING("SENSOR OK", textAt(7));
  TEST_ASSERT_EQUAL_UINT32(9600, Serial.baud);
  TEST_ASSERT_EQUAL_UINT32(25000, Wire.timeoutUs);
  TEST_ASSERT_EQUAL_UINT8(12, sensors.resolution);
}

void test_invalid_reads_preserve_extremes() {
  setup();
  sampleAt(0, 20.0f);
  sampleAt(1000, 30.0f);
  const float invalidReadings[] = {DEVICE_DISCONNECTED_C, NAN, INFINITY,
                                   -56.0f, 126.0f};
  unsigned long timestamp = 2000;
  for (const float invalidReading : invalidReadings) {
    sampleAt(timestamp, invalidReading);
    timestamp += 1000;
    TEST_ASSERT_EQUAL_FLOAT(20.0f, minimumTemperatureC);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, maximumTemperatureC);
    TEST_ASSERT_EQUAL_STRING(" --.-", textAt(2));
    TEST_ASSERT_EQUAL_STRING("SENSOR ERROR", textAt(7));
  }
}

void test_sensorless_startup_has_no_fake_extremes() {
  sensors.present = false;
  setup();
  sampleAt(0, 25.0f);
  TEST_ASSERT_FALSE(hasValidReading);
  TEST_ASSERT_EQUAL_STRING("  --.-", textAt(5));
  TEST_ASSERT_EQUAL_STRING("  --.-", textAt(6));
}

void test_valid_boundaries_and_85_are_accepted() {
  setup();
  sampleAt(0, -55.0f);
  TEST_ASSERT_EQUAL_STRING("-55.0", textAt(2));
  sampleAt(1000, 125.0f);
  TEST_ASSERT_EQUAL_STRING("125.0", textAt(2));
  sampleAt(2000, 85.0f);
  TEST_ASSERT_EQUAL_STRING(" 85.0", textAt(2));
  TEST_ASSERT_EQUAL_FLOAT(-55.0f, minimumTemperatureC);
  TEST_ASSERT_EQUAL_FLOAT(125.0f, maximumTemperatureC);
}

void test_probe_ack_then_data_nack_stops_transfers() {
  setup();
  const unsigned int previousTransactions = Wire.dataTransactionCount;
  Wire.dataResult = 3;
  sampleAt(0, 25.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_TRUE(displayWriteFailed);
  TEST_ASSERT_EQUAL_UINT(previousTransactions + 1, Wire.dataTransactionCount);
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "Temperature:"));
}

void test_initialization_nack_stops_transfers() {
  Wire.dataResult = 3;
  setup();
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT(1, Wire.dataTransactionCount);
}

void test_timeout_stops_transfers() {
  setup();
  const unsigned int previousTransactions = Wire.dataTransactionCount;
  Wire.dataTimeout = true;
  sampleAt(0, 25.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT(previousTransactions + 1, Wire.dataTransactionCount);
}

void test_short_write_disables_display() {
  setup();
  Wire.writeLimit = 0;
  sampleAt(0, 25.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_TRUE(displayWriteFailed);
}

void test_alternative_display_address() {
  Wire.address = 0x3D;
  setup();
  TEST_ASSERT_TRUE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT8(0x3D, displayAddress);
  TEST_ASSERT_EQUAL_UINT8(0x7A, display.state.i2cAddress);
}

void test_absent_display_keeps_serial_working() {
  Wire.present = false;
  setup();
  sampleAt(0, 24.0f);
  sampleAt(1000, 25.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT(0, Wire.dataTransactionCount);
  TEST_ASSERT_EQUAL_UINT(2, sensors.requestCount);
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "Temperature: 25.00 C"));
}

void test_conversion_time_is_included_in_sample_interval() {
  sensors.conversionMs = 750;
  setup();
  sampleAt(0, 25.0f);
  TEST_ASSERT_EQUAL_UINT(1, sensors.requestCount);
  sampleAt(999, 25.0f);
  TEST_ASSERT_EQUAL_UINT(1, sensors.requestCount);
  sampleAt(1000, 25.0f);
  TEST_ASSERT_EQUAL_UINT(2, sensors.requestCount);
}

void test_unchanged_formatted_values_are_not_redrawn() {
  setup();
  sampleAt(0, 25.0f);
  display.draws.clear();
  const unsigned int previousTransactions = Wire.dataTransactionCount;
  sampleAt(1000, 25.01f);
  TEST_ASSERT_EQUAL_UINT(0, display.draws.size());
  TEST_ASSERT_EQUAL_UINT(previousTransactions, Wire.dataTransactionCount);
  TEST_ASSERT_EQUAL_FLOAT(25.01f, maximumTemperatureC);
}

void test_changed_content_redraws_complete_page_buffer_frame() {
  setup();
  sampleAt(0, 20.0f);
  display.draws.clear();
  sampleAt(1000, 25.0f);
  TEST_ASSERT_EQUAL_UINT(2, display.frameCount);
  TEST_ASSERT_EQUAL_STRING(" 25.0", textAt(2));
  TEST_ASSERT_EQUAL_STRING("  20.0", textAt(5));
  TEST_ASSERT_EQUAL_STRING("  25.0", textAt(6));
}

void test_error_transition_redraws_once_and_preserves_extremes() {
  setup();
  sampleAt(0, 20.0f);
  display.draws.clear();
  sampleAt(1000, DEVICE_DISCONNECTED_C);
  TEST_ASSERT_EQUAL_UINT(2, display.frameCount);
  TEST_ASSERT_EQUAL_STRING(" --.-", textAt(2));
  TEST_ASSERT_EQUAL_STRING("SENSOR ERROR", textAt(7));
  display.draws.clear();
  sampleAt(2000, DEVICE_DISCONNECTED_C);
  TEST_ASSERT_EQUAL_UINT(0, display.draws.size());
}

void test_sensor_discovery_recovers_at_bounded_interval() {
  sensors.present = false;
  setup();
  sampleAt(0, 22.0f);
  sensors.present = true;
  sampleAt(4000, 22.0f);
  TEST_ASSERT_FALSE(hasValidReading);
  TEST_ASSERT_EQUAL_UINT(1, sensors.beginCount);
  sampleAt(5000, 22.0f);
  TEST_ASSERT_TRUE(hasValidReading);
  TEST_ASSERT_EQUAL_UINT(2, sensors.beginCount);
  TEST_ASSERT_FALSE(sensorNeedsDiscovery);
  TEST_ASSERT_EQUAL_STRING(" 22.0", textAt(2));
}

void test_missing_sensor_retries_are_not_per_sample() {
  sensors.present = false;
  setup();
  for (unsigned long timestamp = 0; timestamp < 5000; timestamp += 1000) {
    sampleAt(timestamp, 22.0f);
  }
  TEST_ASSERT_EQUAL_UINT(1, sensors.beginCount);
  sampleAt(5000, 22.0f);
  sampleAt(6000, 22.0f);
  TEST_ASSERT_EQUAL_UINT(2, sensors.beginCount);
  sampleAt(10000, 22.0f);
  TEST_ASSERT_EQUAL_UINT(3, sensors.beginCount);
}

void test_display_recovery_invalidates_cached_text() {
  setup();
  sampleAt(0, 22.0f);
  Wire.dataResult = 3;
  sampleAt(1000, 23.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  Wire.dataResult = 0;
  display.draws.clear();
  sampleAt(4000, 23.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  sampleAt(5000, 23.0f);
  TEST_ASSERT_TRUE(displayAvailable);
  TEST_ASSERT_FALSE(displayWriteFailed);
  TEST_ASSERT_EQUAL_STRING(" 23.0", textAt(2));
  TEST_ASSERT_EQUAL_STRING("  22.0", textAt(5));
  TEST_ASSERT_EQUAL_STRING("  23.0", textAt(6));
  TEST_ASSERT_EQUAL_STRING("SENSOR OK", textAt(7));
}

void test_displayless_startup_recovers() {
  Wire.present = false;
  setup();
  sampleAt(0, 22.0f);
  Wire.present = true;
  sampleAt(4000, 22.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  sampleAt(5000, 22.0f);
  TEST_ASSERT_TRUE(displayAvailable);
  TEST_ASSERT_EQUAL_STRING(" 22.0", textAt(2));
}

void test_sample_timer_rollover() {
  mockMillis = UINT32_MAX - 500;
  setup();
  sampleAt(UINT32_MAX - 500, 22.0f);
  sampleAt(498, 23.0f);
  TEST_ASSERT_EQUAL_UINT(1, sensors.requestCount);
  sampleAt(499, 23.0f);
  TEST_ASSERT_EQUAL_UINT(2, sensors.requestCount);
}

void test_recovery_timer_rollover() {
  mockMillis = UINT32_MAX - 500;
  Wire.present = false;
  sensors.present = false;
  setup();
  sampleAt(UINT32_MAX - 500, 22.0f);
  Wire.present = true;
  sensors.present = true;
  sampleAt(3498, 22.0f);
  sampleAt(4498, 22.0f);
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT(1, sensors.beginCount);
  sampleAt(5498, 22.0f);
  TEST_ASSERT_TRUE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT(2, sensors.beginCount);
}

void test_clock_preserves_saved_time_and_disables_charging() {
  rtc.time = RtcDateTime(2025, 12, 31, 23, 59, 59);
  setup();
  sampleAt(0, 25.0f);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
  TEST_ASSERT_EQUAL_UINT8(DS1302Tcr_Disabled, rtc.charger);
  TEST_ASSERT_TRUE(rtc.protectedWrites);
  TEST_ASSERT_EQUAL_STRING("23:59:59", clockText);
  TEST_ASSERT_EQUAL_STRING("2025-12-31", dateText);
}

void test_clock_formats_new_year_and_leap_day() {
  setup();
  rtc.time = RtcDateTime(2025, 12, 31, 23, 59, 59);
  sampleAt(0, 25.0f);
  rtc.time = RtcDateTime(2026, 1, 1, 0, 0, 0);
  sampleAt(1000, 25.0f);
  TEST_ASSERT_EQUAL_STRING("00:00:00", clockText);
  TEST_ASSERT_EQUAL_STRING("2026-01-01", dateText);
  rtc.time = RtcDateTime(2028, 2, 29, 6, 7, 8);
  sampleAt(2000, 25.0f);
  TEST_ASSERT_EQUAL_STRING("06:07:08", clockText);
  TEST_ASSERT_EQUAL_STRING("2028-02-29", dateText);
}

void test_invalid_or_stopped_clock_does_not_stop_temperature() {
  setup();
  rtc.running = false;
  sampleAt(0, 25.0f);
  TEST_ASSERT_FALSE(clockValid);
  TEST_ASSERT_EQUAL_STRING("--:--:--", clockText);
  rtc.running = true;
  rtc.time = RtcDateTime(2026, 2, 30, 12, 0, 0);
  sampleAt(1000, 26.0f);
  TEST_ASSERT_FALSE(clockValid);
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "Temperature: 26.00 C"));
}

void test_time_command_validates_calendar_and_format() {
  RtcDateTime parsed;
  TEST_ASSERT_TRUE(parseTimeCommand("TIME 2028-02-29 23:59:59", parsed));
  const char* invalidCommands[] = {
      "TIME 2026-02-29 12:00:00", "TIME 2026-04-31 12:00:00",
      "TIME 1999-12-31 12:00:00", "TIME 2100-01-01 12:00:00",
      "TIME 2026-01-01 24:00:00", "TIME 2026-01-01 12:60:00",
      "TIME 2026-01-01 12:00:60", "TIME 2026/01/01 12:00:00",
      "TIME 202x-01-01 12:00:00", "TIME 2026-00-01 12:00:00",
      "TIME 2026-01-00 12:00:00", "TIME 2026-01-01 12:00:00 extra"};
  for (const char* command : invalidCommands) {
    TEST_ASSERT_FALSE(parseTimeCommand(command, parsed));
  }
}

void test_explicit_time_command_starts_clock_and_protects_writes() {
  rtc.running = false;
  setup();
  Serial.input = "TIME 2028-02-29 06:07:08\r\n";
  sampleAt(0, 25.0f);
  TEST_ASSERT_EQUAL_UINT(1, rtc.setTimeCount);
  TEST_ASSERT_TRUE(rtc.running);
  TEST_ASSERT_TRUE(rtc.protectedWrites);
  TEST_ASSERT_EQUAL_UINT8(DS1302Tcr_Disabled, rtc.charger);
  TEST_ASSERT_EQUAL_STRING("06:07:08", clockText);
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "RTC time set"));
}

void test_invalid_and_overlong_serial_commands_do_not_write_clock() {
  setup();
  Serial.input = "TIME 2026-02-30 12:00:00\n";
  serviceSerial();
  Serial.input = std::string(60, 'x') + "\n";
  for (uint8_t pass = 0; pass < 4 && Serial.available(); ++pass) serviceSerial();
  TEST_ASSERT_EQUAL_INT(0, Serial.available());
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
  Serial.input = "TIME 2026-10-06 12:00:00\n";
  serviceSerial();
  TEST_ASSERT_EQUAL_UINT(1, rtc.setTimeCount);
}

void test_failed_time_write_is_reported() {
  setup();
  rtc.acceptWrites = false;
  Serial.input = "TIME 2026-10-06 13:00:00\n";
  serviceSerial();
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "Error: RTC write failed"));
}

void test_clock_read_is_after_temperature_conversion() {
  setup();
  sensors.conversionMs = 750;
  loop();
  TEST_ASSERT_FALSE(sensors.waitForConversion);
  TEST_ASSERT_EQUAL_UINT32(0, mockMillis);
  TEST_ASSERT_EQUAL_UINT(0, sensors.readCount);
  mockMillis = 749;
  loop();
  TEST_ASSERT_EQUAL_UINT(0, sensors.readCount);
  mockMillis = 750;
  loop();
  TEST_ASSERT_TRUE(clockValid);
  TEST_ASSERT_EQUAL_UINT32(750, mockMillis);
  TEST_ASSERT_EQUAL_UINT32(750, rtc.lastReadMs);
}

void test_ui_bounds_and_yellow_header_positions() {
  setup();
  const float readings[] = {-55.0f, 125.0f, 85.0f, NAN};
  unsigned long timestamp = 0;
  for (const float reading : readings) {
    sampleAt(timestamp, reading);
    timestamp += 1000;
    TEST_ASSERT_TRUE(display.geometryValid);
    bool foundClock = false;
    bool foundDate = false;
    for (const auto& draw : display.draws) {
      if (draw.text == clockText) {
        TEST_ASSERT_EQUAL_UINT16(12, draw.row);
        foundClock = true;
      }
      if (draw.text == dateText) {
        TEST_ASSERT_EQUAL_UINT16(11, draw.row);
        foundDate = true;
      }
      if (draw.font == u8g2_font_logisoso24_tn) {
        TEST_ASSERT_EQUAL_UINT16(43, draw.row);
      }
    }
    TEST_ASSERT_TRUE(foundClock);
    TEST_ASSERT_TRUE(foundDate);
  }
}

void test_clock_tick_triggers_frame_with_unchanged_temperature() {
  setup();
  sampleAt(0, 25.0f);
  rtc.time = RtcDateTime(2026, 10, 6, 12, 34, 57);
  sampleAt(1000, 25.0f);
  TEST_ASSERT_EQUAL_UINT(2, display.frameCount);
  TEST_ASSERT_EQUAL_STRING("12:34:57", displayedClock);
}

void test_clock_keeps_updating_when_temperature_sensor_is_missing() {
  sensors.present = false;
  setup();
  sampleAt(0, 25.0f);
  rtc.time = RtcDateTime(2026, 10, 6, 12, 34, 57);
  sampleAt(1000, 25.0f);
  TEST_ASSERT_EQUAL_STRING("12:34:57", displayedClock);
  TEST_ASSERT_EQUAL_STRING("SENSOR ERROR", textAt(7));
  TEST_ASSERT_EQUAL_STRING(" --.-", textAt(2));
}

void test_clock_recovery_and_charge_register_verification() {
  setup();
  rtc.running = false;
  sampleAt(0, 25.0f);
  TEST_ASSERT_FALSE(clockValid);
  TEST_ASSERT_EQUAL_UINT(1, rtc.chargerReadCount);
  rtc.running = true;
  sampleAt(1000, 25.0f);
  TEST_ASSERT_TRUE(clockValid);
  TEST_ASSERT_EQUAL_UINT(2, rtc.chargerReadCount);
  rtc.charger = 0xA5;
  sampleAt(2000, 25.0f);
  TEST_ASSERT_EQUAL_UINT8(DS1302Tcr_Disabled, rtc.charger);
  TEST_ASSERT_TRUE(rtc.protectedWrites);
  TEST_ASSERT_EQUAL_UINT(4, rtc.chargerReadCount);
  rtc.charger = 0xA5;
  rtc.acceptWrites = false;
  sampleAt(3000, 25.0f);
  TEST_ASSERT_FALSE(clockValid);
  TEST_ASSERT_EQUAL_UINT(6, rtc.chargerReadCount);
  TEST_ASSERT_EQUAL_STRING("--:--:--", clockText);
  TEST_ASSERT_EQUAL_STRING("SENSOR OK", textAt(7));
}

void test_rtc_pin_mapping() {
  TEST_ASSERT_EQUAL_UINT8(4, rtcWire.dataPin);
  TEST_ASSERT_EQUAL_UINT8(3, rtcWire.clockPin);
  TEST_ASSERT_EQUAL_UINT8(5, rtcWire.enablePin);
}

void buttonStateAt(uint32_t timestamp, uint8_t pin, bool pressed) {
  mockMillis = timestamp;
  mockPinValues[pin] = pressed ? LOW : HIGH;
  serviceButtons();
}

void enterEditor() {
  buttonStateAt(0, 6, true);
  buttonStateAt(30, 6, true);
  buttonStateAt(1030, 6, true);
  buttonStateAt(1040, 6, false);
  buttonStateAt(1070, 6, false);
}

void test_buttons_enter_without_writing_and_long_release_does_not_advance() {
  setup();
  enterEditor();
  TEST_ASSERT_TRUE(editorActive);
  TEST_ASSERT_EQUAL_UINT8(0, selectedField);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
  for (const uint8_t pin : BUTTON_PINS) TEST_ASSERT_EQUAL_UINT8(INPUT_PULLUP, mockPinModes[pin]);
}

void test_button_bounce_is_ignored_and_increment_is_once_per_press() {
  setup();
  enterEditor();
  const uint8_t hour = editDraft.Hour();
  buttonStateAt(1100, 7, true);
  buttonStateAt(1110, 7, false);
  buttonStateAt(1120, 7, true);
  buttonStateAt(1149, 7, true);
  TEST_ASSERT_EQUAL_UINT8(hour, editDraft.Hour());
  buttonStateAt(1150, 7, true);
  buttonStateAt(2150, 7, true);
  TEST_ASSERT_EQUAL_UINT8(hour + 1, editDraft.Hour());
}

void test_short_set_cycles_all_fields() {
  setup();
  enterEditor();
  uint32_t timestamp = 1100;
  for (uint8_t field = 1; field <= 5; ++field) {
    buttonStateAt(timestamp, 6, true);
    buttonStateAt(timestamp + 30, 6, true);
    buttonStateAt(timestamp + 60, 6, false);
    buttonStateAt(timestamp + 90, 6, false);
    TEST_ASSERT_EQUAL_UINT8(field % 5, selectedField);
    timestamp += 100;
  }
}

void test_draft_wraps_fields_and_clamps_calendar_days() {
  setup();
  enterEditor();
  editDraft = RtcDateTime(2028, 2, 29, 23, 59, 0);
  selectedField = EDIT_HOUR;
  adjustDraft(true);
  TEST_ASSERT_EQUAL_UINT8(0, editDraft.Hour());
  selectedField = EDIT_MINUTE;
  adjustDraft(true);
  TEST_ASSERT_EQUAL_UINT8(0, editDraft.Minute());
  selectedField = EDIT_YEAR;
  adjustDraft(true);
  TEST_ASSERT_EQUAL_UINT16(2029, editDraft.Year());
  TEST_ASSERT_EQUAL_UINT8(28, editDraft.Day());
  editDraft = RtcDateTime(2099, 1, 31, 0, 0, 0);
  selectedField = EDIT_MONTH;
  adjustDraft(true);
  TEST_ASSERT_EQUAL_UINT8(28, editDraft.Day());
  selectedField = EDIT_YEAR;
  adjustDraft(true);
  TEST_ASSERT_EQUAL_UINT16(2000, editDraft.Year());
  adjustDraft(false);
  TEST_ASSERT_EQUAL_UINT16(2099, editDraft.Year());
}

void test_editor_timeout_cancels_without_writing() {
  setup();
  enterEditor();
  adjustDraft(true);
  mockMillis = 31030;
  serviceButtons();
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

void test_editor_save_resets_seconds_and_verifies_safety() {
  setup();
  enterEditor();
  adjustDraft(true);
  handleSetHold();
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_EQUAL_UINT(1, rtc.setTimeCount);
  TEST_ASSERT_EQUAL_UINT8(0, rtc.time.Second());
  TEST_ASSERT_EQUAL_UINT8(DS1302Tcr_Disabled, rtc.charger);
  TEST_ASSERT_TRUE(rtc.protectedWrites);
}

void test_editor_failed_save_keeps_draft_and_rejects_serial_write() {
  setup();
  enterEditor();
  adjustDraft(true);
  rtc.acceptWrites = false;
  handleSetHold();
  TEST_ASSERT_TRUE(editorActive);
  TEST_ASSERT_TRUE(editorSaveError);
  Serial.input = "TIME 2026-10-06 13:00:00\n";
  serviceSerial();
  TEST_ASSERT_EQUAL_UINT(1, rtc.setTimeCount);
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "clock edit active"));
}

void test_display_loss_cancels_editor_and_invalid_clock_has_placeholder() {
  setup();
  rtc.time = RtcDateTime(2026, 0, 0, 0, 0, 0);
  enterEditor();
  TEST_ASSERT_EQUAL_UINT16(2000, editDraft.Year());
  TEST_ASSERT_TRUE(validClockDate(editDraft));
  disableDisplay();
  TEST_ASSERT_FALSE(editorActive);
  handleSetHold();
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

void test_short_press_is_processed_during_pending_conversion() {
  setup();
  loop();
  TEST_ASSERT_TRUE(conversionPending);
  mockPinValues[6] = LOW;
  mockMillis = 20;
  loop();
  mockMillis = 50;
  loop();
  mockMillis = 1050;
  loop();
  TEST_ASSERT_TRUE(editorActive);
  mockPinValues[6] = HIGH;
  mockMillis = 1060;
  loop();
  mockMillis = 1090;
  loop();
  TEST_ASSERT_TRUE(conversionPending);
  const unsigned int previousReads = sensors.readCount;
  const uint8_t hour = editDraft.Hour();
  mockPinValues[7] = LOW;
  mockMillis = 1100;
  loop();
  mockMillis = 1130;
  loop();
  mockPinValues[7] = HIGH;
  mockMillis = 1140;
  loop();
  mockMillis = 1170;
  loop();
  TEST_ASSERT_EQUAL_UINT8(hour + 1, editDraft.Hour());
  TEST_ASSERT_EQUAL_UINT(previousReads, sensors.readCount);
  TEST_ASSERT_TRUE(conversionPending);
  TEST_ASSERT_EQUAL_UINT32(1170, mockMillis);
}

void test_long_set_saves_once_and_release_does_not_reenter() {
  setup();
  enterEditor();
  buttonStateAt(1100, 6, true);
  buttonStateAt(1130, 6, true);
  buttonStateAt(2129, 6, true);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
  buttonStateAt(2130, 6, true);
  buttonStateAt(4130, 6, true);
  buttonStateAt(4140, 6, false);
  buttonStateAt(4170, 6, false);
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_EQUAL_UINT(1, rtc.setTimeCount);
}

void test_simultaneous_adjustment_buttons_are_ignored() {
  setup();
  enterEditor();
  const uint8_t hour = editDraft.Hour();
  mockPinValues[7] = LOW;
  mockPinValues[8] = LOW;
  mockMillis = 1100;
  serviceButtons();
  mockMillis = 1130;
  serviceButtons();
  TEST_ASSERT_EQUAL_UINT8(hour, editDraft.Hour());
}

void test_editor_defers_discovery_until_editing_finishes() {
  sensors.present = false;
  setup();
  enterEditor();
  sensors.present = true;
  sampleAt(5000, 25.0f);
  TEST_ASSERT_TRUE(editorActive);
  TEST_ASSERT_NOT_NULL(std::strstr(Serial.output.c_str(), "Draft: "));
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
  TEST_ASSERT_EQUAL_UINT(1, sensors.beginCount);
  TEST_ASSERT_FALSE(hasValidReading);
  handleSetHold();
  sampleAt(6000, 25.0f);
  TEST_ASSERT_EQUAL_UINT(2, sensors.beginCount);
  TEST_ASSERT_TRUE(hasValidReading);
}

void test_button_hold_and_editor_timeout_across_rollover() {
  mockMillis = UINT32_MAX - 500;
  setup();
  buttonStateAt(UINT32_MAX - 500, 6, true);
  buttonStateAt(UINT32_MAX - 470, 6, true);
  buttonStateAt(528, 6, true);
  TEST_ASSERT_FALSE(editorActive);
  buttonStateAt(529, 6, true);
  TEST_ASSERT_TRUE(editorActive);
  buttonStateAt(540, 6, false);
  buttonStateAt(570, 6, false);
  mockMillis = 30528;
  serviceButtons();
  TEST_ASSERT_TRUE(editorActive);
  mockMillis = 30529;
  serviceButtons();
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

void test_selected_field_change_redraws_highlight_immediately() {
  setup();
  enterEditor();
  sampleAt(2000, 25.0f);
  const unsigned int frames = display.frameCount;
  buttonStateAt(2800, 6, true);
  buttonStateAt(2830, 6, true);
  buttonStateAt(2860, 6, false);
  buttonStateAt(2890, 6, false);
  loop();
  TEST_ASSERT_EQUAL_UINT8(1, selectedField);
  TEST_ASSERT_EQUAL_UINT(frames + 1, display.frameCount);
  TEST_ASSERT_EQUAL_STRING("EDIT MIN", textAt(7));
  TEST_ASSERT_TRUE(display.geometryValid);
  bool underlineFound = false;
  for (const auto& line : display.lines) {
    if (line.row == 14 && line.column == 21 && line.width == 14) underlineFound = true;
  }
  TEST_ASSERT_TRUE(underlineFound);
}

void test_rtc_status_change_redraws_unchanged_edit_draft() {
  setup();
  sampleAt(0, 25.0f);
  handleSetHold();
  readClock();
  updateDisplay(true, 25.0f);
  const unsigned int frames = display.frameCount;
  const RtcDateTime originalDraft = editDraft;
  rtc.running = false;
  readClock();
  updateDisplay(true, 25.0f);
  TEST_ASSERT_TRUE(editorActive);
  TEST_ASSERT_EQUAL_UINT32(originalDraft.TotalSeconds(), editDraft.TotalSeconds());
  TEST_ASSERT_EQUAL_UINT(frames + 1, display.frameCount);
  TEST_ASSERT_EQUAL_INT8(0, displayedClockState);
  bool errorShown = false;
  for (const auto& draw : display.draws) {
    if (draw.text == "RTC ERR") errorShown = true;
  }
  TEST_ASSERT_TRUE(errorShown);
  rtc.running = true;
  readClock();
  updateDisplay(true, 25.0f);
  TEST_ASSERT_EQUAL_UINT(frames + 2, display.frameCount);
  TEST_ASSERT_EQUAL_INT8(1, displayedClockState);
}

void test_release_before_hold_deadline_does_not_enter_editor() {
  setup();
  buttonStateAt(0, 6, true);
  buttonStateAt(30, 6, true);
  buttonStateAt(1020, 6, false);
  buttonStateAt(1030, 6, false);
  TEST_ASSERT_FALSE(editorActive);
  buttonStateAt(1050, 6, false);
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

void test_release_before_hold_deadline_does_not_save_editor() {
  setup();
  enterEditor();
  buttonStateAt(1100, 6, true);
  buttonStateAt(1130, 6, true);
  buttonStateAt(2120, 6, false);
  buttonStateAt(2130, 6, false);
  TEST_ASSERT_TRUE(editorActive);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
  buttonStateAt(2150, 6, false);
  TEST_ASSERT_EQUAL_UINT8(1, selectedField);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

void test_button_poll_reads_each_pin_once() {
  setup();
  editorActive = true;
  mockPinValues[7] = LOW;
  buttons[1].rawPressed = true;
  mockMillis = BUTTON_DEBOUNCE_MS;
  serviceButtons();
  for (const uint8_t pin : BUTTON_PINS) {
    TEST_ASSERT_EQUAL_UINT(1, mockPinReads[pin]);
  }
}

void test_clock_formatting_does_not_access_rtc() {
  const unsigned int reads = rtc.chargerReadCount;
  formatClock(RtcDateTime(2028, 2, 29, 6, 7, 8), true);
  TEST_ASSERT_EQUAL_STRING("06:07:08", clockText);
  TEST_ASSERT_EQUAL_STRING("2028-02-29", dateText);
  formatClock(RtcDateTime(), false);
  TEST_ASSERT_EQUAL_STRING("--:--:--", clockText);
  TEST_ASSERT_EQUAL_STRING("---- -- --", dateText);
  TEST_ASSERT_EQUAL_UINT(reads, rtc.chargerReadCount);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
  TEST_ASSERT_EQUAL_UINT32(0, rtc.lastReadMs);
}

void test_oled_disconnect_before_save_prevents_rtc_write() {
  setup();
  enterEditor();
  adjustDraft(true);
  buttonStateAt(1100, 6, true);
  buttonStateAt(1130, 6, true);
  Wire.present = false;
  buttonStateAt(2130, 6, true);
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

void test_oled_disconnect_before_entry_prevents_editing() {
  setup();
  Wire.present = false;
  enterEditor();
  TEST_ASSERT_FALSE(editorActive);
  TEST_ASSERT_FALSE(displayAvailable);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

void test_fixed_clock_formatting_year_limits_and_null_termination() {
  memset(clockText, 'x', sizeof(clockText));
  memset(dateText, 'x', sizeof(dateText));
  formatClock(RtcDateTime(2000, 1, 1, 0, 0, 0), true);
  TEST_ASSERT_EQUAL_STRING("00:00:00", clockText);
  TEST_ASSERT_EQUAL_STRING("2000-01-01", dateText);
  formatClock(RtcDateTime(2099, 12, 31, 23, 59, 59), true);
  TEST_ASSERT_EQUAL_STRING("23:59:59", clockText);
  TEST_ASSERT_EQUAL_STRING("2099-12-31", dateText);
  TEST_ASSERT_EQUAL_CHAR('\0', clockText[8]);
  TEST_ASSERT_EQUAL_CHAR('\0', dateText[10]);
}

void test_button_event_helper_debounce_and_hold_contract() {
  ButtonState button;
  TEST_ASSERT_TRUE(updateButton(button, true, 0, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, true, 29, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, true, 30, true) == ButtonEvent::PRESS);
  TEST_ASSERT_TRUE(updateButton(button, true, 1029, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, true, 1030, true) == ButtonEvent::HOLD);
  TEST_ASSERT_TRUE(updateButton(button, true, 1100, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, false, 1110, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, false, 1140, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, true, 1150, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, true, 1180, true) == ButtonEvent::PRESS);
  TEST_ASSERT_TRUE(updateButton(button, false, 1200, true) == ButtonEvent::NONE);
  TEST_ASSERT_TRUE(updateButton(button, false, 1230, true) == ButtonEvent::RELEASE);
  for (const uint8_t pin : BUTTON_PINS) TEST_ASSERT_EQUAL_UINT(0, mockPinReads[pin]);
  TEST_ASSERT_EQUAL_UINT(0, rtc.setTimeCount);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_valid_read_updates_extremes_and_serial);
  RUN_TEST(test_invalid_reads_preserve_extremes);
  RUN_TEST(test_sensorless_startup_has_no_fake_extremes);
  RUN_TEST(test_valid_boundaries_and_85_are_accepted);
  RUN_TEST(test_probe_ack_then_data_nack_stops_transfers);
  RUN_TEST(test_initialization_nack_stops_transfers);
  RUN_TEST(test_timeout_stops_transfers);
  RUN_TEST(test_short_write_disables_display);
  RUN_TEST(test_alternative_display_address);
  RUN_TEST(test_absent_display_keeps_serial_working);
  RUN_TEST(test_conversion_time_is_included_in_sample_interval);
  RUN_TEST(test_unchanged_formatted_values_are_not_redrawn);
  RUN_TEST(test_changed_content_redraws_complete_page_buffer_frame);
  RUN_TEST(test_error_transition_redraws_once_and_preserves_extremes);
  RUN_TEST(test_sensor_discovery_recovers_at_bounded_interval);
  RUN_TEST(test_missing_sensor_retries_are_not_per_sample);
  RUN_TEST(test_display_recovery_invalidates_cached_text);
  RUN_TEST(test_displayless_startup_recovers);
  RUN_TEST(test_sample_timer_rollover);
  RUN_TEST(test_recovery_timer_rollover);
  RUN_TEST(test_clock_preserves_saved_time_and_disables_charging);
  RUN_TEST(test_clock_formats_new_year_and_leap_day);
  RUN_TEST(test_invalid_or_stopped_clock_does_not_stop_temperature);
  RUN_TEST(test_time_command_validates_calendar_and_format);
  RUN_TEST(test_explicit_time_command_starts_clock_and_protects_writes);
  RUN_TEST(test_invalid_and_overlong_serial_commands_do_not_write_clock);
  RUN_TEST(test_failed_time_write_is_reported);
  RUN_TEST(test_clock_read_is_after_temperature_conversion);
  RUN_TEST(test_ui_bounds_and_yellow_header_positions);
  RUN_TEST(test_clock_tick_triggers_frame_with_unchanged_temperature);
  RUN_TEST(test_clock_keeps_updating_when_temperature_sensor_is_missing);
  RUN_TEST(test_clock_recovery_and_charge_register_verification);
  RUN_TEST(test_rtc_pin_mapping);
  RUN_TEST(test_buttons_enter_without_writing_and_long_release_does_not_advance);
  RUN_TEST(test_button_bounce_is_ignored_and_increment_is_once_per_press);
  RUN_TEST(test_short_set_cycles_all_fields);
  RUN_TEST(test_draft_wraps_fields_and_clamps_calendar_days);
  RUN_TEST(test_editor_timeout_cancels_without_writing);
  RUN_TEST(test_editor_save_resets_seconds_and_verifies_safety);
  RUN_TEST(test_editor_failed_save_keeps_draft_and_rejects_serial_write);
  RUN_TEST(test_display_loss_cancels_editor_and_invalid_clock_has_placeholder);
  RUN_TEST(test_short_press_is_processed_during_pending_conversion);
  RUN_TEST(test_long_set_saves_once_and_release_does_not_reenter);
  RUN_TEST(test_simultaneous_adjustment_buttons_are_ignored);
  RUN_TEST(test_editor_defers_discovery_until_editing_finishes);
  RUN_TEST(test_button_hold_and_editor_timeout_across_rollover);
  RUN_TEST(test_selected_field_change_redraws_highlight_immediately);
  RUN_TEST(test_rtc_status_change_redraws_unchanged_edit_draft);
  RUN_TEST(test_release_before_hold_deadline_does_not_enter_editor);
  RUN_TEST(test_release_before_hold_deadline_does_not_save_editor);
  RUN_TEST(test_button_poll_reads_each_pin_once);
  RUN_TEST(test_clock_formatting_does_not_access_rtc);
  RUN_TEST(test_oled_disconnect_before_save_prevents_rtc_write);
  RUN_TEST(test_oled_disconnect_before_entry_prevents_editing);
  RUN_TEST(test_fixed_clock_formatting_year_limits_and_null_termination);
  RUN_TEST(test_button_event_helper_debounce_and_hold_contract);
  return UNITY_END();
}
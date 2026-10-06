#pragma once

#include <ThreeWire.h>

constexpr uint8_t DS1302Tcr_Disabled = 0x5C;

class RtcDateTime {
public:
  RtcDateTime(uint16_t year = 2000, uint8_t month = 1, uint8_t day = 1,
              uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0)
      : yearValue(year), monthValue(month), dayValue(day), hourValue(hour),
        minuteValue(minute), secondValue(second) {}
  uint16_t Year() const { return yearValue; }
  uint8_t Month() const { return monthValue; }
  uint8_t Day() const { return dayValue; }
  uint8_t Hour() const { return hourValue; }
  uint8_t Minute() const { return minuteValue; }
  uint8_t Second() const { return secondValue; }
  bool IsValid() const {
    if (monthValue < 1 || monthValue > 12 || dayValue < 1 ||
        hourValue > 23 || minuteValue > 59 || secondValue > 59) return false;
    const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = yearValue % 4 == 0 &&
                      (yearValue % 100 != 0 || yearValue % 400 == 0);
    return dayValue <= monthDays[monthValue - 1] + (monthValue == 2 && leap ? 1 : 0);
  }
  uint32_t TotalSeconds() const {
    uint32_t days = 0;
    const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (uint16_t year = 2000; year < yearValue; ++year) {
      days += year % 4 == 0 ? 366 : 365;
    }
    for (uint8_t month = 1; month < monthValue; ++month) {
      days += monthDays[month - 1] + (month == 2 && yearValue % 4 == 0 ? 1 : 0);
    }
    return (days + dayValue - 1) * 86400 + hourValue * 3600 + minuteValue * 60 + secondValue;
  }

private:
  uint16_t yearValue;
  uint8_t monthValue, dayValue, hourValue, minuteValue, secondValue;
};

template <class WireMethod>
class RtcDS1302 {
public:
  bool running = true;
  bool protectedWrites = false;
  bool acceptWrites = true;
  uint8_t charger = 0;
  mutable unsigned int chargerReadCount = 0;
  unsigned int setTimeCount = 0;
  unsigned long lastReadMs = 0;
  RtcDateTime time = RtcDateTime(2026, 10, 6, 12, 34, 56);
  explicit RtcDS1302(WireMethod&) {}
  void Begin() {}
  void SetIsWriteProtected(bool protect) { protectedWrites = protect; }
  bool GetIsWriteProtected() const { return protectedWrites; }
  void SetTrickleChargeSettings(uint8_t settings) {
    if (acceptWrites && !protectedWrites) charger = settings;
  }
  uint8_t GetTrickleChargeSettings() const {
    ++chargerReadCount;
    return charger;
  }
  RtcDateTime GetDateTime() {
    lastReadMs = mockMillis;
    return time;
  }
  void SetDateTime(const RtcDateTime& dateTime) {
    ++setTimeCount;
    if (acceptWrites && !protectedWrites) time = dateTime;
  }
  bool GetIsRunning() const { return running; }
  void SetIsRunning(bool enabled) {
    if (acceptWrites && !protectedWrites) running = enabled;
  }
};
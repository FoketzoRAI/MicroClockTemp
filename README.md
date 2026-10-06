# Nano Temperature And RTC Monitor

A PlatformIO Arduino project for one externally powered DS18B20 and a classic
ATmega328P Nano. The sketch prints Celsius readings approximately once per second
at 9600 baud, using asynchronous 12-bit conversions. Requests are approximately
one second apart; each result is read after 750 ms while buttons and serial
commands continue to be serviced.
An SSD1306 I2C 128x64 OLED shows a DS1302 real-time clock and date in the yellow
header, centered temperature digits in the blue area, minimum/maximum since
startup, and separate sensor/RTC status. The layout assumes the top 16 pixels
are yellow; colors are fixed by the panel, not software.

U8g2's one-page graphics driver uses a 128-byte page buffer instead of a 1024-byte
framebuffer. A changed time, date, rounded temperature, extreme, or status causes
one full page-buffer frame to be rendered; unchanged content is not retransmitted.
Editing state and selected-field changes also invalidate the displayed frame.
No clear-screen operation is sent between frames. Display values use one decimal
place; serial uses two. Min/max still track every valid reading at the sensor's
resolution. Approximately one-second updates can have conversion and recovery
latency; the design does not promise exactly aligned second transitions.

## Wiring

Disconnect power before changing wiring. Verify the sensor pinout from its
datasheet; probe wire colors are not a reliable pinout.

| DS18B20 | Nano |
| --- | --- |
| VDD | 5V |
| GND | GND |
| DQ | D2 |

Connect a 4.7 kOhm pull-up resistor between DQ and VDD unless the module already
includes one. This project does not use parasite power.

| SSD1306 OLED | Nano |
| --- | --- |
| GND | GND |
| VCC | 5V, for the confirmed 5V-compatible module |
| SDA | A4 |
| SCL | A5 |

| DS1302 RTC (MH-Real-Time Clock Modules-2) | Nano |
| --- | --- |
| VCC | 5V |
| GND | GND |
| CLK | D3 |
| DAT | D4 |
| RST (CE) | D5 |

| Normally-open button | Nano connection |
| --- | --- |
| SET | D6 through button to GND |
| + | D7 through button to GND |
| - | D8 through button to GND |

Each button uses INPUT_PULLUP: released is HIGH and pressed is LOW. No external
pull-up resistor or 5V button connection is needed. For four-leg tactile switches,
identify the two separate contacts; legs on the same contact are already joined.

All three modules share power and ground. The RTC uses three-wire communication,
not I2C; its CLK must not be connected to OLED SCL. Follow the module's printed
pin labels. A CR2032 belongs only in the RTC battery holder, with polarity matching
its markings; it is not a supply for the Nano, sensor, or OLED.

CR2032 is non-rechargeable. The firmware explicitly disables the DS1302's internal
trickle charger before reading or setting the clock and restores write protection.
Clock status also requires reading back the disabled charging setting. Software
cannot disable an external board charging circuit: verify the actual module has
no such path before fitting a CR2032. Initially upload and test with the coin cell
removed, then disconnect power before fitting it. A photo is not an electrical
verification of the board. Do not use firmware that enables RTC charging.

The OLED wiring assumes a module compatible with both 5V power and 5V I2C logic,
not a bare SSD1306 controller or a 3.3V-only module.

The firmware probes I2C address 0x3C, then 0x3D at startup. With no responding
display, it prints `OLED unavailable; serial only` and continues sensor reads.
It also disables display updates after a failed connection check, short write,
I2C NACK, or timeout. A project-owned U8x8 callback captures transfer failures
and suppresses subsequent transfers after the first fault. I2C waits are limited
to 25 ms per transaction. While the display is unavailable, initialization is
retried approximately every five seconds. Recovery clears the text cache and
redraws all values. Detection of an I2C address does not prove the device is an
SSD1306; only the specified display should be connected at 0x3C or 0x3D.

The configured dependencies resolve to OneWire 2.3.8, DallasTemperature 4.0.6,
U8g2 2.36.18, and Makuna RTC 2.5.0 in the verified build.

## Build And Use

Run these commands from the project root with PlatformIO installed:

```powershell
pio run -e nanoatmega328
```

After verifying the physical Nano bootloader and identifying its USB serial port,
upload and open the monitor yourself:

```powershell
pio run -e nanoatmega328 -t upload --upload-port COM3
pio device monitor --port COM3 --baud 9600
```

Replace `COM3` with the actual port. Close the monitor before uploading again.
The configured `nanoatmega328` target must match the board's bootloader; newer
bootloader variants may require a different PlatformIO board target.

Example output:

```text
Time: 2026-10-06 12:34:56
Temperature: 23.50 C
```

Missing or invalid readings print `Error: sensor unavailable`. The loop continues
attempting reads. Displaying two decimal places does not imply 0.01 C measurement
accuracy. A completed conversion of 85 C is not automatically rejected.
On a sensor error the OLED shows `--.-` and `SENSOR ERROR`, retaining previous
valid minimum/maximum values. Before the first valid sample, min/max also show
`--.-`. Minimum and maximum reset when the Nano restarts. After failed sensor
reads, rediscovery is attempted approximately every five seconds, including
recovery from a sensorless startup. Successful recovery preserves valid min/max
values from earlier in the same uptime. Discovery, display initialization, and
fault handling can temporarily extend the normal one-second sampling interval.

## Set The Clock

The firmware never replaces saved time with compile time or the computer's time
at boot. With the serial port at 9600 baud, send this command with a newline
(LF, CR, or CRLF); replace the example with the actual desired current time:

```text
TIME 2026-10-06 14:30:00
```

An explicit valid serial command or confirmed button save writes date/time and
starts the oscillator. The
accepted range is 2000-2099, with strict calendar validation including leap days.
Successful read-back prints `RTC time set`; a failed write prints
`Error: RTC write failed`. Invalid or overlong commands are rejected. Input uses
a fixed 32-byte buffer; serial servicing is bounded. OLED transfers, discovery,
and fault handling can still briefly delay polling, but temperature conversion
does not block the loop. No time zone, daylight-saving, network
synchronization, or automatic time setting is applied: displayed time is exactly
the RTC's stored time.

A stopped clock, invalid date, or failed charger-setting verification shows
`--:--:--`, a dashed date, and `RTC ERR`; serial prints `Time: unavailable`.
Temperature monitoring continues even when the clock is invalid, and the clock
continues updating when the sensor is unavailable. Validity checks do not prove
physical RTC presence, time accuracy, oscillator progression, or battery health;
DS1302 has no I2C-style acknowledgement. Runtime reads are checked each sample.

## Button Clock Editor

- Hold SET for one second to enter editing; release it before the next action.
- A short SET press/release cycles hour, minute, year, month, day, then hour.
- Press + or - to adjust the underlined field once. Holding does not auto-repeat.
- Hold SET for one second to save. Seconds reset to 00 to allow synchronization
	against a reference clock. A long press does not also advance the selected field.
- Thirty seconds without an accepted action cancels without writing date/time;
	timeout is postponed while SET is held.

The header shows a draft, not a ticking live clock, while editing. The footer
shows the selected field. Serial timestamps are labeled `Draft:` until editing
ends. Field changes never write date/time; the shared write helper validates the
calendar, disables charging, restores protection, and verifies the saved time.
Year range is 2000-2099; fields wrap within range and month/year changes clamp an
invalid day to the last day of the resulting month.

A stopped clock with valid calendar fields can provide the draft. Invalid RTC
data uses 2000-01-01 00:00:00 as an explicit placeholder; nothing is saved until
confirmed. A failed save shows `SAVE ERR` and keeps the draft for another attempt.
Serial TIME commands are rejected while editing. OLED failure cancels the editor;
entering or saving an edit performs a fresh, timeout-bounded OLED address probe.
If that probe fails, editing is cancelled without writing RTC time. Simultaneous + and - presses,
or adjustment presses while SET is down, are ignored. Buttons use 30 ms debounce.

Temperature sampling continues during editing. Blocking sensor rediscovery and
OLED reinitialization are deferred until editing ends; existing error reporting
and five-second recovery remain active outside editing. All button, hold, timeout,
sample, and conversion timers use rollover-safe unsigned comparisons.

## Host Tests

Install a native GCC/G++ toolchain and make both executables available on PATH.
On this Windows machine they are in `C:\msys64\ucrt64\bin`. The native build uses
static linking to avoid compiler runtime DLL lookup problems. The existing
native environment is host-only and does not upload or open serial ports:

```powershell
pio test -e native -f test_ds18b20_monitor
```

In VS Code this is also available under PlatformIO Project Tasks > native >
Advanced > Test. Unity compiles the real sketch with test-only Arduino, sensor,
Wire, U8g2/U8x8, ThreeWire, and RTC mocks. The Nano environment excludes this suite and builds against
the real hardware libraries; `test_build_src = no` avoids compiling the sketch
twice in the host test executable.

The suite covers valid/invalid readings, min/max, formatting boundaries, sample
timing, serial-only operation, alternate display address, ACK followed by data
NACK, initialization failure, short writes, timeouts, unchanged-frame suppression,
sensor/display recovery, cache reset after recovery, and 32-bit timer rollover.
RTC tests cover saved-time preservation, disabling charging and write protection,
calendar boundary formatting, invalid/stopped clocks, strict time-command parsing,
serial overflow, write verification, and post-conversion RTC reads. UI tests check
header coordinates and approximate text bounds with mocked font widths, clock-only
refreshes, and independence from temperature faults. They do not rasterize real
font glyphs or prove physical color-band alignment.
These mocks verify application behavior, not the physical protocols or the real
libraries' runtime behavior.

Button tests cover input pull-ups, bounce filtering, one increment per press,
field selection and calendar clamping, timeout without writes, safe save and
failed-save handling, serial conflicts, display-loss cancellation, placeholder
dates, long-press/release semantics, simultaneous presses, deferred recovery,
timer rollover, and selected-field redraw. An integration test injects a short
press while conversion is pending and checks it is processed before the sensor
result is read. The conversion test checks no read occurs before 750 ms.

## Verification

The Nano build checks compilation, not wiring or sensor behavior. On hardware,
check several readings, roughly one-second cadence, and response to gentle
warming. With power disconnected, remove the sensor, restart, and check the error
output. Power down again before reconnecting, then restart and check recovery.
Verify the OLED is readable, its rounded temperature matches serial output, and
min/max track valid samples without changing on sensor errors. Test both a
sensorless startup and a displayless startup. After removing the OLED with power
off, restart and confirm serial monitoring still works. The OLED's position,
I2C address, electrical compatibility, and runtime behavior require hardware
verification; compilation and host tests alone do not establish them. Automatic
recovery without restarting the Nano requires a safe, explicitly approved fault
test arrangement. Do not reconnect unknown leads live; power down before normal
wiring changes. Also verify longer-duration stability and compare against a
trusted temperature reference when measurement accuracy matters.

On the actual OLED confirm the clock and date remain within the yellow band and
the longest temperature/error strings do not overlap. Verify the clock advances
across midnight, survives Nano resets, and retains time during main-power loss
with the verified CR2032 arrangement. Compare time against a trusted reference
over a longer duration. Check stopped/invalid clock handling using an approved
safe arrangement; do not short pins or disconnect unknown leads live.
Check real button responsiveness during conversions, accidental double presses,
all five underlines, failed-save feedback, and cancellation. Existing tests use
mocks and cannot prove switch wiring, bounce characteristics, or real font bounds.
The Nano build is close to its flash limit; recheck memory after future features.
---
name: hardware-check
description: "Prepare and interpret Arduino hardware verification, sensor wiring, serial output, and fault tests. Use when checking DS18B20 readings, validating a physical device, or identifying what a build cannot prove."
---

# Hardware Verification

1. Read the firmware's pins, baud rate, timing, error handling, and board configuration. Establish whether hardware access or uploading has been explicitly authorized; otherwise provide a plan only.
2. Before powering or uploading, verify the physical board/bootloader, sensor pinout from its datasheet, supply voltage, common ground, and required pull-ups. Do not infer probe wire functions solely from colors.
3. For the current externally powered DS18B20 setup, check VDD to Nano 5V, GND to GND, DQ to the configured pin (currently D2), and a 4.7 kOhm pull-up from DQ to VDD unless the module already provides a suitable one. Do not assume parasite power is supported by this wiring or asynchronous code.
4. If authorized to upload, identify the correct device port and target first. Stop on ambiguous device identity. Report upload results separately from runtime results.
5. If authorized to monitor, use the configured baud rate (currently 9600), account for the Nano possibly resetting when the serial port opens, and capture timestamped readings over several samples. Close the monitor before any later upload and after the check.
6. Check normal behavior against acceptance criteria: readable units/format, expected sampling cadence, plausible readings, and a gentle warming/cooling response within sensor ratings. Compare with a trusted reference when accuracy matters; plausible output alone is not calibration.
7. Plan safe missing-sensor and recovery checks. Power down before changing wiring. Never short supply rails or reconnect unknown leads live. If live disconnection/reconnection behavior matters, require a safe, explicitly approved test arrangement.
8. Investigate repeated 85 C readings as a possible uncompleted conversion or reset symptom, not an automatic error: 85 C can also be a valid measurement. Treat the library's -127 C sentinel as a read failure rather than a real temperature.
9. Check power-cycle startup, expected recovery after reconnect/restart, and longer-duration stability if relevant. Report failed recovery without silently changing the firmware.

## Output

Provide a compact checklist with expected observations and actual evidence. Label each item passed, failed, or pending. Distinguish compilation, upload, serial communication, sensor response, and measurement accuracy. Never invent observations when no physical test was performed.
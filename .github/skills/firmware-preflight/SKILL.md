---
name: firmware-preflight
description: "Check Arduino and PlatformIO requirements before implementing firmware. Use when planning a firmware change, selecting pins, setting sampling rates, or deciding what must be verified."
---

# Firmware Preflight

1. Read the requested behavior, platformio.ini, and the smallest relevant source or test. Do not map the entire project.
2. Identify the active environment, MCU, framework, declared dependencies, and serial baud rate. Distinguish the configured board from physically verified hardware, including Nano bootloader variants.
3. Identify input/output pins, sensor supply voltage, pull-ups, sensor count, and expected electrical interface. Ask only for unknowns that prevent a safe implementation; otherwise state conservative assumptions.
4. Define observable acceptance criteria: output format and units, sampling interval, acceptable responsiveness, and behavior on missing or invalid data.
5. State one falsifiable local hypothesis and the cheapest discriminating check. Reuse nearby tests and known commands; do not introduce a broad test suite without need.
6. Choose the smallest implementation and a proven library where appropriate. Consider RAM/flash constraints, timing, and compatibility before adding dependencies.

## Output

Report the controlling code, assumptions, acceptance criteria, and focused implementation/check sequence. Planning alone is not verification. Respect the current agent's tool restrictions; do not upload or operate hardware without explicit permission.
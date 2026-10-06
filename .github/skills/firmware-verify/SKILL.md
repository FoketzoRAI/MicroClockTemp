---
name: firmware-verify
description: "Verify Arduino and PlatformIO firmware changes with focused tests, compilation, memory checks, and error-path review. Use after code changes, before declaring completion, or when reviewing whether firmware works."
---

# Firmware Verification

1. Read the acceptance criteria, touched code, platformio.ini, and any relevant existing test. Inspect the changed behavior rather than unrelated modules.
2. Run the cheapest behavior-scoped check first. If it fails, repair the same slice and rerun it before widening scope. Do not fix unrelated failures.
3. Inspect actual test files; a test directory alone is not coverage. For an existing host/native environment, run only the relevant suite with `pio test -e <environment> -f <suite>`. Confirm that the environment is host-only first: embedded test commands can upload and operate hardware. Do not invent a native environment just to claim testing.
4. Build the affected environment with `pio run -e <environment>` from the project root. Reuse a successful build from the current change if source, configuration, and dependencies have not changed since. Do not clean or rebuild everything routinely.
5. Record compiler warnings relevant to the change, resolved dependency versions when relevant, and flash/RAM usage from the build. Flag exceeded limits and explain that static RAM usage excludes runtime stack and heap requirements.
6. Review applicable risks: library error sentinels, missing devices, blocking conversions, bounded waits/retries, timer rollover, precision versus accuracy, and serial format/baud consistency. Skip irrelevant checklist items.
7. Check editor diagnostics when available and inspect the diff for unintended edits. Do not treat a clean diff or diagnostics as substitutes for executable checks.
8. Separate software evidence from pending hardware checks. Use hardware-check for the device test plan. Never upload or open serial ports without explicit permission.

## Output

Report checks as passed, failed, or not run, with exact commands and meaningful results. Include memory usage, remaining risks, and unverified requirements. With no executable tests, explicitly state that behavior was reviewed but not automatically tested. Do not claim hardware success from compilation.
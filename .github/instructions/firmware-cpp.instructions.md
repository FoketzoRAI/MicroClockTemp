---
description: "C++ style and resource-safety standards for Arduino firmware and PlatformIO tests. Use when creating or modifying firmware source."
applyTo: "src/**/*.cpp,src/**/*.h,src/**/*.hpp,include/**/*.h,include/**/*.hpp,lib/**/*.cpp,lib/**/*.h,lib/**/*.hpp,test/**/*.cpp,test/**/*.h"
---

# Firmware C++ Standards

- Preserve nearby style. For new code, use two-space indentation, braces on the opening line, and descriptive names: lowerCamelCase for functions/variables, UpperCamelCase for types, and UPPER_SNAKE_CASE for constants.
- Include each directly used dependency. Use const or constexpr for fixed pins, intervals, and configuration. Name units explicitly, such as temperatureC and intervalMs.
- Use fixed-width integers for bounded values where appropriate. Use unsigned long or uint32_t for millis() timestamps; compare elapsed time with unsigned subtraction so rollover is handled.
- Keep setup() for initialization and loop() for repeated work. Keep functions focused and avoid speculative abstractions or unnecessary globals.
- Prefer fixed-size buffers and avoid heap allocation or repeated Arduino String concatenation on small AVR boards. Use F() for fixed Serial messages on AVR.
- Check hardware/library error results before using data. Treat valid extreme readings separately from error sentinels; do not substitute a plausible temperature for a failed read.
- Distinguish sensor resolution, display precision, and measurement accuracy. Choose resolution and sampling intervals for the actual requirement.
- Use nonblocking scheduling when other tasks must remain responsive. A blocking conversion or delay is acceptable for a single-purpose monitor if its latency is stated.
- Bound retry loops and waits. Keep interrupt handlers short; do not perform blocking sensor reads or Serial output inside them.
- Avoid unnecessary casts, macros, one-letter names, and comments that merely narrate statements. Add comments only for non-obvious constraints or hardware behavior.
- Separate pure logic from hardware access when it enables useful tests without excessive restructuring. Test boundary values, error paths, and timer rollover when applicable.
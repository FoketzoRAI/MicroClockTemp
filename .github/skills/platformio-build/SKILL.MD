---
name: platformio-build
description: "Build PlatformIO firmware and diagnose compiler errors. Use when asked to build firmware or fix a PlatformIO build."
---

# PlatformIO Build

1. Read platformio.ini to identify the target environment.
2. Run `pio run` from the project root.
3. If the build fails, identify the first meaningful error.
4. Read the relevant source before proposing a fix.
5. Make the smallest relevant change and build again.
6. Report the result and any remaining errors.

Do not upload firmware unless explicitly requested.
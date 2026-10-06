# Project Standards

- This is an Arduino firmware project managed by PlatformIO. Treat platformio.ini as the source of truth for board, framework, dependencies, and serial speed.
- Keep source in src/, shared declarations in include/, reusable project libraries in lib/, and tests in test/. Add modules only when they remove meaningful complexity.
- Before editing, identify the requested behavior, its controlling code, and the cheapest check that could disprove the proposed fix. State hardware assumptions when they are not verified.
- Preserve user changes. Keep edits focused; do not refactor unrelated code or change board targets, pins, baud rates, or dependencies without a task-related reason.
- Reuse established libraries for hardware protocols. Declare dependencies in platformio.ini; do not edit installed libraries or generated .pio/ files. Use bounded versions and report resolved versions when relevant.
- Prefer incremental builds of the affected environment. Do not clean builds, update packages, or build unrelated environments by default.
- After a substantive edit, run the narrowest relevant executable check before expanding scope, then build the affected firmware if it changed.
- Reuse existing test files. Add focused regression tests for meaningful logic when practical; avoid introducing a framework for a trivial change.
- Do not upload firmware, open serial ports, or operate hardware unless explicitly requested. A build skill does not grant tools or override an agent's restrictions.
- Report compilation, automated tests, and physical-device verification separately. Never claim that a successful build proves runtime behavior or electrical correctness.
- State remaining assumptions, missing tests, and unverified hardware requirements. Stop after relevant checks pass; do not add checks only for reassurance.

## Workflow

1. Use firmware-preflight to establish requirements and a focused validation plan.
2. Implement one small, testable change.
3. Use platformio-build for compile failures or a normal build, and firmware-verify for broader completion checks. Reuse a build result if no relevant inputs changed.
4. Use hardware-check to prepare or interpret physical-device checks. Report pending checks when hardware access is not authorized or available.

## Agent Roles

- Firmware Planner: read-only requirements and implementation brief.
- Firmware Developer: focused implementation and immediate software validation.
- Firmware Verifier: independent checks; reports failures without applying fixes.
- Firmware Reviewer: read-only correctness review, with findings and evidence gaps.
- Firmware Tutor: read-only explanations and learning questions.
- Recommended sequence: Planner -> Developer -> Verifier -> Reviewer. Return failed checks or concrete findings to Developer, then rerun the affected verification. Small tasks can start directly with Developer.
- Choose agents manually from the chat selector and provide the relevant brief/results when starting a new conversation. These roles do not automatically orchestrate each other, and skills do not override role restrictions.

## Completion Report

Summarize what changed, the exact checks and results, flash/RAM usage when available, and what still requires hardware testing.
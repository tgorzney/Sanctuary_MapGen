---
name: feedback-no-click-sim-tests
description: "Never run GL-backed / click-simulation test binaries in this project — they interrupt the user's own active Visual Studio session."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 839ff35d-4d20-422c-ace0-33b4c5629979
  modified: 2026-08-30T18:39:49.778Z
---

Never run a diagnostic test that simulates actual clicks/drags (GL-backed imgui tests driving real
`io.AddMousePosEvent`/`io.AddKeyEvent` press/drag/release sequences), even to gather evidence for a
bug investigation.

**Why:** the user explicitly said "never run an actual click sim as it interrupts my current
activities" — they run/debug the app themselves via Visual Studio's Local Windows Debugger, and
launching a GL-backed test binary apparently steals focus or otherwise disrupts that session.

**How to apply:** stick to pure static code reading (Read/Grep/Glob) and headless/non-GL unit tests
(pure data-structure tests with no imgui window, no `io.AddMousePosEvent`) for diagnosis. Building
normally (`cmake --build`) is fine — it's *running* a windowed/GL test binary that's off-limits. If a
bug can't be diagnosed from code alone and would normally call for a live-frame test to get real
evidence, say so explicitly and ask the user rather than running one. Related: [[feedback_no_manual_testing]]
(never launch/click through the app manually) — this extends the same "don't touch the running app"
boundary to automated GL-backed tests too.

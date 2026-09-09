---
name: feedback-no-manual-testing
description: User never wants Claude to personally run/interact with the app to test UI or code behavior
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 6510ee7d-78d7-40f7-85cf-ecf264205dba
  modified: 2026-08-18T06:49:45.748Z
---

Never attempt to personally run or interact with the built application to test code (e.g.
clicking through a UI flow, launching the app, driving a dialog). This applies even when a
change (like a new confirm-dialog widget) would benefit from an interactive click-through to
fully verify.

**Why:** explicit user instruction ("NEVER try to perform an actual run code test yourself"),
given right after Claude noted a confirm-dialog popup's actual click sequence hadn't been
interactively verified and offered it as a gap.

**How to apply:** Verification in this project (SanGen Map Generator) happens through automated
test binaries (the `*_IO_Test.exe`/`*_UI_Test.exe` targets built via CMake/Ninja) and direct
source-code reading — never through launching the app and interacting with it live. When a
feature's full correctness would normally call for a manual/interactive check (e.g. "does the
dialog actually pop up and does OK/Cancel work"), say so explicitly as a known gap for the human
to verify themselves, but do not attempt to run the app to check it. This is distinct from
running non-interactive automated test executables, which remains expected and encouraged.

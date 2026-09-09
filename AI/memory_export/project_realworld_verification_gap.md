---
name: project-realworld-verification-gap
description: "Automated 94/94 test-passing claims for STEP24-42 were contradicted by the human's own real-world run of the app"
metadata: 
  node_type: memory
  type: project
  originSessionId: 61de7290-a826-4efd-b0bd-97dca54c5385
  modified: 2026-08-20T16:55:04.532Z
---

After STEP24-42 shipped with independently-verified 94/94 automated tests passing, the human
ran the actual built app themselves and found it "a mess, nowhere near usable" — and a real
`.sanmap` file import (`Pandemonium Isthmus.sanmap`, the exact file STEP24's "never-refuse
import" work was originally motivated by) still did not import correctly.

**Why:** the automated test suite (CMake/ctest `*_IO_Test.exe`/`*_UI_Test.exe` binaries) verifies
unit-level correctness, not real end-to-end usability — a green suite is not proof the app works
for a human. This is a real, demonstrated gap, not a hypothetical one.

**How to apply:** Do not report a feature "done" or "verified" on the strength of test-suite
results alone when the human can run the real app. Prioritize the human's own reproduction
reports over prior "independently verified" session claims — re-verify against the specific
real file/scenario they report broken before trusting old handoff docs' status claims. See
[[feedback-short-responses]] for the response-style rules layered on top of this (the human is
now driving action-by-action, wants tight, verifiable loops, not large batched "verified" claims).

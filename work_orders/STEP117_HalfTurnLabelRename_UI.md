# STEP117 — Rename "Half Turn" symmetry label to "180° Rotation"

**Layer:** UI (display-string only). **Domain:** `PlacementRuleSections_UI.h`'s shared symmetry-axis label table, used by all four placement tabs (Markers/Armies/Props/Areas). **Sequence:** no dependency on other undone work-orders.

Pure display-string rename — zero behavior/semantic change. Confirmed this session: `"Half Turn"` (`src/ui/PlacementRuleSections_UI.h:31`) is definitionally 180-degree rotational symmetry about the map center (`Params::SymmetryAxis::RotateHalfTurn`, `src/params/Symmetry_PARAMS.h:16` — own comment: "180 degrees about the map center (point symmetry)"; consistent usage confirmed in `src/pipeline/SymmetryOrbitQuery_PIPELINE.h:42-44`, `src/proc/Placement_Symmetry_PROC.h:35-37`, `src/proc/Placement_SymmetryOrbit_PROC.h:26`). Human explicitly approved "180° Rotation" as the replacement wording.

## Problem
`placementSymmetryAxisLabels` (`src/ui/PlacementRuleSections_UI.h:30-32`) is the ONE shared label table every placement tab's independent symmetry tick-boxes draw from (`DrawPlacementSymmetryAxes`/`DrawIndependentSymmetryAxes`, consumed at `PlacementRuleSections_UI.cpp:35`, `DrawCheckbox(placementSymmetryAxisLabels[axisIndex], bAxisSet)`). Index 2's label reads `"Half Turn"`, which does not name the axis as clearly as the equivalent degree figure.

Grepped exhaustively for the literal string `"Half Turn"` under `src/`: it appears in exactly ONE production location — `src/ui/PlacementRuleSections_UI.h:31`, inside the five-entry array `{ "Mirror X", "Mirror Z", "Half Turn", "Quarter Turns", "Radial" }`. The only other hits are three COMMENTS in `src/ui/SymmetryTab_UI_Test.cpp:46,58,60` — none of them assert against the literal string; every actual check in that file drives `RotateHalfTurn`/index `2` directly (`SymmetryTab_UI_Test.cpp:48-60`), so no test asserts on the display string and none needs a code change to stay green.

## Fix
`src/ui/PlacementRuleSections_UI.h:31` — one-line change:
```cpp
inline const char* const placementSymmetryAxisLabels[kPlacementSymmetryAxisCount] = {
    "Mirror X", "Mirror Z", "180° Rotation", "Quarter Turns", "Radial"
};
```
No other production file references this string; no signature, index, or bit-mapping changes (`PlacementSymmetryAxisBit`/`IsPlacementSymmetryAxisSet`/`PlacementSymmetryMaskAfterToggle`, lines 34-54, are all index-driven and untouched).

**Optional, non-blocking:** `SymmetryTab_UI_Test.cpp:46,58,60`'s three comments still say "Half Turn" — cosmetic only (comments, not assertions), can be left as-is or refreshed to "180° Rotation" in the same pass at the Coder's discretion; does not affect correctness or test outcome either way.

**Degree-sign glyph coverage — verified safe, resolved (was an open question during drafting).**
Grepped for any custom font/glyph-range setup (`AddFontFromFileTTF`, `GetGlyphRanges*`, `ImFontAtlas`
construction) anywhere in `src/` — none found. Every `io.Fonts` usage in the codebase is test-harness
`GetTexDataAsRGBA32`/`SetTexID` calls against whatever font is already resident; no production code
path restricts the glyph range at all, meaning the app relies on Dear ImGui's default embedded font
build, which uses `GetGlyphRangesDefault()` — Basic Latin + Latin-1 Supplement, U+0020–00FF —
covering U+00B0 (the degree sign, decimal 176). **"180° Rotation" is safe to ship as written; no
fallback wording needed.**

## Out of scope
- Any change to `RotateHalfTurn`'s enum name, bit value, or any PARAMS/PIPELINE/PROC symbol — this ticket is the UI display string only.
- `SymmetryTab_UI_Test.cpp`'s comment text — optional, not required (see Fix).
- Any other symmetry-axis label ("Mirror X", "Mirror Z", "Quarter Turns", "Radial") — untouched.

## Files touched
- `src/ui/PlacementRuleSections_UI.h` — `placementSymmetryAxisLabels[2]` string literal only.

## Verify
No test file asserts against the literal label string today (confirmed above — `SymmetryTab_UI_Test.cpp` drives the bit/index logic only, never the display string), so there is no existing UI-label-array test to extend, and this ticket does not add one: a five-entry `const char*` table has no pure logic to assert against beyond "the array still has 5 non-null entries," which `RunAxisOptionChecks`'s existing index-loop already exercises indirectly. This is a real test-coverage gap (label-array content is unverified by any binary) rather than a fabricated test — noted honestly, not closed here.
- **Manual/visual check only** (per this project's no-manual-testing-by-agents posture, this is the Coder/human's acceptance step, not an agent's): confirm index 2's checkbox reads "180° Rotation" in a live frame with no tofu/missing-glyph box.

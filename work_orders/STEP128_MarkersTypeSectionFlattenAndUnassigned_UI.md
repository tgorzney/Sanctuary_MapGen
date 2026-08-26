# STEP128 — Flatten Ungrouped sections + present-only "(Unassigned)" bucket

**Layer:** UI. **Domain:** `MarkersTab_TypeSections_UI.h/.cpp`, `MarkersTab_ManualLayerRowBody_UI.cpp`,
`MarkersTab_RuleLayers_UI.cpp` (ungrouped rule-layer row, if it draws its own body separately).
**Sequence:** independent — no dependency on any other Round-2 ticket. Land first; gates the
hierarchy re-verification (brief item 6).

Ratifies `work_orders/DESIGN_MarkersUICorrectionRound2_R1.md` items 4 and 5 (reviewed by ARCH, no
subsection needed — recorded in `sangen_arch_pack/INDEX.md`'s narrative).

## Item 5 — flatten "Ungrouped ..." sections

**Confirmed:** `MarkersTab_TypeSections_UI.cpp:77-91` wraps both ungrouped lists in their own
`DrawSectionBegin("Ungrouped Procedural Rules"/"Ungrouped Manual Marker Layers", ...)`. Human's exact
words: "Ungrouped markers get listed individually after all the groups" — plain rows, no enclosing
header/collapse chrome.

**Fix:**
- Delete both `DrawSectionBegin`/`DrawSectionEnd` wrapper pairs in `DrawMarkerTypeSections`.
- Delete `ungroupedProceduralSection`/`ungroupedManualSection` from `MarkerTypeSectionState_UI`
  (`MarkersTab_TypeSections_UI.h:38-42`) — dead once the wrappers are gone.
- `DrawRuleLayerListBody`/`DrawAddMarkerRuleLayerButton`/`DrawManualMarkerLayerListBody` calls stay
  exactly as they are — only the enclosing header goes away.
- Add a plain `ImGui::Separator()` between the Bundle tree and the flat ungrouped rows, and another
  between the ungrouped-procedural rows and the ungrouped-manual rows, for visual separation (no
  state, no interaction).

## Item 4 — "(Unassigned)" bucket becomes present-only + self-service type field

**Confirmed:** `EnumerateMarkerTypeSectionNames`'s `""` bucket is unconditionally appended
(`MarkersTab_TypeSections_UI.cpp:50-56`), unlike every other name which is present-only
(`CollectDistinctNonEmptyTypeName`, `.cpp:24-28`, skips empty strings on the way in).

**Fix:**
- Remove the unconditional `ordered.push_back("")`. Instead, run the exact same presence test every
  other name gets, without the `.empty()` skip: if ANY bundle/ruleLayer/instanceLayer has
  `markerTypeName == ""`, append `""` to `ordered` (still last, after the alphabetical others); if
  none do, the bucket doesn't appear at all.
- Add a conditional free-text "Marker Type" field to ungrouped `MarkerRuleLayer`/`MarkerInstanceLayer`
  rows, drawn ONLY when `layer.markerTypeName.empty()`, at the top of the row's own draw (mirror
  `MarkersTab_BundleNodeBody_UI.cpp:87`'s existing `DrawTextInput("Marker Type", bundle.markerTypeName,
  typeRules)` verbatim — same `TextInputRules`, same soft-validation posture, no new pattern).
  Typing a real name here moves the row to that Type section next frame (a pure consequence of
  `EnumerateMarkerTypeSectionNames`'s existing per-frame enumeration — no extra wiring needed).
- Bundles already have this field (`MarkersTab_BundleNodeBody_UI.cpp:87`) — unaffected by this
  ticket, confirm unconditional-vs-conditional draw isn't accidentally changed there.

## Verify

- `EnumerateMarkerTypeSectionNames`: extend `MarkersTab_TypeSections_UI_Test.cpp`'s existing "all
  legacy-data" fixture-per-empty-typeName check to assert the empty-only-when-nothing-typed case now
  returns `{}` (not `{""}`) when zero bundles/layers exist at all, and returns `{"", ...}` /
  `{..., ""}` correctly when at least one entry is genuinely empty-typed, mixed with named ones.
- New check: a row with `markerTypeName.empty()` renders the free-text field; a row with a non-empty
  `markerTypeName` does not (headless-frame assertion, mirroring
  `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`'s harness).
- New check: typing a name into that field and re-running `EnumerateMarkerTypeSectionNames` on the
  mutated data shows the row's new type taking effect (a plain data-level assertion, no imgui frame
  needed for this half).
- Confirm the Bundle's own existing "Marker Type" field test coverage, if any, stays green — no
  change intended there.
- Existing suites (`MarkersTab_TypeSections_UI_Test`, `MarkersTab_UI_Test`,
  `MarkersTab_Bundles_UI_Test`, `MarkersTab_RuleLayers_UI_Test`, `MarkersTab_ManualLayers_UI_Test`)
  stay green.

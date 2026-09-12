# STEP261 — Unit Placements list editor (Scenarios tab)

**Layer:** UI. **Domain:** new `src/ui/ScenariosTab_DetailUnitPlacements_UI.cpp`,
`src/ui/ScenariosTab_UI.h` (declaration), `src/ui/ScenariosTab_Detail_UI.cpp` (call site).
**Executor:** SanGen Coder. **Sequence:** depends on `STEP260` (the `Params::ScenarioUnitPlacement`
struct must exist first). Implements `ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md`'s
UI-facing follow-up.

## 0. Why

`STEP260` adds `ScenarioBody::unitPlacements` with no editor — authorable only by hand-editing
`.sanmap` JSON. This ticket adds the list editor, following this tab's own established per-domain
split (`ScenariosTab_DetailSpawns_UI.cpp`, `ScenariosTab_DetailAlloys_UI.cpp` are the direct
precedent — each a sibling file called from `DrawScenarioBodyFields`,
`ScenariosTab_Detail_UI.cpp:122-149`).

## 1. New file — `src/ui/ScenariosTab_DetailUnitPlacements_UI.cpp`

Declare in `ScenariosTab_UI.h` alongside the other `DrawScenarioBody*` free functions:

```cpp
void DrawScenarioUnitPlacementsSection(Params::ScenarioBody& body, const std::vector<Params::Army>& armies);
```

Call it from `DrawScenarioBodyFields` (`ScenariosTab_Detail_UI.cpp:148`), immediately after the
existing `DrawScenarioBodyExtendedFields(body, armies);` line, preceded by
`ImGui::SeparatorText("Unit Placements");` matching this function's existing section-heading idiom
(`"Alloys"`/`"Spawns"`/`"Authoring Note"`, same file, `:140-146`).

### Per-row controls

For each `Params::ScenarioUnitPlacement` in `body.unitPlacements`, in a list editor matching the
existing add/remove/reorder row idiom already used by `DrawScenarioSpawnIdsSection`/
`DrawAlloyOverrideList`:

- **Army** — reuse `DrawArmyNameField` verbatim (`ScenariosTab_DetailSpawns_UI.cpp:87`, already
  declared in `ScenariosTab_UI.h`) against `placement.armyName`. Same Combo-over-`armies[i].displayName`
  -> stores `Army::name`, free-text fallback when no armies are authored yet — identical posture to
  the spawn/alloy editors, no new picker logic.
- **Template identifier** — free-text `DrawTextInput`, matching `ScenarioAlloyOverride::markerName`'s
  honest "NOT a validated picker" posture (`ScenariosTab_DetailAlloys_UI.cpp`). No unit-template list
  exists anywhere in this codebase to validate against — do not fabricate one.
- **Position X/Y/Z** — three `DrawSliderScalar` calls with `RealtimeToggle`s using the existing
  `ScenarioWorldPositionRange()` helper (`ScenariosTab_Detail_UI.cpp:46`, `{-8192, 8192, 0}`) — same
  range already used for alloy override positions, do not invent a new range constant.
- **Rotation** — no existing UI in this codebase decomposes an arbitrary quaternion to euler for
  display (confirmed by grep across `src/ui`); the only quaternion-adjacent UI logic is
  `MarkersTab_BundleNodeBody_UI.cpp:49`'s bundle-rotate tool, which builds a quaternion from a single
  yaw-degrees float via `Math::YawQuaternion` but never decomposes one back. Mirror that direction,
  not invent decomposition:
  - Default-collapsed row: one **"Facing (degrees, yaw only)"** slider (0-360, wraps) that writes
    `rotationY`/`rotationW` via `Math::YawQuaternion(angleRadians, 0, yawY, 0, yawW)` (X/Z axis
    components zeroed — pure yaw), same call already used by the bundle-rotate tool.
  - An **"Advanced"** toggle reveals the raw `rotationX`/`Y`/`Z`/`W` four fields directly, for
    values that didn't come from the yaw slider (e.g. imported from a foreign file's non-yaw-only
    rotation, once `STEP264`'s extractor exists — that extractor may only ever produce yaw-only
    values in practice, but the storage and this editor must not assume it).
  - Storage is **always** the quaternion fields; there is no separate stored "degrees" field — the
    slider is a view/write convenience only, matching `ARCH_15_14`'s explicit rejection of a stored
    `facingDegrees` float.

Add/remove/duplicate row buttons follow the same idiom as the existing spawn/alloy list editors —
do not invent a new list-editing pattern for this one section.

## 2. Out of scope

- The Files-tab "Import scenario data" action and any review/assign UI for extracted candidates —
  `STEP265`.
- Any change to `DrawScenarioBodyExtendedFields`, `DrawScenarioSpawnIdsSection`, or
  `DrawAlloyOverrideList`'s own bodies — this ticket only adds a new sibling section and one new call
  site line.
- A validated unit-template picker — no template list exists in this codebase; do not build one as a
  side effect of this ticket.

## 3. Tests

Follow this tab's existing acceptance-test shape (pure-logic extraction where the file already
separates signal-application from drawing, GL-backed only where no such split exists — check the
existing `ScenariosTab_DetailSpawns_UI_Test.cpp`/`ScenariosTab_DetailAlloys_UI_Test.cpp`, if present,
for the exact harness pattern before choosing one):

1. Adding a row appends a default-constructed `ScenarioUnitPlacement` (identity rotation, zero
   position, empty army/template) to `body.unitPlacements`.
2. Editing the yaw slider updates `rotationY`/`rotationW` via `Math::YawQuaternion` and leaves
   `rotationX`/`rotationZ` at 0 (pure yaw, no drift into other axes).
3. Toggling "Advanced" and hand-editing a raw rotation field (e.g. a non-yaw-only value) is preserved
   verbatim — the yaw slider must not silently re-derive/overwrite it on the next frame.
4. Army field reuses `DrawArmyNameField`'s existing behavior unchanged (regression check only, not new
   coverage of that function itself).

## 4. Files touched

**New:** `src/ui/ScenariosTab_DetailUnitPlacements_UI.cpp`, plus its test file.

**Modified:** `src/ui/ScenariosTab_UI.h` (new declaration), `src/ui/ScenariosTab_Detail_UI.cpp` (one
new `ImGui::SeparatorText` + one new call site in `DrawScenarioBodyFields`).

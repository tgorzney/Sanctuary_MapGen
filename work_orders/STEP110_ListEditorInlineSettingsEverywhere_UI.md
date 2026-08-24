# STEP110 — Apply STEP104's inline-settings fix to every other list editor in the UI

**Layer:** UI. **Domain:** every tab with a "list of expandable rows + settings drawn separately
at the bottom" antipattern. **Sequence:** independent per-file; generalizes STEP104
(`work_orders/STEP104_LayerEditorInlineSettingsAndAddButton_UI.md`, committed `1352e6f`), which
fixed exactly this shape for the Heightmap Layer Editor.

**⚠️ Two files are explicitly HELD, NOT in this ticket's scope — do not touch them:**
`src/ui/MarkersTab_ManualLayers_UI.cpp`/`.h` and `src/ui/MarkersTab_ManualInstance_UI.cpp`.
A peer session has drafted (not yet dispatched) STEP106/STEP107 touching these same files
(`DrawSelectedLayer`, `DrawSelectedMarkerInstance`/`DrawMarkerInstanceListButtons`) for unrelated
marker-layer-lock/grid-snap/symmetry features. Coordinate before any future ticket touches either
file — confirmed via direct cross-session communication, not assumed.

## Root problem
STEP104 fixed one instance of this pattern (Heightmap Layer Editor: each `Params::Layer`'s
settings now render inside its own expanded `DraggableList` row, not once at the bottom for
whatever the global `selectedLayerIndex` happened to point at). The SAME shape exists,
independently, in most of the other list-editing tabs in `src/ui/` — confirmed by direct
investigation this session. The shared widget both patterns are built on,
`DraggableList<T>::Render` (`src/ui/DraggableListWidget_UI.h`), already supports correct
inline drawing via its `drawRowBody(int rowIndex)` callback, gated on that row's own
`ImGui::CollapsingHeader` open state (`bExpanded`) — **not** on any `selectedIndex`. Every file
below currently passes a no-op `drawRowBody` and instead draws that item's real settings
separately, once, gated by a `selected...Index`-style state field. This is per-tab bespoke work:
there is no second shared "selected-item settings panel" helper to fix once.

## Fix — the general pattern, apply per file below
For each file: move the call(s) currently made from the bottom-of-stack settings-draw function
into the list's own `drawRowBody` callback, operating on that row's own item (already in scope
inside the callback — no `selected...Index` lookup needed for WHICH item to draw, only whether
existing code elsewhere still needs that state for something else, e.g. row-action button
gating — check before deleting any state field, exactly as STEP104 had to for
`selectedGeoLayerIndex`/`selectedLayerIndex`). Keep whatever `PlacementRuleSections_UI.cpp`
section-widget calls (Gates/Affinities/Transform/Symmetry/TemplatePicker) each file already
makes — this ticket relocates WHERE they're called from, not what they draw.

### File-by-file scope (confirmed by direct investigation; verify exact current line numbers/
signatures before editing, since other tickets may have shifted them since this was drafted)

1. **`src/ui/LayersTab_UI.cpp`** — the OLDER, parallel GeoLayer/Layer list implementation STEP104
   explicitly did not touch (it fixed `LayerEditor_Group_UI.cpp`, a different file). List:
   `DrawGroupBody`/`DrawGeoLayerList` (inner `DraggableList<Params::Layer>::Render`, no-op body).
   Settings: `DrawLayerControls`, called once from `DrawLayersTab`. State: `selectedGeoLayerIndex`/
   `selectedLayerIndex`. One flat noise/blend settings block to relocate (Noise Type, Fractal
   Type, Frequency, Octaves, Gain, Lacunarity, Opacity, Blend Mode, Height Blend Window).

2. **`src/ui/PropsTab_UI.cpp`** (+ `PropsTab_Rules_UI.cpp`, `PlacementRuleSections_UI.cpp`) — List:
   `DrawRuleList` (no-op body). Settings: inline block at the end of `DrawRuleStack`. State:
   `state.selectedRuleIndex` (`PropsTabState`). Sections to relocate: `DrawPropRuleGates`,
   `DrawPropRuleAffinities`, `DrawPlacementGateSection`, `DrawPlacementSymmetryAxes`,
   `DrawPlacementTransformSection`, `DrawPlacementTemplatePicker`, `DrawResolvePropFootprintButton`
   (STEP96).

3. **`src/ui/PropsTab_Decals_UI.cpp`** — decals live entirely inside the Props tab files (no
   separate `DecalsTab_*.cpp`). List: `DrawDecalList` (no-op body). Settings: inline block at the
   end of `DrawDecalRuleStack`. State: `state.selectedRuleIndex` (`DecalRuleStackState`). Sections:
   `DrawDecalGates`, `DrawPlacementGateSection`, `DrawPlacementSymmetryAxes`,
   `DrawPlacementTransformSection`, `DrawPlacementTemplatePicker`.

4. **`src/ui/PropsTab_Manual_UI.cpp`** — List: `DrawLayerList` (no-op body). Settings:
   `DrawSelectedLayer`. State: `state.selectedLayerIndex` (`ManualPropLayersState`). One small
   block: Name / Color / Icon Scale.

5. **`src/ui/PropsTab_ManualDecals_UI.cpp`** — exact mirror of (4) for decal manual layers. List:
   `DrawLayerList` (no-op body). Settings: `DrawSelectedLayer`. State: `state.selectedLayerIndex`
   (`ManualDecalLayersState`). Same Name / Color / Icon Scale block.

6. **`src/ui/ArmiesTab_UI.cpp`** — List: `DrawArmyList` (no-op body). Settings: `DrawArmySettings`,
   called from `DrawArmiesTab`. State: `state.selectedArmyIndex`. One block: Engine ID / Name /
   Alias / Team Color / Faction / Alloys / Energy / Mirror button (STEP75 — verify the Mirror
   button's own click-handling still works correctly once relocated, it's a real action not just
   a display field).

7. **`src/ui/ArmiesTab_Units_UI.cpp`** — **structurally different, bigger lift, flagged
   explicitly.** The list is drawn via `RenderVirtualRows`/`ImGui::Selectable`
   (`VirtualListWidget_UI.h`), NOT `CollapsingHeader`/`DraggableList` — rows are not expandable at
   all today. Settings: `DrawUnitRuleSettings` + `DrawPlacementGateSection`/
   `DrawPlacementSymmetryAxes`/`DrawPlacementTransformSection`/`DrawPlacementTemplatePicker`/
   `DrawResolveUnitFootprintButton`, called from `DrawArmyUnitList`. State:
   `state.selectedRuleIndex` (`ArmyUnitListState`). An inline fix here can't just add a
   `drawRowBody` callback the way the other rows can — it needs the row to become expandable
   first (either convert to `DraggableList`-style rows, or bolt a per-row inline draw onto
   `RenderVirtualRows` some other way). **If this proves substantially harder than the other
   files, it is acceptable to defer this ONE file to its own follow-up ticket** — report that
   explicitly rather than forcing a rushed fix; do not let it block the other 10 files in this
   ticket.

8. **`src/ui/MarkersTab_RuleLayers_UI.cpp`** + **`src/ui/MarkersTab_RuleLayerSettings_UI.cpp`** +
   **`src/ui/MarkersTab_UI.cpp`** — a TWO-TIER version of the same bug (rule layers containing
   rules), the direct twin of what STEP104 fixed for GeoLayers containing Layers. Outer list:
   `DrawRuleLayerListBody`/`DrawMarkerRuleLayerList`. Inner list (nested inside each rule layer's
   expanded row): `DrawRuleLayerBody` (its own `DraggableList<Params::MarkerRule>::Render`,
   no-op body). Layer-tier settings: `DrawSelectedRuleLayerSettings` — its own comment explicitly
   cites the OLD pattern as intentional at the time ("mirroring how DrawLayersTab draws per-layer
   controls BELOW the nested list rather than inside a row body" — STEP80); that citation is now
   stale once this ticket lands, since `DrawLayersTab`'s own pattern (file 1 above) is also being
   fixed in this same ticket. Layer tier: 1 block (Name/Enabled/Hidden/Symmetry). Rule-tier
   settings: inline block in `DrawRuleStack` (`MarkersTab_UI.cpp`). Rule tier: `DrawMarkerRuleGates`,
   `DrawMarkerRuleQuantity`, `DrawMarkerRuleArea`, `DrawMarkerRuleFocus`,
   `DrawPlacementGateSection`, `DrawPlacementTransformSection`, `DrawPlacementTemplatePicker`.
   State: `state.selectedRuleLayerIndex`, `state.selectedRuleIndex` (`MarkersTabState`).
   **Confirmed CLEAR of the peer's held marker-tab files** — this cluster is procedural marker
   RULES, not the manual marker roster (`MarkersTab_Manual_UI.cpp`/`ManualLayers`/`ManualInstance`).

9. **`src/ui/MarkersTab_Manual_UI.cpp`** — the manual marker GROUP list (distinct from the two
   held files, which are the manual LAYER list and the manual INSTANCE editor — confirmed clear).
   List: `DrawMarkerGroupList` (no-op body, comment: "header-only rows: the editor is below").
   Settings: `DrawSelectedMarkerGroupSettings`, called from `DrawMarkerGroupSection`. State:
   `state.selectedGroupIndex` (`ManualMarkersState`). One small block: Name / Resource checkbox.

10. **`src/ui/AreasTab_UI.cpp`** — List: `DrawAreaList` (no-op body, comment: "header-only rows:
    the editor is below"). Settings: `DrawAreaSettings`, called from `DrawAreasTab`. State:
    `state.selectedAreaIndex`. One flat block (no sub-sections): Name / X,Z Position / Width /
    Length / Color / "Set to Map Size" button.

11. **`src/ui/ScenariosTab_Lists_UI.cpp`** + **`src/ui/ScenariosTab_ListMechanics_UI.h`** — TWO
    separate lists sharing ONE `selectedTier`/`selectedIndex` pair (`ScenariosTabState`) — a fix
    must keep the tier discriminator correct per row (a Pattern-tier row's inline settings must
    only draw when THAT row, in THAT list, is expanded — not cross-wired to the Count tier's rows
    just because they share one state struct). Pattern-tier list: `DrawScenarioPatternList`
    (no-op body); its settings currently drawn inline at the end of `DrawScenarioPatternTier`:
    `DrawSlotPatternToggleRow`, `DrawScenarioSpawnsWarningBanner`, `DrawScenarioBodyFields`.
    Count-tier list: `DrawScenarioCountList` (no-op body); settings at the end of
    `DrawScenarioCountTier`: `DrawScenarioCountConditionsEditor`, `DrawScenarioSpawnsWarningBanner`,
    `DrawScenarioBodyFields`.

### Files confirmed to need NO change (do not touch, listed so nobody re-investigates)
- `src/ui/MaskLayerTab_UI.cpp` — routes through the already-fixed `LayerEditor_UI.h`/
  `LayerEditor_Group_UI.cpp` (STEP104); nothing left to do.
- `src/ui/PlacementRuleSections_UI.cpp` — a pure section-widget library, no list, no
  `selectedIndex` of its own; not itself broken. Callers still call the same functions, only from
  a different location.
- `src/ui/ScenariosTab_Detail_UI.cpp`, `ScenariosTab_DetailAlloys_UI.cpp` (spawns/alloy-override/
  alloy-removal rows) — already always-visible inline flat blocks, no collapse/defer split.
- `src/ui/MarkersTab_Placed_UI.cpp`, `ArmiesTab_Mirror_UI.cpp`, and every other
  `ScenariosTab_*.cpp` file not named above — confirmed no `CollapsingHeader`/`DraggableList`
  list-editor shape.

## ARCH rules invoked
- Same as STEP104: pure UI layout, zero PARAMS/DATA/IO/PROC change anywhere in this ticket.
- Constitution — smallest reusable unit; this ticket is explicitly per-file, parallel-safe,
  dispatchable as independent units since no two files in the CLEAR list share any code.

## Explicit out-of-scope
- **`MarkersTab_ManualLayers_UI.cpp`/`.h`, `MarkersTab_ManualInstance_UI.cpp`** — HELD, see the
  warning at the top. Do not touch under any circumstance until the peer session's sequencing is
  resolved and a human/this session explicitly clears them.
- `ArmiesTab_Units_UI.cpp` — may be deferred to its own follow-up if the virtual-list-to-expandable
  conversion proves substantially harder than the other 10 files; not a hard requirement to land
  in the same pass.
- Any change to what settings an item has, their ranges, defaults, or validation — zero PARAMS
  change anywhere. Purely relocating where the same existing widgets render.
- `DraggableList<T>::Render` itself (`DraggableListWidget_UI.h`) — confirmed already correct,
  needs no change.

## Acceptance test (per file, mirroring STEP104's own test shape)
For each fixed file: a fixture with 2+ items with deliberately different settings values —
expanding one item's row shows THAT item's own settings, not another's (proves no bleed from a
stale "last selected" global); collapsing hides them; the item's own existing actions (delete,
duplicate, footprint-resolve buttons, etc.) still fire correctly from their new location. Full
solo rebuild + `ctest -C Debug` after each file/cluster: previously-passing suite stays green.

## Verify
No manual/interactive verification — automated test binaries and code reading only, per the
project's standing testing law.

# STEP80 — Markers tab: procedural rules rewired for `MarkerRuleLayer` (two-level list + layer symmetry)

*Constitution §7. Executor: SanGen Coder. From the SanGen UI Expert. This is the ticket for the gap
`STEP66_MarkerRuleLayer_PARAMS.md` "Explicit out-of-scope" bullet 4 flagged with no owner.*

**Sequencing (binding).** `STEP66` + `STEP79_MarkerRuleLayerProcConsumer_PROC.md` are a single
dispatch unit and must land first — STEP66 alone leaves `src/proc/` red, and this ticket's whole
premise is the PARAMS shape STEP66 introduces. Dispatch order: STEP66 → STEP79 → **this**.

## Root problem
`MarkersTab_UI.cpp:100-101` draws per-rule symmetry:
```cpp
DrawPlacementSymmetryAxes("markerSymmetry", rule->bSymmetryUseGlobal, rule->symmetryMask,
                          previewDriver);
```
STEP66 deletes all three symmetry fields from `Params::MarkerRule` and puts them on
`MarkerRuleLayer::symmetry` (`Params::SymmetrySetting`, ARCH_16_01_NewParamsShapes.md §16.1), so those two bindings stop
existing and the file stops compiling. The same commit turns `MapRecipe::markerRules`
(`std::vector<MarkerRule>`) into `markerRuleLayers` (`std::vector<MarkerRuleLayer>`), which the tab
renders as a **flat** `DraggableList` today (`MarkersTab_UI.cpp:20-37`) and reaches through
`SelectedMarkerRule(recipe.markerRules, …)` from two other UI files
(`Application_AssetPanel_UI.cpp:47`, `Application_Recipe_UI.cpp:62`).

## Target files
- `src/ui/MarkersTab_RuleLayers_UI.h` / `.cpp` (**new**) — the two-level list, its signal appliers,
  the add/remove buttons, and the selected-layer settings block. Split out rather than grown into
  `MarkersTab_UI.cpp` (128 lines today) so neither file breaches ARCH_01_05_FileSizeCeilings.md §1.5's hard 150.
- `src/ui/MarkersTab_UI.h` — `MarkersTabState` gains `selectedRuleLayerIndex`;
  `SelectedMarkerRule`'s first parameter becomes `std::vector<Params::MarkerRuleLayer>&`.
- `src/ui/MarkersTab_UI.cpp` — `DrawRuleList` / `ApplyRuleListSignal` / `DrawRuleListButtons`
  (lines 20-75) move to the new file; `DrawRuleStack` (78-106) calls into it and **deletes** its
  `DrawPlacementSymmetryAxes` call (100-101).
- `src/ui/Application_AssetPanel_UI.cpp` — line 47 call site follows the new signature.
- `src/ui/Application_Recipe_UI.cpp` — `AddDefaultPlacementRules` (line 62) seeds the spawn rule
  inside one default `MarkerRuleLayer` instead of pushing it onto a flat vector.
- `src/ui/MarkersTab_UI_Test.cpp` — extended (see acceptance test).

## Layer & accuracy class
UI. Accuracy class: Visual.

## Backend policy
N/A (no compute). Every commit goes through the existing `NotifyPlacementChange(bCommitted,
previewDriver)`; this tab never chooses a dirty tier — `Pipeline::PreviewDriver` derives it.

## ARCH rules invoked
- `ARCH_16_01_NewParamsShapes.md` §16.1 — the `MarkerRuleLayer` / `SymmetrySetting` shape, and the
  ruling that **`MarkerRule::bHidden` stays per-rule and is independent of the layer's**, not
  relocated. `MarkerRule::bEnabled` likewise stays.
- `ARCH_01_05_FileSizeCeilings.md` — soft 100 / hard 150 per file, functions ≤ 40 lines. The split
  above exists to honour it; no exception is claimed by this ticket.
- Constitution §1 (UI owns no sim logic), §2 (no abbreviations), §8 (limits are settings).
- ARCH_16_09_NonArchItems.md §16.9 — UI-internal naming needs no ARCH ratification; the constants this ticket adds are
  ordinary implementation hygiene.

## Solution — shape

### 1. The two-level list — reuse `LayersTab_UI`'s GeoLayer precedent verbatim
**A precedent exists; nothing new is invented.** `DraggableList<T>::Render`
(`DraggableListWidget_UI.h:64-100`) already takes a `drawRowBody(int rowIndex)` callback drawn
inside `ImGui::Indent()` under an `ImGui::PushID(rowIndex)` — nesting a second `DraggableList` in
it is already the shipped pattern at `LayersTab_UI.cpp:84-116` (`DrawGeoLayerList`), where
`geoLayers` is the outer list and each group's `layers` the inner one. Mirror that file's three
mechanics exactly:
- **Only the selected layer renders its rule list** (`LayersTab_UI.cpp:62-67`); an unselected row
  body prints `"%d rule(s) - select this layer to edit them"`. This is what makes a cross-layer
  drag structurally impossible rather than merely guarded.
- **The inner signal is captured out of the body lambda** into a local plus its owning layer index
  (`LayersTab_UI.cpp:86-87, 98-102`).
- **The inner signal is applied FIRST**, before the outer one (`LayersTab_UI.cpp:105-114`) — a layer
  Delete in the same frame would invalidate the indices the rule signal is expressed in. Copy that
  comment's reasoning, not just its code.

Two `inline` appliers in the new header, shaped one-for-one on `ApplyGeoLayerListSignal` /
`ApplyLayerListSignal` (`LayersTab_UI.h:58-100`) — they take an `int&` selection index, **not**
`MarkersTabState`, which is what keeps the new header free of any include back into
`MarkersTab_UI.h` (no cycle):
- `ApplyMarkerRuleLayerListSignal(std::vector<MarkerRuleLayer>&, const DraggableListSignal&, int& selectedRuleLayerIndex, int& selectedRuleIndex)`
  — `ToggleVisibility` → `layer.bEnabled`, `ToggleLock` → `layer.bHidden`, `Select` → move the
  selection and reset `selectedRuleIndex` to 0, `Reorder`/`Delete` → `ApplyDraggableListSignal`.
  Returns true only when the **recipe** moved (a Select did not) — same contract as the precedent.
- `ApplyMarkerRuleListSignal(MarkerRuleLayer&, const DraggableListSignal&, MarkersTabState&)` —
  the existing `ApplyRuleListSignal` body (`MarkersTab_UI.cpp:41-62`) rehomed against
  `layer.rules`, keeping its `LoadMarkerRuleValues` + `LoadMarkerRuleEnumIndices` calls on Select.

`SelectedMarkerRule` becomes a two-index walk (both indices bounds-checked, null on either miss),
exactly like `SelectedLayer` at `LayersTab_UI.cpp:120-127`. A new `SelectedMarkerRuleLayer` does the
outer half and is what the layer settings block below binds to.

### 2. Where symmetry now draws — a "Selected Layer" block, not a row body
`DrawRuleStack` gains one section between the list and the per-rule detail sections, mirroring how
`DrawLayersTab` draws per-layer controls *below* the nested list (`LayersTab_UI.cpp:140-144`) rather
than inside a row body. Bound to `SelectedMarkerRuleLayer(...)`; when it is null, print
`"Select a marker layer to edit it."` and draw no rule detail either. Contents:
- **Name** — `DrawTextInput("Layer Name", layer.name, nameRules)` with the `AreasTab_UI.cpp:96-99`
  rules shape (`maximumLength = 48`, `bAllowEmpty = false`, `fallbackText = "Marker Layer"`). No
  uniqueness repair: `MarkerRuleLayer::name` is a plain array element on the wire (STEP66's
  `MarkersStack` shape), not a dictionary key — unlike `recipe.markers` in STEP49.
- **Enabled / Hidden** — the two checkboxes specified in §3.
- **Symmetry** — the relocated call, now `DrawPlacementSymmetryAxes("markerLayerSymmetry",
  layer.symmetry.bSymmetryUseGlobal, layer.symmetry.symmetryMask, previewDriver)`. Only the selected
  layer draws it, so one imgui id is enough. `DrawPlacementSymmetryAxes`
  (`PlacementRuleSections_UI.cpp:17-25`) is unchanged by this ticket.

**What the rule rows and the rule detail lose:** exactly one thing — the deleted
`DrawPlacementSymmetryAxes` call at `MarkersTab_UI.cpp:100-101`. Every other per-rule section
(`DrawMarkerRuleGates`, `DrawMarkerRuleQuantity`, `DrawMarkerRuleArea`, `DrawMarkerRuleFocus`,
`DrawPlacementGateSection`, `DrawPlacementTransformSection`, `DrawPlacementTemplatePicker`) is
untouched, as is the rule row label `"%d: %s x%d"` (`MarkersTab_UI.cpp:27-28`).

### 3. `bEnabled` is a generation gate — the UI must say so
Human ruling, binding: `MarkerRuleLayer::bEnabled` is a **real generation gate**, not a view filter.
`Placement_Rules_PROC.cpp:24` today reads `if (!rule.bEnabled && !rule.bHidden) continue;`; after
STEP79 the effective suppression is
`(!layer.bEnabled && !layer.bHidden) || (!rule.bEnabled && !rule.bHidden)`.
`DraggableList`'s affordance strip renders `bVisible` as an eye-ish `[o]`/`[-]` glyph
(`DraggableListWidget_UI.h:110-111`) — a glyph that reads as "show/hide" and would misdescribe this
flag if it were the only signal. Two requirements, no new widget:
- **The row label carries the state in words.** Format the layer row as
  `"<name> (<n> rules)"`, appending `" - DISABLED, not generated"` when `!bEnabled` and
  `" - hidden, still generated"` when `bHidden`. `DraggableListRow::label` is borrowed for the call
  only (`DraggableListWidget_UI.h:29-33`), so the existing stack-`char[]`-plus-`snprintf` pattern
  (`MarkersTab_UI.cpp:22`, `LayersTab_UI.cpp:68`) applies — widen the buffer to fit.
- **The checkbox pair in the Selected Layer block is the authoritative control**, worded like the
  honest per-rule label already at `MarkersTab_Rules_UI.cpp:36`:
  `"Enabled (off = this whole layer is not generated)"` and
  `"Hidden (still generated for clearance/fairness, not drawn)"`.

Extending `DraggableListRow` with a semantic per-row glyph or tooltip would be a shared-widget
change affecting every list in the app — **out of scope, flagged below**, not attempted here.

### 4. Add / remove / reorder, and what a deleted layer takes with it
`DrawRuleListButtons` becomes a two-row button block, still applied **after** the list has drawn so
the vectors never move under a live row (the existing comment at `MarkersTab_UI.cpp:64` states the
rule; keep it):
- `"Add Layer"` → `markerRuleLayers.push_back(Params::MarkerRuleLayer())`, then select it and reset
  `selectedRuleIndex` to 0. Always available, including at zero layers.
- `"Add Rule"` / `"Remove Selected Rule"` → operate on `SelectedMarkerRuleLayer(...)->rules`;
  **drawn `ImGui::BeginDisabled()`-wrapped when no layer is selected**, never silently no-op.
- Reorder of layers and of rules within a layer is the `DraggableList` drag, applied by the two
  appliers in §1. There is **no cross-layer drag** — a rule moves between layers only by being
  removed and re-added. Say so in the header comment; do not build it.
- **Deleting a layer deletes its rules with it.** `MarkerRuleLayer` *owns* its
  `std::vector<MarkerRule> rules` (ARCH_16_01_NewParamsShapes.md §16.1 calls it a real container, unlike the flyweight
  `MarkerInstanceLayer`), so `ApplyDraggableListSignal`'s `items.erase`
  (`DraggableListWidget_UI.h:47-50`) takes them along. Nothing is orphaned and nothing is
  reparented. Because that is unrecoverable, gate it: when the targeted layer's `rules` is
  non-empty, route the `Delete` signal through `DrawConfirmDialog` (`ConfirmDialog_UI.h:43-44`)
  with `bClosableWithoutChoice = false`, body text naming the layer and its rule count, and apply
  the erase only on `bPrimaryClicked`. An empty layer deletes immediately. Hold the pending target
  as an index in `MarkersTabState` and re-validate it against `markerRuleLayers.size()` before
  applying — the vector may have moved between the request frame and the confirm frame
  (Constitution §6: indices are clamped or rejected, never trusted).

### 5. Default recipe
`AddDefaultPlacementRules` (`Application_Recipe_UI.cpp:55-62`) builds the same spawn `MarkerRule`
and pushes it into one `MarkerRuleLayer` named `"Spawn Layer"` appended to `recipe.markerRuleLayers`.
Its `symmetry` keeps `SymmetrySetting`'s defaults (`bSymmetryUseGlobal = true`) — identical
effective behaviour to today's rule-level default, so no default map changes shape.

## Explicit out-of-scope
- **Anything manual-marker.** `MarkersTab_ManualLayers_UI`, `Params::MarkerInstanceLayer`,
  `recipe.markerLayers`, and the layer picker on STEP49's per-instance editor all belong to
  `work_orders/STEP81_MarkersTabManualLayers_UI.md`. Manual instances are a different domain from
  procedural rules (ARCH_16_01_NewParamsShapes.md §16.1: the two "Layer" types deliberately do **not** share a containment
  shape). This ticket touches no manual-marker file and defines none of STEP81's content; where the
  Markers tab composes both, STEP81 owns the composition of its own half.
- **Any PROC change.** The `(!layer.bEnabled && !layer.bHidden)` gate quoted in §3 is described here
  only so the UI wording matches it; `Placement_Rules_PROC.cpp` / `Placement_Hash_PROC.cpp` belong
  to `STEP79_MarkerRuleLayerProcConsumer_PROC.md`.
- **Any PARAMS or IO change.** STEP66 owns the shape and its round-trip; STEP66's own out-of-scope
  list owns `MarkersStack_Migrate_V3`.
- **A UI control for `radialSymmetryRepeatCount`.** Confirmed by grep: it has **zero** call sites
  anywhere under `src/ui/` today — no placement tab (markers, props, decals, units) has ever exposed
  it, and `DrawPlacementSymmetryAxes` takes only the other two fields. Adding it here alone would
  make markers the lone tab with the control. Flagged as a cross-tab follow-up ticket (Constitution
  §8 — the field is a legitimate tweakable and should be exposed once, consistently), not built here.
- **Extending `DraggableListRow` / `DraggableListSignalKind`** with a semantic glyph, tooltip, or a
  cross-list drag payload. Shared-widget surgery affecting every list in the app; §3's label wording
  is the in-scope answer.
- **Props / Decals / Units tabs.** `PropsTab_UI.cpp:86`, `PropsTab_Decals_UI.cpp:111` and
  `ArmiesTab_Units_UI.cpp:117` keep their per-rule `DrawPlacementSymmetryAxes` calls — ARCH_16_01_NewParamsShapes.md §16.1
  explicitly does **not** retrofit `SymmetrySetting` onto those types.

## Acceptance test
Extend `src/ui/MarkersTab_UI_Test.cpp` (windowless, pure-logic, same shape as its existing
`RunRuleMirrorChecks`, and as `ParameterTabs_Layers_UI_Test.cpp:28-54` which already tests the
GeoLayer appliers this ticket copies):
1. Build a `std::vector<Params::MarkerRuleLayer>` of 2 layers with 2 rules each. `Reorder(0→1)` on
   the layer list: assert the moved layer **and its rules** land at index 1 intact.
2. `Select` on a layer returns false (no recipe move) and moves `selectedRuleLayerIndex`;
   `ToggleVisibility` flips `layer.bEnabled` and returns true; `ToggleLock` flips `layer.bHidden`.
3. `Delete` a layer with rules: `markerRuleLayers` shrinks by one, total rule count drops by that
   layer's rule count, and no rule from it survives anywhere.
4. Out-of-range `sourceRowIndex` on both appliers returns false and mutates nothing.
5. `SelectedMarkerRule` returns null for an out-of-range layer index, for an out-of-range rule index
   within a valid layer, and for an empty `markerRuleLayers`.
6. Grep the tree: **no `src/ui/` file references `rule.bSymmetryUseGlobal` / `rule.symmetryMask` on
   a `MarkerRule`**, and `DrawPlacementSymmetryAxes` appears exactly once in the markers tab, bound
   to `layer.symmetry.*`.
7. `wc -l` every touched and new file: all ≤ 150 lines, no function over 40 (ARCH_01_05_FileSizeCeilings.md §1.5).

Full `SanGenV2` build stays clean and every existing test passes — unlike STEP66 standing alone, this
ticket lands after STEP79, so a red build here **is** a regression.

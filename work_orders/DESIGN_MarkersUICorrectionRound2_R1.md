# Design: Markers UI Correction Round 2

Grounded entirely in direct reads of the current code (files/lines cited throughout). Every point the
brief states as a confirmed fact or the human's exact words is treated as fixed; every place the
designer had to make a call the brief left open is marked **JUDGMENT CALL**.

---

## 1+14. Global section — one line, 5 controls (icon / label / compact scale slider / normal-color swatch / select-color swatch)

**Root cause confirmed** at `src/ui/MarkersTab_Globals_UI.cpp:61-79` and `src/ui/SliderScalar_UI.cpp:34-53`/`SliderScalar_Track_UI.cpp`: `DrawSliderScalar` is structurally 3 lines (label text → `ReserveScalarSliderTrack`'s own InvisibleButton row → `DragFloat`+RT-button row), because `ReserveScalarSliderTrack` (`SliderScalar_Track_UI.cpp:10-19`) always claims `ImGui::GetContentRegionAvail().x` and nothing SameLines after it.

**Design — add a compact slider entry point, not a rewrite of the existing one.** `DrawSliderScalar`/`DrawSliderScalarInteger` stay byte-identical (every other caller in the codebase depends on the 3-line shape). Add, in the same file (`SliderScalar_UI.cpp`, currently 57 lines — plenty of ARCH §1.5 headroom):

```cpp
// SliderScalar_UI.h
WidgetChange DrawSliderScalarCompact(const char* label, float& value, const ScalarSliderRange& range,
                                      RealtimeToggle& realtimeToggle, float trackWidthPixels,
                                      float fieldWidthPixels, const WidgetStyle& style = WidgetStyle(),
                                      const char* valueFormat = "%.2f");
```

Composition: `PushID(label)` → reserve the track at a **caller-supplied fixed width** (not remaining-content-region) → `SameLine()` → a **narrow** `DragFloat` at `fieldWidthPixels` → `SameLine()` → the existing small RT button (`style.realtimeButtonWidth`, already only 30px, `WidgetHelpers_UI.h:33`) → paint the track. No visible label line; instead `if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label);` on the track item — this is not a new idiom, it's the exact pattern already shipped for the header-extra Color Override checkbox (`MarkersTab_ManualLayerRowBody_UI.cpp:90-91`).

This needs two small, non-breaking, default-preserving additions to the shared widget internals (`SliderScalar_Track_UI.h`/`.cpp`):
- `ReserveScalarSliderTrack(style, requestedWidthPixels = 0.0f)` — `0` keeps today's "rest of the line" behavior; `>0` uses a fixed width. Mirrors `ColorSwatchOptions::swatchWidth`'s exact `<=0`/`>0` convention (`ColorSwatch_UI.h:27-28`) — same pattern, not a new one.
- `DrawFloatFieldRow`'s field width gains the same `<=0`/`>0` override (it's currently anonymous-namespace-local to `SliderScalar_UI.cpp:15-30`; stays there, just gains a parameter — no need to promote it to the internal header since the new function lives in the same TU).

**Row composition** (order is the human's own words, brief line 16-17, plus item 14's addition — not the designer's call to reorder):

`[Icon Button] [Label text] [Compact slider] [Normal-color swatch, label hidden] [Select-color swatch, label hidden]`, all `ImGui::SameLine()`-chained inside one `ImGui::Columns`-free row (columns are exactly what currently forces the multi-line shape and get dropped entirely).

`GlobalMarkerScaleRowFields` (`MarkersTab_Globals_UI.h:70-89`) gains one more pointer, mechanically mirroring `color`:
```cpp
float* selectColor = nullptr;   // 4 floats: selectColorAlloy/Plasma/Spawn (GlobalMarkerSettings_PARAMS.h:28-30)
```
and `ResolveGlobalMarkerScaleRowFields`'s 3-case switch gains `settings.selectColorAlloy/Plasma/Spawn` in each branch. `MarkerGlobalScaleRow` (`.h:41-51`) gains a third `RealtimeToggle selectColorToggle{true};` — kept **independent** of `iconScaleToggle`/`previewColorToggle` (the existing struct already carries two separate toggles for two separate fields; merging any of them into a shared toggle would be a real semantic change to the per-control RT-tweakability model, not something to invent unilaterally here).

Both color swatches use `DrawColorSwatch(..., ColorSwatchOptions{.bLabelHidden = true, .swatchWidth = <small fixed px>}, ...)` — this is exactly the already-shipped pattern at `MarkersTab_ManualLayerRowBody_UI.cpp:94-100`.

**JUDGMENT CALL — width budget.** Nine imgui items end up chained on one line (icon, label, track, drag-field, RT, swatch, RT, swatch, RT). Rough budget at plausible sizes (32px icon / ~50px label / ~90px track / ~40px field / 30px RT / 20px swatch ×2 / 30px RT ×2 / spacing) lands close to ~350-380px. Cannot confirm the Markers tab's actual panel width from the files read alone — flagged as a real implementation risk, not asserted to fit: **the coder ticket should measure against the live panel and, if it overflows, shrink the icon button (48px→24-32px) and the compact track width first** — those are the two most compressible elements — before touching anything else in the row's composition.

---

## 4. "(Unassigned)" bucket disposition — JUDGMENT CALL

**Confirmed via `MarkersTab_TypeSections_UI.cpp:50-56`:** `""` is unconditionally appended, unlike every other type name in the same function which is present-only (`CollectDistinctNonEmptyTypeName`, `.cpp:24-28`, explicitly skips empty strings).

**Recommendation:** don't remove the bucket outright — gate it the same way every other Type-section is gated: **present-only**. Stop special-casing `""`; run the SAME "does anything actually carry this markerTypeName" test the other names already get, just without the `.empty()` early-return skip, and append `""` to `ordered` only if that test is true. Net effect: a brand-new/fully-migrated recipe never shows an "(Unassigned)" section at all (satisfies the real objection — an always-there bootstrap section nobody asked for); a recipe carrying legacy-imported empty-typed data gets exactly one minimal, clearly-labeled recovery section, and it disappears again once nothing is empty-typed.

**Does it get a self-service "assign a type" affordance?** Checked directly:
- **Bundles already have one.** `MarkersTab_BundleNodeBody_UI.cpp:87`: `DrawTextInput("Marker Type", bundle.markerTypeName, typeRules)` — free-text, soft-validated, already reachable from every Bundle's own node body. Since Type-section membership is a pure per-frame function of `markerTypeName`, typing a real type here just moves the Bundle to that section next frame — no new mechanism needed.
- **Ungrouped `MarkerRuleLayer`/`MarkerInstanceLayer` rows have no equivalent.** Confirmed by reading `DrawLayerRowBody` in full (`MarkersTab_ManualLayerRowBody_UI.cpp:24-76`) — no `markerTypeName` field is drawn anywhere in that body.

**Recommendation (judgment call):** give ungrouped `RuleLayer`/`InstanceLayer` rows the identical free-text field, but **only conditionally** — drawn at the top of the row body only when `markerTypeName.empty()`, so it never clutters a normal, already-typed row. This is the minimum fix that closes the dead-end without inventing a new pattern (it's the Bundle's own field, reused).

---

## 5. Flatten "Ungrouped ..." sections

**Human's exact words:** "Ungrouped markers get listed individually after all the groups." Confirmed target: `MarkersTab_TypeSections_UI.cpp:77-91` currently wraps both ungrouped lists in their own `DrawSectionBegin("Ungrouped Procedural Rules"/"Ungrouped Manual Marker Layers", ...)`.

**Fix:** delete both `DrawSectionBegin`/`DrawSectionEnd` wrappers (and the two `SectionState` fields they gate, `ungroupedProceduralSection`/`ungroupedManualSection` in `MarkerTypeSectionState_UI`, `MarkersTab_TypeSections_UI.h:38-42`, which become dead). `DrawRuleLayerListBody`/`DrawAddMarkerRuleLayerButton`/`DrawManualMarkerLayerListBody` calls stay exactly as they are — they already draw *rows*, not their own headers; only the enclosing header/collapse chrome goes away. Net composition inside one Type section becomes: Bundle tree → (flat) ungrouped procedural rows → (flat) ungrouped manual rows, with a plain `ImGui::Separator()` between the tree and the flat rows if a visual break is wanted (small call, not load-bearing).

---

## 6. Full hierarchy re-verification

Acceptance gate, not a ticket. After 4/5 land, walk Type → Group → Layer → Instance against the human's original diagram end to end (and once 13 lands, confirm Procedural instances appear the same way the diagram showed).

---

## 7. `TreeListWidget_UI` header-extra slot (needs ARCH sign-off)

Confirmed by direct read: `TreeListWidget_UI.h`/`TreeListWidget_RowLayout_UI.h` have **no** header-extra mechanism — `RenderNode` draws `ImGui::CollapsingHeader(nameOf(node), flags)` (`TreeListWidget_RowLayout_UI.h:74`) and `RenderLeaf` draws `ImGui::TreeNodeEx(leafLabel(leaf), ...)` (`.h:45-46`) with nothing after.

**Design, mirroring `DraggableList<T>::Render`'s 3-callback overload (`DraggableListWidget_RowLayout_UI.h:83-137`) as precedent, with one deliberate divergence:** the tree has **two** distinct row kinds (node vs. leaf), each with its own identity type (`int nodeIdentifier` vs `const LeafKeyT&`), so it needs **two** header-extra callbacks, not one:

```cpp
// existing 7-callback overload becomes a thin delegator (unchanged call sites keep compiling):
return Render(..., /*drawNodeHeaderExtra=*/[](int){}, /*drawLeafHeaderExtra=*/[](const LeafKeyT&){},
              /*headerExtraWidthPixels=*/0.0f, state, selectedNodeIdentifier);

// new overload, additive:
template <..., typename DrawNodeHeaderExtraFn, typename DrawLeafHeaderExtraFn>
static TreeListSignal<LeafKeyT> Render(..., DrawNodeHeaderExtraFn drawNodeHeaderExtra,
                                        DrawLeafHeaderExtraFn drawLeafHeaderExtra,
                                        float headerExtraWidthPixels, TreeListState& state,
                                        int selectedNodeIdentifier = -1);
```

Threaded into `TreeListDetail::RenderNode`/`RenderLeaf` (`TreeListWidget_RowLayout_UI.h`), drawn after the click/drag-drop detection, before the `if (bExpanded)` body:

```cpp
if (headerExtraWidthPixels > 0.0f) {
    const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;   // measured HERE, this row's own margin
    ImGui::SameLine(rowAvailWidthPixels - headerExtraWidthPixels);
    drawNodeHeaderExtra(nodeIdentifier);   // or drawLeafHeaderExtra(leaf) in RenderLeaf
}
```

**Important: this is deliberately *not* a copy of `DraggableListWidget_RowLayout_UI.h:24-47`'s formula.** The item 8/9 bug is: `RenderCollapsibleRow` computes the header-extra's own SameLine offset *and* (via a width it passes in) `DrawRowAffordances`'s start offset from the **same independently-re-derived formula**, landing both at the same X. Tree rows have **no** affordance strip today, so there's nothing to collide with, and the design above only positions one thing — it's structurally immune to that bug class by construction. Flagged explicitly as a rule for any *future* tree-strip work: position the next element relative to *where the previous one's own draw call left the cursor*, never by re-deriving the same subtraction twice.

**7(b):** wire the Bundle tree's leaf callback (`MarkersTab_Bundles_UI.cpp:97-118`) to pass `drawLeafHeaderExtra = [&](const MarkerGroupLeafKey_UI& leaf) { if (leaf.kind == Manual) DrawManualMarkerLayerColorOverrideHeaderControl(...); }` — gated to `Manual` only, confirmed correct scope by reading `MarkerRuleLayer` (`MarkerRule_PARAMS.h:77-90`): it carries no `color`/`bColorOverrideEnabled` field, so Rule leaves have nothing to draw here, matching the existing DraggableList call site which also only ever wires Manual layers (`MarkersTab_ManualLayers_UI.cpp:85-88`). Once wired, delete `DrawLayerRowBody`'s own body copy (`MarkersTab_ManualLayerRowBody_UI.cpp:35-41`) — its stated reason for existing (line 78-86's own comment) becomes false.

**ARCH sign-off needed:** yes — this is a real shared-widget contract change, same class as the original `DraggableList` header-extra ratification.

---

## 10. Symmetry toggle on the row header

**Scope confirmation:** yes, it must reach Bundle-tree rows too, via the item-7(a) slot. `layer.symmetry` (`MarkerInstance_PARAMS.h:30-36`, `Symmetry_PARAMS.h:47-51`) is a `Params::MarkerInstanceLayer` field, unrelated to `parentBundleIdentifier`; there's no reason a bundled layer should lack a control an ungrouped one has.

**JUDGMENT CALL — what field it toggles.** `layer.symmetry` is `SymmetrySetting{ bSymmetryUseGlobal, symmetryMask, radialSymmetryRepeatCount }` — a bitmask, not a bool, and today it's edited only inside the full "Layer Symmetry" section body (`MarkerLayerSymmetrySection_UI.cpp:43-56`, `DrawPlacementSymmetryAxes`). There is no existing single bool that means "symmetry on/off" for a layer. Recommendation: add a **new**, additive `bool bSymmetryEnabled = true;` field to `MarkerInstanceLayer`, precedented exactly by `bColorOverrideEnabled`'s own shape in the same struct (`MarkerInstance_PARAMS.h:42-49`) — a gate checked wherever `layer.symmetry` is currently consumed (`MarkerDragGesture_UI.cpp`'s `ResolveEffectiveMarkerSymmetry` call sites, `MarkerSymmetryFixCommand_UI.cpp`), forcing the effective mask to `None` when off, **without destructively clearing** the axis checkboxes already configured in the body. Confirmed `layer.symmetry` has zero PROC/bake consumers for manual layers (only the drag-mirror gesture and the Fix Symmetry repair tool read it). **This is a new PARAMS field — needs ARCH sign-off.**

**Placement:** row header-extra region grows to two adjacent controls, `[Symmetry toggle][Color Override]`, left of lock/delete on the flat/ungrouped `DraggableList` rows (`headerExtraWidthPixels` widens to the sum of both controls' widths; the existing single callback just draws two things in sequence — no further widget-library change needed beyond 7(a)'s slot itself).

---

## 11. Unify list-selection with canvas click-selection (needs ARCH sign-off)

Investigated `Application_UI.cpp:74-107`, `MapCanvas_UI.cpp` (`ApplyClick`/`SetSelection`), `MapCanvas_Draw_UI.cpp:76-97`, `MapCanvas_IconLayer_CullEmit_UI.cpp`, `MapCanvas_IconLayer_CullManual_UI.cpp`, `MapCanvas_IconLayer_CullProcedural_UI.cpp`, `MapCanvas_IconLayer_Cull_UI.cpp:109-144`, `MapCanvas_MarkerRosterDraw_UI.cpp` in full. The picture is more specific — and one bug worse — than the brief's own framing:

**What actually happens today:**
- `MapCanvas::ApplyClick` (`MapCanvas_UI.cpp:34-56`) resolves a click via `PickMarker(*pickMarkerSpatialGrid, *pickMarkerInstances, ...)` — a spatial-grid hit test against **`assembler.Placements().markers`**, i.e. `Data::PlacementInstances`. `selectedEntityIdentifier` **is** the resulting array **position** in that SoA.
- `Data::PlacementInstances::markers` is populated **only by procedural marker rules** (`GenerationAssembler_Stages_PIPELINE.cpp:87-88`, `placementStage.Run()`). Confirmed by reading `Placement_Manual_PROC.cpp` in full: `ResolveManualPropsAndDecals()` copies manual **props/decals** into `results.props`/`results.decals` — it never touches `results.markers`. So **canvas click-pick cannot select a manual marker at all today** — `pickMarkerSpatialGrid` is built purely from procedural positions.
- The real icon sprites for manual markers are drawn by an entirely separate cull path, `ResolveMarkersManual` (`MapCanvas_IconLayer_CullManual_UI.cpp:124-176`), which tags each candidate `instance.instanceKey = OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, index, true}` where `index` is the position **within that one `MarkerInstanceGroup`'s own `transforms` vector** (`.cpp:171`) — **not** globally unique, and **not** the same number space `PickMarker` addresses.
- `DrawOverlayIconLayerPass` (`MapCanvas_Draw_UI.cpp:92-94`) builds `selectedInstanceKey` from `selectedEntityIdentifier` under the **same** `PlacementCollectionKind_UI::Markers` tag, meaning **a procedurally-selected marker's `PlacementInstances` position can numerically collide with an unrelated manual marker's within-group transform index**, incorrectly lighting up `instance.bSelected` on that manual marker (`MapCanvas_IconLayer_CullEmit_UI.cpp:68-69`). This is a real, currently-live bug found while investigating item 11, distinct from the brief's own "wrong target" framing.

**Design (grounded in what already exists, not invented from scratch):**

1. **Fix the index-space collision first.** Add `bool bManual = false;` to `OverlayInstanceKey_UI` (`MapCanvas_IconLayer_UI.h:24-29`) and include it in `OverlayInstanceKeysEqual`. Default `false` keeps every existing procedural comparison byte-identical.
2. **Give manual marker candidates a globally-unique key.** `ResolveMarkersManual` should pass `transform.instanceIdentifier` (not the per-group `index`) as the key, with `bManual = true`. This field already exists **for exactly this purpose** — `MarkerInstance_PARAMS.h:72-77`: "Stable, GLOBALLY unique across every MarkerInstanceGroup's transforms... Exists solely for stable UI-selection addressing." Currently unused for selection anywhere in the render path — this is the missing wire, not a new field. Edge case for the coder: an unassigned transform defaults `instanceIdentifier = -1`; confirm every live construction path mints a real id before trusting `-1` never collides with "no selection."
3. **Widen the canvas's selection state from a bare `uint32_t` to the full key.** `MapCanvas::selectedEntityIdentifier`/`SelectedEntityIdentifier()`/`HasSelection()` (`MapCanvas_UI.h:136-140,174`) become backed by `OverlayInstanceKey_UI selectedInstanceKey`, with the old accessors kept as thin reads of `.instanceIndex`/`.bValid` for existing callers that only care about the procedural case (`Application_Draw_UI.cpp:64`). `ApplyClick` tries `PickMarker` first (procedural); on miss, add a small **linear** hit-test over `recipe.markers` (authoring scale — same "no grid needed" posture `MapCanvas_IconLayer_CullManual_UI.cpp`'s own header comment already states for manual layers) so a canvas click can select a manual marker for the first time; on a manual hit, set `selectedInstanceKey = {Markers, transform.instanceIdentifier, true, bManual=true}`.
4. **Widen `selectionChangedCallback`** (`std::function<void(std::uint32_t)>`, `MapCanvas_UI.h:172`) to carry the full key, so `Application::WireCallbacks()` (`Application_UI.cpp:76-78`) can update both `lastSelectedEntityIdentifier` and, when `bManual`, `tabState.markers.selectedManualInstanceIdentifier` — keeping the list's own highlight in sync when the canvas is what changed selection.
5. **List-click → canvas.** `DrawLayerRowBody`'s Selectable click (`MarkersTab_ManualLayerRowBody_UI.cpp:70-72`) needs to reach the canvas's selection, not just the tab-local scalar. Per `Application_UI.cpp:71-73`'s own stated rule (the shell mediates, UI modules don't reach into each other directly), the correct shape is a **new small shell-mediated callback** the same way every other UI↔UI/UI↔PIPELINE boundary in this file is wired: `Application` binds a `std::function<void(int)> selectManualMarkerInstanceCallback` in `WireCallbacks()` to a new public `MapCanvas::SelectManualMarkerByInstanceIdentifier(int)`, threaded down through `DrawMarkerTypeSections`/`DrawLayerRowBody`'s existing call chain (the same chain `previewDriver`/`iconManifest` already ride down) so the row click calls it instead of (in addition to) setting `selectedManualInstanceIdentifier` directly. `selectedManualInstanceIdentifier` itself stays — it's still the list's own "which row is highlighted" bookkeeping, now kept in sync in **both** directions through the shell rather than being a dead-end write.
6. **The dot roster is *not* simply deletable.** `DrawManualMarkerRoster` (`MapCanvas_MarkerRosterDraw_UI.cpp:72-137`) draws every manual marker's dot **always**, and carries logic the real-icon path (`ResolveMarkersManual`) does **not** reproduce — drag-refused tint, drag-ghost points, and `ManualSpawnArmyTint` (Army-color matching for Spawn markers, `.cpp:51-58,110`). Only its selection-highlight branch (`IsInstanceHighlighted`/`ResolveMarkerGroupSelectTintColor`, `.cpp:60-68,96-108`) becomes redundant once the real icon correctly shows `bSelected`. Recommend deleting **only that branch and its now-dead plumbing** (`selectedHighlightInstanceIdentifiers`, `ComputeManualMarkerSelectionHighlight`), as a lower-priority follow-up *after* the real-icon fix is verified — not bundled destructively with the rest of the file, which stays.

**ARCH ratification needed:** yes — this changes a real cross-cutting selection representation (`OverlayInstanceKey_UI`'s new field, `MapCanvas`'s widened public selection surface, the shell↔canvas selection callback contract).

---

## 12. Symmetry grouping in the instance list

**Human's exact words are the spec.** `MarkerTransform::symmetryGroupIdentifier` (`MarkerInstance_PARAMS.h:61-64`) already exists: `0` = ungrouped, non-zero = a real cluster, populated by drag-materialize and `MarkerSymmetryFixCommand_UI.cpp`.

**UI shape**, inside `DrawLayerRowBody`'s existing instance-list block (`MarkersTab_ManualLayerRowBody_UI.cpp:57-74`): partition the layer's `(groupIndex, transformIndex)` pairs (already assembled by `instanceIt->second`) by `symmetryGroupIdentifier`. Non-zero buckets render **first**, each as its own `ImGui::TreeNodeEx`/`CollapsingHeader`-style collapsible node labeled e.g. `"Symmetry Group N (k)"`, containing the same `Selectable` rows the flat list draws today; then every `symmetryGroupIdentifier == 0` instance lists flat, individually, exactly as today, after all the groups. This needs no new widget — a small local grouping pass plus a nested `ImGui::TreeNodeEx` per non-zero bucket, reusing the existing `Selectable`+`bRowSelected` row body unchanged.

**ARCH sign-off:** the grouping UI shape (not a new field) should be recorded even though no PARAMS change is involved.

---

## 13. Procedural instance listing/selection — the mechanism

Overridden per the brief: build it. The mechanism is largely **already the shape the app uses for canvas picking**, not something to invent:

- `Data::PlacementInstance::ruleIndex` (`PlacementInstance_DATA.h:46`) already tags which rule produced each entry.
- The exact same per-frame-index shape `ManualInstanceLayerIndex_UI` already uses generalizes directly: build `std::unordered_map<int /*ruleIndex*/, std::vector<int /*array position*/>>` by walking `assembler.Placements().markers` once per frame, keyed by `ruleIndex`. **No new DATA field needed** — a straight analog of the manual index, just keyed by `ruleIndex` and storing raw SoA positions instead of `(groupIndex, transformIndex)` pairs.
- **Selection key**: the position itself, exactly `OverlayInstanceKey_UI{Markers, position, true, bManual=false}` — i.e. procedural instance selection converges onto the *same* representation item 11 establishes for canvas click-pick (they're the same array). This is a genuine, load-bearing point of convergence between 11 and 13, not a coincidence — build it once as a shared mechanism rather than twice.
- **Session-only, no persistence** — confirmed correct per the brief: selection here never needs to survive a re-bake; a fresh bake invalidates the index and any prior selection simply stops resolving (same posture the manual index already has toward `recipe.markers` edits).

**Symmetry-grouping parity — confirmed and answered plainly.** `Data::PlacementInstance::symmetryIdentifier` (`PlacementInstance_DATA.h:48`) **does** exist and **is** populated for markers, per `Placement_Accept_PROC.cpp:12-51` — but its semantics differ from `symmetryGroupIdentifier`'s "0 = ungrouped" convention: `nextSymmetryIdentifier` starts at `1` (`Placement_PROC.h:127`, `Placement_PROC.cpp:44`) and increments for **every accepted candidate**, mirror or not (`Placement_Accept_PROC.cpp:43`) — so a lone, unmirrored instance still gets its own unique non-zero id; it's never `0`. The correct translation for item 12's grouping criterion is therefore **bucket size, not id value**: bucket procedural instances by `symmetryIdentifier`, and treat a bucket as a real "symmetry cluster" (collapsible) only when it holds **more than one** instance; a bucket of exactly one is a free/ungrouped row, regardless of its (always non-zero) id.

**ARCH ratification needed:** yes — the per-frame positional-index mechanism (and its convergence with item 11's selection key) is a real cross-cutting design decision.

---

## Delivery-ticket split recommendation

- **No design needed, straight to coder** (unchanged from the brief): 2, 3, 8/9.
- **Design complete above, ready for work-orders**: 1+14, 4, 5, 7(a)+7(b), 10, 11, 12, 13.
- **Sequencing:**
  1. 4 and 5 (hierarchy flattening) → gate 6.
  2. 7(a) (TreeListWidget header-extra slot, ARCH-ratified) → then 7(b) and 10 together (both consume the new slot; 10's header composition is two controls in one slot, so building them in the same ticket avoids re-touching the row twice).
  3. 11 (selection-representation unification, ARCH-ratified) is a **prerequisite** for 13, not just sequenced-before-it — 13's own selection key is defined *as* the same `OverlayInstanceKey_UI` positional convergence 11 establishes. Land 11's `OverlayInstanceKey_UI`/`bManual` fix and the canvas-selection-widening **before** starting 13's implementation.
  4. 12 can land independently (touches only `DrawLayerRowBody`'s own instance-list block) any time after 6, but is worth doing alongside 13 since both add a "collapsible symmetry-cluster sub-list" shape to a very similar row and should share the same rendering helper rather than two near-duplicate implementations.

**ARCH ratifications required:**
- **7(a)** — new `TreeListWidget_UI` header-extra contract (two callbacks, not one — a deliberate divergence from `DraggableList`'s shape, worth recording explicitly).
- **10** — a new PARAMS field, `MarkerInstanceLayer::bSymmetryEnabled`, needs sign-off in its own right, separate from 7(a)'s widget contract.
- **11** — `OverlayInstanceKey_UI`'s new `bManual` field, `MapCanvas`'s widened public selection surface, and the new shell-mediated tab→canvas selection callback.
- **12** — the grouping UI shape, even with no PARAMS change.
- **13** — the per-frame positional-index mechanism and its explicit convergence with 11's key shape, plus the `symmetryIdentifier`-bucket-size grouping rule.

---

### Files read in full or in the relevant sections (for reference)
`work_orders/BRIEF_MarkersUICorrectionRound2_R1.md`; `src/ui/MarkersTab_Globals_UI.{h,cpp}`; `src/ui/SliderScalar_UI.{h,cpp}`, `SliderScalar_Track_UI.{h,cpp}`; `src/ui/ColorSwatch_UI.{h,cpp}`; `src/ui/MarkersTab_TypeSections_UI.{h,cpp}`; `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp`; `src/ui/MarkersTab_ManualLayers_UI.cpp`; `src/ui/MarkerLayerSymmetrySection_UI.cpp`; `src/ui/TreeListWidget_UI.h`, `TreeListWidget_RowLayout_UI.h`, `TreeListWidget_Types_UI.h`; `src/ui/DraggableListWidget_UI.h`, `DraggableListWidget_RowLayout_UI.h`, `DraggableListWidget_RowAffordances_UI.h`; `src/ui/MarkersTab_Bundles_UI.cpp`, `MarkersTab_BundleNodeBody_UI.cpp` (grep); `src/params/MarkerInstance_PARAMS.h`, `Symmetry_PARAMS.h`, `MarkerRule_PARAMS.h`, `GlobalMarkerSettings_PARAMS.h`; `src/ui/Application_UI.cpp`, `MapCanvas_UI.{h,cpp}`, `MapCanvas_Draw_UI.cpp`, `MapCanvas_IconLayer_CullEmit_UI.cpp`, `MapCanvas_IconLayer_CullManual_UI.cpp`, `MapCanvas_IconLayer_CullProcedural_UI.cpp`, `MapCanvas_IconLayer_Cull_UI.cpp`, `MapCanvas_MarkerRosterDraw_UI.cpp`, `Picking_UI.h`; `src/data/PlacementInstance_DATA.h`; `src/pipeline/GenerationAssembler_Stages_PIPELINE.cpp`; `src/proc/Placement_Manual_PROC.cpp`, `Placement_Accept_PROC.cpp`, `Placement_PROC.h/.cpp` (grep); `src/ui/ManualInstanceLayerIndex_UI.h`.

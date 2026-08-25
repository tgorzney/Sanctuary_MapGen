# STEP150 — Layer Editor row cleanup: bake toggle, field order, dead control removal

**Layer:** UI. **Domain:** `src/ui/LayerEditor_Group_UI.cpp`, `LayerEditor_Layer_UI.cpp`,
`LayerEditor_BakedImage_UI.h`/`.cpp`, `DraggableListWidget_UI.h`, `FilePathPicker_UI.h`/`.cpp`
(reuse only, no redesign), `Section_UI.h`/`.cpp` (reference for a design precedent, no edits
expected there).

## Root problem
Real-world testing of the Layer Editor against an imported map surfaced six concrete bugs/gaps,
all confirmed by direct code reading (not guessed):

1. **Bake/Unbake button never changes its label.** `DrawLayerRowActions`
   (`LayerEditor_Group_UI.cpp:46-52`) draws a static `ImGui::SmallButton("Bake / Unbake")` — it
   never reflects `layer.bBaked`. The human wants it to read `"Bake"` when the layer is currently
   unbaked and `"Unbake"` when it is currently baked.
2. **Bake/Unbake is in the wrong place.** It currently lives inside the expanded row body, on the
   same line as "Duplicate" (`LayerEditor_Group_UI.cpp:46-52`), reached only when that row is the
   selected/expanded one. The human wants it on the row **header** itself, right-aligned,
   immediately to the right of the existing `X##delete` affordance drawn by `DrawRowAffordances`
   (`DraggableListWidget_UI.h:106-118` — the `[o]/[-]` visibility, `[L]/[U]` lock, `X` delete strip
   that every `DraggableList` row already has). This must be visible on EVERY layer row without
   needing to expand it, so it needs to become a real per-row affordance, not something bolted on
   only when a row happens to be selected.
3. **"Group Stratum Index" is a dead control that confuses users.** `Params::GeoLayer::stratumIndex`
   (`GeoLayer_PARAMS.h:20`) has **zero PROC consumers** — grep confirms it is read/written only for
   IO round-trip (`MapExporter_HeightmapStack_IO.cpp:79`, `MapImporter_HeightmapStack_IO.cpp:96`).
   Only `Params::Layer::stratumIndex` (`Layer_PARAMS.h:19`) actually drives generation
   (`NoiseBlend_Prepare_PROC.cpp:68-70`, `NoiseBlend_PROC.cpp:56`, `NoiseBlend_Blend_PROC.cpp:44,48`).
   Drawing "Group Stratum Index" prominently next to the real, generation-driving "Stratum Index"
   (`LayerEditor_Scalars_UI.cpp:14-15` — the two labels sit side by side in the scalar catalogue)
   reads as two competing controls for the same concept when only one does anything. The human's
   own words: *"Layers should be the only Stratum Index."*
4. **Layer Name is drawn in the wrong position.** Current order in the row body
   (`LayerEditor_Group_UI.cpp:81-92` calling into `DrawLayerEditorLayerSections`,
   `LayerEditor_Layer_UI.cpp:90-91`): Import RAW/Duplicate/Bake action row → `Separator()` →
   **Name** → Stratum Index → Noise → Density → Height Blend. The human wants **Name at the top**,
   above the Import RAW row, with **Stratum Index on the same line as Name** (name field takes most
   of the row width, stratum-index label+control sits compactly to its right).
5. **Import RAW never shows which file is actually imported.** `DrawFilePathPicker` is called with
   `state.importRawPath` (`LayerEditor_Group_UI.cpp:39-45`) — a single **editor-wide scratch string**
   (`LayerEditor_UI.h:73`, documented there as "the last path the picker answered") shared across
   every group/layer in the whole editor, never initialized from the selected layer's own
   `layer.bakedImagePath` when that row is selected/drawn. So the picker shows whatever was last
   picked anywhere, not this layer's real import path, and does not reliably fall back to a genuine
   "None" for a layer that has no imported file. Fix: when drawing the row body for the
   currently-selected layer, sync the picker's bound path from `layer.bakedImagePath` before drawing
   it (empty string renders as "None" through the picker's existing `bStoredPathAllowed`/disabled-text
   path — verify this by reading `FilePathPicker_UI.cpp:30-57`, don't assume), and make sure a
   successful import (`ApplyImportRawAction`, `LayerEditor_BakedImage_UI.cpp:18-34`, which already
   writes `layer.bakedImagePath = action.importRawPath`) is reflected back into that same displayed
   state the next frame.
6. **Procedural settings render even when the layer is baked and inert.** `DrawLayerEditorLayerSections`
   (`LayerEditor_Layer_UI.cpp:83-96`) unconditionally draws Noise/Density/HeightBlend, and the row
   body (`LayerEditor_Group_UI.cpp:89-91`) unconditionally draws Soil and Erosion sections too — none
   gated on `layer.bBaked`. A baked layer's noise/density/height-blend/soil/erosion settings do
   nothing (verify: `NoiseBlend_PROC.cpp` skips rolling noise for a baked layer and reads the cached
   image instead) yet are shown as if live, which is misleading. The file's own comment at
   `LayerEditor_Layer_UI.cpp:20-23` already acknowledges this gap. Fix: gate each of Noise, Density,
   HeightBlend, Soil, and Erosion sections behind `if (!layer.bBaked) { ... }`. When baked, show a
   short static line instead (e.g. "Baked — procedural settings hidden. Unbake to edit.") so the
   space isn't just empty with no explanation.

## Fix approach
Read every file named above in full before editing — several of these bugs sit in the same
functions, so sequence the edits to avoid re-deriving state you already changed:

1. **Bake/Unbake as a real per-row header affordance.** Extend the shared `DraggableList` row
   affordance mechanism (`DraggableListWidget_UI.h`, `DrawRowAffordances` and whatever row-descriptor
   struct feeds it — read the real current struct, it may be `DraggableListRow` or similar) with an
   OPTIONAL extra per-row button: a label string + a click signal, rendered between the lock icon and
   the delete `X` (or immediately after `X`, matching "to the right of the X button" — read the
   existing `SameLine` chain in `DrawRowAffordances` and place it consistently with how visibility/
   lock/delete already chain). This must stay generic — other `DraggableList` consumers (Props,
   Decals, Markers, etc., all touched by STEP110) must be unaffected when they don't populate this
   optional field. This mirrors the STEP104 precedent of extending a shared widget
   (`Section_UI::SectionOptions::reservedRightWidth`) generically rather than hacking around it —
   read `Section_UI.h`/`.cpp` for that precedent's shape before designing this one.
   The Layer Editor's layer-row population code (wherever it builds the `DraggableList` rows for
   `group.layers`, likely in `LayerEditor_Group_UI.cpp`) sets this optional button's label to
   `layer.bBaked ? "Unbake" : "Bake"` and wires its click to the SAME action path the current
   `BakeToggleRequested` button uses (`RecordLayerEditorAction(..., LayerEditorActionKind::BakeToggleRequested, ...)`
   in `ApplyBakeToggleAction`, `LayerEditor_BakedImage_UI.cpp:40-59`) — do not duplicate that logic,
   just change where the button lives and how its label is computed.
   Remove the old static "Bake / Unbake" `SmallButton` from `DrawLayerRowActions` once the header
   affordance replaces it (keep "Duplicate" where it is).
2. **Group Stratum Index**: remove the `DrawLayerEditorIntegerRow(LayerEditorScalar::GroupStratumIndex, ...)`
   call from `DrawGroupSettings` (`LayerEditor_Group_UI.cpp:30-31`). Leave `Params::GeoLayer::stratumIndex`
   itself untouched (IO round-trip still needs the field) — this is a UI-only removal, not a data-model
   change. Do not touch the IO read/write sites.
3. **Reorder the row body**: in the `DraggableList<Params::Layer>::Render` row-body lambda
   (`LayerEditor_Group_UI.cpp:81-92`), draw Name + Stratum Index (as one row: name input wide,
   stratum-index control compact to its right — check whether `ImGui::SameLine()` with a fixed
   right-hand width, or a two-column layout, best matches how other same-line label+control rows in
   this codebase are already built, e.g. anything using `Section_UI`'s or `DraggableListWidget_UI`'s
   existing same-line patterns) BEFORE the Import RAW/Duplicate row and its `Separator()`. Then Noise/
   Density/HeightBlend/Soil/Erosion follow, each gated per point 6 above.
4. **Import RAW path sync**: fix per point 5 above.
5. **Procedural-section gating**: fix per point 6 above.

## Explicit out-of-scope
- The Bake/Unbake **data-safety** bug (unbake-then-rebake silently overwrites the original
  imported/decomposed pixels with a fresh live-noise snapshot, no restore path, no identity check on
  `flatIndex`) — this is a real, separate, higher-risk bug in `ApplyBakeToggleAction`
  (`LayerEditor_BakedImage_UI.cpp:40-59`) that needs a design decision (should re-baking an
  originally-imported layer restore the ORIGINAL import instead of re-snapshotting live noise?) —
  tracked separately, do not touch `ApplyBakeToggleAction`'s snapshot logic in this ticket beyond
  what's needed to relabel/reposition the button that calls it.
- A new per-layer "Disable" toggle (distinct from the existing visibility eye-icon) that fully skips
  generation for that layer — separate ticket.
- Any change to what Erosion/Thermal/FlowAccumulation compute or when they run — separate ticket.
- Any diagnostic/warning text explaining *why* a layer produced no visible output beyond the one
  static "Baked — procedural settings hidden" line specified above — separate ticket.

## Acceptance test
Extend the relevant `LayerEditor_*_UI_Test.cpp` (find the real existing test file(s) covering this
UI — check for a headless/state-only test harness pattern already used elsewhere in this codebase for
ImGui-adjacent logic, e.g. testing the row-affordance label/signal wiring without an actual render,
the way `FilesTab_ResetOnOpen_UI_Test.cpp` tests state transitions). Cover: bake-button label reflects
`bBaked` both ways; toggling it invokes the same `BakeToggleRequested` action as before; Group Stratum
Index control no longer renders (or is absent from whatever the test harness enumerates); the picker's
displayed path for a freshly-selected layer matches that layer's own `bakedImagePath`, including the
empty/"None" case; procedural sections are skipped when `bBaked == true`.

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.

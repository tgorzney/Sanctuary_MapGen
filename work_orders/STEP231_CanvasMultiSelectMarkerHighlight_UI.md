# STEP231 — Canvas never visually highlights any selected marker: two independent, unrelated gaps in the two draw passes

**Layer:** UI. **Domain:** the screen-space overlay icon-atlas draw pass's paint step (`MapCanvas_IconLayer_Draw_UI.cpp`/`MapCanvas_IconLayer_DrawInternal_UI.h`), the manual-marker dot/roster pass's highlight source (`MapCanvas_MarkerDrag_UI.cpp`, `MarkerSelectionHighlight_UI.h`/`.cpp`), `MapCanvas`'s own injected-selection-source plumbing (`MapCanvas_UI.h`, `Application_UI.cpp`), and the Alloy default select color (`GlobalMarkerSettings_PARAMS.h`). **Executor:** SanGen Coder. Authored by the SanGen UI Expert. Every file this ticket cites was read directly against the live tree while drafting it (post-STEP229/230, both confirmed already shipped: `git log` shows `866a078 "Marquee multi-select: fix Ctrl/Shift parity and canvas highlighting (STEP229/230)"` on this branch already).

## Session coordination (required before EVERY file edit, not just once at ticket start)
Multiple Claude Code sessions may be active on this machine concurrently, editing the SAME working directory. A single check at the start of this ticket is NOT sufficient — a peer can start editing any of this ticket's files at any point after your initial check. Before EACH individual file edit in §1-10 below (not just once, up front):
1. Call `ListAgents` to enumerate active/open peer sessions on this machine.
2. Message each one (`SendMessage`) naming the SPECIFIC file you are about to edit right now, asking if they are currently editing it or planning to.
3. Wait for replies before making that edit.
4. If a peer reports current or planned work in that exact file, do NOT edit concurrently — negotiate a sequential order (whichever session is further along lands and merges first; the other rebases onto that afterward) and record the agreed order in this ticket's own notes before proceeding.
5. If no peer claims that file, proceed with that one edit — then repeat steps 1-4 for the NEXT file before editing it. A "no conflict" answer for one file is not an answer for another, and an answer from earlier in the session is not an answer for right now — re-check per file, every time.

**Sibling-ticket note:** `STEP232` (drafted alongside this ticket) touches `MapCanvas_UI.h`/`MapCanvas_UI.cpp`/`Application_UI.cpp` too (`SetSelectionChangedCallback`'s signature and the `selectionChangedCallback` closure), but a DIFFERENT region of each — this ticket touches `SetManualMarkerSelectionSource`/`manualMarkerSelectedInstanceIdentifier` (a different method/field) and a different, earlier part of the `Application_UI.cpp` closure than STEP232's own edit. Re-diff both at merge time regardless; land whichever is further along first.

## Summary — two unrelated bugs, both live in the SAME symptom
There are TWO separate draw passes touching manual markers, both invoked unconditionally, every frame, from `MapCanvas::Draw` (`MapCanvas_Draw_UI.cpp:45-47`): the icon-atlas pass (`DrawOverlayIconLayerPass` → `DrawOverlayIconLayers`) and the dot/roster pass (`DrawManualMarkerDragPass` → `DrawManualMarkerRoster`). Confirmed by direct read of `Application_OverlaySetup_Seed_UI.cpp`, manual markers are seeded into `overlayLayers` with `OverlaySubLayerKind_UI::Manual` in production, so they are drawn by BOTH passes today — the roster's opaque `AddCircleFilled` dot draws AFTER (on top of) the icon-atlas pass in the SAME `Draw()` call, so for manual markers the roster dot is what the user actually sees; the icon-atlas pass's own tint is the one and only rendering for PROCEDURAL (rule-placed) markers, which have no roster/dot equivalent at all.

**Gap 1 — the icon-atlas pass computes `bSelected` correctly but never paints it (affects BOTH manual and procedural, but is masked for manual by the roster dot drawn on top).** `OverlayVisibleInstance::bSelected` is correctly computed against the full multi-select set (`MapCanvas_IconLayer_CullEmit_UI.cpp:74-75`, `instance.bSelected = input.selectedInstanceKeys != nullptr && SelectionSetContains(*input.selectedInstanceKeys, instance.instanceKey);` — confirmed STEP229 already shipped this correctly for every candidate, procedural and manual alike). But `bSelected` is used ONLY to route an instance into the C2 cache's selected/non-selected bucket split (`SplitSelected`, `MapCanvas_IconLayer_Draw_UI.cpp:20-26`) — it is **never read as a color/tint switch** anywhere in the actual paint code. Confirmed by full read of `FlushIconLayerBucket` (`MapCanvas_IconLayer_Draw_UI.cpp:87-112`): the ONLY place a per-instance color is computed is
```cpp
const ImU32 tint = ImGui::ColorConvertFloat4ToU32(
    ImVec4(instance.tintColorRed, instance.tintColorGreen, instance.tintColorBlue, instance.tintAlpha));
```
— `instance.bSelected` is never referenced anywhere in this function's body. Since procedural markers have NO other rendering path at all, this means **procedural markers get zero visual highlight, ever**, regardless of how many are selected.

**Gap 2 — the dot/roster pass IS the only place a genuinely different color gets painted for a selected manual marker, but it is fed a single scalar, not the real multi-select set.** `DrawManualMarkerDragPass` (`MapCanvas_MarkerDrag_UI.cpp:18-40`) computes its highlight via `ComputeManualMarkerSelectionHighlight(..., manualMarkerSelectedInstanceIdentifier != nullptr ? *manualMarkerSelectedInstanceIdentifier : -1)` — `manualMarkerSelectedInstanceIdentifier` is a single `const int*` (`MapCanvas_UI.h:417`), injected via `SetManualMarkerSelectionSource` (`MapCanvas_UI.h:187-189`), wired at `Application_UI.cpp:165` to `&tabState.markers.selectedManualInstanceIdentifier` — the pre-STEP141/pre-§21.1 "primary only" field. `ComputeManualMarkerSelectionHighlight` (`MarkerSelectionHighlight_UI.cpp`, read in full) resolves exactly that one primary plus its own symmetry-orbit siblings, nothing more. Net effect: box-select or Ctrl-click N manual markers, and at most 1 (+ its own symmetric siblings) gets `ManualMarkerRoster`'s special select-tint (`ResolveMarkerGroupSelectTintColor`, opaque full-fill-replacement, `MapCanvas_MarkerRosterDraw_UI.cpp:106-116`); the rest silently keep their ordinary type/layer tint.

**Consequence:** selecting N manual markers colors at most 1 (+ symmetry siblings); procedural markers never light up at all, no matter the selection.

## Required reading
`ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md` §19.18 ("selected replaces fill" as its own visual language, distinct from the drag-ghost's unfilled-ring vocabulary; the priority order this ticket does not disturb — refused-drag red still wins over everything). `ARCH_19_19_StaticHighlightComputationAndWiring.md` §19.19 (the single-scalar `SetManualMarkerSelectionSource` shape this ticket supersedes — see Interpretation call 1 for why superseding it needs no new ARCH ruling). `ARCH_19_17_SelectColorFields.md`/`GlobalMarkerSettings_PARAMS.h:25-31` (the four `selectColor*` fields, all defaulting to the SAME yellow `{1,1,0,1}` today, confirmed by direct read). `ARCH_14_09_RenderingPerformance.md` §14.9 (the C2 "never clustered/capped away" contract `bSelected` already correctly serves today and this ticket does not touch).

---

## 1. Modified: `src/ui/MapCanvas_IconLayer_DrawInternal_UI.h`

Add a new, non-anonymous (cross-TU-visible, so `MapCanvas_IconLayer_Draw_UI_Test.cpp` can reference it by name rather than duplicating the literal) constant. Insert immediately after the existing `kIconLayerBucketChunkQuadCap` declaration (currently line 26):
```cpp
constexpr int kIconLayerBucketChunkQuadCap = 16000;   // 16,000 quads = 64,000 vertices < 65,536
```
becomes
```cpp
constexpr int kIconLayerBucketChunkQuadCap = 16000;   // 16,000 quads = 64,000 vertices < 65,536

// STEP231 — the icon-atlas pass's own "selected" indicator. A LITERAL, matching
// GlobalMarkerSettings::selectColorAlloy's own new lime-green default (GlobalMarkerSettings_PARAMS.h)
// for a consistent "selected = green" visual language against the roster/dot pass's own select tint
// (Params::ResolveMarkerGroupSelectTintColor) — but NOT read from Params here: FlushIconLayerBucket
// is this module's one deliberately domain-agnostic choke point (serves Markers/Props/Decals/Units
// uniformly, confirmed by direct read of every call site in MapCanvas_IconLayer_Cull*_UI.cpp), and
// threading a per-marker-category selectColor* resolution all the way through
// EmitCandidateIfVisible/AppendCandidate would mean giving this function marker-domain knowledge it
// does not otherwise have. This mirrors the SAME file-family's own established precedent for a fixed,
// non-parameter-driven state-indicator color: MapCanvas_MarkerRosterDraw_UI.cpp's own
// refusedTint/ghostTint (two IM_COL32 literals for drag-refused-red/drag-ghost-grey, neither threaded
// through GlobalMarkerSettings either). A category-correct selectColor* threaded all the way through
// the cull/emit pipeline is a real, larger, separate follow-up (see this ticket's own Interpretation
// calls) — not invented here.
constexpr ImU32 kIconLayerSelectedTint = IM_COL32(51, 255, 51, 255);   // matches {0.2, 1.0, 0.2, 1.0}
```

---

## 2. Modified: `src/ui/MapCanvas_IconLayer_Draw_UI.cpp`

The one line in this ticket that changes runtime rendering behavior for every marker collection. Currently (`MapCanvas_IconLayer_Draw_UI.cpp:93-97`):
```cpp
            const OverlayVisibleInstance& instance = bucket.quads[chunk.quadStart + i];
            const float half = instance.screenSize * 0.5f;
            const ImU32 tint = ImGui::ColorConvertFloat4ToU32(
                ImVec4(instance.tintColorRed, instance.tintColorGreen, instance.tintColorBlue, instance.tintAlpha));
            const ImDrawIdx base = static_cast<ImDrawIdx>(drawList._VtxCurrentIdx);
```
Replace with:
```cpp
            const OverlayVisibleInstance& instance = bucket.quads[chunk.quadStart + i];
            const float half = instance.screenSize * 0.5f;
            // STEP231 — the actual fix: bSelected previously had ZERO visual effect anywhere in this
            // pass (it only ever routed an instance into the C2 cache's selected/non-selected bucket
            // split, MapCanvas_IconLayer_Draw_UI.cpp's own SplitSelected). ARCH §19.18's own "selected
            // replaces fill, full opacity" visual language (ratified for the roster/dot pass) is
            // applied here too — kIconLayerSelectedTint is already full alpha (255), so a selected
            // instance in a low-opacity layer is not left faint the way multiplying instance.tintAlpha
            // in would leave it.
            const ImU32 tint = instance.bSelected
                ? kIconLayerSelectedTint
                : ImGui::ColorConvertFloat4ToU32(
                      ImVec4(instance.tintColorRed, instance.tintColorGreen, instance.tintColorBlue, instance.tintAlpha));
            const ImDrawIdx base = static_cast<ImDrawIdx>(drawList._VtxCurrentIdx);
```
No other line in this file changes. `RebuildAndCache`'s own `SplitSelected(budgeted, nonSelected, selected)` call (line 50) and its two separate `FlushBuckets` calls (cached non-selected bytes, line 53; live selected bytes, line 54) are untouched — selected instances already ALWAYS flow through `FlushIconLayerBucket` fresh every frame (never from cached raw vertex bytes), so this tint override applies correctly on every frame with zero cache-invalidation interaction.

---

## 3. Modified: `src/ui/MapCanvas_IconLayer_Draw_UI_Test.cpp`

Add a new check function, mirroring `CheckBucketingProducesOnePageOneCommand`'s own established style exactly (same file, same `MakeQuad`/`check` helpers already in this anonymous namespace, no new includes). Insert immediately after `CheckBucketingProducesOnePageOneCommand` (currently ending line 70), before `CheckFullPipelineEmitsADrawCommand`:
```cpp
// STEP231 — the actual bug fix: FlushIconLayerBucket must override a bSelected instance's tint to
// kIconLayerSelectedTint regardless of its own resolved tintColorRed/Green/Blue, and must leave an
// UNSELECTED instance's own resolved tint completely alone. Inspects drawList.VtxBuffer directly
// (no ImGui::Render()/GetDrawData() round-trip needed — FlushIconLayerBucket writes straight into the
// passed-in ImDrawList via PrimWriteVtx) for the exact per-vertex color each of the two quads produced.
void CheckFlushIconLayerBucketAppliesSelectedTintOverride() {
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("IconLayerSelectedTintTestWindow");
    ImDrawList& drawList = *ImGui::GetWindowDrawList();

    OverlayVisibleInstance unselected = MakeQuad(0, 555ull);
    unselected.tintColorRed = 1.0f; unselected.tintColorGreen = 0.0f; unselected.tintColorBlue = 0.0f;
    unselected.tintAlpha = 1.0f; unselected.bSelected = false;
    OverlayVisibleInstance selected = unselected;
    selected.bSelected = true;   // deliberately keeps the SAME (red) resolved tint as unselected above —
                                 // proves the override is driven by bSelected, not by a coincidentally
                                 // different tintColorRed/Green/Blue value.

    AtlasPageBucket bucket;
    bucket.atlasPage = 0; bucket.textureIdentifier = 555ull;
    bucket.quads.push_back(unselected);
    bucket.quads.push_back(selected);

    FlushIconLayerBucket(drawList, bucket);
    check(drawList.VtxBuffer.Size == 8, "two quads write exactly 8 vertices (4 each), no more, no less");

    const ImU32 expectedUnselectedTint = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    bool bUnselectedCorrect = true, bSelectedCorrect = true;
    for (int i = 0; i < 4; ++i) if (drawList.VtxBuffer[i].col != expectedUnselectedTint) bUnselectedCorrect = false;
    for (int i = 4; i < 8; ++i) if (drawList.VtxBuffer[i].col != kIconLayerSelectedTint) bSelectedCorrect = false;
    check(bUnselectedCorrect, "an UNSELECTED instance keeps its own resolved per-instance tint, unchanged");
    check(bSelectedCorrect,
          "STEP231 - a SELECTED instance's quad is overridden to kIconLayerSelectedTint (bright lime "
          "green) regardless of its own resolved tintColorRed/Green/Blue - this is the actual fix: "
          "bSelected previously had ZERO visual effect anywhere in this pass, which meant procedural "
          "(rule-placed) markers never showed any highlight at all, no matter the selection");

    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();
}
```
Register it in `RunMapCanvasIconLayerDrawChecks` (currently lines 105-108):
```cpp
void RunMapCanvasIconLayerDrawChecks() {
    CheckBucketingProducesOnePageOneCommand();
    CheckFullPipelineEmitsADrawCommand();
}
```
to
```cpp
void RunMapCanvasIconLayerDrawChecks() {
    CheckBucketingProducesOnePageOneCommand();
    CheckFlushIconLayerBucketAppliesSelectedTintOverride();
    CheckFullPipelineEmitsADrawCommand();
}
```
No `CMakeLists.txt` change — this file is already wired into the `MapCanvas_IconLayer_UI_Test` target.

---

## 4. Modified: `src/params/GlobalMarkerSettings_PARAMS.h`

Currently (line 28):
```cpp
    float selectColorAlloy[4]   = {1.0f, 1.0f, 0.0f, 1.0f};
```
Replace with:
```cpp
    // STEP231 — human's own bug report: the default selection tint (identical to every OTHER
    // selectColor* default — see the three lines below, all still {1,1,0,1}) was a plain yellow, not
    // visually distinct enough from a legitimate in-map color to read clearly as "selected." Changed
    // to a bright/lime green — a color no existing GlobalMarkerSettings field defaults to
    // (colorAlloy/Plasma/Spawn are yellow/teal/red, lines 18-20 above, confirmed by direct read), so
    // "selected" reads unambiguously against every one of them, and matches the SAME lime-green
    // literal the icon-atlas pass's own kIconLayerSelectedTint now uses
    // (MapCanvas_IconLayer_DrawInternal_UI.h) for a consistent cross-pass visual language.
    // selectColorPlasma/Spawn/Default are DELIBERATELY UNCHANGED (still yellow): the human's own
    // report named Alloy specifically; there is no confirmed bug against the other three, so changing
    // them too would be unrequested scope creep (see this ticket's own Interpretation calls) — the
    // underlying ResolveMarkerGroupSelectTintColor mechanism itself already works correctly for all
    // four (GlobalMarkerSettings_PARAMS_Test.cpp already exercises every one with distinct non-default
    // values), so this is purely a default-VALUE change, not a mechanism fix.
    float selectColorAlloy[4]   = {0.2f, 1.0f, 0.2f, 1.0f};
```
No other line in this file changes. Confirmed safe against every existing test that touches `selectColorAlloy` (`GlobalMarkerSettings_PARAMS_Test.cpp`, `MapCanvas_MarkerDrag_UI_Test.cpp`, `MapImporter_IO_Test.cpp`) — every one of them sets a CUSTOM, non-default value explicitly before asserting against it; none assert the literal default.

---

## 5. Modified: `src/ui/MarkerSelectionHighlight_UI.h`

Add a new declaration, the multi-select counterpart of the existing single-instance function. Insert immediately before the closing `} // namespace Ui` (currently line 31):
```cpp
std::vector<int> ComputeManualMarkerSelectionHighlight(
    const std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
    float distanceTolerance, int selectedInstanceIdentifier);

} // namespace Ui
```
becomes
```cpp
std::vector<int> ComputeManualMarkerSelectionHighlight(
    const std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
    float distanceTolerance, int selectedInstanceIdentifier);

// STEP231 — the multi-select counterpart: unions ComputeManualMarkerSelectionHighlight's own
// single-instance result (the selected instance plus its own orbit siblings) across every id in
// `selectedInstanceIdentifiers`, de-duplicated (a later id's own orbit can legitimately re-discover an
// EARLIER id's own siblings — e.g. two siblings of the same symmetric pair both individually
// selected). Delegates entirely to the existing single-instance primitive, per id, in order, exactly
// once each — no duplicated matching/orbit logic (mirrors STEP230's own
// "loop over the existing single-key primitive" precedent, ToggleEachInSelectionSet).
std::vector<int> ComputeManualMarkerMultiSelectionHighlight(
    const std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
    float distanceTolerance, const std::vector<int>& selectedInstanceIdentifiers);

} // namespace Ui
```

---

## 6. Modified: `src/ui/MarkerSelectionHighlight_UI.cpp`

Add the definition. Insert immediately before the closing `} // namespace Ui` (currently line 59):
```cpp
    return result;
}

} // namespace Ui
```
becomes
```cpp
    return result;
}

std::vector<int> ComputeManualMarkerMultiSelectionHighlight(
        const std::vector<Params::MarkerInstanceGroup>& markers,
        const std::vector<Params::MarkerInstanceLayer>& markerLayers,
        const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
        float distanceTolerance, const std::vector<int>& selectedInstanceIdentifiers) {
    std::vector<int> result;
    for (int selectedInstanceIdentifier : selectedInstanceIdentifiers) {
        const std::vector<int> perInstance = ComputeManualMarkerSelectionHighlight(
            markers, markerLayers, geometry, globalSymmetryMask, globalRadialRepeatCount,
            distanceTolerance, selectedInstanceIdentifier);
        for (int identifier : perInstance) {
            bool bAlreadyPresent = false;
            for (int existing : result) if (existing == identifier) { bAlreadyPresent = true; break; }
            if (!bAlreadyPresent) result.push_back(identifier);
        }
    }
    return result;
}

} // namespace Ui
```

---

## 7. Modified: `src/ui/MarkerSelectionHighlight_UI_Test.cpp`

Add a new check function, mirroring this file's own established style exactly (same `Check`/`Contains`/`MakeTransform`/`MakeTestGeometry` helpers). Insert immediately before the closing `} // namespace` (currently line 146):
```cpp
// STEP231 — ComputeManualMarkerMultiSelectionHighlight: empty input, disjoint union, de-duplication
// when two mutual mirror-siblings are BOTH individually selected, and stale-id tolerance.
void RunMultiSelectionUnionChecks() {
    const Params::Geometry geometry = MakeTestGeometry();

    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].transforms.push_back(MakeTransform(1, 2.0f, 2.0f));
        const std::vector<int> result = ComputeManualMarkerMultiSelectionHighlight(
            markers, kNoLayers, geometry, Params::SymmetryAxis::None, 3, 0.5f, std::vector<int>{});
        Check(result.empty(), "an empty selected-identifier list returns an empty highlight set");
    }
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].transforms.push_back(MakeTransform(40, 1.0f, 1.0f));
        markers[0].transforms.push_back(MakeTransform(41, 9.0f, 9.0f));
        const std::vector<int> result = ComputeManualMarkerMultiSelectionHighlight(
            markers, kNoLayers, geometry, Params::SymmetryAxis::None, 3, 0.5f, std::vector<int>{40, 41});
        Check(Contains(result, 40) && Contains(result, 41) && static_cast<int>(result.size()) == 2,
              "two independently-selected instances with no siblings union to exactly both, no loss");
    }
    // Selecting BOTH sides of a mirrored pair must not double-count: instance 50's own orbit already
    // discovers sibling 51, and instance 51's own orbit (run independently) rediscovers 50 — the
    // union must de-duplicate down to exactly {50, 51}.
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].transforms.push_back(MakeTransform(50, 3.0f, 5.0f));
        markers[0].transforms.push_back(MakeTransform(51, 7.0f, 5.0f));   // exact mirror of 50
        const std::vector<int> result = ComputeManualMarkerMultiSelectionHighlight(
            markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, std::vector<int>{50, 51});
        Check(Contains(result, 50) && Contains(result, 51) && static_cast<int>(result.size()) == 2,
              "selecting BOTH mirror siblings de-duplicates to exactly {50, 51}, not a 4-element list");
    }
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].transforms.push_back(MakeTransform(60, 2.0f, 2.0f));
        const std::vector<int> result = ComputeManualMarkerMultiSelectionHighlight(
            markers, kNoLayers, geometry, Params::SymmetryAxis::None, 3, 0.5f, std::vector<int>{999, 60});
        Check(Contains(result, 60) && static_cast<int>(result.size()) == 1,
              "a stale id contributes nothing (Constitution Sec6, never a crash); the valid id still resolves");
    }
}
```
Register it in `main()` (currently lines 148-155):
```cpp
int main() {
    RunNoSelectionChecks();
    RunStaleIdentifierChecks();
    RunNoneAxisSingleResultChecks();
    RunNeverDraggedSiblingMatchChecks();
    RunDifferentGroupNotMatchedChecks();
    RunToleranceBoundaryChecks();
```
to
```cpp
int main() {
    RunNoSelectionChecks();
    RunStaleIdentifierChecks();
    RunNoneAxisSingleResultChecks();
    RunNeverDraggedSiblingMatchChecks();
    RunDifferentGroupNotMatchedChecks();
    RunToleranceBoundaryChecks();
    RunMultiSelectionUnionChecks();
```
No `CMakeLists.txt` change — this is an existing single-file test target.

---

## 8. Modified: `src/ui/MapCanvas_MarkerDrag_UI.cpp`

`DrawManualMarkerRoster`'s own signature (`MapCanvas_MarkerRosterDraw_UI.cpp`) is completely UNCHANGED by this ticket — it already takes a plain `std::vector<int>` of highlighted identifiers. Only this ONE call site changes what it computes that vector FROM. Currently (`MapCanvas_MarkerDrag_UI.cpp:18-40`):
```cpp
void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    static const std::vector<Params::Army> kNoArmies;
    static const Params::GlobalMarkerSettings kDefaultGlobalMarkerSettings;
    static const Params::MarkerSymmetryFixSettings kDefaultMarkerSymmetryFixSettings;
    // STEP126 — recomputed fresh every frame (ARCH §19.19), discarded after this draw call. Null-safe:
    // no selection source wired -> -1 -> ComputeManualMarkerSelectionHighlight returns empty.
    const std::vector<int> selectedHighlight = (manualMarkerDragGeometry != nullptr)
        ? ComputeManualMarkerSelectionHighlight(*manualMarkerDragMarkers,
              manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers, *manualMarkerDragGeometry,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalSymmetryMask : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->radialSymmetryRepeatCount : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerSymmetryFixSettings.distanceTolerance
                                                 : kDefaultMarkerSymmetryFixSettings.distanceTolerance,
              manualMarkerSelectedInstanceIdentifier != nullptr ? *manualMarkerSelectedInstanceIdentifier : -1)
        : std::vector<int>{};
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalMarkerSettings : kDefaultGlobalMarkerSettings,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          selectedHighlight, *ImGui::GetWindowDrawList());
}
```
Replace with:
```cpp
void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    static const std::vector<Params::Army> kNoArmies;
    static const Params::GlobalMarkerSettings kDefaultGlobalMarkerSettings;
    static const Params::MarkerSymmetryFixSettings kDefaultMarkerSymmetryFixSettings;
    // STEP231 — sourced directly from THIS class's own canonical multi-select set
    // (selectedInstanceKeys, ARCH §21.1, MapCanvas_UI.h:377), not the retired injected single-scalar
    // pointer (SetManualMarkerSelectionSource/manualMarkerSelectedInstanceIdentifier — see this
    // ticket's own Interpretation calls for why: that mechanism predates §21.1's ordered set and had
    // become a stale second copy of data this class already owns as ground truth, the exact "one
    // source of truth, never a second copy" principle every OTHER canvas.Set*Source call already
    // follows). Every manually-selected marker's own symmetry orbit is unioned
    // (ComputeManualMarkerMultiSelectionHighlight), not just the MRU primary's.
    std::vector<int> selectedManualInstanceIdentifiers;
    for (const OverlayInstanceKey_UI& key : selectedInstanceKeys.keys)
        if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
            selectedManualInstanceIdentifiers.push_back(key.instanceIndex);
    const std::vector<int> selectedHighlight = (manualMarkerDragGeometry != nullptr)
        ? ComputeManualMarkerMultiSelectionHighlight(*manualMarkerDragMarkers,
              manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers, *manualMarkerDragGeometry,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalSymmetryMask : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->radialSymmetryRepeatCount : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerSymmetryFixSettings.distanceTolerance
                                                 : kDefaultMarkerSymmetryFixSettings.distanceTolerance,
              selectedManualInstanceIdentifiers)
        : std::vector<int>{};
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalMarkerSettings : kDefaultGlobalMarkerSettings,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          selectedHighlight, *ImGui::GetWindowDrawList());
}
```
`OverlayInstanceKey_UI`/`PlacementCollectionKind_UI` are already visible in this translation unit transitively (`MapCanvas_UI.h` → `MapCanvas_SelectionSet_UI.h` → `MapCanvas_IconLayer_UI.h`, the same chain STEP229 already established) — no new `#include` needed. `#include "MarkerSelectionHighlight_UI.h"` is already present (line 10, unchanged).

---

## 9. Modified: `src/ui/MapCanvas_UI.h`

**9a.** Delete `SetManualMarkerSelectionSource` entirely (currently lines 180-189):
```cpp
    // STEP126 — the static selection-highlight source: `selectedInstanceIdentifier` is the SAME
    // address as MarkersTabState::selectedManualInstanceIdentifier (Application_UI.cpp) — one source
    // of truth, never a second copy. A single scalar pointer, the simplest form of this file's own
    // established null-safe-injection shape (ARCH §19.19 — closer to SetActivePanelSource's
    // one-pointer form than SetManualMarkerDragSource's bundle). Null (no shell has wired a selection
    // source) refuses — the highlight computation treats null identically to "-1: nothing selected,"
    // never defaulting to "everything selected."
    void SetManualMarkerSelectionSource(const int* selectedInstanceIdentifier) {
        manualMarkerSelectedInstanceIdentifier = selectedInstanceIdentifier;
    }

    // STEP133 — the Markers tab's per-Type Hide/Unhide preview filter source. Mirrors
```
Replace with:
```cpp
    // STEP231 — SetManualMarkerSelectionSource (STEP126's original single-scalar injection) is
    // RETIRED: DrawManualMarkerDragPass now reads this class's OWN canonical selectedInstanceKeys
    // (ARCH §21.1) directly instead — see MapCanvas_MarkerDrag_UI.cpp's own comment for why. This
    // predates §21.1's ordered multi-select set (STEP126 shipped before it existed), and had become a
    // stale second copy of data this class already owns as ground truth. ARCH_19_19_
    // StaticHighlightComputationAndWiring.md's own text, which explicitly ratified this exact
    // single-scalar mechanism, goes stale by this retirement — flagged for the ARCH Expert's own
    // documentation-sync pass, not authored here (see this ticket's own Explicit out-of-scope,
    // mirroring STEP214's identical precedent for ARCH_21_08's own stale example line).
    // STEP133 — the Markers tab's per-Type Hide/Unhide preview filter source. Mirrors
```

**9b.** Re-verify the exact live location of a stale doc-comment reference to `SetManualMarkerSelectionSource`'s "existing injection pattern" (a phrase used elsewhere in this header to describe a DIFFERENT field's own convention by analogy) — `grep -n "SetManualMarkerSelectionSource"` the live file immediately before editing §9a above, and if any OTHER comment in this file (outside the two blocks in §9a/§9d) references it by name, update that one occurrence too to say "this file's own established push-in-pointer convention" or equivalent, zero behavior change. Do not guess the line number from this ticket text — confirm fresh.

**9c.** Delete the field (currently lines 416-417):
```cpp
    // STEP126 — the static selection-highlight source (injected, see SetManualMarkerSelectionSource).
    const int*                                      manualMarkerSelectedInstanceIdentifier = nullptr;
    // STEP133 — the per-Type Hide/Unhide preview filter source (injected, see
```
Replace with:
```cpp
    // STEP231 — the STEP126 single-scalar selection-highlight source (manualMarkerSelectedInstanceIdentifier)
    // is retired; DrawManualMarkerDragPass now reads this class's own selectedInstanceKeys directly.
    // STEP133 — the per-Type Hide/Unhide preview filter source (injected, see
```

---

## 10. Modified: `src/ui/Application_UI.cpp`

Remove the now-dead wiring call. Currently (lines 161-165):
```cpp
    // STEP126 — the static selection-highlight source; see MapCanvas_UI.h's
    // SetManualMarkerSelectionSource. Points at the SAME MarkersTabState field the Markers tab's own
    // instance-list rows write (tabState.markers.selectedManualInstanceIdentifier) — one source of
    // truth, never a second copy.
    canvas.SetManualMarkerSelectionSource(&tabState.markers.selectedManualInstanceIdentifier);
    // STEP133 — the per-Type Hide/Unhide preview filter source; see MapCanvas_UI.h's
```
Replace with:
```cpp
    // STEP231 — SetManualMarkerSelectionSource is retired; MapCanvas now reads its own
    // selectedInstanceKeys directly for the roster/dot pass's highlight (see MapCanvas_MarkerDrag_UI.cpp
    // and MapCanvas_UI.h's own retirement comments). tabState.markers.selectedManualInstanceIdentifier
    // itself is UNCHANGED and still live — it still drives the Markers-tab LIST's own row highlight
    // (DrawManualInstanceRow), which is a separate concern this ticket does not touch.
    // STEP133 — the per-Type Hide/Unhide preview filter source; see MapCanvas_UI.h's
```
No other line in this file changes. `tabState.markers.selectedManualInstanceIdentifier` itself is NOT removed or renamed — confirmed still required by `MarkersTab_ManualLayerRowBody_UI.cpp:118`/`MarkersTab_ManualLayers_UI.cpp:40`/`MarkersTab_UI.cpp:202` for the list's own row-highlight machinery, entirely unrelated to this ticket.

---

## ARCH rules invoked
- `ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md` §19.18 — "selected replaces fill... full opacity," applied here to the icon-atlas pass as the SAME already-decided visual language the roster pass already implements, not a new policy invented by this ticket (mirrors STEP214's own "already-ratified math, additional trigger" posture).
- `ARCH_19_19_StaticHighlightComputationAndWiring.md` §19.19 — superseded, not contradicted: its ratified computation (`ComputeManualMarkerSelectionHighlight`'s own orbit-matching logic) is REUSED verbatim by the new multi-select wrapper (§5/§6 above), unmodified; only the SOURCE feeding it (single injected scalar → this class's own canonical set) changes, exactly mirroring STEP229's own precedent one section over (§21.1) for the sibling icon-atlas pass's `selectedInstanceKeys` field. Flagged as documentation-sync territory for the ARCH Expert, not a new boundary invented here (see Explicit out-of-scope).
- `ARCH_14_09_RenderingPerformance.md` §14.9 — the C2 "never clustered/capped away" contract `bSelected` already correctly serves; this ticket adds a paint-time consumer of the SAME already-correct flag, changing no cull/budget/cache logic.
- Constitution §1.5/§6 — every new function this ticket adds delegates to an already-correct existing primitive rather than duplicating logic (`ComputeManualMarkerMultiSelectionHighlight` loops the existing single-instance function; the icon-atlas tint override reuses the already-correct `bSelected` flag).

## Explicit out-of-scope
- **No category-correct `selectColor*` threading through the icon-atlas cull/emit pipeline** (`EmitCandidateIfVisible`/`AppendCandidate`/`ResolveMarkersManual`/`ResolveProceduralSubLayer` and their ~4 call sites). `kIconLayerSelectedTint` is a single fixed literal, uniform across every marker category and every domain (Markers/Props/Decals/Units) — a real, larger follow-up if per-category selection-color differentiation on the icon-atlas pass is ever wanted (see Interpretation call 3).
- **No fix to the pre-existing double-render of manual markers** (both the icon-atlas pass AND the roster/dot pass draw every manual marker today, confirmed by direct read of `Application_OverlaySetup_Seed_UI.cpp`'s `OverlaySubLayerKind_UI::Manual` seeding) — pre-existing, unrelated to this ticket's confirmed bug, and changing it is a rendering-architecture decision, not a selection-highlight one.
- **No touch to `ARCH_19_19_StaticHighlightComputationAndWiring.md`** — this agent never writes ARCH files. Its text describing `SetManualMarkerSelectionSource`'s single-scalar shape as ratified becomes stale prose once this ships; re-syncing ARCH prose to shipped code is the ARCH Expert's own call (mirrors STEP214's identical precedent for `ARCH_21_08_AreaCanvasGesture.md:202`'s own stale example line).
- **No change to `selectColorPlasma`/`selectColorSpawn`/`selectColorDefault`'s own defaults** — only `selectColorAlloy`, per the human's own explicit, specific report. The underlying mechanism (`ResolveMarkerGroupSelectTintColor`) is confirmed already correct and already fully tested for all four fields; nothing there needed repair.
- **No change to `ARCH_19_18`'s own priority order** (refused-drag red > selected > army color > type color) — this ticket's icon-atlas tint override sits BELOW the roster pass in z-order (the roster dot still draws last/on top for manual markers) and does not interact with or reorder that priority chain; it only gives PROCEDURAL markers (which have no roster pass at all) their first-ever highlight.
- **No fix to `tabState.markers.selectedManualInstanceIdentifier`'s (singular) own sync mechanism** — untouched, still wired exactly as before, still drives the Markers-tab LIST's own row highlight. The Shift-range ANCHOR clobber affecting THAT same closure is a separate, already-identified bug — see `STEP232_ManualListShiftRangeAnchorClobber_UI.md`.

## Acceptance test
1. `MapCanvas_IconLayer_UI_Test` (`ctest` binary) passes `ALL PASS`, including the new `CheckFlushIconLayerBucketAppliesSelectedTintOverride`: a selected quad's vertices carry `kIconLayerSelectedTint` regardless of its own resolved tint; an unselected quad's vertices are unaffected.
2. `MarkerSelectionHighlight_UI_Test` (`ctest` binary) passes `ALL PASS`, including the new `RunMultiSelectionUnionChecks`: empty input, disjoint union, mutual-mirror-sibling de-duplication, stale-id tolerance.
3. `GlobalMarkerSettings_PARAMS_Test` continues to pass unmodified (every existing assertion uses custom, non-default values).
4. Every pre-existing case in every touched test file continues to pass unmodified.
5. Full `SanGenV2` build stays clean; every existing test in the suite continues to pass.

## Interpretation calls made
1. **Retiring `SetManualMarkerSelectionSource`/`manualMarkerSelectedInstanceIdentifier` needs no new ARCH ruling.** ARCH §19.19's own text already generalizes its ratified shape as "a caller-injected pointer (OR pointer bundle)" — this ticket does not widen the injected shape at all; it goes one step further and eliminates the injection entirely, since `MapCanvas` (the consumer, `DrawManualMarkerDragPass`) is ALSO the class that already owns `selectedInstanceKeys` as its own member (ARCH §21.1, which did not exist when §19.19 was ratified). This mirrors STEP229's own already-accepted precedent one section over: STEP229 replaced `DrawOverlayIconLayersInput::selectedInstanceKey` (a narrowed single-key COPY) with a pointer directly at `MapCanvas`'s own `selectedInstanceKeys` member, for the icon-atlas pass. This ticket does the identical move for the roster/dot pass — the sibling this ticket exists to fix.
2. **The icon-atlas tint-override, not a widened roster/dot pass, is the fix for PROCEDURAL markers.** The dot/roster pass has zero concept of procedural markers at all — it only ever iterates `Params::MarkerInstanceGroup`/`MarkerTransform` (the manual roster); procedural markers live in `Data::PlacementInstances`, an unrelated SoA data model. Giving the roster pass that capability would mean re-implementing a large fraction of `ResolveMarkersManual`'s procedural-cull sibling in a totally different code path — a large, duplicative undertaking for zero benefit, since the icon-atlas pass ALREADY has fully correct `bSelected` data for procedural markers today (STEP229) and only needed a ~10-line paint-time consumer.
3. **The icon-atlas override uses a fixed literal, not a threaded per-category `selectColor*`.** Threading category-correct select tint through `EmitCandidateIfVisible`/`AppendCandidate` would require widening two shared, domain-agnostic function signatures and touching every one of their ~4 call sites (manual markers, procedural markers, and by extension Props/Decals/Units, which have no `selectColor*` concept at all — confirmed by direct read, `GlobalScatterSettings_PARAMS.h:7`: "no `selectColor*` fields yet, no selection-highlight consumer exists for Props/Decals data"). This is a real, larger, separate architectural change; the fixed literal fully satisfies the confirmed bug ("procedural markers get SOME visual highlight") at a fraction of the blast radius, and reuses this exact codebase's own established `refusedTint`/`ghostTint` literal-constant precedent one file over.
4. **The roster/dot pass draws AFTER (visually on top of) the icon-atlas pass for manual markers** (confirmed by direct read of `MapCanvas_Draw_UI.cpp:45-47`'s own call order) — this is why BOTH halves of this ticket are necessary: the icon-atlas fix alone would be invisible for manual markers (masked by the roster dot drawn on top of it), and the roster fix alone leaves procedural markers with zero highlight (no roster pass exists for them at all).
5. **`selectColorAlloy`'s default changes; `selectColorPlasma`/`selectColorSpawn`/`selectColorDefault` do not.** The human's own report named Alloy specifically; all four share today's default only coincidentally (confirmed by direct read, `GlobalMarkerSettings_PARAMS.h:28-31`, all four literally `{1,1,0,1}`), and there is no confirmed bug report against the other three — changing them too would be unrequested scope creep.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_DrawInternal_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullEmit_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Draw_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_MarkerDrag_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_MarkerRosterDraw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_MarkerDrag_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkerSelectionHighlight_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkerSelectionHighlight_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkerSelectionHighlight_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_OverlaySetup_Seed_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\params\GlobalMarkerSettings_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\GlobalMarkerSettings_PARAMS_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\ARCH_19_17_SelectColorFields.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_19_19_StaticHighlightComputationAndWiring.md`,
and `work_orders\STEP214_AreaAltCenterResizeModifier_UI.md`/`STEP229_MarqueeMultiSelectHighlight_UI.md`/`STEP230_MarqueeCtrlToggleShiftUnion_UI.md` (structural/rigor templates and session-coordination wording, per the dispatching instruction).

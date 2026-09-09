# STEP259 — Group/Layer header click: recursive instance selection + canvas sync wiring

**Layer:** UI. **Domain:** `src/ui/MarkersTab_Bundles_UI.h`, `src/ui/MarkersTab_BundleTreeSignals_UI.cpp`
(Bundle tree's `ApplyMarkerLayerBundleTreeSignal`), `src/ui/MarkersTab_Bundles_UI.cpp` (call site),
`src/ui/MarkersTab_ManualLayers_UI.h`/`.cpp` (flat list's `ApplyLayerListSignal`), plus their two
existing acceptance-test binaries `MarkersTab_Bundles_UI_Test.cpp`/`MarkersTab_ManualLayers_UI_Test.cpp`.
**Executor:** SanGen Coder. Pure UI-wiring fix — no PARAMS/IO/PIPELINE change, no new fields.

## The three bugs (all independently re-verified against live code this ticket)

**Bug 1** — clicking a Group header does not select its instances. `ApplyMarkerLayerBundleTreeSignal`
(`src/ui/MarkersTab_BundleTreeSignals_UI.cpp:34-56`) branches on `TreeListSignalKind::Select`:
`TreeNodeSourceKind::Node` (line 35-36) only sets `state.selectedBundleIdentifier`; `TreeNodeSourceKind::Leaf`
(lines 37-55) already clears/repopulates `selectedManualInstanceIdentifiers` from
`instanceIndex.instancesByLayerIndex`. Confirmed: the Node branch has no equivalent — grep-confirmed no
other write to `selectedManualInstanceIdentifiers` exists anywhere on the Node path.

**Bug 2** — selecting a Layer (flat list OR Bundle tree) does not highlight in the preview. Confirmed:
`MapCanvas::SyncManualMarkerSelection` (`src/ui/MapCanvas_UI.cpp:128-168`, contract documented
`MapCanvas_UI.h:230-286`) is the only method that keeps the canvas's own independent
`selectedInstanceKeys` in sync with a list-driven selection, and it is only ever reachable through the
`selectManualMarkerInstanceCallback` closure Application wires at `src/ui/Application_UI.cpp:218-221`.
Both `ApplyMarkerLayerBundleTreeSignal`'s Leaf branch (`MarkersTab_BundleTreeSignals_UI.cpp:37-55`) and
`ApplyLayerListSignal`'s Select branch (`src/ui/MarkersTab_ManualLayers_UI.cpp:27-42`) populate the three
tabState fields (`selectedManualInstanceIdentifier(s)`/anchor) but never call this callback — confirmed
neither function even takes it as a parameter today, despite both of their own callers
(`DrawMarkerLayerBundleTree`, `MarkersTab_Bundles_UI.cpp:108-121`, and `DrawManualMarkerLayerListBody`,
`MarkersTab_ManualLayers_UI.cpp:233-245`) already receiving it as their own parameter and using it
elsewhere in the same function (`DrawMarkerGroupLeafBody`/`DrawLayerList` calls, `MarkersTab_Bundles_UI.cpp:47`,
`MarkersTab_ManualLayers_UI.cpp:192`).

**Bug 3 (folded into Bug 1's fix)** — the open design question: does Group-wide selection scope to direct
children only, or recurse into nested sub-Groups? Resolved below with real precedent, not a guess.

## Design decision 1 — Group selection scope: RECURSIVE, Manual-only

`Params::CollectMarkerLayerBundleRecursiveManualMembers` (`src/params/MarkerLayerBundleQuery_PARAMS.h:78-104`)
already exists and is already the resolver `ApplyMarkerLayerBundleMove`/`ApplyMarkerLayerBundleRotation`
(`src/ui/MarkersTab_BundleNodeBody_UI.cpp:21-32`, `:34-64`) use for "act on everything under this Group" —
confirmed both call it directly, and its own header comment (`:69-77`) states it is "MANUAL ONLY,
deliberately excludes Procedural layers" and walks descendants via
`CollectMarkerLayerBundleDescendantIdentifiers` (`:25-40`), which is explicitly recursive (walks
`parentBundleIdentifier` child links transitively, cycle-safe).

This is the governing precedent, not Delete's Group-Only-vs-Cascade split
(`src/ui/MarkersTab_BundleDelete_UI.h:18-32`): Delete offers two explicit modes specifically *because* it
is destructive and an accidental cascade is costly (its own doc comment frames "Group Only" as promoting
children to survive, "All" as erasing everything — a real, consequential fork). Select is non-destructive
and a single click away from correcting — there is no analogous need for a narrower "Group Only" select
mode, and no existing precedent offers one. Move/Rotate, the actually-analogous "operate on what's under
this Group" family, use ONLY the recursive resolver, unconditionally, with no opt-out. Selection follows
that family. This also matches the tree's own visual nesting (a sub-Group renders as a child of its parent
Group when expanded — `TreeListWidget_UI`'s own node/child recursion, same `parentBundleIdentifier` links
`CollectMarkerLayerBundleDescendantIdentifiers` walks), so the fix is also visually consistent with what
the user sees expand.

## Design decision 2 — callback call contract

`SyncManualMarkerSelection`'s own header contract (`MapCanvas_UI.h:271-280`): `clickedInstanceIdentifier`
is "the row the list itself just says is 'current'" — if present in `selectedInstanceIdentifiers` it
becomes the new primary; if absent, the set's own trailing order picks the fallback primary. For a bulk
Group/Layer select there is no single "clicked" row, but both `Apply*` functions already compute
`anchorIdentifier = selectedManualInstanceIdentifiers.front()` (or `-1` if empty) as their own required
output. Passing `anchorIdentifier` as `clickedInstanceIdentifier` and the full
`selectedManualInstanceIdentifiers` as the selected set is therefore not an arbitrary choice: `anchorIdentifier`
is guaranteed present in the set whenever non-empty (it IS the set's own front by construction), exactly
mirroring an ordinary single-row click where the clicked identifier is the only member of the just-updated
set. Firing with an empty set / `clickedInstanceIdentifier == -1` (the Procedural-leaf/no-manual-members
case) is also correct and required — it is a real change (clearing a stale prior canvas selection), not a
no-op to skip.

## Design decision 3 — guard pattern

Established precedent, `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp:61`:
`if (interaction.selectManualMarkerInstanceCallback) { ... }` before invoking — the callback is a
`std::function` that may be unset (test harnesses, or any call site that never wired one). Both fixes
below use this exact guard shape.

## Design decision 4 — `ApplyLayerListSignal` promotion out of the anonymous namespace

`ApplyLayerListSignal` (`src/ui/MarkersTab_ManualLayers_UI.cpp:17-84`, the WHOLE anonymous namespace
contains only this one function) is not declared in `MarkersTab_ManualLayers_UI.h` and has zero direct
test coverage today (grep-confirmed: no test file references it). STEP125 already established the exact
precedent for this situation on the Bundle tree's own sibling function — `ApplyMarkerLayerBundleTreeSignal`
was "extracted verbatim into a named function... so a test fixture can drive it directly without an imgui
frame" (`MarkersTab_BundleTreeSignals_UI.cpp:22-25`). Since this ticket is touching `ApplyLayerListSignal`
anyway to fix Bug 2, promoting it the same way (declare in the header, remove the anonymous-namespace
wrapper) is the natural, non-scope-creeping way to give this ticket's own new callback-wiring logic the
same pure-function test coverage the Bundle tree side already has, rather than requiring a GL-backed
imgui-frame test for something that is pure logic.

## Exact changes

### 1. `src/ui/MarkersTab_Bundles_UI.h` — widen `ApplyMarkerLayerBundleTreeSignal`'s declaration (`:289-296`)

No new `#include` needed — `<functional>` (`:14`) and `MarkerLayerBundleQuery_PARAMS.h` (`:27`, for
`Params::CollectMarkerLayerBundleRecursiveManualMembers`) are already included.

```cpp
void ApplyMarkerLayerBundleTreeSignal(const TreeListSignal<MarkerGroupLeafKey_UI>& signal,
                                      std::vector<Params::MarkerLayerBundle>& bundles,
                                      std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                      std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                      const std::vector<Params::MarkerInstanceGroup>& markers,
                                      const ManualInstanceLayerIndex_UI& instanceIndex,
                                      MarkerLayerBundlesState& state, int& selectedManualInstanceIdentifier,
                                      std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                      const std::function<void(int clickedInstanceIdentifier,
                                                               const std::vector<int>& selectedInstanceIdentifiers)>&
                                          selectManualMarkerInstanceCallback = {});   // NEW — STEP259, trailing
                                                                                      // default keeps every
                                                                                      // pre-existing call site
                                                                                      // compiling unchanged
```

### 2. `src/ui/MarkersTab_BundleTreeSignals_UI.cpp` — the fix itself (replaces lines 26-56)

```cpp
void ApplyMarkerLayerBundleTreeSignal(const TreeListSignal<MarkerGroupLeafKey_UI>& signal,
                                      std::vector<Params::MarkerLayerBundle>& bundles,
                                      std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                      std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                      const std::vector<Params::MarkerInstanceGroup>& markers,
                                      const ManualInstanceLayerIndex_UI& instanceIndex,
                                      MarkerLayerBundlesState& state, int& selectedManualInstanceIdentifier,
                                      std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                      const std::function<void(int clickedInstanceIdentifier,
                                                               const std::vector<int>& selectedInstanceIdentifiers)>&
                                          selectManualMarkerInstanceCallback) {
    if (signal.kind == TreeListSignalKind::Select) {
        if (signal.sourceKind == TreeNodeSourceKind::Node) {
            state.selectedBundleIdentifier = signal.sourceNodeIdentifier;
            // Human's own bug report (Bug 1) — a single click on a GROUP header now does what a Layer
            // header already did below: selects every Instance organizationally under it. RECURSIVE
            // (nested sub-Groups included) and MANUAL-ONLY (a Procedural layer contributes no members) —
            // the SAME resolution ApplyMarkerLayerBundleMove/Rotation already use
            // (Params::CollectMarkerLayerBundleRecursiveManualMembers, MarkerLayerBundleQuery_PARAMS.h,
            // MarkersTab_BundleNodeBody_UI.cpp), NOT Delete's own "Group Only" narrower scope — that
            // split exists because deleting is destructive and needs an escape hatch; selecting has no
            // analogous need, so there is no "Group Only" select mode to offer.
            selectedManualInstanceIdentifiers.clear();
            const std::vector<std::pair<int, int>> members =
                Params::CollectMarkerLayerBundleRecursiveManualMembers(signal.sourceNodeIdentifier, bundles,
                                                                       instanceLayers, markers);
            for (const std::pair<int, int>& groupTransformIndex : members)
                selectedManualInstanceIdentifiers.push_back(
                    markers[static_cast<std::size_t>(groupTransformIndex.first)]
                        .transforms[static_cast<std::size_t>(groupTransformIndex.second)]
                        .instanceIdentifier);
            anchorIdentifier = selectedManualInstanceIdentifiers.empty()
                              ? -1 : selectedManualInstanceIdentifiers.front();
            selectedManualInstanceIdentifier = anchorIdentifier;
            // Human's own bug report (Bug 2) — writing the three tabState fields above alone leaves the
            // canvas's own independent selection copy (MapCanvas::selectedInstanceKeys) stale; only this
            // callback keeps it in sync (MapCanvas::SyncManualMarkerSelection). Guarded exactly like the
            // established call site (MarkersTab_ManualLayerRowBody_UI.cpp's own
            // `if (interaction.selectManualMarkerInstanceCallback)`).
            if (selectManualMarkerInstanceCallback)
                selectManualMarkerInstanceCallback(anchorIdentifier, selectedManualInstanceIdentifiers);
        } else {
            // Human's own bug report — a single click on a Layer header selects that Layer (the
            // highlight) AND every Instance it owns (a Procedural leaf owns none, so it clears the
            // manual selection instead — it is not a "no selection change" no-op).
            state.selectedLeaf = signal.sourceLeaf;
            selectedManualInstanceIdentifiers.clear();
            if (signal.sourceLeaf.kind == MarkerGroupLeafKey_UI::Kind::Manual) {
                const auto memberIt = instanceIndex.instancesByLayerIndex.find(signal.sourceLeaf.layerIndex);
                if (memberIt != instanceIndex.instancesByLayerIndex.end())
                    for (const std::pair<int, int>& groupTransformIndex : memberIt->second)
                        selectedManualInstanceIdentifiers.push_back(
                            markers[static_cast<std::size_t>(groupTransformIndex.first)]
                                .transforms[static_cast<std::size_t>(groupTransformIndex.second)]
                                .instanceIdentifier);
            }
            anchorIdentifier = selectedManualInstanceIdentifiers.empty()
                              ? -1 : selectedManualInstanceIdentifiers.front();
            selectedManualInstanceIdentifier = anchorIdentifier;
            // Bug 2 fix — same reasoning as the Node branch above.
            if (selectManualMarkerInstanceCallback)
                selectManualMarkerInstanceCallback(anchorIdentifier, selectedManualInstanceIdentifiers);
        }
    }

    if (signal.kind == TreeListSignalKind::Reparent) {
        // ... UNCHANGED, no edit below this point ...
```

### 3. `src/ui/MarkersTab_Bundles_UI.cpp` — widen the call site (`:177-180`)

```cpp
    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, markers, instanceIndex, state,
                                     rootState.selectedManualInstanceIdentifier,
                                     rootState.selectedManualInstanceIdentifiers,
                                     rootState.manualInstanceSelectionAnchorIdentifier,
                                     selectManualMarkerInstanceCallback);   // NEW — STEP259, already this
                                                                            // function's own parameter
```

### 4. `src/ui/MarkersTab_ManualLayers_UI.h` — declare `ApplyLayerListSignal` (new, near `DrawManualMarkerLayerListBody`, before it)

`<functional>` already included (`:31`).

```cpp
// The Select/ToggleLock/ToggleVisibility/Delete/Reorder signal-application logic DrawManualMarkerLayerListBody
// already ran inline, anonymous-namespace-local (STEP81) — promoted out and named here (STEP259),
// mirroring STEP125's identical treatment of the Bundle tree's own sibling
// ApplyMarkerLayerBundleTreeSignal (MarkersTab_Bundles_UI.h), so a test fixture can drive it directly
// without an imgui frame.
// Human's own bug report — a Select signal now ALSO replaces the caller's whole manual selection with
// every Instance belonging to the clicked Layer (mirrors the Bundle tree's own Leaf-select branch,
// MarkersTab_BundleTreeSignals_UI.cpp) AND fires `selectManualMarkerInstanceCallback` (when set) so the
// canvas's own independent selection copy stays in sync (MapCanvas::SyncManualMarkerSelection) — the
// second half of that same bug report. Reports whether `markers` moved, which feeds no pipeline stage
// (SCOPE NOTE 3).
bool ApplyLayerListSignal(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                          std::vector<Params::MarkerInstanceGroup>& markers,
                          const ManualInstanceLayerIndex_UI& instanceIndex, ManualMarkerLayersState& state,
                          int& selectedManualInstanceIdentifier, std::vector<int>& selectedManualInstanceIdentifiers,
                          int& anchorIdentifier, const DraggableListSignal& signal,
                          const std::function<void(int clickedInstanceIdentifier,
                                                   const std::vector<int>& selectedInstanceIdentifiers)>&
                              selectManualMarkerInstanceCallback = {});
```

### 5. `src/ui/MarkersTab_ManualLayers_UI.cpp` — remove the anonymous-namespace wrapper (`:17`/`:84`), widen the function, invoke the callback

Delete the `namespace {` at line 17 and its matching `} // namespace` at line 84 (the function moves to
file scope, still inside `namespace Ui`). Widen the Select branch:

```cpp
bool ApplyLayerListSignal(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                         std::vector<Params::MarkerInstanceGroup>& markers,
                         const ManualInstanceLayerIndex_UI& instanceIndex, ManualMarkerLayersState& state,
                         int& selectedManualInstanceIdentifier, std::vector<int>& selectedManualInstanceIdentifiers,
                         int& anchorIdentifier, const DraggableListSignal& signal,
                         const std::function<void(int clickedInstanceIdentifier,
                                                  const std::vector<int>& selectedInstanceIdentifiers)>&
                             selectManualMarkerInstanceCallback) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedLayerIndex = signal.sourceRowIndex;
        // Human's own bug report — mirrors the Bundle tree's own Leaf-select branch
        // (MarkersTab_BundleTreeSignals_UI.cpp): a single click on a Layer header also selects every
        // Instance it owns.
        selectedManualInstanceIdentifiers.clear();
        const auto memberIt = instanceIndex.instancesByLayerIndex.find(signal.sourceRowIndex);
        if (memberIt != instanceIndex.instancesByLayerIndex.end())
            for (const std::pair<int, int>& groupTransformIndex : memberIt->second)
                selectedManualInstanceIdentifiers.push_back(
                    markers[static_cast<std::size_t>(groupTransformIndex.first)]
                        .transforms[static_cast<std::size_t>(groupTransformIndex.second)].instanceIdentifier);
        anchorIdentifier = selectedManualInstanceIdentifiers.empty()
                          ? -1 : selectedManualInstanceIdentifiers.front();
        selectedManualInstanceIdentifier = anchorIdentifier;
        // Bug 2 fix (STEP259) — same reasoning as the Bundle tree's own Leaf branch: writing the three
        // tabState fields above alone leaves the canvas's own independent selection copy stale.
        if (selectManualMarkerInstanceCallback)
            selectManualMarkerInstanceCallback(anchorIdentifier, selectedManualInstanceIdentifiers);
        return false;
    }
    // ... UNCHANGED below (ToggleLock/ToggleVisibility/Delete/Reorder branches) ...
```

Note: the default-argument (`= {}`) belongs only in the header declaration (§4) — do not repeat it on
this definition (standard C++ rule).

Widen the call site inside `DrawManualMarkerLayerListBody` (`:257-259`):

```cpp
    if (signal.bHasSignal())
        ApplyLayerListSignal(markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier,
                             selectedManualInstanceIdentifiers, anchorIdentifier, signal,
                             selectManualMarkerInstanceCallback);   // NEW — STEP259, already this
                                                                    // function's own parameter
```

## Explicit out-of-scope

- No "Group Only" (direct-children-only) select mode — Design decision 1 explains why no such mode is
  warranted; not adding one as a half-measure.
- `MarkerLayerBundlesState`/`ManualMarkerLayersState`'s own shapes — no new fields, both already have
  everything this fix needs.
- `MapCanvas::SyncManualMarkerSelection`'s own body/contract (`MapCanvas_UI.cpp:128-168`) — already
  correct and fully covered by `MarkersTab_ListCanvasSelectionSync_UI_Test.cpp`; this ticket only adds
  more callers of the existing entry point, does not modify it.
- The canvas-native `ApplySelectionGesture`/`SelectManualMarkerByInstanceIdentifier` paths (STEP232/233,
  ARCH §21.1) — untouched, a different, already-correct selection origin.
- Procedural Rule Layer's own instance-list selection (`MarkersTab_RuleLayerInstances_UI.cpp`) — a
  separate, already-correct STEP132 path; this ticket is Manual-only per the Leaf branch's own existing
  `Kind::Manual` gate.
- Delete/Reparent signal handling in either `Apply*` function — untouched.
- No PARAMS/IO/PIPELINE change; no new `.sanmap` fields.
- No opportunistic file-size-ceiling split of `MarkersTab_ManualLayers_UI.cpp` (already 268 lines
  pre-ticket, pre-existing, not introduced by this ~15-line addition) — mirrors STEP256's own explicit
  "not this ticket's remediation to invent" posture; flag separately if a formal ceiling check trips.

## Files touched

**New:** none. **Deleted:** none.

**Modified:**
- `src/ui/MarkersTab_Bundles_UI.h` — widened `ApplyMarkerLayerBundleTreeSignal` declaration (`:289-296`).
- `src/ui/MarkersTab_BundleTreeSignals_UI.cpp` — Node branch gains recursive Manual-member selection;
  both Select branches gain the guarded callback invocation; widened signature (`:26-81`).
- `src/ui/MarkersTab_Bundles_UI.cpp` — widened call site (`:177-180`).
- `src/ui/MarkersTab_ManualLayers_UI.h` — new `ApplyLayerListSignal` declaration (promoted out of the
  anonymous namespace), placed before `DrawManualMarkerLayerListBody`.
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `ApplyLayerListSignal` moved to file scope (anonymous
  namespace removed), Select branch gains the guarded callback invocation; widened call site inside
  `DrawManualMarkerLayerListBody` (`:17-84`, `:257-259`).
- `src/ui/MarkersTab_Bundles_UI_Test.cpp` — new acceptance tests (below); existing call sites at `:244-246`,
  `:554-556`, `:592-594` need NO edit (the new parameter defaults to `{}`).
- `src/ui/MarkersTab_ManualLayers_UI_Test.cpp` — new acceptance tests (below); first direct test coverage
  of `ApplyLayerListSignal`.

## Acceptance tests

`MarkersTab_Bundles_UI_Test.cpp` (pure logic, no imgui frame, mirroring the file's own existing
`TestManualLeafSelectSignalSelectsMemberInstancesAndHighlight`/`TestProceduralLeafSelectSignalClearsManualSelection`
shape — added as NEW test functions, not edits to those two, so their own existing assertions stay green
unmodified):

1. `TestNodeSelectSignalSelectsRecursiveManualMembersAndFiresCallback` — a parent Group (id 1) with a
   direct Procedural layer and a direct Manual layer, plus a nested sub-Group (id 2, `parentBundleIdentifier
   == 1`) with its own Manual layer; three marker transforms: one on the parent's Manual layer, one on the
   sub-Group's Manual layer, one on neither (layerIndex -1, ungrouped). Selecting the parent Group (Node,
   id 1) must select exactly the two Manual instances (direct + nested), never the ungrouped one, never
   contribute from the Procedural layer; `anchorIdentifier`/`selectedManualInstanceIdentifier` land on the
   resolved set's front; the callback fires with the identical clicked/selected values.
2. `TestNodeSelectSignalWithNoManualMembersFiresCallbackWithEmptySet` — a Group containing only a
   Procedural layer: selection resolves empty, anchor/primary both `-1`, and the callback STILL fires
   (with `clickedInstanceIdentifier == -1`, empty set) — proves a stale prior canvas selection is actually
   cleared, not left stuck.
3. `TestManualLeafSelectSignalFiresCallback` — the Leaf/Manual branch (membership-selection half already
   covered by the pre-existing test) now also fires the callback with the same anchor/full set the
   tabState fields settled on.
4. `TestProceduralLeafSelectSignalFiresCallbackWithEmptySet` — the Leaf/Procedural branch fires the
   callback too, even though it clears to an empty selection.

`MarkersTab_ManualLayers_UI_Test.cpp` (new coverage — `ApplyLayerListSignal` had none before this ticket):

5. `RunApplyLayerListSignalSelectPopulatesMembersAndFiresCallbackChecks` — two Manual layers, three marker
   transforms (two on layer 0, one on layer 1); selecting layer 0 replaces the selection with exactly its
   two members, anchor/primary land on the first, and the callback fires with the matching
   clicked/selected values — mirrors `MarkersTab_Bundles_UI_Test.cpp`'s own
   `TestManualLeafSelectSignalSelectsMemberInstancesAndHighlight` shape.
6. `RunApplyLayerListSignalSelectWithNoCallbackDoesNotCrashCheck` — calling `ApplyLayerListSignal` with no
   callback argument at all (relying on the header's `= {}` default) completes normally — the guard
   (`if (selectManualMarkerInstanceCallback)`) is exercised and does not crash.

Full solo rebuild + `ctest -C Debug`: every previously-passing test in `MarkersTab_Bundles_UI_Test` and
`MarkersTab_ManualLayers_UI_Test` stays green (including the pre-existing
`TestManualLeafSelectSignalSelectsMemberInstancesAndHighlight`/`TestProceduralLeafSelectSignalClearsManualSelection`/
`TestApplyMarkerLayerBundleTreeSignalFilteredCopyWriteSafety`/`TestCrossTypeSectionNestedBundleCutoff`,
none of whose own assertions this ticket touches), plus `MarkersTab_ListCanvasSelectionSync_UI_Test`
(untouched, proves this ticket doesn't regress `SyncManualMarkerSelection`'s own already-correct contract).

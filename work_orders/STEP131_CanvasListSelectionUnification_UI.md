# STEP131 — Canvas/list selection unification (ARCH §19.25)

**Layer:** UI. **Domain:** `MapCanvas_IconLayer_UI.h`, `MapCanvas_IconLayer_CullManual_UI.cpp`,
`MapCanvas_UI.h/.cpp`, `MapCanvas_Draw_UI.cpp`, `Application_UI.cpp`,
`MarkersTab_ManualLayerRowBody_UI.cpp/.h`, `MarkersTab_TypeSections_UI.cpp`,
`MarkersTab_ManualLayers_UI.cpp/.h`, `MarkersTab_UI.h/.cpp`. **Sequence:** depends on STEP130
landing (touches `MarkersTab_ManualLayerRowBody_UI.cpp`, which STEP130 also modifies — do not run
concurrently with it). Prerequisite for STEP132 (item 13, procedural instance selection) — land and
verify this ticket fully before STEP132 begins.

Ratifies `ARCH_19_25_SelectionRepresentationUnification.md` in full — the most invasive ticket of
this correction round. Fixes a real, currently-live bug (procedural/manual selection index-space
collision) AND makes clicking an instance in the Markers-tab list actually highlight the real,
visible marker icon on the canvas — which today it does not (it only recolors a near-invisible
auxiliary dot roster, per the brief's own item-11 investigation).

## Fix

### 1. `OverlayInstanceKey_UI` gains `bManual`

`MapCanvas_IconLayer_UI.h:24-28`: add `bool bManual = false;`. Include it in
`OverlayInstanceKeysEqual`. Default `false` — every existing procedural comparison stays
byte-identical.

### 2. Manual candidates get a globally-unique key

`ResolveMarkersManual` (`MapCanvas_IconLayer_CullManual_UI.cpp`): change the key passed into
`instance.instanceKey` from the current per-group `index` to `transform.instanceIdentifier`, with
`bManual = true`.

**Binding edge case — audit before this lands:** a key with `instanceIdentifier < 0` is never a
legal manual selection target. Audit EVERY live `MarkerTransform` construction path — Add Marker,
paste, drag-materialize, symmetry-fix repair, import — and confirm each one mints a real id via
`NextMarkerInstanceIdentifier` (or the equivalent legacy-backfill on import) before this lands. If
any path is found NOT to mint one, that is a real gap to fix as part of this ticket, not a
pre-existing condition to route around.

### 3. `MapCanvas`'s selection state widens to the full key

`selectedEntityIdentifier` becomes backed by `OverlayInstanceKey_UI selectedInstanceKey`.
`SelectedEntityIdentifier()`/`HasSelection()` stay as thin reads of `.instanceIndex`/`.bValid` for
existing procedural-only callers (`Application_Draw_UI.cpp:64`).

**`SetSelection` gains the canonical full-key overload:**
```cpp
void SetSelection(const OverlayInstanceKey_UI& key);
```
The existing `void SetSelection(std::uint32_t entityIdentifier)` becomes a thin wrapper:
```cpp
void SetSelection(std::uint32_t entityIdentifier) {
    SetSelection(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers,
                                       static_cast<int32_t>(entityIdentifier),
                                       entityIdentifier != <emptySentinel>, /*bManual=*/false});
}
```
Every existing procedural call site stays unchanged, compiling against the old overload.

`ApplyClick`: try `PickMarker` (procedural) first; on miss, add a small **linear** hit-test over
`recipe.markers` (authoring scale — no spatial grid needed, same posture
`MapCanvas_IconLayer_CullManual_UI.cpp`'s own header comment already states for manual layers). On a
manual hit, call `SetSelection({Markers, transform.instanceIdentifier, true, bManual=true})`.

### 4. `selectionChangedCallback` widens

From `std::function<void(std::uint32_t)>` to `std::function<void(const OverlayInstanceKey_UI&)>`.
`Application::WireCallbacks()` updates both `lastSelectedEntityIdentifier` (procedural case) and,
when `key.bManual`, `tabState.markers.selectedManualInstanceIdentifier` — keeping the list's own
highlight in sync when the canvas is what changed selection.

### 5. List-click → canvas, shell-mediated

New public `MapCanvas::SelectManualMarkerByInstanceIdentifier(int)`. New `Application`-bound
`std::function<void(int)> selectManualMarkerInstanceCallback`, wired in `WireCallbacks()` mirroring
`SetManualMarkerSelectionSource`'s existing injection pattern, threaded down through
`DrawMarkerTypeSections` → `DrawManualMarkerLayerListBody`/the Bundle-tree leaf body →
`DrawLayerRowBody`'s existing call chain (the same chain `previewDriver`/`iconManifest` already ride
down — no new plumbing shape, just one more parameter riding the same path).

`DrawLayerRowBody`'s instance-list `Selectable` click calls this new callback IN ADDITION TO its
existing `selectedManualInstanceIdentifier` tab-local write (not instead of) — two-way sync: canvas
click updates the list's highlight (item 4), list click updates the canvas's real icon (item 5).

### 6. Dot-roster redundant branch — explicitly OUT of scope for this ticket

Do NOT delete `DrawManualMarkerRoster`'s `IsInstanceHighlighted`/`ResolveMarkerGroupSelectTintColor`/
`selectedHighlightInstanceIdentifiers`/`ComputeManualMarkerSelectionHighlight` in this ticket. Once
the real icon correctly shows `bSelected`, this branch becomes redundant but not harmful — its
sibling logic in the same function (drag-refused tint, drag-ghost points, `ManualSpawnArmyTint`) is
NOT reproduced elsewhere and must stay. Removing the redundant branch is a follow-up ticket, after
this one is verified working end-to-end, not bundled here.

## Verify

- **Index-collision regression, the specific bug this ticket fixes:** construct a fixture where a
  procedural `PlacementInstances` array position numerically equals an unrelated manual transform's
  OLD per-group index; confirm that with `bManual` correctly tagged, selecting the procedural one does
  NOT light up `bSelected` on the manual one (this exact collision was live and unguarded before this
  ticket — prove it's closed, don't just add forward-looking coverage).
- **`SetSelection`'s wrapper overload:** every existing procedural call site's own test still passes
  unchanged (regression proof the old `uint32_t` overload is truly a byte-identical wrapper).
- **`instanceIdentifier < 0` edge case:** confirm (by audit + a direct test if a gap is found) that no
  live `MarkerTransform` construction path ever leaves a negative id reaching selection.
- **Canvas click → manual marker:** a synthetic canvas click at a manual marker's screen position (no
  procedural hit) selects it via the new linear hit-test; `tabState.markers.selectedManualInstanceIdentifier`
  updates to match (item 4's sync).
- **List click → canvas:** clicking an instance row calls the new shell callback with the correct
  `instanceIdentifier`; `MapCanvas`'s own `selectedInstanceKey` updates to `{Markers, id, true, true}`.
- **End-to-end, the actual complaint this ticket fixes:** after a list click, the REAL icon-sprite
  render path (`MapCanvas_IconLayer_CullEmit_UI.cpp`'s `instance.bSelected`) reflects the selection —
  not just the auxiliary dot roster. This is the acceptance bar the brief's item 11 originally asked
  for; verify it directly, not by proxy.
- Existing suites (`MapCanvas_MarkerDrag_UI_Test`, `MapCanvas_IconLayer_UI_Test`,
  `MarkersTab_ManualLayers_UI_Test`, `MarkersTab_UI_Test`, `ApplicationShell_*_UI_Test`) stay green.

## Out of scope

- Item 12 (symmetry grouping in the instance list) — separate ticket, can land independently.
- Item 13 (procedural instance listing/selection) — depends on THIS ticket landing first; separate
  ticket, do not start until this one is verified.
- Dot-roster redundant-branch removal (item 6 above) — explicit follow-up, not this ticket.

[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.25. **Only the ARCH Expert writes this file.**

### 19.25 Canvas/list selection unification — `OverlayInstanceKey_UI::bManual`, `MapCanvas`'s widened selection surface, the shell-mediated tab↔canvas callback; **corrects and narrows §19.20**
Responds to item 11 — the most invasive item of `DESIGN_MarkersUICorrectionRound2_R1.md`.
**Ratified as designed**, re-verified independently against the live code (not taken on the design
doc's word alone): `MapCanvas_IconLayer_UI.h:24-32` confirms `OverlayInstanceKey_UI`'s current
2-field shape; `MapCanvas_UI.cpp:34-56,70-74` confirms `ApplyClick`/`SetSelection` today resolve and
store a bare `std::uint32_t` `selectedEntityIdentifier` sourced only from `PickMarker` (procedural);
`Application_UI.cpp:74-106` confirms `WireCallbacks()`'s shell-mediated-injection pattern
(`SetManualMarkerDragSource`/`SetManualMarkerSelectionSource`) this ruling extends, not invents.

**1. Index-space collision fix — ratified.** `OverlayInstanceKey_UI` (`MapCanvas_IconLayer_UI.h:24-28`)
gains `bool bManual = false;`, included in `OverlayInstanceKeysEqual`. Default `false` keeps every
existing procedural comparison byte-identical. This is the real, currently-live bug fix: procedural
`Data::PlacementInstances` array positions and manual per-group transform indices share one untagged
`int32_t` number space today under the same `PlacementCollectionKind_UI::Markers` tag and can
collide, incorrectly lighting up an unrelated manual marker's `bSelected`.

**2. Manual candidates get a globally-unique key.** `ResolveMarkersManual`
(`MapCanvas_IconLayer_CullManual_UI.cpp`) passes `transform.instanceIdentifier` (§19.16 — minted,
never reused, `-1` = unassigned) as the key, not the per-group `index` it uses today, with
`bManual = true`. **Binding edge case:** a key with `instanceIdentifier < 0` is never a legal manual
selection target (`bValid` for a manual key requires `instanceIdentifier >= 0`); the coder work-order
must audit every live `MarkerTransform` construction path (Add Marker, paste, drag-materialize,
symmetry-fix, import) mints a real id via `NextMarkerInstanceIdentifier` (§19.16) before this lands.

**3. `MapCanvas`'s selection state widens to the full key.** `selectedEntityIdentifier` becomes
backed by `OverlayInstanceKey_UI selectedInstanceKey`; `SelectedEntityIdentifier()`/`HasSelection()`
stay as thin reads of `.instanceIndex`/`.bValid` for existing procedural-only callers
(`Application_Draw_UI.cpp:64`). **`SetSelection` itself gains the canonical full-key overload** —
`void SetSelection(const OverlayInstanceKey_UI& key)` — and the existing
`void SetSelection(std::uint32_t entityIdentifier)` becomes a thin wrapper constructing
`{Markers, static_cast<int32_t>(entityIdentifier), entityIdentifier != emptySentinel,
bManual=false}`, preserving every existing procedural call site unchanged. This is a small,
justified addition beyond the design doc's literal text, grounded directly in the design's own
stated "build it once, not twice" convergence principle (item 13): every selection-setting path
(canvas click-pick, a Markers-tab list click for a manual instance, §19.27's procedural list click)
must resolve through this ONE setter, never three divergent ones.

`ApplyClick` tries `PickMarker` (procedural) first; on miss, a new small **linear** hit-test over
`recipe.markers` (authoring scale — the same "no grid needed" posture
`MapCanvas_IconLayer_CullManual_UI.cpp`'s own header comment already states for manual layers) lets
a canvas click select a manual marker for the first time; on a manual hit, calls
`SetSelection({Markers, transform.instanceIdentifier, true, bManual=true})`.

**4. `selectionChangedCallback` widens** from `std::function<void(std::uint32_t)>` to
`std::function<void(const OverlayInstanceKey_UI&)>`, so `Application::WireCallbacks()` can update
both `lastSelectedEntityIdentifier` (procedural case) and, when `bManual`,
`tabState.markers.selectedManualInstanceIdentifier` — keeping the list's own highlight in sync when
the canvas is what changed selection.

**5. List-click → canvas, shell-mediated — ratified, same pattern as `SetManualMarkerSelectionSource`
(§19.19), not a new module-boundary class.** A new public `MapCanvas::SelectManualMarkerByInstanceIdentifier(int)`
+ a new `Application`-bound `std::function<void(int)> selectManualMarkerInstanceCallback`, wired in
`WireCallbacks()` and threaded down through `DrawMarkerTypeSections`/`DrawLayerRowBody`'s existing
call chain (the same chain `previewDriver`/`iconManifest` already ride down). `DrawLayerRowBody`'s
Selectable click calls this callback (in addition to, not instead of, its existing
`selectedManualInstanceIdentifier` tab-local write) — two-way sync: canvas click updates the list's
highlight (item 4), list click updates the canvas's real icon (item 5).

**6. The dot roster's redundant highlight branch** (`DrawManualMarkerRoster`'s
`IsInstanceHighlighted`/`ResolveMarkerGroupSelectTintColor`, `selectedHighlightInstanceIdentifiers`,
`ComputeManualMarkerSelectionHighlight`) becomes dead once the real icon correctly shows
`bSelected`, but is NOT bundled into this ruling — a lower-priority follow-up ticket, after the
real-icon fix is verified, since the roster's non-selection logic (drag-refused tint, drag-ghost
points, `ManualSpawnArmyTint`) is not reproduced elsewhere and stays.

---

**Corrects §19.20.** §19.20's "manual-only" framing and its claim that "`OverlayInstanceKey_UI`'s
existing procedural-only selection-key pipeline is untouched, unshared, and unreferenced by this
feature" no longer hold — this ruling directly widens that pipeline, and §19.27 converges procedural
list-selection onto the SAME `OverlayInstanceKey_UI` representation this ruling establishes. §19.20's
one binding sentence that DOES still hold, unchanged, and is explicitly honored by both this ruling
and §19.27: **`instanceIdentifier` itself is never repurposed for procedural identity** — procedural
selection is keyed by array position (`bManual=false`), never by `instanceIdentifier`, which stays a
`MarkerTransform`-only field. See §19.20's own file for the full correction note.

[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.3. **Only the ARCH Expert writes this file.**

### 21.3 Drag-gesture genericization — `InstanceDragGestureState`, `Begin/Update/EndInstanceDragGesture<Traits>`, `RepositionSymmetryGroupMember<Traits>`, `HitTestManualInstances<GroupT>`/`CollectManualInstancesInWorldRegion<GroupT>`

**Ratified as designed, with two refinements this section's own direct re-read of
`MarkerDragGesture_UI.h`/`.cpp`, `MarkerDragGesture_Frame_UI.cpp`, `MarkerOrbitCorrespondence_UI.h`,
`PropInstance_PARAMS.h`, and `UniqueNameList_UI.h` required.** Both narrow/correct the relayed
summary rather than contradict its direction; recorded per this ticket's own request to flag any
divergence.

**File moves — confirmed safe, both files genuinely carry zero `Params::` types already.**
`MarkerOrbitCorrespondence_UI.h` (the one-shot matcher, `MatchCorrespondenceToOrbit`) is renamed
`InstanceOrbitCorrespondence_UI.h`; its struct drops "Marker" too (`MarkerOrbitCorrespondence` ->
`InstanceOrbitCorrespondence`) — content otherwise byte-identical, a pure rename, NOT a merge into
the gesture file (merging would undo the exact reason this file was split out of the gesture header
in the first place, per its own header comment: keeping that header under §1.5's soft-100 ceiling).

**Refinement 1 — the state struct itself needs no template parameter at all.** Every field of
`MarkerDragGestureState` (confirmed by direct read, `MarkerDragGesture_UI.h:41-62`) is already a
plain `int`/`float`/`bool` or a `std::vector<InstanceOrbitCorrespondence>` — zero `Params::` types
anywhere in it. Renamed `InstanceDragGestureState` (its `bSpawnGroup` field renames to
`bCardinalityFrozen`, mirroring `IsCardinalityFrozenGroup`'s own domain-neutral name), it is
declared ONCE, as an ordinary non-template struct, in the new `InstanceDragGesture_UI.h`, and
shared VERBATIM by all three domains. Only the four functions that read/write `Params::`
group/transform/layer vectors are templates:
```cpp
template<typename Traits>
bool BeginInstanceDragGesture(InstanceDragGestureState& state,
                              const std::vector<typename Traits::Group>& instances,
                              const std::vector<typename Traits::Layer>& layers,
                              const Params::Geometry& geometry, int globalSymmetryMask,
                              int globalRadialRepeatCount, int groupIndex, int transformIndex);
template<typename Traits>
void UpdateInstanceDragGesture(InstanceDragGestureState& state,
                               std::vector<typename Traits::Group>& instances,
                               const std::vector<typename Traits::Layer>& layers,
                               const Params::Geometry& geometry, float newWorldX, float newWorldZ);
template<typename Traits>
void EndInstanceDragGesture(InstanceDragGestureState& state,
                            std::vector<typename Traits::Group>& instances,
                            const Params::Geometry& geometry);
template<typename Traits>
bool RepositionSymmetryGroupMember(std::vector<typename Traits::Group>& instances,
                                   const std::vector<typename Traits::Layer>& layers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount, int groupIndex,
                                   int movedTransformIndex, float newWorldX, float newWorldZ);
```
Split across `InstanceDragGesture_UI.cpp`/`InstanceDragGesture_Frame_UI.cpp`, mirroring
`MarkerDragGesture_UI.cpp`/`_Frame_UI.cpp`'s own existing two-file split exactly — template body
placement (header vs. included `.hpp`-style body) is standard C++ mechanics, not an ARCH concern.

**`Traits` contract:**
```cpp
struct Traits {
    using Group = /* Params::MarkerInstanceGroup / PropInstanceGroup / DecalInstanceGroup */;
    using Transform = /* Params::MarkerTransform / PropTransform / DecalTransform */;
    using Layer = /* Params::MarkerInstanceLayer / PropInstanceLayer / DecalInstanceLayer */;
    static Group* SelectedGroup(std::vector<Group>&, int groupIndex);
    static Transform* SelectedInstance(std::vector<Transform>&, int transformIndex);
    static bool IsInstanceLayerLocked(const std::vector<Layer>&, int layerIndex);
    static void QuantizePositionToLayerGrid(const std::vector<Layer>&, int layerIndex, float& x, float& z);
    static void ResolveEffectiveSymmetry(const std::vector<Layer>&, int layerIndex, int globalMask,
                                         int globalRadialCount, int& outMask, int& outRadialCount);
    static bool IsCardinalityFrozenGroup(const Group&);   // Markers: Spawn check. Props/Decals: false.
    static int  NextInstanceIdentifier(const std::vector<Group>&);
    static void SeedInstanceName(Transform& materialized, int existingTransformCount);   // Refinement 2
    static void MakeInstanceNamesUnique(std::vector<Transform>&);                        // Refinement 2
};
```

**Refinement 2 — `PropTransform`/`DecalTransform` carry NO `name` field at all (confirmed by direct
read, `PropInstance_PARAMS.h:20-21`), unlike `MarkerTransform`.** The design's proposed
`NextInstanceName`/`MakeInstanceNamesUnique` hooks, called as literal `materialized.name = ...` /
`MakeNamesUnique<Transform>(...)` inside the generic core, would not COMPILE for two of the three
`Traits` instantiations — `UniqueNameList_UI.h`'s `MakeNamesUnique<T>` requires a `.name` member by
construction, and there is no field to assign in the first place. The generic materialize loop
(inside `EndInstanceDragGesture<Traits>`, the direct analogue of today's `EndMarkerDragGesture`'s
own unclaimed-slot materialization, `MarkerDragGesture_Frame_UI.cpp:123-146`) never touches `.name`
directly — it calls `Traits::SeedInstanceName(materialized, existingCount)` once per new sibling,
then `Traits::MakeInstanceNamesUnique(instances[groupIndex].transforms)` once after the loop.
`MarkerDragTraits` implements both as real wrappers (`SeedInstanceName` sets `.name` via the
existing `NextMarkerInstanceName` convention; `MakeInstanceNamesUnique` calls the already-shared
`Ui::MakeNamesUnique<Params::MarkerTransform>` — itself untouched, it was ALREADY a generic
template, `UniqueNameList_UI.h:29`). `PropDragTraits`/`DecalDragTraits` implement both as EMPTY
no-ops — inert by construction, the identical treatment `IsCardinalityFrozenGroup` already gets for
its own Markers-only special case, never a special-cased branch inside the shared core.

**`MarkerDragGesture_UI.h` shrinks to just `MarkerDragTraits`** (thin static wrappers over the
already-existing free functions `SelectedMarkerGroup`/`SelectedMarkerInstance`/
`IsMarkerInstanceLayerLocked`/`QuantizeMarkerPositionToLayerGrid`/`ResolveEffectiveMarkerSymmetry`/
`IsSpawnMarkerGroup`/`NextMarkerInstanceIdentifier`/`NextMarkerInstanceName`/`MakeNamesUnique` — zero
behavior change, every wrapped function keeps its own current name and file) plus
`kArmyKeyedMarkerGroupName`, unchanged. New `PropDragGesture_UI.h`/`DecalDragGesture_UI.h` get
their own `PropDragTraits`/`DecalDragTraits` — **gated**, buildable only once §21.4's PARAMS fields
exist (ratified this session) AND §20.2's already-ratified but NOT-YET-BUILT
`QuantizePropPositionToLayerGrid`/`ResolveEffectivePropSymmetry`/`QuantizeDecalPositionToLayerGrid`/
`ResolveEffectiveDecalSymmetry` exist (confirmed by grep: none of the four exist in `src/` today) —
a coder work-order for `PropDragTraits`/`DecalDragTraits` must build those four resolvers first or
alongside, not assume them present.

**Manual hit-test/region-collect genericize as plain (non-`Traits`) templates**, since
`HitTestManualMarkers`'s own algorithm (confirmed by direct read) touches only
`.transforms[].transform.positionX/positionZ`, name-identical across all three group types
(confirmed: `PropTransform`/`DecalTransform` both wrap `InstancedTransform transform` too,
`PropInstance_PARAMS.h:20-21`) — position-only duck-typing suffices, no `Traits` needed:
```cpp
template<typename GroupT>
bool HitTestManualInstances(const std::vector<GroupT>& instances, const PreviewComposite&,
                            const MapCanvasView&, float regionLocalX, float regionLocalY,
                            float pickRadiusScreenPixels,
                            const std::function<bool(int layerIndex)>& isLayerLocked,   // §21.5
                            int& outGroupIndex, int& outTransformIndex);
template<typename GroupT>
void CollectManualInstancesInWorldRegion(const std::vector<GroupT>& instances,
                                         float worldMinX, float worldMinZ, float worldMaxX, float worldMaxZ,
                                         const std::function<bool(int layerIndex)>& isLayerLocked,   // §21.5
                                         std::vector<std::pair<int,int>>& outGroupTransformPairs);
```
New `ManualInstanceHitTest_UI.h`/`.cpp`, replacing `MapCanvas_MarkerHitTest_UI.cpp`.
`HitTestManualMarkers` stays, unchanged name and signature, as a one-line wrapper calling
`HitTestManualInstances<Params::MarkerInstanceGroup>` — legal to bind either an always-false-locked
predicate (preserving the old bare behavior for any remaining caller that needs it) or the real
`IsMarkerInstanceLayerLocked`-bound predicate at §21.2's new call site; both are instantiations of
the one template, never two copies of the algorithm.

`MapCanvas::TryBeginManualInstanceDrag` (§21.2) is the hand-written, NOT templated, 3-way
nearest-hit-wins dispatcher — it touches three concrete `Params::` group types by name in one
function body (comparing which of three `HitTestManualInstances<GroupT>` calls found the nearest
hit), which is domain-touching dispatch logic, not pure mechanics (§3.5/§19.2's own test routes
this class of code to hand-written, never a template).

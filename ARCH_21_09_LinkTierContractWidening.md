[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.9. **Only the ARCH Expert writes this file.**

### 21.9 Link-tier contract widening — completes §19.33's own "binding consumer-site audit" for the drag-gesture `Traits` contract (§21.3) and the manual hit-test/marquee/delete lock predicate (§21.5, `ManualInstanceHitTest_UI.h`, `ManualInstanceDelete_UI.h`) — RATIFIED 2026-08-31

Responds to a parallel SanGen UI Expert design pass that located two ratified, cross-domain
(Markers/Props/Decals) generic contracts `§19.33`'s own audit named only in outline. Grounded against a
direct re-read of `InstanceDragGesture_UI.h`, `ManualInstanceHitTest_UI.h`/`.cpp`,
`ManualInstanceDelete_UI.h`/`.cpp`, `MarkerDragGesture_UI.h`, `PropDragGesture_UI.h`,
`DecalDragGesture_UI.h`, `MarkersTab_ManualLayerHelpers_UI.h`, `MapCanvas_ManualDragDispatch_UI.cpp`,
`MapCanvas_SelectionGesture_UI.cpp`, `MarkerInstance_PARAMS.h`, `PropInstance_PARAMS.h`, and
`MarkerDragGesture_UI_Test.cpp` — not merely the relayed summary.

#### The question, answered: widen. A Markers-only wrapper is structurally impossible, not merely inconvenient.

**Confirmed by direct read: the claim that every governed-field resolver is called "over EVERY
transform touched mid-drag" is true for two of the three Traits methods in question, not all three —
this matters for how narrow the fix can be.**
- `Traits::QuantizePositionToLayerGrid` — called mid-loop, once per sibling (`UpdateInstanceDragGesture`,
  the ungrouped-path call, the dragged-member call, AND the per-sibling `for` loop) — genuinely needs a
  *different* effective grid resolved per transform, since siblings in one symmetry group can sit on
  different Layers (and, after this correction, different Links) than the dragged member.
- `Traits::ResolveEffectiveSymmetry` — called exactly ONCE per gesture (`BeginInstanceDragGesture`) and
  once per call (`RepositionSymmetryGroupMember`), always for the one named (dragged/moved) transform —
  never inside a per-sibling loop.
- `Traits::IsInstanceLayerLocked` — same as above: exactly once per `BeginInstanceDragGesture` call
  (the dragged transform) and once per `RepositionSymmetryGroupMember` call (the moved transform) —
  never per-sibling.
- `EndInstanceDragGesture<Traits>` calls **none** of the three — confirmed by direct read, it only
  touches `SelectedGroup`/`SelectedInstance`/`NextInstanceIdentifier`/`SeedInstanceName`/
  `MakeInstanceNamesUnique`/`Pipeline::BuildWorldSymmetryOrbit` (the last using the gesture-start
  symmetry snapshot already resolved at Begin). **`EndInstanceDragGesture` needs no widening at all.**

**A Markers-only wrapper sitting outside the template cannot fix any of the three, for two independent
reasons, not one:**
1. **The per-sibling call (`QuantizePositionToLayerGrid`) is unreachable from outside.** It fires deep
   inside `UpdateInstanceDragGesture<Traits>`'s own per-frame loop, once per correspondence entry the
   template itself is iterating. A caller-side wrapper has no hook between "the template started this
   frame's update" and "the template wrote sibling N's position" — the only way to intercept there is to
   copy the loop itself outside the template, which is precisely the domain-touching duplication of pure
   mechanics `§19.2`/`§21.2` already rule against for this exact class of code.
2. **Even the once-per-gesture calls (`IsInstanceLayerLocked`, `ResolveEffectiveSymmetry`) cannot be
   satisfied by a stateless `Traits` method reaching for `recipe.markerLinks` on its own.** `Traits`
   methods are `static`, deliberately stateless, called with no captured context — the only channel by
   which `MarkerDragTraits::IsInstanceLayerLocked` could reach the Link roster without it being passed in
   is a thread-local/global reference, which would contradict this whole file family's own repeatedly
   stated purity contract ("Pure, imgui-free, testable with no window," `InstanceDragGesture_UI.h`'s own
   header comment). Threading `links` through the template's own parameter list — the same explicit
   channel `layers` already uses — is the only mechanism consistent with that contract.
3. **`BeginInstanceDragGesture`'s own internal lock check is independently load-bearing, not a redundant
   safety net a canvas-side pre-check could replace.** `MarkerDragGesture_UI_Test.cpp` calls
   `BeginMarkerDragGesture`/`RepositionSymmetryGroupMember` directly, with no prior hit-test gate,
   exercising the refusal-on-locked-layer behavior as `BeginInstanceDragGesture`'s own contract
   (`RunLockRefusesBeginMarkerDragGestureChecks`). A future non-canvas caller is free to do the same.
   The internal check must be correct on its own, independent of whatever any particular caller did
   upstream — it cannot be deleted in favor of an outer gate.

**Ruled: widen. Both contracts gain the transform (not merely its `layerIndex`) plus the domain's own
Link roster, threaded exactly as `layers` already is. Props/Decals gain the new parameters too (never
avoidable, per point 2 above) but every one of their own implementations stays an inert pass-through —
the identical treatment `IsCardinalityFrozenGroup`/`SeedInstanceName`/`MakeInstanceNamesUnique` already
get for their own Markers-only special cases (`§21.3`).**

#### New shared placeholder type — `InstanceDragGesture_UI.h`

```cpp
// A domain with no Link concept (Props, Decals) instantiates every Traits::Link-shaped parameter with
// this permanently-empty, never-populated, never-read placeholder — shared so Props/Decals don't each
// invent their own dummy type for the same purpose.
struct NoInstanceLink {};
```

#### `Traits` contract — three methods widened, the rest unchanged; one renamed

```cpp
struct Traits {
    using Group = /* unchanged */;  using Transform = /* unchanged */;  using Layer = /* unchanged */;
    using Link  = /* Markers: Params::MarkerLink. Props/Decals: NoInstanceLink. */

    static Group*     SelectedGroup(std::vector<Group>&, int groupIndex);          // unchanged
    static Transform* SelectedInstance(std::vector<Transform>&, int transformIndex); // unchanged

    // RENAMED from IsInstanceLayerLocked — "LayerLocked" now actively misdescribes it: a Link can lock
    // an instance independent of its Layer (§19.33). WIDENED: takes the whole Transform (not a bare
    // layerIndex — the Transform already carries layerIndex, so this is a simplification, not just an
    // addition) plus the domain's own Link roster.
    // Markers: transform.linkIdentifier >= 0 and resolves -> that Link's bLocked; else -> the owning
    // Layer's bLocked (§19.33's own two-step order). Props/Decals: ignore `links`, forward
    // transform.layerIndex to the unchanged IsPropInstanceLayerLocked/IsDecalInstanceLayerLocked.
    static bool IsInstanceEffectivelyLocked(const std::vector<Layer>& layers, const Transform& transform,
                                            const std::vector<Link>& links);

    // WIDENED — same transform-instead-of-bare-layerIndex shape, same links roster.
    static void QuantizePositionToLayerGrid(const std::vector<Layer>& layers, const Transform& transform,
                                            const std::vector<Link>& links, float& x, float& z);
    static void ResolveEffectiveSymmetry(const std::vector<Layer>& layers, const Transform& transform,
                                         const std::vector<Link>& links, int globalMask,
                                         int globalRadialCount, int& outMask, int& outRadialCount);

    static bool IsCardinalityFrozenGroup(const Group&);                 // unchanged
    static int  NextInstanceIdentifier(const std::vector<Group>&);      // unchanged
    static void SeedInstanceName(Transform&, int);                      // unchanged
    static void MakeInstanceNamesUnique(std::vector<Transform>&);       // unchanged
};
```

#### Generic template functions — three widened, `EndInstanceDragGesture` untouched

```cpp
template<typename Traits>
bool BeginInstanceDragGesture(InstanceDragGestureState& state,
                              const std::vector<typename Traits::Group>& instances,
                              const std::vector<typename Traits::Layer>& layers,
                              const std::vector<typename Traits::Link>& links,     // NEW, inserted
                              const Params::Geometry& geometry, int globalSymmetryMask,     // right after layers
                              int globalRadialRepeatCount, int groupIndex, int transformIndex);

template<typename Traits>
void UpdateInstanceDragGesture(InstanceDragGestureState& state, std::vector<typename Traits::Group>& instances,
                               const std::vector<typename Traits::Layer>& layers,
                               const std::vector<typename Traits::Link>& links,    // NEW
                               const Params::Geometry& geometry, float newWorldX, float newWorldZ);

// EndInstanceDragGesture<Traits> — UNCHANGED. Do not add a links parameter; it would be dead weight
// (confirmed above: it calls none of the three widened Traits methods).

template<typename Traits>
bool RepositionSymmetryGroupMember(std::vector<typename Traits::Group>& instances,
                                   const std::vector<typename Traits::Layer>& layers,
                                   const std::vector<typename Traits::Link>& links,   // NEW
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount, int groupIndex,
                                   int movedTransformIndex, float newWorldX, float newWorldZ);
```
Bodies: every existing `Traits::IsInstanceLayerLocked(layers, X.layerIndex)` call becomes
`Traits::IsInstanceEffectivelyLocked(layers, X, links)`; every `Traits::QuantizePositionToLayerGrid(layers,
X.layerIndex, ...)`/`Traits::ResolveEffectiveSymmetry(layers, X.layerIndex, ...)` call becomes the same
shape with `X` (the already-in-scope `dragged`/`sibling`/`dragged` pointer, dereferenced) replacing
`X.layerIndex` and `links` appended. No other line in `InstanceDragGesture_UI.h` changes.

#### Markers-side resolvers (`MarkersTab_ManualLayerHelpers_UI.h`) — widened, plus the new sixth resolver

`§19.33` specified five of its six instance-tier resolvers concretely and left `bLocked`'s exact
consumer-facing shape to whichever ticket built the drag/hit-test consumers ("a widened predicate
signature `isInstanceLocked(instanceIdentifier, layerIndex)`... flag for the coder ticket explicitly" —
offered as a placeholder, not a ruling). **This section supersedes that placeholder shape** — passing
the transform itself is strictly better than a bare `instanceIdentifier`, since the caller would
otherwise need a second lookup just to read the transform's own `linkIdentifier`.

```cpp
// NEW — the sixth §19.33 governed-field resolver, same three-parameter shape as its five siblings:
bool EffectiveManualMarkerInstanceLocked(const Params::MarkerTransform& transform,
                                         const Params::MarkerInstanceLayer& layer,
                                         const std::vector<Params::MarkerLink>& links);

// NEW — out-of-range-safe convenience wrapper (mirrors IsMarkerInstanceLayerLocked's own established
// convention exactly), what Traits::IsInstanceEffectivelyLocked's Markers implementation actually calls:
bool IsMarkerInstanceLocked(const Params::MarkerTransform& transform,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::MarkerLink>& links);
// IsMarkerInstanceLayerLocked(markerLayers, layerIndex) — UNCHANGED, NOT deleted. Stays the correct
// call for any site with only a layerIndex in hand (a Layer-row header lock toggle), per the identical
// "old two-parameter shape survives, new shape is for per-instance sites" posture §19.33 already ruled.

// WIDENED (was layerIndex-only) — resolves instance-tier first (§19.33), THEN Layer-tier. The
// Layer-tier step itself gains the Link-resolution §19.31 already specified in prose but never gave a
// concrete signature for (`ResolveEffectiveMarkerSymmetry`'s own comment: "this ruling adds a
// Link-resolution step ahead of its existing... resolution") — this ruling closes that latent gap in
// the same stroke, since a coder cannot build the instance tier without the Layer tier underneath it
// also compiling against `links`.
void QuantizeMarkerPositionToLayerGrid(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                       const Params::MarkerTransform& transform,
                                       const std::vector<Params::MarkerLink>& links,
                                       float& worldX, float& worldZ);
void ResolveEffectiveMarkerSymmetry(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                    const Params::MarkerTransform& transform,
                                    const std::vector<Params::MarkerLink>& links,
                                    int globalSymmetryMask, int globalRadialRepeatCount,
                                    int& outMask, int& outRadialRepeatCount);
```
Resolution order for both, identical to every other §19.33 resolver: (1) `transform.linkIdentifier >= 0`
and resolves → that Link's own field; (2) else `layer.linkIdentifier >= 0` and resolves → that Link's
own field (the Layer-tier mechanism); (3) else → the Layer's own stored field. A dangling identifier at
either tier soft-degrades to the next step (Constitution §6), never a refusal.

#### `PropDragTraits`/`DecalDragTraits` — mechanical, no new logic

```cpp
using Link = NoInstanceLink;
static bool IsInstanceEffectivelyLocked(const std::vector<Layer>& layers, const Transform& transform,
                                        const std::vector<Link>&) {           // `links` ignored
    return IsPropInstanceLayerLocked(layers, transform.layerIndex);          // (Decals: IsDecalInstance...)
}
static void QuantizePositionToLayerGrid(const std::vector<Layer>& layers, const Transform& transform,
                                        const std::vector<Link>&, float& x, float& z) {
    QuantizePropPositionToLayerGrid(layers, transform.layerIndex, x, z);     // unchanged body otherwise
}
static void ResolveEffectiveSymmetry(const std::vector<Layer>& layers, const Transform& transform,
                                     const std::vector<Link>&, int globalMask, int globalRadialCount,
                                     int& outMask, int& outRadialCount) {
    ResolveEffectivePropSymmetry(layers, transform.layerIndex, globalMask, globalRadialCount,
                                 outMask, outRadialCount);
}
```
`QuantizePropPositionToLayerGrid`/`ResolveEffectivePropSymmetry`/their Decal siblings — **unchanged**,
still 2-parameter (`layers`, `layerIndex`); Props/Decals have no Link tier to add a third parameter for.

#### `ManualInstanceHitTest_UI.h`/`.cpp` and `ManualInstanceDelete_UI.h`/`.cpp` — predicate widened to take the transform

**New one-line additive alias on each manual Group struct** (`MarkerInstance_PARAMS.h`,
`PropInstance_PARAMS.h`) — lets the two files below stay `GroupT`-generic without hand-written
per-domain trait specializations:
```cpp
struct MarkerInstanceGroup { /* unchanged fields */ using TransformType = MarkerTransform; };
struct PropInstanceGroup   { /* unchanged fields */ using TransformType = PropTransform;  };
struct DecalInstanceGroup  { /* unchanged fields */ using TransformType = DecalTransform; };
```

```cpp
// ManualInstanceHitTest_UI.h — both functions, predicate WIDENED and RENAMED (isLayerLocked ->
// isInstanceLocked, since it can no longer be answered from layerIndex alone):
template<typename GroupT>
bool HitTestManualInstances(const std::vector<GroupT>& instances, const PreviewComposite&,
                            const MapCanvasView&, float regionLocalX, float regionLocalY,
                            float pickRadiusScreenPixels,
                            const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked,
                            int& outGroupIndex, int& outTransformIndex, float* outDistanceSquared = nullptr);
template<typename GroupT>
void CollectManualInstancesInWorldRegion(const std::vector<GroupT>& instances,
                                         float worldMinX, float worldMinZ, float worldMaxX, float worldMaxZ,
                                         const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked,
                                         std::vector<std::pair<int, int>>& outGroupTransformPairs);

// ManualInstanceDelete_UI.h — identical widening, same rename:
template<typename GroupT>
int DeleteManualInstancesById(std::vector<GroupT>& instances, const std::vector<int>& identifiers,
                              const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked);
```
Body change in all three: `isLayerLocked(transform.layerIndex)` → `isInstanceLocked(transform)`. Every
caller-bound lambda simply receives the whole transform instead of a bare int:
- **Markers**: `[&](const Params::MarkerTransform& t){ return layers != nullptr && IsMarkerInstanceLocked(t, *layers, links); }` — needs a `links` (`const std::vector<Params::MarkerLink>&`) closed over alongside the existing `layers` capture. `MapCanvas` already holds `manualMarkerDragRecipe` (a `Params::MapRecipe*`, used today for `globalSymmetryMask`) — `manualMarkerDragRecipe->markerLinks` is that source; **no new injected pointer field is needed on `MapCanvas_UI.h`.**
- **Props/Decals**: `[&](const Params::PropTransform& t){ return layers != nullptr && IsPropInstanceLayerLocked(*layers, t.layerIndex); }` — literally today's logic, reading `.layerIndex` off the now-whole transform instead of receiving it as the lambda's own parameter. Zero new logic.

`DeleteSelectedManualMarkerInstances` (the concrete Markers wrapper) gains a fourth parameter
`const std::vector<Params::MarkerLink>& markerLinks` to build its lambda; `DeleteSelectedManualPropInstances`/
`DeleteSelectedManualDecalInstances` signatures are unchanged (their lambdas need nothing new).

#### Binding consumer-site audit (completes `§19.33`'s own, named per this ruling's specifics)

- **`MapCanvas_ManualDragDispatch_UI.cpp`** — `HitTestManualInstanceAcrossDomains`'s three lambdas
  (widened per above, Markers sourcing `manualMarkerDragRecipe->markerLinks`); `TryBeginManualInstanceDrag`'s
  three `BeginInstanceDragGesture<Traits>` calls and `ContinueManualInstanceDrag`'s three
  `UpdateInstanceDragGesture<Traits>` calls each gain a `links` argument (Markers: `manualMarkerDragRecipe->markerLinks`;
  Props/Decals: a shared `static const std::vector<NoInstanceLink> kNoLinks;`, one instance reusable by
  both, mirroring the file's own existing `kNoMarkerLayers`-style statics). `EndManualInstanceDrag` —
  **no change**, `EndInstanceDragGesture<Traits>` is unwidened.
- **`MapCanvas_SelectionGesture_UI.cpp`** — `ApplyMarqueeGesture`'s three
  `CollectManualInstancesInWorldRegion<GroupT>` calls, identical lambda-widening and links-sourcing
  treatment as the hit-test lambdas above.
- **`MarkerDragGesture_UI.h`**'s three concrete wrapper functions (`BeginMarkerDragGesture`,
  `UpdateMarkerDragGesture`, the concrete `RepositionSymmetryGroupMember` overload) each gain a
  `const std::vector<Params::MarkerLink>& markerLinks` parameter, threaded straight into their own
  `<MarkerDragTraits>` call. Every existing caller of these three (`MarkerDragGesture_UI_Test.cpp`,
  and any other non-canvas call site) needs a `markerLinks` argument added — mechanical, not a design
  question (tests may pass `{}`).
- **`PropDragTraits`/`DecalDragTraits`** — gain `using Link = NoInstanceLink;` plus the three widened,
  inert-passthrough method bodies shown above.
- **`ManualInstanceDelete_UI.h`/`.cpp`** — `DeleteManualInstancesById<GroupT>`'s predicate widened per
  above; `DeleteSelectedManualMarkerInstances` gains the `markerLinks` parameter.
- Any non-UI/PIPELINE/PROC read site touching `IsMarkerInstanceLayerLocked`/`QuantizeMarkerPositionToLayerGrid`/
  `ResolveEffectiveMarkerSymmetry` outside this list needs Compute/Generator Expert sign-off before a
  coder ticket treats it, per `§19.33`'s own general routing instruction — not assumed clear here.

#### Explicitly NOT resolved here — flagged, not guessed

**Whether a symmetry sibling materialized mid-drag (`EndInstanceDragGesture`'s unclaimed-slot loop)
should inherit the dragged member's own `linkIdentifier`.** `EndInstanceDragGesture` is unwidened by
this ruling and, as written, a freshly materialized transform's `linkIdentifier` is whatever
`MarkerTransform`'s own default is (`-1`, per `§19.33`) — an orbit-growth event during a drag on a
Link-tagged group currently produces an UN-tagged new sibling. Neither `§19.33` nor this section rules
on whether that is correct product behavior (arguably the new sibling should join the same Link, since
it is presented as "the same marker, mirrored" — but arguably not, since Link membership was ruled a
deliberate per-instance tag, not an automatic symmetry-orbit property). **Left OPEN for a future,
explicit human ruling** — not a consequence of this contract-widening ruling, and not to be guessed at
by a coder.

#### Cross-references updated

`ARCH_21_03_DragGestureGenericization.md` and `ARCH_21_05_LockedItemExclusionCorrection.md` each gain a
short banner pointing here; `ARCH_19_33_LinkMembershipInstanceTierCorrection.md`'s own "Binding
consumer-site audit" bullet on this topic gains a short forward-reference note. No other file's ruling
text is altered.

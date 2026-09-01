# STEP249 — Widen the shared drag-gesture / hit-test / delete generics for instance-tier Link resolution

**Layer:** UI. **Domain:** `src/ui/InstanceDragGesture_UI.h`, `src/ui/MarkerDragGesture_UI.h`,
`src/ui/PropDragGesture_UI.h`, `src/ui/DecalDragGesture_UI.h`, `src/ui/ManualInstanceHitTest_UI.h/.cpp`,
`src/ui/ManualInstanceDelete_UI.h/.cpp`, `src/ui/MapCanvas_ManualDragDispatch_UI.cpp`,
`src/ui/MapCanvas_SelectionGesture_UI.cpp`. **Sequence:** depends on STEP244 (the `TransformType`
aliases) and STEP246 (`IsMarkerInstanceLocked`, widened `QuantizeMarkerPositionToLayerGrid`/
`ResolveEffectiveMarkerSymmetry`).

⚠️ **This ticket touches shared generics also instantiated for Props and Decals, which have NO Link
concept.** Every Props/Decals change here must be a mechanical, inert pass-through (read `layerIndex`
off the now-whole transform instead of receiving it as a bare int, call the SAME unchanged
`IsPropInstanceLayerLocked`/`QuantizePropPositionToLayerGrid`/etc. functions they already call) — no
new logic, no new behavior, for either domain. Ratifies `ARCH_21_09_LinkTierContractWidening.md` in
full; read it before starting, it specifies exact signatures for every function this ticket touches —
this ticket restates it, it is not a paraphrase to work from independently.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above — Props/Decals maintainers
(if any concurrent session owns those tabs) should be aware this ticket touches their shared
call-path files even though it changes none of their actual behavior.

## Fix

Follow `ARCH_21_09` section-by-section; summarized here for sequencing, not as the full spec:

1. **`InstanceDragGesture_UI.h`** — add the shared placeholder `struct NoInstanceLink {};`. Rename
   `Traits::IsInstanceLayerLocked` → `Traits::IsInstanceEffectivelyLocked`, widen it plus
   `Traits::QuantizePositionToLayerGrid`/`Traits::ResolveEffectiveSymmetry` to take the whole
   `Transform` (not bare `layerIndex`) plus a new `const std::vector<typename Traits::Link>& links`
   parameter. Add `using Link = ...;` to the `Traits` contract. Widen `BeginInstanceDragGesture`/
   `UpdateInstanceDragGesture`/`RepositionSymmetryGroupMember` (all three templates) to accept and
   thread a `links` parameter through to every call site of the three widened `Traits` methods.
   **`EndInstanceDragGesture` is NOT widened** — confirmed by ARCH to call none of the three affected
   methods; do not add a dead `links` parameter to it.
2. **`MarkerDragGesture_UI.h`** — `MarkerDragTraits` implements the widened contract for real:
   `IsInstanceEffectivelyLocked` calls the new `IsMarkerInstanceLocked` (STEP246); the two other
   methods call the newly-widened `QuantizeMarkerPositionToLayerGrid`/`ResolveEffectiveMarkerSymmetry`
   (STEP246) directly. `using Link = Params::MarkerLink;`. The three concrete wrapper functions
   (`BeginMarkerDragGesture`, `UpdateMarkerDragGesture`, the concrete `RepositionSymmetryGroupMember`
   overload) each gain a `const std::vector<Params::MarkerLink>& markerLinks` parameter, threaded into
   their own `<MarkerDragTraits>` template call.
3. **`PropDragGesture_UI.h`/`DecalDragGesture_UI.h`** — `using Link = NoInstanceLink;`; all three
   methods become inert pass-throughs per `ARCH_21_09`'s exact bodies (ignore the `links` parameter,
   forward `transform.layerIndex` to the existing unchanged `IsPropInstanceLayerLocked`/
   `QuantizePropPositionToLayerGrid`/`ResolveEffectivePropSymmetry` — Decal siblings identical shape).
   `QuantizePropPositionToLayerGrid`/`ResolveEffectivePropSymmetry`/Decal equivalents themselves are
   **unchanged, still 2-parameter** — Props/Decals have no third tier to add a parameter for.
4. **`ManualInstanceHitTest_UI.h`/`.cpp`** — `HitTestManualInstances<GroupT>`/
   `CollectManualInstancesInWorldRegion<GroupT>`: rename+widen the predicate parameter from
   `std::function<bool(int layerIndex)> isLayerLocked` to
   `std::function<bool(const typename GroupT::TransformType&)> isInstanceLocked` (uses STEP244's new
   `TransformType` alias). Body change: `isLayerLocked(transform.layerIndex)` →
   `isInstanceLocked(transform)`.
5. **`ManualInstanceDelete_UI.h`/`.cpp`** — `DeleteManualInstancesById<GroupT>`: identical predicate
   rename+widening as step 4. `DeleteSelectedManualMarkerInstances` gains a fourth parameter
   `const std::vector<Params::MarkerLink>& markerLinks` to build its lambda. Props/Decals wrappers
   (`DeleteSelectedManualPropInstances`/`DeleteSelectedManualDecalInstances`) — signatures unchanged.
6. **`MapCanvas_ManualDragDispatch_UI.cpp`** — `HitTestManualInstanceAcrossDomains`'s three lambdas
   widen per step 4/5's new predicate shape:
   - Markers: `[&](const Params::MarkerTransform& t){ return layers != nullptr && IsMarkerInstanceLocked(t, *layers, links); }`,
     sourcing `links` from the already-in-scope `manualMarkerDragRecipe->markerLinks` — **no new
     injected pointer field on `MapCanvas_UI.h`**, this pointer already exists for
     `globalSymmetryMask`.
   - Props/Decals: `[&](const Params::PropTransform& t){ return layers != nullptr && IsPropInstanceLayerLocked(*layers, t.layerIndex); }`
     — literally today's logic, reading `.layerIndex` off the whole transform now in hand.
   `TryBeginManualInstanceDrag`'s three `BeginInstanceDragGesture<Traits>` calls and
   `ContinueManualInstanceDrag`'s three `UpdateInstanceDragGesture<Traits>` calls each gain a `links`
   argument: Markers passes `manualMarkerDragRecipe->markerLinks`; Props/Decals pass a shared
   `static const std::vector<NoInstanceLink> kNoLinks;` (one instance, reused by both). `EndManualInstanceDrag`
   — no change.
7. **`MapCanvas_SelectionGesture_UI.cpp`** — `ApplyMarqueeGesture`'s three
   `CollectManualInstancesInWorldRegion<GroupT>` calls get the identical lambda-widening/links-sourcing
   treatment as step 6's hit-test lambdas.

## Explicitly not this ticket's call

`ARCH_21_09` leaves OPEN whether a symmetry sibling materialized mid-drag (`EndInstanceDragGesture`'s
unclaimed-slot loop) should inherit the dragged member's `linkIdentifier`. **Human ruling, this
session: NO — a freshly materialized sibling starts unlinked (`linkIdentifier = -1`, the struct
default), full stop.** Since `EndInstanceDragGesture` is unwidened by this ticket (per step 1), this
requires no code change at all — confirm it as a test case, not an implementation task: a Link-tagged
marker's symmetry counterpart, when first created via drag-orbit-growth, has `linkIdentifier == -1`.

## Verify

- **Regression floor, checked first**: every existing `MarkerDragGesture_UI_Test.cpp`,
  `PropDragGesture_UI_Test.cpp`/equivalent, `DecalDragGesture_UI_Test.cpp`/equivalent,
  `ManualInstanceHitTest`-related tests, `ManualInstanceDelete_UI_Test.cpp` stays green with ZERO
  behavior change for Props/Decals and for Markers scenarios that don't use Links at all. This is the
  highest-risk ticket in this whole correction (shared generics, three domains) — do not treat a green
  build as sufficient; re-run and read output for all three domains' full suites explicitly.
- New Markers-only cases: a Link-tagged, `bLocked`-Link instance refuses drag-begin even though its
  own Layer is unlocked; a Link-tagged instance's grid-snap/symmetry resolve from its Link during a
  live drag, not its Layer, when the two disagree; hit-test/marquee/Delete all honor an instance-tier
  Link lock the same way `BeginInstanceDragGesture` does.
- The explicitly-not-this-ticket case above: confirm (new test) that a drag-orbit-growth-materialized
  sibling of a Link-tagged instance has `linkIdentifier == -1`.
- Full project test suite green, not just the files this ticket touches — this is the ticket most
  likely to have a non-obvious cross-domain ripple.

## Out of scope

- Any further Markers-only resolver/mechanic change — STEP244-248 already landed by the time this
  ticket runs.
- Deciding symmetry-sibling Link inheritance any way OTHER than "no" — that's a settled human ruling
  for this correction, not open for this ticket to revisit.

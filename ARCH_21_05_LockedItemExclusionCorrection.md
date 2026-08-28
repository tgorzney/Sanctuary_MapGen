[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.5. **Only the ARCH Expert writes this file.**

### 21.5 Locked-item exclusion — corrects §19.18; uniform across click, marquee, and drag; procedural instances unaffected

**Human-decided.** Reverses the reading `ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md`
§19.18 implicitly carried (a locked Marker instance was still a legal click-select target —
confirmed live by direct read: `HitTestManualMarkers`, `MapCanvas_MarkerHitTest_UI.cpp`, has ZERO
lock check anywhere in its scan; only `BeginMarkerDragGesture`'s own `IsMarkerInstanceLayerLocked`
guard, `MarkerDragGesture_UI.cpp:43`, gates drag). **§19.18 is corrected in place** (not left
standing beside a contradicting new rule) — see that file's own amended paragraph; this section is
the binding rule text, §19.18 carries a forward cross-reference.

**Binding rule.** A locked instance's owning layer's `bLocked` — resolved through the transform's
`layerIndex` via `IsMarkerInstanceLayerLocked`/`IsPropInstanceLayerLocked`/`IsDecalInstanceLayerLocked`
(all three confirmed real, live, UI-layer functions — `MarkersTab_ManualLayerHelpers_UI.h:29`,
`PropsTab_Manual_UI.h:102`, `DecalsTab_Manual_UI.h:78`) — gates ALL THREE of:
1. Becoming a click-select hit (§21.2's release-time click resolution).
2. Being collected into a marquee/box-select result (§21.2's release-time marquee resolution).
3. Drag-gesture begin (already correctly gated today for Markers, `BeginMarkerDragGesture`;
   extends unchanged to Props/Decals once §21.3's `PropDragTraits`/`DecalDragTraits` ship).

**Mechanism — one shared gate, not three.** §21.3's `HitTestManualInstances<GroupT>` and
`CollectManualInstancesInWorldRegion<GroupT>` both take an additional
`const std::function<bool(int layerIndex)>& isLayerLocked` parameter; a locked transform is skipped
during the scan and never becomes a candidate at all — satisfying (1) and (2) through the SAME
templated code path, never a second copy. Each domain's caller binds its own real predicate (a
lambda over that domain's own `IsXInstanceLayerLocked` closed over that domain's own layers vector)
— the predicate itself stays domain-agnostic in the template's own signature (a `std::function`, no
concrete `Params::` type), consistent with §21.3's whole genericity posture. (3) is unaffected —
already a direct `Traits::IsInstanceLayerLocked` call inside `BeginInstanceDragGesture<Traits>`.

**No retroactive deselection.** Locking a layer AFTER one of its instances is already selected does
NOT clear that selection — this rule gates ACQUIRING selection (click/marquee/drag-begin), not an
active-selection audit on every lock toggle. §19.18's tint-priority ruling therefore still has real
force for exactly that narrowed case (an instance selected before its layer became locked draws the
select tint normally, per §19.18's own unedited priority order) — the sentence §19.18 amends only
removes the CLAIM that a locked instance can freshly become selected, not the tint behavior once it
already is.

**Procedural instances are entirely unaffected — confirmed by direct read, not merely asserted.**
`bLocked` exists only on `MarkerInstanceLayer`/`PropInstanceLayer`/`DecalInstanceLayer` (the MANUAL
layer-metadata types); `Data::PlacementInstances` (the procedural SoA §21.6's `PickInstancesInRegion`
and `Picking_UI::PickMarker` both read) carries no such column and has no layer-membership concept
at all to resolve one through. §19.27's own binding sentence — procedural instances have no lock
concept — is restated here as still correct, not reopened.

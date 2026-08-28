[← ARCH index](ARCH.md) · SanGen ARCH §21. **Only the ARCH Expert writes this file.**

## §21 Canvas interaction unification — multi-select, drag-gesture genericization, uniform locked-item exclusion, and the shared picking substrate

Ratifies the UI Expert's design-round consult routed by `ARCH_20_04_DragGestureSubstrateRouting.md`
§20.4 (the Props/Decals drag-reposition substrate, unified with the separately-paused canvas
click/box-select initiative, per that section's own explicit instruction to treat both as one
consult). **Closes §20.4's gate** — a coder work-order may now build against §21 directly; §20.4
itself is left standing as the historical routing record, with a closing cross-reference appended
in place.

Independently verified against the live code before ruling, not taken on the design's word alone:
`MapCanvas_UI.h`/`.cpp`, `MapCanvas_Draw_UI.cpp`, `MapCanvas_IconLayer_UI.h`,
`MapCanvas_IconLayer_CullManual_UI.cpp`, `Picking_UI.h`/`.cpp`, `SpatialGrid_DATA.h`,
`RuleBucketIndexSet_DATA.h`, `PlacementResults_DATA.h`, `GenerationAssembler_PIPELINE.h`,
`GenerationAssembler_Stages_PIPELINE.cpp`, `MapCanvas_MarkerHitTest_UI.cpp`,
`MapCanvas_MarkerDrag_UI.h`/`.cpp`, `MarkerDragGesture_UI.h`/`.cpp`, `MarkerDragGesture_Frame_UI.cpp`,
`MarkerOrbitCorrespondence_UI.h`, `MarkerSelectionHighlight_UI.h`,
`MarkersTab_ManualInstanceSelection_UI.h`, `ScatterInstanceLayer_PARAMS.h`, `PropInstance_PARAMS.h`,
`MarkerInstance_PARAMS.h`, `MarkerInstanceId_UI.h`, `UniqueNameList_UI.h`, `Application_UI.cpp`,
`Application_Draw_UI.cpp`. Several real refinements/corrections to the relayed design surfaced by
that re-read, ratified in place below rather than left implicit — most notably: §21.3 finds
`PropTransform`/`DecalTransform` carry no `name` field at all (unlike `MarkerTransform`), so two of
the design's `Traits` hook points cannot be implemented as literally described for Props/Decals and
are ruled inert-by-construction instead; §21.3 also finds the drag-gesture STATE struct itself needs
no template parameter (only the four functions operating on it do); §21.6 corrects the proposed
region-query function's name (`PickMarkersInRegion` → `PickInstancesInRegion`, since its own stated
contract is fully domain-generic).

| § | Ruling |
|---|--------|
| §21.1 | Multi-select representation — `OverlayInstanceKeySet_UI`, the widened `MapCanvas` selection surface, the callback signature |
| §21.2 | Gesture ownership — press-time drag-begin-first, release-time click/marquee, the independent right-button pan |
| §21.3 | Drag-gesture genericization — `InstanceDragGestureState`, `BeginInstanceDragGesture<Traits>`/etc., `MarkerDragTraits`/`PropDragTraits`/`DecalDragTraits`, `HitTestManualInstances<GroupT>`/`CollectManualInstancesInWorldRegion<GroupT>` |
| §21.4 | `PropTransform`/`DecalTransform` gain `instanceIdentifier`/`symmetryGroupIdentifier` — mirrors `MarkerTransform` verbatim |
| §21.5 | Locked-item exclusion — corrects §19.18; uniform across click/marquee/drag; procedural instances unaffected |
| §21.6 | Picking infrastructure — `Data::SpatialGridSet`, `BuildSpatialGridSet`, three new `SpatialGrid` accessors, `PickInstancesInRegion` |
| §21.7 | File-size ceiling flag — `MapCanvas_UI.h` |

**Interlocking, not independently dispatchable — except §21.4.** §21.1–§21.3, §21.5, and §21.6 are
one mechanism (a marquee release alone needs §21.1's set representation, §21.2's release-time
resolution, §21.3's generic collect, §21.5's lock gate, and §21.6's region query, all at once); no
coder work-order should build one in isolation from the rest. §21.4 is the one severable exception —
pure PARAMS+IO, buildable and shippable standalone, exactly as §20.1–§20.3/§20.6 already shipped
ahead of their own gated consumer (§20.4's own closing sentence, restated for §21.4 in that section).

**Units are out of scope for the selection/drag work this ratifies.** §21.6's picking infrastructure
is built 4-way (mirroring `RuleBucketIndexSet`'s own precedent of shipping full-width infrastructure
ahead of every consumer existing), but no `UnitTransform::instanceIdentifier` field, no
`UnitDragGesture`, and no click/marquee consumer for Units is ratified anywhere in this section —
see §21.6's own closing note.

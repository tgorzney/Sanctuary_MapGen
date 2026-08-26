[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.26. **Only the ARCH Expert writes this file.**

### 19.26 Manual-instance symmetry-cluster grouping in the instance list — UI composition only, no PARAMS change, ratified as designed
Responds to item 12. **Ratified as designed** — a pure UI composition, no new field; recorded per
the design's own request that the grouping shape be recorded even though nothing new is added to
`Params`.

Inside `DrawLayerRowBody`'s existing instance-list block (the block `ManualInstanceLayerIndex_UI`
already feeds — `ManualInstanceLayerIndex_UI.h`'s own header comment notes "no dedicated ARCH_19_2x
ruling exists for this UI shape," now superseded by this one): partition the layer's
`(groupIndex, transformIndex)` pairs by `MarkerTransform::symmetryGroupIdentifier`. Non-zero buckets
render FIRST, each its own collapsible `ImGui::TreeNodeEx` node labeled `"Symmetry Group N (k)"`,
containing the same `Selectable` rows the flat list already draws; every
`symmetryGroupIdentifier == 0` instance then lists flat, individually, after all groups —
unchanged row body, no new widget needed.

**Binding: `== 0` is this ruling's ungrouped predicate — manual instances only. Do not port it to
§19.27's procedural grouping.** `symmetryGroupIdentifier` is written only by drag-materialize and the
Fix Symmetry repair tool (confirmed by §19.19's own citation) and legitimately stays `0` for a manual
marker that has never been dragged or repaired — `0` really does mean "no cluster" here. §19.27
rules a DIFFERENT predicate for the procedural analogue, because
`Data::PlacementInstance::symmetryIdentifier`'s minting semantics differ (never `0`, confirmed by
direct read — see §19.27). Conflating the two predicates would silently mis-group one of them; kept
explicitly separate on purpose.

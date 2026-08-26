[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.27. **Only the ARCH Expert writes this file.**

### 19.27 Procedural marker-instance listing/selection — per-frame `ruleIndex` positional index, convergence with §19.25, and the bucket-size symmetry-grouping rule; **narrows §19.20's "manual-only" framing**
Responds to item 13 — overridden by the human directly (`BRIEF_MarkersUICorrectionRound2_R1.md`
item 13: "This was wrong and is now overridden: build it"), after the earlier `ARCH_19_20` ruling had
scoped procedural instances out of listing/selection entirely. **Ratified as designed.**

**Mechanism — a straight analog of `ManualInstanceLayerIndex_UI`, confirmed by direct read of
`ManualInstanceLayerIndex_UI.h`.** No new `DATA` field. A new per-frame index, built once per frame
by walking `assembler.Placements().markers` (`Data::PlacementInstance::ruleIndex`, confirmed present,
`PlacementInstance_DATA.h:46`), keyed by `ruleIndex`, storing raw SoA array positions:
```cpp
struct ProceduralInstanceRuleIndex_UI {
    std::unordered_map<int /*ruleIndex*/, std::vector<int /*array position*/>> instancesByRuleIndex;
};
```
Rebuilt every frame the current `Data::PlacementInstances` snapshot is used from, never persisted —
same non-persistence posture `ManualInstanceLayerIndex_UI` already has. **Zero dirty-hash/DAG
participation** — a pure derived index over already-baked `DATA`, computed strictly after PROC has
run, never triggering or gated by `PlacementStage::ComputeParameterHash()`; this keeps it inside the
Tier C/C2 "zero dirty-hash involvement" cost model `ARCH_14_08_DirtyFlagTiers.md` §14.8 already rules
for authoring-scale UI-only derived state, applied here rather than re-derived.

**Selection key — the genuine convergence point with §19.25, ratified as one shared mechanism, not
two.** The key is the array position itself: `OverlayInstanceKey_UI{Markers, position, /*bValid=*/true,
bManual=false}` — the exact representation §19.25 establishes for canvas click-pick, because it is
the same array (`Data::PlacementInstances::markers`). A procedural-instance list-click routes through
§19.25's now-canonical `MapCanvas::SetSelection(const OverlayInstanceKey_UI&)` setter, the same entry
point canvas click-pick and §19.25's manual list-click both use — built once, per the design's own
explicit instruction, not implemented three times. **Session-only** — this selection never persists
to `.sanmap` and never needs to survive a re-bake; a fresh bake invalidates the index (rebuilt next
frame) and any prior selection simply stops resolving, the identical posture the manual index already
has toward `recipe.markers` edits.

**Symmetry-grouping parity — CONFIRMED, but with a DIFFERENT predicate than §19.26's manual rule; do
not port §19.26's `== 0` convention here.** `Data::PlacementInstance::symmetryIdentifier`
(`PlacementInstance_DATA.h:48`) exists and is populated for markers — confirmed by direct read of
`Placement_Accept_PROC.cpp:43,45`: `const int symmetryIdentifier = nextSymmetryIdentifier++;` is
assigned to every accepted candidate, mirror or not, and `nextSymmetryIdentifier` starts at `1`
(`Placement_PROC.cpp:44`, `Placement_PROC.h:127`) — so a lone, unmirrored instance still gets its own
unique NON-ZERO id; it is never `0`. **Binding rule: bucket procedural instances by
`symmetryIdentifier`; treat a bucket as a real "symmetry cluster" (collapsible, rendered first) ONLY
when it holds MORE THAN ONE instance — bucket size, not id value.** A bucket of exactly one instance
is a free/ungrouped row regardless of its (always non-zero) id, listed flat after every real cluster,
mirroring §19.26's Group-then-flat ordering with the opposite membership test.

**Narrows §19.20.** §19.20's "manual-only selection scope" headline no longer describes the whole
feature: procedural marker instances now have their OWN selection mechanism, this one. §19.20's ONE
binding sentence that still stands, honored here exactly: `instanceIdentifier` is never repurposed
for procedural identity — this mechanism keys procedural instances by array position (`bManual=false`),
a wholly separate, `DATA`-scoped, session-only identity, never `MarkerTransform::instanceIdentifier`.
The static symmetric-sibling highlight (§19.19) and select-color surface (§19.17/§19.18) remain
manual-only, unchanged by this ruling — this ticket did not extend those to procedural instances,
only listing/selection itself. See §19.20's own file for the full correction note.

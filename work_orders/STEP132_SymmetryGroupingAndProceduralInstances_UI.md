# STEP132 — Symmetry-cluster grouping (manual) + procedural instance listing/selection

**Layer:** UI. **Domain:** `MarkersTab_ManualLayerRowBody_UI.cpp`, `MarkersTab_RuleLayers_UI.h/.cpp`,
`MarkersTab_RuleLayerSettings_UI.cpp`, `MarkersTab_TypeSections_UI.cpp`, `MarkersTab_UI.h/.cpp`, a new
`ProceduralInstanceRuleIndex_UI.h`. **Sequence:** depends on STEP131 (landed — `MapCanvas::SetSelection`'s
canonical full-key overload, the shell-mediated selection callback shape). Independent of nothing
else in the current round.

Ratifies `ARCH_19_26_ManualInstanceSymmetryGrouping.md` (item 12) and
`ARCH_19_27_ProceduralInstanceSelectionMechanism.md` (item 13) together, per the design's own
recommendation — both add a "collapsible symmetry-cluster sub-list" shape to a very similar row and
should share one rendering helper rather than two near-duplicate implementations.

## Part A — Manual: symmetry-cluster grouping in `DrawLayerRowBody`'s instance list (ARCH §19.26)

Inside the existing instance-list block (`MarkersTab_ManualLayerRowBody_UI.cpp`, the block
`ManualInstanceLayerIndex_UI` already feeds): partition the layer's `(groupIndex, transformIndex)`
pairs by `MarkerTransform::symmetryGroupIdentifier`. Non-zero buckets render FIRST, each its own
collapsible `ImGui::TreeNodeEx` node labeled `"Symmetry Group N (k)"` (N = the group id, k = member
count), containing the same `Selectable` rows the flat list already draws. Every
`symmetryGroupIdentifier == 0` instance then lists flat, individually, after all groups — same row
body, no change to the row itself, only the surrounding grouping pass.

**Binding: `== 0` is the manual ungrouped predicate. Do NOT reuse this exact predicate for Part B.**

## Part B — Procedural: per-frame `ruleIndex` index + instance list + selection (ARCH §19.27)

**New file `ProceduralInstanceRuleIndex_UI.h`** (mirrors `ManualInstanceLayerIndex_UI.h`'s exact
shape and non-persistence posture):
```cpp
struct ProceduralInstanceRuleIndex_UI {
    std::unordered_map<int /*ruleIndex*/, std::vector<int /*array position*/>> instancesByRuleIndex;
};
// built by walking Data::PlacementInstances::markers, keyed by Data::PlacementInstance::ruleIndex.
// Rebuilt every frame it's used — zero dirty-hash/DAG participation, a pure derived index over
// already-baked DATA, never triggering PlacementStage::ComputeParameterHash().
```

**Instance-list UI, inside the Rule layer's own row body** (find/confirm the exact current draw
function — likely in `MarkersTab_RuleLayerSettings_UI.cpp` or wherever a `MarkerRuleLayer`'s selected
rule's detail is drawn; ground this in the live code, don't assume a function name from this ticket's
own prose). Mirror Part A's shared rendering shape: for the rule's own `ruleIndex`, look up
`instancesByRuleIndex`, group by `Data::PlacementInstance::symmetryIdentifier`, but with the OPPOSITE
membership test from Part A — **treat a bucket as a real cluster (collapsible, rendered first) ONLY
when it holds MORE THAN ONE instance; a bucket of size 1 is a free/ungrouped row, regardless of its
(always non-zero) id.** Extract the shared "draw N collapsible symmetry-cluster groups then M flat
rows" logic into one small helper both Part A and Part B call, parameterized on the bucket→cluster
predicate (`== 0` for manual, `size > 1` for procedural) and the row-label/selection callbacks — this
is the "share one rendering helper" instruction from the design doc, not two independent
implementations.

**Plumbing — `Data::PlacementInstances` must reach this row body.** `DrawMarkersTab` already receives
`const Data::PlacementInstances* placedMarkers = nullptr` (used today only by the separate, unfiltered
`DrawPlacedMarkerList`). Thread the SAME pointer down through `DrawMarkerTypeSections` →
`DrawRuleLayerListBody` → wherever the Rule row body draws (the same chain `previewDriver`/
`iconManifest`/STEP131's new `selectManualMarkerInstanceCallback` already ride down — one more
parameter on an existing chain, no new plumbing shape). Nullable throughout: before the first
generation, the Rule row's instance list renders `"(none — generate first)"`, mirroring
`DrawPlacedMarkerList`'s own existing null-handling.

**Selection — converges on STEP131's canonical setter, per ARCH §19.27, built once not twice.** A
click on a procedural instance row calls `MapCanvas::SetSelection({PlacementCollectionKind_UI::Markers,
position, /*bValid=*/true, /*bManual=*/false})` — the SAME setter and the SAME key shape STEP131
already established for canvas click-pick and manual list-click. This needs the same kind of
shell-mediated callback STEP131 built for the manual case (`selectManualMarkerInstanceCallback`'s
sibling) — do not invent a second selection path; if `MapCanvas::SetSelection(const
OverlayInstanceKey_UI&)` is already public (STEP131 made it so), a comparably-shaped
`selectProceduralMarkerInstanceCallback` threaded through `Application::WireCallbacks()` and down the
same UI call chain is the correct, minimal addition — confirm against STEP131's actual landed
signatures before writing this, don't guess.

**Session-only, no persistence** — this selection never writes to `.sanmap`; a fresh bake invalidates
the index (rebuilt next frame) and any prior selection simply stops resolving.

## Verify

- **Part A:** a fixture with 5 instances — 2 in symmetry group 3, 2 in symmetry group 7, 1 ungrouped
  (`== 0`) — renders exactly 2 collapsible cluster nodes (labeled with correct id/count) followed by
  exactly 1 flat row, in that order. Existing per-row click-to-select behavior (STEP126/STEP131)
  unchanged for rows inside a cluster.
- **Part B:**
  - `ProceduralInstanceRuleIndex_UI`/its builder: a synthetic `Data::PlacementInstances` fixture with
    markers tagged `ruleIndex` 0/0/1, confirms the index maps exactly the right array positions to
    each rule.
  - Symmetry-bucket grouping: a fixture where `symmetryIdentifier` values are `{5,5,9,12}` (bucket 5
    has 2 members, buckets 9 and 12 have 1 each) — confirms exactly 1 collapsible cluster (id 5, count
    2) renders first, then 2 flat rows, proving the bucket-SIZE predicate (not id value, and NOT the
    `== 0` predicate from Part A — id `0` never appears here per ARCH §19.27's own confirmed minting
    semantics).
  - Selection: a click on a procedural instance row calls the SAME `MapCanvas::SetSelection` entry
    point STEP131's manual click uses, with `bManual=false` and the array position as the key — assert
    via the shell callback's own test seam, mirroring STEP131's own selection-callback test shape.
  - Null-`placedMarkers` case renders the "generate first" placeholder, no crash, no interactive rows.
- Existing suites (`MarkersTab_ManualInstanceListRows_UI_Test`, `MarkersTab_RuleLayers_UI_Test`,
  `MarkersTab_UI_Test`, `MapCanvas_Picking_UI_Test`) stay green.

## Out of scope

- Extending select-color/static symmetric-sibling highlight (§19.17/§19.18/§19.19) to procedural
  instances — ARCH §19.27 explicitly keeps those manual-only; this ticket is listing/selection only.
- The dot-roster's redundant highlight branch (STEP131's own deferred item) — untouched here.

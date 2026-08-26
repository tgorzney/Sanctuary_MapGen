# DESIGN_MarkerGroupLayerRestructure_R1.md

Continuation of `work_orders/BRIEF_MarkerGroupLayerRestructure_R1.md` (read in full; its "final
corrected model" §ground-truth is not re-derived here). Cross-references `work_orders/BRIEF_Assembly_R1.md`
+ `work_orders/DESIGN_Assembly_R1.md` as reference material per this round's framing — Assembly is
unratified/unbuilt, so where this brief's new requirements force a call Assembly's design didn't
anticipate, I rule it here explicitly rather than deferring. DESIGN phase only, no code. Not
ratified — ARCH Expert pass required before any ticket is coder-dispatchable.

## 0. Naming collision — ruled

"Group" already means three things before this brief: (1) `Params::MarkerInstanceGroup` — the
manual TYPE-keyed outer dict (`recipe.markers["Alloy"]`, UI symbols `selectedGroupIndex`,
`DrawMarkerGroupSection`); (2) wire key `MarkerGroups` = `std::vector<Params::MarkerInstanceLayer>`
(Correction 16 — confusingly, wire-level "Group" names the C++ **Layer** metadata array); (3) the
procedural `Group(MarkerRuleLayer)→Rule` wrapper the brief cites from Correction 7/15/16's prior
framing (even though its actual wire key is `MarkersStack`, not "Group"). This brief's new
container-above-Layer concept would be a **fourth** meaning if spelled bare "Group."

**Ruling: give the new C++/wire type a distinct name; keep the UI display label "Group" if desired**
(the human's own worked example calls it "Group" — that's a cosmetic string, not a type identity,
so nothing forces the two to match). Checked for collisions in `src/`: "Cluster" is **already live**
and means something unrelated and adjacent — `ClusterByScreenCell` (`MapCanvas_IconLayer_Budget_UI.cpp`,
ARCH §14.9 screen-cell decimation) — do not reuse it, real collision risk with UI Optimization
Expert territory. "Bundle"/"Ensemble"/"Formation" — zero hits, clean. Working name used throughout
this doc: **`MarkerLayerGroup`** (PARAMS type) — pending ARCH's final naming-law pass; ARCH may
prefer "Bundle"/"Ensemble" instead. ❓ ARCH: rule the final type/wire name; confirm the
UI-label-vs-type-name split above is acceptable rather than forcing them to match.

## 1. The generic Group mechanism's shape — central question, ruled

Two genuinely different kinds of genericity are being asked for, and this codebase already has two
different established answers — apply both, don't force one mechanism to cover both:

- **Domain-touching logic** (reads real PARAMS fields: membership resolution, IO read/write, the
  Group's own settings) → **per-domain repeated struct + per-domain repeated pure-function family**.
  Mirrors `PropInstanceLayer`/`DecalInstanceLayer`/`MarkerInstanceLayer` (`PropInstance_PARAMS.h`,
  near-identical shapes, three independent structs) and `ResolvePropInstanceLayerId`/
  `ResolveDecalInstanceLayerId` (same file, two independent bodies). **NOT** one shared
  `Groups: [{id, name, parentGroupId, domain}]` table — a mixed-domain table would need runtime
  domain-filtering to reconstruct a domain-scoped tree, breaks the "array order is the layer's
  identity" convention `PropGroups`/`DecalGroups`/`MarkerGroups` already rely on, and a single
  `markerType`-equivalent field can't mean the same thing across domains (Marker's type is
  `MarkerInstanceGroup::name`; Prop's analogous concept, whatever it becomes, is a different field
  space) without a union/variant this codebase doesn't use for PARAMS elsewhere.
- **Pure container/graph/UI mechanics with zero domain-field access** (tree render, expand/collapse,
  drag-to-reparent, cycle-detection over bare id/parent-id pairs) → **one shared C++
  template / accessor-callback-parameterized function**. Mirrors `DraggableList<T>::Render` and
  `ApplyDraggableListSignal<T>` (`DraggableListWidget_UI.h`) — **already** a template in this
  codebase, proving genericity-via-callback is an accepted pattern here, not a new idea.

So: Markers gets its own `MarkerLayerGroup` struct and its own hand-written resolver functions
(below) — Props/Decals get their own twins later, independently written, not templated. The tree
**widget** and the **cycle-check** predicate are shared, parameterized by accessor lambdas
(`int IdOf(const T&)`, `int ParentIdOf(const T&)`, `const std::string& NameOf(const T&)`) rather than
by field-name coupling — this also means Group's fields don't need to be spelled identically to
`Params::Assembly`'s (`parentGroupIdentifier` vs. `parentIdentifier` can both stay their own most
natural spelling; genericity comes from the lambda, not from forcing matching field names).

❓ ARCH: ratify this domain-touching-vs-pure-mechanics split as the general rule for future
Props/Decals/NavMesh Group work too (not just this ticket) — it resolves the brief's "which shape"
question generally, not just for Markers.

## 2. The Group container's PARAMS shape (Markers)

```cpp
// MarkerLayerGroup_PARAMS.h — NEW FILE (recommended), sibling of MarkerRule_PARAMS.h/
// MarkerInstance_PARAMS.h — parent of BOTH, so it shouldn't live inside either. Naming pending §0.
struct MarkerLayerGroup {
    int identifier            = -1;   // stable, survives reorder/delete — mirrors Params::Assembly::identifier
    std::string name;
    int parentGroupIdentifier = -1;   // -1 = root; enables Group-in-Group nesting (confirmed in scope,
                                       // "hierarchical" per the brief's worked framing)
    std::string markerTypeName;       // single-type scope — same free-form string space as
                                       // MarkerInstanceGroup::name (e.g. "Alloy"), NOT MarkerCategory
                                       // (that enum is explicitly not what type-scoping means today,
                                       // per MarkerInstance_PARAMS.h's own comment)
    int assemblyIdentifier    = -1;   // §3 below — Assembly-references-Group hook
};
```
`MapRecipe` gains `std::vector<MarkerLayerGroup> markerLayerGroups;`.

Back-reference (Layer → Group), additive, on the existing types — **not** a new stable id on the
Layer itself, just a pointer outward:
```cpp
// MarkerRuleLayer gains:      int parentGroupIdentifier = -1;
// MarkerInstanceLayer gains:  int parentGroupIdentifier = -1;
```
Confirms the Format Expert's flagged asymmetry ("`MarkerRuleLayer` has no stable id today") is a
non-issue: this field doesn't need one, it only needs to point outward at `MarkerLayerGroup::identifier`.

**Type-scoping enforcement — ruled soft, not hard.** Nothing today structurally validates that a
`MarkerInstanceLayer`'s actual transforms all belong to the Group's declared `markerTypeName`
(`layerIndex` is a bare untagged int, same as today). Recommendation: enforce at the UI/authoring
flow only (the "Add Marker" action inside a Group-scoped Layer creates instances in the matching
`MarkerInstanceGroup`), not as an import-time hard validation rule — consistent with every other
soft/authoring-time-only invariant already in this format (no clamp/reject exists for a mismatched
`layerIndex`→type today either). ❓ ARCH: confirm this posture, or rule a hard validation is wanted.

**Pure resolvers needed (PARAMS-level, hand-written per domain per §1):**
```cpp
bool WouldReparentMarkerLayerGroupCreateCycle(int candidateId, int newParentId,
                                              const std::vector<MarkerLayerGroup>& groups);
// walk parentGroupIdentifier up from newParentId; reject if candidateId is on the chain.
// SHARED shape via template/callback (§1) with WouldReparentCreateCycle (DESIGN_Assembly_R1 §1) —
// both are pure id/parent-id graph math, zero domain fields touched.

void CollectMarkerLayerGroupRecursiveLayerIndices(int groupIdentifier,
    const std::vector<MarkerLayerGroup>& groups,
    const std::vector<MarkerRuleLayer>& ruleLayers, const std::vector<MarkerInstanceLayer>& instanceLayers,
    std::vector<int>& outRuleLayerIndices, std::vector<int>& outInstanceLayerIndices);
// walks groupIdentifier + descendant groups, collects every Layer (both kinds) whose
// parentGroupIdentifier resolves into that set. Feeds the tree widget's leaf-enumeration callback.

std::vector<std::pair<int,int>> CollectMarkerLayerGroupRecursiveManualMembers(int groupIdentifier,
    const std::vector<MarkerLayerGroup>& groups, const std::vector<MarkerInstanceLayer>& instanceLayers,
    const std::vector<MarkerInstanceGroup>& markers);
// {markerInstanceGroupIndex, transformIndex} pairs — MANUAL ONLY (see §5's exclusion ruling).
```

**Deletion/import posture — restated, not re-derived**: promote-don't-cascade (child Groups AND
child Layers move to the deleted Group's own parent, never destroyed), identical to Assembly's
already-ratified ruling. Cycle-on-import: log, treat as root, loud non-fatal — same convention.

## 3. Assembly-references-Group, live-updating — ruled, diverges from the brief's own strawman

The brief's suggested shape (`{domain, groupIdentifier}` forward-reference list living on Assembly)
**directly contradicts Assembly's own already-ratified rule**: `Params::Assembly` carries no members
list at all — "Rejected a forward-reference/`members: []` shape... duplicates membership truth in
two places that can go out of sync" (`DESIGN_Assembly_R1.md`, restating `BRIEF_Assembly_R1.md`'s
ground truth). This brief's new requirement doesn't need to reopen that ruling — it needs the SAME
backward-tag pattern applied one tier higher.

**Ruling: `MarkerLayerGroup` gets its own scalar `assemblyIdentifier` field** (already shown in §2),
the exact same shape every leaf transform (`MarkerTransform`/`PropTransform`/`DecalTransform`)
already carries per `DESIGN_Assembly_R1.md` §5. A Group can belong to at most one Assembly — same
no-multi-membership rule, just one tier up. **No new reference-shape type is needed on `Assembly`
itself; `Assembly`'s own `{identifier, name, parentIdentifier}` record stays exactly as designed.**

**This gives "live" for free, confirmed** — the brief's own suspicion is correct, but only under this
backward-tag shape, not the forward-list one: `CollectAssemblyRecursiveMembership` (flagged for a
PARAMS home in `DESIGN_Assembly_R1.md` §6) must be **extended** to also scan each domain's Group
table for `assemblyIdentifier` matches, and for every match, recursively fold in that Group's
resolved Layer→member set via `CollectMarkerLayerGroupRecursiveManualMembers` (§2). Because this is a
query-time walk over live PARAMS state (not a cached/snapshotted list), a marker added to a
Group-tagged Layer tomorrow is included the next time the query runs — no extra machinery. Confirmed
as designed, not assumed.

**New sub-rule this recursion forces, not in either prior document — ruled here:** if a nested child
Group has its OWN `assemblyIdentifier` set to a *different* Assembly than its ancestor, **the walk
stops at that child** — it belongs to the other Assembly, full stop (the same no-multi-membership
invariant, checked at every tier, not only at the leaf). An untagged (`-1`) child Group is walked
through transitively as part of its tagged ancestor.

**Domain discrimination**, per this shape, is simply: each domain's Group table is its own typed
array (`markerLayerGroups` today; `propLayerGroups`/`decalLayerGroups` later) — no `GroupDomain`
enum or discriminated-union reference type is needed anywhere, because there's no forward reference
to discriminate.

❓ ARCH/Format/Generator Expert: ratify this whole reframing — it changes what the brief literally
asked for (a `{domain, groupIdentifier}` reference shape) into "one new scalar field on the Group
type + an extension to an already-flagged resolver function." State explicitly whether this is
accepted as the correct resolution of the brief's requirement.

## 4. The Markers tab UI

**Top-level structure** (replaces today's five flat siblings):
```
Markers (Section, unchanged)
  Globals (unchanged — map-wide, not a Group/Layer concept)
  [Group tree — NEW]                         ← TreeListWidget_UI<MarkerLayerGroup>
  Ungrouped Procedural Rules                 ← existing DrawMarkerRuleLayerList, filtered to
                                                parentGroupIdentifier == -1
  Ungrouped Manual Marker Layers             ← existing DrawManualMarkerLayers, same filter
  Manual Markers roster (unchanged)          ← still edits recipe.markers directly, orthogonal to Group tier
  Placed Markers (unchanged, read-only)
```
**Ruling on the brief's "Groups → Layers → Procedural/Manual sub-sections" phrasing**: this is
satisfied by (a) each Layer *node* under a Group carrying an inline Manual/Procedural badge (matching
the human's own verbatim worked example, which tags each Layer inline, not via two sub-headers), and
(b) the residual **ungrouped** layers needing two separate sections because `MarkerRuleLayer` and
`MarkerInstanceLayer` are different C++ types requiring different `DraggableList<T>` instantiations —
not a literal fourth tree tier under every Group node. Flagged as my interpretation, not
self-evidently the only reading — cheap to confirm with the human before dispatch.

**Same widget as Assembly's tree — confirmed, with one additive extension.** `TreeListWidget_UI<T>`
as sketched in `DESIGN_Assembly_R1.md` §1 is Assembly-specific as written (field names like
`sourceAssemblyId`); genericize it per §1 above into `TreeListWidget_UI<T>` templated over the
GROUP-node type (`Params::Assembly` for Assembly, `MarkerLayerGroup` here), accessed via accessor
lambdas, leaves addressed via a caller-defined opaque key (Assembly already has
`AssemblyMemberKey_UI`; this brief needs its own `MarkerGroupLeafKey_UI = {kind: Procedural|Manual,
layerIndex}`, mirroring the pattern, not reusing Assembly's key type since the leaf payload differs).

**Additive extension needed, not in Assembly's original sketch**: Assembly's leaf rows are read-only
(select/drag only — their real editing UI lives in their owning tab). This brief's Layer *nodes* are
NOT read-only references — they need their full existing settings body inline
(`DrawRuleLayerSettings`, `DrawLayerRowBody`). **Good news, confirmed by direct read**: both functions
are already factored as per-row, non-"selected"-gated (STEP110's refactor: "drawn inline per row, not
'selected'-gated") — they can be called directly as the leaf-body callback with zero rewrite. The
widget contract needs one small addition: an optional `DrawExpandedLeafBody(leafKey)` callback,
no-op/unused for Assembly's leaves, wired for Group's Layer leaves. Whichever of {Assembly, Group}
ships its coder-dispatchable ticket first should build `TreeListWidget_UI<T>` WITH this extension so
the second consumer needs no follow-up widget change. ❓ ARCH: confirm sequencing/ownership of who
builds the shared widget first.

**Toolbar**: "Add Group" (name + `markerTypeName` picker/text), inline rename, "Delete" (promote, per
§2), "Add Manual/Procedural Layer" both at root (existing buttons, unchanged, mint
`parentGroupIdentifier = -1`) and inside an expanded Group's own body (mints with that Group's
`identifier`) — Move/Rotate controls per §5.

## 5. Group-select + move/rotate mechanics

**Membership — Manual layers only, ruled, restating Assembly's own §0 ruling one tier up.**
Procedural layers have no stable per-instance identity (`Data::PlacementInstances` is
regenerated every bake, ARCH §14.8) — the exact reasoning `DESIGN_Assembly_R1.md` §0 already used to
scope `AssemblyId` to manual instances only. A Group containing a Procedural Layer shows it in the
tree (organizational membership) but that Layer contributes **zero** members to a move/rotate or to
an Assembly's resolved membership via that Group. ⚠️ Worth flagging loudly since the brief's own
worked example includes a Procedural layer as a Group member and item 5 doesn't explicitly carve this
out — I'm ruling it here as the necessary, consistent restatement, not inventing a new constraint.

**Full recursive membership** = `CollectMarkerLayerGroupRecursiveManualMembers` (§2): walk descendant
Groups, collect in-scope `MarkerInstanceLayer` indices, scan `recipe.markers` for transforms whose
`layerIndex` matches.

**Scope ruling for v1: tab-driven only, no new canvas gesture.** Assembly's own canvas multi-select
machinery (`CrossLayerSelectionState`, ctrl-click/shift-drag/marquee, `DESIGN_Assembly_R1.md` §2) is
itself unbuilt and unratified. Building a SECOND, independently-designed canvas gesture layer for
Group-drag before Assembly's ships would fork two similar-but-different interaction models in
`MapCanvas_Draw_UI.cpp` — a real design smell, and directly against this brief's own "delivery
scoping" instinct to keep the first ticket narrow. **Ruling: Move = X/Z offset fields + "Apply Move"
button; Rotate = degrees field + "Apply Rotation" button**, both operating on the Group's resolved
Manual membership, computed once at Apply-time (not a live gesture, so no "gesture-start" framing
needed). This mirrors Assembly's own §3 tab controls almost exactly and should share the same
underlying rigid-transform-around-centroid MATH function once ARCH places it (§6/§7 below) — not two
copies. Canvas live-drag-of-a-Group is explicitly deferred to a later round, ideally unified with
Assembly's canvas selection once THAT ships, not designed independently here.

**Pivot**: centroid of the resolved (Manual-only) member set — Move is pivot-independent
(translation), Rotate uses the centroid, computed fresh at each Apply click. Same interaction-design
call Assembly's design already made for the identical reason (no natural single-anchor candidate for
a possibly-many-member set); restated, not re-derived, kept consistent between the two features
deliberately.

**STEP94 interaction — ruled, mirrors Assembly's own deferred hazard.** Since v1 has no live canvas
group-drag, the only interaction surface is: does "Apply Move/Rotate" on a Group silently also move a
member's symmetric sibling that's OUTSIDE the Group? **Ruled: no, deferred, same as Assembly's own
§4 ⚠️.** v1 applies the flat rigid delta to exactly the resolved member set and nothing else; a
sibling outside the Group is left untouched, full stop, no composition attempted. The eventual design
(if/when built) should be the SAME precedence rule `DESIGN_Assembly_R1.md` §4 already proposed
(co-selected symmetric pairs suppress follow; non-co-selected members keep independent orbit-follow)
— recommend Generator Expert resolve this ONCE for both Assembly and Group rather than being asked
twice for what is structurally the identical question.

## 6. Delivery scoping

**Ticket A — Markers-only PARAMS + IO** (first, unblocks everything else):
`MarkerLayerGroup` (new file), `MapRecipe::markerLayerGroups`, `parentGroupIdentifier` on
`MarkerRuleLayer`/`MarkerInstanceLayer`, `assemblyIdentifier` on `MarkerLayerGroup` (inert until
Assembly exists — cheap to add now, avoids a second migration pass), the three resolver functions
(§2), new wire section (name pending §0), additive/no-version-bump.

**Ticket B — Markers-only UI** (depends on A, and on whichever ticket builds `TreeListWidget_UI<T>`
first — §4): the tab restructure, Group toolbar, tab-driven Move/Rotate. Reuses
`DrawRuleLayerSettings`/`DrawLayerRowBody` unchanged (§4's "good news" finding) — minimizes blast
radius, most of the settings-panel work is already correctly factored from STEP110.

**NOT in Ticket A/B, separate/later:**
- `CollectAssemblyRecursiveMembership`'s Group-table-walking extension (§3) — depends on BOTH
  Assembly's own ticket AND at least one domain's Group ticket existing; sequenced after both.
- Props/Decals' own `PropLayerGroup`/`DecalLayerGroup` twins — same generic foundation, independently
  ticketed later, per the brief's own sequencing.
- NavMesh — explicitly out of scope, future session; nothing in this design forecloses it (a
  movement-type-per-Layer set fits the same `parentGroupIdentifier` back-reference shape whenever
  that domain exists).
- Canvas live-drag-of-a-Group — deferred per §5.
- ⚠️ **Separate bug ticket** (already flagged in the brief, restated, not fixed here): manual marker
  exporter never writes `layerIndex` (`BuildMarkerTransformJson`, `MapExporter_Markers_IO.cpp:17-39`)
  — this restructure's new `parentGroupIdentifier`/Group wire fields must not silently inherit the
  same omission; whoever builds Ticket A's IO half should check the exporter writes ALL of
  `layerIndex`/`symmetryGroupIdentifier`/the new fields, and file the pre-existing `layerIndex` bug
  separately if not already ticketed.

## 7. Flagged for ARCH — full ratification list

1. Final type/wire name for the new Group concept (§0) — "Bundle"/"Ensemble"/"Formation" candidates,
   `Cluster` ruled out (real collision with `ClusterByScreenCell`, UI Optimization Expert territory).
   Confirm the UI-label-vs-type-name split is acceptable.
2. The domain-touching-vs-pure-mechanics genericity split (§1) as the general rule for all future
   per-domain Group work, not just Markers.
3. Field spellings for `MarkerLayerGroup` (`identifier`/`parentGroupIdentifier`/`markerTypeName`/
   `assemblyIdentifier`) — flag the same "Id" abbreviation-law question already open for
   `AssemblyId`/`layerId` (ARCH §16.5); confirm accessor-lambda genericity means field names do NOT
   need to match `Params::Assembly`'s own spelling.
4. New top-level wire key/shape for `markerLayerGroups`, jointly with Format Expert — PascalCase
   per §1.6, additive/no-version-bump, jointly with IO Architecture Expert.
5. §3's whole reframing of "Assembly-references-Group" — a scalar `assemblyIdentifier` on the Group
   type plus an extension to `CollectAssemblyRecursiveMembership`, instead of the brief's own
   suggested `{domain, groupIdentifier}` forward-reference list. Explicit ratification needed since
   this changes what the brief asked for.
6. The child-Group-with-its-own-`assemblyIdentifier` "cuts off the walk" sub-rule (§3) — new, not in
   either prior document, forced by this brief's recursion.
7. `TreeListWidget_UI<T>`'s additive optional-leaf-body-callback extension (§4) and which ticket
   (Assembly's or Group's) builds the shared widget first.
8. Module-boundary placement (PARAMS vs. MATH) of the pure resolver functions (§2) and the rigid
   rotate-around-centroid math (§5) — this is the **third** round to raise the identical
   PROC/MATH/PARAMS placement question (`BuildSymmetryOrbit` in `DESIGN_MarkerLayerSymmetry_R1.md`
   §4 item 3; rigid-rotate math in `DESIGN_Assembly_R1.md` §4/§6; now again here) — recommend ARCH
   settle this class of question once, generally, rather than per-feature.
9. Confirm Group's rigid-transform math should be the SAME shared MATH function Assembly's design
   already flagged (§5), not a second copy.
10. Confirm §5's procedural-layer-exclusion restatement is consistent with Assembly's §0 ruling, not
    an independently-drifting rule.
11. Confirm §5's tab-driven-only v1 scoping (no new canvas gesture) as the right sequencing call.
12. `SANMAP_FORMAT_SPEC.md` staleness already flagged in the brief (missing `MarkerGroups` fields;
    `layerId`→`layerIdentifier` abbreviation follow-up) — now also touches this new Group type if
    `identifier` naming is chosen; bundle into whatever cleanup pass ARCH already has pending.
13. §2's "soft, UI-enforced-only" posture for Group→type-scope consistency — confirm or rule a hard
    import-time validation is wanted instead.

## Who else this touches
- **Format Expert**: §2/§4 wire shape and casing, jointly with ARCH's naming ruling (item 3/4 above).
- **IO Architecture Expert**: additive/no-bump confirmation for the new section + fields (expected
  yes, same precedent class as every prior addition this session).
- **Generator Expert**: the symmetry-follow-on-group-move precedence rule (§5) — recommend answering
  once for both Assembly and Group, not twice.
- **UI Optimization Expert**: not consulted — authoring-scale only (tens of Groups/Layers); flagged
  `Cluster` name collision above is the only overlap with their territory, avoided by not reusing the
  name.

No coder-dispatchable ticket from this document alone — needs the ARCH pass above first.

---

Files read to ground this design (absolute paths under `D:\Projects\Sanctuary\Map Generator\`):
`work_orders\BRIEF_MarkerGroupLayerRestructure_R1.md`, `work_orders\BRIEF_Assembly_R1.md`,
`work_orders\DESIGN_Assembly_R1.md`, `work_orders\DESIGN_MarkerLayerSymmetry_R1.md`,
`sangen_arch_pack\specs\SANMAP_FORMAT_SPEC.md` (Corrections 7/11/12/14/15/16 region),
`src\params\MarkerRule_PARAMS.h`, `src\params\MarkerInstance_PARAMS.h`,
`src\params\GlobalMarkerSettings_PARAMS.h`, `src\params\PropInstance_PARAMS.h`,
`src\params\MapRecipe_PARAMS.h` (grep), `src\ui\MarkersTab_UI.cpp`, `src\ui\MarkersTab_RuleLayers_UI.h`,
`src\ui\MarkersTab_ManualLayers_UI.cpp`, `src\ui\MarkersTab_ManualLayers_UI.h`,
`src\ui\MarkersTab_Manual_UI.h`, `src\ui\MarkerLayerIndexRepair_UI.h`, `src\ui\MarkerLayerId_UI.h`,
`src\ui\MarkerDragGesture_UI.h`, `src\ui\DraggableListWidget_UI.h`.

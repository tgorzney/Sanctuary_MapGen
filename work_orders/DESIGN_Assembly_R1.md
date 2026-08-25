# Design Output — Cross-Layer Assembly Grouping, Round 1

Continuation of `BRIEF_Assembly_R1.md` (read that first — its "already confirmed" section is
ground truth, not re-derived here). Design phase only, no code. Not yet ratified — no
coder-dispatchable ticket until ARCH (+ Format/Generator/IO Architecture Experts as flagged) rule
on §7's items. Structured to answer the brief's six numbered asks in order, plus an explicit
scoping ruling (§0) the brief asked to be settled outright.

## 0. Scoping ruling — manual-instances-only (RULED, not left open)

**`AssemblyId` membership is manual-instances-only.** `PropTransform`/`DecalTransform`/
`MarkerTransform` are hand-authored, IO-round-tripped, and already have a stable per-instance
address (array position inside a stable-order vector, wrapped in a named struct — the exact shape
`layerIndex` already tags). `Data::PlacementInstances` is PROC-regenerated on every bake tier-A/B
recompute (`ARCH_14_08_DirtyFlagTiers.md`) with **no cross-bake stable identity** — nothing to hang
a persisted `AssemblyId` on without inventing a new PROC-owned deterministic per-instance id system
first (a Generator-Expert-owned feature this brief never asks for). This also matches ARCH §3.2 (UI
owns no sim logic): an Assembly is an authored, persisted human decision, not something a live
regeneration pass can safely re-derive. Confirms the brief's working assumption; not open.
Reclaim is not a fourth domain — it is `PropTransform` entries with `group.bReclaimable == true`
(`MapCanvas_IconLayer_CullManual_UI.cpp`'s `ResolvePropsManual`), already covered by "Props."

## 1. The tree widget — `TreeListWidget_UI.h`

**No existing widget generalizes.** Confirmed by direct read: `DraggableListWidget_UI.h` is
strictly flat (one `std::vector<T>`, index-based reorder). `LayerEditor_Group_UI.cpp`'s
GeoLayer→Layer and `ArmiesTab_Units_UI.cpp`'s Army→Units are both two hardcoded levels via caller
composition (an outer `DraggableList` drawing an inner `DraggableList` only for the selected outer
row) — neither expresses arbitrary Assembly-in-Assembly depth. New primitive, as the brief already
ruled.

**Data shape passed in — flat, not pre-nested.** `recipe.assemblies` really is
`std::vector<Params::Assembly>` with a `parentIdentifier` back-pointer (§5) — the same shape
`Params::Layer::bLocked`-style flat-with-tag data already takes elsewhere in this codebase. The
widget takes that flat list plus a caller-supplied leaf-enumeration callback, and rebuilds its own
id→children index **once per `Render` call** (O(assemblyCount), authoring-scale — tens, not
100k — same "not virtualized on purpose" posture `DraggableListWidget_UI.h`'s own header states for
itself; `VirtualListWidget_UI.h` is for the 100k-entity case, not this one):

```cpp
// TreeListWidget_UI.h
enum class TreeListSignalKind : int { None = 0, Select, Reparent, Delete };
enum class TreeDropZone : int { Above, Below, OnAsChild };
enum class TreeNodeSourceKind : int { Assembly, Leaf };

struct TreeListSignal {
    TreeListSignalKind kind = TreeListSignalKind::None;
    TreeNodeSourceKind sourceKind = TreeNodeSourceKind::Assembly;
    int  sourceAssemblyId = -1;         // valid when sourceKind == Assembly
    AssemblyMemberKey_UI sourceLeaf;    // valid when sourceKind == Leaf (§2)
    int  targetAssemblyId = -1;         // -1 = the root/"Ungrouped" drop zone
    TreeDropZone dropZone = TreeDropZone::OnAsChild;   // meaningless for a Leaf source (see below)
    bool bHasSignal() const { return kind != TreeListSignalKind::None; }
};

// Caller-owned, never a function static (Section_UI.h's rule) — a stable per-node-id map, NOT
// index-keyed, because reparenting changes a node's position in the flattened draw order every
// frame but never its identity (Params::Assembly::identifier / AssemblyMemberKey_UI equality).
struct TreeListState {
    std::unordered_map<int, bool> expandedAssemblyIds;   // default false on first sight
    // ...drag-in-progress bookkeeping, mirrors DraggableList's own internal payload handling
};
```

**Deliberate deviation from `DraggableList`'s "mutates nothing" purity — justified.**
Expand/collapse is presentation-only, zero PARAMS consequence — exactly `Section_UI.h`'s own
justification for owning it directly (`StepSectionHeader` mutates `SectionState&` in place, no
signal). `TreeListWidget_UI::Render` follows `Section_UI`, not `DraggableList`, for this one
piece: it takes `TreeListState&` and toggles `expandedAssemblyIds` directly on an arrow click. It
still returns a `TreeListSignal` for everything that DOES touch PARAMS (Reparent/Delete/Select),
following `DraggableList`'s contract there unchanged. Two different established precedents,
applied to the two different kinds of state a tree row actually carries — not an inconsistency.

**Reparent drop zones.** Three zones per row, a named tweakable fraction
(`kTreeDropZoneEdgeFraction`, default 0.25 — top/bottom 25% of the row rect = Above/Below, middle
50% = OnAsChild; Constitution §8), mirroring the VSCode/Unity-outliner convention the brief asked
for. Meaning depends on source kind:
- **Assembly source, Above/Below** → reparent to the SAME parent as the target (sibling), and
  physically move the dragged entry adjacent to the target inside `recipe.assemblies` (a real
  array-position move, generalizing `ApplyDraggableListSignal`'s vector-splice for a tree — sibling
  ORDER is stored as array order, following the established SanGen "array order is the layer's
  identity" convention already used for `PropGroups`/`DecalGroups`/`MarkerGroups`, not a separate
  order field).
- **Assembly source, OnAsChild** → reparent under the target Assembly (`parentIdentifier = target`).
- **Leaf source, any zone** → always the same outcome: `assemblyIdentifier = target` (or `-1` on the
  root drop zone). Leaves carry no stored order among Assembly siblings (nothing in the ruled data
  model gives one), so the three-zone distinction is a no-op for a Leaf drag — the widget still
  detects and reports whichever zone geometrically matched (keeps the widget itself domain-agnostic
  about what a signal MEANS), the caller's apply step just doesn't branch on it for this source kind.
- Leaf rows never draw an OnAsChild zone target FOR THEMSELVES as a destination (a leaf cannot have
  children) — enforced structurally by the row descriptor (`bCanHaveChildren = false` for a leaf),
  not caught after the fact, per Constitution §6 posture.

**Cycle prevention is a PARAMS-level pure predicate, not the widget's own logic** (mirrors the
`Params::ResolvePropInstanceLayerId` precedent, `PropInstance_PARAMS.h:37-44`, "a shared,
PARAMS-level pure resolver used by both a PROC/IO pass and a live UI read path"):

```cpp
// proposed — needs a PARAMS home, see §6
bool WouldReparentCreateCycle(int candidateId, int newParentId,
                              const std::vector<Params::Assembly>& assemblies);
```

Used by (a) the widget's caller before applying a Reparent signal (refuse it, leave `recipe.assemblies`
untouched — same "reject, don't corrupt" posture already used everywhere else in this format) and
(b) import-time cycle recovery (walk, log, treat as root — the brief's already-decided loud-non-fatal
convention). One function, two callers, no drift — the exact shape the brief pointed at.

**Leaf rows in the tree** show domain icon/kind + template identifier + owning manual-layer name,
read-only except drag-to-reparent and click-to-select (which also feeds the shared selection state,
§2). They are enumerated per-Assembly via a caller-supplied callback the widget never dereferences
into `recipe.markers`/`props`/`decals` itself — keeps the widget domain-agnostic. The caller (the
Assemblies tab) should build one `assemblyId → vector<AssemblyMemberKey_UI>` index **once per
frame** (same per-frame-cache posture `MapCanvas`'s own `overlayIconLayerFrameCache` already uses),
not re-scan `recipe.markers`/`props`/`decals` once per tree node per frame.

## 2. Cross-layer multi-select mechanism

**New key type needed before anything else — a real gap, found while reading, not invented.**
`OverlayInstanceKey_UI` (`MapCanvas_IconLayer_UI.h:24-32`) is `{collection, instanceIndex}` — a
single flat index. For Props/Decals that index is **local to one `PropInstanceGroup`/
`DecalInstanceGroup`'s `transforms` vector** (`MapCanvas_IconLayer_CullManual_UI.cpp`'s
`ResolvePropsManual`/`ResolveDecalsManual` both loop `group` outer, `index` inner, per-group), not
globally unique across the recipe's groups — and Markers are addressed the identical two-tier way
today (`MarkerDragGestureState::groupIndex` + `draggedTransformIndex`,
`MarkersTab_Manual_UI.h`'s `selectedGroupIndex`/`selectedInstanceIndex`). Confirms
`OverlayInstanceKey_UI` is already an incomplete simplification for all three collections, not just
an Assembly-specific gap — and today's selection-highlight even gates itself to Markers only
(`MapCanvas_IconLayer_Cull_UI.cpp:112`, `if (...collection != Markers) return;`). Rather than widen
an already-pervasive type (bigger blast radius than needed), propose a new, purpose-scoped type:

```cpp
// AssemblySelection_UI.h — new file
struct AssemblyMemberKey_UI {
    PlacementCollectionKind_UI collection = PlacementCollectionKind_UI::Markers;  // Markers/Props/Decals only, this domain
    int groupIndex     = -1;   // recipe.markers[g] / recipe.props[g] / recipe.decals[g]
    int transformIndex = -1;   // that group's .transforms[t]
};

struct CrossLayerSelectionState {
    std::vector<AssemblyMemberKey_UI> members;
    int boundAssemblyId = -1;   // -1 = ad-hoc selection; >=0 = mirrors that Assembly's full resolved membership
};
```

**Unifies two use cases into one state, deliberately** — the ad-hoc "build a new Assembly from
scratch" selection AND "I clicked an existing Assembly, here's its resolved membership, ready to
move" are the SAME underlying concept (a live set of leaf keys), not two parallel mechanisms. Any
manual ctrl-click/marquee edit always drops `boundAssemblyId` to `-1` (an edited selection is never
silently written back onto an existing Assembly's identity — "Create Assembly from Selection"
always mints a new one, never mutates an existing one through incidental editing).

**Where it lives — ❓ ARCH Expert, not guessed here.** `Application_TabState_UI.h`'s own header
states its "one instance each" rule exists specifically because "two tabs sharing a selection or a
drag is the v1 function-static defect the widget library exists to kill." This is the first
genuinely-wanted exception — several draw sites (MarkersTab, PropsTab [which also hosts Decals,
`PropsTab_UI.h:22-23` — there is no separate DecalsTab], MapCanvas, the new AssembliesTab)
DELIBERATELY need the SAME live selection, not independently-evolved same-shaped state that
accidentally collides. Recommendation (not self-ratified): do not add it inside
`ApplicationTabState`'s per-tab bag at all — give it its OWN sibling home on the shell, injected by
pointer into each consumer the same way `MapCanvas` already receives `SetOverlayRecipe`/
`SetManualMarkerDragSource` (`MapCanvas_UI.h:80,97-105`) rather than reached back through
Application. **Exact ruling needed: does this extend the "one instance each" rule (a deliberate,
named exception) or does it need a wholly different architectural concept? Where precisely does it
live?**

**Gating "visible + unlocked" — a real gap, not fully buildable as literally stated.**
"Visible" has a home: the View toolbar's `OverlaySubLayerRef_UI::bEnabled`
(`OverlayLayer_Settings_UI.h:27`, §14.7). **"Locked" does not exist for manual entity layers at
all today** — `PropInstanceLayer`/`DecalInstanceLayer`/`MarkerInstanceLayer` (`PropInstance_PARAMS.h:30-31`)
carry only `name`/`color`/`iconScale`/`layerId`, no lock flag; only `Params::Layer` (the GEO/terrain
stack, `Layer_PARAMS.h:18`) and `MarkerRuleLayer` (procedural rules) carry `bLocked`/`bHidden`.
Even the already-shipped `HitTestManualMarkers` (`MapCanvas_MarkerDrag_UI.cpp:33-65`) scans every
group/transform unconditionally today — no visibility/lock gate exists in the one hit-test that
DOES exist yet either. **Recommendation for this round: gate on "visible" only (real, wireable);
drop "locked" from v1's gating criterion rather than inventing a new PARAMS field speculatively —
flag a `bLocked` field on the three manual-layer types as the natural extension point if/when the
human wants real per-layer locking.** Confirm with the human before building either way.

**Canvas gesture, disambiguated against `MapCanvas_Draw_UI.cpp`'s existing branching**
(`ApplyPointerInput`, `:101-150`). Current order: wheel→zoom; ScenarioEditMode exclusive early-out;
press-activated tries `TryBeginManualMarkerDrag` first; else travel-based pan/click. New order,
inserted between the ScenarioEditMode early-out and `TryBeginManualMarkerDrag`:

1. **Ctrl+click** (press-activated, `io.KeyCtrl`, no `io.KeyShift`, hits a gated-selectable
   instance) → toggle that exact `AssemblyMemberKey_UI` in/out of `selection.members`;
   `boundAssemblyId = -1`. Fully consumed at press-time — no pan, no drag, no fallthrough.
2. **Shift+drag** (press-activated with `io.KeyShift`) → begin a marquee gesture (origin +
   live-updated rect, screen-space only — ARCH §14.8 tier-C redraw, zero GPU recompute). On
   release: every gated-selectable instance whose world position projects inside the rect is
   added (plain Shift) or additionally-merged (Shift+Ctrl held together) into `selection.members`;
   `boundAssemblyId = -1`. A plain Shift+drag with nothing already selected behaves like "replace."
3. **Plain press on an already-multi-selected member, `selection.members.size() > 1`** → begin
   the GROUP move gesture (§4), not the single-instance path.
4. Else — unchanged: `TryBeginManualMarkerDrag` (existing single-marker symmetry-follow), then the
   existing pan/click fallback.

**Hit-testing across all three collections.** `HitTestManualMarkers`'s existing shape (O(N)
screen-space radius scan over `recipe.markers`, `MapCanvas_MarkerDrag_UI.cpp:33-65`) is the
precedent to generalize, not reinvent — a sibling `HitTestManualInstances` scanning
`recipe.markers`/`props`/`decals` and returning an `AssemblyMemberKey_UI`, same tie-break
convention (first/lowest group-then-transform wins within radius). Props/Decals have **no**
hit-test consumer of any kind today (STEP94 built this only for Markers) — this is real new
surface for the coder, not purely wiring existing machinery.

**Existing-selection click rule (already decided by the brief, restated for placement in this
flow).** A plain click (no modifier) on a grouped instance selects the **root-most ancestor
Assembly** — resolve via a `ResolveAssemblyRootAncestor(assemblyId, assemblies)` walk (needs a
PARAMS home, §6), then set `selection = {full recursive membership of that root}`,
`boundAssemblyId = root`. Double-click drills in one level: resolve which immediate child (of the
currently-bound Assembly) is the ancestor-or-self of the double-clicked instance; if that child is
itself an Assembly, `boundAssemblyId` becomes its id and `selection` its own resolved membership;
if it's a bare leaf, `selection = {that one leaf}`, `boundAssemblyId = -1`.

**Staleness — the selection set is index-based and held across frames, unlike a single-frame
pick; it needs the same self-healing this codebase already uses elsewhere.** Deleting/inserting
into any of `recipe.markers[g].transforms`/`props[g].transforms`/`decals[g].transforms` shifts
every later `transformIndex` — an `AssemblyMemberKey_UI` held in `selection.members` across that
mutation can silently point at the wrong instance. Precedent to generalize:
`ResolvedMarkerGroupSelection`/`ResolvedMarkerInstanceSelection` (`MarkersTab_Manual_UI.h:79-93`,
clamp-on-shrink). Recommend: any structural mutation to one of the three transform vectors
re-validates every `selection.members` entry against current bounds, dropping (not clamping) an
entry that no longer resolves — dropping is correct here (unlike a clamp-to-last-valid, which would
silently re-target a DIFFERENT instance into the selection).

## 3. The Assemblies tab

New `ApplicationPanel::Assemblies` entry (`Application_Panels_UI.h:26-30`), Environment group,
same placement logic already used for `ApplicationPanel::Scenarios` — "spans marker/prop/decal
domains, doesn't belong nested in any one of them." No preview-visibility toggle
(`PreviewVisibilityTarget::None`), same reasoning as Scenarios' own row comment — `recipe.assemblies`
feeds no PROC stage and drives no composite field layer.

**Body**: `TreeListWidget_UI::Render` over `recipe.assemblies` + per-node leaf enumeration (§1).
Toolbar: "Create Assembly from Selection" (enabled when `selection.members.size() >= 1` — a
single-member "group" is degenerate but nothing in the ruled data model forbids it), "Ungroup"
(applies the brief's already-decided promote-don't-cascade delete to the tree-selected Assembly),
inline rename on the expanded node body (a plain live-bound `DrawTextInput` against
`assembly.name`, no signal needed — same posture `DrawGroupSettings`'s GeoLayer rename already
uses, `LayerEditor_Group_UI.cpp:24`). A small live readout ("3 markers, 1 prop, 2 decals selected")
mirrors the brief's own example composition.

**"Create Assembly from Selection"**: always mints a NEW `Params::Assembly` (never repurposes an
existing one); default `parentIdentifier = -1` unless every current `selection.members` entry
already resolves to the same existing parent (a nice-to-have default, not required for
correctness); writes the new id onto every selected leaf's `assemblyIdentifier` (overwriting
whatever it held — a natural, not special-cased, consequence of "no multi-membership"); updates
`selection.boundAssemblyId` to the new id. **Only ever bundles LEAVES this way** — nesting an
EXISTING Assembly under another Assembly is the tree widget's drag-to-reparent job (§1), not this
canvas-selection path (`selection.members` holds only `AssemblyMemberKey_UI` leaves, never
Assembly ids — deliberate scope split, stated explicitly so it isn't ambiguous later).

**Group move/rotate controls** (own the tab-driven half of §4): X/Z offset fields + "Apply Move"
(also the target of the live canvas group-drag, §4) and a degrees field + "Apply Rotation" —
both operate around the centroid pivot, §4.

## 4. Group move/rotate mechanics

**Membership**: for `boundAssemblyId >= 0`, `selection.members` IS already the resolved full
recursive membership (computed once at selection time by `CollectAssemblyRecursiveMembership`,
§6 — walk `recipe.assemblies` down from the bound id, collect every leaf tagged with it or any
descendant). For an ad-hoc selection (`boundAssemblyId == -1`), `selection.members` is already
leaf-level — no recursive walk needed.

**Pivot — ruled: centroid of the full member set's positions, frozen at gesture-start.** Move
(translate) is pivot-independent so this only matters for Rotate. A designated single "anchor"
member (considered, per the brief's framing) has no natural candidate for a cross-domain,
possibly-many-member selection the way a single symmetry group has a seed point — centroid is the
predictable, unsurprising default and needs no extra per-Assembly state to store. This is an
interaction-design call, squarely mine to make (not routed as a question).

**Move — generalizes cleanly from existing machinery, deliberately simpler than the single-marker
case.** A rigid group translate is a flat per-frame world-space delta `(dx, dz)` applied identically
to every member's position — no orbit/correspondence math needed at all (unlike
`MarkerDragGesture_UI.h`'s symmetry-follow, this has no cardinality-change problem: the member SET
is fixed for the gesture's duration). Reuses the ARCH §14.8 C2 tier exactly as named for the
existing single-marker drag: non-selected instances' vertex bytes stay cached at gesture-start;
only the moving selection's overlay vertices regenerate live.

**Rotate — ruled out of live canvas-drag scope this round, tab-driven only.** No existing
single-instance rotate-via-drag gesture exists anywhere to generalize from (`MarkerDragGesture_UI.h`
writes position only, never `rotationY`) — this would be new interaction surface invented from
nothing, not a generalization, and the brief's item 4 doesn't mandate it be a live drag
specifically. Tab-driven (§3's degrees field + Apply button) rotates every member's position around
the frozen centroid AND that member's own facing in place — a rigid-body rotation, standard
`(x',z') = pivot + R(θ)·(pos - pivot)`. This pure math (position-around-pivot plus per-member
orientation update) is a MATH/PARAMS-boundary candidate mirroring `BuildSymmetryOrbit`'s own
PROC-vs-MATH placement question from last round (`DESIGN_MarkerLayerSymmetry_R1.md` §4 item 3) —
flagged for ARCH, §6.

⚠️ **Symmetry-follow composition hazard (brief's flagged item) — deferred, not solved silently.**
"Optional symmetry-following" on a group move is **out of scope for this round** — the brief
explicitly permits deferring it, and building it correctly needs Generator Expert sign-off this
round doesn't have time to get. Seeding the eventual design so that round isn't unguided:

**Proposed precedence rule (not yet Generator-Expert-verified — ❓ below), for whenever
symmetry-following on group-move IS built**: at gesture-start (one-shot, frozen for the gesture's
duration — same posture `MarkerDragGestureState`'s own snapshot fields already use), for every
selected member with `symmetryGroupIdentifier != 0`, check whether that member's own mirror
sibling(s) are ALSO present in the same moved set. If a pair is co-selected, **suppress
symmetry-follow for that pair — both members receive only the flat rigid group-delta, exactly as
if the flag were off for them specifically.** Symmetry-follow stays active (per last round's
per-member-independent-orbit ruling) for any selected member whose sibling is NOT part of the
moved group, composed additively with that member's own share of the group's rigid delta. Decided
once at gesture-start, never re-evaluated live, so it structurally cannot "silently oscillate"
frame-to-frame the way the brief's hazard warning describes.

## 5. The `.sanmap` shape

```cpp
// proposed — names below are a strawman, casing pending ❓ in §7
struct Assembly {
    int identifier       = -1;   // stable, survives array reorder/delete — MUST NOT be positional
                                  // (unlike PropGroups/DecalGroups; matches MarkerGroups' own
                                  // "Id — stable, legacy-backfill by array index on import when
                                  // absent" precedent, SANMAP_FORMAT_SPEC.md line 853, because
                                  // Assemblies form an arbitrary forest, not a flat display stack)
    std::string name;
    int parentIdentifier = -1;   // -1/absent = root
};
// MapRecipe gains: std::vector<Assembly> assemblies;
```

Wire: new top-level `Assemblies` array (PascalCase, SanGen-owned, sibling of `MarkerGroups`/
`PropGroups`/`DecalGroups`, same "unrecognized top-level section parsed then dropped" safety the
brief's ground truth already confirms). Per-instance: one new merged field on
`PropTransform`/`DecalTransform`/`MarkerTransform` (lowerCamelCase, direct field injection — the
identical, now-doubly-production-proven pattern `layerIndex`/`symmetryGroupIdentifier` already use).
**Additive, no `SanGenVersion` bump** — matches Corrections 12/14/16/18's identical precedent (new
field merged into an existing collection, nothing reshaped); not self-ratified, Format/IO
Architecture Expert territory to confirm formally, but nothing here contradicts that precedent.

**Import validation** for the per-instance field: no range to check (mirrors
`symmetryGroupIdentifier`'s own "no range to validate... any positive value accepted as-is" ruling,
SANMAP_FORMAT_SPEC.md line 899-902) — `-1`/absent = ungrouped is always legal, any Assembly
identifier not found in the `Assemblies` table degrades to ungrouped (loud, logged), same posture
as every other soft-degrade in this format.

## 6. Flagged for ARCH — the module-boundary rulings this brief's item 6 asked to be named, not guessed

1. **Selection-set-state-home** (§2) — does the "one instance each" rule in
   `Application_TabState_UI.h` extend to a deliberate, named shared-state exception, or does
   `CrossLayerSelectionState` need a different architectural home entirely? Recommendation stated
   in §2 (sibling-to-`ApplicationTabState`, injected-pointer pattern like `MapCanvas`'s existing
   `SetOverlayRecipe`/`SetManualMarkerDragSource`) — not self-ratified.
2. **New PARAMS types**: `Params::Assembly`, the three new per-instance merged fields, plus new
   pure helper functions needing a declared PARAMS (or MATH, for the rotate math) home, mirroring
   the `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId` precedent
   (`PropInstance_PARAMS.h:37-44`) exactly as the brief pointed at:
   - `ResolveAssemblyRootAncestor(assemblyId, assemblies)`
   - `WouldReparentCreateCycle(candidateId, newParentId, assemblies)`
   - `CollectAssemblyRecursiveMembership(assemblyId, assemblies, markers/props/decals)`
   - the rigid rotate-around-pivot math (§4) — MATH-layer candidate, same PROC-vs-MATH placement
     question `BuildSymmetryOrbit` already raised last round and (per that round's own notes) may
     still be unsettled.
3. New widget-library file/type naming (`TreeListWidget_UI.h`, `AssemblySelection_UI.h`,
   `AssemblyMemberKey_UI`) — low-risk, `_UI`-suffix/naming-law conformant by construction; flagging
   for a casual ARCH pass rather than treating as a real open question.

## 7. ❓ Precise questions to relay (per the brief's routing instructions — not guessed at)

**❓ ARCH Expert — selection-set state home.** Exact text to relay: *"A new, deliberately-shared
`CrossLayerSelectionState` is needed across MarkersTab, PropsTab (which also hosts Decals — no
separate DecalsTab exists), MapCanvas, and a new AssembliesTab. `Application_TabState_UI.h`'s
header states its 'one instance each' rule exists specifically to prevent tabs UNINTENTIONALLY
sharing state. Does this rule extend to a deliberate, named exception for genuinely-wanted shared
state, or does this selection concept need a different architectural home — e.g. a new sibling
struct on the shell (like `ApplicationTabState` itself), injected by pointer into each consumer the
way `MapCanvas::SetOverlayRecipe`/`SetManualMarkerDragSource` already are? Please rule the exact
home."*

**❓ ARCH/Format Expert — `.sanmap` field naming.** Exact text to relay: *"The brief's own working
spelling (`AssemblyId`/`parentAssemblyId`) uses the 'Id' abbreviation `ARCH §16.5` already ruled OUT
for the sibling new field `symmetryGroupIdentifier` on `MarkerTransform` ('Id' is not on §1.1's
permitted abbreviation list) — and `SANMAP_FORMAT_SPEC.md` (lines 875-884) already flags its OWN
existing `layerId`/`MarkerInstanceLayer::layerId` as a likely-wrong precedent predating that ban.
Given that: (a) does the new per-instance merged field become `assemblyIdentifier` (lowerCamelCase,
matching `layerIndex`/`symmetryGroupIdentifier`'s existing merge convention)? (b) does the new
top-level `Assemblies` array's own stable-id field become unqualified `identifier`/wire
`Identifier` (mirroring how `PropInstanceLayer`/`MarkerInstanceLayer` spell their OWN stable field
unqualified as `layerId`, not `propInstanceLayerId`), or fully-qualified
`assemblyIdentifier`/`ParentAssemblyIdentifier` for raw-JSON readability outside C++ type context?
Please rule the exact spelling for both."*

**❓ Generator Expert — symmetry-follow group-move precedence (non-blocking, deferred feature).**
Exact text to relay: *"§4's proposed precedence rule for group-move symmetry-following (deferred
this round, not built): at gesture-start, detect whether a selected member's own symmetry-mirror
sibling is ALSO in the same moved set; if so, suppress symmetry-follow for that pair for the whole
gesture (both receive only the flat rigid group-delta) — decided once, frozen, never re-evaluated
live. Symmetry-follow stays active, composed with the group delta, for any member whose sibling is
NOT co-selected. Is this geometrically/architecturally sound as the eventual design, given last
round's 'per-member-independent-orbit, not one combined group orbit' ruling?"*

**Confirm with the human directly (not an expert-routing question)**: whether "locked" gating for
the cross-layer multi-select (§2) should be dropped for this round (no PARAMS field exists for it
on any manual entity layer type today) or should trigger a new `bLocked` field on
`PropInstanceLayer`/`DecalInstanceLayer`/`MarkerInstanceLayer` now.

## Who else this touches
- **ARCH Expert**: §6/§7 items 1-3 — likely needs a full pass before anything here is
  coder-dispatchable, given the brief's own scope estimate (new widget primitive + new selection-set
  concept + new PARAMS shape).
- **Format Expert**: §5's wire shape and casing (jointly with ARCH's naming ruling, §7).
- **IO Architecture Expert**: migration posture for §5 (expected additive/no-bump, not
  self-ratified here).
- **Generator Expert**: §7's symmetry-follow question, only once group-move symmetry-following is
  actually scheduled (this round explicitly defers it).
- **UI Optimization Expert**: not consulted this round — nothing here yet exceeds authoring-scale
  (tens of Assemblies, hundreds of manually-authored leaves); flag if Assembly-eligible instance
  counts ever approach the 100k-entity overlay-rendering regime ARCH §14.9 already governs.

No coder-dispatchable ticket this round.

---

Files read to ground this design (all absolute paths under `D:\Projects\Sanctuary\Map Generator\`):
`work_orders\BRIEF_Assembly_R1.md`, `work_orders\DESIGN_MarkerLayerSymmetry_R1.md`/`_R2.md`,
`work_orders\STEP81_MarkersTabManualLayers_UI.md`, `work_orders\STEP94_MarkerDragAndFollowSymmetry_UI.md`,
`src\ui\DraggableListWidget_UI.h`, `src\ui\Section_UI.h`, `src\ui\VirtualListWidget_UI.h`,
`src\ui\Application_TabState_UI.h`, `src\ui\Application_Panels_UI.h`, `src\ui\MapCanvas_UI.h`,
`src\ui\MapCanvas_Draw_UI.cpp`, `src\ui\MapCanvas_MarkerDrag_UI.cpp`, `src\ui\MarkerDragGesture_UI.h`,
`src\ui\MarkerOrbitCorrespondence_UI.h`, `src\ui\MapCanvas_IconLayer_UI.h`,
`src\ui\MapCanvas_IconLayer_Cull_UI.cpp`, `src\ui\MapCanvas_IconLayer_CullManual_UI.cpp`,
`src\ui\MarkersTab_Manual_UI.h`, `src\ui\ArmiesTab_Units_UI.cpp`, `src\ui\LayerEditor_Group_UI.cpp`,
`src\ui\PropsTab_UI.h`/`.cpp`, `src\params\PropInstance_PARAMS.h`, `src\params\Layer_PARAMS.h`,
`sangen_arch_pack\specs\SANMAP_FORMAT_SPEC.md`, `ARCH_14_PreviewOverlayLayering.md`, `ARCH_14_12_Naming.md`.

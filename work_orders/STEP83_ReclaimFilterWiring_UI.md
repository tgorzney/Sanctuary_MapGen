# STEP83 — Wiring `bReclaimable` as the Props/Reclaim overlay filter predicate

**Layer:** UI. **Domain:** `Ui::ConfigureDefaultOverlayLayers` (STEP51's
`Application_OverlaySetup_UI.cpp`), `MapCanvas_IconLayer_Cull_UI.cpp` (STEP53's Manual sub-layer
walk). **Sequence:** consumer of `work_orders/STEP62_ReclaimPropFilter_PARAMS.md` (Gap 3,
`SEQUENCE_PreviewOverlayLayering.md:61`); sits between Phase 2.1 (STEP51) and Phase 3.1 (STEP53).
**Author:** SanGen UI Expert, with the UI Optimization Expert consulted on the hot-path placement
call in Fix item 0.

---

## 0. Verdict on the open scoping call — **own ticket, and the wiring is in NEITHER STEP50 nor (mostly) STEP53**

The consolidation asked: STEP50's bucket build, STEP53's draw pass, or both — and standalone ticket
or in-place amendment? All three named candidates are partly wrong, which is itself the reason this
needs its own file.

**The answer is that `bReclaimable` is not a filter predicate at all for the procedural half. It is
a routing decision, and it belongs at seed time in STEP51.** The flag is rule-level — it lives on
`Params::PropRule` (STEP62 Fix item 1), and `AppendPropRules` iterates `recipe.propRules` assigning
`configuration.ruleIndex = static_cast<int>(index)` (`src/proc/Placement_Rules_PROC.cpp:60,64`), so
every instance in the CSR bucket for rule `i` came from `recipe.propRules[i]` and carries that
rule's `bReclaimable` uniformly. A value that is constant across an entire bucket is resolved once,
where the bucket is *selected* — not N times, where its contents are *drawn*.

That is exactly what `SeedMarkerDomains` already does for the `category` column
(`STEP51_OverlayLayerDataModel_UI.md:198-210`), and STEP62 states in terms that Props/Reclaim is
"the **exact same pattern**" (`STEP62_ReclaimPropFilter_PARAMS.md:113-127`). Conforming to the
ratified precedent is the whole job for the procedural half.

The manual half cannot use that mechanism, for a structural reason established in §5 below, and
lands a **group-level** early-out in STEP53's Manual walk — still never a per-instance predicate.

### Why not STEP50's bucket build (ruled out, definitively)
1. **STEP50 already decided this exact question and answered "no."** Its own section "Why
   `category` gets no bucket index of its own" (`STEP50_ProceduralSubLayerCsrBucketIndex_UI.md:55-64`)
   rules that a *rule-level classification* — "fixed per rule, copied unchanged onto every instance
   that rule emits" — gets no index of its own, because the caller "already has the cheaper path:
   for each `recipe.markerRules[i]` whose `category == Spawn`, read that rule's own bucket from the
   `ruleIndex`-keyed index... **no second CSR, no drift risk between two indices that would have to
   agree**." `bReclaimable` is that same shape, so it gets that same answer. Adding a reclaim-aware
   index would be the drift risk STEP50 wrote that paragraph to forbid.
2. **It would break the invariant STEP50 deliberately protects.** `Build` is documented to take
   `bucketTotal` as "the caller's OWN rule-array size... NOT re-derived from `max(key)`, so a
   trailing rule with zero accepted instances still gets an addressable empty bucket"
   (`STEP50:109-117`), and STEP50's own unit test asserts it. Filtering inside `Build` would make
   bucket indices no longer equal `recipe.propRules` positions — silently invalidating every
   `OverlaySubLayerRef_UI::index` that resolves through it.
3. **It would break the non-overlay consumers.** `Data::RuleBucketIndex` is a general DATA-layer
   index over `Data::PlacementInstances`; picking and hit-testing legitimately want *all* props.
4. **It inverts the dependency direction.** Constitution §1: DATA is the computed output layer and
   "nothing depends upward." `bReclaimable` is PARAMS and the Props/Reclaim split is a UI concern —
   `ARCH_14_01_ModuleBoundaryDataVsParams.md` §14.1 rules `OverlayLayer_UI`/`overlayLayers` UI and
   session-only. Teaching a `src/data/` CSR index about `OverlayDomainKind_UI` would put a UI enum's
   partition inside DATA. That alone is disqualifying.

**STEP50 requires zero changes from this ticket.** Stated here so a coder does not "helpfully" add one.

### Why not a per-instance predicate in STEP53's draw loop (ruled out on the §14.9 constraint)
`ARCH_14_09_RenderingPerformance.md:4-8` mandates bulk `PrimReserve` + raw vertex/index writes and
names per-instance `AddImage()` a non-starter at 30-60ms/600k. A per-instance `bReclaimable` test
fails the same test that rule fails, for the same reason, in three separate ways:

- **It evaluates a loop-invariant N times.** The value is provably constant across the bucket
  (paragraph 2 above). At the 400k-500k placeholder budget (§14.9) that is up to half a million
  redundant data-dependent branches per frame, inside the exact region §14.9 requires be a tight
  bulk write.
- **It doubles the walk, not just the branch — and it does so precisely in the unbounded case.**
  With both a Props and a Reclaim layer enabled, a per-instance design has *each* layer walk *every*
  procedural props bucket and discard the complement: 2x the candidate visits, projections, and
  grid/AABB queries for identical drawn output. §14.9 states the grid gives "**zero help**
  fully-zoomed-out (everything visible, every bucket queried) — that case is genuinely O(N); the
  cross-layer budget above is what bounds it." A 2x multiplier applied to the one case that has no
  bound but the budget is the worst possible place to put avoidable work. Seed-time routing makes
  each layer walk only its own buckets: 1x total, partitioned.
- **It risks the double-count STEP62 explicitly forbids.** If the discard happens after emission
  rather than before, the same `PlacementInstance` enters the cross-layer visible-vertex budget
  twice — verbatim the failure STEP62's partition rationale exists to prevent
  (`STEP62:118-123`). Resolving at sub-layer selection makes the double-count structurally
  impossible rather than merely avoided by careful coding.

STEP53 does gain a change, but only for the **manual** half, and only as a **per-group** early-out
(§5) — O(groups), never O(instances), and outside the vertex-write region entirely.

### Why a standalone ticket rather than an in-place amendment to STEP50/STEP53
An in-place amendment was seriously considered and rejected. The decision spans **three** tickets,
not two, and its load-bearing content is the *reasoning*, not the diffs:

- The single most important output of this analysis is **"STEP50 changes nothing, and here is the
  four-part argument for why."** A negative ruling has no in-place home — it cannot be written as a
  diff against STEP50 without adding a section to a ticket that is not changing.
- The procedural delta lands in **STEP51**, which the consolidation did not even list as a
  candidate. Splitting the ruling across an edit to STEP51 and an edit to STEP53 would leave
  neither file carrying the reason the split is shaped the way it is — and the manual/procedural
  asymmetry (§5) is exactly the kind of thing a later reader "fixes" into a single convention if the
  justification is not in one place.
- Two ratified statements must be **retracted**, not just extended: STEP51's
  `// stays empty — no data/rule yet` (`STEP51:241`) and STEP53's out-of-scope line "Reclaim domain
  — no data or rule type exists yet" (`STEP53:350-351`). Retractions want an authority to point at.
- This file also surfaces a **blocking cross-ticket defect between STEP57 and STEP50** (§7) that
  belongs to neither STEP51 nor STEP53.

So: standalone ticket. Its content is nonetheless deliberately written as **surgical, quotable
deltas** against STEP51's and STEP53's specified code, so folding is mechanical. See §2 for
dispatch order.

---

## 1. Root problem
STEP62 ships `bool bReclaimable = false;` on `Params::PropRule` and `Params::PropInstanceGroup`
plus its `"Reclaimable"` wire round-trip, and closes with "**Zero rendering/overlay consumer in
this ticket**" (`STEP62:129-136`). Nothing reads the flag. Concretely, today:

- STEP51 seeds the Reclaim layer with **zero sub-layers, unconditionally**, annotated
  `// stays empty — no data/rule yet` (`STEP51_OverlayLayerDataModel_UI.md:240-241`), and seeds
  **every** `recipe.propRules[i]` into the Props layer via
  `PushProceduralRefs(propsLayer.subLayers, recipe.propRules.size())` (`STEP51:238`) regardless of
  the flag.
- STEP53 lists "Reclaim domain — no data or rule type exists yet (§14.2's table); zero cost if/when
  it lands, not exercised by this ticket's tests" as explicitly out of scope (`STEP53:350-351`).
- `ARCH_14_02_DataModel.md:39`'s table still reads `| Reclaim | n/a — no data yet | n/a — no rule
  type yet; slot reserved, zero cost until it ships |`.

Net effect once STEP62 lands: a designer can mark a prop rule reclaimable, it survives export and
import, and **every reclaimable prop still draws in the Props layer while the Reclaim layer stays
permanently empty**. Hiding Props would hide reclaim; the two domains do not partition anything.

Verified field/collection shapes this ticket is written against (read, not assumed):
- `Params::PropRule` — `src/params/ScatterRule_PARAMS.h:12-38`; `bReclaimable` inserts next to
  `bNearCliffs` (line 20) per STEP62.
- `Params::PropInstanceGroup { std::string blueprintPath; std::vector<PropTransform> transforms; }`
  — `src/params/PropInstance_PARAMS.h:24`; `bReclaimable` inserts here per STEP62.
- `Params::PropTransform { InstancedTransform transform; int layerIndex = 0; }` —
  `src/params/PropInstance_PARAMS.h:19`. Carries **no** reclaim flag, by STEP62's explicit ruling
  ("never to `PropTransform` (the per-instance placement record)", `STEP62:22-23`).
- `Params::PropInstanceLayer { name; color[4]; iconScale; }` —
  `src/params/PropInstance_PARAMS.h:30`. Carries **no** reclaim flag either. This is the fact §5 turns on.
- `Params::MapRecipe::propRules` (`MapRecipe_PARAMS.h:57`), `::props` (`:105`), `::propLayers` (`:107`).
- `Data::PlacementInstance::ruleIndex`/`category` — `src/data/PlacementInstance_DATA.h:46-47`. No
  reclaim column exists and this ticket adds none.

---

## 2. Dispatch order (read before anything else)
STEP51 and STEP53 are both **DRAFTED, not landed** (`SEQUENCE_PreviewOverlayLayering.md`, rows 2.1
and 3.1); none of `OverlayLayer_UI`, `Application_OverlaySetup_UI.cpp`, or
`MapCanvas_IconLayer_*_UI.*` exist in `src/` today. Therefore:

- **If STEP51 has not yet been dispatched to the Coder** when this ticket is: fold Fix item 1 into
  STEP51's dispatch. Same for Fix item 2 into STEP53's.
- **If either has already landed**: apply that item as the standalone diff written below.

Either way **this file is the authority on both deltas**, and STEP62 must be landed first (this
ticket does not compile without `bReclaimable`). This ticket has no dependency on STEP50 beyond
STEP50's public surface being unchanged.

---

## 3. Layer & accuracy class
UI. Accuracy class: **Visual** — screen-space overlay presentation only, the same Visual-class
posture STEP53 declares for the whole draw pass (`STEP53:298-300`). Nothing here feeds bake or
export; `ARCH_14_11`'s guardrail that overlay-side filtering may never mutate
`Data::PlacementInstances` applies unchanged and is verified in §8.

## 4. Backend policy
CPU only. Fix item 1 is a launch-time seeding loop; Fix item 2 is a scalar branch at group
granularity in an imgui-side CPU walk. No GPU kernel, therefore no `.glsl` sibling — the same
CPU/GPU-pairing exemption STEP53 already states for this module (`STEP53:72-74`).

---

## 5. The one asymmetry, and why it is forced (read before Fix item 2)
The procedural and manual halves resolve the partition at **different granularities**. This is not
two conventions invented for convenience; the manual half structurally cannot use the procedural
half's mechanism:

- **Procedural**: `bReclaimable` is on `PropRule`, and one `OverlaySubLayerRef_UI{ProceduralRule, i}`
  maps 1:1 to `recipe.propRules[i]`. The ref *is* the rule, so the ref can be routed to one domain.
- **Manual**: `bReclaimable` is on `PropInstanceGroup` (`recipe.props[i]`), but the Props Manual
  sub-layer ref index is a **`recipe.propLayers` index**, not a `recipe.props` index — STEP51 seeds
  `PushManualRefs(propsLayer.subLayers, recipe.propLayers.size())` (`STEP51:237`), matching §14.2's
  binding table row `| Props | recipe.propLayers[i] (PropInstanceLayer) | recipe.propRules[i] |`
  (`ARCH_14_02_DataModel.md:35`). Group and layer are related many-to-many through
  `PropTransform::layerIndex` (`PropInstance_PARAMS.h:19`), and `PropInstanceLayer`
  (`PropInstance_PARAMS.h:30`) carries no reclaim flag. **One `propLayers[k]` can legally hold
  transforms from both reclaimable and non-reclaimable groups**, so a manual sub-layer is not
  uniformly one domain's, and the partition cannot close at ref granularity. It closes one level
  down, at the group.

This is precisely the class of asymmetry `ARCH_14_06_OverlayDomainKindCoexistence.md:6-8` already
warns about — "Domain-kind is **asymmetric** versus DATA buckets... a coder must not assume
`domain == DATA-bucket identity`." Do not "simplify" the two halves into one mechanism in either
direction. Routing the manual half at seed time would silently drop instances; pushing the
procedural half down to group/instance granularity would re-import every cost §0 rejects.

Both halves nonetheless satisfy the same binding rule: **the predicate is evaluated at the coarsest
level at which the flag is constant, and never once per instance.**

---

## 6. Fix

### Item 1 — procedural half: seed-time routing in STEP51's `Application_OverlaySetup_UI.cpp`

Replace STEP51's Props/Reclaim block (`STEP51:235-242`) with:

```cpp
    OverlayLayer_UI propsLayer;   propsLayer.name   = "Props";
    propsLayer.domainKind   = OverlayDomainKind_UI::Props;
    OverlayLayer_UI reclaimLayer; reclaimLayer.name = "Reclaim";
    reclaimLayer.domainKind = OverlayDomainKind_UI::Reclaim;
    SeedPropReclaimDomains(propsLayer, reclaimLayer, recipe);
```
The `// stays empty — no data/rule yet` comment on `reclaimLayer` (`STEP51:241`) is **retracted** —
delete it, do not amend it. `Decals` and every other domain's seeding is untouched.

New file-local helper, placed next to `SeedMarkerDomains` (`STEP51:198-210`) whose shape it
deliberately mirrors:

```cpp
// Props/Reclaim mutually-exclusively partition `recipe.propRules` by `bReclaimable` — the same
// seed-time routing SeedMarkerDomains performs on `category` (§14.6; STEP62's "exact same
// pattern"). `index` stays the GLOBAL recipe.propRules position, never a per-domain running
// count: it is what STEP50's CSR bucket is keyed on (bucketTotal == recipe.propRules.size()).
// Manual refs go to BOTH domains on purpose — one recipe.propLayers[k] can hold transforms from
// reclaimable AND non-reclaimable PropInstanceGroups, so a propLayer is not uniformly one
// domain's; that half of the partition closes at group granularity in the draw pass (STEP83 §5).
void SeedPropReclaimDomains(OverlayLayer_UI& propsLayer, OverlayLayer_UI& reclaimLayer,
                            const Params::MapRecipe& recipe) {
    const int manualLayerCount = static_cast<int>(recipe.propLayers.size());
    PushManualRefs(propsLayer.subLayers,   manualLayerCount);   // Manual first — STEP51's order
    PushManualRefs(reclaimLayer.subLayers, manualLayerCount);
    for (std::size_t index = 0; index < recipe.propRules.size(); ++index) {
        OverlayLayer_UI& target = recipe.propRules[index].bReclaimable ? reclaimLayer : propsLayer;
        target.subLayers.push_back(OverlaySubLayerRef_UI{
            OverlaySubLayerKind_UI::ProceduralRule, static_cast<int>(index), true});
    }
}
```

**Trap, stated because it is the one way to get this silently wrong:** do **not** renumber the
procedural refs per domain. If Reclaim's first ref were given `index == 0` instead of its real
`recipe.propRules` position, it would read a bucket belonging to a different rule and draw the
wrong instances with no error anywhere. `SeedMarkerDomains` has the identical constraint and solves
it the identical way (`STEP51:198-210`'s single shared `flatIndex`, incremented for every rule
regardless of which layer it routes to).

**§1.5 compliance — the split STEP51 made conditional becomes mandatory.** STEP51's own note
(`STEP51:273-277`) says to split `Application_OverlaySetup_UI.cpp` into a
`Application_OverlaySetup_Seed_UI.cpp` aspect file "if it runs long." Its reference implementation
is already ~94 lines (`STEP51:178-271`); this item adds ~16. That crosses the soft-100 ceiling
(`ARCH_01_05_FileSizeCeilings.md`), so **perform the split — it is no longer optional.** Move the
four seeding helpers (`PushProceduralRefs`, `PushManualRefs`, `SeedMarkerDomains`,
`SeedUnitsManualSubLayers`) plus `SeedPropReclaimDomains` into
`src/ui/Application_OverlaySetup_Seed_UI.cpp` behind the same declarations, leaving
`ConfigureDefaultOverlayLayers` and `ResolveUnitsManualSubLayer` in the original file. Both land
comfortably under 100 lines; no ceiling exception is requested by this ticket. `src/ui/*.cpp` is
already glob-collected (`CMakeLists.txt:148`), so the new file needs no CMake edit.

### Item 2 — manual half: per-group early-out in STEP53's Manual sub-layer walk

STEP53 step 4 (`STEP53:109-111`) says only "For each enabled `Manual` sub-layer, walk its
(typically small, authored) array directly." The enclosing walk's exact shape stays STEP53's to
define; this ticket binds only **the predicate and the loop level it sits at**:

```cpp
// MapCanvas_IconLayer_Cull_UI.cpp — Manual sub-layer walk, Props and Reclaim domains ONLY
// (Units/Decals/Alloy/SpawnsArmies are untouched by this ticket).
// Evaluated ONCE PER PropInstanceGroup — never per PropTransform, never inside the
// PrimReserve/PrimWrite region (§14.9). A non-matching group is skipped whole.
const bool bWantReclaimable = (layer.domainKind == OverlayDomainKind_UI::Reclaim);
for (const Params::PropInstanceGroup& group : recipe.props) {
    if (group.bReclaimable != bWantReclaimable) continue;      // en bloc, before any transform
    for (const Params::PropTransform& propTransform : group.transforms) {
        if (propTransform.layerIndex != subLayerRef.index) continue;
        /* ...STEP53's existing per-instance candidate emission, unchanged... */
    }
}
```

Binding properties of this predicate:
- **Group-level, not instance-level.** The test hoists out of the transform loop entirely.
- **Before emission, not after.** A skipped group contributes zero candidates, so it never enters
  the cross-layer visible-vertex budget — the double-count STEP62 forbids cannot occur.
- **The procedural walk gains no new code at all.** Item 1 already routed those refs; a procedural
  sub-layer present in a layer's list is by construction that layer's, and its CSR bucket is read
  exactly as STEP53 already specifies.

Also **retract** STEP53's out-of-scope line "Reclaim domain — no data or rule type exists yet
(§14.2's table); zero cost if/when it lands, not exercised by this ticket's tests"
(`STEP53:350-351`). Replace with: *"Reclaim domain — Props/Reclaim partition `recipe.propRules`/
`recipe.props` by `bReclaimable` per STEP62/STEP83; the procedural half is resolved at seed time in
STEP51 and costs this pass nothing, the manual half is STEP83 Fix item 2's per-group early-out."*

### Item 3 — no new appearance slot, no new struct field
`OverlayLayerSettings` gets **no** `reclaimAppearance` member. §14.5's split (transcribed at
`STEP51:279-292`) gives Props/Decals their appearance through
`recipe.propLayers[ref.index].color`/`.iconScale` read directly; Reclaim's Manual refs are
`recipe.propLayers` indices too (Item 1), so they resolve through the identical path with no shadow
copy. `OverlaySessionAppearance` exists only for the three domains that have no such record
(Alloy/SpawnsArmies/Units) and Reclaim is not one of them. Likewise **no new field on
`OverlayLayer_UI`** — its §14.2 binding shape is unchanged by this ticket, and no reclaim column is
added to `Data::PlacementInstance`/`Data::PlacementInstances`.

---

## 7. Blocking cross-ticket defect found while drafting — NOT fixed here, must be routed

**`STEP57_ManualPropsDecalsProcResolution_PROC.md` will pollute STEP50's props CSR bucket 0.**
Verified by reading both tickets:

- STEP57's `MakeManualInstance` (`STEP57:163-171`) constructs a `Data::PlacementInstance` and
  deliberately leaves `ruleIndex` at its struct default, ruling it out explicitly:
  "`ruleIndex`, `category`, `symmetryIdentifier`... all left at `Data::PlacementInstance`'s own
  struct defaults (`PlacementInstance_DATA.h:46-51`: `ruleIndex = 0`...)" (`STEP57:91-98`). It then
  `results.props.Append(instance)` (`STEP57:180`).
- STEP50 builds the props index over `placementResults.props.ruleIndex` with
  `bucketTotal = recipe.propRules.size()` (`STEP50:202-204`), dropping only keys **outside**
  `[0, bucketTotal)` (`STEP50:118-134`).
- `0` is in range whenever `propRules` is non-empty. So **every manually-authored prop instance
  lands in the bucket belonging to `recipe.propRules[0]`.**
- STEP57's out-of-scope section asserts the two are unrelated — "Phase 1.3's CSR bucket-index build
  ... unrelated column, unrelated mechanism" (`STEP57:272-274`). That is incorrect: they share the
  `ruleIndex` column.

Consequence for this ticket specifically: post-STEP57, procedural sub-layer ref `{ProceduralRule, 0}`
would draw manual props too — double-drawing them alongside their own Manual sub-layer, and
mis-assigning any reclaimable manual group's instances to whichever domain rule 0 routed to.

**Recommended one-line fix, routed to the ARCH Expert and STEP57's owner — explicitly NOT
authorized by this ticket:** set `instance.ruleIndex = -1;` in STEP57's `MakeManualInstance`, the
same "not applicable" sentinel STEP57 already adopts for `manualLayerId`/`armyIndex`. STEP50's
`Build` already drops negative keys by its own documented rule, so no STEP50 change would be needed
and the exclusion becomes automatic. This is a PROC/DATA-layer semantic change and is not a UI
Expert call to make.

**Precondition:** this ticket is correct as written for any dispatch order in which STEP57 has not
landed, and remains correct after STEP57 lands **only** if the above (or an equivalent) is applied.
If STEP57 is scheduled ahead of STEP53, resolve this first.

Two smaller inherited gaps, named so they are not re-derived, neither owned here:
- **Launch-time-only seeding.** `ConfigureDefaultOverlayLayers` runs once from `Application`'s
  constructor and STEP51 out-of-scopes live resync (`STEP51:126-130, 338-339`). So toggling
  `bReclaimable` mid-session — or importing a `.sanmap` — leaves the routing stale. This is
  **identical** to the staleness `SeedMarkerDomains` already has for `category`, and is a
  whole-overlay-stack gap, not a Reclaim one. Deliberately **not** given a Reclaim-specific
  workaround: diverging one domain from the ratified precedent to dodge a shared gap is worse than
  the gap. Route as one cross-domain re-seed ticket.
- **§14.2 table.** `ARCH_14_02_DataModel.md:39`'s Reclaim row still reads "n/a — no data yet."
  STEP62 already supplies the exact replacement text for both the Reclaim and Props rows
  (`STEP62:95-111`) for the ARCH Expert to apply. No agent but the ARCH Expert writes it.

---

## 8. Verify

**Item 1 — extend `src/ui/OverlayLayer_Settings_UI_Test.cpp` (STEP51's test file):**
- **Every existing STEP51 case stays green with zero edits.** This is load-bearing, so state why it
  holds: `MakeDefaultMapRecipe()` (`src/ui/Application_Recipe_UI.cpp:72-84`) never touches
  `propLayers` or `props`, and `bReclaimable` defaults to `false` (STEP62), so STEP51 test 2's
  "Props has exactly one `{ProceduralRule, 0}`... Reclaim has zero sub-layers" and test 4's
  `[{Manual,0},{Manual,1},{ProceduralRule,0..2}]` assertion both still hold exactly. If either
  needs editing, Item 1 was implemented wrong — do not edit the test to match.
- **New — the partition.** Recipe with four `propRules`, `bReclaimable` = `{false, true, false, true}`.
  Assert Props' procedural refs are exactly `[{ProceduralRule,0},{ProceduralRule,2}]` and Reclaim's
  exactly `[{ProceduralRule,1},{ProceduralRule,3}]`. **The indices are the assertion** — this is the
  test that catches per-domain renumbering.
- **New — mutual exclusivity, mechanically.** Over the same recipe, assert every
  `recipe.propRules` index appears in exactly one of the two layers' procedural ref sets, and their
  union covers `[0, propRules.size())` with no gap. Order-preserving within each domain.
- **New — all-reclaimable and none-reclaimable edges.** All four `true`: Props has zero procedural
  refs, Reclaim has four. All `false`: the reverse. Neither layer is ever dropped from
  `overlayLayers` — both rows still exist for the View toolbar in both cases.
- **New — manual refs go to both.** Recipe with 3 `propLayers`: both Props and Reclaim carry
  `[{Manual,0},{Manual,1},{Manual,2}]`, Manual-before-Procedural order preserved in both.
- **Unchanged domains.** Alloy/SpawnsArmies/Units/Decals sub-layer sets are byte-identical to
  STEP51's seeding for every fixture above.

**Item 2 — extend STEP53's culling test (`MapCanvas_IconLayer_*_UI_Test.cpp`):**
- Recipe with one `propLayer` and three `PropInstanceGroup`s, `bReclaimable` = `{true, false, true}`,
  each holding a known distinct transform count with `layerIndex == 0`. Walking the Props layer's
  `{Manual,0}` ref yields exactly group 1's transforms; walking Reclaim's `{Manual,0}` yields
  exactly groups 0 and 2's. Union equals the full set, intersection is empty.
- **Group-level, proven not assumed:** a call counter on the predicate site asserts it is invoked
  exactly `groupCount` times per sub-layer walk — **not** `transformCount` times. This is the check
  that catches a regression to a per-instance test, which would otherwise pass every correctness
  assertion above while violating §14.9.
- A `propLayer` holding only reclaimable groups contributes zero candidates to Props (and vice
  versa) — an empty contribution, never a crash and never a zero-quad draw command
  (`STEP53:197`'s existing guard).
- **§14.11 determinism guardrail:** byte-compare `Data::PlacementInstances` and STEP50's
  `Data::RuleBucketIndexSet` before and after a full partitioned draw — bit-identical. The Reclaim
  partition is a read-side view only and must never touch DATA.
- **Budget non-inflation:** with both layers enabled over the fixture above, the pre-decimation
  candidate count equals the count with only Props enabled plus the count with only Reclaim
  enabled — no instance counted twice (STEP62's stated concern, directly asserted).

**Build:** full `SanGenV2` rebuild clean; `ctest -C Debug -R OverlayLayer_Settings_UI_Test` and
STEP53's icon-layer suite green; `ApplicationShell_*`, `MapCanvas_*`, and STEP50's
`RuleBucketIndex_DATA_Test` all green with **zero edits** — STEP50 is untouched by this ticket
(§0), and any edit needed there means Item 1 or 2 was implemented in the wrong place.

---

## 9. Files touched
- `src/ui/Application_OverlaySetup_UI.cpp` (STEP51's) — Props/Reclaim block replaced; helpers moved out.
- `src/ui/Application_OverlaySetup_Seed_UI.cpp` — **new**, the mandated §1.5 split; hosts
  `SeedPropReclaimDomains` alongside STEP51's four existing seeding helpers.
- `src/ui/MapCanvas_IconLayer_Cull_UI.cpp` (STEP53's) — per-group early-out in the Manual walk.
- `src/ui/OverlayLayer_Settings_UI_Test.cpp` (STEP51's) — new cases only, no edits to existing ones.
- STEP53's icon-layer test file — new cases per §8.

Unchanged, deliberately: `src/data/RuleBucketIndex_DATA.h`, `src/data/RuleBucketIndexSet_DATA.h`,
`src/pipeline/GenerationAssembler_*`, `src/ui/OverlayLayer_Settings_UI.h`, `src/params/*`,
`src/io/*`, `CMakeLists.txt`.

## 10. ARCH rules invoked
- **Constitution §1** — dependency direction; a UI domain partition may not live in a DATA index (§0).
- **Constitution §7** — work-order schema; §11's estimate carries its basis tag.
- **`ARCH_01_05_FileSizeCeilings.md`** — soft-100 ceiling forces the mandated seed-file split
  (Item 1); no exception requested.
- **§14.1** — `overlayLayers` is UI and session-only; the partition is a UI-layer decision.
- **§14.2** — binding `OverlayLayer_UI` shape unchanged (no new field); the sub-layer→data mapping
  table is the authority for Manual refs being `propLayers` indices (§5). Its Reclaim row still
  needs the ARCH Expert's STEP62-supplied update (§7).
- **§14.6** — `domain != DATA-bucket identity`; the governing precedent both for the
  `Alloy`/`SpawnsArmies` routing this ticket copies and for the manual/procedural asymmetry (§5).
- **§14.9** — bulk vertex writes; no per-instance predicate in the draw region; the fully-zoomed-out
  case is bounded only by the cross-layer budget, so avoidable 2x walks there are disqualifying (§0).
- **§14.11** — overlay-side filtering may never mutate `Data::PlacementInstances` (§8).
- **§14.12** — `_UI` suffix throughout.

## 11. Performance estimate (basis: STRUCTURAL — no benchmark needed or claimed)
No benchmark is offered because this ticket's performance content is a *removal* of work relative to
the rejected alternatives, not an optimization with a measurable delta against a shipped baseline.

- **Item 1** is a launch-time loop over `recipe.propRules`, run once in `Application`'s constructor.
  Same order of cost as the `SeedMarkerDomains` call already beside it. Not a frame-time path.
- **Item 2** adds one boolean compare per `PropInstanceGroup` per Manual sub-layer walk —
  O(groups x manual sub-layers), with groups at blueprint cardinality (tens to hundreds), against
  instance counts §14.9 sizes at 600k. Structurally negligible, and total transform-touching work is
  **unchanged**: each transform is visited by exactly one of the two domains, with its group skipped
  wholesale in the other.
- **Procedural draw path: exactly zero added per-frame cost.** The partition was consumed at seed
  time; the per-frame walk reads the same buckets the same way STEP53 already specifies.
- Relative to the rejected per-instance design (§0): avoids up to ~450k redundant branches per frame
  at the §14.9 placeholder budget, and avoids a 2x candidate-visit multiplier in the fully-zoomed-out
  case §14.9 identifies as the one genuinely O(N) case. Those figures are §14.9's own placeholders,
  not ratified constants (`ARCH_14_09_RenderingPerformance.md:9-16`), and are cited here only to
  size the comparison — no new number is introduced.

## 12. Explicit out-of-scope
- **Any change to STEP50** — ruled out with reasons in §0. Zero edits to `RuleBucketIndex_DATA.h`,
  `RuleBucketIndexSet_DATA.h`, or `GenerationAssembler_*`.
- **Fixing STEP57's `ruleIndex = 0` collision** (§7) — PROC/DATA semantics, routed to ARCH/STEP57.
- **Live re-seeding of `overlayLayers` on recipe change or import** (§7) — a whole-stack gap STEP51
  already owns; must not be solved Reclaim-specifically.
- **Applying the `ARCH_14_02_DataModel.md` §14.2 table edit** — ARCH Expert only; STEP62 already
  supplies the text.
- **Editing STEP50/STEP51/STEP53/STEP62 themselves.** This file specifies the deltas; the Coder
  applies them at dispatch per §2.
- **Any authoring UI for `bReclaimable`** (a checkbox in `PropsTab_Rules_UI.cpp` /
  `PropsTab_Manual_UI.cpp`) — STEP62 out-of-scopes it and this ticket does not reopen it. Until it
  exists the flag is set only by import, which is sufficient to exercise everything above.
- **Deriving `bReclaimable` from real blueprint data** — game props express reclaim as `tags`
  containing `"HARVESTABLE"` plus an `economy.harvest{alloys, plasma|energy}` yield table, which
  SanGen's bool deliberately does **not** mirror 1:1 (`STEP62:14-19`). Importer derivation belongs
  to the unscoped texture/sanpack importer track. Do not add a yield table, a tag array, or any
  richer reclaim representation here.
- **Decals** — STEP62 is Props-domain only (`STEP62:160-161`); `DecalRule`/`DecalInstanceGroup` get
  no reclaim flag and the Decals layer's seeding and walk are untouched.
- **Reclaim-specific icons, colors, LOD thresholds, or a `reclaimAppearance` slot** (Item 3).
- **A reclaim column on `Data::PlacementInstance`/`Data::PlacementInstances`** — not needed by
  either half, and adding one would put a PARAMS classification into the DATA SoA for no consumer.

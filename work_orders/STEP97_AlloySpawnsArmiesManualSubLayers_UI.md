# STEP97 — Alloy/SpawnsArmies gain Manual overlay sub-layers (corrects STEP51's "zero Manual sub-layers" design)

**Layer:** UI. **Domain:** `SeedMarkerDomains` in the real, shipped
`src/ui/Application_OverlaySetup_Seed_UI.cpp` (STEP83 §1.5's split of
`Application_OverlaySetup_UI.cpp`; forward-declared at `Application_OverlaySetup_UI.cpp:16` and
called at `:28`). **Ratifying authority:** `ARCH_14_14_AlloySpawnsArmiesManualRouting.md` (§14.14,
ratified) and `ARCH_14_02_DataModel.md:38`'s now-updated sub-layer -> data mapping table row.
**Corrects:** the zero-Manual-refs `SeedMarkerDomains` STEP51 shipped
(`Application_OverlaySetup_Seed_UI.cpp:25-37`) — this ticket edits that function directly; it is
real, landed code, not work-order prose.

## Premise correction from the prior draft of this ticket
The previous version of this ticket was drafted when STEP51/STEP60/STEP66 had zero trace in
`src/` and the ARCH-level routing question (does `Params::MarkerInstanceLayer` need a discriminator
field?) was open. Both are now resolved:
- STEP51, STEP60, and STEP66 are landed. `SeedMarkerDomains` is real code
  (`Application_OverlaySetup_Seed_UI.cpp:25-37`), `Params::MarkerInstanceLayer`/`MarkerTransform`/
  `MarkerInstanceGroup` are real (`src/params/MarkerInstance_PARAMS.h:23-56`), and
  `MapRecipe::markerLayers`/`markerRuleLayers` are real (`src/params/MapRecipe_PARAMS.h:60,105-111`).
- `ARCH_14_14_AlloySpawnsArmiesManualRouting.md` rules the routing question: no new field on
  `MarkerInstanceLayer`; route **per-transform** by comparing the owning `MarkerInstanceGroup::name`
  against the (to-be-promoted) `Params::kSpawnMarkerGroupName` reserved literal.

## Root problem
The real, shipped `SeedMarkerDomains` only walks the Procedural side today:

```cpp
// Application_OverlaySetup_Seed_UI.cpp:25-37 (verbatim, current shipped body)
void SeedMarkerDomains(OverlayLayer_UI& alloyLayer, OverlayLayer_UI& spawnsArmiesLayer,
                       const Params::MapRecipe& recipe) {
    int flatIndex = 0;
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers) {
        for (const Params::MarkerRule& rule : layer.rules) {
            OverlayLayer_UI& target = rule.category == Params::MarkerCategory::Spawn
                                           ? spawnsArmiesLayer : alloyLayer;
            target.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::ProceduralRule, flatIndex, true});
            ++flatIndex;
        }
    }
}
```

The function's own header comment (`:21-24`) says it out loud: *"Zero Manual refs for either domain
(STEP51 scope; Manual Alloy/SpawnsArmies routing over `recipe.markerLayers` is a later,
ARCH_14_14-ruled successor ticket, gated on this one landing)."* This ticket is that successor.

Every other Manual-carrying domain seeds refs from its manual roster before its procedural refs
(`Application_OverlaySetup_Seed_UI.cpp:16-19` `PushManualRefs`, called from `SeedPropReclaimDomains`
at `:57-58` and directly in `Application_OverlaySetup_UI.cpp:43` for Decals). Alloy/SpawnsArmies are
the one pair still stuck at "zero Manual sub-layers," even though `recipe.markers`/`recipe.markerLayers`
(the manual marker roster and its display-bucket metadata, STEP60) have existed in `src/` the whole
time this function has been landed.

## ARCH ruling — quoted, this is the design this ticket implements
`ARCH_14_14_AlloySpawnsArmiesManualRouting.md:50-61` (the operative paragraph):

> `SeedMarkerDomains` routes per-transform, not per-`markerLayers` entry. For each
> `recipe.markerLayers[i]`, `SeedMarkerDomains` (already walking `recipe.markers[*].transforms[*]`
> to seed refs) determines, independently:
> - does `layerIndex == i` have at least one contributing transform whose owning
>   `MarkerInstanceGroup::name == Params::kSpawnMarkerGroupName`? If so, push a `Manual`
>   `OverlaySubLayerRef_UI{ index = i }` into **SpawnsArmies**' `subLayers`.
> - does `layerIndex == i` have at least one contributing transform whose owning group's `name !=
>   Params::kSpawnMarkerGroupName`? If so, push the same-shaped ref into **Alloy**'s `subLayers`.
>
> A single `recipe.markerLayers[i]` entry legally appears in **both** lists when it mixes types —
> this is correct, not a bug: the direct consequence of layers being cross-cutting display buckets.

And the render-time corollary (`:62-70`, amending §14.2's "index" semantics for these two domains
only): a Manual `OverlaySubLayerRef_UI::index` for Alloy/SpawnsArmies selects the `markerLayers`
entry, but the (not-yet-built) render-time consumer must ALSO filter by owning-group name — `index`
alone no longer means "every transform at this `layerIndex`" for these two domains, same asymmetry
§14.6 already names for Props/Reclaim ("a coder must not assume `domain == DATA-bucket identity`").

**The discriminator, confirmed real today:** `Ui::kSpawnMarkerGroupName` (currently UI-only,
`src/ui/MarkersTab_Manual_UI.h:41`, value `"Spawn"`, comment: *"The fixed group name
SANMAP_FORMAT_SPEC reserves for the commander-spawn roster... confirmed live in-game"*), already
load-bearing at `MarkersTab_Manual_UI.h:104-106` (`IsSpawnMarkerGroup`) and independently duplicated
as a raw literal at `src/io/MapImporter_ArmyIdentityNormalize_IO.cpp:59` (`if (group.name != "Spawn")
continue;`). ARCH_14_14 (`:38-48`) rules this constant is promoted to PARAMS —
`Params::kSpawnMarkerGroupName`, `inline constexpr const char*`, `"Spawn"`, declared in
`src/params/MarkerInstance_PARAMS.h` beside `MarkerInstanceGroup` — with `IO`'s raw literal and
`UI`'s local constant both becoming references to the one symbol. This ticket folds that promotion
in: `PARAMS` is where both `IO` and `UI` are already legally allowed to depend, and
`SeedMarkerDomains` itself needs the constant.

## Precedent cross-check — STEP83's `SeedPropReclaimDomains` is a structurally different answer to a similar-looking problem, and this ticket does NOT copy its Manual pattern
STEP83 solved an analogous "one manual layer, mixed group membership" problem for Props/Reclaim
(`Application_OverlaySetup_Seed_UI.cpp:47-63`): a `recipe.propLayers[k]` can hold transforms from
both reclaimable and non-reclaimable `PropInstanceGroup`s, so `SeedPropReclaimDomains` pushes a
Manual ref for **every** `propLayers` index into **both** `propsLayer.subLayers` and
`reclaimLayer.subLayers` unconditionally (`:57-58`, `PushManualRefs(propsLayer.subLayers,
manualLayerCount)` / `PushManualRefs(reclaimLayer.subLayers, manualLayerCount)`), and defers the
actual reclaimable/non-reclaimable split to the DRAW pass, evaluated once per `PropInstanceGroup`
(`MapCanvas_IconLayer_CullManual_UI.cpp:78-100` `ResolvePropsManual`, gated by
`layer.domainKind == OverlayDomainKind_UI::Reclaim` at `:85`).

Alloy/SpawnsArmies do **not** get the same "push to both unconditionally, resolve later" treatment,
and ARCH_14_14 is explicit about this (its pseudocode is a **conditional existence check per
`layerIndex`**, not an unconditional dual-push): Props/Reclaim's group-level predicate
(`bReclaimable`) is cheap and total — every `PropInstanceGroup` has exactly one `bReclaimable` value,
so pushing "maybe applicable" refs to both lists and filtering at draw time costs nothing extra.
Alloy/SpawnsArmies' predicate is instead keyed on `MarkerInstanceGroup::name`, an open string with no
guaranteed "every layer has some Spawn content" property — most authored marker layers will contain
**zero** Spawn-type transforms, and the unconditional-dual-push pattern would put a dead, always-empty
`{Manual, i}` ref into `spawnsArmiesLayer.subLayers` for every non-Spawn-touching layer. ARCH_14_14's
actual-existence-check design keeps `subLayers.size()` meaningful at the cost of one real scan over
`recipe.markers[*].transforms[*]` at seed time — already the cost `SeedUnitsManualSubLayers`
(`Application_OverlaySetup_Seed_UI.cpp:39-45`) and `ResolveUnitsManualSubLayer`
(`Application_OverlaySetup_UI.cpp:50-66`) already pay for a structurally similar walk. This is
ARCH's ruling, not a design choice open to this ticket — documented here so a future reader does not
"simplify" this function back toward STEP83's push-both pattern under the mistaken belief the two are
interchangeable.

## Fix

### 1. Promote the reserved literal to PARAMS
In `src/params/MarkerInstance_PARAMS.h`, beside `MarkerInstanceGroup` (after line 56):

```cpp
// The fixed group name SANMAP_FORMAT_SPEC reserves for the commander-spawn roster (moved from
// UI-only `MarkersTab_Manual_UI.h::kSpawnMarkerGroupName`, ARCH_14_14, so IO/UI/PIPELINE consumers
// share one symbol instead of three independent occurrences of the same literal).
inline constexpr const char* kSpawnMarkerGroupName = "Spawn";
```

- `src/ui/MarkersTab_Manual_UI.h:41` — replace the local `inline constexpr const char*
  kSpawnMarkerGroupName = "Spawn";` with a reference to `Params::kSpawnMarkerGroupName` (either a
  `using Params::kSpawnMarkerGroupName;` inside `namespace Ui`, or rewrite the file's call sites —
  `:41` decl, `:105` `IsSpawnMarkerGroup` — to qualify `Params::kSpawnMarkerGroupName` directly).
  `src/ui/MarkerDragGesture_UI.h:34`'s `kArmyKeyedMarkerGroupName = kSpawnMarkerGroupName` keeps
  compiling unchanged either way (unqualified lookup still resolves inside `namespace Ui`).
- `src/io/MapImporter_ArmyIdentityNormalize_IO.cpp:59` — replace the raw `"Spawn"` literal in
  `if (group.name != "Spawn") continue;` with `Params::kSpawnMarkerGroupName`.

### 2. `SeedMarkerDomains` — add the per-transform Manual walk, Manual-before-Procedural (STEP51's order)
`src/ui/Application_OverlaySetup_Seed_UI.cpp:25-37`, rewritten:

```cpp
void SeedMarkerDomains(OverlayLayer_UI& alloyLayer, OverlayLayer_UI& spawnsArmiesLayer,
                       const Params::MapRecipe& recipe) {
    // Manual (ARCH_14_14): route per-TRANSFORM, not per recipe.markerLayers[i] entry. A layer is a
    // cross-cutting display bucket with no category field of its own (§16.1) — a single layerIndex
    // legally mixes a Spawn-type transform and a non-Spawn transform, so it can legally push a ref
    // into BOTH domains. Existence-checked, not unconditional (contrast SeedPropReclaimDomains'
    // push-both-then-filter-at-draw pattern — deliberately NOT reused here, see this ticket's
    // precedent cross-check).
    const std::size_t manualLayerCount = recipe.markerLayers.size();
    std::vector<bool> hasSpawnContribution(manualLayerCount, false);
    std::vector<bool> hasAlloyContribution(manualLayerCount, false);
    for (const Params::MarkerInstanceGroup& group : recipe.markers) {
        const bool bIsSpawnGroup = group.name == Params::kSpawnMarkerGroupName;
        for (const Params::MarkerTransform& transform : group.transforms) {
            if (transform.layerIndex < 0
                || static_cast<std::size_t>(transform.layerIndex) >= manualLayerCount) continue;
            const std::size_t layerIndex = static_cast<std::size_t>(transform.layerIndex);
            if (bIsSpawnGroup) hasSpawnContribution[layerIndex] = true;
            else               hasAlloyContribution[layerIndex] = true;
        }
    }
    for (std::size_t layerIndex = 0; layerIndex < manualLayerCount; ++layerIndex) {
        if (hasSpawnContribution[layerIndex])
            spawnsArmiesLayer.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::Manual, static_cast<int>(layerIndex), true});
        if (hasAlloyContribution[layerIndex])
            alloyLayer.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::Manual, static_cast<int>(layerIndex), true});
    }

    // Procedural — unchanged from STEP51's shipped body.
    int flatIndex = 0;
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers) {
        for (const Params::MarkerRule& rule : layer.rules) {
            OverlayLayer_UI& target = rule.category == Params::MarkerCategory::Spawn
                                           ? spawnsArmiesLayer : alloyLayer;
            target.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::ProceduralRule, flatIndex, true});
            ++flatIndex;
        }
    }
}
```

`recipe.markers` and `recipe.markerLayers` are both already real fields
(`src/params/MapRecipe_PARAMS.h:105,111`); `MarkerTransform::layerIndex` is real
(`src/params/MarkerInstance_PARAMS.h:43`). No signature change to `SeedMarkerDomains` — its call
site (`Application_OverlaySetup_UI.cpp:28`) is untouched.

### 3. Retire the stale "carries zero Manual sub-layers" comment
`src/ui/OverlayLayer_Settings_UI.h:49-51`'s `OverlaySessionAppearance` comment currently reads
*"Alloy/SpawnsArmies carry zero Manual sub-layers this sequence (blocked...)."* Update it to reflect
this ticket's landing — `OverlaySessionAppearance` itself is unaffected (Alloy/SpawnsArmies read/
write their Manual `OverlaySubLayerRef_UI::index` against `recipe.markerLayers` directly, same
no-shadow-copy posture Props/Decals already use), so this is a comment-only correction, not a struct
change.

### 4. `MapCanvas_IconLayer_CullManual_UI.cpp`'s `ResolveManualSubLayer` switch — explicitly NOT touched here
`:130-145`'s switch hits `default: return; // Alloy/SpawnsArmies carry no Manual sub-layers this
sequence` for any domain other than Units/Props/Reclaim/Decals. This ticket's `SeedMarkerDomains` fix
will start producing `OverlaySubLayerRef_UI{Manual, i}` entries in `alloyLayer.subLayers`/
`spawnsArmiesLayer.subLayers` — and every one of them will silently no-op through this `default:
return` until a follow-up ticket adds a `ResolveMarkersManual` case (walking
`recipe.markers[*].transforms[*]` filtered by `layerIndex == subLayerArrayIndex` AND
`(group.name == Params::kSpawnMarkerGroupName) == (layer.domainKind ==
OverlayDomainKind_UI::SpawnsArmies)`, per ARCH_14_14's corollary). ARCH_14_14 itself names this
consumer as a separate item — *"its render-time Manual-sub-layer instance-gathering consumer (not
yet named/built)"* — distinct from the `SeedMarkerDomains` fix this ticket makes. **This ticket is
purely a seed-time/data-model fix with no visible on-canvas effect until that follow-up lands.**

Confirmed this is a real, currently-open gap, not already closed by STEP94: STEP94's own "Gap 6"
found manual markers have *"no rendering consumer of any kind"* through the real overlay pipeline.
STEP94 shipped a narrow workaround (plain `ImDrawList::AddCircleFilled` dots) to have something to
click for its own drag-gesture feature — that workaround does not touch `ResolveManualSubLayer` and
is not the Phase-5 pipeline consumer, so Gap 6 is still open. This ticket does not close it either;
it only makes `SeedMarkerDomains`' data correct so that whichever ticket does close Gap 6 (a
`ResolveMarkersManual` case in `MapCanvas_IconLayer_CullManual_UI.cpp`, matching the Units/Props/
Decals precedent already in that same switch) has the right `subLayers` to consume.

## Files touched
- `src/params/MarkerInstance_PARAMS.h` — add `Params::kSpawnMarkerGroupName`.
- `src/ui/MarkersTab_Manual_UI.h` — `:41` reference the promoted constant instead of a local
  duplicate; `:104-106` (`IsSpawnMarkerGroup`) unaffected in behavior.
- `src/io/MapImporter_ArmyIdentityNormalize_IO.cpp` — `:59` reference the promoted constant instead
  of the raw `"Spawn"` literal.
- `src/ui/Application_OverlaySetup_Seed_UI.cpp` — `SeedMarkerDomains`, `:25-37`, rewritten per Fix
  item 2.
- `src/ui/OverlayLayer_Settings_UI.h` — `:49-51` comment correction only (Fix item 3); no struct
  change.
- `src/ui/OverlayLayer_Settings_UI_Test.cpp` — new assertions (see Acceptance test below); existing
  assertions are unaffected — none of the existing fixtures populate the manual marker roster, so
  this fix produces zero refs against every currently-passing test's input and cannot regress them.

## ARCH rules invoked
- `ARCH_14_14_AlloySpawnsArmiesManualRouting.md` (§14.14) — the routing ruling this ticket
  implements verbatim.
- `ARCH_14_02_DataModel.md:38` — the sub-layer -> data mapping table row, already updated in place to
  match §14.14.
- `ARCH_14_PreviewOverlayLayering.md` §14.6 — "domain != DATA-bucket identity," the same asymmetry
  the render-time corollary (out of scope, item 4 above) will have to respect.
- Constitution §8 (tweakability) — not implicated; no new UI control, no new tunable.

## Explicit out-of-scope
- The render-time `ResolveMarkersManual` consumer in `MapCanvas_IconLayer_CullManual_UI.cpp`'s
  `ResolveManualSubLayer` switch (Fix item 4) — a separate follow-up ticket, per ARCH_14_14's own
  file list naming it as "not yet named/built."
- Any View-toolbar UI surfacing of the new Manual sub-layer count/labels for Alloy/SpawnsArmies rows
  — out of this sequence's Phase 4 scope, unaffected either way.
- STEP94's `ImDrawList` workaround draw path — untouched; it does not read `OverlayLayerSettings` at
  all and is not this ticket's concern.
- Re-deriving or second-guessing ARCH_14_14's routing ruling itself — implemented as ratified, not
  re-litigated here.

## Acceptance test
Extend `src/ui/OverlayLayer_Settings_UI_Test.cpp` with a new `RunMarkerManualPartitionChecks()`
(called from `main()` alongside the existing check functions), mirroring
`RunPropReclaimPartitionChecks()`'s fixture shape since it is the closest structural precedent, but
proving the "existence-checked, not unconditional-dual-push" behavior this ticket deliberately
differs on:
1. **Pure-Spawn layer:** `recipe.markerLayers` has one entry; `recipe.markers` has one
   `MarkerInstanceGroup{name="Spawn"}` with two `transforms`, both `layerIndex == 0`. Assert
   `spawnsArmiesLayer.subLayers == [{Manual, 0}]` and `alloyLayer.subLayers.empty()`.
2. **Pure-non-Spawn layer:** same shape, group `name = "Alloys"`. Assert the mirror image.
3. **Mixed layer (the case STEP97/ARCH_14_14 exists for):** one `markerLayers` entry, two groups
   (`"Spawn"` and `"Alloys"`) each contributing one transform at `layerIndex == 0`. Assert **both**
   `alloyLayer.subLayers == [{Manual, 0}]` and `spawnsArmiesLayer.subLayers == [{Manual, 0}]` — the
   entry legally appears in both lists.
4. **Untouched layer:** a `markerLayers` entry with index 1 that no transform references at all
   (every transform in the fixture uses `layerIndex == 0`). Assert neither domain gets a `{Manual, 1}`
   ref — proves the existence-check, not a blanket `PushManualRefs(markerLayerCount)`.
5. **Manual-before-Procedural ordering**, same recipe extended with one `markerRuleLayers` Spawn rule:
   assert `spawnsArmiesLayer.subLayers` is `[{Manual, 0}, {ProceduralRule, 0}]` in that order (STEP51's
   established Manual-first convention).
6. **Unchanged-domain guard:** Units/Props/Reclaim/Decals stay empty across every fixture above (none
   populate `armies`/`propRules`/`propLayers`/`decalRules`/`decalLayers`).
Full solo rebuild + `OverlayLayer_Settings_UI_Test` green, plus every other currently-green suite
under `src/ui/` untouched by this change.

## Verify
No performance estimate applies (Constitution §7) — `O(transforms + markerLayers +
markerRuleLayers)` seed-time scan, launch-time only, same posture STEP51/STEP83 already state for
their own seeding helpers. This ticket does not change anything drawn on screen yet (Fix item 4 is
explicitly out of scope) — acceptance is data-model-only, provable entirely through
`OverlayLayer_Settings_UI_Test`'s `main()` exit code.

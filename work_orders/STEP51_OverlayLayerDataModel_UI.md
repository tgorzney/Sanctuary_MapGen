# STEP51 — Overlay-layer data model: `OverlayLayer_UI` + the session-only `overlayLayers` container

**Layer:** UI. **Domain:** new `OverlayLayer_Settings_UI.h`, `Application_Defaults_UI.h`/
`Application_OverlaySetup_UI.cpp`, `Application_UI.h`. **Sequence:** Phase 2.1,
`work_orders/SEQUENCE_PreviewOverlayLayering.md` (line 33). No dependency on any other undone
work-order in that sequence.

**Partial dependency on a separate, in-progress thread — corrected after this ticket's first
draft.** A different work-order thread (`ARCH_16_MarkerLayerSymmetry.md` §16, `STEP66_MarkerRuleLayer_PARAMS.md`) renames
`Params::MapRecipe::markerRules` → `markerRuleLayers` (`std::vector<MarkerRuleLayer>`, each owning
its own `std::vector<MarkerRule> rules`). Only this ticket's `SeedMarkerDomains` (Alloy/SpawnsArmies
seeding) is affected — every other domain's seeding (Units/Props/Decals/Reclaim) is untouched by
that rename and may land independent of it. The fix below flattens over the new nested structure
under the same "flat/global index" assumption `STEP50` makes for its `markers` bucket index —
kept consistent between the two tickets. **CONFIRMED —
`work_orders/STEP79_MarkerRuleLayerProcConsumer_PROC.md`'s "⭐ Downstream authority ruling" section
verifies `ruleIndex` stays a flat, running index over the layer-concatenated rule sequence, and
states this ticket's `SeedMarkerDomains` (its flat-index loop over `recipe.markerRuleLayers`) is
"correct as written." No code change is needed here — the assumption previously flagged as
provisional is resolved.**

## Problem
`ARCH_14_PreviewOverlayLayering.md` §14 ratifies a six-domain screen-space overlay compositor (Alloy, Spawns/Armies, Units,
Props, Reclaim, Decals) that closes the "Props/Units/Decals never reach the canvas" gap. §14.2
pins the binding data-model shape (`OverlayLayer_UI`/`OverlayDomainKind_UI`/
`OverlaySubLayerKind_UI`/`OverlaySubLayerRef_UI`) that every later phase in the sequence — the
icon draw pass (Phase 3), the View toolbar (Phase 4) — is built against. None of these four types,
nor the `overlayLayers` container itself, exist anywhere in `src/` today (confirmed: no match for
`OverlayLayer_UI`/`OverlayDomainKind_UI` under `src/`). Building Phase 3/4 without this landing
first would force each of them to invent an ad-hoc shape and then retrofit — exactly the
"drift" class `ARCH.md` repeatedly rules against (e.g. `SpatialGrid_DATA.h`'s "a second copy...
is exactly how a picker drifts from its index," cited verbatim in `STEP47_WorldScreenProjection_UI.md`).

This work-order is data-model-only: the struct shape, its default-seeding against a real
`Params::MapRecipe`, and its session-only storage on `Application`. **No rendering consumer
exists yet** — `MapCanvas_IconLayer_UI.cpp` (§14.9) and the View toolbar (§14.7) are separately
scoped, later phases. Acceptance here is struct-level unit tests, not end-to-end rendering.

## Fix

### 1. The four ARCH-pinned types + two new session-only helper types, in a new sibling file
`ARCH_14_01_ModuleBoundaryDataVsParams.md` §14.1 states `OverlayLayer_UI`/`overlayLayers` is "`UI`, the same precedent as
`PreviewCompositeSettings::fieldLayers`" (`src/ui/PreviewComposite_Settings_UI.h`) — session-only
presentation, never recipe-serialized. But it is **not** a member of `PreviewCompositeSettings`
itself: that struct is owned by `PreviewComposite`, the GPU field/terrain compositor
(`src/ui/PreviewComposite_UI.h:118` `PreviewCompositeSettings settings;` member, wired through
`composite.Settings()`). The overlay stack is consumed by a different, still-unscheduled renderer
— the screen-space icon draw pass, `MapCanvas_IconLayer_UI.cpp` (§14.9) — and §14.7's View-toolbar
popup already treats "Overlays (screen-space)" as `overlayLayers` on its own, not
`PreviewCompositeSettings::overlayLayers`. **Decision (named explicitly, per the dispatching
agent's instruction): a new sibling settings file**, mirroring `PreviewComposite_Settings_UI.h`'s
own structure (header comment block stating the UI/session-only posture, plain data structs, no
logic) — not an addition to the existing file:

```cpp
// src/ui/OverlayLayer_Settings_UI.h
// OverlayLayer_Settings_UI.h — the six-domain screen-space overlay stack: what View-toolbar row
// draws which sub-layer, in what Z order, at what opacity (ARCH_14_02_DataModel.md §14.2). Layer: UI.
// Session-only presentation, the same precedent as `PreviewComposite_Settings_UI.h`'s
// `PreviewCompositeSettings::fieldLayers` (§14.1): never recipe-serialized, no PARAMS home. A
// SEPARATE settings object from `PreviewCompositeSettings` on purpose (see this work-order's
// Fix section item 1) — consumed by the screen-space icon draw pass
// (`MapCanvas_IconLayer_UI.cpp`, §14.9, unscheduled), not by `PreviewComposite`'s GPU
// field/terrain compositor.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

// The six confirmed dynamic overlay domains (ARCH_14_PreviewOverlayLayering.md §14 intro). Open/additive — a new domain is a
// new enumerator, never a reshuffled existing one (§14.2).
enum class OverlayDomainKind_UI { Alloy, SpawnsArmies, Units, Props, Reclaim, Decals };

// Manual = hand-authored pass-through data (`Params::MapRecipe`'s own arrays); ProceduralRule =
// a `recipe.*Rules[i]` scatter rule's resolved instances (§14.1).
enum class OverlaySubLayerKind_UI { Manual, ProceduralRule };

// `index` resolves differently per (domainKind, kind) pair — see the §14.2 mapping table and
// `Application_OverlaySetup_UI.cpp`'s seeding below for the per-domain resolution each pairing
// uses.
struct OverlaySubLayerRef_UI { OverlaySubLayerKind_UI kind; int index; bool bEnabled = true; };

// One row of the View toolbar's "Overlays (screen-space)" section. BINDING SHAPE (ARCH_14_02_DataModel.md §14.2) —
// do not add fields here; `color`/`iconScale` deliberately live elsewhere (§14.5, see
// `OverlaySessionAppearance` below).
struct OverlayLayer_UI {
    std::string name;
    OverlayDomainKind_UI domainKind = OverlayDomainKind_UI::Alloy;
    bool bEnabled = true;
    float opacity = 1.0f;                              // layer-wide alpha multiplier (§14.2/§14.8)
    std::vector<OverlaySubLayerRef_UI> subLayers;       // any mix/count of Manual + ProceduralRule
    float thumbnailLodThresholdPixels = 5.0f;           // §14.3, Constitution §8 tunable
};

// §14.5's "UI-session defaults" half: color/iconScale for a domain with no recipe-serialized
// layer-metadata record yet. Props/Decals do NOT use this — they read/write
// `Params::PropInstanceLayer`/`DecalInstanceLayer` directly through each Manual sub-layer's own
// `index` (no shadow copy, §14.5, see Fix item 3). One entry per such domain, not per sub-layer:
// Units' `Army.groups` carries no per-group appearance field to mirror, and Alloy/SpawnsArmies
// carry zero Manual sub-layers this sequence (blocked — `SEQUENCE_PreviewOverlayLayering.md`'s
// Phase 5 table, "Manual Alloy/SpawnsArmies sub-layers... outside this sequence").
struct OverlaySessionAppearance { float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; float iconScale = 1.0f; };

// The session container itself (§14.1's `overlayLayers: vector<OverlayLayer_UI>`), plus its
// three UI-session appearance slots (§14.5). Never serialized into `mapGeneratorData` — the same
// posture `PreviewCompositeSettings` already states for `fieldLayers`.
struct OverlayLayerSettings {
    std::vector<OverlayLayer_UI> overlayLayers;   // vector order = Z order (§14.2)
    OverlaySessionAppearance alloyAppearance;
    OverlaySessionAppearance spawnsArmiesAppearance;
    OverlaySessionAppearance unitsAppearance;
};

} // namespace Ui
} // namespace SanmapGen
```
Naming: the four ARCH-pinned types keep the `_UI` suffix ARCH itself gives them (§14.12); the two
new session-only helper types (`OverlaySessionAppearance`, `OverlayLayerSettings`) drop it,
matching the existing precedent that types *inside* a `_UI`-suffixed file do not each re-carry the
suffix (`PreviewCompositeSettings`, `PreviewFieldLayer` in `PreviewComposite_Settings_UI.h` carry
no `_UI` either — only the file does, per §1.2).

### 2. Default-seeding — one `OverlayLayer_UI` per domain, sub-layers per the §14.2 mapping table
New file `Application_OverlaySetup_UI.cpp`, declared in `Application_Defaults_UI.h` next to the
existing `ConfigureDefaultPreview` declaration (`Application_Defaults_UI.h:22-23`) — same
free-function-drivable-without-a-window posture that file's own header comment states
(`Application_Defaults_UI.h:5-6`). **One-shot launch default, not a live resync** — same posture
`ConfigureDefaultPreview` already has for `fieldLayers` (it runs once, in `Application`'s
constructor, `Application_UI.cpp:39-40`). Keeping `overlayLayers[*].subLayers` in step with a
recipe that grows mid-session (a new `PropInstanceLayer` added from the Props tab, say) is
explicitly OUT OF SCOPE here — deciding when/how to re-seed is for whichever future work-order
wires this into the live edit flow (Phase 3/4 of the sequence).

```cpp
// Application_Defaults_UI.h — add:
#include "OverlayLayer_Settings_UI.h"
...
void ConfigureDefaultOverlayLayers(OverlayLayerSettings& overlaySettings,
                                    const Params::MapRecipe& recipe);   // Application_OverlaySetup_UI.cpp
// The one flattening Units' Manual sub-layers need (§14.4): resolves a flat
// `OverlaySubLayerRef_UI::index` back to the owning army + top-level group. Army-major,
// group-minor — the SAME order `ConfigureDefaultOverlayLayers` seeds these refs in; do not
// re-derive a second flattening convention at a future call site (§8.3 "one copy" precedent,
// cited the same way in `STEP47_WorldScreenProjection_UI.md`).
bool ResolveUnitsManualSubLayer(const Params::MapRecipe& recipe, int flatSubLayerIndex,
                                 int& outArmyIndex, int& outGroupIndex);   // Application_OverlaySetup_UI.cpp
```

Per-domain seeding, grounded in §14.2's mapping table:
- **Alloy / SpawnsArmies** (one `MarkerRule_PARAMS.h` split, §14.6): flattened over
  `recipe.markerRuleLayers[j].rules[i]` (post-`STEP66` shape — see this ticket's header note),
  filtered by `category` (`MarkerRule_PARAMS.h:14,20`) — `MarkerCategory::Spawn` goes to
  SpawnsArmies, every other category (`Generic`/`Alloys`/`Expansion` — "Spawn vs. rest",
  §14.2's own wording) goes to Alloy. The `index` assigned to each `OverlaySubLayerRef_UI` is the
  flat running count across the layer-concatenated sequence, matching `STEP50`'s bucket-index
  numbering assumption. **Zero Manual sub-layers for either domain** — no
  `MarkerInstanceLayer` PARAMS type exists yet to split `recipe.markers`
  (`MarkerInstance_PARAMS.h:23-29`) the same way; confirmed out-of-scope-for-this-sequence by
  `SEQUENCE_PreviewOverlayLayering.md`'s Phase 5 table. (Note: a separate thread's `ARCH_16_01_NewParamsShapes.md` §16.1
  has since extended `MarkerInstanceLayer`'s eventual shape with a `symmetry` field — still doesn't
  exist in `src/` today, this bullet's "zero Manual sub-layers" conclusion is unaffected either way.)
  ⚠️ STALE CLAIM — verify against current ARCH_NN_*.md: `ARCH_14_02_DataModel.md` §14.2's
  sub-layer -> data mapping table now reads, for the Alloy/Spawns-Armies row: "was blocked —
  `Params::MarkerInstanceLayer` now exists (ARCH §16); this row's data mapping updates to match
  §16.1's `recipe.markerLayers[i]`/`recipe.markerRuleLayers[i].rules[j]` shape, superseding the
  placeholder 'single undifferentiated Manual bucket' text" — i.e. the currently-ratified ARCH text
  says `MarkerInstanceLayer` DOES now exist and that Alloy/SpawnsArmies SHOULD gain Manual sub-layers
  over `recipe.markerLayers[i]`. That directly contradicts this bullet's "no `MarkerInstanceLayer`
  PARAMS type exists yet" / "zero Manual sub-layers for either domain" conclusion and this ticket's
  `SeedMarkerDomains` implementation (which emits no Manual refs for Alloy/SpawnsArmies at all).
  Not silently resolved here — re-verify against the live ARCH_14_02_DataModel.md and
  ARCH_16_01_NewParamsShapes.md before treating this ticket's Alloy/SpawnsArmies seeding as current.
- **Units**: Manual = one ref per top-level `Army.groups[name]` (`Army_PARAMS.h:37-48`), flattened
  army-major/group-minor across `recipe.armies` (§14.4 — nested `UnitGroup.groups` never
  separately addressable); Procedural = one ref per `recipe.unitRules[i]`
  (`ScatterRule_PARAMS.h:66`). Push Manual refs first, then Procedural.
- **Props**: Manual = one ref per `recipe.propLayers[i]` (`PropInstanceLayer`,
  `PropInstance_PARAMS.h:30`); Procedural = one ref per `recipe.propRules[i]`
  (`ScatterRule_PARAMS.h:12`). Manual first, then Procedural.
- **Decals**: same shape as Props — `recipe.decalLayers[i]` (`DecalInstanceLayer`,
  `PropInstance_PARAMS.h:31`) then `recipe.decalRules[i]` (`ScatterRule_PARAMS.h:40`).
- **Reclaim**: zero sub-layers, always — "no data yet... no rule type yet; slot reserved, zero
  cost until it ships" (§14.2 table, verbatim). The layer itself is still seeded (so the View
  toolbar has a row to show once it lands) but stays inert.

`overlayLayers` vector order (= Z order, §14.2) on a fresh launch: Alloy, SpawnsArmies, Units,
Props, Reclaim, Decals — the same order the domain enum and the ARCH intro paragraph both list
them in. Reference implementation:

```cpp
// Application_OverlaySetup_UI.cpp
#include "Application_Defaults_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

void PushProceduralRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int ruleCount) {
    for (int index = 0; index < ruleCount; ++index)
        subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, index, true});
}

void PushManualRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int recordCount) {
    for (int index = 0; index < recordCount; ++index)
        subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, index, true});
}

// CONFIRMED (STEP79 "⭐ Downstream authority ruling"): flat/global index over the layer-concatenated
// rule sequence — see this ticket's header note and STEP50's matching, now-confirmed assumption.
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

void SeedUnitsManualSubLayers(OverlayLayer_UI& unitsLayer, const Params::MapRecipe& recipe) {
    int flatIndex = 0;
    for (const Params::Army& army : recipe.armies)
        for (std::size_t group = 0; group < army.groups.size(); ++group)
            unitsLayer.subLayers.push_back(
                OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, flatIndex++, true});
}

} // namespace

void ConfigureDefaultOverlayLayers(OverlayLayerSettings& overlaySettings,
                                    const Params::MapRecipe& recipe) {
    OverlayLayer_UI alloyLayer;        alloyLayer.name        = "Alloy";
    alloyLayer.domainKind                                     = OverlayDomainKind_UI::Alloy;
    OverlayLayer_UI spawnsArmiesLayer; spawnsArmiesLayer.name = "Spawns/Armies";
    spawnsArmiesLayer.domainKind                               = OverlayDomainKind_UI::SpawnsArmies;
    SeedMarkerDomains(alloyLayer, spawnsArmiesLayer, recipe);

    OverlayLayer_UI unitsLayer; unitsLayer.name = "Units";
    unitsLayer.domainKind = OverlayDomainKind_UI::Units;
    SeedUnitsManualSubLayers(unitsLayer, recipe);
    PushProceduralRefs(unitsLayer.subLayers, static_cast<int>(recipe.unitRules.size()));

    OverlayLayer_UI propsLayer; propsLayer.name = "Props";
    propsLayer.domainKind = OverlayDomainKind_UI::Props;
    PushManualRefs(propsLayer.subLayers, static_cast<int>(recipe.propLayers.size()));
    PushProceduralRefs(propsLayer.subLayers, static_cast<int>(recipe.propRules.size()));

    OverlayLayer_UI reclaimLayer; reclaimLayer.name = "Reclaim";
    reclaimLayer.domainKind = OverlayDomainKind_UI::Reclaim;   // stays empty — no data/rule yet

    OverlayLayer_UI decalsLayer; decalsLayer.name = "Decals";
    decalsLayer.domainKind = OverlayDomainKind_UI::Decals;
    PushManualRefs(decalsLayer.subLayers, static_cast<int>(recipe.decalLayers.size()));
    PushProceduralRefs(decalsLayer.subLayers, static_cast<int>(recipe.decalRules.size()));

    overlaySettings.overlayLayers = {alloyLayer, spawnsArmiesLayer, unitsLayer,
                                      propsLayer, reclaimLayer, decalsLayer};
}

bool ResolveUnitsManualSubLayer(const Params::MapRecipe& recipe, int flatSubLayerIndex,
                                 int& outArmyIndex, int& outGroupIndex) {
    outArmyIndex = -1;
    outGroupIndex = -1;
    if (flatSubLayerIndex < 0) return false;
    int remaining = flatSubLayerIndex;
    for (std::size_t army = 0; army < recipe.armies.size(); ++army) {
        const int groupCount = static_cast<int>(recipe.armies[army].groups.size());
        if (remaining < groupCount) {
            outArmyIndex  = static_cast<int>(army);
            outGroupIndex = remaining;
            return true;
        }
        remaining -= groupCount;
    }
    return false;
}

} // namespace Ui
} // namespace SanmapGen
```
⚠️ Watch the §1.5 100-line soft ceiling while formatting this — if it runs long, split
`SeedMarkerDomains`/`SeedUnitsManualSubLayers`/`PushProceduralRefs`/`PushManualRefs` into a
`Application_OverlaySetup_Seed_UI.cpp` aspect file behind the same declarations, same pattern
`Application_UI.h`'s own header comment already documents for its other aspect `.cpp` units
(`Application_UI.h:8-14`).

### 3. `color`/`iconScale` — no new fields, §14.5's split honored exactly
§14.2's own binding-shape comment says `color[4]`/`iconScale` are "intentionally NOT always here"
— i.e. not fields of `OverlayLayer_UI` at all. §14.5 splits by domain:
- **Props/Decals**: a Manual sub-layer's `index` already *is* the position in
  `recipe.propLayers`/`recipe.decalLayers` — a future consumer reads
  `recipe.propLayers[ref.index].color`/`.iconScale` directly (`PropInstance_PARAMS.h:30-31`). No
  new field anywhere; this work-order adds no accessor for it since there is no rendering
  consumer yet (Problem section) — recorded here so a later work-order does not invent a shadow
  copy "because the field wasn't there."
- **Alloy/SpawnsArmies/Units**: `OverlayLayerSettings::alloyAppearance` /
  `spawnsArmiesAppearance` / `unitsAppearance` (item 1 above) are the UI-session default this
  work-order provides. One record per domain, not per sub-layer — Alloy/SpawnsArmies carry no
  Manual sub-layers this sequence (item 2) and Units' `Army.groups` has no natural per-group
  appearance field to key a per-sub-layer record off of.

### 4. Storage: a new member on `Application`, not inside `PreviewComposite`
`OverlayLayerSettings` is NOT a candidate for `ApplicationHostedSettings`
(`Application_HostedSettings_UI.h`) — that struct is an explicit temporary parking lot for fields
that WILL move into `Params::MapRecipe` once a home exists (its own header comment,
`Application_HostedSettings_UI.h:1-2,13-14`). `overlayLayers` will never move there — §14.1 rules
it session-only, permanently. It gets its own member, next to `composite`/`canvas`
(`Application_UI.h:118,120`):

```cpp
// Application_UI.h — add near the other includes (line ~28):
#include "OverlayLayer_Settings_UI.h"
// public accessor, next to Canvas() (Application_UI.h:80):
OverlayLayerSettings& OverlaySettings() { return overlaySettings; }
// private member, after `canvas` (Application_UI.h:120):
OverlayLayerSettings overlaySettings;
```
Constructor wiring (`Application_UI.cpp`), right after the existing `ConfigureDefaultPreview` call
(`Application_UI.cpp:39-40`):
```cpp
ConfigureDefaultOverlayLayers(overlaySettings, recipe);
```
`recipe` is already a fully-constructed member at this point in the init list (`Application_UI.cpp:28`,
`recipe(MakeDefaultMapRecipe())`, ahead of `composite`/`previewDriver`) — same ordering guarantee
`ConfigureDefaultPreview`'s own call site already relies on.

## Files touched
- `src/ui/OverlayLayer_Settings_UI.h` — **new.** `OverlayDomainKind_UI`, `OverlaySubLayerKind_UI`,
  `OverlaySubLayerRef_UI`, `OverlayLayer_UI` (ARCH_14_02_DataModel.md §14.2 binding shape, verbatim), plus
  `OverlaySessionAppearance`/`OverlayLayerSettings` (this work-order's own session-storage
  design, §14.5).
- `src/ui/Application_OverlaySetup_UI.cpp` — **new.** `ConfigureDefaultOverlayLayers()`,
  `ResolveUnitsManualSubLayer()`.
- `src/ui/Application_Defaults_UI.h` — add the two new declarations + the new header include.
- `src/ui/Application_UI.h` — new include, `overlaySettings` member, `OverlaySettings()` accessor.
- `src/ui/Application_UI.cpp` — one new call, `ConfigureDefaultOverlayLayers(overlaySettings, recipe);`.
- `src/ui/OverlayLayer_Settings_UI_Test.cpp` — **new** unit test (below). Register in
  `CMakeLists.txt` via `add_sangen_test(OverlayLayer_Settings_UI_Test src/ui/OverlayLayer_Settings_UI_Test.cpp)`
  near the other `PreviewComposite_*_Test`/`MapCanvas_*_Test` registrations (`CMakeLists.txt:339-346`)
  — `src/ui/*.cpp` itself is already glob-collected (`CMakeLists.txt:148`), so the two new
  non-test `.cpp`/`.h` files need no CMake edit.

## Out of scope (explicit)
- Any rendering consumer (`MapCanvas_IconLayer_UI.cpp`, §14.9/Phase 3) and the View toolbar
  (§14.7/Phase 4) — this work-order ships the data model and its default seed only.
- Live resync of `overlayLayers[*].subLayers` as the recipe changes mid-session (item 2) —
  `ConfigureDefaultOverlayLayers` is launch-time-only, exactly like `ConfigureDefaultPreview`.
- Manual Alloy/SpawnsArmies sub-layers — blocked on a `MarkerInstanceLayer` PARAMS type that does
  not exist yet (`BRIEF_MarkersTabUI.md`/`BRIEF_MarkersTabUI_R2.md`), confirmed outside this
  sequence by `SEQUENCE_PreviewOverlayLayering.md`'s Phase 5 table.
- Any accessor that dereferences `recipe.propLayers[ref.index]`/`decalLayers[ref.index]` for
  color/iconScale (item 3) — no consumer exists yet to call it.
- Real footprint-size table, icon atlas pairing lookup — separate READY work-orders in the same
  sequence (Phase 2.2/2.3), not this one.

## Verify
Pure unit tests, no imgui frame / window / GL context — same posture `SlopeTab_UI_Test.cpp` and
`STEP47`'s new tests already have. New file `src/ui/OverlayLayer_Settings_UI_Test.cpp`,
`Check()`/`failureCount` harness (`SlopeTab_UI_Test.cpp:11-19` pattern):
1. **Struct defaults**: a default-constructed `OverlayLayer_UI` has `bEnabled == true`,
   `opacity == 1.0f`, `thumbnailLodThresholdPixels == 5.0f`, `subLayers.empty()`; a
   default-constructed `OverlaySubLayerRef_UI{kind, index}` has `bEnabled == true`.
2. **Default seeding against `MakeDefaultMapRecipe()`**
   (`Application_Recipe_UI.cpp:73-85` — one Spawn `markerRule`, one `propRule`, nothing else):
   - `overlaySettings.overlayLayers.size() == 6`, in order Alloy, SpawnsArmies, Units, Props,
     Reclaim, Decals, each `domainKind` matching its position.
   - SpawnsArmies has exactly one `{ProceduralRule, 0}` sub-layer; Alloy has zero (the only
     marker rule is category `Spawn`).
   - Props has exactly one `{ProceduralRule, 0}` sub-layer, zero `Manual` (no `propLayers` yet).
   - Units, Reclaim, Decals all have zero sub-layers (no `unitRules`/armies, no rule/data,
     no `decalRules`/`decalLayers`).
3. **Category split correctness**: build a recipe with one `markerRuleLayers` entry whose `rules`
   has three `MarkerRule`s of categories `{Spawn, Alloys, Generic}` (in that order, post-`STEP66`
   shape — see this ticket's header note); assert SpawnsArmies gets exactly `[{ProceduralRule, 0}]`
   and Alloy gets exactly `[{ProceduralRule, 1}, {ProceduralRule, 2}]` — filtered-but-order-preserving
   over the flattened sequence. Add a second case spanning two layers (2 rules in layer 0, 1 rule in
   layer 1) to prove the flat index continues across the layer boundary, not resetting per layer.
4. **Manual + Procedural ordering for Props/Decals/Units**: build a recipe with 2 `propLayers`
   and 3 `propRules`; assert `propsLayer.subLayers == [{Manual,0},{Manual,1},{ProceduralRule,0},
   {ProceduralRule,1},{ProceduralRule,2}]` (Manual first, per item 2's stated order). Same shape
   check for Decals.
5. **`ResolveUnitsManualSubLayer` round-trip**: build a recipe with three armies with group
   counts `{2, 0, 3}`. Assert flat indices `0,1` resolve to `(army 0, group 0/1)`, indices `2,3,4`
   resolve to `(army 2, group 0/1/2)` (army 1 contributes nothing), and that
   `SeedUnitsManualSubLayers`'s own emitted flat indices (via `ConfigureDefaultOverlayLayers` on
   the same recipe) agree with these resolutions for every entry. Assert index `5` and index `-1`
   both return `false` with `outArmyIndex`/`outGroupIndex` left at `-1`.
6. **Read/write correctness of the struct itself**: mutate a seeded `OverlayLayerSettings` in
   place — reorder `overlayLayers` (swap two entries), flip a `bEnabled`, change an `opacity`,
   toggle a `subLayers[i].bEnabled`, write then read back `alloyAppearance.color`/`.iconScale` —
   and assert every mutation is visible through the same accessor path with no aliasing between
   entries (e.g. mutating `overlayLayers[0]` does not perturb `overlayLayers[1]`).

No performance estimate applies (Constitution §7's benchmark-backed-estimate requirement is for
runtime-cost work; this is a compile-time struct + a handful of `O(ruleCount + groupCount)` loops
run once at launch, not a hot path). Build: full solo rebuild +
`ctest -C Debug -R OverlayLayer_Settings_UI_Test` green, plus the existing `ApplicationShell_*`
suite green with zero edits (the new member/constructor call must not change any existing
observable `Application` behavior).

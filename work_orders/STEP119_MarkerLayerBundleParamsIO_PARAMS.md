# STEP119 — `MarkerLayerBundle` PARAMS + IO (ARCH §19, Ticket A)

**Layer:** PARAMS, IO. **Domain:** `Params::MarkerLayerBundle` (new file), `MapRecipe::
markerLayerBundles`, `MarkerRuleLayer::parentBundleIdentifier`, `MarkerInstanceLayer::
parentBundleIdentifier`, the `MarkerLayerBundles` wire array (Correction 19), the two merged
`ParentBundleIdentifier` wire keys on `MarkersStack`/`MarkerGroups`. **Sequence:** no dependency on
other undone work-orders; unblocks Ticket B (the tab UI, `TreeListWidget_UI<T>`, separately drafted).

Ratifies `work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md` §2/§6 "Ticket A" per
`ARCH_19_MarkerLayerBundle.md` and its 12 subsections. **Every field/function name below is ARCH's
final ruling, not the design doc's working names** — `MarkerLayerGroup`→`MarkerLayerBundle`,
`parentGroupIdentifier`→`parentBundleIdentifier`, `CollectMarkerLayerGroupRecursive*`→
`CollectMarkerLayerBundleRecursive*`, `WouldReparentMarkerLayerGroupCreateCycle`→
`WouldReparentMarkerLayerBundleCreateCycle`. UI display label stays "Group" (ARCH §19.1) — cosmetic
only, out of scope for this PARAMS/IO ticket regardless.

## Problem
No Group-above-Layer container exists anywhere in the codebase today (confirmed: no `MarkerLayerBundle`/`MarkerLayerGroup`/`parentBundleIdentifier` match under `src/`). `Params::MapRecipe`
carries `markerRuleLayers` (`src/params/MarkerRule_PARAMS.h:77-86`) and `markerLayers`
(`src/params/MarkerInstance_PARAMS.h:23-50`) as two flat, unrelated arrays with no parent tier; the
Markers tab UI faithfully renders this flatness as five flat sibling sections
(`DESIGN_MarkerGroupLayerRestructure_R1.md`, "Already confirmed this session"). ARCH §19 ratifies
the new container type, its field spellings, its wire shape, and its module-boundary placement;
this ticket builds the PARAMS type, the two back-reference fields, the pure resolver functions, and
the IO round trip — no UI.

## Fix

### 1. New file — `src/params/MarkerLayerBundle_PARAMS.h`
Sibling of `MarkerRule_PARAMS.h`/`MarkerInstance_PARAMS.h` — parent of BOTH
(`ARCH_19_03_FieldSpellings.md`), so it does not live inside either.

```cpp
// MarkerLayerBundle_PARAMS.h — Params::MarkerLayerBundle: the Group-above-Layer container ARCH §19
// ratifies (ratifies work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md). Layer: PARAMS. New file,
// sibling of MarkerRule_PARAMS.h/MarkerInstance_PARAMS.h — parent of BOTH
// (ARCH_19_03_FieldSpellings.md), so it does not live inside either. UI display label stays "Group"
// (ARCH §19.1); the C++/wire type is spelled "Bundle" specifically to avoid a 4th collision with the
// word "Group" (MarkerInstanceGroup / the MarkerGroups wire array / the MarkersStack
// Group(MarkerRuleLayer)->Rule wrapper already use it for three different things).
//
// Pure resolvers below carry a Params::-typed parameter (the bundle table itself) -> PARAMS,
// hand-written per domain, per ARCH §19.8 (Props/Decals get their own independent twins later, NOT
// a shared template — ARCH §19.2's domain-touching-vs-pure-mechanics split), mirroring
// ResolvePropInstanceLayerId/ResolveDecalInstanceLayerId's per-domain-repeated shape
// (PropInstance_PARAMS.h:37-44).
#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include "MarkerInstance_PARAMS.h"
#include "MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Params {

// ARCH_19_03_FieldSpellings.md. Additive-only; MapRecipe::markerLayerBundles is a fresh vector.
// Wire array MarkerLayerBundles (Correction 19) spells its stable id `Identifier` in full from day
// one — does NOT repeat MarkerGroups' pre-§1.9 "Id" abbreviation defect (ARCH §1.9).
struct MarkerLayerBundle {
    int identifier             = -1;   // stable, survives reorder/delete — NOT this array's own
                                        // position (array order is not this array's identity,
                                        // unlike MarkerGroups/PropGroups/DecalGroups).
    std::string name;
    int parentBundleIdentifier = -1;   // -1 = root; enables Bundle-in-Bundle nesting.
    std::string markerTypeName;        // single-type scope, free-form string space, same as
                                        // MarkerInstanceGroup::name (e.g. "Alloy"), NOT MarkerCategory.
    int assemblyIdentifier     = -1;   // ARCH §19.5 — Assembly-references-Bundle hook. Inert until
                                        // the separate, still-unbuilt Assembly feature exists; no
                                        // Params::Assembly type exists yet to validate against.
};

// Internal to this header, shared by the two Collect* functions below — not itself part of the
// documented per-domain resolver family. Collects `rootBundleIdentifier` and every identifier
// reachable by descending parentBundleIdentifier child links, into `outIdentifiers` (root always
// first). Cycle-safe: an identifier already collected is never re-expanded, so a corrupt/cyclic
// table (before RepairCyclicMarkerLayerBundleParents has run, MapImporter_MarkerLayerBundle_IO.cpp)
// cannot loop forever — Constitution §6 defensive posture.
inline void CollectMarkerLayerBundleDescendantIdentifiers(int rootBundleIdentifier,
                                                           const std::vector<MarkerLayerBundle>& bundles,
                                                           std::vector<int>& outIdentifiers) {
    outIdentifiers.push_back(rootBundleIdentifier);
    for (std::size_t frontierIndex = 0; frontierIndex < outIdentifiers.size(); ++frontierIndex) {
        const int currentIdentifier = outIdentifiers[frontierIndex];
        for (const MarkerLayerBundle& bundle : bundles) {
            if (bundle.parentBundleIdentifier != currentIdentifier) continue;
            bool bAlreadyCollected = false;
            for (int collected : outIdentifiers) {
                if (collected == bundle.identifier) { bAlreadyCollected = true; break; }
            }
            if (!bAlreadyCollected) outIdentifiers.push_back(bundle.identifier);
        }
    }
}

// True when reparenting `candidateId` under `newParentId` would create a cycle (including
// `candidateId == newParentId` itself). Walks parentBundleIdentifier up from newParentId; mirrors
// WouldReparentCreateCycle's proposed shape for Params::Assembly (DESIGN_Assembly_R1.md §1),
// confirmed PARAMS-resident and NOT shared/templated with Assembly's own body by ARCH §19.8 (this
// one's signature carries a Params:: type). Bounded to bundles.size()+1 steps so an
// already-corrupt/cyclic table cannot hang the caller — used both to REFUSE a live reparent (Ticket
// B, UI) and to REPAIR a cyclic import (MapImporter_MarkerLayerBundle_IO.cpp, this ticket).
inline bool WouldReparentMarkerLayerBundleCreateCycle(int candidateId, int newParentId,
                                                       const std::vector<MarkerLayerBundle>& bundles) {
    if (candidateId == newParentId) return true;
    int walk = newParentId;
    std::size_t stepsRemaining = bundles.size() + 1;
    while (walk != -1 && stepsRemaining > 0) {
        if (walk == candidateId) return true;
        int nextWalk = -1;
        for (const MarkerLayerBundle& bundle : bundles) {
            if (bundle.identifier == walk) { nextWalk = bundle.parentBundleIdentifier; break; }
        }
        walk = nextWalk;
        --stepsRemaining;
    }
    return false;
}

// ARCH §19.9's WIDE enumeration — every MarkerRuleLayer/MarkerInstanceLayer index (Procedural AND
// Manual) organizationally nested under `bundleIdentifier`, direct or via a descendant Bundle. Feeds
// the tree widget's leaf-enumeration callback (Ticket B); deliberately does NOT filter out
// Procedural layers (they are legitimate tree members, just zero-member for Move/Rotate — see
// CollectMarkerLayerBundleRecursiveManualMembers below, the separate NARROW function).
inline void CollectMarkerLayerBundleRecursiveLayerIndices(int bundleIdentifier,
    const std::vector<MarkerLayerBundle>& bundles, const std::vector<MarkerRuleLayer>& ruleLayers,
    const std::vector<MarkerInstanceLayer>& instanceLayers, std::vector<int>& outRuleLayerIndices,
    std::vector<int>& outInstanceLayerIndices) {
    outRuleLayerIndices.clear();
    outInstanceLayerIndices.clear();
    std::vector<int> inScopeIdentifiers;
    CollectMarkerLayerBundleDescendantIdentifiers(bundleIdentifier, bundles, inScopeIdentifiers);
    for (std::size_t layerIndex = 0; layerIndex < ruleLayers.size(); ++layerIndex)
        for (int identifier : inScopeIdentifiers)
            if (ruleLayers[layerIndex].parentBundleIdentifier == identifier) {
                outRuleLayerIndices.push_back(static_cast<int>(layerIndex));
                break;
            }
    for (std::size_t layerIndex = 0; layerIndex < instanceLayers.size(); ++layerIndex)
        for (int identifier : inScopeIdentifiers)
            if (instanceLayers[layerIndex].parentBundleIdentifier == identifier) {
                outInstanceLayerIndices.push_back(static_cast<int>(layerIndex));
                break;
            }
}

// ARCH §19.9's NARROW enumeration — {markerInstanceGroupIndex, transformIndex} pairs for every
// MarkerTransform whose layerIndex resolves into a MarkerInstanceLayer organizationally nested
// under `bundleIdentifier`. MANUAL ONLY, deliberately excludes Procedural layers (Data::
// PlacementInstances has no cross-bake stable identity to hang a persisted tag on, ARCH §14.8 —
// same restriction, same reasoning, Assembly's own already-ratified AssemblyId scoping, one tier
// up, ARCH §19.9). This is the one function BOTH the tab-driven Move/Rotate Apply (Ticket B) and
// the future CollectAssemblyRecursiveMembership Bundle-walking extension (ARCH §19.5, NOT this
// ticket) call — see this ticket's Out-of-Scope note on why the §19.6 assemblyIdentifier-cutoff rule
// is NOT implemented in this function's own recursion (Params::Assembly does not exist yet).
inline std::vector<std::pair<int,int>> CollectMarkerLayerBundleRecursiveManualMembers(
    int bundleIdentifier, const std::vector<MarkerLayerBundle>& bundles,
    const std::vector<MarkerInstanceLayer>& instanceLayers,
    const std::vector<MarkerInstanceGroup>& markers) {
    std::vector<std::pair<int,int>> outMembers;
    std::vector<int> inScopeIdentifiers;
    CollectMarkerLayerBundleDescendantIdentifiers(bundleIdentifier, bundles, inScopeIdentifiers);
    std::vector<int> inScopeInstanceLayerIndices;
    for (std::size_t layerIndex = 0; layerIndex < instanceLayers.size(); ++layerIndex)
        for (int identifier : inScopeIdentifiers)
            if (instanceLayers[layerIndex].parentBundleIdentifier == identifier) {
                inScopeInstanceLayerIndices.push_back(static_cast<int>(layerIndex));
                break;
            }
    for (std::size_t groupIndex = 0; groupIndex < markers.size(); ++groupIndex) {
        const MarkerInstanceGroup& group = markers[groupIndex];
        for (std::size_t transformIndex = 0; transformIndex < group.transforms.size(); ++transformIndex) {
            const int transformLayerIndex = group.transforms[transformIndex].layerIndex;
            for (int inScopeLayerIndex : inScopeInstanceLayerIndices)
                if (transformLayerIndex == inScopeLayerIndex) {
                    outMembers.emplace_back(static_cast<int>(groupIndex), static_cast<int>(transformIndex));
                    break;
                }
        }
    }
    return outMembers;
}

} // namespace Params
} // namespace SanmapGen
```
**Line-count risk, flagged not dictated**: this file lands close to ARCH §1.5's soft-100/hard-150
line ceiling once the header comments above are included. If the Coder's actual line count exceeds
150, split the two `Collect*` functions (and their shared `CollectMarkerLayerBundleDescendant
Identifiers` helper) into a companion `MarkerLayerBundleQuery_PARAMS.h`, mirroring how
`MapImporter_MarkerLayerReconcile_IO.cpp` was split out of `MapImporter_Markers_IO.cpp` for the
identical reason (STEP115) — a documented work-order exception either way (Constitution §7), the
Coder's call which shape to take.

### 2. Back-reference field — `MarkerRuleLayer` (`src/params/MarkerRule_PARAMS.h:77-86`)
Insert after the existing `SymmetrySetting symmetry;` (line 84), before `std::vector<MarkerRule>
rules;` (line 85):
```cpp
    int parentBundleIdentifier = -1;   // ARCH §19.3/§19.4 — -1 = root (ungrouped). Additive.
```

### 3. Back-reference field — `MarkerInstanceLayer` (`src/params/MarkerInstance_PARAMS.h:23-50`)
Insert after the existing `bool bColorOverrideEnabled = false;` (line 42, comment runs to line 49),
before the closing `};` (line 50):
```cpp
    int parentBundleIdentifier = -1;   // ARCH §19.3/§19.4 — -1 = root (ungrouped). Additive.
```

### 4. `MapRecipe` — new include + field (`src/params/MapRecipe_PARAMS.h`)
Include, alphabetical between `MarkerInstance_PARAMS.h` (line 20) and `MarkerRule_PARAMS.h`
(line 21):
```cpp
#include "MarkerLayerBundle_PARAMS.h"
```
Field, after the existing `std::vector<MarkerInstanceLayer> markerLayers;` (line 115), before the
`Scenarios` comment (line 116):
```cpp
    std::vector<MarkerLayerBundle> markerLayerBundles;   // ARCH §19, Correction 19. Additive.
```

### 5. Exporter — merged `ParentBundleIdentifier` on `MarkersStack`/`MarkerGroups`
`BuildMarkerRuleLayerJson` (`src/io/MapExporter_MarkersStack_IO.cpp:54-67`) — insert after the
existing `json["RadialSymmetryRepeatCount"] = layer.symmetry.radialSymmetryRepeatCount;` (line 61),
before the `Rules` array build (line 62):
```cpp
    json["ParentBundleIdentifier"] = layer.parentBundleIdentifier;
```
`BuildMarkerGroupsJson` (`src/io/MapExporter_Markers_IO.cpp:65-84`) — insert after the existing
`layerJson["ColorOverrideEnabled"] = layer.bColorOverrideEnabled;` (line 80), before
`markerGroups.push_back(layerJson);` (line 81):
```cpp
    layerJson["ParentBundleIdentifier"] = layer.parentBundleIdentifier;
```

### 6. Exporter — new `BuildMarkerLayerBundlesJson`
Append to `src/io/MapExporter_Markers_IO.cpp`, after `BuildMarkerGroupsJson` (line 84), before the
closing `} // namespace Io` (line 86). Needs `#include "../params/MarkerLayerBundle_PARAMS.h"`
added to this file's own include block (already includes `"../params/MapRecipe_PARAMS.h"`, which
transitively brings the type in via §4's new include — add the direct include anyway for clarity,
matching this codebase's explicit-include convention):
```cpp
// `MarkerLayerBundles` — SanGen-owned Group-above-Layer container, top-level PascalCase array
// (ARCH §19, Correction 19), a fresh sibling of `MarkerGroups`/`markers`. Array order is NOT this
// array's identity (unlike MarkerGroups/PropGroups/DecalGroups) — membership/nesting resolve by
// `Identifier`, since a Bundle forest can be reordered/reparented independently of array position.
nlohmann::ordered_json BuildMarkerLayerBundlesJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markerLayerBundles = nlohmann::ordered_json::array();
    for (const Params::MarkerLayerBundle& bundle : recipe.markerLayerBundles) {
        nlohmann::ordered_json bundleJson;
        bundleJson["Identifier"] = bundle.identifier;
        bundleJson["Name"] = bundle.name;
        bundleJson["ParentBundleIdentifier"] = bundle.parentBundleIdentifier;
        bundleJson["MarkerTypeName"] = bundle.markerTypeName;
        bundleJson["AssemblyIdentifier"] = bundle.assemblyIdentifier;
        markerLayerBundles.push_back(bundleJson);
    }
    return markerLayerBundles;
}
```
Declare in `src/io/MapExporter_Recipe_IO.h`, after the existing `BuildMarkerGroupsJson` declaration
(line 71):
```cpp
nlohmann::ordered_json BuildMarkerLayerBundlesJson(const Params::MapRecipe& recipe);
```
Call site — `src/io/MapExporter_DocumentAssembly_IO.cpp`, in `AppendEntityDomainsJson`, after the
existing `document["MarkerGroups"] = BuildMarkerGroupsJson(recipe);` (line 66):
```cpp
    document["MarkerLayerBundles"] = BuildMarkerLayerBundlesJson(recipe);
```

### 7. Importer — merged `ParentBundleIdentifier` on `MarkersStack`/`MarkerGroups`
`ReadMarkerRuleLayerJson` (`src/io/MapImporter_MarkersStack_IO.cpp:55-65`) — insert after the
existing `ReadJsonIntegerClamped(json, "RadialSymmetryRepeatCount", ...)` (lines 61-63), before
`ReadRuleArray(json, "Rules", layer.rules, ReadMarkerRuleJson);` (line 64):
```cpp
    ReadJsonInteger(json, "ParentBundleIdentifier", layer.parentBundleIdentifier);
```
`ReadMarkerGroupsJson` (`src/io/MapImporter_Markers_IO.cpp:116-145`) — insert after the existing
`ReadJsonBoolean(layerJson, "ColorOverrideEnabled", layer.bColorOverrideEnabled);` (line 141),
before the closing brace of the `if (layerJson.is_object())` block (line 142):
```cpp
            ReadJsonInteger(layerJson, "ParentBundleIdentifier", layer.parentBundleIdentifier);
```
Both new fields need **no range validation** (ARCH §19.4: a dangling reference is a query-time
miss, not a structural error — same posture already ruled for `symmetryGroupIdentifier`) — an
absent key on a pre-Bundle export keeps the struct default (`-1`, root).

### 8. New file — `src/io/MapImporter_MarkerLayerBundle_IO.cpp`
Own file, not folded into `MapImporter_Markers_IO.cpp` (already 161 lines — over the ARCH §1.5 hard
ceiling with this addition), mirroring `MapImporter_MarkerLayerReconcile_IO.cpp`'s own split
(STEP115) and header-comment style:
```cpp
// MapImporter_MarkerLayerBundle_IO.cpp — the top-level `MarkerLayerBundles` array ->
// `recipe.markerLayerBundles` (ARCH §19, Correction 19). Own file, not folded into
// MapImporter_Markers_IO.cpp (already at the ARCH §1.5 line-count ceiling) — mirrors the
// MapImporter_ParseDocument_IO.cpp-out-of-MapImporter_IO.cpp (STEP35) / MapImporter_
// MarkerLayerReconcile_IO.cpp-out-of-MapImporter_Markers_IO.cpp (STEP115) split precedent.
//
// No range/clamp validation on `ParentBundleIdentifier`/`AssemblyIdentifier` (ARCH §19.4 — a
// dangling reference is a query-time miss, not a structural error, same posture as
// symmetryGroupIdentifier). The one thing THIS file does validate: a cyclic ParentBundleIdentifier
// chain, logged and treated as root, never a refusal (ARCH §19.12, Assembly's own already-decided
// convention restated at the Bundle tier).
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void PopulateMarkerLayerBundlesFromJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("MarkerLayerBundles") || !document["MarkerLayerBundles"].is_array()) return;
    outRecipe.markerLayerBundles.clear();
    for (const nlohmann::json& bundleJson : document["MarkerLayerBundles"]) {
        Params::MarkerLayerBundle bundle;
        if (bundleJson.is_object()) {
            ReadJsonInteger(bundleJson, "Identifier", bundle.identifier);
            ReadJsonText(bundleJson, "Name", bundle.name);
            ReadJsonInteger(bundleJson, "ParentBundleIdentifier", bundle.parentBundleIdentifier);
            ReadJsonText(bundleJson, "MarkerTypeName", bundle.markerTypeName);
            ReadJsonInteger(bundleJson, "AssemblyIdentifier", bundle.assemblyIdentifier);
        }
        outRecipe.markerLayerBundles.push_back(bundle);
    }
}

// ARCH §19.12: a cyclic ParentBundleIdentifier chain is logged and treated as root, never a
// refusal — same convention as Assembly's own already-ratified cycle-on-import rule, applied here
// at the Bundle tier. Runs AFTER the whole table is populated: WouldReparentMarkerLayerBundle
// CreateCycle needs every entry present to walk the chain.
void RepairCyclicMarkerLayerBundleParents(std::vector<Params::MarkerLayerBundle>& bundles,
                                          MapImportResult& result) {
    for (Params::MarkerLayerBundle& bundle : bundles) {
        if (bundle.parentBundleIdentifier == -1) continue;
        if (!Params::WouldReparentMarkerLayerBundleCreateCycle(bundle.identifier,
                                                               bundle.parentBundleIdentifier, bundles))
            continue;
        result.Warn("MarkerLayerBundle \"" + bundle.name + "\" (Identifier "
                   + std::to_string(bundle.identifier)
                   + ") has a cyclic ParentBundleIdentifier chain; treated as root.");
        bundle.parentBundleIdentifier = -1;
    }
}

} // namespace

void ReadMarkerLayerBundlesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                                MapImportResult& result) {
    PopulateMarkerLayerBundlesFromJson(document, outRecipe);
    RepairCyclicMarkerLayerBundleParents(outRecipe.markerLayerBundles, result);
}

} // namespace Io
} // namespace SanmapGen
```
Declare in `src/io/MapImporter_Recipe_IO.h`, after the existing `ReadChainsJson` declaration
(line 70), before the Props/Decals block comment (line 72):
```cpp
// MapImporter_MarkerLayerBundle_IO.cpp — `MarkerLayerBundles` -> `recipe.markerLayerBundles`
// (ARCH §19, Correction 19). No load-bearing ordering relative to ReadMarkerGroupsJson/
// ReadMarkersJson/ReadMarkersStackJson — parentBundleIdentifier has no range to validate (§19.4),
// so nothing downstream depends on markerLayerBundles.size() the way layerIndex's clamp does.
void ReadMarkerLayerBundlesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                                MapImportResult& result);
```
Call site — `src/io/MapImporter_ParseDocument_IO.cpp`, in `ParseEntityDomainsJson`, after the
existing `ReadMarkerGroupsJson(document, outRecipe);` (line 74), before `ReadMarkersJson(document,
outRecipe, result);` (line 75):
```cpp
    ReadMarkerLayerBundlesJson(document, outRecipe, result);
```

### 9. Known-top-level-key allowlist — `src/io/Sanmap_KnownTopLevelKeys_IO.cpp:29`
**Load-bearing, not optional** — `CheckKnownTopLevelSanmapKeysCoverage`
(`src/io/MapImporter_IO_Test.cpp:1991-2001`) asserts every key `BuildSanmapJsonText` writes is
present in this allowlist; skipping this edit fails that EXISTING test the moment step 6's
unconditional `document["MarkerLayerBundles"] = ...` write lands (`BuildMarkerLayerBundlesJson`
always writes the key, even as an empty array, same posture as `BuildMarkerGroupsJson`). Add to the
existing literal on line 29:
```cpp
        "areas", "armies", "markers", "MarkerGroups", "MarkerLayerBundles", "chains",
```

### 10. §19.6's assemblyIdentifier-cutoff rule — explicitly NOT implemented here
`ARCH_19_06_NestedBundleAssemblyCutoff.md` rules that a nested child Bundle tagged to a *different*
Assembly stops a recursive walk at that child. This ticket's `CollectMarkerLayerBundleRecursive
ManualMembers` (§1 above) does **not** implement that cutoff — it walks every descendant Bundle
unconditionally, regardless of `assemblyIdentifier`. This is deliberate, not an oversight: `Params::
Assembly` does not exist in this codebase yet (confirmed, no ratified/built Assembly ticket), so
there is nothing real to gate against, and §19.5's own "Sequencing note" explicitly defers the
`CollectAssemblyRecursiveMembership` Bundle-walking extension to a later ticket that depends on
Assembly's own ticket existing. Tab-driven Move/Rotate (Ticket B, `ARCH_19_10_TabDrivenV1Scoping.md`)
has no Assembly concept to respect either. When Assembly's own future ticket lands, it must
implement the §19.6 cutoff in its own walk logic (calling this function only per-subtree up to a
cutoff, not once over an entire tree) — flagged here so it is not silently forgotten, not fixed now.

## Out of scope
- **All UI** — no `src/ui/*` file is touched. Ticket B (separate, drafted in parallel) owns the tab
  restructure, the toolbar, tab-driven Move/Rotate, and the soft type-consistency "Add Marker"
  enforcement (ARCH §19.12).
- **`TreeListWidget_UI<T>`** — Ticket B's job (ARCH §19.7).
- **`CollectAssemblyRecursiveMembership`'s Bundle-table-walking extension** — depends on Assembly's
  own still-unbuilt ticket existing (ARCH §19.5's own sequencing note); not this ticket.
- **The §19.6 assemblyIdentifier-cutoff rule inside this ticket's own recursive-membership
  function** — see §10 above; explicitly deferred to Assembly's future ticket.
- **Props/Decals' own `PropLayerBundle`/`DecalLayerBundle` twins** — later, independently ticketed
  (ARCH §19.2).
- **The rigid rotate/translate-around-centroid MATH function** (ARCH §19.8's third bullet) — that is
  a MATH-layer function with zero `Params::` type in its signature, a separate ticket from this
  PARAMS/IO one; not built here.
- **Deletion/promote-don't-cascade logic** — a UI-triggered operation (Ticket B); this ticket adds no
  delete/promote code, only the cycle-detection primitive Ticket B's delete flow will call.
- **The pre-existing `layerIndex` export bug** (`BuildMarkerTransformJson` never writes
  `layerIndex`, `src/io/MapExporter_Markers_IO.cpp:17-39`, confirmed still live by direct read).
  None of this ticket's edits land inside `BuildMarkerTransformJson` — the new `ParentBundleIdentifier`
  field lives on the Layer types (`MarkerRuleLayer`/`MarkerInstanceLayer`), not on `MarkerTransform`,
  so there is no natural "touching this exact function" opportunity to opportunistically fix it.
  Already recorded at `ARCH_19_11_FormatSpecCorrectionBundle.md` item 4 and in
  `SANMAP_FORMAT_SPEC.md`'s own "Conversion / import-export logic" section — flag as its own
  separate ticket to file, not fixed here.
- **`Params::Assembly` itself, `assemblyIdentifier`'s validation against it.** Inert field only.

## Files touched
- `src/params/MarkerLayerBundle_PARAMS.h` — **new file**: `MarkerLayerBundle` struct,
  `WouldReparentMarkerLayerBundleCreateCycle`, `CollectMarkerLayerBundleRecursiveLayerIndices`,
  `CollectMarkerLayerBundleRecursiveManualMembers`, the shared internal
  `CollectMarkerLayerBundleDescendantIdentifiers` helper
- `src/params/MarkerRule_PARAMS.h` — `MarkerRuleLayer` gains `parentBundleIdentifier`
- `src/params/MarkerInstance_PARAMS.h` — `MarkerInstanceLayer` gains `parentBundleIdentifier`
- `src/params/MapRecipe_PARAMS.h` — new include, `markerLayerBundles` field
- `src/io/MapExporter_Markers_IO.cpp` — new `BuildMarkerLayerBundlesJson`; `BuildMarkerGroupsJson`
  writes `"ParentBundleIdentifier"`; new include
- `src/io/MapExporter_MarkersStack_IO.cpp` — `BuildMarkerRuleLayerJson` writes
  `"ParentBundleIdentifier"`
- `src/io/MapExporter_Recipe_IO.h` — declares `BuildMarkerLayerBundlesJson`
- `src/io/MapExporter_DocumentAssembly_IO.cpp` — `AppendEntityDomainsJson` writes
  `document["MarkerLayerBundles"]`
- `src/io/MapImporter_MarkerLayerBundle_IO.cpp` — **new file**: `ReadMarkerLayerBundlesJson`,
  `PopulateMarkerLayerBundlesFromJson`, `RepairCyclicMarkerLayerBundleParents`
- `src/io/MapImporter_Markers_IO.cpp` — `ReadMarkerGroupsJson` reads `"ParentBundleIdentifier"`
- `src/io/MapImporter_MarkersStack_IO.cpp` — `ReadMarkerRuleLayerJson` reads
  `"ParentBundleIdentifier"`
- `src/io/MapImporter_Recipe_IO.h` — declares `ReadMarkerLayerBundlesJson`
- `src/io/MapImporter_ParseDocument_IO.cpp` — `ParseEntityDomainsJson` calls
  `ReadMarkerLayerBundlesJson`
- `src/io/Sanmap_KnownTopLevelKeys_IO.cpp` — allowlist gains `"MarkerLayerBundles"` (line 29;
  **required**, not optional — see §9)
- `src/io/MapImporter_IO_Test.cpp` — extends `FillFixtureMarkersAndChains`/`CheckMarkersAndChains`,
  `CheckMarkerRuleLayerTwoLevelRoundTrip`; new test functions (see Verify)
- `src/params/MarkerLayerBundle_PARAMS_Test.cpp` — **new file**, pure resolver unit tests
- `CMakeLists.txt` — one new line, `add_sangen_test(MarkerLayerBundle_PARAMS_Test
  src/params/MarkerLayerBundle_PARAMS_Test.cpp)`, inserted alphabetically between
  `MapRecipe_PARAMS_Test` (line 420) and `PlacementRules_PARAMS_Test` (line 421) in the `# PARAMS`
  block. (New `.cpp`s under `src/` are otherwise auto-discovered by `SANGEN_V2_SOURCES`'s
  `GLOB_RECURSE CONFIGURE_DEPENDS`, `CMakeLists.txt:174` — only this explicit test-binary
  registration needs a manual edit, mirroring every other `*_PARAMS_Test` entry.)

## Verify
Acceptance bar: `MarkerLayerBundle` round-trips through export/import including legacy files with
none of the new keys present; both merged `ParentBundleIdentifier` fields round-trip and default
correctly; cycle-on-import is detected and repaired, loud, non-fatal; the two recursive resolvers
and the cycle predicate are covered by direct unit tests; the existing known-top-level-key coverage
test stays green.

- **Extend `FillFixtureMarkersAndChains`/`CheckMarkersAndChains`** (`MapImporter_IO_Test.cpp:1201-
  1239`/`655-729`): set `markerLayer.parentBundleIdentifier = 42;` (non-default) in the fixture;
  assert `loadedLayer.parentBundleIdentifier == originalLayer.parentBundleIdentifier` in the check.
  Push a `Params::MarkerLayerBundle` (non-default `identifier`, `name`, `parentBundleIdentifier`,
  `markerTypeName`, `assemblyIdentifier`) onto `recipe.markerLayerBundles` in the fixture; assert
  `loaded.markerLayerBundles.size() == 1` and every field survives in the check — this exercises the
  full `BuildSanmapJsonText`/`ParseSanmapJsonText` path via `RunRoundTripTests`, including the
  no-warning assertion that path makes (a non-cyclic `parentBundleIdentifier = -1` on the fixture's
  one bundle must not trip `RepairCyclicMarkerLayerBundleParents`).
- **Extend `CheckMarkerRuleLayerTwoLevelRoundTrip`** (`MapImporter_IO_Test.cpp:363-435`): set
  `layerOne.parentBundleIdentifier = 5;` and leave `layerTwo.parentBundleIdentifier` at its `-1`
  default; assert both survive exactly through `BuildMarkersStackJson`/`ReadMarkersStackJson` in the
  existing per-layer comparison loop.
- **New unit test — `MarkerLayerBundles` legacy default**: hand-build a `MarkerLayerBundles` JSON
  array entry with none of `"Identifier"`/`"Name"`/`"ParentBundleIdentifier"`/`"MarkerTypeName"`/
  `"AssemblyIdentifier"` present, call `Io::ReadMarkerLayerBundlesJson` directly, assert struct
  defaults (`identifier == -1`, `name.empty()`, `parentBundleIdentifier == -1`,
  `markerTypeName.empty()`, `assemblyIdentifier == -1`), mirroring
  `CheckMarkerGroupsLegacyLockAndSnapDefaults`'s exact shape (`MapImporter_IO_Test.cpp:2025-2043`).
- **New unit test — merged `ParentBundleIdentifier` legacy default**: hand-build a `MarkerGroups`
  entry with no `"ParentBundleIdentifier"` key, call `Io::ReadMarkerGroupsJson` directly, assert
  `layer.parentBundleIdentifier == -1`; same for a `MarkersStack` entry via `Io::ReadMarkersStackJson`
  and `layer.parentBundleIdentifier` on the resulting `MarkerRuleLayer`.
- **New unit test — cycle repair on import (the 2-cycle case)**: hand-build a `MarkerLayerBundles`
  array with two entries, `{Identifier: 1, ParentBundleIdentifier: 2}` and `{Identifier: 2,
  ParentBundleIdentifier: 1}`; call `Io::ReadMarkerLayerBundlesJson(document, recipe, result)`
  directly; assert both entries' `parentBundleIdentifier == -1` after the call, and
  `result.warningCount == 2` (one Warn per cyclic entry — mirrors the direct-call/`result.
  warningCount` assertion style `CheckMarkerLayerSynthesisOnEmptyMarkerGroups` already uses,
  `MapImporter_IO_Test.cpp:2093-2128`).
- **New unit test — cycle repair is a no-op on a valid chain**: hand-build a valid 3-level
  root/child/grandchild chain (`{1,-1}`, `{2,1}`, `{3,2}`), call `ReadMarkerLayerBundlesJson`, assert
  all three `parentBundleIdentifier` values are unchanged and `result.warningCount == 0`.
- **New unit test — `Sanmap_KnownTopLevelKeys_IO.cpp` regression**: no new test needed beyond
  confirming the existing `CheckKnownTopLevelSanmapKeysCoverage` (`MapImporter_IO_Test.cpp:1991-2001`)
  stays green post-edit — it already iterates every key `BuildSanmapJsonText` writes, so it fails
  loud if §9's allowlist edit is skipped.
- **New file — `src/params/MarkerLayerBundle_PARAMS_Test.cpp`**, mirroring
  `GlobalMarkerSettings_PARAMS_Test.cpp`'s exact `Check`/`failureCount`/`main()` style (header
  compiles standalone, no JSON, no CMake link beyond the default):
  - `WouldReparentMarkerLayerBundleCreateCycle`: `candidateId == newParentId` (self-parent) returns
    `true`; a simple valid reparent (candidate not anywhere on the new parent's chain) returns
    `false`; a 3-level chain where `candidateId` sits two levels up from `newParentId` returns
    `true`; an already-corrupt 3-entry mutual-cycle table (`{1,2}`,`{2,3}`,`{3,1}`) queried for an
    unrelated candidate/parent pair returns without hanging (the test completing and printing its
    result at all is the proof — no explicit timeout needed, given the bounded-step-count design).
  - `CollectMarkerLayerBundleRecursiveLayerIndices`: build a bundle tree — root (`identifier=1,
    parentBundleIdentifier=-1`), child (`2`, parent `1`), grandchild (`3`, parent `2`), an unrelated
    sibling root (`4`, parent `-1`); build `ruleLayers`/`instanceLayers` with entries whose
    `parentBundleIdentifier` values are `1`, `2`, `3`, `4`, and `-1` (ungrouped) respectively; call
    the function with `bundleIdentifier = 1`; assert the returned index sets include exactly the
    layers parented at `1`/`2`/`3` and exclude the ones parented at `4` and `-1`.
  - `CollectMarkerLayerBundleRecursiveManualMembers`: same bundle tree; build one
    `MarkerInstanceLayer` per in-scope/out-of-scope identifier above; build a `MarkerInstanceGroup`
    whose `transforms` carry `layerIndex` values spanning both in-scope and out-of-scope layers;
    assert the returned `{groupIndex, transformIndex}` set contains exactly the in-scope transforms.
    Confirm (by the function's own signature, no `ruleLayers` parameter) that a Procedural-only
    layer structurally cannot appear in this result — the two-function split (ARCH §19.9) is
    correct by construction, not by an extra runtime filter.
- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `MapExporter_IO_Test`, `MapImporter_IO_Test` (every non-Bundle fixture/check byte-identical),
  `MapImporter_PropsDecals_IO_Test`, `MapImporter_Scenarios_IO_Test`, and every other IO test binary
  registered in `CMakeLists.txt` that touches `MapRecipe`/`BuildSanmapJsonText`/
  `ParseSanmapJsonText` — a brand-new additive field/array must not change any existing assertion's
  outcome.

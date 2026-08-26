# STEP125 — Markers tab Type-section outer loop (ARCH §19.14/§19.15, Ticket B)

**Layer:** UI (Markers tab). **Domain:** new `MarkersTab_TypeSections_UI.h/.cpp`; restructures
`MarkersTab_Bundles_UI.h/.cpp`, `MarkersTab_RuleLayers_UI.h/.cpp`,
`MarkersTab_RuleLayerSettings_UI.cpp`, `MarkersTab_ManualLayers_UI.h/.cpp`, `MarkersTab_UI.h/.cpp`.
**Sequence:** depends on STEP124 (landed) for `markerTypeName` on `Params::MarkerRuleLayer`/
`Params::MarkerInstanceLayer`; `Params::MarkerLayerBundle::markerTypeName` already existed from
STEP119. Ticket C (instance list + selection highlight) is disjoint and unblocked either way, per
`DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s own delivery-split recommendation.

Ratifies `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s Ticket B scope (Open Q1, Item 3,
Open Q6/Item 1&4) per `ARCH_19_14_TypeSectionUiDerived.md` and `ARCH_19_15_TypeSectionTreeComposition.md`,
both ratified "as designed." Every ARCH-cited rule below is quoted or paraphrased from those two
files; every other design decision below (the composition fixes needed to make the ratified pieces
actually work together in one frame — block-wide-settings placement, the tab-wide-singleton
Add-Rule/Remove-Rule controls, the Unassigned-bucket-always-present bootstrap rule, the ImGui ID
salting requirement) is this ticket's own call, flagged as such, not a repeated ARCH citation.

## Problem

STEP120 built one flat, unconditional "Groups" section (the Bundle tree) plus two flat "Ungrouped
Procedural Rules" / "Ungrouped Manual Marker Layers" sections, confirmed still true by direct read
of the live `DrawMarkersTab` (`src/ui/MarkersTab_UI.cpp:40-67`):
```cpp
DrawMarkerLayerBundleTree(recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers,
                          recipe.markers, ..., state.bundles, state, previewDriver, iconManifest);
DrawRuleStack(recipe, state, previewDriver, iconManifest);              // "Ungrouped Procedural Rules"
DrawManualMarkerLayers(state.manualLayers, recipe.markerLayers, ...);   // "Ungrouped Manual Marker Layers"
```
STEP124 landed `markerTypeName` (free-form `std::string`, default `""`) on both leaf-layer types,
completing the three-way union `ARCH_19_14` requires, but nothing in the UI reads it yet — every
Bundle/Layer still renders in the same single global list regardless of its own `markerTypeName`.
Per `ARCH_19_14`, the tab must instead enumerate one collapsible **Type-section** per distinct
`markerTypeName` value actually present across `recipe.markerLayerBundles[*]` ∪
`recipe.markerRuleLayers[*]` ∪ `recipe.markerLayers[*]` (deduped), Alloy/Plasma/Spawn first (only if
present), then other distinct values alphabetical, then a final `"(Unassigned)"` bucket — and inside
each, a **filtered** Bundle tree plus the two **filtered** "Ungrouped ..." lists, per `ARCH_19_15`.

## Fix

### 1. New enumeration + per-type state — `src/ui/MarkersTab_TypeSections_UI.h`

`ARCH_19_14` #2 (design doc's exhaustive ARCH-rulings-needed list): "the Type-section tier is
UI-derived (dynamic enumeration over existing `markerTypeName` values), not a new stored `Params`
container — no `Params::MarkerTypeSection` struct." This file is the ONLY place that fact lives.

```cpp
// MarkersTab_TypeSections_UI.h — the Markers tab's dynamic Type-section outer loop (STEP125,
// ARCH §19.14/§19.15, Ticket B). Layer: UI. Enumerates Params::MarkerLayerBundle::markerTypeName /
// Params::MarkerRuleLayer::markerTypeName / Params::MarkerInstanceLayer::markerTypeName (STEP119/
// STEP124) into one collapsible Section per distinct value present — no Params::MarkerTypeSection
// struct exists or should exist (ARCH_19_14's own binding ruling).
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Section_UI.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState;
struct IconAtlasManifest;

// The ordered, deduped list of Type-section keys this frame should render, ARCH_19_14's binding
// order: Alloy, Plasma, Spawn first (each only if actually present — this stays a DYNAMIC
// enumeration, not three hardcoded tabs), then every other distinct non-empty value, alphabetical,
// then "" last. "" is the internal key for the "(Unassigned)" bucket (a Bundle/Layer whose own
// markerTypeName == "") — see DrawMarkerTypeSections's own header comment for why "" is ALWAYS
// appended (a STEP125 bootstrap ruling, not literally spelled out by ARCH_19_14's own text).
std::vector<std::string> EnumerateMarkerTypeSectionNames(
    const std::vector<Params::MarkerLayerBundle>& bundles,
    const std::vector<Params::MarkerRuleLayer>& ruleLayers,
    const std::vector<Params::MarkerInstanceLayer>& instanceLayers);

// One Type-section's own three independent collapse toggles: the section itself, plus its two
// nested "Ungrouped ..." sub-sections (Item 3: "nested one level deeper", not retired). Grouped in
// one struct so ONE map lookup (below) reaches all three, mirroring how MarkersTabState already
// groups several SectionStates together per block.
struct MarkerTypeSectionState_UI {
    SectionState outerSection;
    SectionState ungroupedProceduralSection;
    SectionState ungroupedManualSection;
};

// Caller-owned (Section_UI.h's rule; MarkersTabState's "one instance each" posture), keyed by the
// SAME string EnumerateMarkerTypeSectionNames returns for that section — string-keyed persistent UI
// state has one direct precedent in this codebase, IconAtlasPairing_UI.h:49's
// `std::unordered_map<std::string, IconIdentifierPairing>`. A type name typed into a Bundle's "Marker
// Type" free-text field (MarkersTab_BundleNodeBody_UI.cpp) that nobody has expanded yet default-
// constructs its SectionState (bOpen = true, the struct's own default) on first `operator[]` access,
// mirroring TreeListState::expandedNodeIdentifiers's own "default on first sight" contract.
struct MarkerTypeSectionsState {
    std::unordered_map<std::string, MarkerTypeSectionState_UI> stateByTypeName;
};

// The whole outer loop: one collapsible Section per EnumerateMarkerTypeSectionNames entry, each
// containing a type-filtered Bundle tree (MarkersTab_Bundles_UI.h) and the two type-filtered
// "Ungrouped ..." lists (MarkersTab_RuleLayers_UI.h / MarkersTab_ManualLayers_UI.h), REPLACING
// DrawMarkersTab's three old flat calls (DrawMarkerLayerBundleTree/DrawRuleStack/
// DrawManualMarkerLayers) at their one call site (MarkersTab_UI.cpp). Also draws the handful of
// controls that are correctly TAB-WIDE, not per-type, exactly once each — see this function's own
// .cpp header comment for the composition reasoning.
void DrawMarkerTypeSections(Params::MapRecipe& recipe, MarkersTabState& state,
                            Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest);

} // namespace Ui
} // namespace SanmapGen
```

**`EnumerateMarkerTypeSectionNames` — `src/ui/MarkersTab_TypeSections_UI.cpp`:**
```cpp
namespace {
void CollectDistinctNonEmptyTypeName(const std::string& name, std::vector<std::string>& outDistinct) {
    if (name.empty()) return;
    for (const std::string& existing : outDistinct) if (existing == name) return;
    outDistinct.push_back(name);
}
} // namespace

std::vector<std::string> EnumerateMarkerTypeSectionNames(
        const std::vector<Params::MarkerLayerBundle>& bundles,
        const std::vector<Params::MarkerRuleLayer>& ruleLayers,
        const std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    std::vector<std::string> distinct;   // every non-empty markerTypeName present, union+dedup
    for (const Params::MarkerLayerBundle& bundle : bundles) CollectDistinctNonEmptyTypeName(bundle.markerTypeName, distinct);
    for (const Params::MarkerRuleLayer& layer : ruleLayers) CollectDistinctNonEmptyTypeName(layer.markerTypeName, distinct);
    for (const Params::MarkerInstanceLayer& layer : instanceLayers) CollectDistinctNonEmptyTypeName(layer.markerTypeName, distinct);

    std::vector<std::string> ordered;
    for (const char* fixedName : { "Alloy", "Plasma", "Spawn" })   // ARCH_19_14: fixed order, present-only
        for (const std::string& name : distinct)
            if (name == fixedName) { ordered.push_back(name); break; }
    std::vector<std::string> others;
    for (const std::string& name : distinct)
        if (name != "Alloy" && name != "Plasma" && name != "Spawn") others.push_back(name);
    std::sort(others.begin(), others.end());
    for (const std::string& name : others) ordered.push_back(name);
    ordered.push_back("");   // "(Unassigned)" — ALWAYS appended, not gated on presence (STEP125's own
                              // bootstrap ruling, see .h comment): a brand-new recipe with zero
                              // Bundles/Layers must still have SOMEWHERE to click "Add Group"/
                              // "Add Layer" for the very first one; every other section already
                              // requires the type to exist in data first (no chicken-and-egg problem
                              // for a NAMED type — its first Bundle/Layer is minted from an ALREADY-
                              // open Unassigned section, then typed into via the free-text field).
    return ordered;
}
```
**Flag for ARCH sign-off:** the "Unassigned bucket is unconditional" rule above is NOT literally
stated by `ARCH_19_14` (its own text reads "a final `"(Unassigned)"` bucket for `markerTypeName ==
""`", which could be read either as "present when the data has one" or "always present" — this
ticket adopts the latter reading because the former leaves a brand-new/emptied recipe with zero
Type-sections and therefore no "Add Group" affordance anywhere in the tab, a real bootstrapping
defect, not a style choice).

**Known, accepted quirk, flagged not fixed (same reporting posture ARCH_19_15 itself uses for the
`bRowSuppressed` blast-radius note):** a user who types the literal text `"(Unassigned)"` into a
Bundle's free-text "Marker Type" field (`MarkersTab_BundleNodeBody_UI.cpp`'s `DrawTextInput("Marker
Type", ...)`, soft-validated only per §19.12) creates a SECOND, distinct section whose internal key
is the non-empty string `"(Unassigned)"` (sorted alphabetically among "other distinct values", NOT
merged into the real Unassigned bucket, whose internal key is the empty string) but whose DISPLAY
LABEL is also `"(Unassigned)"` — a label collision, not a data collision. The two sections stay
functionally independent (different `markerTypeName` values, different filtered copies, different
`MarkerTypeSectionState_UI` map entries) the whole time; only the header TEXT is ambiguous. Not
worth guarding against for a free-text field with no combo/autocomplete (§19.12's own "no combo
built" ruling).

### 2. `DrawMarkerLayerBundleTree` gains the ARCH_19_15(a) filter parameter

`ARCH_19_15(a)`, ratified: "`DrawMarkerLayerBundleTree` gains one parameter, `const std::string&
markerTypeNameFilter`; it builds a fresh `std::vector<Params::MarkerLayerBundle>` per type,
containing only bundles whose `markerTypeName` matches, and passes that filtered copy to
`TreeListWidget_UI<MarkerLayerBundle>::Render`... 'Add Group' inside a Type-section seeds
`bundle.markerTypeName = markerTypeNameFilter` at creation."

`src/ui/MarkersTab_Bundles_UI.h` — signature gains the parameter (appended last, no default — every
call site is now type-scoped, there is no more "root/global" tree render):
```cpp
void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                               const std::string& markerTypeNameFilter);   // NEW — ARCH §19.15(a)
```
Also declares two NEW pure helpers (extracted from the function body specifically so this ticket's
own required Verify coverage — "filtered-copy read/write safety" and the cross-section cutoff test —
has a real, imgui-free seam to call, mirroring STEP120's own "defer the imgui-coupled path, test the
definitely-pure pieces" posture for `MarkersTab_Bundles_UI_Test.cpp`):
```cpp
// The filtered COPY (ARCH §19.15(a)) — safe to pass as TreeListWidget_UI's `nodes` parameter because
// Render uses `nodes` for tree LAYOUT only; every mutation path resolves the REAL
// Params::MarkerLayerBundle& by identifier-keyed lookup into the caller's own `bundles` vector
// (ApplyMarkerLayerBundleTreeSignal, below), never by position within this copy.
std::vector<Params::MarkerLayerBundle> BuildFilteredMarkerLayerBundlesByType(
    const std::vector<Params::MarkerLayerBundle>& bundles, const std::string& markerTypeNameFilter);

// The Select/Reparent signal-application logic DrawMarkerLayerBundleTree already ran inline
// (STEP120) — extracted verbatim, UNCHANGED behavior, purely so it has a name and can be driven
// directly by a test fixture without an imgui frame (STEP125's own required "filtered-copy write
// safety" coverage, see Verify).
void ApplyMarkerLayerBundleTreeSignal(const TreeListSignal<MarkerGroupLeafKey_UI>& signal,
                                      std::vector<Params::MarkerLayerBundle>& bundles,
                                      std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                      std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                      MarkerLayerBundlesState& state);
```
`MarkerLayerBundlesState` loses its `SectionState section;` field — the "Groups" header
`DrawMarkerLayerBundleTree` used to draw around itself is GONE (the Type-section's own outer
`DrawSectionBegin` already supplies that collapsible; Item 3's own wording, "drawn after that
section's `DrawMarkerLayerBundleTree` call, inside the same Type-section collapsible", places the
tree directly inside the Type-section body, not behind a second nested "Groups" header). Confirmed
by direct read this is `state.bundles.section`'s ONLY use site — safe to remove, no dangling
reference. `treeState`/`selectedBundleIdentifier`/the Move-Rotate scratch triple stay exactly as they
are: Bundle identifiers are globally unique across every Type-section (`NextMarkerLayerBundleId`
scans the WHOLE, unfiltered `bundles` vector — unchanged, still called with the real vector, never
the filtered copy), so ONE shared `MarkerLayerBundlesState` correctly backs all N per-type `Render`
calls — a bundle's own expand-state and "is this the selected bundle" bit are inherently tab-wide
concepts, not per-section ones. Flagged here as a confirmed-safe reuse, not an assumption.

`src/ui/MarkersTab_Bundles_UI.cpp` — new body:
```cpp
void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest*,
                               const std::string& markerTypeNameFilter) {
    if (ImGui::Button("Add Group")) {
        Params::MarkerLayerBundle bundle;
        bundle.identifier     = NextMarkerLayerBundleId(bundles);   // scans the REAL, unfiltered vector
        bundle.name           = "Group";
        bundle.markerTypeName = markerTypeNameFilter;                // ARCH §19.15(a)
        bundles.push_back(bundle);
        state.selectedBundleIdentifier = bundle.identifier;
    }

    const std::vector<Params::MarkerLayerBundle> filteredBundles =
        BuildFilteredMarkerLayerBundlesByType(bundles, markerTypeNameFilter);
    const MarkerLayerBundleLeafIndex_UI leafIndex = BuildMarkerLayerBundleLeafIndex(ruleLayers, instanceLayers);
    // ^ UNCHANGED, still built over the full ruleLayers/instanceLayers: a Bundle's own DIRECT leaves
    // are looked up by that Bundle's own (globally unique) identifier regardless of which
    // Type-section is currently rendering, so this index needs no filtering of its own.

    const TreeListSignal<MarkerGroupLeafKey_UI> signal =
        TreeListWidget_UI<Params::MarkerLayerBundle, MarkerGroupLeafKey_UI>::Render(
            "markerLayerBundles", filteredBundles,   // <-- the ONLY thing that changed inside Render's
            [](const Params::MarkerLayerBundle& bundle) { return bundle.identifier; },              // own call: filteredBundles, not bundles.
            [](const Params::MarkerLayerBundle& bundle) { return bundle.parentBundleIdentifier; },
            [](const Params::MarkerLayerBundle& bundle) { return bundle.name.empty() ? "Group" : bundle.name.c_str(); },
            [&](int bundleIdentifier) {
                DrawMarkerLayerBundleNodeBody(bundleIdentifier, bundles, ruleLayers, instanceLayers, markers,
                                              state, rootState, previewDriver);   // unchanged: real bundles
            },
            [&](int bundleIdentifier) -> const std::vector<MarkerGroupLeafKey_UI>& {
                static const std::vector<MarkerGroupLeafKey_UI> kNoLeaves;
                const auto it = leafIndex.leavesByBundleIdentifier.find(bundleIdentifier);
                return it != leafIndex.leavesByBundleIdentifier.end() ? it->second : kNoLeaves;
            },
            [&](const MarkerGroupLeafKey_UI& leaf) { return MarkerGroupLeafLabel(leaf, ruleLayers, instanceLayers); },
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafBody(leaf, ruleLayers, instanceLayers, markers, geometry, globalSymmetryMask,
                                        globalRadialRepeatCount, markerSymmetryFixSettings, rootState, previewDriver);
            },
            state.treeState, state.selectedBundleIdentifier);

    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, state);
}
```
`BuildFilteredMarkerLayerBundlesByType`/`ApplyMarkerLayerBundleTreeSignal` definitions: exactly the
body already shown above (filter) and STEP120's own existing Select/Reparent block, byte-identical,
just wrapped in a named function instead of left inline — no behavior change, only a name.

**Cross-Type-section nested-Bundle cutoff — `ARCH_19_15(b)`, ratified, "confirmed for free":** a
nested child Bundle whose own `markerTypeName` differs from its parent's is simply absent from the
PARENT's filtered copy (`BuildFilteredMarkerLayerBundlesByType` only matches on the child's OWN
field, never the parent's); `TreeListWidget_UI<T,LeafKeyT>::Render`'s own dangling-parent-resolves-
to-root rule — already proven generically by STEP120's own `TreeListWidget_UI_Test.cpp` — then
renders that child as a ROOT inside its own, different Type-section's `Render` call (where its true
`parentBundleIdentifier` still doesn't resolve, because that filtered copy ALSO excludes the actual
parent). Zero new widget code, exactly as ARCH_19_15(b) states; this ticket's job is only to prove
`BuildFilteredMarkerLayerBundlesByType` produces the two filtered vectors the already-proven generic
contract needs (see Verify).

### 3. The two "Ungrouped ..." lists — `bRowSuppressed` composes two predicates (`ARCH_19_15(c)`)

`ARCH_19_15(c)`, ratified: `row.bRowSuppressed = (layer.parentBundleIdentifier != -1) ||
(layer.markerTypeName != thisSection.typeName);` — "composing two independent boolean filters with
`||` is ordinary predicate usage, not a contract violation. Signed off as legal."

Each predicate is pulled into its own one-line, directly-testable `inline` function (this ticket's
own choice, not an ARCH requirement, but the natural, minimal seam for the "double-filter
composition" Verify coverage this ticket must provide — mirrors `IsMarkerInstanceLayerLocked`'s own
existing shape in the same file family):

`src/ui/MarkersTab_RuleLayers_UI.h`, beside `ApplyMarkerRuleLayerListSignal`:
```cpp
inline bool IsMarkerRuleLayerRowSuppressed(const Params::MarkerRuleLayer& layer,
                                           const std::string& markerTypeNameFilter) {
    return layer.parentBundleIdentifier != -1 || layer.markerTypeName != markerTypeNameFilter;
}
```
`src/ui/MarkersTab_ManualLayerHelpers_UI.h` (NEW — see §6's file-size remediation), beside the
relocated `IsMarkerInstanceLayerLocked`:
```cpp
inline bool IsMarkerInstanceLayerRowSuppressed(const Params::MarkerInstanceLayer& layer,
                                               const std::string& markerTypeNameFilter) {
    return layer.parentBundleIdentifier != -1 || layer.markerTypeName != markerTypeNameFilter;
}
```

**`MarkersTab_RuleLayers_UI.cpp`:** `DrawRuleLayerListBody` (currently anonymous-namespace-local,
`lines 70-106`) is promoted out of the anonymous namespace, declared in `MarkersTab_RuleLayers_UI.h`,
gains `const std::string& markerTypeNameFilter`, and returns `bool` (whether the recipe moved from
LIST signals alone — its own `DrawPendingDeleteRuleLayerDialog` call and trailing
`NotifyPlacementChange` move OUT, see §5):
```cpp
bool DrawRuleLayerListBody(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                           const std::string& markerTypeNameFilter) {
    ... unchanged body ...
        [&](int rowIndex) {
            const Params::MarkerRuleLayer& layer = markerRuleLayers[static_cast<std::size_t>(rowIndex)];
            ...
            DraggableListRow row;
            row.bRowSuppressed = IsMarkerRuleLayerRowSuppressed(layer, markerTypeNameFilter);   // CHANGED
            row.label    = rowLabel;
            row.bVisible = layer.bEnabled;
            row.bLocked  = layer.bHidden;
            return row;
        },
        ... unchanged ...
    return ApplyRuleLayerFrameSignals(markerRuleLayers, state, ruleSignal, ruleSignalLayerIndex, layerSignal);
    // no DrawPendingDeleteRuleLayerDialog / NotifyPlacementChange here anymore — §5
}
```

**`MarkersTab_ManualLayers_UI.cpp`:** `DrawLayerList` (anonymous, `lines 33-58`) is promoted out of
the anonymous namespace, declared in the header, gains the same parameter:
```cpp
DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted,
                                  const std::string& markerTypeNameFilter) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) {
            const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.bRowSuppressed = IsMarkerInstanceLayerRowSuppressed(layer, markerTypeNameFilter);   // CHANGED
            row.label   = ManualMarkerLayerRowLabel(layer);
            row.bLocked = layer.bLocked;
            return row;
        },
        ... unchanged ...
```

**Known, accepted, inherited quirk (`ARCH_19_15(c)`'s own text, recorded again here since it now
applies across N sections, not one):** both lists still walk the SAME full, unfiltered backing
vector under a per-section suppression predicate; a reorder-drag issued from one Type-section's
filtered view operates on real vector indices and can silently land past a row belonging to a
DIFFERENT, currently-invisible type OR section. Signed off in ARCH_19_15(c); not owed by this ticket.

### 4. "Add Layer" buttons pre-set `markerTypeName` too — extends Item 3/§19.15(a)'s own pattern

**Flag for ARCH sign-off — this ticket's own necessary extension, not literally covered by
`ARCH_19_14`/`ARCH_19_15`:** those two files rule on the Bundle tree's own "Add Group" pre-setting
`markerTypeName` (§19.15(a)), but say nothing about the two "Ungrouped ..." lists' own "Add Layer"
buttons. Left unset, a layer added from inside, say, the "Alloy" section's "Ungrouped Procedural
Rules" sub-list would mint with `markerTypeName == ""` and re-render in the UNRELATED "(Unassigned)"
section on the very next frame — visually "vanishing" from the section the user just added it in, a
real correctness gap by direct analogy to the Bundle case ARCH_19_15(a) already fixed one tier up.
This ticket extends the identical, already-ratified pattern to both Add-Layer buttons:

`src/ui/MarkersTab_RuleLayerSettings_UI.cpp`, `DrawAddMarkerRuleLayerButton` gains a second optional
parameter, threaded exactly like `parentBundleIdentifierForNewLayer`:
```cpp
bool DrawAddMarkerRuleLayerButton(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                                  int parentBundleIdentifierForNewLayer = -1,
                                  const std::string& markerTypeNameForNewLayer = "") {
    if (!ImGui::Button(parentBundleIdentifierForNewLayer < 0 ? "Add Layer" : "Add Procedural Layer Here"))
        return false;
    Params::MarkerRuleLayer layer;
    layer.parentBundleIdentifier = parentBundleIdentifierForNewLayer;
    layer.markerTypeName         = markerTypeNameForNewLayer;   // NEW — STEP125
    markerRuleLayers.push_back(layer);
    state.selectedRuleLayerIndex = static_cast<int>(markerRuleLayers.size()) - 1;
    state.selectedRuleIndex      = 0;
    return true;
}
```
(Declaration in `MarkersTab_RuleLayers_UI.h` gains the matching default parameter.) Its EXISTING
Bundle-node-body call site (`MarkersTab_BundleNodeBody_UI.cpp`:
`DrawAddMarkerRuleLayerButton(ruleLayers, rootState, bundleIdentifier)`) is unaffected — a Layer
added INSIDE a Bundle inherits its type from the Bundle's own `markerTypeName` at ITS next
`BuildFilteredMarkerLayerBundlesByType`-adjacent lookup (the leaf-index lookup, unaffected by this
ticket), so this ticket does NOT thread a `markerTypeNameForNewLayer` through that call site — a
Bundle-scoped Layer's own `markerTypeName` field is set from the SECTION's type only when added via
the Type-section's own Ungrouped list, matching this ticket's own new call site below.

`src/ui/MarkersTab_ManualLayers_UI.h`/`.cpp`, `DrawLayerListButtons` gains the identical parameter:
```cpp
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer = -1,
                          const std::string& markerTypeNameForNewLayer = "");
// body: layer.markerTypeName = markerTypeNameForNewLayer;  added beside the existing
// layer.parentBundleIdentifier assignment.
```

### 5. Composition fixes needed to actually draw N sections in one frame

Three tab-wide-not-per-type things this ticket's own restructure would otherwise silently duplicate
or desync N ways — each is this ticket's own necessary call, not an ARCH ruling, flagged as such:

**(a) The Manual block-wide settings (Use Group Color / Layer Icon Scale) are drawn ONCE, before the
loop, not once per section.** `DrawLayerSettings` (`MarkersTab_ManualLayers_UI.cpp`'s
anonymous-namespace `lines 20-27`) edits `state.bUseGroupColor`/`groupColor`/`layerIconScale` —
map-wide UI-only preview preferences with no Type/Bundle scope of their own (unchanged by this
ticket's own Item 1). Promoted out of the anonymous namespace, renamed
`DrawManualMarkerLayerBlockSettings`, declared in the header, called exactly once by
`DrawMarkerTypeSections` before its own per-type loop starts. `DrawManualMarkerLayers` (the old,
Section-wrapping, `lines 110-126`) is RETIRED — its three jobs (block settings / list body / Section
wrap) split three ways: (a) here, (b) `DrawManualMarkerLayerListBody` below, (c) the Type-section's
own `DrawSectionBegin("Ungrouped Manual Marker Layers", ...)` call.

New `DrawManualMarkerLayerListBody` (replaces `DrawManualMarkerLayers`'s remaining, per-type-scoped
job — list + Add button + repair, no Section wrap, no block settings):
```cpp
void DrawManualMarkerLayerListBody(ManualMarkerLayersState& state, std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                                   int globalSymmetryMask, int globalRadialRepeatCount,
                                   Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                   const std::string& markerTypeNameFilter) {
    bool bLayersMoved = DrawLayerListButtons(markerLayers, state, -1, markerTypeNameFilter);
    bool bAnyNameCommitted = false;
    const DraggableListSignal signal = DrawLayerList(markerLayers, markers, geometry, globalSymmetryMask,
        globalRadialRepeatCount, markerSymmetryFixSettings, state, bAnyNameCommitted, markerTypeNameFilter);
    if (signal.bHasSignal()) ApplyLayerListSignal(markerLayers, markers, state, signal);
    bLayersMoved = bAnyNameCommitted || bLayersMoved;
    if (bLayersMoved) MakeNamesUnique(markerLayers);   // unchanged: still keys the WHOLE vector, safe
}                                                       // to re-run once per section that itself moved
```

**(b) Add Rule / Remove Selected Rule, and the non-empty-layer delete confirm, are drawn ONCE,
tab-wide, after the whole loop — not once per section.** Both operate on single, tab-wide scalars
(`state.selectedRuleLayerIndex`/`state.selectedRuleIndex`/`state.pendingDeleteRuleLayerIndex`/
`state.deleteRuleLayerConfirmState`) that are NOT type-scoped — the currently-selected rule layer can
live in any one section regardless of which section is currently drawing. Drawing them once per
Type-section would render up to N redundant copies, ALL operating on the same single global
selection (confusing: a "Remove Selected Rule" click inside the Alloy section could delete a rule
actually selected in the Plasma section). `DrawRuleLayerButtons` (`MarkersTab_RuleLayerSettings_UI.cpp`,
`lines 64-87`) is split: its own `DrawAddMarkerRuleLayerButton(..., -1)` call is retired (Add Layer is
now per-section, §4 above); its Add Rule / Remove Selected Rule block becomes a new, standalone
function, called once:
```cpp
void DrawMarkerRuleButtons(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver) {
    Params::MarkerRuleLayer* const layer = SelectedMarkerRuleLayer(markerRuleLayers, state);
    bool bRecipeMoved = false;
    ImGui::BeginDisabled(layer == nullptr);
    if (ImGui::Button("Add Rule") && layer != nullptr) {
        layer->rules.push_back(Params::MarkerRule());
        state.selectedRuleIndex = static_cast<int>(layer->rules.size()) - 1;
        bRecipeMoved = true;
    }
    ImGui::SameLine();
    const bool bRuleSelected = layer != nullptr && state.selectedRuleIndex >= 0
        && state.selectedRuleIndex < static_cast<int>(layer->rules.size());
    if (ImGui::Button("Remove Selected Rule") && bRuleSelected) {
        layer->rules.erase(layer->rules.begin() + state.selectedRuleIndex);
        state.selectedRuleIndex = static_cast<int>(layer->rules.size()) - 1;
        bRecipeMoved = true;
    }
    ImGui::EndDisabled();
    NotifyPlacementChange(bRecipeMoved, previewDriver);
}
```
`DrawRuleLayerButtons`/`DrawMarkerRuleLayerList` (the old combined Section-free-but-list+buttons
entry point, `MarkersTab_RuleLayers_UI.cpp` `lines 142-149`) and `DrawRuleStack`
(`MarkersTab_UI.cpp`'s anonymous-namespace `lines 20-25`) are all RETIRED — `DrawRuleLayerButtons`
had exactly one caller (`DrawMarkerRuleLayerList`), which had exactly one caller (`DrawRuleStack`),
which had exactly one caller (`DrawMarkersTab`); all three are dead once `DrawMarkerTypeSections`
replaces that one call site (§7).

**(c) `DrawMarkerTypeSections`'s own per-type ImGui ID salting — load-bearing, confirmed by direct
read of `Section_UI.cpp:44,65`.** `DrawSectionBegin`'s own `ImGui::PushID(label)`/`PopID()`
(`Section_UI.cpp:44,65`) wraps ONLY the header bar's own draw calls — it POPS before returning
`change.bBodyVisible`, so the BODY drawn between `DrawSectionBegin`/`DrawSectionEnd` is NOT inside
that PushID scope. Since `TreeListWidget_UI::Render`'s own tree identifier (`"markerLayerBundles"`)
and both `DraggableList<T>::Render`'s own identifiers (`"markerRuleLayers"`, `"manualMarkerLayers"`)
are FIXED LITERALS, calling `DrawMarkerLayerBundleTree`/`DrawRuleLayerListBody`/`DrawLayerList` once
per Type-section with the SAME literal identifiers would collide across sections (two sections'
"markerLayerBundles" trees would fight over the same ImGui ID stack) unless something upstream
salts them apart first. `DrawMarkerTypeSections` MUST wrap each Type-section's entire body —
`ImGui::PushID(typeName.c_str())` immediately before that section's `DrawSectionBegin` call,
`ImGui::PopID()` immediately after that section's own `DrawSectionEnd()` — so every fixed-literal
child identifier is salted by its own section's type name first. Full composition:
```cpp
void DrawMarkerTypeSections(Params::MapRecipe& recipe, MarkersTabState& state,
                            Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    DrawManualMarkerLayerBlockSettings(state.manualLayers);   // (a) — once, tab-wide

    const std::vector<std::string> typeNames = EnumerateMarkerTypeSectionNames(
        recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers);

    for (const std::string& typeName : typeNames) {
        ImGui::PushID(typeName.c_str());   // (c) — salts every fixed-literal child ID this section owns
        MarkerTypeSectionState_UI& perType = state.typeSections.stateByTypeName[typeName];
        const char* const label = typeName.empty() ? "(Unassigned)" : typeName.c_str();
        if (DrawSectionBegin(label, perType.outerSection)) {
            DrawMarkerLayerBundleTree(recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers,
                                      recipe.markers, recipe.geometry, recipe.globalSymmetryMask,
                                      recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                      state.bundles, state, previewDriver, iconManifest, typeName);

            if (DrawSectionBegin("Ungrouped Procedural Rules", perType.ungroupedProceduralSection)) {
                bool bRecipeMoved = DrawRuleLayerListBody(recipe.markerRuleLayers, state, previewDriver,
                                                          iconManifest, typeName);
                bRecipeMoved = DrawAddMarkerRuleLayerButton(recipe.markerRuleLayers, state, -1, typeName)
                             || bRecipeMoved;
                NotifyPlacementChange(bRecipeMoved, previewDriver);
                DrawSectionEnd();
            }
            if (DrawSectionBegin("Ungrouped Manual Marker Layers", perType.ungroupedManualSection)) {
                DrawManualMarkerLayerListBody(state.manualLayers, recipe.markerLayers, recipe.markers,
                                              recipe.geometry, recipe.globalSymmetryMask,
                                              recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                              typeName);
                DrawSectionEnd();
            }
            DrawSectionEnd();   // outer Type-section
        }
        ImGui::PopID();
    }

    DrawMarkerRuleButtons(recipe.markerRuleLayers, state, previewDriver);   // (b) — once, tab-wide
    NotifyPlacementChange(DrawPendingDeleteRuleLayerDialog(recipe.markerRuleLayers, state), previewDriver);
}
```

### 6. File-size ceiling — required remediation, `src/ui/MarkersTab_ManualLayers_UI.h` (two splits, both required, per `ARCH_19_22_ManualLayersHeaderSplit.md`'s FINAL combined ruling)

Confirmed by direct `wc -l` before any edit: **`MarkersTab_ManualLayers_UI.h` is currently 165
lines** — already over `ARCH_01_05_FileSizeCeilings.md`'s 150-line hard ceiling, un-remediated
(same class of pre-existing, unflagged violation STEP124 found and fixed for
`MapImporter_Markers_IO.cpp`). This ticket's own additions to it (§3's new predicate declaration if
left here, §4/§5's signature changes, `MarkersTab_TypeSections_UI.h`'s forward references) would
land it at roughly 178-182 lines, a further silent ratchet on an already-broken ceiling.

**This ticket's own diff must land BOTH of the following splits — a separately-ratified, still-unbuilt
RowBody split (already ruled by `ARCH_19_22` before this ticket existed) plus this ticket's own
Helpers split — since neither alone clears the ceiling once this ticket's own new declarations are
counted (`ARCH_19_22`'s FINAL combined ruling, 2026-08-26, worked the arithmetic).**

**Split A — `src/ui/MarkersTab_ManualLayerRowBody_UI.h` (NEW; a pre-existing ARCH ruling this ticket
must finally deliver, not this ticket's own invention).** Moves out of `MarkersTab_ManualLayers_UI.h`,
verbatim (declarations + existing doc comments):
- `bool DrawLayerRowBody(Params::MarkerInstanceLayer&, int, const std::vector<Params::MarkerInstanceLayer>&, std::vector<Params::MarkerInstanceGroup>&, const Params::Geometry&, int, int, Params::MarkerSymmetryFixSettings&, ManualMarkerLayersState&);`
- `void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer&, ManualMarkerLayersState&, bool&);`
- `kMarkerLayerColorOverrideHeaderWidthPixels` / `kMarkerLayerColorOverrideSwatchWidthPixels`.

`#include`s `MarkersTab_ManualLayers_UI.h` (needs `ManualMarkerLayersState`) plus the PARAMS headers
those two signatures need. Consumer `#include` updates, all required by this ticket's own diff:
`MarkersTab_ManualLayerRowBody_UI.cpp` (its own first include becomes this new header),
`MarkersTab_ManualLayers_UI.cpp` (its `DrawLayerList` calls both functions and uses
`kMarkerLayerColorOverrideHeaderWidthPixels` directly), `MarkersTab_Bundles_UI.cpp` (calls
`DrawLayerRowBody` as the tree's Manual leaf-body callback), and
`MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp` (exercises
`DrawManualMarkerLayerColorOverrideHeaderControl` by name).

**Split B — `src/ui/MarkersTab_ManualLayerHelpers_UI.h` (NEW; this ticket's own required split).**
The five small, pure, standalone helper functions (`IsMarkerInstanceLayerLocked`,
`QuantizeMarkerPositionToLayerGrid`, `EffectiveManualMarkerLayerColor`, `ManualMarkerLayerRowLabel`,
`NextMarkerLayerName` — named explicitly here; **`SelectedManualMarkerLayer` is NOT included**, per
`ARCH_19_22`'s explicit carve-out: it is dead code with zero call sites and stays in the parent header
untouched, not silently relocated by this ticket) share no state with the struct definition or the
entry-point declarations — the same kind of "pure logic, own file" content
`RigidTransformPivot_MATH.h`/`MarkerLayerId_UI.h` already isolate elsewhere in this codebase. Moves
all five out verbatim, plus this ticket's own new `IsMarkerInstanceLayerRowSuppressed` (§3) beside its
nearest existing neighbor (`IsMarkerInstanceLayerLocked`). `#include`s `<cmath>`, `UniqueNameList_UI.h`,
and `MarkersTab_ManualLayers_UI.h` (needs `ManualMarkerLayersState`). Consumer `#include` updates:
`MarkersTab_ManualLayers_UI.cpp` (defines `NextMarkerLayerName`'s call site, `ManualMarkerLayerRowLabel`,
and now `IsMarkerInstanceLayerRowSuppressed` per §3) and `MarkersTab_ManualLayers_UI_Test.cpp`
(exercises `IsMarkerInstanceLayerLocked`/`QuantizeMarkerPositionToLayerGrid` today, extended per this
ticket's own Verify section to also exercise `IsMarkerInstanceLayerRowSuppressed` and
`DrawLayerListButtons`'s new parameter).

**Resulting `MarkersTab_ManualLayers_UI.h` shape (survives, shrunk).** Keeps the header comment
(updated to name both companion files plus `MarkersTab_TypeSections_UI.h` one level up), the
`ManualMarkerLayersState` struct, `SelectedManualMarkerLayer`, and gains
`#include "DraggableListWidget_UI.h"` (new requirement — `DrawLayerList`'s promoted declaration now
returns `DraggableListSignal` by name, the same include `MarkersTab_RuleLayers_UI.h` already uses for
the identical reason); drops `<cmath>`/`UniqueNameList_UI.h` (only served the relocated Helpers). Per
`ARCH_19_22`'s own arithmetic, doing BOTH splits together lands this file at roughly **110-115 lines**
— comfortably under the 150-line hard ceiling (STEP125's own earlier estimate of "145-150" assumed
only the Helpers split, unaware the RowBody split was separately ratified and still pending — doing
both together, not either alone, is what actually clears the ceiling with real margin). Coder: confirm
the actual count once written; if still over, that is this ticket's own exception to flag explicitly
(Constitution §7), not a signal to silently absorb.

`src/ui/MarkersTab_Bundles_UI.cpp`'s own projected size (138 lines today; this ticket removes the
`DrawSectionBegin`/`DrawSectionEnd("Groups", ...)` wrap but adds `BuildFilteredMarkerLayerBundlesByType`
+ the extracted `ApplyMarkerLayerBundleTreeSignal`) lands close to the 150-line ceiling — **Coder:
verify the actual formatted count; if it crosses 150, split `BuildFilteredMarkerLayerBundlesByType`/
`ApplyMarkerLayerBundleTreeSignal` into a new `MarkersTab_BundleTreeSignals_UI.cpp`** (declared,
unchanged, in `MarkersTab_Bundles_UI.h`), mirroring the EXACT precedent this file pair already uses
(`MarkersTab_BundleNodeBody_UI.cpp` split out of `MarkersTab_Bundles_UI.cpp` for the identical
150-line reason, STEP120) — not pre-emptively split here since the exact count depends on final
formatting (same "Coder verifies, splits only if needed" posture STEP120 itself used for
`TreeListWidget_UI.h`).

### 7. `MarkersTab_UI.h`/`.cpp` wiring

`MarkersTab_UI.h`: `#include "MarkersTab_TypeSections_UI.h"` (replaces the now-redundant-but-still-
needed `#include "MarkersTab_Bundles_UI.h"` — `MarkersTab_TypeSections_UI.h` itself needs
`MarkersTab_Bundles_UI.h` transitively for `MarkerLayerBundlesState`, so `MarkersTab_UI.h` keeps
including it directly too, no cycle: mirrors the existing multi-include pattern already in this
file). `MarkersTabState` gains, beside `bundles`:
```cpp
    // STEP125: the dynamic Type-section outer loop's own per-type collapse state, keyed by
    // markerTypeName (MarkersTab_TypeSections_UI.h). `bundles` above still holds the ONE shared
    // Bundle-tree state (expand/select), reused across every Type-section's own filtered Render call.
    MarkerTypeSectionsState typeSections;
```
`MarkersTab_UI.cpp`, `DrawMarkersTab` — the three old calls collapse to one:
```cpp
void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                    const IconAtlasPairingLookup* pairingLookup,
                    const Data::PlacementInstances* placedMarkers) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, recipe.globalMarkerSettings, iconManifest, pairingLookup);
    // STEP125: replaces the old flat DrawMarkerLayerBundleTree/DrawRuleStack/DrawManualMarkerLayers
    // trio with the dynamic Type-section outer loop (ARCH §19.14/§19.15).
    DrawMarkerTypeSections(recipe, state, previewDriver, iconManifest);
    state.manual.positionHorizontalRange = MarkerPositionHorizontalSliderRange(recipe.geometry.mapSize);
    DrawManualMarkers(recipe.markers, recipe.armies, recipe.markerLayers, state.manual,
                      state.manualLayers.selectedLayerIndex, iconManifest);
    DrawPlacedMarkerList(placedMarkers, state.placedList);
    ImGui::PopID();
}
```
The anonymous-namespace `DrawRuleStack` (§5(b)) is deleted from this file entirely.

## Out of scope

- **Ticket C** (instance list, selection state, `MapCanvas` wiring, tint-priority rewrite,
  sibling-orbit computation) — separately ticketed, logically independent per the design doc's own
  delivery-split recommendation; needs only STEP124's `instanceIdentifier`/select-color fields, not
  anything this ticket builds.
- **A combo/autocomplete for the "Marker Type" free-text field.** §19.12's own "no combo built"
  ruling stands; still a plain `DrawTextInput`, unchanged by this ticket.
- **Narrowing reorder-drag to the currently-visible/filtered rows.** ARCH_19_15(c)'s own signed-off
  quirk; a future ticket's call if it becomes a real authoring complaint.
- **Any change to `Params::MarkerLayerBundle`/`MarkerRuleLayer`/`MarkerInstanceLayer`/IO.** Pure UI
  restructure over STEP119/STEP124's already-landed fields; no PARAMS or IO file is touched.
- **Canvas rendering of Type — no color/icon consequence.** Exactly as STEP120 already scoped out
  for Bundle membership: a Type-section is purely organizational/UI, same as a Bundle; this ticket
  changes no draw-time tint/icon resolution.
- **Assembly's own tab.** Untouched; `TreeListWidget_UI<T,LeafKeyT>` itself is not modified by this
  ticket (only its Bundle CALL SITE changes, from one instantiation to N filtered instantiations).

## Files touched

- `src/ui/MarkersTab_TypeSections_UI.h` — **NEW**: `EnumerateMarkerTypeSectionNames`,
  `MarkerTypeSectionState_UI`, `MarkerTypeSectionsState`, `DrawMarkerTypeSections`
- `src/ui/MarkersTab_TypeSections_UI.cpp` — **NEW**: both functions' bodies
- `src/ui/MarkersTab_TypeSections_UI_Test.cpp` — **NEW**: `EnumerateMarkerTypeSectionNames` acceptance
- `src/ui/MarkersTab_ManualLayerRowBody_UI.h` — **NEW**: `DrawLayerRowBody`/
  `DrawManualMarkerLayerColorOverrideHeaderControl` declarations + their two width constants,
  relocated out of `MarkersTab_ManualLayers_UI.h` — the separately-ratified `ARCH_19_22` RowBody
  split, still unbuilt before this ticket; this ticket delivers it (file-size remediation, §6)
- `src/ui/MarkersTab_ManualLayerHelpers_UI.h` — **NEW**: five helpers relocated out of
  `MarkersTab_ManualLayers_UI.h` (file-size remediation, §6) + new `IsMarkerInstanceLayerRowSuppressed`
- `src/ui/MarkersTab_Bundles_UI.h` — `DrawMarkerLayerBundleTree` gains `markerTypeNameFilter`; new
  `BuildFilteredMarkerLayerBundlesByType`/`ApplyMarkerLayerBundleTreeSignal` declarations;
  `MarkerLayerBundlesState` loses `section`
- `src/ui/MarkersTab_Bundles_UI.cpp` — `DrawMarkerLayerBundleTree` filters + drops its Section wrap;
  new `BuildFilteredMarkerLayerBundlesByType`/`ApplyMarkerLayerBundleTreeSignal` definitions; adds
  `#include "MarkersTab_ManualLayerRowBody_UI.h"` (calls `DrawLayerRowBody` as the tree's Manual
  leaf-body callback — required consumer-include update per §6)
- `src/ui/MarkersTab_BundleNodeBody_UI.cpp` — unaffected in body; the "Add Layer here" call sites it
  owns (`DrawAddMarkerRuleLayerButton(ruleLayers, rootState, bundleIdentifier)`,
  `DrawLayerListButtons(instanceLayers, rootState.manualLayers, bundleIdentifier)`) keep their
  existing 2-argument-plus-parent calls (default `markerTypeNameForNewLayer = ""`), confirmed
  unaffected by §4's new parameter (Bundle-scoped Layers do not inherit a section type this way)
- `src/ui/MarkersTab_RuleLayers_UI.h` — new `IsMarkerRuleLayerRowSuppressed`; `DrawRuleLayerListBody`
  declared (promoted, gains filter param, returns bool); `DrawAddMarkerRuleLayerButton` gains
  `markerTypeNameForNewLayer`; `DrawRuleLayerButtons`/`DrawMarkerRuleLayerList` declarations removed;
  new `DrawMarkerRuleButtons` declared
- `src/ui/MarkersTab_RuleLayers_UI.cpp` — `DrawRuleLayerListBody` promoted + filtered + returns bool;
  `DrawMarkerRuleLayerList` removed
- `src/ui/MarkersTab_RuleLayerSettings_UI.cpp` — `DrawAddMarkerRuleLayerButton` gains the new param;
  `DrawRuleLayerButtons` removed, replaced by new `DrawMarkerRuleButtons`
- `src/ui/MarkersTab_ManualLayers_UI.h` — shrinks per §6; `DrawLayerList`/`DrawManualMarkerLayerBlockSettings`/
  `DrawManualMarkerLayerListBody` declared; `DrawLayerListButtons` gains `markerTypeNameForNewLayer`;
  `DrawManualMarkerLayers` declaration removed
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `DrawLayerList` promoted + filtered; `DrawLayerSettings`
  renamed/promoted to `DrawManualMarkerLayerBlockSettings`; `DrawManualMarkerLayers` replaced by
  `DrawManualMarkerLayerListBody`; `DrawLayerListButtons` gains the new param; adds
  `#include "MarkersTab_ManualLayerRowBody_UI.h"` and `#include "MarkersTab_ManualLayerHelpers_UI.h"`
  (both required consumer-include updates per §6)
- `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp` — its own first `#include` becomes the new paired
  `MarkersTab_ManualLayerRowBody_UI.h` (§6); no body change
- `src/ui/MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp` — adds
  `#include "MarkersTab_ManualLayerRowBody_UI.h"` (§6); no assertion change
- `src/ui/MarkersTab_UI.h` — includes `MarkersTab_TypeSections_UI.h`; `MarkersTabState` gains
  `typeSections`
- `src/ui/MarkersTab_UI.cpp` — `DrawMarkersTab` calls `DrawMarkerTypeSections`; `DrawRuleStack` removed
- `src/ui/MarkersTab_Bundles_UI_Test.cpp` — extended: `BuildFilteredMarkerLayerBundlesByType`,
  `ApplyMarkerLayerBundleTreeSignal`, the cross-Type-section cutoff proof
- `src/ui/MarkersTab_RuleLayers_UI_Test.cpp` — extended: `IsMarkerRuleLayerRowSuppressed`,
  `DrawAddMarkerRuleLayerButton`'s new-param behavior
- `src/ui/MarkersTab_ManualLayers_UI_Test.cpp` — extended: `IsMarkerInstanceLayerRowSuppressed`,
  `DrawLayerListButtons`'s new-param behavior (include path updates to
  `MarkersTab_ManualLayerHelpers_UI.h` for the relocated checks)
- `CMakeLists.txt` — new `add_sangen_test(MarkersTab_TypeSections_UI_Test
  src/ui/MarkersTab_TypeSections_UI_Test.cpp)`

## Verify

Acceptance bar: every ARCH_19_14/19.15 ruling this ticket implements has a direct, pure, imgui-free
test (mirroring STEP120's own "test the pure pieces" posture); the composition-only pieces (§5) are
verified by direct code construction/review, flagged as such, not fabricated as automated coverage
that doesn't exist.

- **`MarkersTab_TypeSections_UI_Test.cpp`** (new binary, own `main()`):
  - Empty `bundles`/`ruleLayers`/`instanceLayers` → `EnumerateMarkerTypeSectionNames` returns exactly
    `{""}` — proves the Unassigned-always-present bootstrap rule (§1).
  - A fixture with bundles typed `{"Spawn", "Alloy", "Expansion"}`, a rule layer typed `"Generic"`,
    an instance layer typed `""` (explicit empty) → the returned order is exactly
    `{"Alloy", "Spawn", "Expansion", "Generic", ""}` — proves Alloy/Plasma/Spawn-first (Plasma
    absent, correctly skipped, not a hole in the output), alphabetical-others (`"Expansion"` before
    `"Generic"`), Unassigned last, and cross-collection union (a name appearing on only a rule layer
    still shows up).
  - The SAME `"Alloy"` value present on both a Bundle AND a rule layer produces exactly ONE `"Alloy"`
    entry — proves the union is deduped, not per-collection.
  - A fixture where `bundles`/`ruleLayers`/`instanceLayers` are each non-empty but EVERY entry has
    `markerTypeName == ""` (no named type anywhere) → returns exactly `{""}` — the all-legacy-data
    case degrades to the single Unassigned bucket, no fixed-name entries invented.
- **`MarkersTab_Bundles_UI_Test.cpp`** (extend the existing file):
  - `BuildFilteredMarkerLayerBundlesByType`: a fixture of 3 bundles typed `{"Alloy", "", "Alloy"}`
    filtered by `"Alloy"` returns exactly the two `"Alloy"` bundles (by identifier, order preserved);
    filtered by `""` returns exactly the one untyped bundle; filtered by `"Plasma"` (absent from the
    fixture) returns empty.
  - `ApplyMarkerLayerBundleTreeSignal` — **the filtered-copy read/write safety proof this ticket's
    own required coverage asks for**: a fixture of 3 bundles (identifiers 10/20/30, only 20 typed
    `"Alloy"`); build the `"Alloy"` filtered copy (contains only bundle 20); synthesize a
    `TreeListSignal<MarkerGroupLeafKey_UI>{kind=Reparent, sourceKind=Node, sourceNodeIdentifier=20,
    targetNodeIdentifier=-1}` DIRECTLY (no imgui, no `Render` call — the signal is exactly the shape
    `Render` would have returned); call `ApplyMarkerLayerBundleTreeSignal(signal, bundles, ...)`
    (passing the REAL, unfiltered `bundles`, never the filtered copy); assert bundle 20's
    `parentBundleIdentifier` in the REAL vector changed, and bundles 10/30 (never in the filtered
    copy at all) are untouched — proves a write sourced from a filtered-copy-driven signal correctly
    lands on the real vector by identifier, exactly as ARCH_19_15(a)'s contract claims, not merely
    assumed from re-reading `TreeListWidget_UI`'s own already-tested generic behavior.
  - **The cross-Type-section nested-Bundle cutoff — the test this ticket's own task explicitly asks
    for**: a fixture of 2 bundles — parent (identifier 1, `markerTypeName = "Alloy"`), child
    (identifier 2, `markerTypeName = "Plasma"`, `parentBundleIdentifier = 1`). Assert
    `BuildFilteredMarkerLayerBundlesByType(bundles, "Alloy")` contains ONLY the parent (child
    excluded — different type). Assert `BuildFilteredMarkerLayerBundlesByType(bundles, "Plasma")`
    contains ONLY the child, and that within THIS filtered copy the child's own
    `parentBundleIdentifier` (still `1`) does not resolve to any OTHER entry in the copy (a direct
    linear scan: no bundle with `identifier == 1` exists in the Plasma-filtered copy) — this is
    precisely the input condition `TreeListWidget_UI::Render`'s own already-proven
    dangling-parent-is-root rule (`TreeListWidget_UI_Test.cpp`, STEP120) consumes to render a node as
    a root; proving THIS input condition, combined with that already-tested generic contract, proves
    the cutoff end to end without re-deriving the generic widget behavior a second time.
- **`MarkersTab_RuleLayers_UI_Test.cpp`** (extend, sibling TU in the `MarkersTab_UI_Test` binary):
  - `IsMarkerRuleLayerRowSuppressed`: a layer with `parentBundleIdentifier = -1, markerTypeName =
    "Alloy"` filtered by `"Alloy"` → not suppressed; filtered by `"Plasma"` → suppressed
    (type-mismatch only); a layer with `parentBundleIdentifier = 5, markerTypeName = "Alloy"`
    filtered by `"Alloy"` → suppressed (bundle-membership only, proves the `||` composition, not just
    either predicate alone); a layer with BOTH conditions true → suppressed (proves the compound case
    isn't accidentally an XOR).
  - `DrawAddMarkerRuleLayerButton`'s new parameter: calling it with
    `markerTypeNameForNewLayer = "Alloy"` and asserting the newly pushed
    `markerRuleLayers.back().markerTypeName == "Alloy"`; calling with the parameter omitted (default)
    asserts `.markerTypeName.empty()` — unchanged existing-call-site behavior preserved.
- **`MarkersTab_ManualLayers_UI_Test.cpp`** (extend the existing file; update its `#include` to add
  `MarkersTab_ManualLayerHelpers_UI.h` per §6's relocation):
  - `IsMarkerInstanceLayerRowSuppressed`: the same four-case shape as
    `IsMarkerRuleLayerRowSuppressed` above (type-mismatch-only / bundle-membership-only / both /
    neither), mirrored on `Params::MarkerInstanceLayer`.
  - `DrawLayerListButtons`'s new parameter: mirrors the procedural-side check above —
    `markerTypeNameForNewLayer = "Spawn"` lands on the newly pushed layer's own `markerTypeName`;
    omitted defaults to empty.
  - Every EXISTING check in this file (`RunIsMarkerInstanceLayerLockedChecks`,
    `RunQuantizeMarkerPositionToLayerGridChecks`, `RunRealtimeDefaultChecks`) stays green — the
    relocation to `MarkersTab_ManualLayerHelpers_UI.h` is a pure file move, no signature change to
    any of the three functions those checks already call.
- **The composition pieces in §5 (block-wide-settings-once, Add-Rule/Remove-Rule-once, the
  `ImGui::PushID(typeName)` salting requirement) have no automated imgui-frame test in this ticket —
  verified by direct construction/code-review only, same posture already established in this
  codebase for pure-composition imgui code with no dedicated headless-frame harness
  (`MarkersTab_UI_Test.cpp`'s own header comment: "drives the tab's PURE logic... needs no imgui
  frame" — `DrawMarkerTypeSections` itself is the one new piece of this ticket that is NOT
  independently exercised this way, by design; every piece it CALLS is).** Flagged explicitly, not
  silently omitted.
- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `TreeListWidget_UI_Test.cpp`'s own generic dangling-parent-is-root proof (untouched, this
  ticket only feeds it a differently-built `nodes` vector at the ONE real call site);
  `MarkersTab_Bundles_UI_Test.cpp`'s five pre-existing checks (`TestBuildMarkerLayerBundleLeafIndex`,
  `TestNextMarkerLayerBundleId`, `TestApplyMarkerLayerBundleMove`/`Rotation`,
  `TestProceduralOnlyBundleResolvesToEmptyMembership` — none of their own called functions'
  signatures change); every other `MarkersTab_*`/`ArmiesTab_*`/`PropsTab_*` test binary, since no
  shared widget (`DraggableList<T>`, `TreeListWidget_UI<T,LeafKeyT>`, `Section_UI.h`) changes its own
  contract, only this tab's OWN call sites into them.
- **No manual/interactive verification substitutes for the above** — every piece has a real headless
  seam except the one explicitly flagged in the composition-pieces bullet above.

---

**Files read to ground this ticket** (absolute paths under `D:\Projects\Sanctuary\Map Generator\`):
`work_orders\DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`;
`work_orders\STEP124_MarkerTypeSectionsParamsIO_PARAMS.md`; `ARCH_19_14_TypeSectionUiDerived.md`;
`ARCH_19_15_TypeSectionTreeComposition.md`; `work_orders\STEP120_MarkersTabBundleUI_UI.md`;
`src\ui\MarkersTab_Bundles_UI.h`/`.cpp`, `MarkersTab_BundleNodeBody_UI.cpp`,
`MarkersTab_RuleLayers_UI.h`/`.cpp`, `MarkersTab_RuleLayerSettings_UI.cpp`,
`MarkersTab_ManualLayers_UI.h`/`.cpp`, `MarkersTab_UI.h`/`.cpp`, `TreeListWidget_UI.h`,
`TreeListWidget_RowLayout_UI.h`, `TreeListWidget_Types_UI.h`, `DraggableListWidget_Types_UI.h`,
`Section_UI.h`, `Section_UI.cpp`, `SliderScalar_UI.h`, `MarkersTab_Bundles_UI_Test.cpp`,
`MarkersTab_RuleLayers_UI_Test.cpp`, `MarkersTab_ManualLayers_UI_Test.cpp`, `MarkersTab_UI_Test.cpp`;
`src\params\MarkerLayerBundle_PARAMS.h`, `MarkerLayerBundleQuery_PARAMS.h`, `MarkerRule_PARAMS.h`,
`MarkerInstance_PARAMS.h`, `GlobalMarkerSettings_PARAMS.h`, `MapRecipe_PARAMS.h` (grep for
`markerLayerBundles`/`markerRuleLayers`/`markerLayers` field spellings); `CMakeLists.txt` (grep for
existing `MarkersTab`/`TreeListWidget` registrations). Current line counts confirmed via `wc -l`
immediately before drafting (all files named in §6 above), not assumed from STEP120's own quoted
figures.

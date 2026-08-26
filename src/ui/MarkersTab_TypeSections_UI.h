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

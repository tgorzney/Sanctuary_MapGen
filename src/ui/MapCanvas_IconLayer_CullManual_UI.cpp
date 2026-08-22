// MapCanvas_IconLayer_CullManual_UI.cpp — §1 item 4: one hand-authored (Manual) sub-layer's
// candidate instances. Layer: UI. Pure, imgui-free, headless-testable. No grid needed at authoring-
// list scale (manual layers have no locality guarantee a grid could exploit anyway) — every
// instance gets the same per-instance world-rect test the procedural walker applies.
//
// Units: ARCH_14_04_NestedUnitGroupAddressing.md §14.4 — nested UnitGroup.groups draw as part of
// their top-level parent, never separately addressable; Application_Defaults_UI.h's
// ResolveUnitsManualSubLayer resolves the flat OverlaySubLayerRef_UI::index back to (army, group).
// Props/Decals: PropTransform/DecalTransform::layerIndex is read POSITIONALLY against
// recipe.propLayers/decalLayers's array index (not PropInstanceLayer::layerId) — the two arrays'
// exact relationship is not spelled out anywhere this ticket cites; this is a documented, flagged
// coder choice, not a silent one, consistent with STEP51's own "indexed by ...layerIndex" phrasing.
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "Application_Defaults_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

void ConsiderManualInstance(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                            int layerIndex, const std::string& templateIdentifier,
                            float worldX, float worldZ, float scale,
                            PlacementCollectionKind_UI collection, std::int32_t instanceIndex,
                            int* stableOrderCounter, LayerWorldAabb_UI* outAabb,
                            const ViewWorldRect_UI* viewRect, IconLayerCullDiagnostics_UI* diagnostics,
                            std::vector<OverlayVisibleInstance>& outCandidates) {
    if (outAabb != nullptr) WidenAabb(*outAabb, worldX, worldZ);
    if (viewRect == nullptr) return;
    if (worldX < viewRect->lowWorldX || worldX > viewRect->highWorldX
        || worldZ < viewRect->lowWorldZ || worldZ > viewRect->highWorldZ)
        return;
    EmitCandidateIfVisible(input, layer, layerIndex, templateIdentifier, worldX, worldZ, scale,
                           collection, instanceIndex, stableOrderCounter, diagnostics, outCandidates);
}

// "UI/Sprites/.../<tpId>.dds" -> "<tpId>", mirroring Application_Assets_UI.cpp's FileStemOfEntryName
// (that helper is anonymous-namespace-local there; duplicated here rather than exported, since it
// is a five-line mechanical string op, not a policy this module should own a shared copy of).
std::string TemplateIdentifierFromBlueprintPath(const std::string& blueprintPath) {
    const std::size_t lastSeparator = blueprintPath.find_last_of("/\\");
    const std::size_t stemBegin = lastSeparator == std::string::npos ? 0 : lastSeparator + 1;
    const std::size_t lastDot = blueprintPath.find_last_of('.');
    const std::size_t stemEnd = (lastDot == std::string::npos || lastDot < stemBegin) ? blueprintPath.size() : lastDot;
    return blueprintPath.substr(stemBegin, stemEnd - stemBegin);
}

void CollectUnitGroupInstances(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                               int layerIndex, const Params::UnitGroup& group, int* stableOrderCounter,
                               LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                               IconLayerCullDiagnostics_UI* diagnostics,
                               std::vector<OverlayVisibleInstance>& outCandidates) {
    for (std::size_t unitIndex = 0; unitIndex < group.units.size(); ++unitIndex) {
        const Params::UnitTransform& unit = group.units[unitIndex];
        ConsiderManualInstance(input, layer, layerIndex, TemplateIdentifierToString8(unit.templateIdentifier),
                               unit.positionX, unit.positionZ, unit.scaleX,
                               PlacementCollectionKind_UI::Units, static_cast<std::int32_t>(unitIndex),
                               stableOrderCounter, outAabb, viewRect, diagnostics, outCandidates);
    }
    for (const Params::UnitGroup& childGroup : group.groups)
        CollectUnitGroupInstances(input, layer, layerIndex, childGroup, stableOrderCounter, outAabb,
                                  viewRect, diagnostics, outCandidates);
}

void ResolveUnitsManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                        int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                        LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                        IconLayerCullDiagnostics_UI* diagnostics,
                        std::vector<OverlayVisibleInstance>& outCandidates) {
    int armyIndex = -1, groupIndex = -1;
    if (!ResolveUnitsManualSubLayer(*input.recipe, subLayerArrayIndex, armyIndex, groupIndex)) return;
    const Params::Army& army = input.recipe->armies[static_cast<std::size_t>(armyIndex)];
    CollectUnitGroupInstances(input, layer, layerIndex, army.groups[static_cast<std::size_t>(groupIndex)],
                              stableOrderCounter, outAabb, viewRect, diagnostics, outCandidates);
}

void ResolvePropsManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                        int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                        LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                        IconLayerCullDiagnostics_UI* diagnostics,
                        std::vector<OverlayVisibleInstance>& outCandidates) {
    for (const Params::PropInstanceGroup& group : input.recipe->props) {
        const std::string templateIdentifier = TemplateIdentifierFromBlueprintPath(group.blueprintPath);
        for (std::size_t index = 0; index < group.transforms.size(); ++index) {
            const Params::PropTransform& propTransform = group.transforms[index];
            if (propTransform.layerIndex != subLayerArrayIndex) continue;
            ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                                   propTransform.transform.positionX, propTransform.transform.positionZ,
                                   propTransform.transform.scaleX, PlacementCollectionKind_UI::Props,
                                   static_cast<std::int32_t>(index), stableOrderCounter, outAabb, viewRect,
                                   diagnostics, outCandidates);
        }
    }
}

void ResolveDecalsManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                         int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                         LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                         IconLayerCullDiagnostics_UI* diagnostics,
                         std::vector<OverlayVisibleInstance>& outCandidates) {
    for (const Params::DecalInstanceGroup& group : input.recipe->decals) {
        const std::string templateIdentifier = TemplateIdentifierFromBlueprintPath(group.blueprintPath);
        for (std::size_t index = 0; index < group.transforms.size(); ++index) {
            const Params::DecalTransform& decalTransform = group.transforms[index];
            if (decalTransform.layerIndex != subLayerArrayIndex) continue;
            ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                                   decalTransform.transform.positionX, decalTransform.transform.positionZ,
                                   decalTransform.transform.scaleX, PlacementCollectionKind_UI::Decals,
                                   static_cast<std::int32_t>(index), stableOrderCounter, outAabb, viewRect,
                                   diagnostics, outCandidates);
        }
    }
}

} // namespace

void ResolveManualSubLayer(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                           int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                           LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                           IconLayerCullDiagnostics_UI* diagnostics,
                           std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.recipe == nullptr) return;
    if (diagnostics != nullptr) ++diagnostics->subLayerWalksIssued;
    switch (layer.domainKind) {
        case OverlayDomainKind_UI::Units:
            ResolveUnitsManual(input, layer, layerIndex, subLayerArrayIndex, stableOrderCounter, outAabb,
                               viewRect, diagnostics, outCandidates);
            return;
        case OverlayDomainKind_UI::Props:
            ResolvePropsManual(input, layer, layerIndex, subLayerArrayIndex, stableOrderCounter, outAabb,
                               viewRect, diagnostics, outCandidates);
            return;
        case OverlayDomainKind_UI::Decals:
            ResolveDecalsManual(input, layer, layerIndex, subLayerArrayIndex, stableOrderCounter, outAabb,
                                viewRect, diagnostics, outCandidates);
            return;
        default: return;   // Alloy/SpawnsArmies/Reclaim carry no Manual sub-layers this sequence
    }
}

} // namespace Ui
} // namespace SanmapGen

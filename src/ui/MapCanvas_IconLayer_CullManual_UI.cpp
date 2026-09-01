// MapCanvas_IconLayer_CullManual_UI.cpp — §1 item 4: one hand-authored (Manual) sub-layer's
// candidate instances. Layer: UI. Pure, imgui-free, headless-testable. No grid needed at authoring-
// list scale (manual layers have no locality guarantee a grid could exploit anyway) — every
// instance gets the same per-instance world-rect test the procedural walker applies.
//
// Units: ARCH_14_04_NestedUnitGroupAddressing.md §14.4 — nested UnitGroup.groups draw as part of
// their top-level parent, never separately addressable; Application_Defaults_UI.h's
// ResolveUnitsManualSubLayer resolves the flat OverlaySubLayerRef_UI::index back to (army, group).
// Props/Decals: PropTransform/DecalTransform::layerIndex is resolved to its owning layer's stable
// `layerId` (Params::ResolvePropInstanceLayerId/ResolveDecalInstanceLayerId, `PropInstance_PARAMS.h`)
// and matched against the target sub-layer's own layerId, resolved the same way from
// `subLayerArrayIndex` — the positional layerIndex-vs-subLayerArrayIndex comparison this file used
// to make was a documented, flagged exception (ARCH_14_15_ManualCullStableIdMigration.md), now
// retired in favor of the stable-id standard the InstanceId game-load test de-risked. Same live
// PARAMS read timing as before (zero DAG coupling, zero staleness); only the match key changed.
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "Application_Defaults_UI.h"
#include "MapCanvas_SelectionSet_UI.h"   // STEP240 — SelectionSetContains, for the selected-scale fold-in
#include "MarkersTab_MarkerLinkInstanceResolvers_UI.h"   // STEP246 — per-instance Hidden/IconScale/Color
#include "MarkerTypeVisibility_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/Army_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

void ConsiderManualInstance(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                            int layerIndex, const std::string& templateIdentifier,
                            float worldX, float worldZ, float scale,
                            PlacementCollectionKind_UI collection, std::int32_t instanceIndex,
                            float tintColorRed, float tintColorGreen, float tintColorBlue,
                            int* stableOrderCounter, LayerWorldAabb_UI* outAabb,
                            const ViewWorldRect_UI* viewRect, IconLayerCullDiagnostics_UI* diagnostics,
                            std::vector<OverlayVisibleInstance>& outCandidates, bool bManual = false) {
    if (outAabb != nullptr) WidenAabb(*outAabb, worldX, worldZ);
    if (viewRect == nullptr) return;
    if (worldX < viewRect->lowWorldX || worldX > viewRect->highWorldX
        || worldZ < viewRect->lowWorldZ || worldZ > viewRect->highWorldZ)
        return;
    EmitCandidateIfVisible(input, layer, layerIndex, templateIdentifier, worldX, worldZ, scale,
                           collection, instanceIndex, tintColorRed, tintColorGreen, tintColorBlue,
                           stableOrderCounter, diagnostics, outCandidates, bManual);
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

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-C: tint reads Params::Army::armyColor directly — the
// owning army's real color, resolved once by the caller and threaded down, not
// OverlaySessionAppearance::unitsAppearance.color (retired for this purpose).
void CollectUnitGroupInstances(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                               int layerIndex, const Params::UnitGroup& group,
                               float tintColorRed, float tintColorGreen, float tintColorBlue,
                               int* stableOrderCounter, LayerWorldAabb_UI* outAabb,
                               const ViewWorldRect_UI* viewRect,
                               IconLayerCullDiagnostics_UI* diagnostics,
                               std::vector<OverlayVisibleInstance>& outCandidates) {
    for (std::size_t unitIndex = 0; unitIndex < group.units.size(); ++unitIndex) {
        const Params::UnitTransform& unit = group.units[unitIndex];
        ConsiderManualInstance(input, layer, layerIndex, TemplateIdentifierToString8(unit.templateIdentifier),
                               unit.positionX, unit.positionZ, unit.scaleX,
                               PlacementCollectionKind_UI::Units, static_cast<std::int32_t>(unitIndex),
                               tintColorRed, tintColorGreen, tintColorBlue,
                               stableOrderCounter, outAabb, viewRect, diagnostics, outCandidates);
    }
    for (const Params::UnitGroup& childGroup : group.groups)
        CollectUnitGroupInstances(input, layer, layerIndex, childGroup,
                                  tintColorRed, tintColorGreen, tintColorBlue, stableOrderCounter, outAabb,
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
                              army.armyColor[0], army.armyColor[1], army.armyColor[2],
                              stableOrderCounter, outAabb, viewRect, diagnostics, outCandidates);
}

// STEP83 §5/Item 2: Props/Reclaim partition by bReclaimable, evaluated ONCE PER GROUP — never per
// PropTransform, never inside the PrimReserve/PrimWrite region (§14.9); a skipped group is en bloc.
void ResolvePropsManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                        int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                        LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                        IconLayerCullDiagnostics_UI* diagnostics,
                        std::vector<OverlayVisibleInstance>& outCandidates) {
    const bool bWantReclaimable = (layer.domainKind == OverlayDomainKind_UI::Reclaim);
    const int targetLayerId = Params::ResolvePropInstanceLayerId(subLayerArrayIndex, input.recipe->propLayers);
    float layerTintRed = 1.0f, layerTintGreen = 1.0f, layerTintBlue = 1.0f;
    Params::ResolvePropInstanceLayerColor(subLayerArrayIndex, input.recipe->propLayers,
                                          layerTintRed, layerTintGreen, layerTintBlue);
    for (const Params::PropInstanceGroup& group : input.recipe->props) {
        if (diagnostics != nullptr) ++diagnostics->reclaimGroupPredicateEvaluations;
        if (group.bReclaimable != bWantReclaimable) continue;   // en bloc, before any transform
        const std::string templateIdentifier = TemplateIdentifierFromBlueprintPath(group.blueprintPath);
        for (std::size_t index = 0; index < group.transforms.size(); ++index) {
            const Params::PropTransform& propTransform = group.transforms[index];
            if (Params::ResolvePropInstanceLayerId(propTransform.layerIndex, input.recipe->propLayers) != targetLayerId) continue;
            // ARCH §21.4 — the selection key is `propTransform.instanceIdentifier` (globally
            // unique within Props, minted, never reused), NOT the per-group `index` above — the
            // same array-position/stable-identity collision §19.25 already fixed for Markers.
            // `bManual=true` tags the key so a procedural array-position key of the same numeric
            // value never compares equal to it.
            ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                                   propTransform.transform.positionX, propTransform.transform.positionZ,
                                   propTransform.transform.scaleX, PlacementCollectionKind_UI::Props,
                                   propTransform.instanceIdentifier, layerTintRed, layerTintGreen, layerTintBlue,
                                   stableOrderCounter, outAabb, viewRect,
                                   diagnostics, outCandidates, /*bManual=*/true);
        }
    }
}

} // namespace

// STEP114 §4b — the manual-marker resolver Alloy/SpawnsArmies dead-end into today. Partitions by
// the owning group's name, mirroring ResolvePropsManual's bReclaimable en-bloc gate above —
// evaluated once per GROUP, not per transform. ARCH §19.25: declared non-anonymous (see
// MapCanvas_IconLayer_CullInternal_UI.h) so the C2 cache's replay-frame path can scope a walk to one
// `targetInstanceIdentifier`.
void ResolveMarkersManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                          int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                          LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                          IconLayerCullDiagnostics_UI* diagnostics,
                          std::vector<OverlayVisibleInstance>& outCandidates,
                          const int* targetInstanceIdentifier) {
    const bool bWantSpawnGroups = (layer.domainKind == OverlayDomainKind_UI::SpawnsArmies);
    // subLayerArrayIndex/markerLayers are invariant for the whole function — only the RESOLVED
    // (Hidden/color-override/iconScale) reads move per-instance below (STEP246), not this lookup.
    const bool bSubLayerInRange = subLayerArrayIndex >= 0
        && static_cast<std::size_t>(subLayerArrayIndex) < input.recipe->markerLayers.size();
    const Params::MarkerInstanceLayer* const subLayer =
        bSubLayerInRange ? &input.recipe->markerLayers[static_cast<std::size_t>(subLayerArrayIndex)] : nullptr;
    for (const Params::MarkerInstanceGroup& group : input.recipe->markers) {
        // STEP133 — the per-Type Hide/Unhide preview filter, gated at group level (the group's own
        // `name` IS the marker Type name, MarkerInstance_PARAMS.h), mirroring ResolvePropsManual's
        // bReclaimable en-bloc gate above: evaluated once per GROUP, never per transform.
        if (input.markerTypeVisibility != nullptr && input.markerTypeVisibility->IsHidden(group.name)) continue;
        const bool bIsSpawnGroup = group.name == Params::kSpawnMarkerGroupName;
        if (bIsSpawnGroup != bWantSpawnGroups) continue;
        const float groupTypeScale = Params::ResolveMarkerGroupTypeScale(group.name, input.recipe->globalMarkerSettings);   // STEP122
        for (std::size_t index = 0; index < group.transforms.size(); ++index) {
            const Params::MarkerTransform& transform = group.transforms[index];
            // Positional match — MarkerTransform::layerIndex is NOT resolved through a stable
            // layerId indirection the way Props/Decals now are (this file's own header comment,
            // lines 9-15); markers never got that migration, and SeedMarkerDomains
            // (Application_OverlaySetup_Seed_UI.cpp) seeds subLayerArrayIndex as the SAME plain
            // recipe.markerLayers position layerIndex already uses everywhere else in the marker
            // domain (IsMarkerInstanceLayerLocked, QuantizeMarkerPositionToLayerGrid).
            if (transform.layerIndex != subLayerArrayIndex) continue;
            // ARCH §19.25 — the scoped single-instance resolve (C2 cache replay path): skip every
            // transform except the one target, when a target is given.
            if (targetInstanceIdentifier != nullptr && transform.instanceIdentifier != *targetInstanceIdentifier)
                continue;
            // STEP246, ARCH §19.33/§21.9 — Hidden/color-override/iconScale are now resolved PER
            // INSTANCE (transform-tier-first, then Layer-tier), moved INSIDE this loop from a
            // once-per-Layer hoist: different transforms on the same Layer can resolve differently
            // once any one of them is Link-tagged (that's the whole point of instance-tier tagging;
            // a Layer's own raw bHidden is no longer a valid "skip the whole Layer" shortcut). This
            // is a deliberate perf-shape change (hoisted-once -> per-instance) — negligible at
            // authoring-scale instance counts, not an accidental regression.
            if (subLayer != nullptr && EffectiveManualMarkerInstanceHidden(transform, *subLayer, input.recipe->markerLinks))
                continue;
            float groupTintRed = 1.0f, groupTintGreen = 1.0f, groupTintBlue = 1.0f;
            float layerIconScale = 1.0f;
            if (subLayer != nullptr) {
                layerIconScale = EffectiveManualMarkerInstanceIconScale(transform, *subLayer, input.recipe->markerLinks);
                if (EffectiveManualMarkerInstanceColorOverrideEnabled(transform, *subLayer, input.recipe->markerLinks)) {
                    const float* const overrideColor =
                        EffectiveManualMarkerInstanceColor(transform, *subLayer, input.recipe->markerLinks);
                    groupTintRed = overrideColor[0]; groupTintGreen = overrideColor[1]; groupTintBlue = overrideColor[2];
                } else {
                    Params::ResolveMarkerGroupTypeTintColor(group.name, input.recipe->globalMarkerSettings,
                                                             groupTintRed, groupTintGreen, groupTintBlue);
                }
            } else {
                Params::ResolveMarkerGroupTypeTintColor(group.name, input.recipe->globalMarkerSettings,
                                                         groupTintRed, groupTintGreen, groupTintBlue);
            }
            const std::string templateIdentifier =
                ResolveMarkerIconTemplateIdentifier(transform, group, input.recipe->globalMarkerSettings);
            // ARCH §19.25 — the selection key is `transform.instanceIdentifier` (globally unique,
            // minted, never reused, §19.16), NOT the per-group `index` above (still used for
            // Units/Props/Decals, which have no working picker yet): the two number spaces are
            // unrelated and can collide at the same value under the same `Markers` collection tag,
            // incorrectly lighting up an unrelated manual marker's `bSelected` — the fix this ruling
            // closes. `bManual=true` tags the key so a procedural array-position key of the same
            // numeric value never compares equal to it.
            //
            // ARCH §19.32/STEP240 — the render-consumer fold-in for GlobalMarkerSettings'
            // scaleSelected*: resolved and composed HERE, in this markers-domain-aware function,
            // deliberately NOT threaded through the shared, domain-agnostic
            // EmitCandidateIfVisible/AppendCandidate choke point every collection kind (Units/Props/
            // Decals/Markers) funnels through (MapCanvas_IconLayer_DrawInternal_UI.h's own header
            // comment already rejects giving that function marker-domain knowledge, for the adjacent
            // selectColor* case — the identical reasoning applies here). `bSelected` therefore has to
            // be known at THIS site too, one frame ahead of AppendCandidate's own canonical
            // SelectionSetContains check — not a second, driftable source of truth, since both sites
            // query the same live `input.selectedInstanceKeys` with the same key shape, they simply
            // query it twice in the same frame. When `targetInstanceIdentifier != nullptr` this call
            // is already the C2 cache's own scoped replay-frame resolve for exactly this one
            // instanceIdentifier (MapCanvas_IconLayer_Cull_UI.cpp's ResolveSelectedManualMarkerCandidate),
            // which only ever runs for an instance already known to be selected — no redundant
            // membership query needed in that path.
            const bool bSelected = targetInstanceIdentifier != nullptr
                || (input.selectedInstanceKeys != nullptr
                    && SelectionSetContains(*input.selectedInstanceKeys,
                                            OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers,
                                                                  transform.instanceIdentifier, true, true}));
            const float selectedTypeScale = bSelected
                ? Params::ResolveMarkerGroupSelectedTypeScale(group.name, input.recipe->globalMarkerSettings)
                : 1.0f;
            ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                                   transform.transform.positionX, transform.transform.positionZ,
                                   // STEP122 groupTypeScale/layerIconScale, STEP240 selectedTypeScale
                                   transform.transform.scaleX * groupTypeScale * layerIconScale * selectedTypeScale,
                                   PlacementCollectionKind_UI::Markers, transform.instanceIdentifier,
                                   groupTintRed, groupTintGreen, groupTintBlue,   // STEP116
                                   stableOrderCounter, outAabb, viewRect, diagnostics, outCandidates,
                                   /*bManual=*/true);
        }
    }
}

namespace {

void ResolveDecalsManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                         int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                         LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                         IconLayerCullDiagnostics_UI* diagnostics,
                         std::vector<OverlayVisibleInstance>& outCandidates) {
    const int targetLayerId = Params::ResolveDecalInstanceLayerId(subLayerArrayIndex, input.recipe->decalLayers);
    float layerTintRed = 1.0f, layerTintGreen = 1.0f, layerTintBlue = 1.0f;
    Params::ResolveDecalInstanceLayerColor(subLayerArrayIndex, input.recipe->decalLayers,
                                           layerTintRed, layerTintGreen, layerTintBlue);
    for (const Params::DecalInstanceGroup& group : input.recipe->decals) {
        const std::string templateIdentifier = TemplateIdentifierFromBlueprintPath(group.blueprintPath);
        for (std::size_t index = 0; index < group.transforms.size(); ++index) {
            const Params::DecalTransform& decalTransform = group.transforms[index];
            if (Params::ResolveDecalInstanceLayerId(decalTransform.layerIndex, input.recipe->decalLayers) != targetLayerId) continue;
            // ARCH §21.4 — same array-position/stable-identity fix as ResolvePropsManual above.
            ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                                   decalTransform.transform.positionX, decalTransform.transform.positionZ,
                                   decalTransform.transform.scaleX, PlacementCollectionKind_UI::Decals,
                                   decalTransform.instanceIdentifier, layerTintRed, layerTintGreen, layerTintBlue,
                                   stableOrderCounter, outAabb, viewRect,
                                   diagnostics, outCandidates, /*bManual=*/true);
        }
    }
}

} // namespace

// STEP114 §4a — type-default resolution, mirroring v1's exact order (Widget_MapCanvas.cpp:341-370):
// override wins if set, else map the owning group's name to the matching GlobalMarkerSettings
// field, else fall back to the raw group name (a miss on that just logs-once-and-draws-nothing,
// the same posture every other unresolved templateIdentifier already gets in this file). Declared
// non-anonymous (MapCanvas_IconLayer_CullInternal_UI.h), mirroring ResolveMarkerCategoryTintColor's
// own posture, so it is directly unit-testable.
std::string ResolveMarkerIconTemplateIdentifier(const Params::MarkerTransform& transform,
                                                const Params::MarkerInstanceGroup& group,
                                                const Params::GlobalMarkerSettings& globalMarkerSettings) {
    if (!transform.iconNameOverride.empty()) return transform.iconNameOverride;
    if (group.name == Params::kSpawnMarkerGroupName || group.name == "Spawns")
        return globalMarkerSettings.iconNameSpawn;
    if (group.name == "Alloy" || group.name == "Alloys")
        return globalMarkerSettings.iconNameAlloy;
    if (group.name == "Plasma" || group.name == "Plasmas")
        return globalMarkerSettings.iconNamePlasma;
    return group.name;   // v1 Widget_MapCanvas.cpp:341 precedent — raw type name as last resort
}

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
        case OverlayDomainKind_UI::Reclaim:   // STEP62/STEP83 bReclaimable partition; group-level filter above
            ResolvePropsManual(input, layer, layerIndex, subLayerArrayIndex, stableOrderCounter, outAabb,
                               viewRect, diagnostics, outCandidates);
            return;
        case OverlayDomainKind_UI::Decals:
            ResolveDecalsManual(input, layer, layerIndex, subLayerArrayIndex, stableOrderCounter, outAabb,
                                viewRect, diagnostics, outCandidates);
            return;
        case OverlayDomainKind_UI::Alloy:
        case OverlayDomainKind_UI::SpawnsArmies:   // STEP114 — the manual roster's own icon render consumer
            ResolveMarkersManual(input, layer, layerIndex, subLayerArrayIndex, stableOrderCounter, outAabb,
                                 viewRect, diagnostics, outCandidates);
            return;
        default: return;
    }
}

} // namespace Ui
} // namespace SanmapGen

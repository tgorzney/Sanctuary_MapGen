// MapCanvas_IconLayer_TestFixture_UI_Test.h — the synthetic scene STEP53's whole test binary
// shares: a composed (CPU, headless) composite + view showing the whole 4x4 test world at zoom 1,
// plus small builders for a Data::PlacementResults instance and an atlas-pairing entry. Test-support
// only; mirrors PreviewComposite_TestScene_UI.h's own precedent (a shared, header-only fixture, not
// a duplicated setup per translation unit).
#pragma once
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "IconAtlasPairing_UI.h"
#include "IconGridWidget_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include "../io/WorldFootprintSizeTable_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Ui {

inline void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

// World (2,2) is comfortably inside the view rect at zoom 1; world (1000,1000) is comfortably
// outside it.
struct IconLayerTestFixture {
    PreviewTestScene scene;
    PreviewComposite* composite;
    MapCanvasView view;
    Data::PlacementResults placements;
    Data::RuleBucketIndexSet ruleBucketIndex;
    OverlayLayerSettings overlaySettings;
    OverlayRenderingSettings renderingSettings;
    IconAtlasPairingLookup pairingLookup;
    IconAtlasManifest atlasManifest;
    Io::WorldFootprintSizeTable footprintTable;
    Params::MapRecipe recipe;
    IconLayerAabbCache_UI aabbCache;
    IconLayerFrameCache frameCache;

    IconLayerTestFixture() {
        BuildPreviewTestScene(scene);
        composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                         scene.instances, scene.entityIdentifiers);
        ConfigurePreviewSettings(composite->Settings());
        composite->ComposeOnCpu();
        view.SetPreviewResolution(composite->Resolution());
        view.SetRegionSide(256.0f);
    }
    ~IconLayerTestFixture() { delete composite; }
    IconLayerTestFixture(const IconLayerTestFixture&) = delete;
    IconLayerTestFixture& operator=(const IconLayerTestFixture&) = delete;

    DrawOverlayIconLayersInput Input() const {
        DrawOverlayIconLayersInput input;
        input.overlayLayerSettings = &overlaySettings;
        input.renderingSettings = &renderingSettings;
        input.placements = &placements;
        input.ruleBucketIndex = &ruleBucketIndex;
        input.recipe = &recipe;
        input.pairingLookup = &pairingLookup;
        input.atlasManifest = &atlasManifest;
        input.footprintSizeTable = &footprintTable;
        input.composite = composite;
        input.view = &view;
        input.regionSidePixels = 256.0f;
        return input;
    }
};

inline void AppendPropInstance(Data::PlacementResults& placements, float worldX, float worldZ,
                               int ruleIndex, const char* templateIdentifier, float scale = 1.0f) {
    Data::PlacementInstance instance;
    instance.positionX = worldX; instance.positionZ = worldZ;
    instance.scaleX = instance.scaleY = instance.scaleZ = scale;
    instance.ruleIndex = ruleIndex;
    instance.templateIdentifier = Data::MakeTemplateIdentifier(templateIdentifier);
    placements.props.Append(instance);
}

inline void AppendMarkerInstance(Data::PlacementResults& placements, float worldX, float worldZ,
                                 int ruleIndex, Params::MarkerCategory category,
                                 const char* templateIdentifier, float scale = 1.0f) {
    Data::PlacementInstance instance;
    instance.positionX = worldX; instance.positionZ = worldZ;
    instance.scaleX = instance.scaleY = instance.scaleZ = scale;
    instance.ruleIndex = ruleIndex;
    instance.category = static_cast<int>(category);
    instance.templateIdentifier = Data::MakeTemplateIdentifier(templateIdentifier);
    placements.markers.Append(instance);
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-B: armyIndex is a real, per-instance field
// (Data::PlacementInstance::armyIndex, "units only; -1 for markers/props/decals") — a procedural
// units fixture must set it explicitly, unlike AppendPropInstance/AppendMarkerInstance's domains.
inline void AppendUnitInstance(Data::PlacementResults& placements, float worldX, float worldZ,
                               int ruleIndex, int armyIndex, const char* templateIdentifier,
                               float scale = 1.0f) {
    Data::PlacementInstance instance;
    instance.positionX = worldX; instance.positionZ = worldZ;
    instance.scaleX = instance.scaleY = instance.scaleZ = scale;
    instance.ruleIndex = ruleIndex;
    instance.armyIndex = armyIndex;
    instance.templateIdentifier = Data::MakeTemplateIdentifier(templateIdentifier);
    placements.units.Append(instance);
}

inline void SeedAtlasEntry(IconAtlasPairingLookup& pairingLookup, IconAtlasManifest& manifest,
                          const std::string& templateIdentifier, int iconId, int atlasPage = 0) {
    pairingLookup.SetThumbnailIconId(templateIdentifier, iconId);
    while (static_cast<int>(manifest.entries.size()) <= iconId) {
        IconAtlasEntry entry; entry.iconId = static_cast<int>(manifest.entries.size());
        manifest.entries.push_back(entry);
    }
    manifest.entries[static_cast<std::size_t>(iconId)].atlasPage = atlasPage;
    while (static_cast<int>(manifest.pageTextureIdentifiers.size()) <= atlasPage)
        manifest.pageTextureIdentifiers.push_back(
            static_cast<std::uint64_t>(manifest.pageTextureIdentifiers.size()) + 1u);
}

} // namespace Ui
} // namespace SanmapGen

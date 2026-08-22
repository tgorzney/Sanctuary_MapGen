// MapImporter_IO_Test.cpp — acceptance test for "Load Sanmap" (section D / PARITY_BACKLOG PB-1).
// This unit holds the binary's main(), the fixture recipe, and the headline check: a populated
// MapRecipe written by MapExporter and read back by MapImporter compares equal field for field.
// The validation and baked-field halves live in the two sibling units (MapFormat_TestSupport_IO.h).
#include "MapFormat_TestSupport_IO.h"
#include "MapImporter_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "Sanmap_KnownTopLevelKeys_IO.h"

namespace SanmapGen {
namespace MapFormatTest {
namespace {

// STEP25_MapNameCredits_IO acceptance test item 1: `mapName`/`mapCredits` are flat document-root
// fields (SANMAP_FORMAT_SPEC "Base") that now live on the recipe itself, not on the export-only
// `MapExportOptions` — a non-default mapName and a genuinely EMPTY mapCredits both survive exactly.
void CheckMapNameAndCredits(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.mapName == original.mapName && original.mapName == "Nomad's Crossing",
          "mapName survives the round trip, exercising a non-default value");
    Check(loaded.mapCredits == original.mapCredits && original.mapCredits.empty(),
          "an empty mapCredits survives verbatim — legitimate real content, not a gap");
}

void CheckGeometryAndWater(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.geometry.mapSize == original.geometry.mapSize, "mapSize survives");
    Check(loaded.geometry.seed == original.geometry.seed, "seed survives");
    Check(NearlyEqual(loaded.geometry.terrainMinHeight, original.geometry.terrainMinHeight),
          "terrainMinHeight survives");
    Check(NearlyEqual(loaded.geometry.terrainMaxHeight, original.geometry.terrainMaxHeight),
          "terrainMaxHeight survives");
    Check(loaded.geometry.bScaleFeaturesToMapSize == original.geometry.bScaleFeaturesToMapSize,
          "scale-features-to-map-size survives");
    Check(NearlyEqual(loaded.geometry.worldUnitsPerCell, original.geometry.worldUnitsPerCell),
          "worldUnitsPerCell survives");
    Check(loaded.globalSymmetryMask == original.globalSymmetryMask,
          "the global symmetry mask survives");
    Check(loaded.water.bEnabled == original.water.bEnabled
          && NearlyEqual(loaded.water.waterLevelMaximum, original.water.waterLevelMaximum)
          && NearlyEqual(loaded.water.deepWaterDepthMinimum, original.water.deepWaterDepthMinimum)
          && NearlyEqual(loaded.water.deepWaterDepthMaximum, original.water.deepWaterDepthMaximum),
          "the whole water block survives");
}

// SANMAP_FORMAT_SPEC Correction 2: `GlobalGravity`, the one genuinely new field
// (`Params::GeneralMapSettings`, not a rival store for per-stratum gravity — see that header's own
// comment). The other 4 `GeneralMapSettings` fields (Seed/ScaleFeaturesToMapSize/TerrainMinHeight/
// WorldUnitsPerCell) are already covered by CheckGeometryAndWater above — they still land on
// `recipe.geometry`, only their WIRE location moved.
void CheckGeneralMapSettings(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(NearlyEqual(loaded.generalMapSettings.globalGravity, original.generalMapSettings.globalGravity),
          "globalGravity survives through the new top-level GeneralMapSettings object");
}

// SANMAP_FORMAT_SPEC Correction 2: `Seed`/`ScaleFeaturesToMapSize`/`TerrainMinHeight`/
// `WorldUnitsPerCell` are RELOCATED, not dual-written, into the top-level `GeneralMapSettings`
// object. STEP36_LegacyBlobDeletion_IO: a fresh export no longer writes `mapGeneratorData` at ALL
// (TestDocumentCarriesTheFormatsOwnFields, MapExporter_IO_Test.cpp, covers that globally), so the
// narrower "not nested inside mapGeneratorData" check this used to make is now moot — confirmed
// trivially by the blob's total absence — and the MapSize/TerrainMaxHeight legacy-blob regression
// guard this used to run is gone with it (both values still round-trip via their own top-level
// homes — CheckGeometryAndWater/CheckGeneralMapSettings — untouched by this ticket).
void CheckGeneralMapSettingsTopLevelNotNested(const std::string& documentText) {
    const nlohmann::json document = nlohmann::json::parse(documentText);
    Check(document.contains("GeneralMapSettings") && document["GeneralMapSettings"].is_object(),
          "GeneralMapSettings appears as its own top-level object");
}

// SANMAP_FORMAT_SPEC Correction 3: `HeightmapStack` — `simulationGrouping`, and every existing
// `GeoLayer`/`Layer` field (the bulk of the content, a pure relocation that must not silently drop
// or corrupt any of it), plus the two genuinely new symmetry-override fields on both levels,
// non-default AND distinct between the two levels (catches a field mix-up between them).
void CheckLayerStackAndRules(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.layerStack.simulationGrouping == original.layerStack.simulationGrouping
          && original.layerStack.simulationGrouping == Params::SimulationGrouping::Separate,
          "simulationGrouping survives through HeightmapStack, exercising the non-default value");
    Check(loaded.layerStack.geoLayers.size() == original.layerStack.geoLayers.size(),
          "the geo-layer count survives");
    if (!loaded.layerStack.geoLayers.empty()) {
        const Params::GeoLayer& originalGeoLayer = original.layerStack.geoLayers[0];
        const Params::GeoLayer& geoLayer = loaded.layerStack.geoLayers[0];
        Check(geoLayer.name == "Ridges" && geoLayer.bEnabled == originalGeoLayer.bEnabled
              && geoLayer.mode == originalGeoLayer.mode && geoLayer.bErodeBelow
              && geoLayer.blendMode == originalGeoLayer.blendMode && geoLayer.stratumIndex == 2,
              "the geo-layer's own settings survive, all fields");
        Check(originalGeoLayer.bSymmetryUseGlobal == false
              && geoLayer.bSymmetryUseGlobal == originalGeoLayer.bSymmetryUseGlobal
              && geoLayer.symmetryMask == originalGeoLayer.symmetryMask && geoLayer.symmetryMask == 5,
              "the geo-layer's own local symmetry override survives (new field, Correction 3)");
        Check(geoLayer.layers.size() == 1, "and it kept its noise layer");
        if (!geoLayer.layers.empty()) {
            const Params::Layer& originalLayer = originalGeoLayer.layers[0];
            const Params::Layer& layer = geoLayer.layers[0];
            Check(layer.name == "Base Noise" && layer.bEnabled == originalLayer.bEnabled
                  && layer.bLocked == originalLayer.bLocked
                  && layer.stratumIndex == originalLayer.stratumIndex,
                  "the layer's identity/stack-control fields survive");
            Check(layer.noiseType == originalLayer.noiseType
                  && layer.fractalType == originalLayer.fractalType
                  && NearlyEqual(layer.frequency, originalLayer.frequency)
                  && layer.octaves == originalLayer.octaves
                  && NearlyEqual(layer.gain, originalLayer.gain)
                  && NearlyEqual(layer.lacunarity, originalLayer.lacunarity)
                  && NearlyEqual(layer.weightedStrength, originalLayer.weightedStrength)
                  && NearlyEqual(layer.pingPongStrength, originalLayer.pingPongStrength)
                  && NearlyEqual(layer.cellularJitter, originalLayer.cellularJitter),
                  "the layer's noise-source fields survive");
            Check(NearlyEqual(layer.landDensity, originalLayer.landDensity)
                  && NearlyEqual(layer.mountainDensity, originalLayer.mountainDensity)
                  && NearlyEqual(layer.plateauDensity, originalLayer.plateauDensity)
                  && NearlyEqual(layer.rampDensity, originalLayer.rampDensity),
                  "the layer's density-shaping fields survive");
            Check(NearlyEqual(layer.levelsShadows, originalLayer.levelsShadows)
                  && NearlyEqual(layer.levelsMidtones, originalLayer.levelsMidtones)
                  && NearlyEqual(layer.levelsHighlights, originalLayer.levelsHighlights)
                  && NearlyEqual(layer.levelsOutputBlack, originalLayer.levelsOutputBlack)
                  && NearlyEqual(layer.levelsOutputWhite, originalLayer.levelsOutputWhite),
                  "the layer's Levels fields survive");
            Check(layer.blendMode == originalLayer.blendMode
                  && NearlyEqual(layer.opacity, originalLayer.opacity)
                  && NearlyEqual(layer.heightBlendContrast, originalLayer.heightBlendContrast)
                  && NearlyEqual(layer.heightBlendMinimum, originalLayer.heightBlendMinimum)
                  && NearlyEqual(layer.heightBlendMaximum, originalLayer.heightBlendMaximum),
                  "the layer's stack-combine fields survive");
            Check(originalLayer.bSymmetryUseGlobal == false
                  && layer.bSymmetryUseGlobal == originalLayer.bSymmetryUseGlobal
                  && layer.symmetryMask == originalLayer.symmetryMask && layer.symmetryMask == 9,
                  "the layer's own local symmetry override survives, independent of the geo-layer's "
                  "(new field, Correction 3)");
        }
    }
    // Size is no longer 1 here: `ReadStratumLayersJson` (SANMAP_FORMAT_SPEC Correction 13) grows
    // `strata` to the format's fixed 9 layers regardless of how many the fixture populated — see
    // CheckStratumAppearance below, which asserts that cardinality directly. bSlopeGateEnabled/
    // maximumSlopeDegrees now round-trip through the new top-level `StratumGenerationSettings`
    // array (Correction 12, CheckStratumGenerationSettings below) rather than the legacy
    // `mapGeneratorData.Stratums` blob; tileCount still comes through the untouched legacy blob.
    Check(!loaded.strata.empty() && loaded.strata[0].bSlopeGateEnabled
          && NearlyEqual(loaded.strata[0].maximumSlopeDegrees, 55.0f)
          && NearlyEqual(loaded.strata[0].tileCount, 24.0f), "the stratum settings survive");
    Check(loaded.markerRules.size() == 1 && loaded.markerRules[0].count == 8
          && loaded.markerRules[0].symmetryMask == 1, "the marker rules survive");
    Check(loaded.propRules.size() == 1 && loaded.propRules[0].bAvoidWater
          && loaded.propRules[0].bReclaimable
          && loaded.propRules[0].bSymmetryUseGlobal == false && loaded.propRules[0].symmetryMask == 2,
          "the prop rules survive, including the per-rule symmetry override");
    Check(loaded.decalRules.size() == 1 && NearlyEqual(loaded.decalRules[0].spacingMinimum, 6.0f)
          && loaded.decalRules[0].bSymmetryUseGlobal == false && loaded.decalRules[0].symmetryMask == 8,
          "the decal rules survive, including the per-rule symmetry override");
    Check(loaded.unitRules.size() == 1 && loaded.unitRules[0].armyIndex == 2
          && loaded.unitRules[0].count == 5 && loaded.unitRules[0].bSymmetryUseGlobal == false
          && loaded.unitRules[0].symmetryMask == 4,
          "the unit rules survive, including the per-rule symmetry override");
}

// SANMAP_FORMAT_SPEC Correction 3: `SimulationGrouping`/`GeoLayers` are RELOCATED, not dual-written,
// into the top-level `HeightmapStack` object. STEP36_LegacyBlobDeletion_IO: a fresh export no longer
// writes `mapGeneratorData` at ALL (TestDocumentCarriesTheFormatsOwnFields, MapExporter_IO_Test.cpp,
// covers that globally), so the narrower "not nested inside mapGeneratorData" check this used to
// make is now moot, and the MapSize/TerrainMaxHeight legacy-blob regression guard this used to run
// is gone with it (both values still round-trip via their own top-level homes, untouched).
void CheckHeightmapStackTopLevelNotNested(const std::string& documentText) {
    const nlohmann::json document = nlohmann::json::parse(documentText);
    Check(document.contains("HeightmapStack") && document["HeightmapStack"].is_object(),
          "HeightmapStack appears as its own top-level object");
    Check(document["HeightmapStack"].contains("SimulationGrouping")
          && document["HeightmapStack"].contains("GeoLayers")
          && document["HeightmapStack"]["GeoLayers"].is_array(),
          "HeightmapStack carries both SimulationGrouping and the GeoLayers array");
}

// SANMAP_FORMAT_SPEC Correction 4 (STEP16): `GlobalSymmetryMask` is RELOCATED, not dual-written,
// into the new top-level `Symmetry` object, carrying all 10 Correction-4 fields this ticket writes
// (`SymAlgorithm` is explicitly out of scope — ruling #1 — and is not checked here).
// STEP36_LegacyBlobDeletion_IO: a fresh export no longer writes `mapGeneratorData` at ALL
// (TestDocumentCarriesTheFormatsOwnFields, MapExporter_IO_Test.cpp, covers that globally), so the
// narrower "not nested inside mapGeneratorData" check this used to make is now moot.
void CheckSymmetryTopLevelNotNested(const std::string& documentText) {
    const nlohmann::json document = nlohmann::json::parse(documentText);
    Check(document.contains("Symmetry") && document["Symmetry"].is_object(),
          "Symmetry appears as its own top-level object");
    const nlohmann::json& symmetry = document["Symmetry"];
    Check(symmetry.contains("GlobalSymmetryMask") && symmetry.contains("RadialSymmetryRepeatCount")
          && symmetry.contains("SnapImperfectSymmetry") && symmetry.contains("SymmetryDetectionTolerance")
          && symmetry.contains("SymSuperpositionBlend") && symmetry.contains("SymmetryBlurRadius")
          && symmetry.contains("CrossFadeWidth") && symmetry.contains("CylinderZScale")
          && symmetry.contains("TorusMajorRadius") && symmetry.contains("TorusMinorRadius"),
          "Symmetry carries all 10 Correction-4 fields this ticket writes");
}

// STEP16_SymmetryGlobalSettings_IO: `recipe.radialSymmetryRepeatCount`/`symmetryDetection`/
// `symmetryBlend`, plus the per-rule/per-layer `radialSymmetryRepeatCount` sibling of
// `SymmetryMask` on each of MarkerRule/PropRule/DecalRule/UnitRule/GeoLayer/Layer — every value
// non-default, so a round-trip bug in any single one is caught.
void CheckSymmetryFields(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.radialSymmetryRepeatCount == original.radialSymmetryRepeatCount
          && original.radialSymmetryRepeatCount == 5,
          "recipe.radialSymmetryRepeatCount survives through the new top-level Symmetry section");
    Check(loaded.symmetryDetection.bSnapImperfectSymmetry == original.symmetryDetection.bSnapImperfectSymmetry
          && original.symmetryDetection.bSnapImperfectSymmetry == true
          && NearlyEqual(loaded.symmetryDetection.detectionTolerance,
                        original.symmetryDetection.detectionTolerance),
          "recipe.symmetryDetection survives, exercising the non-default snap path");
    const Params::SymmetryBlend& originalBlend = original.symmetryBlend;
    const Params::SymmetryBlend& loadedBlend = loaded.symmetryBlend;
    Check(NearlyEqual(loadedBlend.superpositionBlend, originalBlend.superpositionBlend)
          && NearlyEqual(loadedBlend.blurRadius, originalBlend.blurRadius)
          && NearlyEqual(loadedBlend.crossFadeWidth, originalBlend.crossFadeWidth)
          && NearlyEqual(loadedBlend.cylinderZScale, originalBlend.cylinderZScale)
          && NearlyEqual(loadedBlend.torusMajorRadius, originalBlend.torusMajorRadius)
          && NearlyEqual(loadedBlend.torusMinorRadius, originalBlend.torusMinorRadius),
          "recipe.symmetryBlend survives, all six exotic-blend scalars");

    Check(!loaded.markerRules.empty()
          && loaded.markerRules[0].radialSymmetryRepeatCount == original.markerRules[0].radialSymmetryRepeatCount
          && original.markerRules[0].radialSymmetryRepeatCount == 6,
          "MarkerRule::radialSymmetryRepeatCount survives, sibling of SymmetryMask");
    Check(!loaded.propRules.empty()
          && loaded.propRules[0].radialSymmetryRepeatCount == original.propRules[0].radialSymmetryRepeatCount
          && original.propRules[0].radialSymmetryRepeatCount == 7,
          "PropRule::radialSymmetryRepeatCount survives, sibling of SymmetryMask");
    Check(!loaded.decalRules.empty()
          && loaded.decalRules[0].radialSymmetryRepeatCount == original.decalRules[0].radialSymmetryRepeatCount
          && original.decalRules[0].radialSymmetryRepeatCount == 8,
          "DecalRule::radialSymmetryRepeatCount survives, sibling of SymmetryMask");
    Check(!loaded.unitRules.empty()
          && loaded.unitRules[0].radialSymmetryRepeatCount == original.unitRules[0].radialSymmetryRepeatCount
          && original.unitRules[0].radialSymmetryRepeatCount == 9,
          "UnitRule::radialSymmetryRepeatCount survives, sibling of SymmetryMask");

    if (!loaded.layerStack.geoLayers.empty()) {
        const Params::GeoLayer& originalGeoLayer = original.layerStack.geoLayers[0];
        const Params::GeoLayer& geoLayer = loaded.layerStack.geoLayers[0];
        Check(geoLayer.radialSymmetryRepeatCount == originalGeoLayer.radialSymmetryRepeatCount
              && originalGeoLayer.radialSymmetryRepeatCount == 10,
              "GeoLayer::radialSymmetryRepeatCount survives, sibling of SymmetryMask");
        if (!geoLayer.layers.empty()) {
            const Params::Layer& originalLayer = originalGeoLayer.layers[0];
            const Params::Layer& layer = geoLayer.layers[0];
            Check(layer.radialSymmetryRepeatCount == originalLayer.radialSymmetryRepeatCount
                  && originalLayer.radialSymmetryRepeatCount == 11,
                  "Layer::radialSymmetryRepeatCount survives, sibling of SymmetryMask, independent "
                  "of the geo-layer's own value");
        }
    }
}

// STEP13_PlacementStacks_IO: the 4 new `MarkerRule` fields (SANMAP_FORMAT_SPEC Correction 7's
// confirmed cardinality change: v1 global scalars, now per-layer fields) and the whole
// `GlobalMarkerSettings` block (ARCH §11), all through the new top-level `MarkersStack`/
// `GlobalMarkerSettings` keys — REPLACING the deleted `mapGeneratorData.PlacementRules` object.
void CheckMarkerRuleNewFieldsAndGlobalMarkerSettings(const Params::MapRecipe& original,
                                                     const Params::MapRecipe& loaded) {
    Check(!loaded.markerRules.empty()
          && NearlyEqual(loaded.markerRules[0].hydroMultiplier, original.markerRules[0].hydroMultiplier)
          && NearlyEqual(loaded.markerRules[0].reclaimDensity, original.markerRules[0].reclaimDensity)
          && NearlyEqual(loaded.markerRules[0].mexDensity, original.markerRules[0].mexDensity)
          && loaded.markerRules[0].spawnPointCount == original.markerRules[0].spawnPointCount,
          "hydroMultiplier/reclaimDensity/mexDensity/spawnPointCount survive on MarkerRule "
          "(previously write-only-to-nothing)");

    const Params::GlobalMarkerSettings& originalSettings = original.globalMarkerSettings;
    const Params::GlobalMarkerSettings& loadedSettings = loaded.globalMarkerSettings;
    Check(loadedSettings.iconNameAlloy == originalSettings.iconNameAlloy
          && loadedSettings.iconNamePlasma == originalSettings.iconNamePlasma
          && loadedSettings.iconNameSpawn == originalSettings.iconNameSpawn,
          "GlobalMarkerSettings's three icon names survive");
    Check(NearlyEqual(loadedSettings.colorAlloy[0], originalSettings.colorAlloy[0])
          && NearlyEqual(loadedSettings.colorAlloy[1], originalSettings.colorAlloy[1])
          && NearlyEqual(loadedSettings.colorAlloy[2], originalSettings.colorAlloy[2])
          && NearlyEqual(loadedSettings.colorAlloy[3], originalSettings.colorAlloy[3])
          && NearlyEqual(loadedSettings.colorPlasma[0], originalSettings.colorPlasma[0])
          && NearlyEqual(loadedSettings.colorSpawn[0], originalSettings.colorSpawn[0]),
          "GlobalMarkerSettings's three colors survive, all four components each");
    Check(NearlyEqual(loadedSettings.scaleAlloy, originalSettings.scaleAlloy)
          && NearlyEqual(loadedSettings.scalePlasma, originalSettings.scalePlasma)
          && NearlyEqual(loadedSettings.scaleSpawn, originalSettings.scaleSpawn),
          "GlobalMarkerSettings's three scales survive");
}

// SANMAP_FORMAT_SPEC Correction 7, ruling #3: `PlacementRules` is RELOCATED, not dual-written, into
// the 5 new top-level keys checked below (ruling #1/#2). STEP36_LegacyBlobDeletion_IO: a fresh
// export no longer writes `mapGeneratorData` at ALL — the old home `PlacementRules` used to be
// relocated OUT of — (TestDocumentCarriesTheFormatsOwnFields, MapExporter_IO_Test.cpp, covers that
// globally), so the narrower "not nested inside mapGeneratorData" check this used to make is now
// moot.
void CheckPlacementStacksTopLevelNotNested(const std::string& documentText) {
    const nlohmann::json document = nlohmann::json::parse(documentText);
    Check(document.contains("MarkersStack") && document["MarkersStack"].is_array()
          && document.contains("PropsStack") && document["PropsStack"].is_array()
          && document.contains("DecalsStack") && document["DecalsStack"].is_array()
          && document.contains("UnitsStack") && document["UnitsStack"].is_array()
          && document.contains("GlobalMarkerSettings") && document["GlobalMarkerSettings"].is_object(),
          "MarkersStack/PropsStack/DecalsStack/UnitsStack/GlobalMarkerSettings all appear as their "
          "own top-level objects/arrays");
}

// SANMAP_FORMAT_SPEC Correction 13: `BuildStratumLayersJson`'s real writes (albedo/normal/mask
// paths, tileSizeFar no longer aliasing the near tileCount, the six fields that were never
// written at all, farColorRemap) and `ReadStratumLayersJson`, the wholly new importer, both
// exercised through the live `.sanmap` round trip for the first time.
void CheckStratumAppearance(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    // `stratumLayers[9]` is a fixed format invariant — the importer grows `strata` to 9 regardless
    // of how many entries the fixture itself populated (only 1, here).
    Check(loaded.strata.size() == 9, "the importer grows strata to the format's fixed 9 layers");
    if (loaded.strata.empty()) return;
    const Params::Stratum& originalStratum = original.strata[0];
    const Params::Stratum& loadedStratum = loaded.strata[0];
    const Params::StratumAppearance& originalAppearance = originalStratum.appearance;
    const Params::StratumAppearance& loadedAppearance = loadedStratum.appearance;
    // STEP37_StratumAppearanceRoundtrip_IO: name/environmentName/materialName — name previously
    // wrote a generated placeholder instead of the real value; the other two were never written or
    // read anywhere in src/io/ at all.
    Check(loadedAppearance.name == originalAppearance.name
          && loadedAppearance.environmentName == originalAppearance.environmentName
          && loadedAppearance.materialName == originalAppearance.materialName,
          "name/environmentName/materialName round-trip the real designer-set values (name no "
          "longer writes the generated \"Stratum <index>\" placeholder)");
    Check(NearlyEqual(loadedStratum.tintRed, originalStratum.tintRed)
          && NearlyEqual(loadedStratum.tintGreen, originalStratum.tintGreen)
          && NearlyEqual(loadedStratum.tintBlue, originalStratum.tintBlue),
          "tintRed/Green/Blue survive through diffuseRemap");
    Check(loadedAppearance.albedoTexturePath == originalAppearance.albedoTexturePath
          && loadedAppearance.normalTexturePath == originalAppearance.normalTexturePath
          && loadedAppearance.compositeTexturePath == originalAppearance.compositeTexturePath,
          "the three texture paths survive through the albedo/normal/mask wrapper objects "
          "(previously always hardcoded empty strings)");
    Check(NearlyEqual(loadedAppearance.farTileCount, originalAppearance.farTileCount)
          && !NearlyEqual(loadedAppearance.farTileCount, loadedStratum.tileCount),
          "farTileCount survives distinct from tileCount (the original tileSizeFar/tileCount "
          "aliasing bug is fixed)");
    Check(NearlyEqual(loadedAppearance.triplanarTileCount, originalAppearance.triplanarTileCount)
          && NearlyEqual(loadedAppearance.farTriplanarTileCount, originalAppearance.farTriplanarTileCount),
          "the triplanar tile counts survive (previously never written)");
    Check(NearlyEqual(loadedAppearance.normalScale, originalAppearance.normalScale)
          && NearlyEqual(loadedAppearance.farNormalScale, originalAppearance.farNormalScale),
          "normalScale/normalScaleFar survive (previously never written)");
    Check(NearlyEqual(loadedAppearance.normalFarNearBlend, originalAppearance.normalFarNearBlend)
          && NearlyEqual(loadedAppearance.heightFarNearBlend, originalAppearance.heightFarNearBlend),
          "the normal/height far-near blends survive (previously never written)");
    Check(NearlyEqual(loadedAppearance.farColorRemapColor[0], originalAppearance.farColorRemapColor[0])
          && NearlyEqual(loadedAppearance.farColorRemapColor[1], originalAppearance.farColorRemapColor[1])
          && NearlyEqual(loadedAppearance.farColorRemapColor[2], originalAppearance.farColorRemapColor[2])
          && NearlyEqual(loadedAppearance.farColorRemapColor[3], originalAppearance.farColorRemapColor[3]),
          "farColorRemapColor survives, all four components (previously never written)");
}

// SANMAP_FORMAT_SPEC Correction 12: the new top-level `StratumGenerationSettings` array — per-
// stratum soil physics (6 fields, genuinely new writes: nothing serialized `soilPhysics` at all
// before this ticket) plus the 9 slope-gate fields (`SlopeUseGlobal` + the 8 relocated verbatim
// from the legacy `mapGeneratorData.Stratums` blob), including the override path
// (`bSlopeUseGlobal = false`) round-tripping correctly, not just the shared-default path.
void CheckStratumGenerationSettings(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    const Params::Stratum& originalStratum = original.strata[0];
    const Params::Stratum& loadedStratum = loaded.strata[0];
    const Params::StratumSoilPhysics& originalSoilPhysics = originalStratum.soilPhysics;
    const Params::StratumSoilPhysics& loadedSoilPhysics = loadedStratum.soilPhysics;
    Check(NearlyEqual(loadedSoilPhysics.hardness, originalSoilPhysics.hardness)
          && NearlyEqual(loadedSoilPhysics.friction, originalSoilPhysics.friction)
          && NearlyEqual(loadedSoilPhysics.cohesion, originalSoilPhysics.cohesion)
          && NearlyEqual(loadedSoilPhysics.capacityMultiplier, originalSoilPhysics.capacityMultiplier)
          && NearlyEqual(loadedSoilPhysics.absorptionRate, originalSoilPhysics.absorptionRate)
          && loadedSoilPhysics.bErodable == originalSoilPhysics.bErodable,
          "all 6 soilPhysics fields survive through StratumGenerationSettings "
          "(previously write-only-to-nothing)");
    Check(originalStratum.bSlopeUseGlobal == false
          && loadedStratum.bSlopeUseGlobal == originalStratum.bSlopeUseGlobal,
          "SlopeUseGlobal survives through StratumGenerationSettings, exercising the override path "
          "(bSlopeUseGlobal = false), not just the shared-default path");
    Check(loadedStratum.bSlopeGateEnabled == originalStratum.bSlopeGateEnabled
          && NearlyEqual(loadedStratum.minimumSlopeDegrees, originalStratum.minimumSlopeDegrees)
          && NearlyEqual(loadedStratum.maximumSlopeDegrees, originalStratum.maximumSlopeDegrees)
          && NearlyEqual(loadedStratum.slopeFeatherDegreesLow, originalStratum.slopeFeatherDegreesLow)
          && NearlyEqual(loadedStratum.slopeFeatherDegreesHigh, originalStratum.slopeFeatherDegreesHigh)
          && loadedStratum.bUseSmoothstep == originalStratum.bUseSmoothstep
          && loadedStratum.bInvertSlopeGate == originalStratum.bInvertSlopeGate
          && NearlyEqual(loadedStratum.slopeGateStrength, originalStratum.slopeGateStrength),
          "the 8 relocated slope-gate fields survive through StratumGenerationSettings");
}

// Regression guard (Correction 12, STEP11): the 5 fields those tickets duplicated onto the
// top-level `stratumLayers[]` entries (`ImportedMaskMode`/`MaskRemapMinimum`/`Maximum`/`Enabled`/
// `TintRed`/`Green`/`Blue`/`TileCount`) still round-trip correctly. STEP36_LegacyBlobDeletion_IO:
// a fresh export no longer writes the legacy `mapGeneratorData.Stratums` blob at all, so on THIS
// path (the real exporter) these values necessarily flow through their top-level `stratumLayers[]`
// duplicate now — the gated legacy-blob reader itself is untouched and still covered separately by
// MapImporter_IO_Test.cpp's own synthetic-old-shaped-document coverage (import side only).
void CheckLegacyStratumBlobFieldsStillSurvive(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    const Params::Stratum& originalStratum = original.strata[0];
    const Params::Stratum& loadedStratum = loaded.strata[0];
    Check(loadedStratum.importedMaskMode == originalStratum.importedMaskMode
          && originalStratum.importedMaskMode == Params::ImportedMaskMode::StaticOverride,
          "ImportedMaskMode still survives, via the top-level stratumLayers[] duplicate");
    Check(loadedStratum.bEnabled == originalStratum.bEnabled && originalStratum.bEnabled == false,
          "Enabled still survives, via the top-level stratumLayers[] duplicate");
    Check(NearlyEqual(loadedStratum.maskRemapMinimum[0], originalStratum.maskRemapMinimum[0])
          && NearlyEqual(loadedStratum.maskRemapMinimum[1], originalStratum.maskRemapMinimum[1])
          && NearlyEqual(loadedStratum.maskRemapMinimum[2], originalStratum.maskRemapMinimum[2])
          && NearlyEqual(loadedStratum.maskRemapMinimum[3], originalStratum.maskRemapMinimum[3])
          && NearlyEqual(loadedStratum.maskRemapMaximum[0], originalStratum.maskRemapMaximum[0])
          && NearlyEqual(loadedStratum.maskRemapMaximum[1], originalStratum.maskRemapMaximum[1])
          && NearlyEqual(loadedStratum.maskRemapMaximum[2], originalStratum.maskRemapMaximum[2])
          && NearlyEqual(loadedStratum.maskRemapMaximum[3], originalStratum.maskRemapMaximum[3]),
          "MaskRemapMinimum/Maximum still survive, via the top-level stratumLayers[] duplicate, all "
          "four components each");
    Check(NearlyEqual(loadedStratum.tintRed, originalStratum.tintRed)
          && NearlyEqual(loadedStratum.tintGreen, originalStratum.tintGreen)
          && NearlyEqual(loadedStratum.tintBlue, originalStratum.tintBlue),
          "TintRed/Green/Blue still survive, via the top-level stratumLayers[] duplicate");
    Check(NearlyEqual(loadedStratum.tileCount, originalStratum.tileCount),
          "TileCount still survives, via the top-level stratumLayers[] duplicate");
}

// Correction 12 is a RELOCATION, not a duplication: the 8 slope-gate keys must no longer appear
// under any legacy `mapGeneratorData.Stratums` blob. STEP36_LegacyBlobDeletion_IO: a fresh export
// no longer writes `mapGeneratorData` at ALL, so this dedicated per-key scoped check is now fully
// subsumed by the blanket "no mapGeneratorData key anywhere in the document" assertion
// (TestDocumentCarriesTheFormatsOwnFields, MapExporter_IO_Test.cpp) — removed rather than kept as
// a vacuously-true duplicate.

// Since `mapSize` never leaves the fixture, this asserts flip-then-unflip is the identity without
// the test needing to know the map-size constant, per the work-order's acceptance test wording.
void CheckArmiesAndAreas(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.areas.size() == 1, "one area survives");
    if (!loaded.areas.empty()) {
        const Params::MapArea& originalArea = original.areas[0];
        const Params::MapArea& loadedArea = loaded.areas[0];
        Check(loadedArea.name == originalArea.name, "the area's name (JSON key) survives");
        Check(NearlyEqual(loadedArea.originX, originalArea.originX), "the area's originX survives");
        // originZ round-trips VERBATIM — MapArea never gets the coordinate flip (finding 3).
        Check(NearlyEqual(loadedArea.originZ, originalArea.originZ),
              "the area's originZ survives UNFLIPPED");
        Check(NearlyEqual(loadedArea.width, originalArea.width)
              && NearlyEqual(loadedArea.length, originalArea.length),
              "the area's width/length survive");
    }

    Check(loaded.armies.size() == 1, "one army survives");
    if (loaded.armies.empty()) return;
    const Params::Army& originalArmy = original.armies[0];
    const Params::Army& loadedArmy = loaded.armies[0];
    Check(loadedArmy.name == originalArmy.name && loadedArmy.faction == originalArmy.faction
          && NearlyEqual(loadedArmy.alloys, originalArmy.alloys)
          && NearlyEqual(loadedArmy.energy, originalArmy.energy)
          && loadedArmy.alias == originalArmy.alias, "the army's own fields survive");
    Check(NearlyEqual(loadedArmy.armyColor[0], originalArmy.armyColor[0])
          && NearlyEqual(loadedArmy.armyColor[1], originalArmy.armyColor[1])
          && NearlyEqual(loadedArmy.armyColor[2], originalArmy.armyColor[2])
          && NearlyEqual(loadedArmy.armyColor[3], originalArmy.armyColor[3]),
          "armyColor survives all four components");

    Check(loadedArmy.groups.size() == 1, "one unit group survives");
    if (loadedArmy.groups.empty()) return;
    const Params::UnitGroup& originalGroup = originalArmy.groups[0];
    const Params::UnitGroup& loadedGroup = loadedArmy.groups[0];
    Check(loadedGroup.name == originalGroup.name, "the unit group's name survives");
    Check(loadedGroup.units.size() == 1, "one unit transform survives");
    if (loadedGroup.units.empty()) return;
    const Params::UnitTransform& originalUnit = originalGroup.units[0];
    const Params::UnitTransform& loadedUnit = loadedGroup.units[0];
    Check(loadedUnit.name == originalUnit.name, "the unit's name survives");
    Check(NearlyEqual(loadedUnit.positionX, originalUnit.positionX)
          && NearlyEqual(loadedUnit.positionY, originalUnit.positionY),
          "positionX/Y survive untouched by the flip");
    // The flip (export) and its inverse (import) compose to the identity — this is what actually
    // exercises the coordinate flip, without the test needing to know the map-size constant.
    Check(NearlyEqual(loadedUnit.positionZ, originalUnit.positionZ),
          "positionZ round-trips through the flip back to its original value");
    Check(NearlyEqual(loadedUnit.rotationX, originalUnit.rotationX)
          && NearlyEqual(loadedUnit.rotationY, originalUnit.rotationY)
          && NearlyEqual(loadedUnit.rotationZ, originalUnit.rotationZ)
          && NearlyEqual(loadedUnit.rotationW, originalUnit.rotationW),
          "the non-identity rotation survives verbatim, with no flip applied");
    Check(NearlyEqual(loadedUnit.scaleX, originalUnit.scaleX)
          && NearlyEqual(loadedUnit.scaleY, originalUnit.scaleY)
          && NearlyEqual(loadedUnit.scaleZ, originalUnit.scaleZ), "the non-unit scale survives");
    Check(std::string(loadedUnit.templateIdentifier) == std::string(originalUnit.templateIdentifier),
          "the bounded templateIdentifier (tpid) survives");
    Check(loadedUnit.legacyTypeTag == originalUnit.legacyTypeTag, "legacyTypeTag survives verbatim");
}

// Since `mapSize` never leaves the fixture, this asserts flip-then-unflip is the identity without
// the test needing to know the map-size constant, mirroring CheckArmiesAndAreas's exact style.
void CheckMarkersAndChains(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.markers.size() == 1, "one marker group survives");
    if (!loaded.markers.empty()) {
        const Params::MarkerInstanceGroup& originalGroup = original.markers[0];
        const Params::MarkerInstanceGroup& loadedGroup = loaded.markers[0];
        Check(loadedGroup.name == originalGroup.name, "the marker group's name (JSON key) survives");
        Check(loadedGroup.bResource == originalGroup.bResource, "bResource survives, non-default");

        Check(loadedGroup.transforms.size() == 1, "one marker transform survives");
        if (!loadedGroup.transforms.empty()) {
            const Params::MarkerTransform& originalMarker = originalGroup.transforms[0];
            const Params::MarkerTransform& loadedMarker = loadedGroup.transforms[0];
            Check(loadedMarker.name == originalMarker.name, "the marker's name survives");
            Check(loadedMarker.alias == originalMarker.alias, "the marker's alias survives");
            Check(NearlyEqual(loadedMarker.transform.positionX, originalMarker.transform.positionX)
                  && NearlyEqual(loadedMarker.transform.positionY, originalMarker.transform.positionY),
                  "positionX/Y survive untouched by the flip");
            // The flip (export) and its inverse (import) compose to the identity — this is what
            // actually exercises the coordinate flip, without the test needing to know mapSize.
            Check(NearlyEqual(loadedMarker.transform.positionZ, originalMarker.transform.positionZ),
                  "positionZ round-trips through the flip back to its original value");
            Check(NearlyEqual(loadedMarker.transform.rotationX, originalMarker.transform.rotationX)
                  && NearlyEqual(loadedMarker.transform.rotationY, originalMarker.transform.rotationY)
                  && NearlyEqual(loadedMarker.transform.rotationZ, originalMarker.transform.rotationZ)
                  && NearlyEqual(loadedMarker.transform.rotationW, originalMarker.transform.rotationW),
                  "the non-identity rotation survives verbatim, with no flip applied");
            Check(NearlyEqual(loadedMarker.transform.scaleX, originalMarker.transform.scaleX)
                  && NearlyEqual(loadedMarker.transform.scaleY, originalMarker.transform.scaleY)
                  && NearlyEqual(loadedMarker.transform.scaleZ, originalMarker.transform.scaleZ),
                  "scale survives");
        }
    }

    Check(loaded.chains.size() == 1, "one marker chain survives");
    if (loaded.chains.empty()) return;
    const Params::MarkerChain& originalChain = original.chains[0];
    const Params::MarkerChain& loadedChain = loaded.chains[0];
    Check(loadedChain.name == originalChain.name, "the chain's name (JSON key) survives");
    Check(loadedChain.markers.size() == 2, "both chain markers survive");
    if (loadedChain.markers.size() != 2) return;
    Check(loadedChain.markers[0].type == originalChain.markers[0].type
          && loadedChain.markers[0].name == originalChain.markers[0].name,
          "the first chain marker's type/name survive");
    Check(loadedChain.markers[1].type == originalChain.markers[1].type
          && loadedChain.markers[1].name == originalChain.markers[1].name,
          "the second chain marker's type/name survive, and order is preserved");
}

// Since `mapSize` never leaves the fixture, this asserts flip-then-unflip is the identity without
// the test needing to know the map-size constant, mirroring CheckArmiesAndAreas/
// CheckMarkersAndChains's exact style. Both layerIndex values are IN RANGE (0) — this fixture feeds
// RunRoundTripTests's "no warning" assertion, so it must not itself trip the layerIndex clamp; the
// clamp path is PropsDecals_IO_Test's job (MapImporter_PropsDecals_IO_Test.cpp).
void CheckPropsAndDecals(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.propLayers.size() == 1, "one prop layer survives");
    Check(loaded.props.size() == 1, "one prop group survives");
    if (!loaded.propLayers.empty() && !loaded.props.empty()) {
        const Params::PropInstanceGroup& originalGroup = original.props[0];
        const Params::PropInstanceGroup& loadedGroup = loaded.props[0];
        Check(loadedGroup.blueprintPath == originalGroup.blueprintPath,
              "PropInstanceGroup::blueprintPath survives");
        Check(loadedGroup.transforms.size() == 1, "one prop transform survives");
        if (!loadedGroup.transforms.empty()) {
            const Params::PropTransform& originalTransform = originalGroup.transforms[0];
            const Params::PropTransform& loadedTransform = loadedGroup.transforms[0];
            Check(NearlyEqual(loadedTransform.transform.positionX, originalTransform.transform.positionX)
                  && NearlyEqual(loadedTransform.transform.positionY, originalTransform.transform.positionY),
                  "prop positionX/Y survive untouched by the flip");
            Check(NearlyEqual(loadedTransform.transform.positionZ, originalTransform.transform.positionZ),
                  "prop positionZ round-trips through the flip back to its original value");
            Check(loadedTransform.layerIndex == originalTransform.layerIndex,
                  "the in-range prop layerIndex survives exactly");
        }
    }

    Check(loaded.decalLayers.size() == 1, "one decal layer survives");
    Check(loaded.decals.size() == 1, "one decal group survives");
    if (!loaded.decalLayers.empty() && !loaded.decals.empty()) {
        const Params::DecalInstanceGroup& originalGroup = original.decals[0];
        const Params::DecalInstanceGroup& loadedGroup = loaded.decals[0];
        Check(loadedGroup.blueprintPath == originalGroup.blueprintPath,
              "DecalInstanceGroup::blueprintPath survives");
        Check(loadedGroup.transforms.size() == 1, "one decal transform survives");
        if (!loadedGroup.transforms.empty()) {
            const Params::DecalTransform& originalTransform = originalGroup.transforms[0];
            const Params::DecalTransform& loadedTransform = loadedGroup.transforms[0];
            Check(NearlyEqual(loadedTransform.transform.positionX, originalTransform.transform.positionX)
                  && NearlyEqual(loadedTransform.transform.positionY, originalTransform.transform.positionY),
                  "decal positionX/Y survive untouched by the flip");
            Check(NearlyEqual(loadedTransform.transform.positionZ, originalTransform.transform.positionZ),
                  "decal positionZ round-trips through the flip back to its original value");
            Check(loadedTransform.layerIndex == originalTransform.layerIndex,
                  "the in-range decal layerIndex survives exactly");
        }
    }
}

// ATMOSPHERE_PARAMS_SPEC / STEP9_Atmosphere_PARAMS_IO: exercises the wrapper-object path fields
// (sunCookiePath/skyboxPath), the {r,g,b,a} colors (sunTint/skylightTint/backgroundColor), and the
// string-typed skyboxIntensityMode (a non-Exposure value, so the round trip actually proves the
// string mapping rather than just surviving the default).
void CheckAtmosphere(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    const Params::AtmosphereSun& originalSun = original.atmosphere.sun;
    const Params::AtmosphereSun& loadedSun = loaded.atmosphere.sun;
    Check(NearlyEqual(loadedSun.sunRightAscension, originalSun.sunRightAscension)
          && NearlyEqual(loadedSun.sunDeclination, originalSun.sunDeclination)
          && NearlyEqual(loadedSun.sunIntensity, originalSun.sunIntensity)
          && NearlyEqual(loadedSun.sunTemperature, originalSun.sunTemperature)
          && NearlyEqual(loadedSun.sunAngularDiameter, originalSun.sunAngularDiameter)
          && NearlyEqual(loadedSun.sunVolumetricMultiplier, originalSun.sunVolumetricMultiplier)
          && NearlyEqual(loadedSun.sunVolumetricShadowDimmer, originalSun.sunVolumetricShadowDimmer),
          "the sun's scalar fields survive (sunRA/sunDA and the sunVolumetrics* mismatched keys)");
    Check(NearlyEqual(loadedSun.sunTint[0], originalSun.sunTint[0])
          && NearlyEqual(loadedSun.sunTint[1], originalSun.sunTint[1])
          && NearlyEqual(loadedSun.sunTint[2], originalSun.sunTint[2])
          && NearlyEqual(loadedSun.sunTint[3], originalSun.sunTint[3]),
          "sunTint survives as {r,g,b,a}, all four components");
    Check(NearlyEqual(loadedSun.sunPosition[0], originalSun.sunPosition[0])
          && NearlyEqual(loadedSun.sunPosition[1], originalSun.sunPosition[1])
          && NearlyEqual(loadedSun.sunPosition[2], originalSun.sunPosition[2]),
          "sunPosition survives as {x,y,z}");
    Check(NearlyEqual(loadedSun.sunCookieSize[0], originalSun.sunCookieSize[0])
          && NearlyEqual(loadedSun.sunCookieSize[1], originalSun.sunCookieSize[1]),
          "sunCookieSize survives as {x,y}");
    Check(loadedSun.sunCookiePath == originalSun.sunCookiePath,
          "sunCookiePath survives through the {\"path\":...} wrapper object");

    const Params::AtmosphereSkylight& originalSkylight = original.atmosphere.skylight;
    const Params::AtmosphereSkylight& loadedSkylight = loaded.atmosphere.skylight;
    Check(NearlyEqual(loadedSkylight.skylightIntensity, originalSkylight.skylightIntensity)
          && NearlyEqual(loadedSkylight.skylightTemperature, originalSkylight.skylightTemperature),
          "skylight scalars survive");
    Check(NearlyEqual(loadedSkylight.skylightTint[0], originalSkylight.skylightTint[0])
          && NearlyEqual(loadedSkylight.skylightTint[1], originalSkylight.skylightTint[1])
          && NearlyEqual(loadedSkylight.skylightTint[2], originalSkylight.skylightTint[2])
          && NearlyEqual(loadedSkylight.skylightTint[3], originalSkylight.skylightTint[3]),
          "skylightTint survives as {r,g,b,a}, all four components");

    const Params::AtmosphereExposureSkybox& originalExposure = original.atmosphere.exposureSkybox;
    const Params::AtmosphereExposureSkybox& loadedExposure = loaded.atmosphere.exposureSkybox;
    Check(NearlyEqual(loadedExposure.exposure, originalExposure.exposure)
          && NearlyEqual(loadedExposure.exposureCompensation, originalExposure.exposureCompensation)
          && NearlyEqual(loadedExposure.skyboxRotation, originalExposure.skyboxRotation)
          && NearlyEqual(loadedExposure.skyboxExposure, originalExposure.skyboxExposure)
          && NearlyEqual(loadedExposure.skyboxMultiplier, originalExposure.skyboxMultiplier)
          && NearlyEqual(loadedExposure.skyboxLuxValue, originalExposure.skyboxLuxValue),
          "exposure/skybox scalars survive");
    Check(loadedExposure.skyboxPath == originalExposure.skyboxPath,
          "skyboxPath survives through the {\"path\":...} wrapper object");
    Check(loadedExposure.skyboxIntensityMode == originalExposure.skyboxIntensityMode
          && originalExposure.skyboxIntensityMode == Params::SkyboxIntensityMode::Lux,
          "the non-Exposure skyboxIntensityMode survives its string round-trip");

    const Params::AtmosphereLegacyFog& originalLegacyFog = original.atmosphere.legacyFog;
    const Params::AtmosphereLegacyFog& loadedLegacyFog = loaded.atmosphere.legacyFog;
    Check(NearlyEqual(loadedLegacyFog.legacyFogAttenuationDistance, originalLegacyFog.legacyFogAttenuationDistance)
          && NearlyEqual(loadedLegacyFog.legacyFogBaseHeight, originalLegacyFog.legacyFogBaseHeight)
          && NearlyEqual(loadedLegacyFog.legacyFogMaximumHeight, originalLegacyFog.legacyFogMaximumHeight)
          && NearlyEqual(loadedLegacyFog.legacyFogMaximumDistance, originalLegacyFog.legacyFogMaximumDistance)
          && NearlyEqual(loadedLegacyFog.legacyFogAnisotropy, originalLegacyFog.legacyFogAnisotropy),
          "the legacyFog* fields survive through their mismatched fog* JSON keys");

    const Params::AtmosphereBackgroundFog& originalBackgroundFog = original.atmosphere.backgroundFog;
    const Params::AtmosphereBackgroundFog& loadedBackgroundFog = loaded.atmosphere.backgroundFog;
    Check(NearlyEqual(loadedBackgroundFog.backgroundFogIntensity, originalBackgroundFog.backgroundFogIntensity)
          && NearlyEqual(loadedBackgroundFog.backgroundFogRange, originalBackgroundFog.backgroundFogRange)
          && NearlyEqual(loadedBackgroundFog.backgroundFogMinimum, originalBackgroundFog.backgroundFogMinimum)
          && NearlyEqual(loadedBackgroundFog.backgroundSkyColorIntensity, originalBackgroundFog.backgroundSkyColorIntensity)
          && NearlyEqual(loadedBackgroundFog.backgroundColorIntensity, originalBackgroundFog.backgroundColorIntensity)
          && NearlyEqual(loadedBackgroundFog.backgroundColorFadeoutRange, originalBackgroundFog.backgroundColorFadeoutRange)
          && NearlyEqual(loadedBackgroundFog.backgroundColorFadeoutPower, originalBackgroundFog.backgroundColorFadeoutPower),
          "backgroundFog scalars survive");
    Check(NearlyEqual(loadedBackgroundFog.backgroundColor[0], originalBackgroundFog.backgroundColor[0])
          && NearlyEqual(loadedBackgroundFog.backgroundColor[1], originalBackgroundFog.backgroundColor[1])
          && NearlyEqual(loadedBackgroundFog.backgroundColor[2], originalBackgroundFog.backgroundColor[2])
          && NearlyEqual(loadedBackgroundFog.backgroundColor[3], originalBackgroundFog.backgroundColor[3]),
          "backgroundColor survives as {r,g,b,a}, all four components");

    const Params::AtmosphereHeightFog& originalHeightFog = original.atmosphere.heightFog;
    const Params::AtmosphereHeightFog& loadedHeightFog = loaded.atmosphere.heightFog;
    Check(NearlyEqual(loadedHeightFog.heightFogIntensity, originalHeightFog.heightFogIntensity)
          && NearlyEqual(loadedHeightFog.heightFogStart, originalHeightFog.heightFogStart)
          && NearlyEqual(loadedHeightFog.heightFogEnd, originalHeightFog.heightFogEnd)
          && NearlyEqual(loadedHeightFog.heightFogPower, originalHeightFog.heightFogPower),
          "heightFog scalars survive");
    Check(NearlyEqual(loadedHeightFog.heightFogRange[0], originalHeightFog.heightFogRange[0])
          && NearlyEqual(loadedHeightFog.heightFogRange[1], originalHeightFog.heightFogRange[1]),
          "heightFogRange survives as {x,y}");

    const Params::AtmosphereLinearFog& originalLinearFog = original.atmosphere.linearFog;
    const Params::AtmosphereLinearFog& loadedLinearFog = loaded.atmosphere.linearFog;
    Check(NearlyEqual(loadedLinearFog.linearFogIntensity, originalLinearFog.linearFogIntensity)
          && NearlyEqual(loadedLinearFog.linearFogStart, originalLinearFog.linearFogStart)
          && NearlyEqual(loadedLinearFog.linearFogEnd, originalLinearFog.linearFogEnd)
          && NearlyEqual(loadedLinearFog.linearFogPower, originalLinearFog.linearFogPower)
          && NearlyEqual(loadedLinearFog.linearFogCameraIntensity, originalLinearFog.linearFogCameraIntensity)
          && NearlyEqual(loadedLinearFog.linearFogCameraStart, originalLinearFog.linearFogCameraStart)
          && NearlyEqual(loadedLinearFog.linearFogCameraEnd, originalLinearFog.linearFogCameraEnd),
          "linearFog scalars survive");

    const Params::AtmosphereGlobalWind& originalGlobalWind = original.atmosphere.globalWind;
    const Params::AtmosphereGlobalWind& loadedGlobalWind = loaded.atmosphere.globalWind;
    Check(NearlyEqual(loadedGlobalWind.globalWindSpeed, originalGlobalWind.globalWindSpeed)
          && NearlyEqual(loadedGlobalWind.globalWindDirection, originalGlobalWind.globalWindDirection),
          "globalWind* fields survive through their mismatched wind* JSON keys");
}

// STEP10_SlopeDefaults_Mechanism: the top-level `SlopeDefaults` object, sibling of `armies`/
// `atmosphere`, not nested in `mapGeneratorData`.
void CheckSlopeDefaults(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    const Params::SlopeDefaults& originalSlopeDefaults = original.slopeDefaults;
    const Params::SlopeDefaults& loadedSlopeDefaults = loaded.slopeDefaults;
    Check(loadedSlopeDefaults.bSlopeGateEnabled == originalSlopeDefaults.bSlopeGateEnabled
          && NearlyEqual(loadedSlopeDefaults.minimumSlopeDegrees, originalSlopeDefaults.minimumSlopeDegrees)
          && NearlyEqual(loadedSlopeDefaults.maximumSlopeDegrees, originalSlopeDefaults.maximumSlopeDegrees)
          && NearlyEqual(loadedSlopeDefaults.slopeFeatherDegreesLow, originalSlopeDefaults.slopeFeatherDegreesLow)
          && NearlyEqual(loadedSlopeDefaults.slopeFeatherDegreesHigh, originalSlopeDefaults.slopeFeatherDegreesHigh)
          && loadedSlopeDefaults.bUseSmoothstep == originalSlopeDefaults.bUseSmoothstep
          && loadedSlopeDefaults.bInvertSlopeGate == originalSlopeDefaults.bInvertSlopeGate
          && NearlyEqual(loadedSlopeDefaults.slopeGateStrength, originalSlopeDefaults.slopeGateStrength),
          "the whole SlopeDefaults block survives, top-level and outside mapGeneratorData");
}

// STEP17_FlowAccumulation_Reserved_IO: the top-level `Flow` object, sibling of `armies`/
// `atmosphere`/`SlopeDefaults`, not nested in `mapGeneratorData`.
void CheckFlow(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(NearlyEqual(loaded.flow.flowMapColor[0], original.flow.flowMapColor[0])
          && NearlyEqual(loaded.flow.flowMapColor[1], original.flow.flowMapColor[1])
          && NearlyEqual(loaded.flow.flowMapColor[2], original.flow.flowMapColor[2])
          && NearlyEqual(loaded.flow.flowMapColor[3], original.flow.flowMapColor[3]),
          "recipe.flow.flowMapColor survives all four components, top-level and outside "
          "mapGeneratorData");
}

// STEP18_DetailNormal_IO: the top-level `DetailNormal` object, sibling of `armies`/`atmosphere`/
// `SlopeDefaults`/`Flow`, not nested in `mapGeneratorData`.
void CheckDetailNormal(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.detailNormal.mapSize == original.detailNormal.mapSize,
          "recipe.detailNormal.mapSize survives, top-level and outside mapGeneratorData");
}

// STEP17_FlowAccumulation_Reserved_IO's acceptance test: `Accumulation` writes as a genuinely
// empty JSON object (no invented fields), as a top-level sibling of `mapGeneratorData`.
void CheckAccumulationWritesEmptyObject(const std::string& documentText) {
    const nlohmann::json document = nlohmann::json::parse(documentText);
    Check(document.contains("Accumulation") && document["Accumulation"].is_object(),
          "Accumulation appears as a top-level sibling of mapGeneratorData, not nested inside it");
    Check(document["Accumulation"].empty(),
          "Accumulation writes as a genuinely empty object — no fields are invented");
}

void FillFixtureLayerStackAndStrata(Params::MapRecipe& recipe) {
    // SANMAP_FORMAT_SPEC Correction 3: non-default (the LayerStack_PARAMS.h default is Unified), so
    // the round trip exercises HeightmapStack's SimulationGrouping key for real
    // (CheckLayerStackAndRules).
    recipe.layerStack.simulationGrouping = Params::SimulationGrouping::Separate;

    Params::GeoLayer geoLayer;
    geoLayer.name = "Ridges";
    geoLayer.bEnabled = false;
    geoLayer.mode = Params::GeoLayerMode::Shaper;
    geoLayer.bErodeBelow = true;
    geoLayer.blendMode = Params::HeightBlendMode::Multiply;
    geoLayer.stratumIndex = 2;
    // Correction 3's genuinely new field pair, non-default (the override path, not the
    // shared-default path).
    geoLayer.bSymmetryUseGlobal = false;
    geoLayer.symmetryMask = 5;
    // STEP16_SymmetryGlobalSettings_IO: the new sibling field, non-default (CheckSymmetryFields).
    geoLayer.radialSymmetryRepeatCount = 10;

    Params::Layer layer;
    layer.name = "Base Noise";
    layer.bEnabled = false;
    layer.bLocked = true;
    layer.stratumIndex = 4;
    layer.noiseType = Params::NoiseType::Cellular;
    layer.fractalType = Params::FractalType::Ridged;
    layer.frequency = 0.031f;
    layer.octaves = 6;
    layer.gain = 0.65f;
    layer.lacunarity = 2.4f;
    layer.weightedStrength = 0.2f;
    layer.pingPongStrength = 1.5f;
    layer.cellularJitter = 0.6f;
    layer.landDensity = 0.7f;
    layer.mountainDensity = 0.3f;
    layer.plateauDensity = 0.15f;
    layer.rampDensity = 0.1f;
    layer.levelsShadows = 0.05f;
    layer.levelsMidtones = 1.4f;
    layer.levelsHighlights = 0.9f;
    layer.levelsOutputBlack = 0.02f;
    layer.levelsOutputWhite = 0.95f;
    layer.blendMode = Params::HeightBlendMode::Overlay;
    layer.opacity = 0.75f;
    layer.heightBlendContrast = 1.6f;
    layer.heightBlendMinimum = 0.1f;
    layer.heightBlendMaximum = 0.9f;
    // Correction 3's genuinely new field pair, non-default AND distinct from the GeoLayer's own
    // override above (catches a field mix-up between the two levels).
    layer.bSymmetryUseGlobal = false;
    layer.symmetryMask = 9;
    // STEP16_SymmetryGlobalSettings_IO: the new sibling field, non-default AND distinct from the
    // geo-layer's own value above (CheckSymmetryFields).
    layer.radialSymmetryRepeatCount = 11;

    geoLayer.layers.push_back(layer);
    recipe.layerStack.geoLayers.push_back(geoLayer);

    Params::Stratum stratum;
    // SANMAP_FORMAT_SPEC Correction 12: the whole slope-gate block, non-default, including
    // `bSlopeUseGlobal = false` — the override path, not just the shared-default path
    // (CheckStratumGenerationSettings). All 8 round-trip through the new top-level
    // `StratumGenerationSettings` array now, not the legacy `mapGeneratorData.Stratums` blob.
    stratum.bSlopeUseGlobal         = false;
    stratum.bSlopeGateEnabled       = true;
    stratum.minimumSlopeDegrees     = 12.0f;
    stratum.maximumSlopeDegrees     = 55.0f;
    stratum.slopeFeatherDegreesLow  = 3.0f;
    stratum.slopeFeatherDegreesHigh = 6.0f;
    stratum.bUseSmoothstep          = true;
    stratum.bInvertSlopeGate        = true;
    stratum.slopeGateStrength       = 0.6f;

    // The 5 keys Correction 12 leaves untouched in the legacy `mapGeneratorData.Stratums` blob
    // (CheckLegacyStratumBlobFieldsStillSurvive) — non-default, so a regression in the
    // grow-and-merge loop losing one of them after 8 fewer keys flow through it would be caught.
    stratum.importedMaskMode   = Params::ImportedMaskMode::StaticOverride;
    stratum.bEnabled           = false;
    stratum.maskRemapMinimum[0] = 0.05f; stratum.maskRemapMinimum[1] = 0.15f;
    stratum.maskRemapMinimum[2] = 0.25f; stratum.maskRemapMinimum[3] = 0.35f;
    stratum.maskRemapMaximum[0] = 0.65f; stratum.maskRemapMaximum[1] = 0.75f;
    stratum.maskRemapMaximum[2] = 0.85f; stratum.maskRemapMaximum[3] = 0.95f;
    stratum.tileCount = 24.0f;
    stratum.tintRed   = 0.4f;
    stratum.tintGreen = 0.5f;
    stratum.tintBlue  = 0.6f;

    // SANMAP_FORMAT_SPEC Correction 12: all 6 `soilPhysics` fields, non-default — genuinely new
    // writes; nothing serialized this sub-struct at all before this ticket
    // (CheckStratumGenerationSettings).
    Params::StratumSoilPhysics& soilPhysics = stratum.soilPhysics;
    soilPhysics.hardness           = 0.65f;
    soilPhysics.friction           = 0.35f;
    soilPhysics.cohesion           = 0.9f;
    soilPhysics.capacityMultiplier = 3.5f;
    soilPhysics.absorptionRate     = 0.08f;
    soilPhysics.bErodable          = false;

    // SANMAP_FORMAT_SPEC Correction 13: every real `StratumAppearance` field, non-default, so a
    // round-trip bug in any single one is caught (CheckStratumAppearance).
    Params::StratumAppearance& appearance = stratum.appearance;
    // STEP37_StratumAppearanceRoundtrip_IO: the designer-editable identity fields — a hardcoded
    // placeholder for `name` and total non-existence for the other two, before this ticket.
    appearance.name            = "Lush Grass";
    appearance.environmentName = "Temperate Forest";
    appearance.materialName    = "Grass_01";
    appearance.albedoTexturePath    = "Textures/Grass_Albedo.dds";
    appearance.normalTexturePath    = "Textures/Grass_Normal.dds";
    appearance.compositeTexturePath = "Textures/Grass_Mask.dds";
    appearance.farTileCount          = 8.0f;    // distinct from tileCount (24) — catches the
                                                 // tileSizeFar/tileCount aliasing bug directly
    appearance.triplanarTileCount    = 3.0f;
    appearance.farTriplanarTileCount = 2.0f;
    appearance.normalScale           = 1.5f;
    appearance.farNormalScale        = 0.5f;
    appearance.normalFarNearBlend    = 0.3f;
    appearance.heightFarNearBlend    = 0.7f;
    appearance.farColorRemapColor[0] = 0.2f;
    appearance.farColorRemapColor[1] = 0.3f;
    appearance.farColorRemapColor[2] = 0.4f;
    appearance.farColorRemapColor[3] = 0.8f;

    recipe.strata.push_back(stratum);
}

void FillFixturePlacementRules(Params::MapRecipe& recipe) {
    Params::MarkerRule markerRule;
    markerRule.count = 8;
    markerRule.clearanceSpacing = 14.0f;
    markerRule.symmetryMask = 1;
    // STEP16_SymmetryGlobalSettings_IO: the new sibling field, non-default (CheckSymmetryFields).
    markerRule.radialSymmetryRepeatCount = 6;
    // STEP13_PlacementStacks_IO: the 4 new per-layer fields, non-default
    // (CheckMarkerRuleNewFieldsAndGlobalMarkerSettings).
    markerRule.hydroMultiplier = 1.8f;
    markerRule.reclaimDensity  = 0.35f;
    markerRule.mexDensity      = 0.6f;
    markerRule.spawnPointCount = 6;
    recipe.markerRules.push_back(markerRule);
    Params::PropRule propRule;
    propRule.density = 0.4f;
    propRule.bAvoidWater = true;
    propRule.bReclaimable = true;
    propRule.bSymmetryUseGlobal = false;
    propRule.symmetryMask = 2;
    // STEP16_SymmetryGlobalSettings_IO: the new sibling field, non-default (CheckSymmetryFields).
    propRule.radialSymmetryRepeatCount = 7;
    recipe.propRules.push_back(propRule);
    Params::DecalRule decalRule;
    decalRule.spacingMinimum = 6.0f;
    decalRule.bSymmetryUseGlobal = false;
    decalRule.symmetryMask = 8;
    decalRule.radialSymmetryRepeatCount = 8;
    recipe.decalRules.push_back(decalRule);
    Params::UnitRule unitRule;
    unitRule.armyIndex = 2;
    unitRule.count = 5;
    unitRule.bSymmetryUseGlobal = false;
    unitRule.symmetryMask = 4;
    unitRule.radialSymmetryRepeatCount = 9;
    recipe.unitRules.push_back(unitRule);

    // STEP13_PlacementStacks_IO: every GlobalMarkerSettings field, non-default (ARCH §11).
    Params::GlobalMarkerSettings& globalMarkerSettings = recipe.globalMarkerSettings;
    globalMarkerSettings.iconNameAlloy  = "AlloyIconAlt";
    globalMarkerSettings.iconNamePlasma = "PlasmaIconAlt";
    globalMarkerSettings.iconNameSpawn  = "SpawnIconAlt";
    globalMarkerSettings.colorAlloy[0] = 0.11f; globalMarkerSettings.colorAlloy[1] = 0.22f;
    globalMarkerSettings.colorAlloy[2] = 0.33f; globalMarkerSettings.colorAlloy[3] = 0.44f;
    globalMarkerSettings.colorPlasma[0] = 0.55f; globalMarkerSettings.colorPlasma[1] = 0.66f;
    globalMarkerSettings.colorPlasma[2] = 0.77f; globalMarkerSettings.colorPlasma[3] = 0.88f;
    globalMarkerSettings.colorSpawn[0] = 0.12f; globalMarkerSettings.colorSpawn[1] = 0.34f;
    globalMarkerSettings.colorSpawn[2] = 0.56f; globalMarkerSettings.colorSpawn[3] = 0.78f;
    globalMarkerSettings.scaleAlloy  = 0.25f;
    globalMarkerSettings.scalePlasma = 0.3f;
    globalMarkerSettings.scaleSpawn  = 0.2f;
}

void FillFixtureArmiesAndAreas(Params::MapRecipe& recipe) {
    Params::MapArea area;
    area.name = "PlayableArea";
    area.originX = 10.0f;
    area.originZ = 20.0f;
    area.width = 100.0f;
    area.length = 150.0f;
    recipe.areas.push_back(area);

    Params::UnitTransform unit;
    unit.name = "Unit One";
    unit.positionX = 5.0f;
    unit.positionY = 1.0f;
    unit.positionZ = 17.0f;                        // non-zero: actually exercises the flip
    unit.rotationX = 0.1f; unit.rotationY = 0.2f;
    unit.rotationZ = 0.3f; unit.rotationW = 0.9f;   // non-identity
    unit.scaleX = 2.0f; unit.scaleY = 3.0f; unit.scaleZ = 4.0f;   // non-unit
    const char templateIdentifier[] = "UNIT001";    // 7 chars + NUL, fits char[8]
    for (std::size_t index = 0; index < sizeof(unit.templateIdentifier); ++index)
        unit.templateIdentifier[index] = index < sizeof(templateIdentifier) ? templateIdentifier[index] : '\0';
    unit.legacyTypeTag = "LegacyUnit";

    Params::UnitGroup group;
    group.name = "Group One";
    group.units.push_back(unit);

    Params::Army army;
    army.name = "Army One";
    army.faction = Params::Faction::Guard;
    army.alloys = 750.0f;
    army.energy = 600.0f;
    army.armyColor[0] = 0.2f; army.armyColor[1] = 0.4f;
    army.armyColor[2] = 0.6f; army.armyColor[3] = 0.8f;
    army.alias = "Blue Army";
    army.groups.push_back(group);
    recipe.armies.push_back(army);
}

void FillFixtureMarkersAndChains(Params::MapRecipe& recipe) {
    Params::MarkerTransform markerTransform;
    markerTransform.name = "Mex 0";
    markerTransform.transform.positionX = 8.0f;
    markerTransform.transform.positionY = 2.0f;
    markerTransform.transform.positionZ = 23.0f;                     // non-zero: exercises the flip
    markerTransform.transform.rotationX = 0.05f; markerTransform.transform.rotationY = 0.15f;
    markerTransform.transform.rotationZ = 0.25f; markerTransform.transform.rotationW = 0.95f;  // non-identity
    markerTransform.transform.scaleX = 1.5f; markerTransform.transform.scaleY = 1.25f;
    markerTransform.transform.scaleZ = 1.75f;                        // non-unit
    markerTransform.alias = "North Mex";

    Params::MarkerInstanceGroup group;
    group.name = "Alloys";
    group.bResource = true;                                          // non-default
    group.transforms.push_back(markerTransform);
    recipe.markers.push_back(group);

    Params::MarkerChain chain;
    chain.name = "FirstChain";
    chain.markers.push_back({ "Alloys", "Mex 0" });
    chain.markers.push_back({ "Spawn", "Spawn 0" });
    recipe.chains.push_back(chain);
}

// One prop group and one decal group, each with a single, IN-RANGE-layerIndex transform — mirrors
// FillFixtureArmiesAndAreas/FillFixtureMarkersAndChains's style. This is what makes CheckPropsAndDecals
// exercise the LIVE BuildSanmapJsonText/ParseSanmapJsonText path for props/decals for the first
// time (STEP4_PropsDecals_IO's own test only drove the pure builders/readers directly).
void FillFixturePropsAndDecals(Params::MapRecipe& recipe) {
    Params::PropInstanceLayer propLayer;
    propLayer.name = "Foreground Props";
    propLayer.color[0] = 0.1f; propLayer.color[1] = 0.2f;
    propLayer.color[2] = 0.3f; propLayer.color[3] = 0.4f;
    propLayer.iconScale = 1.5f;
    recipe.propLayers.push_back(propLayer);

    Params::PropTransform propTransform;
    propTransform.transform.positionX = 5.0f;
    propTransform.transform.positionY = 1.0f;
    propTransform.transform.positionZ = 17.0f;                    // non-zero: exercises the flip
    propTransform.transform.rotationX = 0.1f; propTransform.transform.rotationY = 0.2f;
    propTransform.transform.rotationZ = 0.3f; propTransform.transform.rotationW = 0.9f;  // non-identity
    propTransform.transform.scaleX = 2.0f; propTransform.transform.scaleY = 3.0f;
    propTransform.transform.scaleZ = 4.0f;                        // non-unit
    propTransform.layerIndex = 0;                                 // in range: no clamp warning

    Params::PropInstanceGroup propGroup;
    propGroup.blueprintPath = "Props/Rock/Rock01.santp";
    propGroup.transforms.push_back(propTransform);
    recipe.props.push_back(propGroup);

    Params::DecalInstanceLayer decalLayer;
    decalLayer.name = "Ground Decals";
    decalLayer.color[0] = 0.5f; decalLayer.color[1] = 0.6f;
    decalLayer.color[2] = 0.7f; decalLayer.color[3] = 0.8f;
    decalLayer.iconScale = 0.75f;
    recipe.decalLayers.push_back(decalLayer);

    Params::DecalTransform decalTransform;
    decalTransform.transform.positionX = 8.0f;
    decalTransform.transform.positionY = 2.0f;
    decalTransform.transform.positionZ = 23.0f;                   // non-zero: exercises the flip
    decalTransform.transform.rotationX = 0.05f; decalTransform.transform.rotationY = 0.15f;
    decalTransform.transform.rotationZ = 0.25f; decalTransform.transform.rotationW = 0.95f;  // non-identity
    decalTransform.transform.scaleX = 1.5f; decalTransform.transform.scaleY = 1.25f;
    decalTransform.transform.scaleZ = 1.75f;                      // non-unit
    decalTransform.layerIndex = 0;                                // in range: no clamp warning

    Params::DecalInstanceGroup decalGroup;
    decalGroup.blueprintPath = "Decals/Blood/Blood01.santp";
    decalGroup.transforms.push_back(decalTransform);
    recipe.decals.push_back(decalGroup);
}

// Non-default values across all eight sub-structs, including a non-Exposure skyboxIntensityMode
// (STEP9_Atmosphere_PARAMS_IO's acceptance test).
void FillFixtureAtmosphere(Params::MapRecipe& recipe) {
    Params::AtmosphereSun& sun = recipe.atmosphere.sun;
    sun.sunRightAscension = 45.0f;
    sun.sunDeclination = -12.5f;
    sun.sunIntensity = 22000.0f;
    sun.sunTint[0] = 0.9f; sun.sunTint[1] = 0.8f; sun.sunTint[2] = 0.7f; sun.sunTint[3] = 0.6f;
    sun.sunTemperature = 5500.0f;
    sun.sunAngularDiameter = 0.75f;
    sun.sunVolumetricMultiplier = 3.2f;
    sun.sunVolumetricShadowDimmer = 0.9f;
    sun.sunPosition[0] = 100.0f; sun.sunPosition[1] = 50.0f; sun.sunPosition[2] = 200.0f;
    sun.sunCookiePath = "Textures/SunCookie.tga";
    sun.sunCookieSize[0] = 512.0f; sun.sunCookieSize[1] = 256.0f;

    Params::AtmosphereSkylight& skylight = recipe.atmosphere.skylight;
    skylight.skylightIntensity = 1.5f;
    skylight.skylightTint[0] = 0.5f; skylight.skylightTint[1] = 0.6f;
    skylight.skylightTint[2] = 0.7f; skylight.skylightTint[3] = 0.8f;
    skylight.skylightTemperature = 8500.0f;

    Params::AtmosphereExposureSkybox& exposureSkybox = recipe.atmosphere.exposureSkybox;
    exposureSkybox.exposure = 10.0f;
    exposureSkybox.exposureCompensation = 1.5f;
    exposureSkybox.skyboxPath = "Textures/Skybox.hdr";
    exposureSkybox.skyboxRotation = 90.0f;
    exposureSkybox.skyboxIntensityMode = Params::SkyboxIntensityMode::Lux;   // non-default
    exposureSkybox.skyboxExposure = 11.0f;
    exposureSkybox.skyboxMultiplier = 2.0f;
    exposureSkybox.skyboxLuxValue = 8000.0f;

    Params::AtmosphereLegacyFog& legacyFog = recipe.atmosphere.legacyFog;
    legacyFog.legacyFogAttenuationDistance = 300.0f;
    legacyFog.legacyFogBaseHeight = 20.0f;
    legacyFog.legacyFogMaximumHeight = 150.0f;
    legacyFog.legacyFogMaximumDistance = 2000.0f;
    legacyFog.legacyFogAnisotropy = 0.7f;

    Params::AtmosphereBackgroundFog& backgroundFog = recipe.atmosphere.backgroundFog;
    backgroundFog.backgroundFogIntensity = 0.8f;
    backgroundFog.backgroundFogRange = 2048.0f;
    backgroundFog.backgroundFogMinimum = 0.2f;
    backgroundFog.backgroundSkyColorIntensity = 0.9f;
    backgroundFog.backgroundColor[0] = 0.1f; backgroundFog.backgroundColor[1] = 0.2f;
    backgroundFog.backgroundColor[2] = 0.3f; backgroundFog.backgroundColor[3] = 0.4f;
    backgroundFog.backgroundColorIntensity = 0.5f;
    backgroundFog.backgroundColorFadeoutRange = 200000.0f;
    backgroundFog.backgroundColorFadeoutPower = 0.6f;

    Params::AtmosphereHeightFog& heightFog = recipe.atmosphere.heightFog;
    heightFog.heightFogIntensity = 0.9f;
    heightFog.heightFogRange[0] = -20.0f; heightFog.heightFogRange[1] = 200.0f;
    heightFog.heightFogStart = -20.0f;
    heightFog.heightFogEnd = 600.0f;
    heightFog.heightFogPower = 7.0f;

    Params::AtmosphereLinearFog& linearFog = recipe.atmosphere.linearFog;
    linearFog.linearFogIntensity = 0.5f;
    linearFog.linearFogStart = 200.0f;
    linearFog.linearFogEnd = 6000.0f;
    linearFog.linearFogPower = 1.5f;
    linearFog.linearFogCameraIntensity = 0.3f;
    linearFog.linearFogCameraStart = 600.0f;
    linearFog.linearFogCameraEnd = 6000.0f;

    Params::AtmosphereGlobalWind& globalWind = recipe.atmosphere.globalWind;
    globalWind.globalWindSpeed = 0.6f;
    globalWind.globalWindDirection = 220.0f;
}

// STEP16_SymmetryGlobalSettings_IO: `recipe.radialSymmetryRepeatCount`/`symmetryDetection`/
// `symmetryBlend`, non-default (CheckSymmetryFields). `globalSymmetryMask` itself is already set,
// non-default, in BuildPopulatedRecipe.
void FillFixtureSymmetry(Params::MapRecipe& recipe) {
    recipe.radialSymmetryRepeatCount = 5;
    recipe.symmetryDetection.bSnapImperfectSymmetry = true;
    recipe.symmetryDetection.detectionTolerance = 0.08f;
    Params::SymmetryBlend& blend = recipe.symmetryBlend;
    blend.superpositionBlend = 0.65f;
    blend.blurRadius         = 6.5f;
    blend.crossFadeWidth     = 12.0f;
    blend.cylinderZScale     = 1.4f;
    blend.torusMajorRadius   = 80.0f;
    blend.torusMinorRadius   = 22.0f;
}

// STEP10_SlopeDefaults_Mechanism: non-default values in every one of the 7 fields, so a
// round-trip bug in any single one is caught (not just the ones that happen to equal
// Stratum's own hardcoded defaults already).
void FillFixtureSlopeDefaults(Params::MapRecipe& recipe) {
    Params::SlopeDefaults& slopeDefaults = recipe.slopeDefaults;
    slopeDefaults.bSlopeGateEnabled       = true;
    slopeDefaults.minimumSlopeDegrees     = 8.0f;
    slopeDefaults.maximumSlopeDegrees     = 62.0f;
    slopeDefaults.slopeFeatherDegreesLow  = 2.5f;
    slopeDefaults.slopeFeatherDegreesHigh = 4.5f;
    slopeDefaults.bUseSmoothstep          = true;
    slopeDefaults.bInvertSlopeGate        = true;
    slopeDefaults.slopeGateStrength       = 0.72f;
}

// STEP17_FlowAccumulation_Reserved_IO: `recipe.flow.flowMapColor`, non-default in every component
// (CheckFlow). `recipe.accumulation` stays on its (empty) defaults — Accumulation has no fields yet.
void FillFixtureFlow(Params::MapRecipe& recipe) {
    Params::Flow& flow = recipe.flow;
    flow.flowMapColor[0] = 0.9f;
    flow.flowMapColor[1] = 0.1f;
    flow.flowMapColor[2] = 0.5f;
    flow.flowMapColor[3] = 0.75f;
}

// STEP18_DetailNormal_IO: `recipe.detailNormal.mapSize`, non-default (CheckDetailNormal).
void FillFixtureDetailNormal(Params::MapRecipe& recipe) {
    recipe.detailNormal.mapSize = 2048;
}

} // namespace

Params::MapRecipe BuildPopulatedRecipe() {
    Params::MapRecipe recipe;
    // STEP25_MapNameCredits_IO acceptance test item 1: a non-default mapName, and a genuinely
    // EMPTY mapCredits — real official-map content (every official Supreme Commander demo map's
    // credits field is empty), not a gap. CheckMapNameAndCredits below asserts both survive.
    recipe.mapName = "Nomad's Crossing";
    recipe.mapCredits = "";
    recipe.geometry.mapSize = 512;
    recipe.geometry.seed = 1337u;
    recipe.geometry.terrainMinHeight = 12.0f;
    recipe.geometry.terrainMaxHeight = 300.0f;
    recipe.geometry.bScaleFeaturesToMapSize = false;
    recipe.geometry.worldUnitsPerCell = 2.5f;
    // SANMAP_FORMAT_SPEC Correction 2: the one genuinely new field, non-default (CheckGeneralMapSettings).
    recipe.generalMapSettings.globalGravity = 6.5f;
    recipe.globalSymmetryMask = 3;
    recipe.water.bEnabled = true;
    recipe.water.waterLevelMaximum = 40.0f;
    recipe.water.deepWaterDepthMinimum = 3.0f;
    recipe.water.deepWaterDepthMaximum = 17.0f;
    FillFixtureLayerStackAndStrata(recipe);
    FillFixturePlacementRules(recipe);
    FillFixtureArmiesAndAreas(recipe);
    FillFixtureMarkersAndChains(recipe);
    FillFixturePropsAndDecals(recipe);
    FillFixtureAtmosphere(recipe);
    FillFixtureSlopeDefaults(recipe);
    FillFixtureSymmetry(recipe);
    FillFixtureFlow(recipe);
    FillFixtureDetailNormal(recipe);
    return recipe;
}

void RunRoundTripTests() {
    const Params::MapRecipe original = BuildPopulatedRecipe();
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(original);
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(documentText, loaded, Io::MapImportOptions(), result),
          "the exporter's own document parses");
    // STEP36_LegacyBlobDeletion_IO: a fresh export no longer writes `mapGeneratorData` at all, so
    // the importer's own gated "no legacy mapGeneratorData block" notice (MapImporter_ParseDocument_IO.cpp)
    // now fires on every normal export from this build onward. STEP38_MapGeneratorDataWarningWording_IO
    // reworded that notice AND demoted it from Warn() to Log(): absence is the EXPECTED, NORMAL state
    // for a current-format export, not a degraded-recovery signal — every field it used to guard has
    // its own top-level home, as every Check* call below (and the exact round-trip text-equality
    // check at the end of this function) still proves field for field. Any warning still fails this
    // test — this notice is informational only.
    Check(result.warningCount == 0
          && result.debugLog.find("No legacy mapGeneratorData block present") != std::string::npos,
          "with exactly the one expected informational mapGeneratorData-absence notice, and no "
          "warning at all: the two halves still agree key for key");
    CheckMapNameAndCredits(original, loaded);
    CheckGeometryAndWater(original, loaded);
    CheckGeneralMapSettings(original, loaded);
    CheckGeneralMapSettingsTopLevelNotNested(documentText);
    CheckLayerStackAndRules(original, loaded);
    CheckHeightmapStackTopLevelNotNested(documentText);
    CheckMarkerRuleNewFieldsAndGlobalMarkerSettings(original, loaded);
    CheckPlacementStacksTopLevelNotNested(documentText);
    CheckStratumAppearance(original, loaded);
    CheckStratumGenerationSettings(original, loaded);
    CheckLegacyStratumBlobFieldsStillSurvive(original, loaded);
    CheckArmiesAndAreas(original, loaded);
    CheckMarkersAndChains(original, loaded);
    CheckPropsAndDecals(original, loaded);
    CheckAtmosphere(original, loaded);
    CheckSlopeDefaults(original, loaded);
    CheckSymmetryTopLevelNotNested(documentText);
    CheckSymmetryFields(original, loaded);
    CheckFlow(original, loaded);
    CheckAccumulationWritesEmptyObject(documentText);
    CheckDetailNormal(original, loaded);

    // STEP36_LegacyBlobDeletion_IO acceptance test item 3: a full export -> import -> export round
    // trip on a fresh (never-legacy) recipe stays stable — the second export also carries no
    // mapGeneratorData key, and every other field matches the first export exactly.
    const std::string reExportedDocumentText = Io::MapExporter::BuildSanmapJsonText(loaded);
    Check(reExportedDocumentText.find("\"mapGeneratorData\"") == std::string::npos,
          "the second export also carries no mapGeneratorData key");
    Check(reExportedDocumentText == documentText,
          "and the second export matches the first export exactly, field for field");
}

// A pure-reader check, deliberately NOT routed through ParseSanmapJsonText/RunRoundTripTests
// (which asserts warningCount == 0, since STEP38_MapGeneratorDataWarningWording_IO demoted the
// expected mapGeneratorData-absence notice from Warn() to Log(), and no other warning, on an
// otherwise-clean document) — an unrecognized
// skyboxIntensityMode string must fall back to Exposure with a LOGGED warning, never a crash
// (STEP9_Atmosphere_PARAMS_IO's acceptance test).
void CheckUnrecognizedSkyboxIntensityModeFallsBackSafely() {
    nlohmann::json document;
    document["skyboxIntensityMode"] = "NotARealMode";
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadAtmosphereJson(document, loaded, result);
    Check(loaded.atmosphere.exposureSkybox.skyboxIntensityMode == Params::SkyboxIntensityMode::Exposure,
          "an unrecognized skyboxIntensityMode string falls back to Exposure");
    Check(result.warningCount > 0,
          "the unrecognized skyboxIntensityMode fallback is logged as a warning, not silent");
}

// SANMAP_FORMAT_SPEC Correction 13's cardinality invariant: `stratumLayers[9]` is fixed by the
// format. A document with the wrong array length is a LOGGED WARNING, never a crash and never a
// silent truncation (Constitution §6) — a pure-reader check, deliberately NOT routed through
// ParseSanmapJsonText/RunRoundTripTests (which asserts warningCount == 0, since
// STEP38_MapGeneratorDataWarningWording_IO demoted the expected mapGeneratorData-absence notice
// from Warn() to Log(), on an otherwise-clean document), mirroring
// CheckUnrecognizedSkyboxIntensityModeFallsBackSafely's own style above.
void CheckStratumLayersCardinalityMismatchWarns() {
    nlohmann::json document;
    document["stratumLayers"] = nlohmann::json::array();
    for (int index = 0; index < 5; ++index) {                  // wrong length: 5, not 9
        nlohmann::json layer;
        layer["albedo"] = { { "path", "Textures/Wrong.dds" } };
        document["stratumLayers"].push_back(layer);
    }
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadStratumLayersJson(document, loaded, result);
    Check(result.warningCount > 0,
          "a stratumLayers array of the wrong length logs a warning, not a crash");
    Check(loaded.strata.size() == 5,
          "the entries actually present are still read in full, never silently truncated");
    Check(loaded.strata[0].appearance.albedoTexturePath == "Textures/Wrong.dds",
          "and the fields those entries carry still land");
}

// SANMAP_FORMAT_SPEC Correction 12's cardinality invariant: `StratumGenerationSettings`'s actual
// array length is compared against `stratumLayers`'s actual array length (the two SanGen-owned
// arrays, to EACH OTHER, not each independently against `sanmapStratumCount`) — a mismatch is a
// LOGGED WARNING, never a crash and never a silent truncation, mirroring
// CheckStratumLayersCardinalityMismatchWarns's own style above.
void CheckStratumGenerationSettingsCardinalityMismatchWarns() {
    nlohmann::json document;
    document["stratumLayers"] = nlohmann::json::array();
    for (int index = 0; index < 9; ++index) document["stratumLayers"].push_back(nlohmann::json::object());
    document["StratumGenerationSettings"] = nlohmann::json::array();
    for (int index = 0; index < 5; ++index) {                  // wrong length: 5, not 9
        nlohmann::json entry;
        entry["Hardness"] = 0.77f;
        document["StratumGenerationSettings"].push_back(entry);
    }
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadStratumGenerationSettingsJson(document, loaded, result);
    Check(result.warningCount > 0,
          "a StratumGenerationSettings length that disagrees with stratumLayers's length logs a "
          "warning, not a crash");
    Check(loaded.strata.size() == 5,
          "the entries actually present are still read in full, never silently truncated");
    Check(NearlyEqual(loaded.strata[0].soilPhysics.hardness, 0.77f),
          "and the fields those entries carry still land");
}

// STEP17_FlowAccumulation_Reserved_IO's forward-compatibility requirement: a document whose
// `Accumulation` object carries keys no current field list recognizes (because none exists yet)
// reads without error — a future ticket that adds real Accumulation fields must not need this
// reader rewritten just to tolerate unknown keys. A pure-reader check, deliberately NOT routed
// through ParseSanmapJsonText/RunRoundTripTests's own fixture (which has no such keys to begin
// with), mirroring CheckUnrecognizedSkyboxIntensityModeFallsBackSafely's own style above.
void CheckAccumulationReaderToleratesUnrecognizedKeys() {
    nlohmann::json document;
    document["Accumulation"]["SomeFutureField"] = 1.5f;
    document["Accumulation"]["AnotherFutureField"] = "future value";
    Params::MapRecipe loaded;
    Io::ReadAccumulationJson(document, loaded);
    // Reaching here without throwing/crashing IS the assertion; ReadAccumulationJson has no
    // MapImportResult parameter to check a warning count against — it degrades silently by design.
    Check(true, "an Accumulation object carrying unrecognized future keys reads without error");
}

// STEP23_RadialSymmetryOrbit_PROC acceptance test 6: `ReadJsonIntegerClamped` round-trips an
// in-range value exactly and CLAMPS an out-of-range `RadialSymmetryRepeatCount` into [2, 12],
// OVERWRITING `destination` with the clamped result rather than leaving it at its prior/default —
// verified via raw-JSON-text fixtures (not just call-site inspection), at the `Symmetry` global
// section and one per-rule stack (`MarkersStack`), mirroring
// CheckUnrecognizedSkyboxIntensityModeFallsBackSafely's own pure-reader style above.
void CheckRadialSymmetryRepeatCountClampsOnImport() {
    // --- the Symmetry global section: in-range round-trips exactly, out-of-range clamps.
    {
        nlohmann::json document;
        document["Symmetry"]["RadialSymmetryRepeatCount"] = 7;
        Params::MapRecipe loaded;
        Io::ReadSymmetryJson(document, loaded);
        Check(loaded.radialSymmetryRepeatCount == 7,
              "Symmetry.RadialSymmetryRepeatCount round-trips an in-range value exactly");
    }
    {
        nlohmann::json document;
        document["Symmetry"]["RadialSymmetryRepeatCount"] = 500;
        Params::MapRecipe loaded;
        Io::ReadSymmetryJson(document, loaded);
        Check(loaded.radialSymmetryRepeatCount == Params::radialSymmetryRepeatCountMaximum,
              "Symmetry.RadialSymmetryRepeatCount clamps 500 down to the [2, 12] maximum, "
              "overwriting destination rather than leaving it at its prior/default");
    }
    for (int outOfRangeValue : { 0, 1, -5 }) {
        nlohmann::json document;
        document["Symmetry"]["RadialSymmetryRepeatCount"] = outOfRangeValue;
        Params::MapRecipe loaded;
        loaded.radialSymmetryRepeatCount = 9999;   // a value the clamp must overwrite, not preserve
        Io::ReadSymmetryJson(document, loaded);
        Check(loaded.radialSymmetryRepeatCount == Params::radialSymmetryRepeatCountMinimum,
              "Symmetry.RadialSymmetryRepeatCount clamps an out-of-range low value up to the "
              "[2, 12] minimum, overwriting destination");
    }

    // --- one per-rule stack (MarkersStack): the same clamp behavior on MarkerRule::
    // radialSymmetryRepeatCount, confirming the wiring is not Symmetry-section-specific.
    {
        nlohmann::json document;
        document["MarkersStack"] = nlohmann::json::array();
        nlohmann::json markerJson;
        markerJson["RadialSymmetryRepeatCount"] = 0;
        document["MarkersStack"].push_back(markerJson);
        Params::MapRecipe loaded;
        Io::ReadMarkersStackJson(document, loaded);
        Check(!loaded.markerRules.empty()
              && loaded.markerRules[0].radialSymmetryRepeatCount == Params::radialSymmetryRepeatCountMinimum,
              "MarkersStack[0].RadialSymmetryRepeatCount clamps 0 up to the [2, 12] minimum on import");
    }
    {
        nlohmann::json document;
        document["MarkersStack"] = nlohmann::json::array();
        nlohmann::json markerJson;
        markerJson["RadialSymmetryRepeatCount"] = 500;
        document["MarkersStack"].push_back(markerJson);
        Params::MapRecipe loaded;
        Io::ReadMarkersStackJson(document, loaded);
        Check(!loaded.markerRules.empty()
              && loaded.markerRules[0].radialSymmetryRepeatCount == Params::radialSymmetryRepeatCountMaximum,
              "MarkersStack[0].RadialSymmetryRepeatCount clamps 500 down to the [2, 12] maximum on import");
    }
}

// STEP25_MapNameCredits_IO acceptance test item 2: a document with a missing or empty top-level
// `name` key imports with recipe.mapName == "mapdef" (the fallback), matching the Files tab's own
// enforced invariant (TextInputRules{bAllowEmpty=false, fallbackText="mapdef"}). A pure
// ParseSanmapJsonText check, deliberately NOT routed through RunRoundTripTests's own fixture (which
// has no such keys to begin with), mirroring CheckUnrecognizedSkyboxIntensityModeFallsBackSafely's
// own style above. `SanGenVersion` is set on each hand-built document purely to clear the migration
// gate (RunSanmapMigrations) that runs before any domain reader — unrelated to this ticket.
void CheckMapNameFallsBackWhenMissingOrEmpty() {
    {
        nlohmann::json document;
        document["SanGenVersion"] = Io::kCurrentSanGenVersion;   // no "name" key at all
        Params::MapRecipe loaded;
        Io::MapImportResult result;
        Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
              "a document with no name key at all still parses");
        Check(loaded.mapName == "mapdef",
              "and mapName falls back to \"mapdef\" when the key is missing entirely");
    }
    {
        nlohmann::json document;
        document["SanGenVersion"] = Io::kCurrentSanGenVersion;
        document["name"] = "";
        Params::MapRecipe loaded;
        Io::MapImportResult result;
        Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
              "a document with an empty name key still parses");
        Check(loaded.mapName == "mapdef",
              "and mapName falls back to \"mapdef\" when the key is present but empty");
    }
}

// STEP25_MapNameCredits_IO acceptance test item 3: a document with an empty top-level `credits`
// key imports with recipe.mapCredits == "" — confirming NO fallback is applied here, unlike
// mapName (an empty credits string is legitimate real content: every official Supreme Commander
// demo map's credits field is genuinely empty).
void CheckMapCreditsHasNoFallbackWhenEmpty() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["credits"] = "";
    Params::MapRecipe loaded;
    loaded.mapCredits = "a prior value the reader must overwrite";
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a document with an empty credits key still parses");
    Check(loaded.mapCredits.empty(),
          "and mapCredits stays empty — no fallback is applied, unlike mapName");
}

// STEP27_WaterTopLevelImport_IO acceptance test item 1: a document with the top-level `hasWater`/
// `waterLevel`/`waterDepth` mirrors and NO `mapGeneratorData` block at all (simulating a real
// official/SupCom map, which never carries that block) imports with recipe.water populated from
// those top-level keys instead of silently staying at the Params::Water struct defaults. A pure
// ParseSanmapJsonText check, mirroring CheckMapNameFallsBackWhenMissingOrEmpty's own style.
void CheckWaterImportsFromTopLevelWhenNoGeneratorData() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["hasWater"]   = true;
    document["waterLevel"] = 25.0f;
    document["waterDepth"] = 12.0f;
    // No "mapGeneratorData" key at all.
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a document with top-level water keys and no mapGeneratorData block still parses");
    Check(loaded.water.bEnabled == true
          && NearlyEqual(loaded.water.waterLevelMaximum, 25.0f)
          && NearlyEqual(loaded.water.deepWaterDepthMaximum, 12.0f),
          "water.bEnabled/waterLevelMaximum/deepWaterDepthMaximum populate from the top-level "
          "hasWater/waterLevel/waterDepth mirrors when mapGeneratorData is absent");
}

// STEP27_WaterTopLevelImport_IO acceptance test item 2: a document with BOTH the top-level water
// mirrors AND a mapGeneratorData.Water block present, where the two genuinely disagree, imports
// with the legacy blob's values winning — confirms call-order precedence (the gated ReadWaterJson
// call runs AFTER the unconditional top-level reads), matching the existing terrainMaxHeight
// precedent at MapImporter_IO.cpp's mapGeneratorData gate.
void CheckWaterLegacyBlobWinsOverTopLevelMirrorsOnDisagreement() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["hasWater"]   = false;
    document["waterLevel"] = 10.0f;
    document["waterDepth"] = 5.0f;
    nlohmann::json water;
    water["Enabled"]           = true;
    water["WaterLevelMax"]     = 99.0f;
    water["DeepWaterDepthMin"] = 1.0f;
    water["DeepWaterDepthMax"] = 88.0f;
    nlohmann::json generatorData;
    generatorData["Water"] = water;
    document["mapGeneratorData"] = generatorData;
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a document with both water locations, disagreeing, still parses");
    Check(loaded.water.bEnabled == true
          && NearlyEqual(loaded.water.waterLevelMaximum, 99.0f)
          && NearlyEqual(loaded.water.deepWaterDepthMinimum, 1.0f)
          && NearlyEqual(loaded.water.deepWaterDepthMaximum, 88.0f),
          "the legacy mapGeneratorData.Water block's values win over the disagreeing top-level "
          "mirrors, confirming call-order precedence");
}

// STEP30_LegacyBlobFieldHoming_IO acceptance test item 1: a document with ONLY
// `GeneralMapSettings.TerrainMaxHeight` set (no top-level `height`, no `mapGeneratorData` block at
// all) imports at full float precision — proving the new reader, not the pre-existing lossy
// top-level `height` int mirror, is what lands the value. Mirrors
// CheckWaterImportsFromTopLevelWhenNoGeneratorData's own style.
void CheckTerrainMaxHeightImportsFromGeneralMapSettingsAtFullPrecision() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["GeneralMapSettings"]["TerrainMaxHeight"] = 142.375f;
    // No "height" key, no "mapGeneratorData" block at all.
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a document with only GeneralMapSettings.TerrainMaxHeight still parses");
    Check(NearlyEqual(loaded.geometry.terrainMaxHeight, 142.375f),
          "terrainMaxHeight round-trips at full float precision through GeneralMapSettings, not "
          "the lossy top-level height int mirror");
}

// STEP30_LegacyBlobFieldHoming_IO acceptance test item 2: a document with ONLY
// `stratumLayers[0].ImportedMaskMode`/`Enabled` set (no `mapGeneratorData` block at all) imports
// both fields from that entry. Mirrors CheckWaterImportsFromTopLevelWhenNoGeneratorData's own style.
void CheckStratumImportedMaskModeAndEnabledImportFromStratumLayers() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    nlohmann::json layer;
    layer["ImportedMaskMode"] = static_cast<int>(Params::ImportedMaskMode::StaticOverride);
    layer["Enabled"] = false;
    document["stratumLayers"] = nlohmann::json::array();
    document["stratumLayers"].push_back(layer);
    // No "mapGeneratorData" block at all.
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a document with only stratumLayers[0].ImportedMaskMode/Enabled still parses");
    Check(!loaded.strata.empty()
          && loaded.strata[0].importedMaskMode == Params::ImportedMaskMode::StaticOverride
          && loaded.strata[0].bEnabled == false,
          "ImportedMaskMode/Enabled import from stratumLayers[0], both non-default values, when "
          "mapGeneratorData is absent");
}

// STEP37_StratumAppearanceRoundtrip_IO acceptance test item 2: an old-shaped `stratumLayers[0]`
// entry missing `name`/`environmentName`/`materialName` entirely must import without crashing and
// leave the fields on their sane default (empty string) — matching `StratumNameRules()`
// (StratumsTab_UI.h)'s own `bAllowEmpty = true` invariant, the SAME rule the live Stratums tab
// already enforces for a brand-new, never-named stratum. No "mapdef"-style fallback text applies
// here (unlike STEP25's `mapName`): an empty stratum name is legitimate, UI-legal content, not a
// gap needing a placeholder.
void CheckStratumAppearanceIdentityFieldsMissingKeysImportWithSaneFallback() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    nlohmann::json layer;
    layer["albedo"] = { { "path", "Textures/Old.dds" } };   // an old-shaped entry with some content,
                                                             // but no name/environmentName/materialName
    document["stratumLayers"] = nlohmann::json::array();
    document["stratumLayers"].push_back(layer);
    // No "mapGeneratorData" block at all.
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "an old-shaped stratumLayers[0] entry missing name/environmentName/materialName still "
          "parses, no crash");
    Check(!loaded.strata.empty()
          && loaded.strata[0].appearance.name.empty()
          && loaded.strata[0].appearance.environmentName.empty()
          && loaded.strata[0].appearance.materialName.empty(),
          "all three fields land on their sane empty default, matching StratumNameRules()'s own "
          "bAllowEmpty invariant rather than a crash or an invented non-empty placeholder");
}

// STEP30_LegacyBlobFieldHoming_IO acceptance test item 3: a document with ONLY the top-level
// `deepWaterDepthMin` key set (no `mapGeneratorData` block at all) imports it. Mirrors
// CheckWaterImportsFromTopLevelWhenNoGeneratorData's own style.
void CheckDeepWaterDepthMinImportsFromTopLevelWhenNoGeneratorData() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["deepWaterDepthMin"] = 7.5f;
    // No "mapGeneratorData" block at all.
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a document with only the top-level deepWaterDepthMin key still parses");
    Check(NearlyEqual(loaded.water.deepWaterDepthMinimum, 7.5f),
          "deepWaterDepthMinimum populates from the top-level deepWaterDepthMin mirror when "
          "mapGeneratorData is absent");
}

// STEP30_LegacyBlobFieldHoming_IO acceptance test item 4: a document carrying BOTH the new
// top-level/array homes for all 4 fields AND the legacy mapGeneratorData blob, disagreeing on all
// 4, imports with the legacy blob's values winning for every single one — not just the water field
// STEP27 already proved. Mirrors CheckWaterLegacyBlobWinsOverTopLevelMirrorsOnDisagreement's own
// style, extended to the other 3 fields this ticket adds.
void CheckLegacyBlobWinsOverAllFourNewFieldHomesOnDisagreement() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;

    // The 4 new homes: one set of values.
    document["GeneralMapSettings"]["TerrainMaxHeight"] = 100.0f;
    document["deepWaterDepthMin"] = 1.0f;
    nlohmann::json layer;
    layer["ImportedMaskMode"] = static_cast<int>(Params::ImportedMaskMode::ProceduralStart);
    layer["Enabled"] = true;
    document["stratumLayers"] = nlohmann::json::array();
    document["stratumLayers"].push_back(layer);

    // The legacy mapGeneratorData blob: disagreeing values on all 4, which must win.
    nlohmann::json generatorData;
    generatorData["TerrainMaxHeight"] = 500.0f;
    nlohmann::json water;
    water["DeepWaterDepthMin"] = 9.0f;
    generatorData["Water"] = water;
    nlohmann::json legacyStratum;
    legacyStratum["ImportedMaskMode"] = static_cast<int>(Params::ImportedMaskMode::StaticOverride);
    legacyStratum["Enabled"] = false;
    generatorData["Stratums"] = nlohmann::json::array();
    generatorData["Stratums"].push_back(legacyStratum);
    document["mapGeneratorData"] = generatorData;

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a document with all 4 new homes and a disagreeing legacy blob still parses");
    Check(NearlyEqual(loaded.geometry.terrainMaxHeight, 500.0f),
          "the legacy blob's TerrainMaxHeight wins over GeneralMapSettings' disagreeing value");
    Check(NearlyEqual(loaded.water.deepWaterDepthMinimum, 9.0f),
          "the legacy blob's Water.DeepWaterDepthMin wins over the disagreeing top-level mirror");
    Check(!loaded.strata.empty()
          && loaded.strata[0].importedMaskMode == Params::ImportedMaskMode::StaticOverride
          && loaded.strata[0].bEnabled == false,
          "the legacy blob's Stratums[0] ImportedMaskMode/Enabled win over the disagreeing "
          "stratumLayers[0] entry, for both fields");
}

// STEP36_LegacyBlobDeletion_IO acceptance test item 2: a SYNTHETIC old-shaped document — a real
// `mapGeneratorData` block and NOTHING else (no top-level `GeneralMapSettings`, no
// `stratumLayers[].ImportedMaskMode`/`Enabled`, no top-level `hasWater`/`waterLevel`/
// `deepWaterDepthMin` mirrors at all) — still imports its terrainMaxHeight/water/stratum settings
// correctly from the legacy blob alone. This is the never-refuse law (STEP24) and the gated legacy
// readers (`ReadGeometryJson`/`ReadWaterJson`/`ReadStrataSettingsJson`) proven completely unaffected
// by this ticket's export-side-only deletion — real old files like `World_Domination.sanmap`/
// `Pandemonium Isthmus.sanmap` still carry exactly this shape.
void CheckPureOldShapedDocumentStillImportsFromLegacyBlobAlone() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;

    nlohmann::json generatorData;
    generatorData["TerrainMaxHeight"] = 175.5f;
    nlohmann::json water;
    water["Enabled"]           = true;
    water["WaterLevelMax"]     = 33.0f;
    water["DeepWaterDepthMin"] = 4.0f;
    water["DeepWaterDepthMax"] = 21.0f;
    generatorData["Water"] = water;
    nlohmann::json legacyStratum;
    legacyStratum["ImportedMaskMode"] = static_cast<int>(Params::ImportedMaskMode::StaticOverride);
    legacyStratum["Enabled"]          = false;
    legacyStratum["TileCount"]        = 12.0f;
    generatorData["Stratums"] = nlohmann::json::array();
    generatorData["Stratums"].push_back(legacyStratum);
    document["mapGeneratorData"] = generatorData;
    // No GeneralMapSettings, no stratumLayers, no top-level water mirrors at all — a pure old-shaped
    // document, exactly like a real pre-STEP30 export.

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, Io::MapImportOptions(), result),
          "a purely old-shaped document (legacy blob only, nothing top-level) still parses "
          "(never-refuse, STEP24)");
    Check(NearlyEqual(loaded.geometry.terrainMaxHeight, 175.5f),
          "terrainMaxHeight imports from the legacy mapGeneratorData.TerrainMaxHeight alone");
    Check(loaded.water.bEnabled == true
          && NearlyEqual(loaded.water.waterLevelMaximum, 33.0f)
          && NearlyEqual(loaded.water.deepWaterDepthMinimum, 4.0f)
          && NearlyEqual(loaded.water.deepWaterDepthMaximum, 21.0f),
          "the whole water block imports from the legacy mapGeneratorData.Water alone");
    Check(!loaded.strata.empty()
          && loaded.strata[0].importedMaskMode == Params::ImportedMaskMode::StaticOverride
          && loaded.strata[0].bEnabled == false
          && NearlyEqual(loaded.strata[0].tileCount, 12.0f),
          "stratum settings import from the legacy mapGeneratorData.Stratums alone");
}

// KnownTopLevelSanmapKeys_IO_Test (STEP24_ImportNeverRefuses_IO ruling 4's paired coverage test):
// every key `BuildSanmapJsonText` writes is present in `IsKnownTopLevelSanmapKey`'s maintained
// allowlist (`Sanmap_KnownTopLevelKeys_IO.cpp`) — a future coder adding a new top-level export key
// without updating that allowlist fails loud here in CI, rather than that key silently mis-bagging
// into a future import's Unknown-Import passthrough.
void CheckKnownTopLevelSanmapKeysCoverage() {
    const Params::MapRecipe recipe = BuildPopulatedRecipe();
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe);
    const nlohmann::json document = nlohmann::json::parse(documentText);
    for (const auto& [key, value] : document.items()) {
        (void)value;
        Check(Io::IsKnownTopLevelSanmapKey(key),
              ("BuildSanmapJsonText's top-level key \"" + key
               + "\" is present in the KnownTopLevelSanmapKeys allowlist").c_str());
    }
}

} // namespace MapFormatTest
} // namespace SanmapGen

int main() {
    SanmapGen::MapFormatTest::RunRoundTripTests();
    SanmapGen::MapFormatTest::CheckKnownTopLevelSanmapKeysCoverage();
    SanmapGen::MapFormatTest::CheckUnrecognizedSkyboxIntensityModeFallsBackSafely();
    SanmapGen::MapFormatTest::CheckStratumLayersCardinalityMismatchWarns();
    SanmapGen::MapFormatTest::CheckStratumGenerationSettingsCardinalityMismatchWarns();
    SanmapGen::MapFormatTest::CheckAccumulationReaderToleratesUnrecognizedKeys();
    SanmapGen::MapFormatTest::CheckRadialSymmetryRepeatCountClampsOnImport();
    SanmapGen::MapFormatTest::CheckMapNameFallsBackWhenMissingOrEmpty();
    SanmapGen::MapFormatTest::CheckMapCreditsHasNoFallbackWhenEmpty();
    SanmapGen::MapFormatTest::CheckWaterImportsFromTopLevelWhenNoGeneratorData();
    SanmapGen::MapFormatTest::CheckWaterLegacyBlobWinsOverTopLevelMirrorsOnDisagreement();
    SanmapGen::MapFormatTest::CheckTerrainMaxHeightImportsFromGeneralMapSettingsAtFullPrecision();
    SanmapGen::MapFormatTest::CheckStratumImportedMaskModeAndEnabledImportFromStratumLayers();
    SanmapGen::MapFormatTest::CheckStratumAppearanceIdentityFieldsMissingKeysImportWithSaneFallback();
    SanmapGen::MapFormatTest::CheckDeepWaterDepthMinImportsFromTopLevelWhenNoGeneratorData();
    SanmapGen::MapFormatTest::CheckLegacyBlobWinsOverAllFourNewFieldHomesOnDisagreement();
    SanmapGen::MapFormatTest::CheckPureOldShapedDocumentStillImportsFromLegacyBlobAlone();
    SanmapGen::MapFormatTest::RunValidationTests();
    SanmapGen::MapFormatTest::RunBakedFieldTests();
    if (SanmapGen::MapFormatTest::FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", SanmapGen::MapFormatTest::FailureCount());
    return 1;
}

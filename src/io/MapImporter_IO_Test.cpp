// MapImporter_IO_Test.cpp — acceptance test for "Load Sanmap" (section D / PARITY_BACKLOG PB-1).
// This unit holds the binary's main(), the fixture recipe, and the headline check: a populated
// MapRecipe written by MapExporter and read back by MapImporter compares equal field for field.
// The validation and baked-field halves live in the two sibling units (MapFormat_TestSupport_IO.h).
#include "MapFormat_TestSupport_IO.h"
#include "MapImporter_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "MapExporter_Recipe_IO.h"
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
        // STEP152: bDisabled round-trips independently of bEnabled (the fixture sets them to
        // opposite values on purpose).
        Check(geoLayer.bDisabled == originalGeoLayer.bDisabled && originalGeoLayer.bDisabled == true
              && geoLayer.bDisabled != geoLayer.bEnabled,
              "the geo-layer's generation-inclusion bDisabled survives, independent of bEnabled");
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
            // STEP152: same independence check as the geo-layer's own pair above.
            Check(layer.bDisabled == originalLayer.bDisabled && originalLayer.bDisabled == true
                  && layer.bDisabled != layer.bEnabled,
                  "the layer's generation-inclusion bDisabled survives, independent of bEnabled");
            // STEP99_BakedImageLayer_PARAMS: bBaked=true, a non-default bakedImagePath, and a
            // non--1 layerIdentifier all round-trip exactly.
            Check(layer.bBaked == originalLayer.bBaked && originalLayer.bBaked == true
                  && layer.bakedImagePath == originalLayer.bakedImagePath
                  && originalLayer.bakedImagePath == "Textures/imported_ridge.raw"
                  && layer.layerIdentifier == originalLayer.layerIdentifier
                  && originalLayer.layerIdentifier == 42,
                  "the layer's baked/image-source fields survive (bBaked/bakedImagePath/"
                  "layerIdentifier, STEP99)");
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
    Check(loaded.markerRuleLayers.size() == 1 && loaded.markerRuleLayers[0].rules.size() == 1
          && loaded.markerRuleLayers[0].rules[0].count == 8
          && loaded.markerRuleLayers[0].symmetry.symmetryMask == 1, "the marker rules survive");
    // STEP96_FootprintBakeAndStalenessCheck_IO.md: the fixture now carries a SECOND prop/unit rule
    // each (a never-baked one, acceptance test 5) alongside the original baked one — size 2, not 1.
    Check(loaded.propRules.size() == 2 && loaded.propRules[0].bAvoidWater
          && loaded.propRules[0].bReclaimable
          && loaded.propRules[0].symmetry.bSymmetryUseGlobal == false
          && loaded.propRules[0].symmetry.symmetryMask == 2,
          "the prop rules survive, including the per-rule symmetry override");
    Check(loaded.decalRules.size() == 1 && NearlyEqual(loaded.decalRules[0].spacingMinimum, 6.0f)
          && loaded.decalRules[0].symmetry.bSymmetryUseGlobal == false
          && loaded.decalRules[0].symmetry.symmetryMask == 8,
          "the decal rules survive, including the per-rule symmetry override");
    Check(loaded.unitRules.size() == 2 && loaded.unitRules[0].armyIndex == 2
          && loaded.unitRules[0].count == 5 && loaded.unitRules[0].symmetry.bSymmetryUseGlobal == false
          && loaded.unitRules[0].symmetry.symmetryMask == 4,
          "the unit rules survive, including the per-rule symmetry override");
}

// STEP96_FootprintBakeAndStalenessCheck_IO.md acceptance tests 4/5: the baked PropRule/UnitRule's
// baseFootprintWidth/Depth/footprintBakeFingerprint round-trip exactly (floats via NearlyEqual,
// fingerprint fields exact), and the SECOND, never-baked rule of each kind round-trips with
// IsValid() == false on both ends — no crash on the absent nested "FootprintBakeFingerprint" key.
void CheckFootprintBakeFields(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.propRules.size() == 2 && original.propRules.size() == 2,
          "the fixture carries exactly one baked and one never-baked prop rule");
    const Params::PropRule& bakedProp = loaded.propRules[0];
    Check(NearlyEqual(bakedProp.baseFootprintWidth, original.propRules[0].baseFootprintWidth)
          && NearlyEqual(bakedProp.baseFootprintDepth, original.propRules[0].baseFootprintDepth)
          && NearlyEqual(bakedProp.baseFootprintWidth, 5.5f)
          && NearlyEqual(bakedProp.baseFootprintDepth, 6.5f),
          "PropRule::baseFootprintWidth/Depth survive the round trip");
    Check(bakedProp.footprintBakeFingerprint.sourcePath == "Templates/Props/rock_01.santp"
          && bakedProp.footprintBakeFingerprint.byteSize == 4096ull
          && bakedProp.footprintBakeFingerprint.modifiedTime == 1700000000ull
          && bakedProp.footprintBakeFingerprint.contentHash == 123456789ull
          && bakedProp.footprintBakeFingerprint.IsValid(),
          "PropRule::footprintBakeFingerprint survives byte-for-byte and reports valid");
    Check(!loaded.propRules[1].footprintBakeFingerprint.IsValid()
          && !original.propRules[1].footprintBakeFingerprint.IsValid()
          && NearlyEqual(loaded.propRules[1].baseFootprintWidth, 4.0f)
          && NearlyEqual(loaded.propRules[1].baseFootprintDepth, 4.0f),
          "a never-baked PropRule round-trips with IsValid() == false and STEP58's own default size");

    Check(loaded.unitRules.size() == 2 && original.unitRules.size() == 2,
          "the fixture carries exactly one baked and one never-baked unit rule");
    const Params::UnitRule& bakedUnit = loaded.unitRules[0];
    Check(NearlyEqual(bakedUnit.baseFootprintWidth, original.unitRules[0].baseFootprintWidth)
          && NearlyEqual(bakedUnit.baseFootprintDepth, original.unitRules[0].baseFootprintDepth)
          && NearlyEqual(bakedUnit.baseFootprintWidth, 1.4f)
          && NearlyEqual(bakedUnit.baseFootprintDepth, 1.6f),
          "UnitRule::baseFootprintWidth/Depth survive the round trip");
    Check(bakedUnit.footprintBakeFingerprint.sourcePath == "Templates/Units/uca1001.santp"
          && bakedUnit.footprintBakeFingerprint.byteSize == 2048ull
          && bakedUnit.footprintBakeFingerprint.modifiedTime == 1650000000ull
          && bakedUnit.footprintBakeFingerprint.contentHash == 987654321ull
          && bakedUnit.footprintBakeFingerprint.IsValid(),
          "UnitRule::footprintBakeFingerprint survives byte-for-byte and reports valid");
    Check(!loaded.unitRules[1].footprintBakeFingerprint.IsValid()
          && !original.unitRules[1].footprintBakeFingerprint.IsValid()
          && NearlyEqual(loaded.unitRules[1].baseFootprintWidth, 2.0f)
          && NearlyEqual(loaded.unitRules[1].baseFootprintDepth, 2.0f),
          "a never-baked UnitRule round-trips with IsValid() == false and STEP58's own default size");
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

    Check(!loaded.markerRuleLayers.empty()
          && loaded.markerRuleLayers[0].symmetry.radialSymmetryRepeatCount
                 == original.markerRuleLayers[0].symmetry.radialSymmetryRepeatCount
          && original.markerRuleLayers[0].symmetry.radialSymmetryRepeatCount == 6,
          "MarkerRuleLayer::symmetry.radialSymmetryRepeatCount survives, sibling of SymmetryMask "
          "(STEP66: promoted off the individual MarkerRule)");
    Check(!loaded.propRules.empty()
          && loaded.propRules[0].symmetry.radialSymmetryRepeatCount
                 == original.propRules[0].symmetry.radialSymmetryRepeatCount
          && original.propRules[0].symmetry.radialSymmetryRepeatCount == 7,
          "PropRule::symmetry.radialSymmetryRepeatCount survives, sibling of SymmetryMask");
    Check(!loaded.decalRules.empty()
          && loaded.decalRules[0].symmetry.radialSymmetryRepeatCount
                 == original.decalRules[0].symmetry.radialSymmetryRepeatCount
          && original.decalRules[0].symmetry.radialSymmetryRepeatCount == 8,
          "DecalRule::symmetry.radialSymmetryRepeatCount survives, sibling of SymmetryMask");
    Check(!loaded.unitRules.empty()
          && loaded.unitRules[0].symmetry.radialSymmetryRepeatCount
                 == original.unitRules[0].symmetry.radialSymmetryRepeatCount
          && original.unitRules[0].symmetry.radialSymmetryRepeatCount == 9,
          "UnitRule::symmetry.radialSymmetryRepeatCount survives, sibling of SymmetryMask");

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

// STEP13_PlacementStacks_IO: the whole `GlobalMarkerSettings` block (ARCH §11) through the new
// top-level `GlobalMarkerSettings` key — REPLACING the deleted `mapGeneratorData.PlacementRules`
// object. (This used to also check MarkerRule's own hydroMultiplier/reclaimDensity/mexDensity/
// spawnPointCount fields — retired: struct-default dead weight with no UI and no PROC consumer.)
void CheckGlobalMarkerSettingsSurvives(const Params::MapRecipe& original,
                                       const Params::MapRecipe& loaded) {
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
    Check(NearlyEqual(loadedSettings.selectColorAlloy[0], originalSettings.selectColorAlloy[0])
          && NearlyEqual(loadedSettings.selectColorAlloy[1], originalSettings.selectColorAlloy[1])
          && NearlyEqual(loadedSettings.selectColorAlloy[2], originalSettings.selectColorAlloy[2])
          && NearlyEqual(loadedSettings.selectColorAlloy[3], originalSettings.selectColorAlloy[3])
          && NearlyEqual(loadedSettings.selectColorPlasma[0], originalSettings.selectColorPlasma[0])
          && NearlyEqual(loadedSettings.selectColorPlasma[1], originalSettings.selectColorPlasma[1])
          && NearlyEqual(loadedSettings.selectColorPlasma[2], originalSettings.selectColorPlasma[2])
          && NearlyEqual(loadedSettings.selectColorPlasma[3], originalSettings.selectColorPlasma[3])
          && NearlyEqual(loadedSettings.selectColorSpawn[0], originalSettings.selectColorSpawn[0])
          && NearlyEqual(loadedSettings.selectColorSpawn[1], originalSettings.selectColorSpawn[1])
          && NearlyEqual(loadedSettings.selectColorSpawn[2], originalSettings.selectColorSpawn[2])
          && NearlyEqual(loadedSettings.selectColorSpawn[3], originalSettings.selectColorSpawn[3])
          && NearlyEqual(loadedSettings.selectColorDefault[0], originalSettings.selectColorDefault[0])
          && NearlyEqual(loadedSettings.selectColorDefault[1], originalSettings.selectColorDefault[1])
          && NearlyEqual(loadedSettings.selectColorDefault[2], originalSettings.selectColorDefault[2])
          && NearlyEqual(loadedSettings.selectColorDefault[3], originalSettings.selectColorDefault[3]),
          "GlobalMarkerSettings's four selectColor* fields survive, all four components each");
}

// ARCH §20: `GlobalPropSettings`/`GlobalDecalSettings` through their own top-level keys, siblings
// of `PropsStack`/`DecalsStack`, mirroring `CheckGlobalMarkerSettingsSurvives`'s exact style.
void CheckGlobalPropDecalSettingsSurvives(const Params::MapRecipe& original,
                                          const Params::MapRecipe& loaded) {
    const Params::GlobalPropSettings& originalProp = original.globalPropSettings;
    const Params::GlobalPropSettings& loadedProp = loaded.globalPropSettings;
    Check(NearlyEqual(loadedProp.colorProp[0], originalProp.colorProp[0])
          && NearlyEqual(loadedProp.colorProp[1], originalProp.colorProp[1])
          && NearlyEqual(loadedProp.colorProp[2], originalProp.colorProp[2])
          && NearlyEqual(loadedProp.colorProp[3], originalProp.colorProp[3])
          && NearlyEqual(loadedProp.colorReclaim[0], originalProp.colorReclaim[0])
          && NearlyEqual(loadedProp.colorReclaim[1], originalProp.colorReclaim[1])
          && NearlyEqual(loadedProp.colorReclaim[2], originalProp.colorReclaim[2])
          && NearlyEqual(loadedProp.colorReclaim[3], originalProp.colorReclaim[3]),
          "GlobalPropSettings's two colors survive, all four components each");

    const Params::GlobalDecalSettings& originalDecal = original.globalDecalSettings;
    const Params::GlobalDecalSettings& loadedDecal = loaded.globalDecalSettings;
    Check(NearlyEqual(loadedDecal.colorDecal[0], originalDecal.colorDecal[0])
          && NearlyEqual(loadedDecal.colorDecal[1], originalDecal.colorDecal[1])
          && NearlyEqual(loadedDecal.colorDecal[2], originalDecal.colorDecal[2])
          && NearlyEqual(loadedDecal.colorDecal[3], originalDecal.colorDecal[3]),
          "GlobalDecalSettings's color survives, all four components");
}

// STEP66_MarkerRuleLayer_PARAMS acceptance test: a recipe with 2 `MarkerRuleLayer`s (different
// symmetry settings, 2+ rules each, including non-default per-rule fields) round-trips exactly
// through `MarkersStack`'s two-level shape (ARCH_16_01_NewParamsShapes.md §16.1,
// SANMAP_FORMAT_SPEC Correction 15). Confirms, via the raw JSON TEXT (not just call sites), that no
// `MarkerRule` object carries `SymmetryUseGlobal`/`SymmetryMask`/`RadialSymmetryRepeatCount`, and
// that each layer carries its own three symmetry keys exactly once, at the layer level.
void CheckMarkerRuleLayerTwoLevelRoundTrip() {
    Params::MapRecipe original;

    Params::MarkerRuleLayer layerOne;
    layerOne.name = "Expansions";
    layerOne.bEnabled = true;
    layerOne.bHidden  = false;
    layerOne.symmetry.bSymmetryUseGlobal = false;
    layerOne.symmetry.symmetryMask = Params::SymmetryAxis::MirrorAcrossX;
    layerOne.symmetry.radialSymmetryRepeatCount = 4;
    layerOne.parentBundleIdentifier = 5;                          // STEP119, non-default
    layerOne.markerTypeName = "Alloy";                            // STEP124, non-default
    Params::MarkerRule ruleOneA; ruleOneA.count = 3; ruleOneA.clearanceSpacing = 1.4f;
    Params::MarkerRule ruleOneB; ruleOneB.count = 5; ruleOneB.density = 0.25f;
    layerOne.rules.push_back(ruleOneA);
    layerOne.rules.push_back(ruleOneB);
    original.markerRuleLayers.push_back(layerOne);

    Params::MarkerRuleLayer layerTwo;
    layerTwo.name = "Start Alloys";
    layerTwo.bEnabled = false;   // a real generation gate — must survive as false, not just Hidden
    layerTwo.bHidden  = true;
    layerTwo.symmetry.bSymmetryUseGlobal = true;
    layerTwo.symmetry.symmetryMask = Params::SymmetryAxis::Radial;
    layerTwo.symmetry.radialSymmetryRepeatCount = 6;
    Params::MarkerRule ruleTwoA; ruleTwoA.density = 0.6f; ruleTwoA.areaRadiusMinimum = 2.0f;
    Params::MarkerRule ruleTwoB; ruleTwoB.count = 7; ruleTwoB.clearanceSpacing = 2.2f;
    layerTwo.rules.push_back(ruleTwoA);
    layerTwo.rules.push_back(ruleTwoB);
    original.markerRuleLayers.push_back(layerTwo);

    const nlohmann::ordered_json markersStackJson = Io::BuildMarkersStackJson(original);
    const std::string markersStackText = markersStackJson.dump();
    Check(markersStackText.find("SymmetryUseGlobal") != std::string::npos
          && markersStackText.find("SymmetryMask") != std::string::npos
          && markersStackText.find("RadialSymmetryRepeatCount") != std::string::npos,
          "the layer's own three symmetry keys are present in the exported MarkersStack");

    // Confirm each `Rules[]` element (not the layer object itself) carries no symmetry keys, by
    // walking the parsed structure directly rather than a flat text search (a flat search can't
    // distinguish "present on the layer" from "present on a rule").
    bool bAnyRuleCarriesSymmetryKey = false;
    int  layerSymmetryUseGlobalKeyCount = 0;
    for (const nlohmann::ordered_json& layerJson : markersStackJson) {
        if (layerJson.contains("SymmetryUseGlobal")) ++layerSymmetryUseGlobalKeyCount;
        if (!layerJson.contains("Rules")) continue;
        for (const nlohmann::ordered_json& ruleJson : layerJson["Rules"]) {
            if (ruleJson.contains("SymmetryUseGlobal") || ruleJson.contains("SymmetryMask")
                || ruleJson.contains("RadialSymmetryRepeatCount"))
                bAnyRuleCarriesSymmetryKey = true;
        }
    }
    Check(!bAnyRuleCarriesSymmetryKey,
          "no MarkerRule JSON object in the exported document contains SymmetryUseGlobal/"
          "SymmetryMask/RadialSymmetryRepeatCount");
    Check(layerSymmetryUseGlobalKeyCount == static_cast<int>(original.markerRuleLayers.size()),
          "the layer's own SymmetryUseGlobal key appears exactly once per layer");

    nlohmann::json document;
    document["MarkersStack"] = markersStackJson;
    Params::MapRecipe loaded;
    Io::ReadMarkersStackJson(document, loaded);

    Check(loaded.markerRuleLayers.size() == 2, "both MarkerRuleLayers survive the round trip");
    for (std::size_t layerIndex = 0; layerIndex < original.markerRuleLayers.size(); ++layerIndex) {
        const Params::MarkerRuleLayer& originalLayer = original.markerRuleLayers[layerIndex];
        const Params::MarkerRuleLayer& loadedLayer   = loaded.markerRuleLayers[layerIndex];
        Check(loadedLayer.name == originalLayer.name
              && loadedLayer.bEnabled == originalLayer.bEnabled
              && loadedLayer.bHidden == originalLayer.bHidden
              && loadedLayer.symmetry.bSymmetryUseGlobal == originalLayer.symmetry.bSymmetryUseGlobal
              && loadedLayer.symmetry.symmetryMask == originalLayer.symmetry.symmetryMask
              && loadedLayer.symmetry.radialSymmetryRepeatCount
                     == originalLayer.symmetry.radialSymmetryRepeatCount
              && loadedLayer.parentBundleIdentifier == originalLayer.parentBundleIdentifier
              && loadedLayer.markerTypeName == originalLayer.markerTypeName
              && loadedLayer.rules.size() == originalLayer.rules.size(),
              "the layer's own fields (Name/Enabled/Hidden/symmetry/parentBundleIdentifier/"
              "markerTypeName) and rule count survive");
        for (std::size_t ruleIndex = 0; ruleIndex < originalLayer.rules.size(); ++ruleIndex) {
            const Params::MarkerRule& originalRule = originalLayer.rules[ruleIndex];
            const Params::MarkerRule& loadedRule   = loadedLayer.rules[ruleIndex];
            Check(loadedRule.count == originalRule.count
                  && NearlyEqual(loadedRule.clearanceSpacing, originalRule.clearanceSpacing)
                  && NearlyEqual(loadedRule.density, originalRule.density)
                  && NearlyEqual(loadedRule.areaRadiusMinimum, originalRule.areaRadiusMinimum),
                  "each rule's own fields (count/ClearanceSpacing/Density/AreaRadiusMinimum) "
                  "survive independent of the layer's symmetry");
        }
    }
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
    Check(loadedArmy.name == originalArmy.name && loadedArmy.displayName == originalArmy.displayName
          && loadedArmy.faction == originalArmy.faction
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
    Check(loaded.markerLayers.size() == 1, "one marker layer survives");
    if (!loaded.markerLayers.empty()) {
        const Params::MarkerInstanceLayer& originalLayer = original.markerLayers[0];
        const Params::MarkerInstanceLayer& loadedLayer = loaded.markerLayers[0];
        Check(loadedLayer.name == originalLayer.name, "MarkerInstanceLayer::name survives");
        Check(NearlyEqual(loadedLayer.color[0], originalLayer.color[0])
              && NearlyEqual(loadedLayer.color[1], originalLayer.color[1])
              && NearlyEqual(loadedLayer.color[2], originalLayer.color[2])
              && NearlyEqual(loadedLayer.color[3], originalLayer.color[3]),
              "MarkerInstanceLayer::color survives all four components");
        Check(NearlyEqual(loadedLayer.iconScale, originalLayer.iconScale),
              "MarkerInstanceLayer::iconScale survives");
        Check(loadedLayer.layerId == originalLayer.layerId, "MarkerInstanceLayer::layerId (7) survives");
        Check(loadedLayer.bLocked == originalLayer.bLocked, "MarkerInstanceLayer::bLocked survives, non-default");
        Check(loadedLayer.bHidden == originalLayer.bHidden,
              "MarkerInstanceLayer::bHidden survives, non-default (STEP144)");
        Check(loadedLayer.bGridSnapEnabled == originalLayer.bGridSnapEnabled,
              "MarkerInstanceLayer::bGridSnapEnabled survives, non-default");
        Check(NearlyEqual(loadedLayer.gridSnapSizeWorldUnits, originalLayer.gridSnapSizeWorldUnits),
              "MarkerInstanceLayer::gridSnapSizeWorldUnits survives, non-default");
        Check(loadedLayer.bColorOverrideEnabled == originalLayer.bColorOverrideEnabled,
              "MarkerInstanceLayer::bColorOverrideEnabled survives, non-default");
        Check(loadedLayer.bSymmetryEnabled == originalLayer.bSymmetryEnabled && !loadedLayer.bSymmetryEnabled,
              "MarkerInstanceLayer::bSymmetryEnabled survives, non-default (STEP130, ARCH §19.24)");
        Check(loadedLayer.parentBundleIdentifier == originalLayer.parentBundleIdentifier,
              "MarkerInstanceLayer::parentBundleIdentifier survives, non-default (STEP119)");
        Check(loadedLayer.markerTypeName == originalLayer.markerTypeName,
              "MarkerInstanceLayer::markerTypeName survives, non-default (STEP124)");
    }

    Check(loaded.markerLayerBundles.size() == 1, "one MarkerLayerBundle survives (STEP119)");
    if (!loaded.markerLayerBundles.empty()) {
        const Params::MarkerLayerBundle& originalBundle = original.markerLayerBundles[0];
        const Params::MarkerLayerBundle& loadedBundle = loaded.markerLayerBundles[0];
        Check(loadedBundle.identifier == originalBundle.identifier,
              "MarkerLayerBundle::identifier survives");
        Check(loadedBundle.name == originalBundle.name, "MarkerLayerBundle::name survives");
        Check(loadedBundle.parentBundleIdentifier == originalBundle.parentBundleIdentifier,
              "MarkerLayerBundle::parentBundleIdentifier survives");
        Check(loadedBundle.markerTypeName == originalBundle.markerTypeName,
              "MarkerLayerBundle::markerTypeName survives");
        Check(loadedBundle.assemblyIdentifier == originalBundle.assemblyIdentifier,
              "MarkerLayerBundle::assemblyIdentifier survives");
    }

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
            Check(loadedMarker.layerIndex == originalMarker.layerIndex,
                  "the in-range marker layerIndex survives exactly");
            Check(loadedMarker.symmetryGroupIdentifier == originalMarker.symmetryGroupIdentifier,
                  "the marker's symmetryGroupIdentifier survives, sibling of alias (STEP68)");
            Check(loadedMarker.iconNameOverride == originalMarker.iconNameOverride,
                  "the marker's iconNameOverride survives, sibling of alias/symmetryGroupIdentifier");
            Check(loadedMarker.instanceIdentifier == originalMarker.instanceIdentifier,
                  "the marker's explicit instanceIdentifier (999) survives the OVERWRITE half of "
                  "the backfill-then-overwrite logic (STEP124)");
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
    if (!loaded.propLayers.empty()) {
        const Params::PropInstanceLayer& originalLayer = original.propLayers[0];
        const Params::PropInstanceLayer& loadedLayer = loaded.propLayers[0];
        Check(loadedLayer.layerId == originalLayer.layerId,
              "PropInstanceLayer::layerId survives the live BuildSanmapJsonText/ParseSanmapJsonText path");
        Check(loadedLayer.bLocked == originalLayer.bLocked, "PropInstanceLayer::bLocked survives, non-default");
        Check(loadedLayer.bHidden == originalLayer.bHidden,
              "PropInstanceLayer::bHidden survives, non-default (ARCH §20)");
        Check(loadedLayer.bGridSnapEnabled == originalLayer.bGridSnapEnabled,
              "PropInstanceLayer::bGridSnapEnabled survives, non-default (ARCH §20)");
        Check(NearlyEqual(loadedLayer.gridSnapSizeWorldUnits, originalLayer.gridSnapSizeWorldUnits),
              "PropInstanceLayer::gridSnapSizeWorldUnits survives, non-default (ARCH §20)");
        Check(loadedLayer.bColorOverrideEnabled == originalLayer.bColorOverrideEnabled,
              "PropInstanceLayer::bColorOverrideEnabled survives, non-default (ARCH §20)");
        Check(loadedLayer.bSymmetryEnabled == originalLayer.bSymmetryEnabled && !loadedLayer.bSymmetryEnabled,
              "PropInstanceLayer::bSymmetryEnabled survives, non-default (ARCH §20)");
        Check(loadedLayer.parentBundleIdentifier == originalLayer.parentBundleIdentifier,
              "PropInstanceLayer::parentBundleIdentifier survives, non-default (ARCH §20)");
        Check(loadedLayer.propTypeName == originalLayer.propTypeName,
              "PropInstanceLayer::propTypeName survives, non-default (ARCH §20)");
    }

    Check(loaded.propLayerBundles.size() == 1, "one PropLayerBundle survives (ARCH §20)");
    if (!loaded.propLayerBundles.empty()) {
        const Params::PropLayerBundle& originalBundle = original.propLayerBundles[0];
        const Params::PropLayerBundle& loadedBundle = loaded.propLayerBundles[0];
        Check(loadedBundle.identifier == originalBundle.identifier, "PropLayerBundle::identifier survives");
        Check(loadedBundle.name == originalBundle.name, "PropLayerBundle::name survives");
        Check(loadedBundle.parentBundleIdentifier == originalBundle.parentBundleIdentifier,
              "PropLayerBundle::parentBundleIdentifier survives");
        Check(loadedBundle.propTypeName == originalBundle.propTypeName, "PropLayerBundle::propTypeName survives");
        Check(loadedBundle.assemblyIdentifier == originalBundle.assemblyIdentifier,
              "PropLayerBundle::assemblyIdentifier survives");
    }
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
    if (!loaded.decalLayers.empty()) {
        const Params::DecalInstanceLayer& originalLayer = original.decalLayers[0];
        const Params::DecalInstanceLayer& loadedLayer = loaded.decalLayers[0];
        Check(loadedLayer.layerId == originalLayer.layerId,
              "DecalInstanceLayer::layerId survives the live BuildSanmapJsonText/ParseSanmapJsonText path");
        Check(loadedLayer.bLocked == originalLayer.bLocked, "DecalInstanceLayer::bLocked survives, non-default");
        Check(loadedLayer.bHidden == originalLayer.bHidden,
              "DecalInstanceLayer::bHidden survives, non-default (ARCH §20)");
        Check(loadedLayer.bGridSnapEnabled == originalLayer.bGridSnapEnabled,
              "DecalInstanceLayer::bGridSnapEnabled survives, non-default (ARCH §20)");
        Check(NearlyEqual(loadedLayer.gridSnapSizeWorldUnits, originalLayer.gridSnapSizeWorldUnits),
              "DecalInstanceLayer::gridSnapSizeWorldUnits survives, non-default (ARCH §20)");
        Check(loadedLayer.bColorOverrideEnabled == originalLayer.bColorOverrideEnabled,
              "DecalInstanceLayer::bColorOverrideEnabled survives, non-default (ARCH §20)");
        Check(loadedLayer.bSymmetryEnabled == originalLayer.bSymmetryEnabled && !loadedLayer.bSymmetryEnabled,
              "DecalInstanceLayer::bSymmetryEnabled survives, non-default (ARCH §20)");
        Check(loadedLayer.parentBundleIdentifier == originalLayer.parentBundleIdentifier,
              "DecalInstanceLayer::parentBundleIdentifier survives, non-default (ARCH §20)");
    }

    Check(loaded.decalLayerBundles.size() == 1, "one DecalLayerBundle survives (ARCH §20)");
    if (!loaded.decalLayerBundles.empty()) {
        const Params::DecalLayerBundle& originalBundle = original.decalLayerBundles[0];
        const Params::DecalLayerBundle& loadedBundle = loaded.decalLayerBundles[0];
        Check(loadedBundle.identifier == originalBundle.identifier, "DecalLayerBundle::identifier survives");
        Check(loadedBundle.name == originalBundle.name, "DecalLayerBundle::name survives");
        Check(loadedBundle.parentBundleIdentifier == originalBundle.parentBundleIdentifier,
              "DecalLayerBundle::parentBundleIdentifier survives");
        Check(loadedBundle.assemblyIdentifier == originalBundle.assemblyIdentifier,
              "DecalLayerBundle::assemblyIdentifier survives");
    }
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
    // STEP152: non-default AND deliberately the OPPOSITE of bEnabled above -- proves the two
    // fields round-trip independently, not as one flag under two names (CheckLayerStackAndRules).
    geoLayer.bDisabled = true;
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
    // STEP152: non-default AND the OPPOSITE of bEnabled above, same reasoning as the geo-layer's
    // own pair.
    layer.bDisabled = true;
    layer.bLocked = true;
    layer.stratumIndex = 4;
    // STEP99_BakedImageLayer_PARAMS: non-default, so the round trip exercises all three new
    // baked/image-source fields for real (CheckLayerStackAndRules).
    layer.bBaked = true;
    layer.bakedImagePath = "Textures/imported_ridge.raw";
    layer.layerIdentifier = 42;
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
    // STEP66_MarkerRuleLayer_PARAMS: the symmetry triplet moved up one tier, off MarkerRule onto
    // its wrapping MarkerRuleLayer.
    Params::MarkerRuleLayer markerRuleLayer;
    markerRuleLayer.name = "Fixture Marker Layer";
    markerRuleLayer.symmetry.symmetryMask = 1;
    // STEP16_SymmetryGlobalSettings_IO: the new sibling field, non-default (CheckSymmetryFields).
    markerRuleLayer.symmetry.radialSymmetryRepeatCount = 6;
    Params::MarkerRule markerRule;
    markerRule.count = 8;
    markerRule.clearanceSpacing = 14.0f;
    markerRuleLayer.rules.push_back(markerRule);
    recipe.markerRuleLayers.push_back(markerRuleLayer);
    Params::PropRule propRule;
    propRule.density = 0.4f;
    propRule.bAvoidWater = true;
    propRule.bReclaimable = true;
    propRule.symmetry.bSymmetryUseGlobal = false;
    propRule.symmetry.symmetryMask = 2;
    // STEP16_SymmetryGlobalSettings_IO: the new sibling field, non-default (CheckSymmetryFields).
    propRule.symmetry.radialSymmetryRepeatCount = 7;
    // STEP96_FootprintBakeAndStalenessCheck_IO.md acceptance test 4: a baked PropRule, non-default
    // and distinct from the unit rule's own values below (catches a field mix-up between the two).
    propRule.baseFootprintWidth  = 5.5f;
    propRule.baseFootprintDepth  = 6.5f;
    propRule.footprintBakeFingerprint.sourcePath   = "Templates/Props/rock_01.santp";
    propRule.footprintBakeFingerprint.byteSize     = 4096ull;
    propRule.footprintBakeFingerprint.modifiedTime = 1700000000ull;
    propRule.footprintBakeFingerprint.contentHash  = 123456789ull;
    recipe.propRules.push_back(propRule);
    // STEP96 acceptance test 5: a SECOND prop rule that was never baked -- must round-trip with
    // IsValid() == false on both ends, no crash on the absent nested "FootprintBakeFingerprint" key.
    recipe.propRules.push_back(Params::PropRule());
    Params::DecalRule decalRule;
    decalRule.spacingMinimum = 6.0f;
    decalRule.symmetry.bSymmetryUseGlobal = false;
    decalRule.symmetry.symmetryMask = 8;
    decalRule.symmetry.radialSymmetryRepeatCount = 8;
    recipe.decalRules.push_back(decalRule);
    Params::UnitRule unitRule;
    unitRule.armyIndex = 2;
    unitRule.count = 5;
    unitRule.symmetry.bSymmetryUseGlobal = false;
    unitRule.symmetry.symmetryMask = 4;
    unitRule.symmetry.radialSymmetryRepeatCount = 9;
    unitRule.baseFootprintWidth  = 1.4f;
    unitRule.baseFootprintDepth  = 1.6f;
    unitRule.footprintBakeFingerprint.sourcePath   = "Templates/Units/uca1001.santp";
    unitRule.footprintBakeFingerprint.byteSize     = 2048ull;
    unitRule.footprintBakeFingerprint.modifiedTime = 1650000000ull;
    unitRule.footprintBakeFingerprint.contentHash  = 987654321ull;
    recipe.unitRules.push_back(unitRule);
    recipe.unitRules.push_back(Params::UnitRule());   // never baked -- see the prop rule note above

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
    // STEP124: the four selectColor* fields, non-default, each component distinct (ARCH §19.17).
    globalMarkerSettings.selectColorAlloy[0] = 0.91f; globalMarkerSettings.selectColorAlloy[1] = 0.92f;
    globalMarkerSettings.selectColorAlloy[2] = 0.93f; globalMarkerSettings.selectColorAlloy[3] = 0.94f;
    globalMarkerSettings.selectColorPlasma[0] = 0.81f; globalMarkerSettings.selectColorPlasma[1] = 0.82f;
    globalMarkerSettings.selectColorPlasma[2] = 0.83f; globalMarkerSettings.selectColorPlasma[3] = 0.84f;
    globalMarkerSettings.selectColorSpawn[0] = 0.71f; globalMarkerSettings.selectColorSpawn[1] = 0.72f;
    globalMarkerSettings.selectColorSpawn[2] = 0.73f; globalMarkerSettings.selectColorSpawn[3] = 0.74f;
    globalMarkerSettings.selectColorDefault[0] = 0.41f; globalMarkerSettings.selectColorDefault[1] = 0.42f;
    globalMarkerSettings.selectColorDefault[2] = 0.43f; globalMarkerSettings.selectColorDefault[3] = 0.44f;

    // ARCH §20: GlobalPropSettings/GlobalDecalSettings, every field non-default.
    Params::GlobalPropSettings& globalPropSettings = recipe.globalPropSettings;
    globalPropSettings.colorProp[0] = 0.15f; globalPropSettings.colorProp[1] = 0.25f;
    globalPropSettings.colorProp[2] = 0.35f; globalPropSettings.colorProp[3] = 0.45f;
    globalPropSettings.colorReclaim[0] = 0.55f; globalPropSettings.colorReclaim[1] = 0.65f;
    globalPropSettings.colorReclaim[2] = 0.75f; globalPropSettings.colorReclaim[3] = 0.85f;

    Params::GlobalDecalSettings& globalDecalSettings = recipe.globalDecalSettings;
    globalDecalSettings.colorDecal[0] = 0.05f; globalDecalSettings.colorDecal[1] = 0.95f;
    globalDecalSettings.colorDecal[2] = 0.15f; globalDecalSettings.colorDecal[3] = 0.85f;
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
    // STEP76_ArmyIdentityNaming_IO: `name` is now the machine-owned engine identity (never a
    // human-authored string) — a fixture value must already be ARMY_XX-shaped, or the unconditional
    // import-side normalizer (MapImporter_ArmyIdentityNormalize_IO.cpp) rewrites it and logs a
    // warning, breaking this test's own `warningCount == 0` round-trip assertion below. The
    // human-authored label this position used to carry now lives on `displayName` instead.
    army.name = "ARMY_01";
    army.displayName = "Army One";
    army.faction = Params::Faction::Guard;
    army.alloys = 750.0f;
    army.energy = 600.0f;
    army.armyColor[0] = 0.2f; army.armyColor[1] = 0.4f;
    army.armyColor[2] = 0.6f; army.armyColor[3] = 0.8f;
    army.alias = "Blue Army";
    army.groups.push_back(group);
    recipe.armies.push_back(army);
}

// STEP60_MarkerInstanceLayer_PARAMS: one MarkerInstanceLayer with a non-default layerId (7), and
// the fixture marker transform's layerIndex set to 0 (in range against the one MarkerGroups
// entry) — this fixture feeds RunRoundTripTests's "no warning" assertion, same constraint
// CheckPropsAndDecals's own comment states.
void FillFixtureMarkersAndChains(Params::MapRecipe& recipe) {
    Params::MarkerInstanceLayer markerLayer;
    markerLayer.name = "Resource Markers";
    markerLayer.color[0] = 0.2f; markerLayer.color[1] = 0.4f;
    markerLayer.color[2] = 0.6f; markerLayer.color[3] = 0.8f;
    markerLayer.iconScale = 1.25f;
    markerLayer.layerId = 7;                                          // non-default, survives verbatim
    markerLayer.bLocked = true;                                       // STEP106, non-default
    markerLayer.bHidden = true;                                       // STEP144, non-default
    markerLayer.bGridSnapEnabled = true;                              // STEP106, non-default
    markerLayer.gridSnapSizeWorldUnits = 4.0f;                        // STEP106, non-default
    markerLayer.bColorOverrideEnabled = true;                         // STEP116, non-default
    markerLayer.bSymmetryEnabled = false;                             // STEP130, non-default (ARCH §19.24)
    markerLayer.parentBundleIdentifier = 42;                          // STEP119, non-default
    markerLayer.markerTypeName = "Spawn";                             // STEP124, non-default
    recipe.markerLayers.push_back(markerLayer);

    // STEP119: one MarkerLayerBundle, non-default on every field, non-cyclic (parentBundleIdentifier
    // == -1) so RunRoundTripTests's own "no warning" assertion is not tripped by
    // RepairCyclicMarkerLayerBundleParents.
    Params::MarkerLayerBundle bundle;
    bundle.identifier = 11;
    bundle.name = "Resource Bundle";
    bundle.parentBundleIdentifier = -1;
    bundle.markerTypeName = "Alloy";
    bundle.assemblyIdentifier = 5;
    recipe.markerLayerBundles.push_back(bundle);

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
    markerTransform.layerIndex = 0;                                  // in range: no clamp warning
    markerTransform.symmetryGroupIdentifier = 3;                      // non-zero: STEP68
    markerTransform.iconNameOverride = "CustomAlloyIcon";             // non-empty: STEP114
    markerTransform.instanceIdentifier = 999;                         // non-default, explicit: STEP124

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
    propLayer.layerId = 7;                    // non-default: exercises the "Id" wire key round-trip
    propLayer.bLocked = true;                                       // non-default
    propLayer.bHidden = true;                                       // ARCH §20, non-default
    propLayer.bGridSnapEnabled = true;                              // ARCH §20, non-default
    propLayer.gridSnapSizeWorldUnits = 4.0f;                        // ARCH §20, non-default
    propLayer.bColorOverrideEnabled = true;                         // ARCH §20, non-default
    propLayer.bSymmetryEnabled = false;                             // ARCH §20, non-default
    propLayer.parentBundleIdentifier = 42;                          // ARCH §20, non-default
    propLayer.propTypeName = "Reclaim";                             // ARCH §20, non-default
    recipe.propLayers.push_back(propLayer);

    // ARCH §20: one PropLayerBundle, non-default on every field, non-cyclic (parentBundleIdentifier
    // == -1) so RunRoundTripTests's own "no warning" assertion is not tripped by
    // RepairCyclicPropLayerBundleParents.
    Params::PropLayerBundle propBundle;
    propBundle.identifier = 11;
    propBundle.name = "Reclaim Bundle";
    propBundle.parentBundleIdentifier = -1;
    propBundle.propTypeName = "Reclaim";
    propBundle.assemblyIdentifier = 5;
    recipe.propLayerBundles.push_back(propBundle);

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
    decalLayer.layerId = 7;                     // non-default: exercises the "Id" wire key round-trip
    decalLayer.bLocked = true;                                       // non-default
    decalLayer.bHidden = true;                                       // ARCH §20, non-default
    decalLayer.bGridSnapEnabled = true;                              // ARCH §20, non-default
    decalLayer.gridSnapSizeWorldUnits = 2.0f;                        // ARCH §20, non-default
    decalLayer.bColorOverrideEnabled = true;                         // ARCH §20, non-default
    decalLayer.bSymmetryEnabled = false;                             // ARCH §20, non-default
    decalLayer.parentBundleIdentifier = 13;                          // ARCH §20, non-default
    recipe.decalLayers.push_back(decalLayer);

    // ARCH §20: one DecalLayerBundle, non-default on every field, non-cyclic (parentBundleIdentifier
    // == -1) so RunRoundTripTests's own "no warning" assertion is not tripped by
    // RepairCyclicDecalLayerBundleParents.
    Params::DecalLayerBundle decalBundle;
    decalBundle.identifier = 9;
    decalBundle.name = "Ground Bundle";
    decalBundle.parentBundleIdentifier = -1;
    decalBundle.assemblyIdentifier = 3;
    recipe.decalLayerBundles.push_back(decalBundle);

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
    CheckFootprintBakeFields(original, loaded);
    CheckHeightmapStackTopLevelNotNested(documentText);
    CheckGlobalMarkerSettingsSurvives(original, loaded);
    CheckGlobalPropDecalSettingsSurvives(original, loaded);
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

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-D acceptance: a hand-authored/partial .sanmap army
// with no "armyColor" key at all backfills from Params::kDefaultArmyColors by roster position, while
// an army that explicitly authored white stays white — a KEY-PRESENCE discriminator, never a value
// comparison against white (which would silently clobber a genuinely-authored white army). Roster
// position is the parse-order position in the (alphabetically-iterated, "ARMY_01" < "ARMY_02" <
// "ARMY_03") armies dictionary — the same order NormalizeArmyIdentities already relies on being
// stable (MapImporter_ArmyIdentityNormalize_IO.cpp). A pure-reader check, mirroring
// CheckUnrecognizedSkyboxIntensityModeFallsBackSafely's own style above.
void CheckArmyColorBackfillUsesKeyPresenceNotValueComparison() {
    nlohmann::json document;
    document["armies"] = nlohmann::json::object();
    document["armies"]["ARMY_01"] = nlohmann::json::object();   // no "armyColor" key at all

    nlohmann::json armyExplicitWhite;
    armyExplicitWhite["armyColor"]["r"] = 1.0f;
    armyExplicitWhite["armyColor"]["g"] = 1.0f;
    armyExplicitWhite["armyColor"]["b"] = 1.0f;
    armyExplicitWhite["armyColor"]["a"] = 1.0f;
    document["armies"]["ARMY_02"] = armyExplicitWhite;

    document["armies"]["ARMY_03"] = nlohmann::json::object();   // no "armyColor" key at all

    Params::MapRecipe loaded;
    loaded.geometry.mapSize = 256;
    Io::ReadArmiesJson(document, loaded);

    Check(loaded.armies.size() == 3, "all three armies parse");
    if (loaded.armies.size() != 3) return;

    Check(NearlyEqual(loaded.armies[0].armyColor[0], Params::kDefaultArmyColors[0][0])
          && NearlyEqual(loaded.armies[0].armyColor[1], Params::kDefaultArmyColors[0][1])
          && NearlyEqual(loaded.armies[0].armyColor[2], Params::kDefaultArmyColors[0][2])
          && NearlyEqual(loaded.armies[0].armyColor[3], Params::kDefaultArmyColors[0][3]),
          "ARMY_01 (roster position 0, no armyColor key): backfilled from kDefaultArmyColors[0]");

    Check(NearlyEqual(loaded.armies[1].armyColor[0], 1.0f) && NearlyEqual(loaded.armies[1].armyColor[1], 1.0f)
          && NearlyEqual(loaded.armies[1].armyColor[2], 1.0f) && NearlyEqual(loaded.armies[1].armyColor[3], 1.0f),
          "ARMY_02 (armyColor key explicitly present, white): stays white, never overwritten by the backfill");

    Check(NearlyEqual(loaded.armies[2].armyColor[0], Params::kDefaultArmyColors[2][0])
          && NearlyEqual(loaded.armies[2].armyColor[1], Params::kDefaultArmyColors[2][1])
          && NearlyEqual(loaded.armies[2].armyColor[2], Params::kDefaultArmyColors[2][2])
          && NearlyEqual(loaded.armies[2].armyColor[3], Params::kDefaultArmyColors[2][3]),
          "ARMY_03 (roster position 2, no armyColor key): backfilled from kDefaultArmyColors[2], "
          "not position 1 — the skipped explicit-white army does not shift the rotation");
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

    // --- one per-rule stack (MarkersStack): the same clamp behavior on MarkerRuleLayer::
    // symmetry.radialSymmetryRepeatCount (STEP66: promoted from the individual MarkerRule onto its
    // wrapping MarkerRuleLayer), confirming the wiring is not Symmetry-section-specific.
    {
        nlohmann::json document;
        document["MarkersStack"] = nlohmann::json::array();
        nlohmann::json layerJson;
        layerJson["RadialSymmetryRepeatCount"] = 0;
        layerJson["Rules"] = nlohmann::json::array();
        document["MarkersStack"].push_back(layerJson);
        Params::MapRecipe loaded;
        Io::ReadMarkersStackJson(document, loaded);
        Check(!loaded.markerRuleLayers.empty()
              && loaded.markerRuleLayers[0].symmetry.radialSymmetryRepeatCount
                     == Params::radialSymmetryRepeatCountMinimum,
              "MarkersStack[0].RadialSymmetryRepeatCount clamps 0 up to the [2, 12] minimum on import");
    }
    {
        nlohmann::json document;
        document["MarkersStack"] = nlohmann::json::array();
        nlohmann::json layerJson;
        layerJson["RadialSymmetryRepeatCount"] = 500;
        layerJson["Rules"] = nlohmann::json::array();
        document["MarkersStack"].push_back(layerJson);
        Params::MapRecipe loaded;
        Io::ReadMarkersStackJson(document, loaded);
        Check(!loaded.markerRuleLayers.empty()
              && loaded.markerRuleLayers[0].symmetry.radialSymmetryRepeatCount
                     == Params::radialSymmetryRepeatCountMaximum,
              "MarkersStack[0].RadialSymmetryRepeatCount clamps 500 down to the [2, 12] maximum on import");
    }

    // --- the manual-layer stack (MarkerGroups, MarkerInstanceLayer::symmetry): the one IO site of
    // the 8 that used to read this field with a plain, unclamped ReadJsonInteger — the fix this
    // test pins down.
    {
        nlohmann::json document;
        document["MarkerGroups"] = nlohmann::json::array();
        document["MarkerGroups"].push_back(
            nlohmann::json::object({ { "RadialSymmetryRepeatCount", 0 } }));
        Params::MapRecipe loaded;
        Io::ReadMarkerGroupsJson(document, loaded);
        Check(!loaded.markerLayers.empty()
              && loaded.markerLayers[0].symmetry.radialSymmetryRepeatCount
                     == Params::radialSymmetryRepeatCountMinimum,
              "MarkerGroups[0].RadialSymmetryRepeatCount clamps 0 up to the [2, 12] minimum on import");
    }
    {
        nlohmann::json document;
        document["MarkerGroups"] = nlohmann::json::array();
        document["MarkerGroups"].push_back(
            nlohmann::json::object({ { "RadialSymmetryRepeatCount", 500 } }));
        Params::MapRecipe loaded;
        Io::ReadMarkerGroupsJson(document, loaded);
        Check(!loaded.markerLayers.empty()
              && loaded.markerLayers[0].symmetry.radialSymmetryRepeatCount
                     == Params::radialSymmetryRepeatCountMaximum,
              "MarkerGroups[0].RadialSymmetryRepeatCount clamps 500 down to the [2, 12] maximum on import");
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

// STEP60_MarkerInstanceLayer_PARAMS: a hand-built `MarkerGroups` array with two entries, neither
// carrying an `"Id"` key, must legacy-backfill by array index — mirrors `ReadPropGroupsJson`'s
// own (future) STEP56-era retrofit convention, exercised directly here since Props/Decals'
// `layerId` had not landed yet at the time this ticket was authored.
void CheckMarkerGroupsLegacyBackfill() {
    nlohmann::json document;
    document["MarkerGroups"] = nlohmann::json::array();
    document["MarkerGroups"].push_back(nlohmann::json::object({ { "Name", "First" } }));
    document["MarkerGroups"].push_back(nlohmann::json::object({ { "Name", "Second" } }));

    Params::MapRecipe recipe;
    Io::ReadMarkerGroupsJson(document, recipe);
    Check(recipe.markerLayers.size() == 2, "both legacy MarkerGroups entries survive");
    if (recipe.markerLayers.size() == 2) {
        Check(recipe.markerLayers[0].layerId == 0, "the first entry legacy-backfills layerId 0");
        Check(recipe.markerLayers[1].layerId == 1, "the second entry legacy-backfills layerId 1");
    }
}

// STEP106: a hand-built `MarkerGroups` entry with none of `"Locked"`/`"GridSnapEnabled"`/
// `"GridSnapSizeWorldUnits"` present (a legacy file saved before this ticket) leaves the struct's
// own defaults untouched.
void CheckMarkerGroupsLegacyLockAndSnapDefaults() {
    nlohmann::json document;
    document["MarkerGroups"] = nlohmann::json::array();
    document["MarkerGroups"].push_back(nlohmann::json::object({ { "Name", "First" } }));

    Params::MapRecipe recipe;
    Io::ReadMarkerGroupsJson(document, recipe);
    Check(recipe.markerLayers.size() == 1, "the legacy MarkerGroups entry survives");
    if (recipe.markerLayers.empty()) return;
    const Params::MarkerInstanceLayer& layer = recipe.markerLayers[0];
    Check(layer.bLocked == false, "bLocked keeps its struct default (false) when the key is absent");
    Check(layer.bHidden == false,
          "bHidden (STEP144) keeps its struct default (false) when the key is absent");
    Check(layer.bGridSnapEnabled == false,
          "bGridSnapEnabled keeps its struct default (false) when the key is absent");
    Check(NearlyEqual(layer.gridSnapSizeWorldUnits, 1.0f),
          "gridSnapSizeWorldUnits keeps its struct default (1.0f) when the key is absent");
    Check(layer.bColorOverrideEnabled == false,
          "bColorOverrideEnabled keeps its struct default (false) when the key is absent");
    Check(layer.bSymmetryEnabled == true,
          "bSymmetryEnabled keeps its struct default (true) when the key is absent (STEP130, ARCH "
          "§19.24) — every pre-existing/legacy layer's configured symmetry mask stays live");
}

// ARCH §20: the Prop/Decal-typed mirror of CheckMarkerGroupsLegacyLockAndSnapDefaults — a hand-built
// `PropGroups`/`DecalGroups` entry with none of the new fields present (a file saved before this
// ticket) leaves every new struct field on its own default.
void CheckPropDecalGroupsLegacyDefaults() {
    nlohmann::json document;
    document["PropGroups"] = nlohmann::json::array();
    document["PropGroups"].push_back(nlohmann::json::object({ { "Name", "First" } }));
    document["DecalGroups"] = nlohmann::json::array();
    document["DecalGroups"].push_back(nlohmann::json::object({ { "Name", "First" } }));

    Params::MapRecipe recipe;
    Io::ReadPropGroupsJson(document, recipe);
    Io::ReadDecalGroupsJson(document, recipe);
    Check(recipe.propLayers.size() == 1, "the legacy PropGroups entry survives");
    Check(recipe.decalLayers.size() == 1, "the legacy DecalGroups entry survives");
    if (!recipe.propLayers.empty()) {
        const Params::PropInstanceLayer& layer = recipe.propLayers[0];
        Check(layer.bLocked == false, "PropInstanceLayer::bLocked keeps its struct default when absent");
        Check(layer.bHidden == false, "PropInstanceLayer::bHidden keeps its struct default when absent");
        Check(layer.bGridSnapEnabled == false,
              "PropInstanceLayer::bGridSnapEnabled keeps its struct default when absent");
        Check(NearlyEqual(layer.gridSnapSizeWorldUnits, 1.0f),
              "PropInstanceLayer::gridSnapSizeWorldUnits keeps its struct default (1.0f) when absent");
        Check(layer.bColorOverrideEnabled == false,
              "PropInstanceLayer::bColorOverrideEnabled keeps its struct default when absent");
        Check(layer.bSymmetryEnabled == true,
              "PropInstanceLayer::bSymmetryEnabled keeps its struct default (true) when absent");
        Check(layer.propTypeName.empty(), "PropInstanceLayer::propTypeName keeps its struct default (empty) when absent");
    }
    if (!recipe.decalLayers.empty()) {
        const Params::DecalInstanceLayer& layer = recipe.decalLayers[0];
        Check(layer.bLocked == false, "DecalInstanceLayer::bLocked keeps its struct default when absent");
        Check(layer.bHidden == false, "DecalInstanceLayer::bHidden keeps its struct default when absent");
        Check(layer.bGridSnapEnabled == false,
              "DecalInstanceLayer::bGridSnapEnabled keeps its struct default when absent");
        Check(NearlyEqual(layer.gridSnapSizeWorldUnits, 1.0f),
              "DecalInstanceLayer::gridSnapSizeWorldUnits keeps its struct default (1.0f) when absent");
        Check(layer.bColorOverrideEnabled == false,
              "DecalInstanceLayer::bColorOverrideEnabled keeps its struct default when absent");
        Check(layer.bSymmetryEnabled == true,
              "DecalInstanceLayer::bSymmetryEnabled keeps its struct default (true) when absent");
    }
}

// STEP60_MarkerInstanceLayer_PARAMS: a hand-built `markers` entry with an out-of-range
// `layerIndex` (5, against zero MarkerGroups entries) must clamp to 0 on import and log a
// warning — mirrors MapImporter_PropsDecals_IO_Test.cpp's own `ClampPropLayerIndex` coverage.
void CheckMarkerLayerIndexClampsOnImport() {
    nlohmann::json document;
    nlohmann::json transformJson;
    transformJson["layerIndex"] = 5;
    nlohmann::json groupJson;
    groupJson["resource"] = false;
    groupJson["transforms"] = nlohmann::json::object({ { "Mex 0", transformJson } });
    document["markers"] = nlohmann::json::object({ { "Alloys", groupJson } });

    Params::MapRecipe recipe;   // outRecipe.markerLayers is empty: 5 is out of range against it
    Io::MapImportResult result;
    Io::ReadMarkersJson(document, recipe, result);

    Check(recipe.markers.size() == 1, "one marker group survives");
    if (!recipe.markers.empty() && !recipe.markers[0].transforms.empty()) {
        Check(recipe.markers[0].transforms[0].layerIndex == 0,
              "the out-of-range marker layerIndex (5) clamps to 0 on import");
    }
    Check(result.warningCount > 0, "the out-of-range marker layerIndex clamp is logged as a warning");
}

// STEP114: a hand-built `markers` transform JSON object with no `"iconNameOverride"` key present
// (a legacy file saved before this ticket) leaves the struct's own default (empty = type default)
// untouched — mirrors CheckMarkerGroupsLegacyLockAndSnapDefaults's own shape.
void CheckMarkerIconNameOverrideLegacyDefault() {
    nlohmann::json document;
    nlohmann::json transformJson;   // deliberately no "iconNameOverride" key
    nlohmann::json groupJson;
    groupJson["resource"] = false;
    groupJson["transforms"] = nlohmann::json::object({ { "Mex 0", transformJson } });
    document["markers"] = nlohmann::json::object({ { "Alloys", groupJson } });

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Io::ReadMarkersJson(document, recipe, result);

    Check(recipe.markers.size() == 1, "one marker group survives");
    if (recipe.markers.empty() || recipe.markers[0].transforms.empty()) return;
    Check(recipe.markers[0].transforms[0].iconNameOverride.empty(),
          "iconNameOverride keeps its struct default (empty) when the key is absent");
}

// STEP119: a hand-built `MarkerLayerBundles` array entry with none of `"Identifier"`/`"Name"`/
// `"ParentBundleIdentifier"`/`"MarkerTypeName"`/`"AssemblyIdentifier"` present (a legacy/foreign
// file with no Bundle concept at all) leaves the struct's own defaults untouched — mirrors
// CheckMarkerGroupsLegacyLockAndSnapDefaults's exact shape.
void CheckMarkerLayerBundlesLegacyDefault() {
    nlohmann::json document;
    document["MarkerLayerBundles"] = nlohmann::json::array();
    document["MarkerLayerBundles"].push_back(nlohmann::json::object());   // deliberately empty

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Io::ReadMarkerLayerBundlesJson(document, recipe, result);

    Check(recipe.markerLayerBundles.size() == 1, "the legacy MarkerLayerBundles entry survives");
    if (recipe.markerLayerBundles.empty()) return;
    const Params::MarkerLayerBundle& bundle = recipe.markerLayerBundles[0];
    Check(bundle.identifier == -1, "identifier keeps its struct default (-1) when the key is absent");
    Check(bundle.name.empty(), "name keeps its struct default (empty) when the key is absent");
    Check(bundle.parentBundleIdentifier == -1,
          "parentBundleIdentifier keeps its struct default (-1) when the key is absent");
    Check(bundle.markerTypeName.empty(),
          "markerTypeName keeps its struct default (empty) when the key is absent");
    Check(bundle.assemblyIdentifier == -1,
          "assemblyIdentifier keeps its struct default (-1) when the key is absent");
}

// STEP119: the two merged `ParentBundleIdentifier` back-reference keys default to -1 (root) when
// absent on a legacy `MarkerGroups`/`MarkersStack` entry, mirroring the shape above.
void CheckMergedParentBundleIdentifierLegacyDefault() {
    nlohmann::json markerGroupsDocument;
    markerGroupsDocument["MarkerGroups"] = nlohmann::json::array();
    markerGroupsDocument["MarkerGroups"].push_back(nlohmann::json::object({ { "Name", "First" } }));
    Params::MapRecipe markerGroupsRecipe;
    Io::ReadMarkerGroupsJson(markerGroupsDocument, markerGroupsRecipe);
    Check(markerGroupsRecipe.markerLayers.size() == 1, "the legacy MarkerGroups entry survives");
    if (!markerGroupsRecipe.markerLayers.empty())
        Check(markerGroupsRecipe.markerLayers[0].parentBundleIdentifier == -1,
              "MarkerInstanceLayer::parentBundleIdentifier defaults to -1 when the key is absent");

    nlohmann::json markersStackDocument;
    markersStackDocument["MarkersStack"] = nlohmann::json::array();
    markersStackDocument["MarkersStack"].push_back(nlohmann::json::object({ { "Name", "First" } }));
    Params::MapRecipe markersStackRecipe;
    Io::ReadMarkersStackJson(markersStackDocument, markersStackRecipe);
    Check(markersStackRecipe.markerRuleLayers.size() == 1, "the legacy MarkersStack entry survives");
    if (!markersStackRecipe.markerRuleLayers.empty())
        Check(markersStackRecipe.markerRuleLayers[0].parentBundleIdentifier == -1,
              "MarkerRuleLayer::parentBundleIdentifier defaults to -1 when the key is absent");
}

// STEP124: the two merged `MarkerTypeName` fields default to empty (std::string's own default) when
// absent on a legacy `MarkerGroups`/`MarkersStack` entry, mirroring
// CheckMergedParentBundleIdentifierLegacyDefault's exact two-part shape.
void CheckMergedMarkerTypeNameLegacyDefault() {
    nlohmann::json markerGroupsDocument;
    markerGroupsDocument["MarkerGroups"] = nlohmann::json::array();
    markerGroupsDocument["MarkerGroups"].push_back(nlohmann::json::object({ { "Name", "First" } }));
    Params::MapRecipe markerGroupsRecipe;
    Io::ReadMarkerGroupsJson(markerGroupsDocument, markerGroupsRecipe);
    Check(markerGroupsRecipe.markerLayers.size() == 1, "the legacy MarkerGroups entry survives");
    if (!markerGroupsRecipe.markerLayers.empty())
        Check(markerGroupsRecipe.markerLayers[0].markerTypeName.empty(),
              "MarkerInstanceLayer::markerTypeName defaults to empty when the key is absent");

    nlohmann::json markersStackDocument;
    markersStackDocument["MarkersStack"] = nlohmann::json::array();
    markersStackDocument["MarkersStack"].push_back(nlohmann::json::object({ { "Name", "First" } }));
    Params::MapRecipe markersStackRecipe;
    Io::ReadMarkersStackJson(markersStackDocument, markersStackRecipe);
    Check(markersStackRecipe.markerRuleLayers.size() == 1, "the legacy MarkersStack entry survives");
    if (!markersStackRecipe.markerRuleLayers.empty())
        Check(markersStackRecipe.markerRuleLayers[0].markerTypeName.empty(),
              "MarkerRuleLayer::markerTypeName defaults to empty when the key is absent");
}

// STEP127 item 3 — the legacy-import-default-jump question the ticket flags: on a document with NO
// "GlobalMarkerSettings" key at all (confirmed real for a non-SanGen-authored .sanmap; every
// SanGen-authored file, v1 or v2, always carries the 9 legacy fields that GlobalMarkerSettings_
// Migrate_V2 relocates into this object before this reader ever runs — see that migration's own
// header), ReadGlobalMarkerSettingsJson early-returns and leaves recipe.globalMarkerSettings at its
// struct default. Before STEP127 that default was 0.17f; after, 0.50f — so a legacy/foreign map's
// markers DO jump in rendered size on this change. Not a regression this ticket can silently
// dismiss (mirrors CheckMergedParentBundleIdentifierLegacyDefault's exact shape).
void CheckGlobalMarkerSettingsLegacyDefault() {
    nlohmann::json document;   // deliberately no "GlobalMarkerSettings" key at all
    Params::MapRecipe recipe;
    Io::ReadGlobalMarkerSettingsJson(document, recipe);

    Check(NearlyEqual(recipe.globalMarkerSettings.scaleAlloy, 0.50f)
          && NearlyEqual(recipe.globalMarkerSettings.scalePlasma, 0.50f)
          && NearlyEqual(recipe.globalMarkerSettings.scaleSpawn, 0.50f),
          "GlobalMarkerSettings::scaleAlloy/Plasma/Spawn keep the struct default (0.50f, STEP127) "
          "when the GlobalMarkerSettings key is absent entirely — confirms the legacy-import-"
          "default-jump risk STEP127 flags rather than silently assuming it away");
}

// STEP124 / ARCH §19.16: instanceIdentifier's legacy-backfill counter is threaded across the WHOLE
// nested `markers` walk, never reset per group — a group with an explicit InstanceIdentifier key
// still consumes (advances) the counter before being overwritten.
void CheckMarkerInstanceIdentifierLegacyBackfillAcrossGroups() {
    // Group "Alloys" (alphabetically first — see §5's load-bearing note on nlohmann::json's
    // sorted, non-insertion-order object iteration) has two transforms: "AAA" (no
    // InstanceIdentifier key) and "BBB" (an explicit, out-of-band value, 500).
    // NOTE (deviation from the work-order's literal text): a default-constructed `nlohmann::json`
    // is JSON NULL, not an empty OBJECT — `ReadNameKeyedObject`'s existing, correct
    // `if (valueJson.is_object())` gate (a non-object entry yields a default-constructed item and
    // never invokes the per-item reader at all) would then skip AAA/CCC entirely, so the backfill
    // line inside ReadMarkerTransformJson would never run for them, leaving both at the struct
    // default (-1) instead of exercising the backfill. Using `nlohmann::json::object()` makes each
    // entry a genuine (empty) object — present, with no "InstanceIdentifier" key — matching the
    // ticket's own stated intent ("no InstanceIdentifier key") rather than its literal code.
    nlohmann::json transformAAA = nlohmann::json::object();   // no "InstanceIdentifier" key
    nlohmann::json transformBBB; transformBBB["InstanceIdentifier"] = 500;
    nlohmann::json groupAlloys;
    groupAlloys["resource"] = true;
    groupAlloys["transforms"] = nlohmann::json::object({ { "AAA", transformAAA }, { "BBB", transformBBB } });

    // Group "Spawn" (alphabetically second) has one transform, also no InstanceIdentifier key —
    // proves the counter is NOT reset per group (ARCH §19.16's core requirement).
    nlohmann::json transformCCC = nlohmann::json::object();   // no "InstanceIdentifier" key
    nlohmann::json groupSpawn;
    groupSpawn["resource"] = false;
    groupSpawn["transforms"] = nlohmann::json::object({ { "CCC", transformCCC } });

    nlohmann::json document;
    document["markers"] = nlohmann::json::object({ { "Alloys", groupAlloys }, { "Spawn", groupSpawn } });

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Io::ReadMarkersJson(document, recipe, result);

    Check(recipe.markers.size() == 2, "both marker groups survive");
    if (recipe.markers.size() != 2) return;
    Check(recipe.markers[0].name == "Alloys" && recipe.markers[1].name == "Spawn",
          "groups are visited in nlohmann::json's own sorted-key order (Alloys before Spawn)");
    Check(recipe.markers[0].transforms.size() == 2 && recipe.markers[1].transforms.size() == 1,
          "both transforms in Alloys and the one transform in Spawn survive");
    if (recipe.markers[0].transforms.size() != 2 || recipe.markers[1].transforms.empty()) return;

    // Counter starts at 0: AAA (no key) backfills to 0; BBB (has key) still CONSUMES counter
    // slot 1 before being overwritten to 500 (the counter always advances); CCC (no key, in the
    // SECOND group) backfills to 2 — proving the counter is threaded across groups, not reset.
    Check(recipe.markers[0].transforms[0].instanceIdentifier == 0,
          "AAA (no key) backfills to the counter's value at its own position (0)");
    Check(recipe.markers[0].transforms[1].instanceIdentifier == 500,
          "BBB's explicit InstanceIdentifier (500) overwrites the counter's positional default");
    Check(recipe.markers[1].transforms[0].instanceIdentifier == 2,
          "CCC (no key, in the SECOND group) backfills to 2, not 0 — the counter is threaded "
          "across the whole nested walk, never reset per group (ARCH §19.16)");
}

// STEP119 / ARCH §19.12: a 2-cycle ParentBundleIdentifier chain ({1,2},{2,1}) is detected and
// repaired — both entries treated as root — with one logged warning per cyclic entry, never a
// refusal. Mirrors CheckMarkerLayerSynthesisOnEmptyMarkerGroups's direct-call/result.warningCount
// assertion style.
void CheckMarkerLayerBundleCycleRepairOnImport() {
    nlohmann::json document;
    document["MarkerLayerBundles"] = nlohmann::json::array({
        nlohmann::json::object({ { "Identifier", 1 }, { "ParentBundleIdentifier", 2 } }),
        nlohmann::json::object({ { "Identifier", 2 }, { "ParentBundleIdentifier", 1 } }),
    });

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Io::ReadMarkerLayerBundlesJson(document, recipe, result);

    Check(recipe.markerLayerBundles.size() == 2, "both cyclic MarkerLayerBundles entries survive");
    if (recipe.markerLayerBundles.size() == 2) {
        Check(recipe.markerLayerBundles[0].parentBundleIdentifier == -1,
              "the first cyclic entry is repaired to root");
        Check(recipe.markerLayerBundles[1].parentBundleIdentifier == -1,
              "the second cyclic entry is repaired to root");
    }
    Check(result.warningCount == 2, "one warning is logged per cyclic entry");
}

// STEP119 / ARCH §19.12: a valid 3-level root/child/grandchild chain ({1,-1},{2,1},{3,2}) is left
// completely untouched — the cycle repair is a true no-op on non-cyclic data, no warnings.
void CheckMarkerLayerBundleCycleRepairIsNoOpOnValidChain() {
    nlohmann::json document;
    document["MarkerLayerBundles"] = nlohmann::json::array({
        nlohmann::json::object({ { "Identifier", 1 }, { "ParentBundleIdentifier", -1 } }),
        nlohmann::json::object({ { "Identifier", 2 }, { "ParentBundleIdentifier", 1 } }),
        nlohmann::json::object({ { "Identifier", 3 }, { "ParentBundleIdentifier", 2 } }),
    });

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Io::ReadMarkerLayerBundlesJson(document, recipe, result);

    Check(recipe.markerLayerBundles.size() == 3, "all three valid-chain entries survive");
    if (recipe.markerLayerBundles.size() == 3) {
        Check(recipe.markerLayerBundles[0].parentBundleIdentifier == -1,
              "the root entry's parentBundleIdentifier is unchanged");
        Check(recipe.markerLayerBundles[1].parentBundleIdentifier == 1,
              "the child entry's parentBundleIdentifier is unchanged");
        Check(recipe.markerLayerBundles[2].parentBundleIdentifier == 2,
              "the grandchild entry's parentBundleIdentifier is unchanged");
    }
    Check(result.warningCount == 0, "a valid, non-cyclic chain produces zero warnings");
}

// ARCH §20: the Prop-typed mirror of CheckMarkerLayerBundleCycleRepairOnImport/
// CheckMarkerLayerBundleCycleRepairIsNoOpOnValidChain, combined into one function since the
// underlying cycle predicate (WouldReparentPropLayerBundleCreateCycle) is a verbatim duplicate of
// the Marker one — both the cyclic-repair and the valid-chain no-op cases are exercised.
void CheckPropLayerBundleCycleRepair() {
    nlohmann::json cyclicDocument;
    cyclicDocument["PropLayerBundles"] = nlohmann::json::array({
        nlohmann::json::object({ { "Identifier", 1 }, { "ParentBundleIdentifier", 2 } }),
        nlohmann::json::object({ { "Identifier", 2 }, { "ParentBundleIdentifier", 1 } }),
    });
    Params::MapRecipe cyclicRecipe;
    Io::MapImportResult cyclicResult;
    Io::ReadPropLayerBundlesJson(cyclicDocument, cyclicRecipe, cyclicResult);
    Check(cyclicRecipe.propLayerBundles.size() == 2, "both cyclic PropLayerBundles entries survive");
    if (cyclicRecipe.propLayerBundles.size() == 2) {
        Check(cyclicRecipe.propLayerBundles[0].parentBundleIdentifier == -1, "the first cyclic entry is repaired to root");
        Check(cyclicRecipe.propLayerBundles[1].parentBundleIdentifier == -1, "the second cyclic entry is repaired to root");
    }
    Check(cyclicResult.warningCount == 2, "one warning is logged per cyclic PropLayerBundle entry");

    nlohmann::json validDocument;
    validDocument["PropLayerBundles"] = nlohmann::json::array({
        nlohmann::json::object({ { "Identifier", 1 }, { "ParentBundleIdentifier", -1 } }),
        nlohmann::json::object({ { "Identifier", 2 }, { "ParentBundleIdentifier", 1 } }),
    });
    Params::MapRecipe validRecipe;
    Io::MapImportResult validResult;
    Io::ReadPropLayerBundlesJson(validDocument, validRecipe, validResult);
    Check(validResult.warningCount == 0, "a valid, non-cyclic PropLayerBundle chain produces zero warnings");
}

// ARCH §20: the Decal-typed mirror of CheckPropLayerBundleCycleRepair.
void CheckDecalLayerBundleCycleRepair() {
    nlohmann::json cyclicDocument;
    cyclicDocument["DecalLayerBundles"] = nlohmann::json::array({
        nlohmann::json::object({ { "Identifier", 1 }, { "ParentBundleIdentifier", 2 } }),
        nlohmann::json::object({ { "Identifier", 2 }, { "ParentBundleIdentifier", 1 } }),
    });
    Params::MapRecipe cyclicRecipe;
    Io::MapImportResult cyclicResult;
    Io::ReadDecalLayerBundlesJson(cyclicDocument, cyclicRecipe, cyclicResult);
    Check(cyclicRecipe.decalLayerBundles.size() == 2, "both cyclic DecalLayerBundles entries survive");
    if (cyclicRecipe.decalLayerBundles.size() == 2) {
        Check(cyclicRecipe.decalLayerBundles[0].parentBundleIdentifier == -1, "the first cyclic entry is repaired to root");
        Check(cyclicRecipe.decalLayerBundles[1].parentBundleIdentifier == -1, "the second cyclic entry is repaired to root");
    }
    Check(cyclicResult.warningCount == 2, "one warning is logged per cyclic DecalLayerBundle entry");

    nlohmann::json validDocument;
    validDocument["DecalLayerBundles"] = nlohmann::json::array({
        nlohmann::json::object({ { "Identifier", 1 }, { "ParentBundleIdentifier", -1 } }),
        nlohmann::json::object({ { "Identifier", 2 }, { "ParentBundleIdentifier", 1 } }),
    });
    Params::MapRecipe validRecipe;
    Io::MapImportResult validResult;
    Io::ReadDecalLayerBundlesJson(validDocument, validRecipe, validResult);
    Check(validResult.warningCount == 0, "a valid, non-cyclic DecalLayerBundle chain produces zero warnings");
}

// STEP115: a real, non-SanGen-authored `.sanmap` never carries `MarkerGroups` — build a raw document
// with a `"markers"` object containing two type-groups and explicitly NO `"MarkerGroups"` key.
// ReadMarkerGroupsJson is a confirmed no-op (it never fabricates the key); ReconcileMarkerLayers then
// synthesizes one layer per marker group and repoints every transform's layerIndex at it.
void CheckMarkerLayerSynthesisOnEmptyMarkerGroups() {
    nlohmann::json document;   // deliberately no "MarkerGroups" key
    nlohmann::json spawnTransformJson;
    nlohmann::json spawnGroupJson;
    spawnGroupJson["transforms"] = nlohmann::json::object({ { "Spawn 0", spawnTransformJson } });

    nlohmann::json alloyTransformOneJson;
    nlohmann::json alloyTransformTwoJson;
    nlohmann::json alloyGroupJson;
    alloyGroupJson["transforms"] = nlohmann::json::object({
        { "Mex 0", alloyTransformOneJson }, { "Mex 1", alloyTransformTwoJson } });

    document["markers"] = nlohmann::json::object({
        { "Spawn", spawnGroupJson }, { "Alloys", alloyGroupJson } });

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Io::ReadMarkerGroupsJson(document, recipe);
    Check(recipe.markerLayers.empty(), "ReadMarkerGroupsJson does not fabricate MarkerGroups");
    Io::ReadMarkersJson(document, recipe, result);
    Io::ReconcileMarkerLayers(recipe, result);

    Check(recipe.markerLayers.size() == 2, "one synthesized layer per marker type group");
    if (recipe.markerLayers.size() != 2 || recipe.markers.size() != 2) return;
    // ReadNameKeyedObject (MapImporter_Markers_IO.cpp) walks the JSON object's own key-iteration
    // order — nlohmann::json (non-ordered) is alphabetical by default, so "Alloys" precedes "Spawn".
    Check(recipe.markerLayers[0].name == "Alloys" && recipe.markerLayers[1].name == "Spawn",
          "synthesized layer names match the real marker-group key-iteration order");
    Check(recipe.markerLayers[0].layerId == 0 && recipe.markerLayers[1].layerId == 1,
          "synthesized layerId is sequential");
    for (const Params::MarkerInstanceGroup& group : recipe.markers) {
        const int expectedLayerIndex = group.name == "Alloys" ? 0 : 1;
        for (const Params::MarkerTransform& transform : group.transforms)
            Check(transform.layerIndex == expectedLayerIndex,
                  "every transform in a synthesized-layer group points at its real layer");
    }
    const Params::MarkerInstanceLayer defaultLayer;
    const Params::MarkerInstanceLayer& synthesizedLayer = recipe.markerLayers[0];
    Check(NearlyEqual(synthesizedLayer.color[0], defaultLayer.color[0])
          && NearlyEqual(synthesizedLayer.color[1], defaultLayer.color[1])
          && NearlyEqual(synthesizedLayer.color[2], defaultLayer.color[2])
          && NearlyEqual(synthesizedLayer.color[3], defaultLayer.color[3])
          && NearlyEqual(synthesizedLayer.iconScale, defaultLayer.iconScale)
          && synthesizedLayer.bLocked == defaultLayer.bLocked
          && synthesizedLayer.bGridSnapEnabled == defaultLayer.bGridSnapEnabled
          && NearlyEqual(synthesizedLayer.gridSnapSizeWorldUnits, defaultLayer.gridSnapSizeWorldUnits),
          "a synthesized layer is struct-default in every field but name/layerId/markerTypeName"
          " (white-as-unset)");
    // NEW — human's own bug report: an unset markerTypeName meant a synthesized layer never matched
    // any Type-section, so a real import's Alloy markers never showed up anywhere but the flat
    // fallback list. Canonicalized: the real group name is plural ("Alloys") but the Type-section
    // this layer must match is singular ("Alloy").
    Check(recipe.markerLayers[0].markerTypeName == "Alloy" && recipe.markerLayers[1].markerTypeName == "Spawn",
          "a synthesized layer's markerTypeName is the alias-canonicalized marker-group name");
    Check(result.warningCount == 1,
          "exactly one aggregate warning fires for the whole synthesis, not one per layer");
}

// STEP115: a document WITH `MarkerGroups` present must be left provably untouched — no double
// synthesis. Reuses FillFixtureMarkersAndChains's existing fixture (already populates one
// MarkerInstanceLayer and one marker group).
void CheckMarkerLayerSynthesisIsNoOpWhenMarkerGroupsPresent() {
    Params::MapRecipe fixture;
    FillFixtureMarkersAndChains(fixture);
    nlohmann::ordered_json document;
    document["MarkerGroups"] = Io::BuildMarkerGroupsJson(fixture);
    document["markers"] = Io::BuildMarkersJson(fixture);

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadMarkerGroupsJson(document, loaded);
    Io::ReadMarkersJson(document, loaded, result);
    Check(loaded.markerLayers.size() == 1, "the one real MarkerGroups entry survives");
    const int warningCountBeforeReconcile = result.warningCount;

    Io::ReconcileMarkerLayers(loaded, result);

    Check(loaded.markerLayers.size() == 1, "ReconcileMarkerLayers is a no-op when MarkerGroups was present");
    if (!loaded.markerLayers.empty()) {
        const Params::MarkerInstanceLayer& layer = loaded.markerLayers[0];
        Check(layer.layerId == fixture.markerLayers[0].layerId
              && layer.name == fixture.markerLayers[0].name
              && NearlyEqual(layer.color[0], fixture.markerLayers[0].color[0])
              && layer.bLocked == fixture.markerLayers[0].bLocked
              && layer.bGridSnapEnabled == fixture.markerLayers[0].bGridSnapEnabled
              && NearlyEqual(layer.gridSnapSizeWorldUnits, fixture.markerLayers[0].gridSnapSizeWorldUnits),
              "the real MarkerGroups entry is byte-identical to before the Reconcile call");
    }
    Check(result.warningCount == warningCountBeforeReconcile,
          "no new warning fires when MarkerGroups was already present");
}

// STEP115: partial coverage (layers present but not covering every group) is explicitly deferred —
// this test pins that behavior so it isn't silently changed by a future edit. The guard is
// `markerLayers.empty()`, not "every group covered".
void CheckMarkerLayerSynthesisPartialCoverageIsNoOp() {
    Params::MapRecipe loaded;
    Params::MarkerInstanceLayer onlyLayer;
    onlyLayer.name = "Spawn";
    onlyLayer.layerId = 0;
    loaded.markerLayers.push_back(onlyLayer);   // exactly ONE entry

    Params::MarkerTransform spawnTransform;   // points at the real layer 0
    spawnTransform.layerIndex = 0;
    Params::MarkerInstanceGroup spawnGroup;
    spawnGroup.name = "Spawn";
    spawnGroup.transforms.push_back(spawnTransform);
    loaded.markers.push_back(spawnGroup);

    Params::MarkerTransform alloyTransform;   // left at its default layerIndex = 0 too — no
    Params::MarkerInstanceGroup alloyGroup;   // synthesis has run for this group
    alloyGroup.name = "Alloys";
    alloyGroup.transforms.push_back(alloyTransform);
    loaded.markers.push_back(alloyGroup);   // TWO groups total, only one covered by markerLayers

    Io::MapImportResult result;
    Io::ReconcileMarkerLayers(loaded, result);

    Check(loaded.markerLayers.size() == 1,
          "partial coverage is untouched — the guard is markerLayers.empty(), not full coverage");
    Check(result.warningCount == 0, "no warning fires for the deferred partial-coverage case");
}

// STEP99_BakedImageLayer_PARAMS acceptance test: a document with none of the three keys
// (Baked/BakedImagePath/LayerIdentifier) present leaves a freshly constructed Layer's defaults
// (false/empty/-1) untouched — mirrors CheckStratumAppearanceIdentityFieldsMissingKeysImportWithSaneFallback's
// own style, routed through the real public entry point (ReadHeightmapStackJson), not the file-
// local ReadLayerJson directly.
void CheckLayerMissingBakedKeysLeaveDefaults() {
    nlohmann::json document;
    nlohmann::json layerJson = nlohmann::json::object();   // no Baked/BakedImagePath/LayerIdentifier
    nlohmann::json geoLayerJson;
    geoLayerJson["Layers"] = nlohmann::json::array({ layerJson });
    document["HeightmapStack"]["GeoLayers"] = nlohmann::json::array({ geoLayerJson });

    Params::LayerStack layerStack;
    Io::ReadHeightmapStackJson(document, layerStack);

    Check(layerStack.geoLayers.size() == 1 && layerStack.geoLayers[0].layers.size() == 1,
          "the hand-built document still yields one GeoLayer with one Layer");
    if (layerStack.geoLayers.empty() || layerStack.geoLayers[0].layers.empty()) return;
    const Params::Layer& layer = layerStack.geoLayers[0].layers[0];
    Check(layer.bBaked == false && layer.bakedImagePath.empty() && layer.layerIdentifier == -1,
          "a document with none of the three baked/image-source keys leaves the freshly "
          "constructed Layer's defaults (false/empty/-1) untouched");
}

// STEP99_BakedImageLayer_PARAMS acceptance test: NextLayerIdentifier on an empty stack returns 0;
// on a stack containing identifiers {0, 2} across two different GeoLayers returns 3.
void CheckNextLayerIdentifier() {
    Params::LayerStack emptyStack;
    Check(Params::NextLayerIdentifier(emptyStack) == 0,
          "NextLayerIdentifier on an empty stack returns 0");

    Params::LayerStack populatedStack;
    Params::GeoLayer geoLayerA;
    Params::Layer layerA; layerA.layerIdentifier = 0;
    geoLayerA.layers.push_back(layerA);
    populatedStack.geoLayers.push_back(geoLayerA);

    Params::GeoLayer geoLayerB;
    Params::Layer layerB; layerB.layerIdentifier = 2;
    geoLayerB.layers.push_back(layerB);
    populatedStack.geoLayers.push_back(geoLayerB);

    Check(Params::NextLayerIdentifier(populatedStack) == 3,
          "NextLayerIdentifier on a stack containing identifiers {0, 2} across two different "
          "GeoLayers returns 3");
}

} // namespace MapFormatTest
} // namespace SanmapGen

int main() {
    SanmapGen::MapFormatTest::RunRoundTripTests();
    SanmapGen::MapFormatTest::CheckKnownTopLevelSanmapKeysCoverage();
    SanmapGen::MapFormatTest::CheckUnrecognizedSkyboxIntensityModeFallsBackSafely();
    SanmapGen::MapFormatTest::CheckArmyColorBackfillUsesKeyPresenceNotValueComparison();
    SanmapGen::MapFormatTest::CheckStratumLayersCardinalityMismatchWarns();
    SanmapGen::MapFormatTest::CheckStratumGenerationSettingsCardinalityMismatchWarns();
    SanmapGen::MapFormatTest::CheckAccumulationReaderToleratesUnrecognizedKeys();
    SanmapGen::MapFormatTest::CheckRadialSymmetryRepeatCountClampsOnImport();
    SanmapGen::MapFormatTest::CheckMarkerRuleLayerTwoLevelRoundTrip();
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
    SanmapGen::MapFormatTest::CheckMarkerGroupsLegacyBackfill();
    SanmapGen::MapFormatTest::CheckMarkerGroupsLegacyLockAndSnapDefaults();
    SanmapGen::MapFormatTest::CheckPropDecalGroupsLegacyDefaults();
    SanmapGen::MapFormatTest::CheckMarkerLayerIndexClampsOnImport();
    SanmapGen::MapFormatTest::CheckMarkerIconNameOverrideLegacyDefault();
    SanmapGen::MapFormatTest::CheckMarkerLayerBundlesLegacyDefault();
    SanmapGen::MapFormatTest::CheckMergedParentBundleIdentifierLegacyDefault();
    SanmapGen::MapFormatTest::CheckMergedMarkerTypeNameLegacyDefault();
    SanmapGen::MapFormatTest::CheckGlobalMarkerSettingsLegacyDefault();
    SanmapGen::MapFormatTest::CheckMarkerInstanceIdentifierLegacyBackfillAcrossGroups();
    SanmapGen::MapFormatTest::CheckMarkerLayerBundleCycleRepairOnImport();
    SanmapGen::MapFormatTest::CheckMarkerLayerBundleCycleRepairIsNoOpOnValidChain();
    SanmapGen::MapFormatTest::CheckPropLayerBundleCycleRepair();
    SanmapGen::MapFormatTest::CheckDecalLayerBundleCycleRepair();
    SanmapGen::MapFormatTest::CheckMarkerLayerSynthesisOnEmptyMarkerGroups();
    SanmapGen::MapFormatTest::CheckMarkerLayerSynthesisIsNoOpWhenMarkerGroupsPresent();
    SanmapGen::MapFormatTest::CheckMarkerLayerSynthesisPartialCoverageIsNoOp();
    SanmapGen::MapFormatTest::CheckLayerMissingBakedKeysLeaveDefaults();
    SanmapGen::MapFormatTest::CheckNextLayerIdentifier();
    SanmapGen::MapFormatTest::RunValidationTests();
    SanmapGen::MapFormatTest::RunBakedFieldTests();
    if (SanmapGen::MapFormatTest::FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", SanmapGen::MapFormatTest::FailureCount());
    return 1;
}

// MapImporter_IO_Test.cpp — acceptance test for "Load Sanmap" (section D / PARITY_BACKLOG PB-1).
// This unit holds the binary's main(), the fixture recipe, and the headline check: a populated
// MapRecipe written by MapExporter and read back by MapImporter compares equal field for field.
// The validation and baked-field halves live in the two sibling units (MapFormat_TestSupport_IO.h).
#include "MapFormat_TestSupport_IO.h"
#include "MapImporter_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "MapExporter_IO.h"

namespace SanmapGen {
namespace MapFormatTest {
namespace {

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

void CheckLayerStackAndRules(const Params::MapRecipe& original, const Params::MapRecipe& loaded) {
    Check(loaded.layerStack.geoLayers.size() == original.layerStack.geoLayers.size(),
          "the geo-layer count survives");
    if (!loaded.layerStack.geoLayers.empty()) {
        const Params::GeoLayer& geoLayer = loaded.layerStack.geoLayers[0];
        Check(geoLayer.name == "Ridges" && geoLayer.bErodeBelow && geoLayer.stratumIndex == 2,
              "the geo-layer's own settings survive");
        Check(geoLayer.layers.size() == 1, "and it kept its noise layer");
        if (!geoLayer.layers.empty()) {
            const Params::Layer& layer = geoLayer.layers[0];
            Check(layer.name == "Base Noise" && layer.octaves == 6, "with its name and octaves");
            Check(NearlyEqual(layer.frequency, 0.031f) && NearlyEqual(layer.opacity, 0.75f)
                  && NearlyEqual(layer.levelsMidtones, 1.4f), "and its noise/blend/levels scalars");
        }
    }
    Check(loaded.strata.size() == 1 && loaded.strata[0].bSlopeGateEnabled
          && NearlyEqual(loaded.strata[0].maximumSlopeDegrees, 55.0f)
          && NearlyEqual(loaded.strata[0].tileCount, 24.0f), "the stratum settings survive");
    Check(loaded.markerRules.size() == 1 && loaded.markerRules[0].count == 8
          && loaded.markerRules[0].symmetryMask == 1, "the marker rules survive");
    Check(loaded.propRules.size() == 1 && loaded.propRules[0].bAvoidWater
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

void FillFixtureLayerStackAndStrata(Params::MapRecipe& recipe) {
    Params::GeoLayer geoLayer;
    geoLayer.name = "Ridges";
    geoLayer.bErodeBelow = true;
    geoLayer.stratumIndex = 2;
    Params::Layer layer;
    layer.name = "Base Noise";
    layer.frequency = 0.031f;
    layer.octaves = 6;
    layer.opacity = 0.75f;
    layer.levelsMidtones = 1.4f;
    geoLayer.layers.push_back(layer);
    recipe.layerStack.geoLayers.push_back(geoLayer);

    Params::Stratum stratum;
    stratum.bSlopeGateEnabled = true;
    stratum.maximumSlopeDegrees = 55.0f;
    stratum.tileCount = 24.0f;
    stratum.tintRed = 0.4f;
    recipe.strata.push_back(stratum);
}

void FillFixturePlacementRules(Params::MapRecipe& recipe) {
    Params::MarkerRule markerRule;
    markerRule.count = 8;
    markerRule.clearanceSpacing = 14.0f;
    markerRule.symmetryMask = 1;
    recipe.markerRules.push_back(markerRule);
    Params::PropRule propRule;
    propRule.density = 0.4f;
    propRule.bAvoidWater = true;
    propRule.bSymmetryUseGlobal = false;
    propRule.symmetryMask = 2;
    recipe.propRules.push_back(propRule);
    Params::DecalRule decalRule;
    decalRule.spacingMinimum = 6.0f;
    decalRule.bSymmetryUseGlobal = false;
    decalRule.symmetryMask = 8;
    recipe.decalRules.push_back(decalRule);
    Params::UnitRule unitRule;
    unitRule.armyIndex = 2;
    unitRule.count = 5;
    unitRule.bSymmetryUseGlobal = false;
    unitRule.symmetryMask = 4;
    recipe.unitRules.push_back(unitRule);
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

} // namespace

Params::MapRecipe BuildPopulatedRecipe() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;
    recipe.geometry.seed = 1337u;
    recipe.geometry.terrainMinHeight = 12.0f;
    recipe.geometry.terrainMaxHeight = 300.0f;
    recipe.geometry.bScaleFeaturesToMapSize = false;
    recipe.geometry.worldUnitsPerCell = 2.5f;
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
    return recipe;
}

void RunRoundTripTests() {
    const Params::MapRecipe original = BuildPopulatedRecipe();
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(original);
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(documentText, loaded, Io::MapImportOptions(), result),
          "the exporter's own document parses");
    Check(result.warningCount == 0, "with no warning: the two halves agree key for key");
    CheckGeometryAndWater(original, loaded);
    CheckLayerStackAndRules(original, loaded);
    CheckArmiesAndAreas(original, loaded);
    CheckMarkersAndChains(original, loaded);
    CheckPropsAndDecals(original, loaded);
    CheckAtmosphere(original, loaded);
}

// A pure-reader check, deliberately NOT routed through ParseSanmapJsonText/RunRoundTripTests
// (which asserts warningCount == 0 on a clean document) — an unrecognized skyboxIntensityMode
// string must fall back to Exposure with a LOGGED warning, never a crash (STEP9_
// Atmosphere_PARAMS_IO's acceptance test).
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

} // namespace MapFormatTest
} // namespace SanmapGen

int main() {
    SanmapGen::MapFormatTest::RunRoundTripTests();
    SanmapGen::MapFormatTest::CheckUnrecognizedSkyboxIntensityModeFallsBackSafely();
    SanmapGen::MapFormatTest::RunValidationTests();
    SanmapGen::MapFormatTest::RunBakedFieldTests();
    if (SanmapGen::MapFormatTest::FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", SanmapGen::MapFormatTest::FailureCount());
    return 1;
}

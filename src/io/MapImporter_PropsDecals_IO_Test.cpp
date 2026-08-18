// MapImporter_PropsDecals_IO_Test.cpp — acceptance test for STEP4_PropsDecals_IO's pure JSON
// round-trip: `BuildPropsJson`/`BuildDecalsJson`/`BuildPropGroupsJson`/`BuildDecalGroupsJson` and
// their reader inverses (`ReadPropsJson`/`ReadDecalsJson`/`ReadPropGroupsJson`/`ReadDecalGroupsJson`).
//
// Deliberately does NOT go through `BuildSanmapJsonText`/`ParseSanmapJsonText` — this stays deep/
// edge-case coverage of the pure builders/readers themselves (multiple transforms, an out-of-range
// layerIndex clamp per family) even now that STEP5_PropsDecalsValidation_UI has live-wired that
// pair; `MapImporter_IO_Test.cpp`'s `CheckPropsAndDecals` is the LIVE-document coverage, exercising
// `BuildSanmapJsonText`/`ParseSanmapJsonText` for props/decals for the first time. Own,
// self-contained test target, built and run independently of `MapImporter_IO_Test.exe`/
// `MapExporter_IO_Test.exe`, which must keep passing UNCHANGED.
#include "MapExporter_Recipe_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>
#include <cstdio>
#include <nlohmann/json.hpp>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

bool NearlyEqual(float left, float right) { return std::fabs(left - right) <= 1.0e-4f; }

// One prop group with two transforms (one in-range layerIndex, one deliberately out of range), one
// prop layer to be in range against, mirroring the shape RunRoundTripTests's fixtures use elsewhere
// in this module (non-zero positionZ to exercise the flip, non-identity rotation, non-unit scale).
Params::MapRecipe BuildFixtureRecipe() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;

    Params::PropInstanceLayer propLayer;
    propLayer.name = "Foreground Props";
    propLayer.color[0] = 0.1f; propLayer.color[1] = 0.2f;
    propLayer.color[2] = 0.3f; propLayer.color[3] = 0.4f;
    propLayer.iconScale = 1.5f;
    recipe.propLayers.push_back(propLayer);   // index 0 — the only valid propLayers index

    Params::PropTransform inRangeProp;
    inRangeProp.transform.positionX = 5.0f;
    inRangeProp.transform.positionY = 1.0f;
    inRangeProp.transform.positionZ = 17.0f;                      // non-zero: exercises the flip
    inRangeProp.transform.rotationX = 0.1f; inRangeProp.transform.rotationY = 0.2f;
    inRangeProp.transform.rotationZ = 0.3f; inRangeProp.transform.rotationW = 0.9f;   // non-identity
    inRangeProp.transform.scaleX = 2.0f; inRangeProp.transform.scaleY = 3.0f;
    inRangeProp.transform.scaleZ = 4.0f;                          // non-unit
    inRangeProp.layerIndex = 0;                                   // in range

    Params::PropTransform outOfRangeProp;
    outOfRangeProp.transform.positionZ = 3.0f;
    outOfRangeProp.layerIndex = 99;                               // out of range: must clamp to 0

    Params::PropInstanceGroup propGroup;
    propGroup.blueprintPath = "Props/Rock/Rock01.santp";
    propGroup.transforms.push_back(inRangeProp);
    propGroup.transforms.push_back(outOfRangeProp);
    recipe.props.push_back(propGroup);

    Params::DecalInstanceLayer decalLayer;
    decalLayer.name = "Ground Decals";
    decalLayer.color[0] = 0.5f; decalLayer.color[1] = 0.6f;
    decalLayer.color[2] = 0.7f; decalLayer.color[3] = 0.8f;
    decalLayer.iconScale = 0.75f;
    recipe.decalLayers.push_back(decalLayer);   // index 0 — the only valid decalLayers index

    Params::DecalTransform inRangeDecal;
    inRangeDecal.transform.positionX = 8.0f;
    inRangeDecal.transform.positionY = 2.0f;
    inRangeDecal.transform.positionZ = 23.0f;                     // non-zero: exercises the flip
    inRangeDecal.transform.rotationX = 0.05f; inRangeDecal.transform.rotationY = 0.15f;
    inRangeDecal.transform.rotationZ = 0.25f; inRangeDecal.transform.rotationW = 0.95f;  // non-identity
    inRangeDecal.transform.scaleX = 1.5f; inRangeDecal.transform.scaleY = 1.25f;
    inRangeDecal.transform.scaleZ = 1.75f;                        // non-unit
    inRangeDecal.layerIndex = 0;                                  // in range

    Params::DecalTransform outOfRangeDecal;
    outOfRangeDecal.transform.positionZ = 9.0f;
    outOfRangeDecal.layerIndex = -1;                              // out of range: must clamp to 0

    Params::DecalInstanceGroup decalGroup;
    decalGroup.blueprintPath = "Decals/Blood/Blood01.santp";
    decalGroup.transforms.push_back(inRangeDecal);
    decalGroup.transforms.push_back(outOfRangeDecal);
    recipe.decals.push_back(decalGroup);

    return recipe;
}

void RunPropsDecalsRoundTripTests() {
    const Params::MapRecipe original = BuildFixtureRecipe();

    // Pure builders — no disk, no live document.
    nlohmann::ordered_json testDocument;
    testDocument["PropGroups"]  = Io::BuildPropGroupsJson(original);
    testDocument["props"]       = Io::BuildPropsJson(original);
    testDocument["DecalGroups"] = Io::BuildDecalGroupsJson(original);
    testDocument["decals"]      = Io::BuildDecalsJson(original);

    Check(testDocument["props"].is_array() && testDocument["props"].size() == 1,
          "props is a plain array, not a dictionary");
    Check(testDocument["PropGroups"].is_array() && testDocument["PropGroups"].size() == 1,
          "PropGroups is a plain array");

    // Pure readers — ReadPropGroupsJson/ReadDecalGroupsJson MUST run before ReadPropsJson/
    // ReadDecalsJson (the layerIndex clamp validates against the *Layers vectors they populate).
    Params::MapRecipe loaded;
    loaded.geometry.mapSize = original.geometry.mapSize;   // already populated from `width`, same
                                                             // contract as the live importer.
    Io::MapImportResult result;
    Io::ReadPropGroupsJson(testDocument, loaded);
    Io::ReadPropsJson(testDocument, loaded, result);
    Io::ReadDecalGroupsJson(testDocument, loaded);
    Io::ReadDecalsJson(testDocument, loaded, result);

    // --- PropInstanceLayer / PropGroups -----------------------------------------------------
    Check(loaded.propLayers.size() == 1, "one prop layer survives");
    if (!loaded.propLayers.empty()) {
        const Params::PropInstanceLayer& originalLayer = original.propLayers[0];
        const Params::PropInstanceLayer& loadedLayer = loaded.propLayers[0];
        Check(loadedLayer.name == originalLayer.name, "PropInstanceLayer::name survives");
        Check(NearlyEqual(loadedLayer.color[0], originalLayer.color[0])
              && NearlyEqual(loadedLayer.color[1], originalLayer.color[1])
              && NearlyEqual(loadedLayer.color[2], originalLayer.color[2])
              && NearlyEqual(loadedLayer.color[3], originalLayer.color[3]),
              "PropInstanceLayer::color survives all four components");
        Check(NearlyEqual(loadedLayer.iconScale, originalLayer.iconScale),
              "PropInstanceLayer::iconScale survives");
    }

    // --- PropInstanceGroup / props -----------------------------------------------------------
    Check(loaded.props.size() == 1, "one prop group survives");
    if (!loaded.props.empty()) {
        const Params::PropInstanceGroup& originalGroup = original.props[0];
        const Params::PropInstanceGroup& loadedGroup = loaded.props[0];
        Check(loadedGroup.blueprintPath == originalGroup.blueprintPath,
              "PropInstanceGroup::blueprintPath survives");
        Check(loadedGroup.transforms.size() == 2, "both prop transforms survive");
        if (loadedGroup.transforms.size() == 2) {
            const Params::PropTransform& originalInRange = originalGroup.transforms[0];
            const Params::PropTransform& loadedInRange = loadedGroup.transforms[0];
            Check(NearlyEqual(loadedInRange.transform.positionX, originalInRange.transform.positionX)
                  && NearlyEqual(loadedInRange.transform.positionY, originalInRange.transform.positionY),
                  "prop positionX/Y survive untouched by the flip");
            // Flip (export) then inverse (import) compose to the identity — this is what actually
            // exercises the coordinate flip, without the test needing to know the map-size constant.
            Check(NearlyEqual(loadedInRange.transform.positionZ, originalInRange.transform.positionZ),
                  "prop positionZ round-trips through the flip back to its original value");
            Check(NearlyEqual(loadedInRange.transform.rotationX, originalInRange.transform.rotationX)
                  && NearlyEqual(loadedInRange.transform.rotationY, originalInRange.transform.rotationY)
                  && NearlyEqual(loadedInRange.transform.rotationZ, originalInRange.transform.rotationZ)
                  && NearlyEqual(loadedInRange.transform.rotationW, originalInRange.transform.rotationW),
                  "the non-identity prop rotation survives verbatim, with no flip applied");
            Check(NearlyEqual(loadedInRange.transform.scaleX, originalInRange.transform.scaleX)
                  && NearlyEqual(loadedInRange.transform.scaleY, originalInRange.transform.scaleY)
                  && NearlyEqual(loadedInRange.transform.scaleZ, originalInRange.transform.scaleZ),
                  "the non-unit prop scale survives");
            Check(loadedInRange.layerIndex == 0, "an in-range prop layerIndex survives exactly");

            const Params::PropTransform& loadedOutOfRange = loadedGroup.transforms[1];
            Check(loadedOutOfRange.layerIndex == 0,
                  "an out-of-range prop layerIndex (99) clamps to 0 on import");
        }
    }
    Check(result.warningCount > 0, "the out-of-range layerIndex clamp is logged as a warning");

    // --- DecalInstanceLayer / DecalGroups ----------------------------------------------------
    Check(loaded.decalLayers.size() == 1, "one decal layer survives");
    if (!loaded.decalLayers.empty()) {
        const Params::DecalInstanceLayer& originalLayer = original.decalLayers[0];
        const Params::DecalInstanceLayer& loadedLayer = loaded.decalLayers[0];
        Check(loadedLayer.name == originalLayer.name, "DecalInstanceLayer::name survives");
        Check(NearlyEqual(loadedLayer.color[0], originalLayer.color[0])
              && NearlyEqual(loadedLayer.color[1], originalLayer.color[1])
              && NearlyEqual(loadedLayer.color[2], originalLayer.color[2])
              && NearlyEqual(loadedLayer.color[3], originalLayer.color[3]),
              "DecalInstanceLayer::color survives all four components");
        Check(NearlyEqual(loadedLayer.iconScale, originalLayer.iconScale),
              "DecalInstanceLayer::iconScale survives");
    }

    // --- DecalInstanceGroup / decals ---------------------------------------------------------
    Check(loaded.decals.size() == 1, "one decal group survives");
    if (!loaded.decals.empty()) {
        const Params::DecalInstanceGroup& originalGroup = original.decals[0];
        const Params::DecalInstanceGroup& loadedGroup = loaded.decals[0];
        Check(loadedGroup.blueprintPath == originalGroup.blueprintPath,
              "DecalInstanceGroup::blueprintPath survives");
        Check(loadedGroup.transforms.size() == 2, "both decal transforms survive");
        if (loadedGroup.transforms.size() == 2) {
            const Params::DecalTransform& originalInRange = originalGroup.transforms[0];
            const Params::DecalTransform& loadedInRange = loadedGroup.transforms[0];
            Check(NearlyEqual(loadedInRange.transform.positionX, originalInRange.transform.positionX)
                  && NearlyEqual(loadedInRange.transform.positionY, originalInRange.transform.positionY),
                  "decal positionX/Y survive untouched by the flip");
            Check(NearlyEqual(loadedInRange.transform.positionZ, originalInRange.transform.positionZ),
                  "decal positionZ round-trips through the flip back to its original value");
            Check(NearlyEqual(loadedInRange.transform.rotationX, originalInRange.transform.rotationX)
                  && NearlyEqual(loadedInRange.transform.rotationY, originalInRange.transform.rotationY)
                  && NearlyEqual(loadedInRange.transform.rotationZ, originalInRange.transform.rotationZ)
                  && NearlyEqual(loadedInRange.transform.rotationW, originalInRange.transform.rotationW),
                  "the non-identity decal rotation survives verbatim, with no flip applied");
            Check(NearlyEqual(loadedInRange.transform.scaleX, originalInRange.transform.scaleX)
                  && NearlyEqual(loadedInRange.transform.scaleY, originalInRange.transform.scaleY)
                  && NearlyEqual(loadedInRange.transform.scaleZ, originalInRange.transform.scaleZ),
                  "the non-unit decal scale survives");
            Check(loadedInRange.layerIndex == 0, "an in-range decal layerIndex survives exactly");

            const Params::DecalTransform& loadedOutOfRange = loadedGroup.transforms[1];
            Check(loadedOutOfRange.layerIndex == 0,
                  "an out-of-range decal layerIndex (-1) clamps to 0 on import");
        }
    }
}

} // namespace

int main() {
    RunPropsDecalsRoundTripTests();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

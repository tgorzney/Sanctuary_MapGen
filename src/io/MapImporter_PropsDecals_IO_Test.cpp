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
#include "../sys/PathStem_SYS.h"
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
    propLayer.layerId = 7;                    // non-default: exercises the "Id" wire key round-trip
    propLayer.bLocked = true;                 // non-default: exercises the "Locked" wire key round-trip
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
    propGroup.bReclaimable = true;
    propGroup.transforms.push_back(inRangeProp);
    propGroup.transforms.push_back(outOfRangeProp);
    recipe.props.push_back(propGroup);

    Params::DecalInstanceLayer decalLayer;
    decalLayer.name = "Ground Decals";
    decalLayer.color[0] = 0.5f; decalLayer.color[1] = 0.6f;
    decalLayer.color[2] = 0.7f; decalLayer.color[3] = 0.8f;
    decalLayer.iconScale = 0.75f;
    decalLayer.layerId = 7;                     // non-default: exercises the "Id" wire key round-trip
    decalLayer.bLocked = true;                  // non-default: exercises the "Locked" wire key round-trip
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
        Check(loadedLayer.layerId == originalLayer.layerId,
              "PropInstanceLayer::layerId survives through the 'Id' wire key");
        Check(loadedLayer.bLocked == originalLayer.bLocked,
              "PropInstanceLayer::bLocked survives");
    }

    // --- PropInstanceGroup / props -----------------------------------------------------------
    Check(loaded.props.size() == 1, "one prop group survives");
    if (!loaded.props.empty()) {
        const Params::PropInstanceGroup& originalGroup = original.props[0];
        const Params::PropInstanceGroup& loadedGroup = loaded.props[0];
        Check(loadedGroup.blueprintPath == originalGroup.blueprintPath,
              "PropInstanceGroup::blueprintPath survives");
        Check(loadedGroup.bReclaimable == originalGroup.bReclaimable,
              "PropInstanceGroup::bReclaimable survives");
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
        Check(loadedLayer.layerId == originalLayer.layerId,
              "DecalInstanceLayer::layerId survives through the 'Id' wire key");
        Check(loadedLayer.bLocked == originalLayer.bLocked,
              "DecalInstanceLayer::bLocked survives");
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

// STEP56 (`ARCH_14_13_OpenItems.md` §14.13 item 3, Work-Order A): an entry with no "Id" key
// legacy-backfills `layerId` from its array position — already unique, safe going forward.
void RunPropDecalGroupsLegacyBackfillTests() {
    nlohmann::ordered_json document;
    document["PropGroups"] = nlohmann::json::array({
        nlohmann::json{ { "Name", "First" } },
        nlohmann::json{ { "Name", "Second" } },
    });
    document["DecalGroups"] = nlohmann::json::array({
        nlohmann::json{ { "Name", "First" } },
        nlohmann::json{ { "Name", "Second" } },
    });

    Params::MapRecipe loaded;
    Io::ReadPropGroupsJson(document, loaded);
    Io::ReadDecalGroupsJson(document, loaded);

    Check(loaded.propLayers.size() == 2, "both legacy PropGroups entries survive");
    if (loaded.propLayers.size() == 2) {
        Check(loaded.propLayers[0].layerId == 0, "the first legacy PropGroups entry backfills layerId 0");
        Check(loaded.propLayers[1].layerId == 1, "the second legacy PropGroups entry backfills layerId 1");
    }

    Check(loaded.decalLayers.size() == 2, "both legacy DecalGroups entries survive");
    if (loaded.decalLayers.size() == 2) {
        Check(loaded.decalLayers[0].layerId == 0, "the first legacy DecalGroups entry backfills layerId 0");
        Check(loaded.decalLayers[1].layerId == 1, "the second legacy DecalGroups entry backfills layerId 1");
    }
}

// STEP108: an entry with no "Locked" key (legacy `.sanmap` files saved before this ticket) keeps the
// struct's own default (`false`) — no clamp, no range validation needed.
void RunPropDecalGroupsLegacyLockedDefaultTests() {
    nlohmann::ordered_json document;
    document["PropGroups"] = nlohmann::json::array({ nlohmann::json{ { "Name", "No Lock Key" } } });
    document["DecalGroups"] = nlohmann::json::array({ nlohmann::json{ { "Name", "No Lock Key" } } });

    Params::MapRecipe loaded;
    Io::ReadPropGroupsJson(document, loaded);
    Io::ReadDecalGroupsJson(document, loaded);

    Check(loaded.propLayers.size() == 1 && !loaded.propLayers[0].bLocked,
          "a PropGroups entry with no 'Locked' key defaults bLocked to false");
    Check(loaded.decalLayers.size() == 1 && !loaded.decalLayers[0].bLocked,
          "a DecalGroups entry with no 'Locked' key defaults bLocked to false");
}

// STEP115: two PropInstanceGroup entries sharing the SAME blueprintPath must synthesize TWO
// separate layers (NOT deduplicated by blueprintPath — pins the "one layer per GROUP entry, never
// collapsed by name" ruling). Repeats the identical shape for DecalInstanceGroup/ReconcileDecalLayers.
void RunPropDecalLayerSynthesisOnEmptyGroupsTests() {
    Params::MapRecipe recipe;   // deliberately no propLayers/decalLayers

    Params::PropInstanceGroup propGroupOne;
    propGroupOne.blueprintPath = "Props/Rock/Rock01.santp";
    propGroupOne.transforms.push_back(Params::PropTransform{});
    recipe.props.push_back(propGroupOne);

    Params::PropInstanceGroup propGroupTwo;
    propGroupTwo.blueprintPath = "Props/Rock/Rock01.santp";   // same blueprintPath as group one
    propGroupTwo.transforms.push_back(Params::PropTransform{});
    propGroupTwo.transforms.push_back(Params::PropTransform{});
    recipe.props.push_back(propGroupTwo);

    Io::MapImportResult result;
    Io::ReconcilePropLayers(recipe, result);

    Check(recipe.propLayers.size() == 2,
          "two PropInstanceGroup entries sharing one blueprintPath synthesize two separate layers");
    if (recipe.propLayers.size() == 2) {
        Check(recipe.propLayers[0].name == "Rock01" && recipe.propLayers[1].name == "Rock01",
              "both synthesized prop layers are named from the shared blueprintPath's stem");
        Check(recipe.propLayers[0].layerId == 0 && recipe.propLayers[1].layerId == 1,
              "synthesized prop layerId is sequential");
    }
    if (recipe.props.size() == 2) {
        Check(recipe.props[0].transforms.size() == 1 && recipe.props[0].transforms[0].layerIndex == 0,
              "the first prop group's transform points at layer 0");
        Check(recipe.props[1].transforms.size() == 2
              && recipe.props[1].transforms[0].layerIndex == 1
              && recipe.props[1].transforms[1].layerIndex == 1,
              "the second prop group's transforms point at layer 1");
    }
    Check(result.warningCount == 1, "one aggregate warning fires for the whole prop synthesis");

    Params::MapRecipe decalRecipe;   // identical shape, DecalInstanceGroup/ReconcileDecalLayers
    Params::DecalInstanceGroup decalGroupOne;
    decalGroupOne.blueprintPath = "Decals/Blood/Blood01.santp";
    decalGroupOne.transforms.push_back(Params::DecalTransform{});
    decalRecipe.decals.push_back(decalGroupOne);

    Params::DecalInstanceGroup decalGroupTwo;
    decalGroupTwo.blueprintPath = "Decals/Blood/Blood01.santp";
    decalGroupTwo.transforms.push_back(Params::DecalTransform{});
    decalGroupTwo.transforms.push_back(Params::DecalTransform{});
    decalRecipe.decals.push_back(decalGroupTwo);

    Io::MapImportResult decalResult;
    Io::ReconcileDecalLayers(decalRecipe, decalResult);

    Check(decalRecipe.decalLayers.size() == 2,
          "two DecalInstanceGroup entries sharing one blueprintPath synthesize two separate layers");
    if (decalRecipe.decalLayers.size() == 2) {
        Check(decalRecipe.decalLayers[0].name == "Blood01" && decalRecipe.decalLayers[1].name == "Blood01",
              "both synthesized decal layers are named from the shared blueprintPath's stem");
        Check(decalRecipe.decalLayers[0].layerId == 0 && decalRecipe.decalLayers[1].layerId == 1,
              "synthesized decal layerId is sequential");
    }
    if (decalRecipe.decals.size() == 2) {
        Check(decalRecipe.decals[0].transforms.size() == 1
              && decalRecipe.decals[0].transforms[0].layerIndex == 0,
              "the first decal group's transform points at layer 0");
        Check(decalRecipe.decals[1].transforms.size() == 2
              && decalRecipe.decals[1].transforms[0].layerIndex == 1
              && decalRecipe.decals[1].transforms[1].layerIndex == 1,
              "the second decal group's transforms point at layer 1");
    }
    Check(decalResult.warningCount == 1, "one aggregate warning fires for the whole decal synthesis");

    // Sys::FileStemFromPath edge cases — a path with no directory separator, and a path with no
    // extension — neither has a dedicated test for the existing UI-layer twin algorithm (repo-wide
    // grep), worth covering once, here.
    Check(Sys::FileStemFromPath("Rock01.santp") == "Rock01",
          "FileStemFromPath with no directory separator still strips the extension");
    Check(Sys::FileStemFromPath("Props/Rock/Rock01") == "Rock01",
          "FileStemFromPath with no extension still strips the directory");
}

// STEP115: reuses BuildFixtureRecipe's existing fixture (already populates one PropInstanceLayer and
// one DecalInstanceLayer) — ReconcilePropLayers/ReconcileDecalLayers must leave both at size 1.
void RunPropDecalLayerSynthesisIsNoOpWhenGroupsPresentTest() {
    Params::MapRecipe recipe = BuildFixtureRecipe();
    const int propLayerCountBefore = static_cast<int>(recipe.propLayers.size());
    const int decalLayerCountBefore = static_cast<int>(recipe.decalLayers.size());

    Io::MapImportResult result;
    Io::ReconcilePropLayers(recipe, result);
    Io::ReconcileDecalLayers(recipe, result);

    Check(static_cast<int>(recipe.propLayers.size()) == propLayerCountBefore && propLayerCountBefore == 1,
          "ReconcilePropLayers is a no-op when propLayers was already populated");
    Check(static_cast<int>(recipe.decalLayers.size()) == decalLayerCountBefore && decalLayerCountBefore == 1,
          "ReconcileDecalLayers is a no-op when decalLayers was already populated");
    Check(result.warningCount == 0, "no synthesis warning fires when both *Groups were already present");
}

} // namespace

int main() {
    RunPropsDecalsRoundTripTests();
    RunPropDecalGroupsLegacyBackfillTests();
    RunPropDecalGroupsLegacyLockedDefaultTests();
    RunPropDecalLayerSynthesisOnEmptyGroupsTests();
    RunPropDecalLayerSynthesisIsNoOpWhenGroupsPresentTest();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

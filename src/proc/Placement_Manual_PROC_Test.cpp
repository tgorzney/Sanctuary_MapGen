// Placement_Manual_PROC_Test.cpp — acceptance test for ResolveManualPropsAndDecals() and the
// manualLayerId correlation column (STEP57). Sibling to Placement_PROC_Test.cpp / Placement_
// Symmetry_PROC_Test.cpp — one test file per concern, kept out of the existing files so neither
// needs editing (their own "stays green, unedited" acceptance criterion). Build with MSVC from
// src/proc: cl /EHsc /std:c++17 /O2 with this file, every Placement_*_PROC.cpp,
// ..\sys\GpuResource_*_SYS.cpp, ..\sys\GpuGlFunctions_SYS.cpp and opengl32.lib.
#include "Placement_PROC.h"
#include "Placement_Test_Terrain.h"
#include <cstdio>

using namespace SanmapGen;

static int failures = 0;
static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failures; }
}

// A recipe with one manual prop layer/instance and one manual decal layer/instance, PLUS a
// procedural PropRule/DecalRule (so the ruleIndex == -1 assertion is load-bearing per the
// work-order's acceptance test 1). Two layers per domain so layerId != layerIndex array position.
static Params::MapRecipe MakeManualRecipe() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = PlacementTest::mapSize;
    recipe.geometry.seed = 55u;
    recipe.geometry.terrainMaxHeight = 128.0f;
    recipe.globalSymmetryMask = Params::SymmetryAxis::None;

    Params::PropInstanceLayer decoyPropLayer; decoyPropLayer.layerId = 40;
    Params::PropInstanceLayer realPropLayer;  realPropLayer.layerId = 41;
    recipe.propLayers.push_back(decoyPropLayer);
    recipe.propLayers.push_back(realPropLayer);

    Params::DecalInstanceLayer decoyDecalLayer; decoyDecalLayer.layerId = 50;
    Params::DecalInstanceLayer realDecalLayer;  realDecalLayer.layerId = 51;
    recipe.decalLayers.push_back(decoyDecalLayer);
    recipe.decalLayers.push_back(realDecalLayer);

    Params::PropTransform propTransform;
    propTransform.transform.positionX = 12.0f; propTransform.transform.positionY = 3.5f;
    propTransform.transform.positionZ = 40.0f;
    propTransform.transform.rotationX = 0.1f; propTransform.transform.rotationY = 0.2f;
    propTransform.transform.rotationZ = 0.3f; propTransform.transform.rotationW = 0.9f;
    propTransform.transform.scaleX = 2.0f; propTransform.transform.scaleY = 2.5f; propTransform.transform.scaleZ = 3.0f;
    propTransform.layerIndex = 1;   // realPropLayer -> layerId 41
    Params::PropInstanceGroup propGroup;
    propGroup.blueprintPath = "Props/Rock/Rock01.santp";
    propGroup.transforms.push_back(propTransform);
    recipe.props.push_back(propGroup);

    Params::DecalTransform decalTransform;
    decalTransform.transform.positionX = 60.0f; decalTransform.transform.positionY = 1.0f;
    decalTransform.transform.positionZ = 22.0f;
    decalTransform.transform.rotationW = 1.0f;
    decalTransform.transform.scaleX = 4.0f; decalTransform.transform.scaleY = 4.0f; decalTransform.transform.scaleZ = 4.0f;
    decalTransform.layerIndex = 1;   // realDecalLayer -> layerId 51
    Params::DecalInstanceGroup decalGroup;
    decalGroup.blueprintPath = "Decals/Scorch/Scorch01.santp";
    decalGroup.transforms.push_back(decalTransform);
    recipe.decals.push_back(decalGroup);

    // A non-empty procedural rule set, so `ruleIndex == 0` (the struct default) would silently
    // collide with a genuine procedural rule 0 if left uncorrected (STEP83 §7).
    Params::PropRule propRule;
    propRule.transform = PlacementTest::MakeTransform("edbm014", 1.0f, 1.0f);
    recipe.propRules.push_back(propRule);
    Params::DecalRule decalRule;
    decalRule.transform = PlacementTest::MakeTransform("dcl001", 1.0f, 1.0f);
    recipe.decalRules.push_back(decalRule);
    return recipe;
}

int main() {
    Data::MapFields fields;
    PlacementTest::BuildTestFields(fields);

    // --- 1. Manual props/decals actually resolve, with correct fields.
    Params::MapRecipe recipe = MakeManualRecipe();
    Data::PlacementResults results;
    Proc::PlacementStage stage(recipe, fields, results);
    stage.Run();

    Check(results.props.Count() >= 1, "manual prop resolves into results.props");
    Check(results.decals.Count() >= 1, "manual decal resolves into results.decals");

    bool bFoundManualProp = false, bFoundManualDecal = false;
    for (std::size_t index = 0; index < results.props.Count(); ++index) {
        if (results.props.positionX[index] == 12.0f && results.props.positionZ[index] == 40.0f) {
            bFoundManualProp = true;
            Check(results.props.positionY[index] == 3.5f, "manual prop positionY copied verbatim (no resample)");
            Check(results.props.rotationX[index] == 0.1f && results.props.rotationY[index] == 0.2f
                  && results.props.rotationZ[index] == 0.3f && results.props.rotationW[index] == 0.9f,
                  "manual prop rotation copied verbatim");
            Check(results.props.scaleX[index] == 2.0f && results.props.scaleY[index] == 2.5f
                  && results.props.scaleZ[index] == 3.0f, "manual prop scale copied verbatim");
            Check(results.props.manualLayerId[index] == 41, "manual prop manualLayerId == layer's layerId, not layerIndex");
            Check(results.props.ruleIndex[index] == -1, "manual prop ruleIndex == -1, not the struct default 0");
        }
    }
    for (std::size_t index = 0; index < results.decals.Count(); ++index) {
        if (results.decals.positionX[index] == 60.0f && results.decals.positionZ[index] == 22.0f) {
            bFoundManualDecal = true;
            Check(results.decals.manualLayerId[index] == 51, "manual decal manualLayerId == layer's layerId, not layerIndex");
            Check(results.decals.ruleIndex[index] == -1, "manual decal ruleIndex == -1, not the struct default 0");
        }
    }
    Check(bFoundManualProp, "the authored manual prop instance was found in results.props");
    Check(bFoundManualDecal, "the authored manual decal instance was found in results.decals");

    // --- 2. Sentinel default on procedural instances: every collection defaults manualLayerId == -1.
    bool bAllProceduralDefaultSentinel = true;
    for (std::size_t index = 0; index < results.markers.Count(); ++index)
        if (results.markers.manualLayerId[index] != -1) bAllProceduralDefaultSentinel = false;
    for (std::size_t index = 0; index < results.units.Count(); ++index)
        if (results.units.manualLayerId[index] != -1) bAllProceduralDefaultSentinel = false;
    for (std::size_t index = 0; index < results.props.Count(); ++index)
        if (results.props.positionX[index] != 12.0f && results.props.manualLayerId[index] != -1)
            bAllProceduralDefaultSentinel = false;   // procedural props are everything but the manual one
    Check(bAllProceduralDefaultSentinel, "procedurally-scattered instances default manualLayerId == -1");

    // --- 3. Out-of-range layerIndex resolves to -1, not a crash.
    Params::MapRecipe outOfRangeRecipe = MakeManualRecipe();
    outOfRangeRecipe.propLayers.clear();          // now layerIndex 1 is out of range (empty array)
    outOfRangeRecipe.decals.clear();              // isolate the prop-side check
    Data::PlacementResults outOfRangeResults;
    Proc::PlacementStage outOfRangeStage(outOfRangeRecipe, fields, outOfRangeResults);
    outOfRangeStage.Run();
    bool bFoundOutOfRangeProp = false;
    for (std::size_t index = 0; index < outOfRangeResults.props.Count(); ++index) {
        if (outOfRangeResults.props.positionX[index] == 12.0f) {
            bFoundOutOfRangeProp = true;
            Check(outOfRangeResults.props.manualLayerId[index] == -1,
                  "out-of-range layerIndex degrades manualLayerId to -1, not a crash/garbage read");
        }
    }
    Check(bFoundOutOfRangeProp, "instance still resolves (count unaffected) despite out-of-range layerIndex");

    // --- 4. No symmetry participation even under a non-None global mask.
    Params::MapRecipe symmetricRecipe = MakeManualRecipe();
    symmetricRecipe.globalSymmetryMask = Params::SymmetryAxis::MirrorAcrossX;
    Data::PlacementResults symmetricResults;
    Proc::PlacementStage symmetricStage(symmetricRecipe, fields, symmetricResults);
    symmetricStage.Run();
    int manualPropCount = 0;
    for (std::size_t index = 0; index < symmetricResults.props.Count(); ++index)
        if (symmetricResults.props.positionX[index] == 12.0f && symmetricResults.props.positionZ[index] == 40.0f)
            ++manualPropCount;
    Check(manualPropCount == 1, "manual prop produces exactly one instance under a non-None globalSymmetryMask");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}

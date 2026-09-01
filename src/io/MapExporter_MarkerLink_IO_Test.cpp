// MapExporter_MarkerLink_IO_Test.cpp — acceptance test for STEP237: BuildMarkerLinksJson's own
// wire shape (ARCH §19.30), plus the two merged `LinkIdentifier` fields BuildMarkerLayerBundlesJson/
// BuildMarkerGroupsJson now write (MapExporter_Markers_IO.cpp). Modelled on
// MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp's own standalone Check()/main() shape — pure,
// disk-free JSON assertions, no scratch folder needed.
#include "MapExporter_MarkerLink_IO.h"
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// 1. BuildMarkerLinksJson's own wire shape — Identifier/Name/ColorOverrideEnabled/Color({r,g,b,a}).
static void TestBuildMarkerLinksJsonShape() {
    Params::MapRecipe recipe;
    Params::MarkerLink link;
    link.identifier = 7;
    link.name = "My Link";
    link.bColorOverrideEnabled = true;
    link.color[0] = 0.25f; link.color[1] = 0.5f; link.color[2] = 0.75f; link.color[3] = 1.0f;
    recipe.markerLinks.push_back(link);

    const nlohmann::ordered_json json = Io::BuildMarkerLinksJson(recipe);
    Check(json.is_array() && json.size() == 1, "one MarkerLinks entry written");
    const nlohmann::ordered_json& linkJson = json[0];
    Check(linkJson["Identifier"].get<int>() == 7, "Identifier spelled in full, not Id");
    Check(linkJson["Name"].get<std::string>() == "My Link", "Name survives");
    Check(linkJson["ColorOverrideEnabled"].get<bool>() == true, "ColorOverrideEnabled survives");
    Check(linkJson["Color"].is_object(), "Color is an object, not a bare array (ARCH §19.30)");
    Check(linkJson["Color"]["r"].get<float>() == 0.25f && linkJson["Color"]["g"].get<float>() == 0.5f
          && linkJson["Color"]["b"].get<float>() == 0.75f && linkJson["Color"]["a"].get<float>() == 1.0f,
          "Color components survive as {r,g,b,a}");
}

// 1b. STEP243: the 7 fields STEP241/242 added, all at non-default values, mirroring MarkerGroups[]'s
// own wire spelling (Hidden/IconScale/GridSnapEnabled/GridSnapSizeWorldUnits/SymmetryEnabled/
// SymmetryUseGlobal/SymmetryMask/RadialSymmetryRepeatCount/Locked — Symmetry flattens, not nested).
static void TestBuildMarkerLinksJsonNewFieldsShape() {
    Params::MapRecipe recipe;
    Params::MarkerLink link;
    link.bHidden = true;                       // default false
    link.iconScale = 2.5f;                     // default 1.0f
    link.bGridSnapEnabled = true;               // default false
    link.gridSnapSizeWorldUnits = 4.0f;         // default 1.0f
    link.bSymmetryEnabled = false;              // default true
    link.symmetry.bSymmetryUseGlobal = false;   // default true
    link.symmetry.symmetryMask = 5;             // default 0
    link.symmetry.radialSymmetryRepeatCount = 6;// default 3
    link.bLocked = true;                        // default false
    recipe.markerLinks.push_back(link);

    const nlohmann::ordered_json json = Io::BuildMarkerLinksJson(recipe);
    const nlohmann::ordered_json& linkJson = json[0];
    Check(linkJson["Hidden"].get<bool>() == true, "Hidden survives");
    Check(linkJson["IconScale"].get<float>() == 2.5f, "IconScale survives");
    Check(linkJson["GridSnapEnabled"].get<bool>() == true, "GridSnapEnabled survives");
    Check(linkJson["GridSnapSizeWorldUnits"].get<float>() == 4.0f, "GridSnapSizeWorldUnits survives");
    Check(linkJson["SymmetryEnabled"].get<bool>() == false, "SymmetryEnabled survives");
    Check(linkJson["SymmetryUseGlobal"].get<bool>() == false,
          "symmetry.bSymmetryUseGlobal flattens to sibling SymmetryUseGlobal key");
    Check(linkJson["SymmetryMask"].get<int>() == 5,
          "symmetry.symmetryMask flattens to sibling SymmetryMask key");
    Check(linkJson["RadialSymmetryRepeatCount"].get<int>() == 6,
          "symmetry.radialSymmetryRepeatCount flattens to sibling RadialSymmetryRepeatCount key");
    Check(!linkJson.contains("Symmetry"), "symmetry is flattened, never wrapped as a Symmetry object");
    Check(linkJson["Locked"].get<bool>() == true, "Locked survives");
}

// 2. Empty markerLinks -> an empty array, never absent/null.
static void TestBuildMarkerLinksJsonEmpty() {
    Params::MapRecipe recipe;
    const nlohmann::ordered_json json = Io::BuildMarkerLinksJson(recipe);
    Check(json.is_array() && json.empty(), "no MarkerLink entries -> an empty JSON array");
}

// 3. The merged LinkIdentifier field on MarkerLayerBundles[i] (BuildMarkerLayerBundlesJson).
static void TestBundleLinkIdentifierMerged() {
    Params::MapRecipe recipe;
    Params::MarkerLayerBundle bundle;
    bundle.identifier = 3;
    bundle.linkIdentifier = 7;
    recipe.markerLayerBundles.push_back(bundle);

    const nlohmann::ordered_json json = Io::BuildMarkerLayerBundlesJson(recipe);
    Check(json[0]["LinkIdentifier"].get<int>() == 7,
          "MarkerLayerBundles[0].LinkIdentifier merges alongside AssemblyIdentifier");
}

// 4. The merged LinkIdentifier field on MarkerGroups[i] (BuildMarkerGroupsJson).
static void TestGroupLinkIdentifierMerged() {
    Params::MapRecipe recipe;
    Params::MarkerInstanceLayer layer;
    layer.linkIdentifier = 9;
    recipe.markerLayers.push_back(layer);

    const nlohmann::ordered_json json = Io::BuildMarkerGroupsJson(recipe);
    Check(json[0]["LinkIdentifier"].get<int>() == 9,
          "MarkerGroups[0].LinkIdentifier merges alongside ParentBundleIdentifier/MarkerTypeName");
}

// 5. -1 (not Link-bound) writes verbatim, same unconditional-write posture as every other sentinel
// scalar in this family (AssemblyIdentifier/ParentBundleIdentifier).
static void TestUnboundLinkIdentifierWritesSentinel() {
    Params::MapRecipe recipe;
    recipe.markerLayerBundles.push_back(Params::MarkerLayerBundle());
    recipe.markerLayers.push_back(Params::MarkerInstanceLayer());

    const nlohmann::ordered_json bundleJson = Io::BuildMarkerLayerBundlesJson(recipe);
    const nlohmann::ordered_json groupJson  = Io::BuildMarkerGroupsJson(recipe);
    Check(bundleJson[0]["LinkIdentifier"].get<int>() == -1, "default MarkerLayerBundle writes -1");
    Check(groupJson[0]["LinkIdentifier"].get<int>() == -1, "default MarkerInstanceLayer writes -1");
}

int main() {
    TestBuildMarkerLinksJsonShape();
    TestBuildMarkerLinksJsonNewFieldsShape();
    TestBuildMarkerLinksJsonEmpty();
    TestBundleLinkIdentifierMerged();
    TestGroupLinkIdentifierMerged();
    TestUnboundLinkIdentifierWritesSentinel();

    if (failureCount == 0) { std::printf("All MapExporter_MarkerLink_IO_Test checks passed.\n"); return 0; }
    std::printf("%d MapExporter_MarkerLink_IO_Test check(s) failed.\n", failureCount);
    return 1;
}

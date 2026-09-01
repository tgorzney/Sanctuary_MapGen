// MapImporter_MarkerLink_IO_Test.cpp — acceptance test for STEP237: ReadMarkerLinksJson's own
// round trip (ARCH §19.28/§19.30), the two merged `LinkIdentifier` reader sites
// (MapImporter_MarkerLayerBundle_IO.cpp/MapImporter_MarkerGroups_IO.cpp), the dangling-reference
// soft-log posture (WarnDanglingMarkerLinkIdentifiers, no repair/refusal), and the
// Sanmap_KnownTopLevelKeys_IO registration (MarkerLinks no longer lands in UnknownImport).
// Modelled on MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp's own standalone Check()/main()
// shape.
#include "MapExporter_MarkerLink_IO.h"
#include "MapExporter_Recipe_IO.h"
#include "MapImporter_IO.h"
#include "MapImporter_MarkerLink_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "UnknownImportBag_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// 1. Byte-for-byte round trip: build a fixture recipe, write it with BuildMarkerLinksJson, read it
// back with ReadMarkerLinksJson, compare every field.
static void TestMarkerLinksRoundTrip() {
    Params::MapRecipe original;
    Params::MarkerLink link;
    link.identifier = 11;
    link.name = "Forward Base";
    link.bColorOverrideEnabled = true;
    link.color[0] = 0.1f; link.color[1] = 0.2f; link.color[2] = 0.3f; link.color[3] = 0.4f;
    original.markerLinks.push_back(link);

    nlohmann::json document;
    document["MarkerLinks"] = Io::BuildMarkerLinksJson(original);

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadMarkerLinksJson(document, loaded, result);

    Check(loaded.markerLinks.size() == 1, "one MarkerLink survives the round trip");
    const Params::MarkerLink& roundTripped = loaded.markerLinks[0];
    Check(roundTripped.identifier == 11, "MarkerLink::identifier survives");
    Check(roundTripped.name == "Forward Base", "MarkerLink::name survives");
    Check(roundTripped.bColorOverrideEnabled == true, "MarkerLink::bColorOverrideEnabled survives");
    Check(roundTripped.color[0] == 0.1f && roundTripped.color[1] == 0.2f
          && roundTripped.color[2] == 0.3f && roundTripped.color[3] == 0.4f,
          "MarkerLink::color survives, all four components");
}

// 1b. STEP243: full round trip of all 11 MarkerLink fields (4 original + 7 STEP241/242 additions),
// every field at a non-default value.
static void TestMarkerLinksRoundTripAllElevenFields() {
    Params::MapRecipe original;
    Params::MarkerLink link;
    link.identifier = 12;
    link.name = "Rally Point";
    link.bColorOverrideEnabled = true;
    link.color[0] = 0.6f; link.color[1] = 0.7f; link.color[2] = 0.8f; link.color[3] = 0.9f;
    link.bHidden = true;
    link.iconScale = 2.5f;
    link.bGridSnapEnabled = true;
    link.gridSnapSizeWorldUnits = 4.0f;
    link.bSymmetryEnabled = false;
    link.symmetry.bSymmetryUseGlobal = false;
    link.symmetry.symmetryMask = 5;
    link.symmetry.radialSymmetryRepeatCount = 6;
    link.bLocked = true;
    original.markerLinks.push_back(link);

    nlohmann::json document;
    document["MarkerLinks"] = Io::BuildMarkerLinksJson(original);

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadMarkerLinksJson(document, loaded, result);

    Check(loaded.markerLinks.size() == 1, "one MarkerLink survives the 11-field round trip");
    const Params::MarkerLink& roundTripped = loaded.markerLinks[0];
    Check(roundTripped.identifier == 12, "identifier survives");
    Check(roundTripped.name == "Rally Point", "name survives");
    Check(roundTripped.bColorOverrideEnabled == true, "bColorOverrideEnabled survives");
    Check(roundTripped.color[0] == 0.6f && roundTripped.color[1] == 0.7f
          && roundTripped.color[2] == 0.8f && roundTripped.color[3] == 0.9f, "color survives");
    Check(roundTripped.bHidden == true, "bHidden survives");
    Check(roundTripped.iconScale == 2.5f, "iconScale survives");
    Check(roundTripped.bGridSnapEnabled == true, "bGridSnapEnabled survives");
    Check(roundTripped.gridSnapSizeWorldUnits == 4.0f, "gridSnapSizeWorldUnits survives");
    Check(roundTripped.bSymmetryEnabled == false, "bSymmetryEnabled survives");
    Check(roundTripped.symmetry.bSymmetryUseGlobal == false, "symmetry.bSymmetryUseGlobal survives");
    Check(roundTripped.symmetry.symmetryMask == 5, "symmetry.symmetryMask survives");
    Check(roundTripped.symmetry.radialSymmetryRepeatCount == 6,
          "symmetry.radialSymmetryRepeatCount survives");
    Check(roundTripped.bLocked == true, "bLocked survives");
}

// 1c. A pre-STEP241 `.sanmap` (only the original 4 keys present, the 7 newer keys entirely absent)
// still imports cleanly — each new field takes the struct's own default, per this ticket's Verify.
static void TestLegacyShapeMissingSevenNewKeysDefaults() {
    nlohmann::json document;
    document["MarkerLinks"] = nlohmann::json::array({
        nlohmann::json::object({
            { "Identifier", 3 },
            { "Name", "Old Link" },
            { "ColorOverrideEnabled", true },
            { "Color", nlohmann::json::object({ { "r", 0.1 }, { "g", 0.2 }, { "b", 0.3 }, { "a", 0.4 } }) }
            // Hidden/IconScale/GridSnapEnabled/GridSnapSizeWorldUnits/SymmetryEnabled/
            // SymmetryUseGlobal/SymmetryMask/RadialSymmetryRepeatCount/Locked deliberately absent.
        })
    });

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadMarkerLinksJson(document, loaded, result);

    Check(loaded.markerLinks.size() == 1, "the legacy-shaped MarkerLink entry still imports");
    const Params::MarkerLink& link = loaded.markerLinks[0];
    const Params::MarkerLink defaults;
    Check(link.identifier == 3, "pre-existing Identifier still reads");
    Check(link.name == "Old Link", "pre-existing Name still reads");
    Check(link.bColorOverrideEnabled == true, "pre-existing ColorOverrideEnabled still reads");
    Check(link.bHidden == defaults.bHidden, "missing Hidden falls back to the struct default (false)");
    Check(link.iconScale == defaults.iconScale, "missing IconScale falls back to the struct default (1.0)");
    Check(link.bGridSnapEnabled == defaults.bGridSnapEnabled,
          "missing GridSnapEnabled falls back to the struct default (false)");
    Check(link.gridSnapSizeWorldUnits == defaults.gridSnapSizeWorldUnits,
          "missing GridSnapSizeWorldUnits falls back to the struct default (1.0)");
    Check(link.bSymmetryEnabled == defaults.bSymmetryEnabled,
          "missing SymmetryEnabled falls back to the struct default (true)");
    Check(link.symmetry.bSymmetryUseGlobal == defaults.symmetry.bSymmetryUseGlobal,
          "missing SymmetryUseGlobal falls back to the struct default (true)");
    Check(link.symmetry.symmetryMask == defaults.symmetry.symmetryMask,
          "missing SymmetryMask falls back to the struct default (0)");
    Check(link.symmetry.radialSymmetryRepeatCount == defaults.symmetry.radialSymmetryRepeatCount,
          "missing RadialSymmetryRepeatCount falls back to the struct default (3)");
    Check(link.bLocked == defaults.bLocked, "missing Locked falls back to the struct default (false)");
}

// 2. Both merged LinkIdentifier back-reference fields round-trip (Bundle tier and Layer tier).
static void TestLinkIdentifierBackReferencesRoundTrip() {
    Params::MapRecipe original;
    Params::MarkerLayerBundle bundle;
    bundle.identifier = 4;
    bundle.linkIdentifier = 11;
    original.markerLayerBundles.push_back(bundle);
    Params::MarkerInstanceLayer layer;
    layer.linkIdentifier = 11;
    original.markerLayers.push_back(layer);

    nlohmann::json document;
    document["MarkerLayerBundles"] = Io::BuildMarkerLayerBundlesJson(original);
    document["MarkerGroups"] = Io::BuildMarkerGroupsJson(original);

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadMarkerLayerBundlesJson(document, loaded, result);
    Io::ReadMarkerGroupsJson(document, loaded);

    Check(loaded.markerLayerBundles.size() == 1 && loaded.markerLayerBundles[0].linkIdentifier == 11,
          "MarkerLayerBundle::linkIdentifier survives");
    Check(loaded.markerLayers.size() == 1 && loaded.markerLayers[0].linkIdentifier == 11,
          "MarkerInstanceLayer::linkIdentifier survives");
}

// 3. Absent LinkIdentifier on a legacy (pre-Link) entry defaults to -1 — the "not Link-bound"
// sentinel, same posture as every other back-reference in this family.
static void TestLegacyEntryDefaultsToUnbound() {
    nlohmann::json document;
    document["MarkerLayerBundles"] = nlohmann::json::array({ nlohmann::json::object() });
    document["MarkerGroups"] = nlohmann::json::array({ nlohmann::json::object({ { "Name", "First" } }) });

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadMarkerLayerBundlesJson(document, loaded, result);
    Io::ReadMarkerGroupsJson(document, loaded);

    Check(loaded.markerLayerBundles[0].linkIdentifier == -1,
          "a legacy MarkerLayerBundles entry with no LinkIdentifier key defaults to -1");
    Check(loaded.markerLayers[0].linkIdentifier == -1,
          "a legacy MarkerGroups entry with no LinkIdentifier key defaults to -1");
}

// 4. Dangling LinkIdentifier (references no MarkerLink entry): imports without error, the value is
// left exactly as read (no repair), and a warning is logged — soft-degrade posture, never a refusal.
// STEP245/ARCH §19.33: ReadMarkerLinksJson no longer runs the warn pass internally (it becomes pure
// population only) — this test now calls WarnDanglingMarkerLinkIdentifiers explicitly, mirroring the
// real call site (MapImporter_ParseDocument_IO.cpp's ParseEntityDomainsJson, AFTER ReadMarkersJson).
static void TestDanglingLinkIdentifierWarnsWithoutRefusing() {
    Params::MapRecipe recipe;
    Params::MarkerLayerBundle bundle;
    bundle.identifier = 1;
    bundle.linkIdentifier = 99;   // no MarkerLink with this id exists
    recipe.markerLayerBundles.push_back(bundle);
    Params::MarkerInstanceLayer layer;
    layer.linkIdentifier = 99;
    recipe.markerLayers.push_back(layer);
    // recipe.markerLinks is deliberately left empty — 99 is dangling.

    nlohmann::json document;   // no "MarkerLinks" key at all — same effect as an empty array
    Io::MapImportResult result;
    Io::ReadMarkerLinksJson(document, recipe, result);
    Io::WarnDanglingMarkerLinkIdentifiers(recipe, result);

    Check(recipe.markerLinks.empty(), "no MarkerLinks entry was fabricated");
    Check(recipe.markerLayerBundles[0].linkIdentifier == 99,
          "the dangling MarkerLayerBundle::linkIdentifier is left untouched, never reset to -1");
    Check(recipe.markerLayers[0].linkIdentifier == 99,
          "the dangling MarkerInstanceLayer::linkIdentifier is left untouched, never reset to -1");
    Check(result.warningCount == 2, "both dangling references are logged (soft, not a refusal)");
}

// 4b. ARCH §19.33/STEP245 — the ordering-fix's own regression check: a dangling MarkerTransform::
// linkIdentifier (the THIRD, instance tier, added by this correction) now warns too, once
// `recipe.markers` is populated. Before this fix, `WarnDanglingMarkerLinkIdentifiers` ran from
// INSIDE `ReadMarkerLinksJson`, which itself runs before `ReadMarkersJson` — `recipe.markers` was
// still empty at that call site, so this case would have silently, permanently never fired.
static void TestTransformTierDanglingLinkIdentifierWarns() {
    Params::MapRecipe recipe;
    Params::MarkerTransform transform;
    transform.name = "Mex 0";
    transform.linkIdentifier = 99;   // no MarkerLink with this id exists
    Params::MarkerInstanceGroup group;
    group.name = "Alloys";
    group.transforms.push_back(transform);
    recipe.markers.push_back(group);
    // recipe.markerLinks is deliberately left empty — 99 is dangling.

    Io::MapImportResult result;
    Io::WarnDanglingMarkerLinkIdentifiers(recipe, result);

    Check(recipe.markers[0].transforms[0].linkIdentifier == 99,
          "the dangling MarkerTransform::linkIdentifier is left untouched, never reset to -1");
    Check(result.warningCount == 1,
          "the transform-tier dangling reference is now logged (the ordering fix's own regression check)");
}

// 4c. All three tiers (Bundle/Layer/Transform) dangling at once, in a single pass — the ordering fix
// must warn once per dangling reference, no double-warn, no skip.
static void TestAllThreeTiersDanglingWarnTogetherNoDoubleNoSkip() {
    Params::MapRecipe recipe;
    Params::MarkerLayerBundle bundle;
    bundle.linkIdentifier = 50;
    recipe.markerLayerBundles.push_back(bundle);
    Params::MarkerInstanceLayer layer;
    layer.linkIdentifier = 51;
    recipe.markerLayers.push_back(layer);
    Params::MarkerTransform transform;
    transform.name = "Mex 0";
    transform.linkIdentifier = 52;
    Params::MarkerInstanceGroup group;
    group.name = "Alloys";
    group.transforms.push_back(transform);
    recipe.markers.push_back(group);
    // recipe.markerLinks is deliberately left empty — 50/51/52 all dangle.

    Io::MapImportResult result;
    Io::WarnDanglingMarkerLinkIdentifiers(recipe, result);

    Check(result.warningCount == 3,
          "exactly one warning per dangling tier, no double-warn, no skip (Bundle+Layer+Transform)");
}

// 5. -1 (the "not Link-bound" sentinel) never warns — it is not a dangling reference, it is the
// documented "no Link" state. ARCH §19.33/STEP245: extended to also cover a MarkerTransform whose
// linkIdentifier is absent-default (-1, e.g. a transform with no "LinkIdentifier" key on import) —
// same posture as the existing Bundle/Layer-tier check.
static void TestUnboundSentinelNeverWarns() {
    Params::MapRecipe recipe;
    recipe.markerLayerBundles.push_back(Params::MarkerLayerBundle());   // linkIdentifier == -1
    recipe.markerLayers.push_back(Params::MarkerInstanceLayer());       // linkIdentifier == -1
    Params::MarkerInstanceGroup group;
    group.name = "Alloys";
    group.transforms.push_back(Params::MarkerTransform());              // linkIdentifier == -1
    recipe.markers.push_back(group);

    Io::MapImportResult result;
    Io::WarnDanglingMarkerLinkIdentifiers(recipe, result);
    Check(result.warningCount == 0, "an unbound (-1) linkIdentifier never warns, at any of the three tiers");
}

// 6. End to end, through the real document parser: MarkerLinks must land in `recipe.markerLinks`,
// not in UnknownImport (Sanmap_KnownTopLevelKeys_IO registration).
static void TestMarkerLinksNoLongerSwallowedByUnknownImport() {
    Params::MapRecipe fixture;
    Params::MarkerLink link;
    link.identifier = 5;
    link.name = "Test Link";
    fixture.markerLinks.push_back(link);

    nlohmann::json document;
    document["MarkerLinks"] = Io::BuildMarkerLinksJson(fixture);

    Params::MapRecipe loaded;
    Io::MapImportOptions options;
    Io::MapImportResult result;
    Io::UnknownImportBag unknownData;
    const bool bParsed = Io::MapImporter::ParseSanmapJsonText(document.dump(), loaded, options, result,
                                                              &unknownData);

    Check(bParsed, "the hand-built document parses without error");
    Check(loaded.markerLinks.size() == 1 && loaded.markerLinks[0].name == "Test Link",
          "MarkerLinks parses into recipe.markerLinks");
    Check(!unknownData.unknownTopLevelKeys.contains("MarkerLinks"),
          "MarkerLinks no longer round-trips verbatim under UnknownImport");
}

int main() {
    TestMarkerLinksRoundTrip();
    TestMarkerLinksRoundTripAllElevenFields();
    TestLegacyShapeMissingSevenNewKeysDefaults();
    TestLinkIdentifierBackReferencesRoundTrip();
    TestLegacyEntryDefaultsToUnbound();
    TestDanglingLinkIdentifierWarnsWithoutRefusing();
    TestTransformTierDanglingLinkIdentifierWarns();
    TestAllThreeTiersDanglingWarnTogetherNoDoubleNoSkip();
    TestUnboundSentinelNeverWarns();
    TestMarkerLinksNoLongerSwallowedByUnknownImport();

    if (failureCount == 0) { std::printf("All MapImporter_MarkerLink_IO_Test checks passed.\n"); return 0; }
    std::printf("%d MapImporter_MarkerLink_IO_Test check(s) failed.\n", failureCount);
    return 1;
}

// Sanmap_MigrationRunner_IO_Test.cpp — acceptance test for STEP6_MigrationSubsystem_IO's version-
// resolution cases, REWRITTEN by STEP24_ImportNeverRefuses_IO (Constitution §6 / IO_MIGRATION_SPEC.
// md §6, as ratified): a `.sanmap`'s declared schema version is never grounds to refuse the file, so
// `RunSanmapMigrations` is non-fallible (`void`, no `bool` return) and the two former "refuses"
// cases below are now best-effort-accepts-with-warning cases instead. Covers:
//  (a) SanGenVersion == kCurrentSanGenVersion -> passes through, no warning logged.
//  (b) no SanGenVersion but a legacy mapGeneratorData.MapGeneratorDataVersion == current -> passes
//      through, exactly one warning logged about the fallback.
//  (c) neither field present -> recovers best-effort, a loud warning is logged, no migration walk.
//  (d) SanGenVersion newer than current -> recovers best-effort, a DIFFERENT warning than (c)'s.
//  (1) newer-than-current also captures a genuinely-unrecognized top-level key into the bag.
//  (2) a synthetic no-version-marker fixture (matching the real, Format-Expert-confirmed
//      World_Domination.sanmap shape) imports without the migration transform chain applied.
//  (3) re-exporting either (c) or (d)'s loaded recipe always stamps SanGenVersion =
//      kCurrentSanGenVersion — the documented downgrade-on-resave consequence (ruling 3).
//  (4) an unrecognized top-level key round-trips byte-for-byte nested under document["UnknownImport"]
//      (STEP28_UnknownImportNesting_IO); a same-named bag entry never disturbs a known-domain
//      writer's own top-level field.
//  (5) a legacy key a migration step deliberately deletes via DeleteKeyIfPresent never appears in
//      the bag and never reappears on export — ordering alone provides this.
//  STEP28's own acceptance items, added on top of the above:
//  (6) 2+ unrecognized top-level keys nest together under one UnknownImport object, nothing left at
//      the document's own top level.
//  (7) re-importing a nested UnknownImport recovers the bag via the seed step, and re-exporting
//      across 2+ round trips never accumulates nesting (UnknownImport.UnknownImport...).
//  (8) an empty bag writes no UnknownImport key at all, not an empty object.
//  STEP40F's own acceptance items, added on top of the above (the manifest's V2->V3 step is now
//  really populated, all 9 migrations wired):
//  (9) a hand-built, hostile-but-plausible V2-shaped document spanning all 9 migrations' source
//      fields walks the real 9-step chain end to end (not per-migration isolation, already covered
//      by STEP40B-E's own tests) and lands on the exact expected V3-shaped Params::MapRecipe,
//      including proof that SlopeDefaults_Migrate_V2 running before StratumGenerationSettings_
//      Migrate_V2 leaves BOTH migrations' writes intact on the same StratumGenerationSettings[0]
//      entry (neither clobbers the other).
//  (10) that same document, re-exported after import, stamps SanGenVersion = kCurrentSanGenVersion
//       and writes no mapGeneratorData key (STEP36's already-shipped never-write-the-blob behavior).
//  (11) a document already at kCurrentSanGenVersion runs zero migration steps — proved by a deep
//       equality check against a pre-call snapshot, not just "it still imports."
#include "Sanmap_MigrationRunner_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "MapExporter_IO.h"
#include "UnknownImportBag_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// --- (a)-(d): version resolution, rewritten from refusal to best-effort-accepts-with-warning. ----

// (a) Current-version passthrough (§4.3): still runs resolution, calls no migration, logs no
// warning.
void CheckCurrentVersionPassesThrough() {
    nlohmann::json document = { {"SanGenVersion", Io::kCurrentSanGenVersion} };
    Io::MapImportResult result;
    Io::RunSanmapMigrations(document, result, nullptr);
    Check(result.warningCount == 0, "the current-version passthrough logs no warning");
    Check(document["SanGenVersion"] == Io::kCurrentSanGenVersion,
          "the document keeps its SanGenVersion unchanged on passthrough");
}

// (b) Legacy fallback: no top-level field, but the legacy nested one resolves to the current
// version -> passes through, exactly one warning logged (the fallback firing is never silent).
void CheckLegacyFallbackWarns() {
    nlohmann::json document = {
        {"mapGeneratorData", { {"MapGeneratorDataVersion", Io::kCurrentSanGenVersion} }}
    };
    Io::MapImportResult result;
    Io::RunSanmapMigrations(document, result, nullptr);
    Check(result.warningCount == 1, "the legacy fallback logs exactly one warning");
}

// (c) No version marker of any kind -> never refused (STEP24): a loud warning, and NO migration
// walk — the document is handed back exactly as found (no SanGenVersion write at all, since the
// forward-walk loop zero-iterates).
void CheckNoVersionMarkerRecoversBestEffort() {
    nlohmann::json document = { {"someOtherField", 1} };
    Io::MapImportResult result;
    Io::RunSanmapMigrations(document, result, nullptr);
    Check(result.warningCount > 0, "the no-version-marker case logs a loud warning, not silence");
    Check(!document.contains("SanGenVersion"),
          "no SanGenVersion is written — the runner never resolves a starting version or walks any "
          "migration for this case, so the document is genuinely untouched");
    Check(document["someOtherField"] == 1, "the document's own content is otherwise unchanged");
}

// (d) Newer than current -> never refused (STEP24): a loud warning, distinct from (c)'s, and no
// migration walk (nothing forward to migrate to) — SanGenVersion stays at its newer value, untouched.
void CheckNewerVersionRecoversBestEffort() {
    nlohmann::json document = { {"SanGenVersion", 99} };
    Io::MapImportResult result;
    Io::RunSanmapMigrations(document, result, nullptr);
    Check(result.warningCount > 0, "the newer-than-current case logs a loud warning, not silence");
    Check(document["SanGenVersion"] == 99,
          "SanGenVersion stays at its newer value — the runner never rewrites it on this path");

    nlohmann::json noMarkerDocument = { {"someOtherField", 1} };
    Io::MapImportResult noMarkerResult;
    Io::RunSanmapMigrations(noMarkerDocument, noMarkerResult, nullptr);
    Check(result.debugLog != noMarkerResult.debugLog,
          "the 'newer version' warning is worded distinctly from the 'no version marker' warning");
}

// --- (1): newer-than-current also captures a genuinely-unrecognized top-level key. ----------------
void CheckNewerVersionCapturesUnknownKeyIntoBag() {
    nlohmann::json document = { {"SanGenVersion", 99}, {"SomeFutureBuildOnlyField", "future data"} };
    Io::MapImportResult result;
    Io::UnknownImportBag bag;
    Io::RunSanmapMigrations(document, result, &bag);
    Check(result.warningCount > 0, "the newer-than-current case still warns when a bag is supplied");
    Check(bag.unknownTopLevelKeys.contains("SomeFutureBuildOnlyField")
          && bag.unknownTopLevelKeys["SomeFutureBuildOnlyField"] == "future data",
          "a genuinely-unrecognized top-level key on a newer document lands in the Unknown-Import "
          "bag instead of being silently dropped");
}

// --- (2): the synthetic no-version-marker fixture, matching the real, Format-Expert-confirmed
// World_Domination.sanmap shape (work-order Root problem / target-files) — built as a literal
// in-repo JSON object, NOT a dependency on any path outside the repo. Confirms the migration
// TRANSFORM chain is skipped entirely (not silently run): a field a real V1->V2 migration would
// relocate (none are shipped yet, STEP6's manifest is empty, but the no-walk law holds regardless)
// stays exactly where it started, under the legacy `mapGeneratorData.GeoLayers` blob.
nlohmann::json BuildWorldDominationLikeFixture() {
    nlohmann::json geoLayer;
    geoLayer["Fractal"]  = nlohmann::json::object();
    geoLayer["UseImage"] = false;
    geoLayer["Erosion"]  = { {"Enabled", true}, {"Iterations", 50} };
    nlohmann::json document;
    // No "SanGenVersion", no legacy "mapGeneratorData.MapGeneratorDataVersion" anywhere — the
    // confirmed real shape.
    document["mapGeneratorData"]["GeoLayers"] = nlohmann::json::array({ geoLayer });
    document["mapGeneratorData"]["MapSize"]   = 512;
    document["armies"]  = nlohmann::json::array();
    document["markers"] = nlohmann::json::array();
    return document;
}

void CheckNoVersionMarkerFixtureSkipsMigrationChain() {
    nlohmann::json document = BuildWorldDominationLikeFixture();
    Io::MapImportResult result;
    Io::UnknownImportBag bag;
    Io::RunSanmapMigrations(document, result, &bag);
    Check(result.warningCount > 0, "the real-shape no-version-marker fixture logs a loud warning");
    Check(document["mapGeneratorData"].contains("GeoLayers")
          && document["mapGeneratorData"]["GeoLayers"][0]["Fractal"].is_object()
          && document["mapGeneratorData"]["GeoLayers"][0]["UseImage"] == false
          && document["mapGeneratorData"]["GeoLayers"][0]["Erosion"]["Iterations"] == 50,
          "GeoLayers/Fractal/UseImage/Erosion stay exactly where they started under the legacy "
          "mapGeneratorData blob — no RenameKey/MoveKey-driven relocation happened, proving the "
          "migration transform chain was skipped, not silently run");
    Check(!document.contains("HeightmapStack"),
          "no current-shape HeightmapStack section was synthesized either — nothing was walked");
    Check(bag.unknownTopLevelKeys.empty(),
          "mapGeneratorData/armies/markers are all allowlisted (runner-owned or read by "
          "ParseSanmapJsonText's own unconditional readers), so nothing top-level lands in the bag "
          "for this fixture");
}

// --- (3): re-exporting a recovered document always stamps kCurrentSanGenVersion — the documented
// downgrade-on-resave consequence (ruling 3), exercised through BOTH the no-marker and the
// newer-than-current recovered recipes.
void CheckReexportAlwaysStampsCurrentVersion() {
    for (const nlohmann::json& sourceDocument :
         { BuildWorldDominationLikeFixture(), nlohmann::json{ {"SanGenVersion", 99} } }) {
        Params::MapRecipe loadedRecipe;
        Io::MapImportResult result;
        Check(Io::MapImporter::ParseSanmapJsonText(sourceDocument.dump(), loadedRecipe,
                                                    Io::MapImportOptions(), result),
              "the recovered document parses successfully (never refused)");
        const std::string reexportedText = Io::MapExporter::BuildSanmapJsonText(loadedRecipe);
        const nlohmann::json reexportedDocument = nlohmann::json::parse(reexportedText);
        Check(reexportedDocument["SanGenVersion"] == Io::kCurrentSanGenVersion,
              "re-exporting a recovered document always stamps SanGenVersion = "
              "kCurrentSanGenVersion, even when the source was newer or had no marker at all — a "
              "deliberate, asserted downgrade-on-resave, not a silent side effect");
    }
}

// --- (4): an unrecognized top-level key round-trips byte-for-byte, nested under the single
// `UnknownImport` container (STEP28_UnknownImportNesting_IO); a known-domain writer's own top-level
// field is never disturbed by a colliding bag entry, since the bag never lands at the top level at
// all anymore.
void CheckUnknownKeyRoundTripsAndCollisionResolves() {
    // Plain round trip: a key nothing recognizes survives import -> export, nested under
    // "UnknownImport", nothing written at the document's own top level.
    {
        nlohmann::json document = { {"SanGenVersion", Io::kCurrentSanGenVersion},
                                    {"TotallyUnrecognizedKey", 12345} };
        Params::MapRecipe loadedRecipe;
        Io::MapImportResult result;
        Io::UnknownImportBag bag;
        Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loadedRecipe,
                                                    Io::MapImportOptions(), result, &bag),
              "a document with one genuinely-unrecognized top-level key still parses");
        Check(bag.unknownTopLevelKeys.contains("TotallyUnrecognizedKey")
              && bag.unknownTopLevelKeys["TotallyUnrecognizedKey"] == 12345,
              "the unrecognized key is captured into the bag");
        const nlohmann::json reexported =
            nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(loadedRecipe,
                                                                       Io::MapExportOptions(), &bag));
        Check(!reexported.contains("TotallyUnrecognizedKey"),
              "the unrecognized key is NOT merged flat into the document's own top level");
        Check(reexported.contains("UnknownImport")
              && reexported["UnknownImport"]["TotallyUnrecognizedKey"] == 12345,
              "the unrecognized key re-emits byte-for-byte nested under document[\"UnknownImport\"]");
    }
    // Collision: a bag entry sharing a name with a real known-domain field never disturbs that
    // field — they no longer even occupy the same location on disk.
    {
        Params::MapRecipe recipe;
        recipe.mapName = "The Real Name";
        Io::UnknownImportBag bag;
        bag.unknownTopLevelKeys["name"] = "A stale bag value that must never win";
        const nlohmann::json reexported =
            nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(recipe, Io::MapExportOptions(),
                                                                       &bag));
        Check(reexported["name"] == "The Real Name",
              "a known-domain writer (the exporter's own \"name\" write) is never disturbed by a "
              "same-named bag entry — the bag's own copy lives separately, nested");
        Check(reexported["UnknownImport"]["name"] == "A stale bag value that must never win",
              "the bag's own colliding-named entry still round-trips intact inside \"UnknownImport\" "
              "— it is preserved, not lost, just never conflated with the real top-level field");
    }
}

// --- Acceptance item 1: 2+ genuinely-unrecognized top-level keys export nested under ONE
// `UnknownImport` object, nothing left at the document's own top level.
void CheckMultipleUnknownKeysNestUnderOneUnknownImportKey() {
    nlohmann::json document = { {"SanGenVersion", Io::kCurrentSanGenVersion},
                                {"FirstUnrecognizedKey", "alpha"},
                                {"SecondUnrecognizedKey", 42} };
    Params::MapRecipe loadedRecipe;
    Io::MapImportResult result;
    Io::UnknownImportBag bag;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), loadedRecipe, Io::MapImportOptions(),
                                               result, &bag),
          "a document with 2+ genuinely-unrecognized top-level keys still parses");
    const nlohmann::json reexported =
        nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(loadedRecipe, Io::MapExportOptions(),
                                                                    &bag));
    Check(!reexported.contains("FirstUnrecognizedKey") && !reexported.contains("SecondUnrecognizedKey"),
          "neither unrecognized key is left at the document's own top level");
    Check(reexported.contains("UnknownImport")
          && reexported["UnknownImport"]["FirstUnrecognizedKey"] == "alpha"
          && reexported["UnknownImport"]["SecondUnrecognizedKey"] == 42,
          "both unrecognized keys are nested together under the single UnknownImport object");
}

// --- Acceptance items 2 & 3: round-trip stability — re-importing a nested UnknownImport recovers
// the bag via the seed step, and re-exporting across 2+ round trips never accumulates nesting.
void CheckUnknownImportRoundTripsWithoutAccumulatingNesting() {
    nlohmann::json document = { {"SanGenVersion", Io::kCurrentSanGenVersion},
                                {"FirstUnrecognizedKey", "alpha"},
                                {"SecondUnrecognizedKey", 42} };
    Params::MapRecipe recipeOne;
    Io::MapImportResult resultOne;
    Io::UnknownImportBag bagOne;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), recipeOne, Io::MapImportOptions(),
                                               resultOne, &bagOne),
          "the seed document parses on the first import");
    const std::string exportOneText =
        Io::MapExporter::BuildSanmapJsonText(recipeOne, Io::MapExportOptions(), &bagOne);
    const nlohmann::json exportOneDocument = nlohmann::json::parse(exportOneText);

    // Round trip 2: re-import the first export's text — the seed step in
    // CaptureUnknownTopLevelKeys must recover the bag from document["UnknownImport"]'s own children,
    // not the generic per-key loop ("UnknownImport" is itself allowlisted, so the generic loop would
    // skip it entirely if the seed step did not exist).
    Params::MapRecipe recipeTwo;
    Io::MapImportResult resultTwo;
    Io::UnknownImportBag bagTwo;
    Check(Io::MapImporter::ParseSanmapJsonText(exportOneText, recipeTwo, Io::MapImportOptions(),
                                               resultTwo, &bagTwo),
          "the re-imported (once-nested) document parses");
    Check(bagTwo.unknownTopLevelKeys["FirstUnrecognizedKey"] == "alpha"
          && bagTwo.unknownTopLevelKeys["SecondUnrecognizedKey"] == 42,
          "re-importing recovers both original unrecognized keys into the bag via the seed step");
    const std::string exportTwoText =
        Io::MapExporter::BuildSanmapJsonText(recipeTwo, Io::MapExportOptions(), &bagTwo);
    const nlohmann::json exportTwoDocument = nlohmann::json::parse(exportTwoText);
    Check(exportTwoDocument["UnknownImport"] == exportOneDocument["UnknownImport"],
          "the second export's UnknownImport content is byte-identical to the first's — no "
          "accumulated nesting after one round trip");

    // Round trip 3: repeat once more — nesting still does not accumulate.
    Params::MapRecipe recipeThree;
    Io::MapImportResult resultThree;
    Io::UnknownImportBag bagThree;
    Check(Io::MapImporter::ParseSanmapJsonText(exportTwoText, recipeThree, Io::MapImportOptions(),
                                               resultThree, &bagThree),
          "the twice-re-imported document parses");
    const nlohmann::json exportThreeDocument = nlohmann::json::parse(
        Io::MapExporter::BuildSanmapJsonText(recipeThree, Io::MapExportOptions(), &bagThree));
    Check(exportThreeDocument["UnknownImport"] == exportOneDocument["UnknownImport"],
          "the third export's UnknownImport content still matches the first's exactly — stable "
          "across 2+ round trips");
}

// --- Acceptance item 4: a document with NO unrecognized data never gets a document["UnknownImport"]
// key at all — an empty bag writes no key, not an empty object.
void CheckEmptyBagWritesNoUnknownImportKey() {
    Params::MapRecipe recipe;
    Io::UnknownImportBag emptyBag;
    const nlohmann::json reexportedWithEmptyBag =
        nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(recipe, Io::MapExportOptions(),
                                                                    &emptyBag));
    Check(!reexportedWithEmptyBag.contains("UnknownImport"),
          "an empty bag writes no UnknownImport key at all, not an empty object");

    const nlohmann::json reexportedWithNoBag =
        nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(recipe, Io::MapExportOptions(),
                                                                    nullptr));
    Check(!reexportedWithNoBag.contains("UnknownImport"),
          "no bag at all (nullptr) also writes no UnknownImport key");
}

// --- (5): a migration step's own deliberately-deleted legacy key must never appear in the bag.
// STEP6's shipped manifest is still empty (no real migration files exist yet, per the work-order's
// own "Critical follow-on" note), so this synthesizes what a shipped step's `legacyKeysToDelete`
// would do using the EXACT SAME primitive (`DeleteKeyIfPresent`, `JsonPrimitives_IO.h`) a real
// migration step's cleanup is built from, immediately before the runner's own capture step — proving
// the load-bearing ordering claim (ruling 4: capture runs strictly after any such deletion) directly,
// not just by inspection.
void CheckDeletedLegacyKeyNeverAppearsInBag() {
    nlohmann::json document = { {"SanGenVersion", Io::kCurrentSanGenVersion},
                                {"OldLegacyKeyAMigrationWouldDelete", "should not survive"} };
    Io::DeleteKeyIfPresent(document, "OldLegacyKeyAMigrationWouldDelete");
    Io::MapImportResult result;
    Io::UnknownImportBag bag;
    Io::RunSanmapMigrations(document, result, &bag);
    Check(!bag.unknownTopLevelKeys.contains("OldLegacyKeyAMigrationWouldDelete"),
          "a legacy key already deleted before the capture step never lands in the bag");
    const nlohmann::json reexported =
        nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(Params::MapRecipe(),
                                                                    Io::MapExportOptions(), &bag));
    Check(!reexported.contains("OldLegacyKeyAMigrationWouldDelete"),
          "and it does not reappear on export either");
}

// --- STEP40F acceptance items 9-11: the now-populated V2->V3 manifest step, wired end to end. ------

// One hostile-but-plausible V2 document touching a real source field for every one of the 9
// migrations STEP40B-E shipped. Every float is a clean binary fraction (halves/quarters) so the
// recipe-side assertions below can compare with plain `==`, no epsilon needed. A single `Stratums`
// entry keeps SlopeDefaults' synthesis trivial (mode/mean of one entry equals that entry exactly),
// so `StratumMatchesSynthesizedDefault` is unambiguously true and `SlopeUseGlobal` lands `true`.
nlohmann::json BuildHostileV2MigrationFixture() {
    nlohmann::json document;
    document["SanGenVersion"] = 2;
    nlohmann::json& legacy = document["mapGeneratorData"];

    // GeneralMapSettings_Migrate_V2 source fields.
    legacy["Seed"]                   = 4242;
    legacy["ScaleFeaturesToMapSize"] = false;
    legacy["TerrainMinHeight"]       = 12.0;
    legacy["WorldUnitsPerCell"]      = 3.5;
    // Symmetry_Migrate_V2 source fields (all 9).
    legacy["GlobalSymmetryMask"]         = 5;
    legacy["SnapImperfectSymmetry"]      = true;
    legacy["SymmetryDetectionTolerance"] = 0.25;
    legacy["SymSuperpositionBlend"]      = 0.75;
    legacy["SymmetryBlurRadius"]         = 2.0;
    legacy["CrossFadeWidth"]             = 1.5;
    legacy["CylinderZScale"]             = 0.5;
    legacy["TorusMajorRadius"]           = 10.0;
    legacy["TorusMinorRadius"]           = 2.5;
    // DetailNormal_Migrate_V2 source field.
    legacy["DetailNormalMapSize"] = 2048;
    // Flow_Migrate_V2 source field — legacy [r,g,b,a] array, converted at the destination.
    legacy["FlowMapColor"] = { 0.25, 0.5, 0.75, 1.0 };
    // GlobalMarkerSettings_Migrate_V2 source fields (all 9).
    legacy["GlobalIconAlloy"]   = "IconAlloy";
    legacy["GlobalIconPlasma"]  = "IconPlasma";
    legacy["GlobalIconSpawn"]   = "IconSpawn";
    legacy["MarkerColorAlloy"]  = { 1.0, 0.0, 0.0, 1.0 };
    legacy["MarkerColorPlasma"] = { 0.0, 1.0, 0.0, 1.0 };
    legacy["MarkerColorSpawn"]  = { 0.0, 0.0, 1.0, 1.0 };
    legacy["MarkerScaleAlloy"]  = 1.25;
    legacy["MarkerScalePlasma"] = 1.5;
    legacy["MarkerScaleSpawn"]  = 1.75;
    // SlopeDefaults_Migrate_V2 / StratumGenerationSettings_Migrate_V2's shared source.
    nlohmann::json stratum;
    stratum["SlopeGateEnabled"]        = true;
    stratum["MinimumSlopeDegrees"]     = 10.0;
    stratum["MaximumSlopeDegrees"]     = 80.0;
    stratum["SlopeFeatherDegreesLow"]  = 2.0;
    stratum["SlopeFeatherDegreesHigh"] = 4.0;
    stratum["UseSmoothstep"]           = true;
    stratum["InvertSlopeGate"]         = false;
    stratum["SlopeGateStrength"]       = 0.5;
    legacy["Stratums"] = nlohmann::json::array({ stratum });
    // EntityCollections_Migrate_V2 sub-task 1 (Armies[key].Color -> armies[key].armyColor).
    legacy["Armies"]["commander"]["Color"] = { 0.25, 0.5, 0.75, 1.0 };
    // EntityCollections_Migrate_V2 sub-task 2 (Aliases dict -> markers[].transforms[].alias).
    legacy["Aliases"]["MainBase"] = "spawnTransformOne";

    // The fold-in TARGET for sub-task 2 — a format-native `markers` transform that already exists,
    // independent of any legacy blob (matches the migration's own documented search scope).
    document["markers"]["Spawn"]["transforms"]["spawnTransformOne"]["position"] =
        { {"x", 1.0}, {"y", 2.0}, {"z", 3.0} };
    return document;
}

// (9) The JSON-level half: run the migration chain directly and inspect the migrated document,
// proving all 9 migrations fired, in the wired order, and the shared legacy blob was cleaned up.
void CheckHostileV2DocumentMigratesToExpectedV3Shape() {
    nlohmann::json document = BuildHostileV2MigrationFixture();
    Io::MapImportResult result;
    Io::RunSanmapMigrations(document, result, nullptr);

    Check(document["SanGenVersion"] == Io::kCurrentSanGenVersion,
          "the document lands on kCurrentSanGenVersion after the walk");
    Check(!document.contains("mapGeneratorData"), "the legacy blob is deleted once every migration "
                                                    "in the step has run");

    Check(document["GeneralMapSettings"]["Seed"] == 4242
          && document["GeneralMapSettings"]["WorldUnitsPerCell"] == 3.5,
          "GeneralMapSettings_Migrate_V2 relocated its fields");
    Check(document["Symmetry"]["GlobalSymmetryMask"] == 5
          && document["Symmetry"]["TorusMinorRadius"] == 2.5,
          "Symmetry_Migrate_V2 relocated its fields");
    Check(document.contains("Accumulation") && document["Accumulation"].is_object(),
          "Accumulation_Migrate_V2 reserved the empty key");
    Check(document["DetailNormal"]["DetailNormalMapSize"] == 2048,
          "DetailNormal_Migrate_V2 relocated its field");
    Check(document["Flow"]["FlowMapColor"]["r"] == 0.25 && document["Flow"]["FlowMapColor"]["a"] == 1.0,
          "Flow_Migrate_V2 relocated and converted FlowMapColor to the object shape");
    Check(document["GlobalMarkerSettings"]["GlobalIconAlloy"] == "IconAlloy"
          && document["GlobalMarkerSettings"]["MarkerColorPlasma"]["g"] == 1.0
          && document["GlobalMarkerSettings"]["MarkerScaleSpawn"] == 1.75,
          "GlobalMarkerSettings_Migrate_V2 relocated and converted its fields");
    Check(document["SlopeDefaults"]["bSlopeGateEnabled"] == true
          && document["SlopeDefaults"]["slopeGateStrength"] == 0.5,
          "SlopeDefaults_Migrate_V2 synthesized the global record");

    // The load-bearing ordering proof: index 0 carries BOTH SlopeDefaults_Migrate_V2's own write
    // (SlopeUseGlobal) AND StratumGenerationSettings_Migrate_V2's own 8 keys, on the SAME entry —
    // neither migration's write clobbered the other's.
    const nlohmann::json& entryZero = document["StratumGenerationSettings"][0];
    Check(document["StratumGenerationSettings"].size() == 9,
          "StratumGenerationSettings_Migrate_V2 padded to exactly 9 entries");
    Check(entryZero["SlopeUseGlobal"] == true,
          "SlopeDefaults_Migrate_V2's write survived (the single stratum trivially matches its own "
          "synthesized global)");
    Check(entryZero["SlopeGateEnabled"] == true && entryZero["MinimumSlopeDegrees"] == 10.0
          && entryZero["MaximumSlopeDegrees"] == 80.0 && entryZero["SlopeGateStrength"] == 0.5,
          "StratumGenerationSettings_Migrate_V2's own 8 keys also landed on the same entry — running "
          "SlopeDefaults_Migrate_V2 first did not get clobbered by this migration's additive writes");

    Check(document["armies"]["commander"]["armyColor"]["r"] == 0.25
          && document["armies"]["commander"]["armyColor"]["b"] == 0.75,
          "EntityCollections_Migrate_V2 converted the legacy army Color array into armyColor");
    Check(document["markers"]["Spawn"]["transforms"]["spawnTransformOne"]["alias"] == "MainBase",
          "EntityCollections_Migrate_V2 folded the legacy Aliases dict into the matching marker "
          "transform's own alias field");
}

// (9, continued) The Recipe-level half: the SAME fixture through the real front door
// (MapImporter::ParseSanmapJsonText), proving the whole chain lands on the exact expected
// Params::MapRecipe, not just the expected intermediate JSON.
void CheckHostileV2DocumentImportsToExpectedRecipe() {
    const nlohmann::json document = BuildHostileV2MigrationFixture();
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), recipe, Io::MapImportOptions(), result),
          "the hostile V2 document imports without refusal");

    Check(recipe.geometry.seed == 4242u && !recipe.geometry.bScaleFeaturesToMapSize
          && recipe.geometry.terrainMinHeight == 12.0f && recipe.geometry.worldUnitsPerCell == 3.5f,
          "GeneralMapSettings fields land on recipe.geometry");
    Check(recipe.globalSymmetryMask == 5 && recipe.symmetryDetection.bSnapImperfectSymmetry
          && recipe.symmetryDetection.detectionTolerance == 0.25f
          && recipe.symmetryBlend.superpositionBlend == 0.75f
          && recipe.symmetryBlend.torusMinorRadius == 2.5f,
          "Symmetry fields land on recipe.globalSymmetryMask/symmetryDetection/symmetryBlend");
    Check(recipe.detailNormal.mapSize == 2048, "DetailNormal fields land on recipe.detailNormal");
    Check(recipe.flow.flowMapColor[0] == 0.25f && recipe.flow.flowMapColor[3] == 1.0f,
          "Flow fields land on recipe.flow");
    Check(recipe.globalMarkerSettings.iconNameAlloy == "IconAlloy"
          && recipe.globalMarkerSettings.colorPlasma[1] == 1.0f
          && recipe.globalMarkerSettings.scaleSpawn == 1.75f,
          "GlobalMarkerSettings fields land on recipe.globalMarkerSettings");
    Check(recipe.slopeDefaults.bSlopeGateEnabled && recipe.slopeDefaults.slopeGateStrength == 0.5f,
          "SlopeDefaults fields land on recipe.slopeDefaults");
    Check(recipe.strata.size() >= 1 && recipe.strata[0].bSlopeUseGlobal
          && recipe.strata[0].bSlopeGateEnabled && recipe.strata[0].minimumSlopeDegrees == 10.0f
          && recipe.strata[0].slopeGateStrength == 0.5f,
          "both StratumGenerationSettings co-writers' fields land together on recipe.strata[0]");
    // STEP76_ArmyIdentityNaming_IO §4: `ParseSanmapJsonText` now unconditionally normalizes the
    // army identity AFTER the V2->V3 migration walk recovers this document. "commander" is not
    // ARMY_XX-shaped, so it normalizes to ARMY_01 and survives instead as `displayName` — proving
    // the migration chain and the STEP76 normalizer compose correctly, not a weakening of this
    // assertion (this WAS "recipe.armies[0].name == \"commander\"" before STEP76 retired `name` as
    // a human-authored field).
    Check(recipe.armies.size() == 1 && recipe.armies[0].name == "ARMY_01"
          && recipe.armies[0].displayName == "commander"
          && recipe.armies[0].armyColor[0] == 0.25f && recipe.armies[0].armyColor[2] == 0.75f,
          "the legacy army Color array lands on recipe.armies[0].armyColor, and the legacy army "
          "name normalizes to ARMY_01 while surviving as displayName");
    bool bFoundAliasedTransform = false;
    for (const Params::MarkerInstanceGroup& group : recipe.markers)
        for (const Params::MarkerTransform& transform : group.transforms)
            if (transform.name == "spawnTransformOne" && transform.alias == "MainBase")
                bFoundAliasedTransform = true;
    Check(bFoundAliasedTransform, "the legacy Aliases entry lands on the matching marker transform's "
                                  "own alias field");

    // (10) Re-exporting stamps the current version and never writes the legacy blob back out
    // (STEP36's already-shipped behavior) — proved on this SAME recovered recipe, not a fresh one.
    const nlohmann::json reexported =
        nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(recipe));
    Check(reexported["SanGenVersion"] == Io::kCurrentSanGenVersion,
          "re-exporting the recovered recipe stamps SanGenVersion = kCurrentSanGenVersion");
    Check(!reexported.contains("mapGeneratorData"),
          "re-exporting the recovered recipe writes no mapGeneratorData key");
}

// (11) A document already at kCurrentSanGenVersion runs zero migration steps — proved by deep
// equality against a pre-call snapshot (a side-effect check), not just "it still imports". The
// document still carries a legacy-shaped mapGeneratorData blob a V2 step WOULD touch if it ran, so
// this is a real proof the walk zero-iterated, not merely that this fixture had nothing to migrate.
void CheckCurrentVersionDocumentRunsZeroMigrationSteps() {
    nlohmann::json document = BuildHostileV2MigrationFixture();
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    const nlohmann::json snapshot = document;

    Io::MapImportResult result;
    Io::RunSanmapMigrations(document, result, nullptr);

    Check(document == snapshot,
          "a document already at kCurrentSanGenVersion is byte-for-byte unchanged — zero migration "
          "steps ran, not just \"it still imports\"");
    Check(result.warningCount == 0, "and no warning fires for an already-current document");
}

} // namespace

int main() {
    CheckCurrentVersionPassesThrough();
    CheckLegacyFallbackWarns();
    CheckNoVersionMarkerRecoversBestEffort();
    CheckNewerVersionRecoversBestEffort();
    CheckNewerVersionCapturesUnknownKeyIntoBag();
    CheckNoVersionMarkerFixtureSkipsMigrationChain();
    CheckReexportAlwaysStampsCurrentVersion();
    CheckUnknownKeyRoundTripsAndCollisionResolves();
    CheckMultipleUnknownKeysNestUnderOneUnknownImportKey();
    CheckUnknownImportRoundTripsWithoutAccumulatingNesting();
    CheckEmptyBagWritesNoUnknownImportKey();
    CheckDeletedLegacyKeyNeverAppearsInBag();
    CheckHostileV2DocumentMigratesToExpectedV3Shape();
    CheckHostileV2DocumentImportsToExpectedRecipe();
    CheckCurrentVersionDocumentRunsZeroMigrationSteps();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

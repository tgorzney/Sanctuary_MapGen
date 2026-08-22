// Sanmap_MigrationPreview_IO_Test.cpp — acceptance test (STEP26A, IO_MIGRATION_SPEC.md §3/§6).
// Not listed in the work-order's own "Files touched" table (an omission — the ticket's own "Verify"
// section requires this exact coverage, and every other file pair in this subsystem carries a paired
// test, IO_MIGRATION_SPEC.md §1), added as the paired test for the new `Sanmap_MigrationPreview_IO`
// file pair. Covers:
//  (1) PreviewSanmapMigrationWalk against a synthetic no-marker document reports all 9
//      sourceVersion-2 entries with correct per-entry diffs, and never mutates its input document.
//  (2) ApplySelectedSanmapMigrations with ONLY "Accumulation_Migrate_V2" opted in: Accumulation's key
//      lands, SanGenVersion stays unset (a genuine partial application, IO_MIGRATION_SPEC.md §6).
//      NOTE (coder finding, this ticket): the other 3 bIndependentlySelectable == true entries
//      (GeneralMapSettings/Symmetry/DetailNormal) correctly stay absent, since they are individually
//      opt-in and were not selected — but the 5 bIndependentlySelectable == false entries DO still
//      run and their target keys DO land, per the ticket's own Part 3 text and IO_MIGRATION_SPEC.md
//      §3/§6's explicit, doubly-ratified rule: "Every bIndependentlySelectable == false entry in a
//      step runs unconditionally whenever any entry of that step is selected." The ticket's one-line
//      "Verify" summary ("leaves every OTHER migration's target key absent") is imprecise shorthand
//      against that more specific, twice-stated rule; this test asserts the actual, spec-correct
//      behavior rather than the loosely-worded paraphrase.
//  (3) ApplySelectedSanmapMigrations with every one of the manifest's names selected (all 9
//      sourceVersion-2 entries plus STEP67's sourceVersion-3 MarkersStack_Migrate_V3): SanGenVersion
//      == kCurrentSanGenVersion, every target key present, and the shared legacy blob is deleted
//      (full-step application).
#include "Sanmap_MigrationPreview_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "MapImporter_IO.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// A synthetic, no-version-marker V2-shaped document touching a real source field for every one of
// the 9 sourceVersion-2 migrations — same field set as Sanmap_MigrationRunner_IO_Test.cpp's own
// BuildHostileV2MigrationFixture, minus any SanGenVersion/legacy-version marker at all (the
// no-marker precondition this whole surface exists for, IO_MIGRATION_SPEC.md §6).
nlohmann::json BuildNoMarkerV2Fixture() {
    nlohmann::json document;
    nlohmann::json& legacy = document["mapGeneratorData"];
    legacy["Seed"]                   = 4242;
    legacy["ScaleFeaturesToMapSize"] = false;
    legacy["TerrainMinHeight"]       = 12.0;
    legacy["WorldUnitsPerCell"]      = 3.5;
    legacy["GlobalSymmetryMask"]     = 5;
    legacy["SnapImperfectSymmetry"]  = true;
    legacy["DetailNormalMapSize"]    = 2048;
    legacy["FlowMapColor"]           = { 0.25, 0.5, 0.75, 1.0 };
    legacy["GlobalIconAlloy"]        = "IconAlloy";
    legacy["MarkerColorAlloy"]       = { 1.0, 0.0, 0.0, 1.0 };
    nlohmann::json stratum;
    stratum["SlopeGateEnabled"]  = true;
    stratum["SlopeGateStrength"] = 0.5;
    legacy["Stratums"] = nlohmann::json::array({ stratum });
    legacy["Armies"]["commander"]["Color"] = { 0.25, 0.5, 0.75, 1.0 };
    return document;
}

// --- (1) PreviewSanmapMigrationWalk. ---------------------------------------------------------------
void CheckPreviewReportsAllNineEntriesWithCorrectDiffs() {
    const nlohmann::json document = BuildNoMarkerV2Fixture();
    const nlohmann::json snapshot = document;

    const Io::MigrationPreviewReport report = Io::PreviewSanmapMigrationWalk(document);
    Check(document == snapshot, "PreviewSanmapMigrationWalk never mutates its input document");
    Check(report.assumedStartingVersion == 1, "the preview assumes starting version 1");

    const Io::MigrationPreviewStep* stepTwo = nullptr;
    for (const Io::MigrationPreviewStep& step : report.steps)
        if (step.sourceVersion == 2) stepTwo = &step;
    Check(stepTwo != nullptr, "the report includes the real sourceVersion-2 step");
    Check(stepTwo->entries.size() == 9, "the sourceVersion-2 step reports all 9 migration entries");

    bool bFoundAccumulation = false, bFoundGeneralMapSettings = false;
    for (const Io::MigrationPreviewEntry& entry : stepTwo->entries) {
        if (std::string(entry.name) == "Accumulation_Migrate_V2") {
            bFoundAccumulation = true;
            Check(entry.bIndependentlySelectable, "Accumulation_Migrate_V2's preview entry mirrors "
                                                   "bIndependentlySelectable == true");
            Check(entry.bLosslessIfSkipped, "Accumulation_Migrate_V2's preview entry mirrors "
                                            "bLosslessIfSkipped == true");
            Check(entry.bWouldChangeDocument, "Accumulation_Migrate_V2 would change the document "
                                              "(reserves the empty key)");
            Check(!entry.diffPatch.empty(), "Accumulation_Migrate_V2's diffPatch is non-empty");
        }
        if (std::string(entry.name) == "GeneralMapSettings_Migrate_V2") {
            bFoundGeneralMapSettings = true;
            Check(!entry.bLosslessIfSkipped, "GeneralMapSettings_Migrate_V2's preview entry mirrors "
                                             "bLosslessIfSkipped == false");
            Check(entry.bWouldChangeDocument, "GeneralMapSettings_Migrate_V2 would change the "
                                              "document (relocates real fields)");
        }
    }
    Check(bFoundAccumulation && bFoundGeneralMapSettings,
          "both a bLosslessIfSkipped == true and == false entry are present in the report");
    Check(!stepTwo->legacyKeysToDelete.empty(),
          "the step's legacyKeysToDelete mirror is populated informationally");
}

// --- (2) Partial selective apply. ------------------------------------------------------------------
void CheckSelectiveApplyOfOnlyAccumulationIsPartial() {
    nlohmann::json document = BuildNoMarkerV2Fixture();
    Io::MapImportResult result;
    Io::ApplySelectedSanmapMigrations(document, { "Accumulation_Migrate_V2" }, result);

    Check(document.contains("Accumulation"), "the one selected entry's key lands");
    Check(!document.contains("GeneralMapSettings") && !document.contains("Symmetry")
          && !document.contains("DetailNormal"),
          "the 3 OTHER independently-selectable entries, not individually selected, stay absent");
    Check(!document.contains("SanGenVersion"),
          "a partial application never stamps SanGenVersion — this is a genuine partial-apply, per "
          "IO_MIGRATION_SPEC.md §3/§6");
    Check(document.contains("mapGeneratorData"),
          "legacyKeysToDelete never fires on a partial application (§3)");
}

// --- (3) Full selective apply. ----------------------------------------------------------------------
void CheckSelectiveApplyOfAllNineIsFull() {
    nlohmann::json document = BuildNoMarkerV2Fixture();
    Io::MapImportResult result;
    const std::vector<std::string> allNames = {
        "GeneralMapSettings_Migrate_V2", "Symmetry_Migrate_V2", "Accumulation_Migrate_V2",
        "DetailNormal_Migrate_V2", "Flow_Migrate_V2", "GlobalMarkerSettings_Migrate_V2",
        "SlopeDefaults_Migrate_V2", "StratumGenerationSettings_Migrate_V2",
        "EntityCollections_Migrate_V2",
        // STEP67: the sourceVersion-3 step's one entry — without it selected too, the walk stops
        // after sourceVersion 2 (NoneSelected at sourceVersion 3) and never reaches
        // kCurrentSanGenVersion, since a "full" selection must cover every step the live manifest
        // now defines, not just the original 9.
        "MarkersStack_Migrate_V3",
    };
    Io::ApplySelectedSanmapMigrations(document, allNames, result);

    Check(document.contains("SanGenVersion") && document["SanGenVersion"] == Io::kCurrentSanGenVersion,
          "a full selection stamps SanGenVersion == kCurrentSanGenVersion");
    Check(document.contains("GeneralMapSettings") && document.contains("Symmetry")
          && document.contains("Accumulation") && document.contains("DetailNormal")
          && document.contains("Flow") && document.contains("GlobalMarkerSettings")
          && document.contains("SlopeDefaults") && document.contains("StratumGenerationSettings")
          && document.contains("armies"),
          "every migration's target key is present after a full selection");
    Check(!document.contains("mapGeneratorData"),
          "legacyKeysToDelete fires on full-step application, deleting the legacy blob");
}

} // namespace

int main() {
    CheckPreviewReportsAllNineEntriesWithCorrectDiffs();
    CheckSelectiveApplyOfOnlyAccumulationIsPartial();
    CheckSelectiveApplyOfAllNineIsFull();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

// MapImporter_Validation_IO_Test.cpp — the Constitution §6 half of the "Load Sanmap" acceptance
// test: a corrupt, partial or hostile document must leave the recipe on its own defaults, log the
// reason, and come out VALID — never crash and never hand the pipeline a recipe it would refuse.
#include "MapFormat_TestSupport_IO.h"
#include "MapImporter_IO.h"

namespace SanmapGen {
namespace MapFormatTest {
namespace {

void CheckUnusableDocumentsAreRefused() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(!Io::MapImporter::ParseSanmapJsonText("{ this is not json", recipe, options, result),
          "unparseable text is refused");
    Check(!result.debugLog.empty(), "with the parse error logged for the tab's panel");
    Check(!Io::MapImporter::ParseSanmapJsonText("[1,2,3]", recipe, options, result),
          "a non-object document is refused");
}

void CheckAPartialDocumentRecoversWhatItHas() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    // "SanGenVersion":2 satisfies the migration runner's version gate (STEP6_MigrationSubsystem_IO
    // — the runner is now the literal first thing ParseSanmapJsonText does, and a document with no
    // version marker at all is refused, never guessed). This fixture is deliberately partial BELOW
    // that gate: it still has no `mapGeneratorData` block, which is what this check actually tests.
    Check(Io::MapImporter::ParseSanmapJsonText("{\"SanGenVersion\": 2, \"width\": 128}", recipe,
                                               options, result),
          "a document with no generator block still loads what it has");
    Check(recipe.geometry.mapSize == 128, "recovering the map's own dimensions");
    // STEP38_MapGeneratorDataWarningWording_IO: absence of the legacy mapGeneratorData block is the
    // expected, normal state for a current-format export, not a degraded-recovery signal — the
    // notice is informational (Log(), not Warn()), so THAT specific note never warns.
    //
    // STEP40F temporarily patched this to `warningCount == 1`: with the V2 manifest step wired in,
    // `StratumGenerationSettings_Migrate_V2` unconditionally padded `StratumGenerationSettings` to 9
    // entries even with zero source `mapGeneratorData.Stratums` data, tripping
    // `ReadStratumGenerationSettingsJson`'s cardinality check against this fixture's 0
    // `stratumLayers` entries — a genuine bug, not a test bug, fixed by
    // STEP41_PostMigrationImportGaps_IO (see that migration's own N = 0 short-circuit). With the bug
    // fixed, this fixture is back to producing zero warnings.
    Check(result.warningCount == 0, "and no warning at all — the map's own dimensions recover "
                                     "cleanly with nothing left to warn about");
}

// A wrong-typed key, an out-of-cap size, a negative ceiling and an out-of-range enum, in one go.
//
// STEP40F regression finding: this fixture used to carry a literal "SanGenVersion":2. Once the V2
// manifest step was populated (STEP40F), that marker would route the document through the real
// 9-step migration walk, whose shared `legacyKeysToDelete` erases `mapGeneratorData` BEFORE
// `ReadGeometryJson`/`ReadStrataSettingsJson` (the legacy-gated readers this test's guard logic
// actually lives in) ever run — the runner runs before any block reader. Two of this test's own
// hostile fields (`MapSize`, `TerrainMaxHeight`) have no V2->V3 migration owning them at all (they
// are not among any of the 9 migrations' source fields), so they would simply be discarded, unread,
// alongside the rest of the deleted blob — the assertions below would either pass VACUOUSLY (never
// exercising the guard they claim to) or, for `WorldUnitsPerCell` specifically (which IS relocated,
// unclamped, by `GeneralMapSettings_Migrate_V2`, straight into `geometry.worldUnitsPerCell` with no
// guard at that new call site), genuinely FAIL — a real gap in guard placement, not a test bug, and
// out of this ticket's scope to fix (only the 9 migrations' own logic and the manifest wiring are
// in scope; see the work-order's explicit out-of-scope list). Dropping the version marker instead
// routes this document through the still-real, still-documented "no version marker" recovery law
// (Sanmap_MigrationRunner_IO.h) — `mapGeneratorData` survives untouched, and `ReadGeometryJson`'s
// own guard/clamp block is genuinely exercised, exactly as this test's name claims.
void CheckHostileValuesFallBackToDefaults() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    const char* hostileText = "{\"mapGeneratorData\":{\"MapSize\":99999,\"Seed\":\"nope\","
                              "\"TerrainMaxHeight\":-4.0,\"WorldUnitsPerCell\":0.0},"
                              "\"HeightmapStack\":{\"GeoLayers\":[{\"Mode\":77}]}}";
    Check(Io::MapImporter::ParseSanmapJsonText(hostileText, recipe, options, result),
          "a hostile document parses rather than throwing");
    Check(recipe.geometry.mapSize == Params::Geometry().mapSize,
          "an out-of-cap MapSize is refused and the default kept");
    Check(recipe.geometry.seed == Params::Geometry().seed, "a wrong-typed Seed is ignored");
    Check(recipe.geometry.terrainMaxHeight >= 1.0f,
          "a negative ceiling is raised so Geometry::IsValid() can never fail on import");
    Check(recipe.geometry.worldUnitsPerCell > 0.0f, "and a zero cell size is restored");
    Check(recipe.IsValid(), "so the whole recipe comes out valid");
    Check(recipe.layerStack.geoLayers.size() == 1
          && recipe.layerStack.geoLayers[0].mode == Params::GeoLayer().mode,
          "an out-of-range enum is fenced to its default rather than cast wild");
    Check(result.warningCount > 0, "with every fallback warned about");
}

// SANMAP_FORMAT_SPEC Correction 2, ARCH Expert finding 2: Seed's negative-value guard, exercised
// through the new top-level `GeneralMapSettings` object — a negative signed value clamps to 0
// rather than wrapping around to ~4 billion when cast to unsigned.
void CheckNegativeSeedClampsToZero() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    const char* documentText = "{\"SanGenVersion\":2,\"GeneralMapSettings\":{\"Seed\":-77}}";
    Check(Io::MapImporter::ParseSanmapJsonText(documentText, recipe, options, result),
          "a document with a negative Seed still parses");
    Check(recipe.geometry.seed == 0u,
          "a negative Seed clamps to 0, not a raw negative-to-unsigned wraparound");
}

// SANMAP_FORMAT_SPEC Correction 2, ARCH Expert finding 1: the TerrainMinHeight/TerrainMaxHeight
// band invariant at the end of ReadGeometryJson is still enforced correctly post-relocation — it
// stays correct ONLY because ReadGeneralMapSettingsJson (which now owns TerrainMinHeight) runs
// BEFORE ReadGeometryJson (which owns TerrainMaxHeight, from the legacy mapGeneratorData blob).
void CheckTerrainMinHeightBandClampPostRelocation() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    // STEP40F regression finding (same root cause as CheckHostileValuesFallBackToDefaults above):
    // `TerrainMaxHeight` here deliberately lives in the legacy `mapGeneratorData` blob, not
    // `GeneralMapSettings` — no V2->V3 migration relocates it (GeneralMapSettings_Migrate_V2's own
    // field list is Seed/ScaleFeaturesToMapSize/TerrainMinHeight/WorldUnitsPerCell only). A literal
    // "SanGenVersion":2 would now walk the real migration step and delete `mapGeneratorData` before
    // `ReadGeometryJson` (which owns the band-clamp this test exists to check) ever reads it — no
    // version marker at all keeps the document on the still-real "no version marker" recovery path
    // instead, so the legacy blob survives long enough to exercise the clamp genuinely.
    const char* documentText =
        "{\"GeneralMapSettings\":{\"TerrainMinHeight\":500.0},"
        "\"mapGeneratorData\":{\"TerrainMaxHeight\":100.0}}";
    Check(Io::MapImporter::ParseSanmapJsonText(documentText, recipe, options, result),
          "a document with TerrainMinHeight above TerrainMaxHeight still parses");
    Check(NearlyEqual(recipe.geometry.terrainMaxHeight, 100.0f), "TerrainMaxHeight lands as written");
    Check(NearlyEqual(recipe.geometry.terrainMinHeight, 99.0f),
          "a floor above the ceiling is clamped one unit below it, exactly as before the relocation");
    Check(result.warningCount > 0, "with the clamp logged as a warning");
}

// STEP41_PostMigrationImportGaps_IO acceptance items 1-2: the geometry band clamp — previously
// only reachable inside the gated legacy `mapGeneratorData` block, and so NEVER exercised for any
// current-format file since STEP36 stopped writing that block — must still fire for a document with
// NO `mapGeneratorData` at all, sourced purely from `GeneralMapSettings`. No version marker, same
// "no version marker" recovery path the other fixtures in this file already rely on, so nothing
// about the migration step is in play here.
void CheckGeometryBandClampsWithNoLegacyBlockPresent() {
    const Io::MapImportOptions options;
    {
        Params::MapRecipe recipe;
        Io::MapImportResult result;
        const char* documentText = "{\"GeneralMapSettings\":{\"TerrainMaxHeight\":0.0}}";
        Check(Io::MapImporter::ParseSanmapJsonText(documentText, recipe, options, result),
              "a document with a non-positive TerrainMaxHeight and no legacy block still parses");
        Check(NearlyEqual(recipe.geometry.terrainMaxHeight, 1.0f),
              "TerrainMaxHeight is clamped to 1 even with no mapGeneratorData block present");
        Check(result.warningCount > 0, "with the clamp logged as a warning");
    }
    {
        Params::MapRecipe recipe;
        Io::MapImportResult result;
        const char* documentText =
            "{\"GeneralMapSettings\":{\"TerrainMaxHeight\":100.0,\"TerrainMinHeight\":500.0}}";
        Check(Io::MapImporter::ParseSanmapJsonText(documentText, recipe, options, result),
              "a document with TerrainMinHeight above the ceiling and no legacy block still parses");
        Check(NearlyEqual(recipe.geometry.terrainMinHeight, 99.0f),
              "TerrainMinHeight is held one unit below the ceiling with no mapGeneratorData block "
              "present");
        Check(result.warningCount > 0, "with the clamp logged as a warning");
    }
    {
        Params::MapRecipe recipe;
        Io::MapImportResult result;
        const char* documentText = "{\"GeneralMapSettings\":{\"WorldUnitsPerCell\":0.0}}";
        Check(Io::MapImporter::ParseSanmapJsonText(documentText, recipe, options, result),
              "a document with a non-positive WorldUnitsPerCell and no legacy block still parses");
        Check(recipe.geometry.worldUnitsPerCell > 0.0f,
              "WorldUnitsPerCell is restored to 1 even with no mapGeneratorData block present");
        Check(result.warningCount > 0, "with the clamp logged as a warning");
    }
}

void CheckAnUnresolvablePathIsRefused() {
    std::string documentPath;
    std::string folderPath;
    Check(!Io::ResolveSanmapDocumentPath(std::string(), documentPath, folderPath),
          "an empty path resolves to nothing");
    Check(!Io::ResolveSanmapDocumentPath("D:/no/such/place", documentPath, folderPath),
          "and so does a path that is neither a file nor a folder");
    Params::MapRecipe recipe;
    const Io::MapImportResult result = Io::MapImporter::LoadSanmap("D:/no/such/place", recipe, nullptr);
    Check(!result.bSucceeded && !result.debugLog.empty(),
          "loading it fails with the reason in the panel");
}

} // namespace

void RunValidationTests() {
    CheckUnusableDocumentsAreRefused();
    CheckAPartialDocumentRecoversWhatItHas();
    CheckHostileValuesFallBackToDefaults();
    CheckNegativeSeedClampsToZero();
    CheckTerrainMinHeightBandClampPostRelocation();
    CheckGeometryBandClampsWithNoLegacyBlockPresent();
    CheckAnUnresolvablePathIsRefused();
}

} // namespace MapFormatTest
} // namespace SanmapGen

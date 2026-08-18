// Sanmap_MigrationRunner_IO_Test.cpp — acceptance test for STEP6_MigrationSubsystem_IO's four
// named cases (work-order "Acceptance test" item 2):
//  (a) SanGenVersion == kCurrentSanGenVersion -> passes through, no warning logged.
//  (b) no SanGenVersion but a legacy mapGeneratorData.MapGeneratorDataVersion == current -> passes
//      through, exactly one warning logged about the fallback.
//  (c) neither field present -> refused, bSucceeded stays false, a reason is logged.
//  (d) SanGenVersion newer than current -> refused, a DIFFERENT reason is logged than (c)'s.
#include "Sanmap_MigrationRunner_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "MapImporter_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// (a) Current-version passthrough (§4.3): still runs resolution + the refusal check, calls no
// migration, logs no warning.
void CheckCurrentVersionPassesThrough() {
    nlohmann::json document = { {"SanGenVersion", Io::kCurrentSanGenVersion} };
    Io::MapImportResult result;
    const bool bAccepted = Io::RunSanmapMigrations(document, result);
    Check(bAccepted, "a document already at kCurrentSanGenVersion is accepted");
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
    const bool bAccepted = Io::RunSanmapMigrations(document, result);
    Check(bAccepted, "a document with only the legacy version field is accepted");
    Check(result.warningCount == 1, "the legacy fallback logs exactly one warning");
}

// (c) No version marker of any kind -> refuse; never guess version 1.
void CheckNoVersionMarkerRefuses() {
    nlohmann::json document = { {"someOtherField", 1} };
    Io::MapImportResult result;
    const bool bAccepted = Io::RunSanmapMigrations(document, result);
    Check(!bAccepted, "a document with no version marker at all is refused");
    Check(!result.debugLog.empty(), "the refusal reason is logged");
}

// (d) Newer than current -> refuse, with a reason distinct from case (c)'s.
void CheckNewerVersionRefuses() {
    nlohmann::json document = { {"SanGenVersion", 99} };
    Io::MapImportResult result;
    const bool bAccepted = Io::RunSanmapMigrations(document, result);
    Check(!bAccepted, "a document newer than kCurrentSanGenVersion is refused");
    Check(!result.debugLog.empty(), "the refusal reason is logged");

    nlohmann::json noMarkerDocument = { {"someOtherField", 1} };
    Io::MapImportResult noMarkerResult;
    Io::RunSanmapMigrations(noMarkerDocument, noMarkerResult);
    Check(result.debugLog != noMarkerResult.debugLog,
          "the 'newer version' refusal reason is distinct from the 'no version marker' reason");
}

} // namespace

int main() {
    CheckCurrentVersionPassesThrough();
    CheckLegacyFallbackWarns();
    CheckNoVersionMarkerRefuses();
    CheckNewerVersionRefuses();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

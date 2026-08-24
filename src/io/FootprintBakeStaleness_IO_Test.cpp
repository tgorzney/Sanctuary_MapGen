// FootprintBakeStaleness_IO_Test.cpp — acceptance test for
// work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md §3: CheckFootprintBakeStaleness's scan
// (acceptance tests 6-10) and MapImporter::LoadSanmap's one-aggregate-Warn() call site 1
// (acceptance test 11, mirroring STEP82 acceptance test 9's own disk-based shape).
#include "FootprintBakeStaleness_IO.h"
#include "MapExporter_IO.h"
#include "MapImporter_IO.h"
#include "TemplateIngest_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <cstring>
#include <filesystem>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void SetTemplateIdentifier(char (&templateIdentifier)[8], const char* text) {
    std::memset(templateIdentifier, 0, sizeof(templateIdentifier));
    std::memcpy(templateIdentifier, text, std::strlen(text) < 8u ? std::strlen(text) : 8u);
}

Params::PropRule MakeBakedPropRule(const char* templateIdentifier, float width, float depth) {
    Params::PropRule rule;
    SetTemplateIdentifier(rule.transform.templateIdentifier, templateIdentifier);
    rule.baseFootprintWidth  = width;
    rule.baseFootprintDepth  = depth;
    rule.footprintBakeFingerprint.sourcePath   = std::string("Templates/") + templateIdentifier + ".santp";
    rule.footprintBakeFingerprint.byteSize     = 1000ull;
    rule.footprintBakeFingerprint.modifiedTime = 2000ull;
    rule.footprintBakeFingerprint.contentHash  = 3000ull;
    return rule;
}

void AddIngestRecord(Io::TemplateIngestReport& report, const std::string& templateIdentifier,
                     float width, float depth, std::uint64_t byteSize, std::uint64_t modifiedTime,
                     std::uint64_t contentHash) {
    Io::TemplateFootprintRecord record;
    record.baseFootprintWidth = width;
    record.baseFootprintDepth = depth;
    record.sourceFingerprint.sourcePath   = "Templates/" + templateIdentifier + ".santp";
    record.sourceFingerprint.byteSize     = byteSize;
    record.sourceFingerprint.modifiedTime = modifiedTime;
    record.sourceFingerprint.contentHash  = contentHash;
    report.footprintByTemplateIdentifier[templateIdentifier] = record;
}

// Acceptance test 6: baked fingerprint matches the current report's fingerprint for that tpId.
void RunFreshChecks() {
    Params::MapRecipe recipe;
    recipe.propRules.push_back(MakeBakedPropRule("edbm0101", 1.0f, 1.0f));
    recipe.propRules[0].footprintBakeFingerprint.byteSize     = 1000ull;
    recipe.propRules[0].footprintBakeFingerprint.modifiedTime = 2000ull;
    recipe.propRules[0].footprintBakeFingerprint.contentHash  = 3000ull;
    Io::TemplateIngestReport report;
    AddIngestRecord(report, "edbm0101", 1.0f, 1.0f, 1000ull, 2000ull, 3000ull);
    const Io::FootprintBakeStalenessReport staleness = Io::CheckFootprintBakeStaleness(recipe, report);
    Check(staleness.AllFresh() && staleness.SummaryText().empty(),
          "a matching fingerprint reports AllFresh() with an empty SummaryText()");
}

// Acceptance test 7: the current report's fingerprint differs -> reported with old/new width/depth.
void RunChangedChecks() {
    Params::MapRecipe recipe;
    recipe.propRules.push_back(MakeBakedPropRule("edbm0101", 0.70f, 0.69f));
    Io::TemplateIngestReport report;
    AddIngestRecord(report, "edbm0101", 0.82f, 0.79f, 1000ull, 2000ull, 9999ull);   // contentHash differs
    const Io::FootprintBakeStalenessReport staleness = Io::CheckFootprintBakeStaleness(recipe, report);
    Check(!staleness.AllFresh() && staleness.staleEntries.size() == 1u, "the change is detected");
    const Io::StaleFootprintEntry& entry = staleness.staleEntries[0];
    Check(!entry.bNoLongerIngestible && entry.ruleKind == "Prop" && entry.templateIdentifier == "edbm0101"
          && entry.oldBaseFootprintWidth == 0.70f && entry.oldBaseFootprintDepth == 0.69f
          && entry.newBaseFootprintWidth == 0.82f && entry.newBaseFootprintDepth == 0.79f,
          "old/new width/depth land on the entry correctly");
    Check(staleness.SummaryText().find("0.70x0.69") != std::string::npos
          && staleness.SummaryText().find("0.82x0.79") != std::string::npos,
          "SummaryText carries the correct old/new width/depth values");
}

// Acceptance test 8: FindByTemplateIdentifier now returns nullptr for a previously-baked tpId.
void RunNoLongerIngestibleChecks() {
    Params::MapRecipe recipe;
    recipe.propRules.push_back(MakeBakedPropRule("edbm0101", 4.0f, 4.0f));
    Io::TemplateIngestReport report;
    // Non-empty report (some OTHER template really was ingested this session), but not this one.
    AddIngestRecord(report, "edbm0202", 1.0f, 1.0f, 1000ull, 2000ull, 3000ull);
    const Io::FootprintBakeStalenessReport staleness = Io::CheckFootprintBakeStaleness(recipe, report);
    Check(!staleness.AllFresh() && staleness.staleEntries.size() == 1u
          && staleness.staleEntries[0].bNoLongerIngestible,
          "a genuinely non-empty report missing this one tpId reports bNoLongerIngestible");
    Check(staleness.SummaryText().find("no longer found in the current game install") != std::string::npos,
          "SummaryText carries the specific no-longer-ingestible wording");
}

// Acceptance test 9: never-baked rules never appear in staleEntries regardless of report content.
void RunNeverBakedChecks() {
    Params::MapRecipe recipe;
    recipe.propRules.push_back(Params::PropRule());   // never baked
    SetTemplateIdentifier(recipe.propRules[0].transform.templateIdentifier, "edbm0101");
    Io::TemplateIngestReport report;
    AddIngestRecord(report, "edbm0101", 9.0f, 9.0f, 1ull, 2ull, 3ull);   // wildly different, irrelevant
    const Io::FootprintBakeStalenessReport staleness = Io::CheckFootprintBakeStaleness(recipe, report);
    Check(staleness.AllFresh(), "a never-baked rule is skipped no matter what the report contains");
}

// Acceptance test 10: a default-constructed TemplateIngestReport returns an empty report immediately
// -- proves the check never requires a game install, even for a recipe with baked rules already in it.
void RunEmptyReportChecks() {
    Params::MapRecipe recipe;
    recipe.propRules.push_back(MakeBakedPropRule("edbm0101", 4.0f, 4.0f));
    recipe.unitRules.push_back(Params::UnitRule());
    const Io::TemplateIngestReport emptyReport;
    const Io::FootprintBakeStalenessReport staleness = Io::CheckFootprintBakeStaleness(recipe, emptyReport);
    Check(staleness.AllFresh(), "a default-constructed report yields an empty staleness report");
}

std::string ScratchFolderPath() {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) / "SanGenFootprintBakeStalenessTest";
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

// Acceptance test 11: three stale rules at once -> exactly one Warn() call on import (mirrors STEP82
// acceptance test 9's "one aggregate warning, not one per army").
void RunOneAggregateWarningChecks() {
    Params::MapRecipe recipe;
    recipe.mapName = "FootprintStalenessFixture";
    recipe.propRules.push_back(MakeBakedPropRule("edbm0101", 1.0f, 1.0f));
    recipe.propRules.push_back(MakeBakedPropRule("edbm0202", 1.0f, 1.0f));
    recipe.unitRules.push_back(Params::UnitRule());
    SetTemplateIdentifier(recipe.unitRules[0].transform.templateIdentifier, "uca1001");
    recipe.unitRules[0].baseFootprintWidth  = 1.2f;
    recipe.unitRules[0].baseFootprintDepth  = 1.2f;
    recipe.unitRules[0].footprintBakeFingerprint.sourcePath   = "Templates/uca1001.santp";
    recipe.unitRules[0].footprintBakeFingerprint.byteSize     = 1000ull;
    recipe.unitRules[0].footprintBakeFingerprint.modifiedTime = 2000ull;
    recipe.unitRules[0].footprintBakeFingerprint.contentHash  = 3000ull;

    const std::string folderPath = ScratchFolderPath();
    const Io::MapExportResult exportResult = Io::MapExporter::ExportSanmapOnly(folderPath, recipe);
    Check(exportResult.bSucceeded, "the fixture recipe exports so the import half has a real file");

    // A live report making ALL THREE baked rules stale at once (contentHash disagrees on every one).
    Io::TemplateIngestReport report;
    AddIngestRecord(report, "edbm0101", 1.1f, 1.1f, 1000ull, 2000ull, 9999ull);
    AddIngestRecord(report, "edbm0202", 1.1f, 1.1f, 1000ull, 2000ull, 9999ull);
    AddIngestRecord(report, "uca1001",  1.3f, 1.3f, 1000ull, 2000ull, 9999ull);

    Params::MapRecipe loaded;
    const Io::MapImportResult importResult =
        Io::MapImporter::LoadSanmap(folderPath, loaded, nullptr, Io::MapImportOptions(), nullptr, &report);
    Check(importResult.bSucceeded, "the import itself still succeeds");
    Check(importResult.warningCount == 1,
          "three simultaneous stale rules produce exactly ONE aggregate Warn() call, not three");
    Check(importResult.debugLog.find("3 baked footprint value(s) are stale") != std::string::npos,
          "the one warning names the correct count in its own summary text");

    std::error_code cleanupError;
    std::filesystem::remove_all(folderPath, cleanupError);
}

} // namespace

int main() {
    RunFreshChecks();
    RunChangedChecks();
    RunNoLongerIngestibleChecks();
    RunNeverBakedChecks();
    RunEmptyReportChecks();
    RunOneAggregateWarningChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

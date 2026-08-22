// Sanmap_MigrationPreview_IO.cpp — see the header for the full contract.
#include "Sanmap_MigrationPreview_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include <string>

namespace SanmapGen {
namespace Io {
namespace {

constexpr const char* kSanGenVersionKey = "SanGenVersion";

// Finds the manifest's step for `sourceVersion`, or nullptr — the table is sparse by construction
// (IO_MIGRATION_SPEC.md §3), so most source versions have no entry at all.
const MigrationStep* FindManifestStep(const std::vector<MigrationStep>& manifest, int sourceVersion) {
    for (const MigrationStep& candidate : manifest)
        if (candidate.sourceVersion == sourceVersion) return &candidate;
    return nullptr;
}

bool IsNameSelected(const std::vector<std::string>& selectedNames, const char* name) {
    for (const std::string& selectedName : selectedNames)
        if (selectedName == name) return true;
    return false;
}

// Builds one step's preview entries and its legacyKeysToDelete informational mirror, advancing
// `workingDocument` as it walks so each entry's own diff reflects its real contribution on top of its
// already-applied siblings (header comment) — extracted to keep `PreviewSanmapMigrationWalk` within
// the ARCH's function-length ceiling. `step` may be nullptr (sparse manifest, §3): an empty preview
// step, correctly.
MigrationPreviewStep BuildPreviewStep(const MigrationStep* step, int sourceVersion,
                                      nlohmann::json& workingDocument) {
    MigrationPreviewStep previewStep;
    previewStep.sourceVersion = sourceVersion;
    if (step == nullptr) return previewStep;

    for (const MigrationEntry& entry : step->migrations) {
        const nlohmann::json before = workingDocument;
        nlohmann::json after = workingDocument;
        entry.function(after);

        MigrationPreviewEntry previewEntry;
        previewEntry.name                     = entry.name;
        previewEntry.description               = entry.description;
        previewEntry.bIndependentlySelectable   = entry.bIndependentlySelectable;
        previewEntry.bLosslessIfSkipped         = entry.bLosslessIfSkipped;
        previewEntry.diffPatch                  = nlohmann::json::diff(before, after);
        previewEntry.bWouldChangeDocument       = !previewEntry.diffPatch.empty();
        previewStep.entries.push_back(std::move(previewEntry));

        workingDocument = std::move(after);
    }
    for (const char* legacyKey : step->legacyKeysToDelete) {
        previewStep.legacyKeysToDelete.push_back(legacyKey);
        DeleteKeyIfPresent(workingDocument, legacyKey);
    }
    return previewStep;
}

// Result of attempting one step during selective apply — drives the caller's loop continue/break
// decision. Extracted (with `ApplyOneStep` below) to keep `ApplySelectedSanmapMigrations` within the
// ARCH's function-length ceiling.
enum class StepApplyOutcome { FullyApplied, PartiallyApplied, NoneSelected };

// Runs `step` against `document` per `selectedNames` (IO_MIGRATION_SPEC.md §3/§6): every
// bIndependentlySelectable == false entry runs unconditionally once ANY entry of the step is
// selected; legacyKeysToDelete fires only when EVERY entry was selected. Never writes SanGenVersion —
// that stamp is the caller's job, mirroring `RunSanmapMigrations`'s own division of responsibility.
StepApplyOutcome ApplyOneStep(const MigrationStep& step, const std::vector<std::string>& selectedNames,
                              nlohmann::json& document, MapImportResult& result) {
    bool bAnySelected = false;
    for (const MigrationEntry& entry : step.migrations)
        if (IsNameSelected(selectedNames, entry.name)) { bAnySelected = true; break; }
    if (!bAnySelected) return StepApplyOutcome::NoneSelected;

    bool bEveryEntrySelected = true;
    for (const MigrationEntry& entry : step.migrations) {
        const bool bThisSelected = IsNameSelected(selectedNames, entry.name);
        if (!bThisSelected) bEveryEntrySelected = false;
        if (!entry.bIndependentlySelectable || bThisSelected) {
            entry.function(document);
            result.Log(std::string("Selective migration apply: ran ") + entry.name + ".");
        }
    }
    if (!bEveryEntrySelected) return StepApplyOutcome::PartiallyApplied;

    for (const char* legacyKey : step.legacyKeysToDelete) DeleteKeyIfPresent(document, legacyKey);
    return StepApplyOutcome::FullyApplied;
}

} // namespace

MigrationPreviewReport PreviewSanmapMigrationWalk(const nlohmann::json& document) {
    MigrationPreviewReport report;
    const std::vector<MigrationStep>& manifest = SanmapMigrationManifest();

    // A working copy the shadow walk mutates so each entry's diff reflects its real contribution on
    // top of its already-applied siblings — `document` itself is never touched.
    nlohmann::json workingDocument = document;

    for (int sourceVersion = report.assumedStartingVersion; sourceVersion < kCurrentSanGenVersion;
         ++sourceVersion) {
        const MigrationStep* step = FindManifestStep(manifest, sourceVersion);
        report.steps.push_back(BuildPreviewStep(step, sourceVersion, workingDocument));
    }
    return report;
}

void ApplySelectedSanmapMigrations(nlohmann::json& document,
                                   const std::vector<std::string>& selectedNames,
                                   MapImportResult& result) {
    const std::vector<MigrationStep>& manifest = SanmapMigrationManifest();

    for (int sourceVersion = 1; sourceVersion < kCurrentSanGenVersion; ++sourceVersion) {
        const MigrationStep* step = FindManifestStep(manifest, sourceVersion);
        if (step == nullptr) {
            // The sparse manifest (§3) has no step for this source version at all — nothing to
            // select, nothing to run, and (deliberately) no SanGenVersion stamp either: this
            // no-marker/assumed-version-1 walk must never claim progress the manifest does not
            // actually define. A real, wired step is what advances the version.
            continue;
        }

        const StepApplyOutcome outcome = ApplyOneStep(*step, selectedNames, document, result);
        if (outcome == StepApplyOutcome::NoneSelected) {
            result.Log("Selective migration apply: sourceVersion=" + std::to_string(sourceVersion)
                       + " had no entries selected; stopping here, SanGenVersion left unchanged.");
            break;
        }
        if (outcome == StepApplyOutcome::PartiallyApplied) {
            result.Log("Selective migration apply: sourceVersion=" + std::to_string(sourceVersion)
                       + " was only partially selected; SanGenVersion left unchanged, and no further "
                         "step will be applied.");
            break;
        }
        document[kSanGenVersionKey] = sourceVersion + 1;
    }
}

} // namespace Io
} // namespace SanmapGen

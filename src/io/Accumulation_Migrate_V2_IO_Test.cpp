// Accumulation_Migrate_V2_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1): confirms
// `Accumulation_Migrate_V2` reserves an empty top-level `Accumulation` object, unconditionally and
// idempotently, and never overwrites one already present.
//
// STEP26A's paired `bLosslessIfSkipped = true` test (IO_MIGRATION_SPEC.md §3): given the step's
// OLD-shape fixture, run every OTHER migration in the step (never Accumulation_Migrate_V2 itself, and
// never the step's legacyKeysToDelete, matching §6's partial-application rule) and confirm the
// resulting document is still valid with Accumulation absent — the spec's own "nothing to carry"
// allowance, since this migration reserves an empty key with zero legacy source to lose.
#include "Accumulation_Migrate_V2_IO.h"
#include "GeneralMapSettings_Migrate_V2_IO.h"
#include "Symmetry_Migrate_V2_IO.h"
#include "DetailNormal_Migrate_V2_IO.h"
#include "Flow_Migrate_V2_IO.h"
#include "GlobalMarkerSettings_Migrate_V2_IO.h"
#include "SlopeDefaults_Migrate_V2_IO.h"
#include "StratumGenerationSettings_Migrate_V2_IO.h"
#include "EntityCollections_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// A V2-shaped document with no Accumulation key at all gets one reserved, empty.
void CheckReservesAnEmptyAccumulationSection() {
    nlohmann::json document;
    document["mapGeneratorData"]["MapSize"] = 512;

    Io::Accumulation_Migrate_V2(document);

    Check(document.contains("Accumulation") && document["Accumulation"].is_object(),
          "Accumulation_Migrate_V2 reserves a top-level Accumulation object");
    Check(document["Accumulation"].empty(), "the reserved Accumulation object is empty");
    Check(document["mapGeneratorData"]["MapSize"] == 512, "the rest of the document is untouched");
}

// If Accumulation already carries data (e.g. a re-run, or a document some later domain already
// touched), this migration never overwrites it.
void CheckNeverOverwritesAnExistingAccumulationSection() {
    nlohmann::json document;
    document["Accumulation"]["SomeFutureField"] = 42;

    Io::Accumulation_Migrate_V2(document);

    Check(document["Accumulation"]["SomeFutureField"] == 42,
          "an already-present Accumulation section is never overwritten");
}

// Idempotency: calling it a second time changes nothing further.
void CheckIdempotent() {
    nlohmann::json document;
    Io::Accumulation_Migrate_V2(document);
    nlohmann::json before = document;
    Io::Accumulation_Migrate_V2(document);
    Check(document == before, "a second call is a safe no-op");
}

// STEP26A's bLosslessIfSkipped paired test: the step's OLD-shape fixture, every OTHER migration in
// the step run (never Accumulation_Migrate_V2, never legacyKeysToDelete), confirming the resulting
// document is still a valid V3-shaped document with Accumulation absent — nothing was ever there to
// lose by skipping this migration alone.
void CheckSkippingAloneLosesNothingWithSiblingsApplied() {
    nlohmann::json document;
    nlohmann::json& legacy = document["mapGeneratorData"];
    legacy["Seed"]                   = 4242;
    legacy["ScaleFeaturesToMapSize"] = false;
    legacy["TerrainMinHeight"]       = 12.0;
    legacy["WorldUnitsPerCell"]      = 3.5;
    legacy["GlobalSymmetryMask"]     = 5;
    legacy["DetailNormalMapSize"]    = 2048;
    legacy["FlowMapColor"]           = { 0.25, 0.5, 0.75, 1.0 };
    legacy["GlobalIconAlloy"]        = "IconAlloy";
    nlohmann::json stratum;
    stratum["SlopeGateEnabled"] = true;
    stratum["SlopeGateStrength"] = 0.5;
    legacy["Stratums"] = nlohmann::json::array({ stratum });
    legacy["Armies"]["commander"]["Color"] = { 0.25, 0.5, 0.75, 1.0 };

    // Every OTHER migration in the sourceVersion-2 step runs — Accumulation_Migrate_V2 itself never
    // does, and neither does the step's legacyKeysToDelete (matching IO_MIGRATION_SPEC.md §6's
    // partial-application rule: legacyKeysToDelete fires only on full-step application).
    Io::GeneralMapSettings_Migrate_V2(document);
    Io::Symmetry_Migrate_V2(document);
    Io::DetailNormal_Migrate_V2(document);
    Io::Flow_Migrate_V2(document);
    Io::GlobalMarkerSettings_Migrate_V2(document);
    Io::SlopeDefaults_Migrate_V2(document);
    Io::StratumGenerationSettings_Migrate_V2(document);
    Io::EntityCollections_Migrate_V2(document);

    Check(!document.contains("Accumulation"),
          "Accumulation stays absent when its own migration is the one skipped");
    Check(document["GeneralMapSettings"]["Seed"] == 4242
          && document["Symmetry"]["GlobalSymmetryMask"] == 5
          && document["DetailNormal"]["DetailNormalMapSize"] == 2048
          && document["Flow"]["FlowMapColor"]["r"] == 0.25
          && document["GlobalMarkerSettings"]["GlobalIconAlloy"] == "IconAlloy"
          && document["SlopeDefaults"]["slopeGateStrength"] == 0.5
          && document["StratumGenerationSettings"][0]["SlopeGateEnabled"] == true
          && document["armies"]["commander"]["armyColor"]["r"] == 0.25,
          "every sibling migration's own data still lands correctly — this document is otherwise a "
          "valid, complete V3-shaped document, just missing the Accumulation key that had nothing to "
          "carry in the first place");
}

} // namespace

int main() {
    CheckReservesAnEmptyAccumulationSection();
    CheckNeverOverwritesAnExistingAccumulationSection();
    CheckIdempotent();
    CheckSkippingAloneLosesNothingWithSiblingsApplied();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

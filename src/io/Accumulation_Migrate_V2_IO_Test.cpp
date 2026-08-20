// Accumulation_Migrate_V2_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1): confirms
// `Accumulation_Migrate_V2` reserves an empty top-level `Accumulation` object, unconditionally and
// idempotently, and never overwrites one already present.
#include "Accumulation_Migrate_V2_IO.h"
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

} // namespace

int main() {
    CheckReservesAnEmptyAccumulationSection();
    CheckNeverOverwritesAnExistingAccumulationSection();
    CheckIdempotent();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

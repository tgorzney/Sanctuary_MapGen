// MapFormat_TestSupport_IO.h — shared scaffolding for the `.sanmap` importer acceptance test.
// Test-only: the pass/fail counter, the scratch folder every disk check runs in, the fully
// populated fixture recipe the round trip is measured against, and the entry points of the aspect
// test units. Split out so each aspect unit stays inside the ARCH §1.5 ceilings.
//
// The fixture is built from the PARAMS structs directly, never from a document — so a key the
// exporter forgets to write shows up as a mismatch rather than as a matching pair of omissions.
#pragma once
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace MapFormatTest {

inline int& FailureCount() { static int failures = 0; return failures; }

inline void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++FailureCount(); }
}

inline bool NearlyEqual(float left, float right) { return std::fabs(left - right) <= 1.0e-4f; }

// A private, EMPTY folder under the platform temp directory. Cleared on the way in, so a run
// never inherits a previous run's files.
inline std::string ScratchFolderPath(const char* folderName) {
    std::error_code pathError;
    const std::filesystem::path folder = std::filesystem::temp_directory_path(pathError) / folderName;
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

// Every block off its defaults — geometry, water, the layer stack, one stratum and one rule of
// each family. MapImporter_IO_Test.cpp owns the definition.
Params::MapRecipe BuildPopulatedRecipe();

// The three aspect units of the binary whose main() lives in MapImporter_IO_Test.cpp.
void RunRoundTripTests();          // MapImporter_IO_Test.cpp
void RunValidationTests();         // MapImporter_Validation_IO_Test.cpp
void RunBakedFieldTests();         // MapImporter_Fields_IO_Test.cpp

} // namespace MapFormatTest
} // namespace SanmapGen

// FilesTab_TestSupport_UI.h — shared scaffolding for the Files / Save tab acceptance test.
// Test-only: the pass/fail counter, the scratch folder the disk round trip runs in, and the entry
// points of the two aspect units. Split out so each stays inside the ARCH §1.5 ceilings.
#pragma once
#include <cstdio>
#include <filesystem>
#include <string>

namespace SanmapGen {
namespace FilesTabTest {

inline int& FailureCount() { static int failures = 0; return failures; }

inline void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++FailureCount(); }
}

// A private, EMPTY folder under the platform temp directory, cleared on the way in.
inline std::string ScratchFolderPath(const char* folderName) {
    std::error_code pathError;
    const std::filesystem::path folder = std::filesystem::temp_directory_path(pathError) / folderName;
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

void RunTabStateTests();      // FilesTab_UI_Test.cpp — labels, the log budget, the refusals
void RunRoundTripTests();     // FilesTab_Roundtrip_UI_Test.cpp — the injected seam + export/open
void RunScenarioExportTests();   // FilesTab_ScenarioExport_UI_Test.cpp — STEP77

} // namespace FilesTabTest
} // namespace SanmapGen

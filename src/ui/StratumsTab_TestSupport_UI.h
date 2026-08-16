// StratumsTab_TestSupport_UI.h — the pass/fail counter and the borrowed sanpack catalogues shared
// by the tab-rebuild C2 acceptance translation units. Test-only scaffolding, not part of the layer
// graph; nothing outside a *_Test.cpp includes it (the same standing as LayerEditor_TestSupport_UI.h).
#pragma once
#include <cstdio>
#include "StratumsTab_UI.h"

namespace SanmapGen {
namespace Ui {

inline int stratumsTabTestFailureCount = 0;

inline void CheckStratumsTab(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++stratumsTabTestFailureCount;
}

inline int ReportStratumsTabTestResult() {
    if (stratumsTabTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", stratumsTabTestFailureCount);
    return 1;
}

// A stand-in for what a host reads out of a `.sanpack` and lends the tab for one frame.
inline const char* const testEnvironmentLabels[] = { "Tropical", "Desert" };
inline const char* const testMaterialLabels[]    = { "Grass01", "Sand02", "Rock03" };

inline StratumsTabAssetOptions MakeTestAssetOptions() {
    StratumsTabAssetOptions options;
    options.environmentLabels = testEnvironmentLabels;
    options.environmentCount  = 2;
    options.materialLabels    = testMaterialLabels;
    options.materialCount     = 3;
    return options;
}

} // namespace Ui
} // namespace SanmapGen

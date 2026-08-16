// LayerEditor_TestSupport_UI.h — the pass/fail counter shared by the tab-rebuild B acceptance
// translation units. Test-only scaffolding, not part of the layer graph; nothing outside a
// *_Test.cpp includes it (the same standing as ParameterTabs_TestSupport_UI.h).
#pragma once
#include <cstdio>

namespace SanmapGen {
namespace Ui {

inline int layerEditorTestFailureCount = 0;

inline void CheckLayerEditor(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++layerEditorTestFailureCount;
}

inline int ReportLayerEditorTestResult() {
    if (layerEditorTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", layerEditorTestFailureCount);
    return 1;
}

} // namespace Ui
} // namespace SanmapGen

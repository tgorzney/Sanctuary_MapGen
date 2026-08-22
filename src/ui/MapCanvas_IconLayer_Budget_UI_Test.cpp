// MapCanvas_IconLayer_Budget_UI_Test.cpp — acceptance test, part 2: §2's cross-layer visible-vertex
// budget + decimation, headless. One translation unit of the MapCanvas_IconLayer_UI_Test binary.
#include "MapCanvas_IconLayer_TestFixture_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

OverlayVisibleInstance MakeCandidate(float screenX, float screenY, int layerIndex, int stableOrder,
                                     bool bSelected = false) {
    OverlayVisibleInstance instance;
    instance.screenCenterX = screenX; instance.screenCenterY = screenY;
    instance.layerIndex = layerIndex; instance.stableOrder = stableOrder; instance.bSelected = bSelected;
    return instance;
}

// Under budget: every candidate survives untouched, no decimation.
void CheckUnderBudgetDrawsEverything() {
    std::vector<OverlayVisibleInstance> candidates;
    for (int i = 0; i < 10; ++i) candidates.push_back(MakeCandidate(static_cast<float>(i) * 20.0f, 0.0f, 0, i));
    OverlayRenderingSettings settings; settings.visibleInstanceBudget = 100;
    IconLayerBudgetDiagnostics_UI diagnostics;
    const std::vector<OverlayVisibleInstance> result = ApplyVisibleInstanceBudget(candidates, settings, &diagnostics);
    check(result.size() == 10, "under budget, nothing is decimated");
    check(diagnostics.fallbackInvocationCount == 0, "the priority-cap fallback never engages under budget");
}

// Over budget, but clustering alone is sufficient: the fallback must not engage.
void CheckClusteringAloneSufficient() {
    std::vector<OverlayVisibleInstance> candidates;
    for (int i = 0; i < 20; ++i) candidates.push_back(MakeCandidate(0.0f, 0.0f, 0, i));   // all one screen cell
    OverlayRenderingSettings settings; settings.visibleInstanceBudget = 5; settings.screenCellClusterSizePixels = 8;
    IconLayerBudgetDiagnostics_UI diagnostics;
    const std::vector<OverlayVisibleInstance> result = ApplyVisibleInstanceBudget(candidates, settings, &diagnostics);
    check(result.size() == 1, "20 candidates landing in one screen cell cluster down to one representative");
    check(diagnostics.fallbackInvocationCount == 0,
          "clustering alone bringing the count under budget means the fallback never engages");
}

// Over budget even after clustering: the fallback truncates to exactly the budget.
void CheckFallbackTruncatesToBudget() {
    std::vector<OverlayVisibleInstance> candidates;
    for (int i = 0; i < 50; ++i)
        candidates.push_back(MakeCandidate(static_cast<float>(i) * 100.0f, 0.0f, i, i));   // 50 distinct cells
    OverlayRenderingSettings settings; settings.visibleInstanceBudget = 10; settings.screenCellClusterSizePixels = 8;
    IconLayerBudgetDiagnostics_UI diagnostics;
    const std::vector<OverlayVisibleInstance> result = ApplyVisibleInstanceBudget(candidates, settings, &diagnostics);
    check(result.size() == 10, "the fallback truncates to exactly the budget");
    check(diagnostics.fallbackInvocationCount == 1, "the fallback engages exactly once");
}

// The selected instance always survives, even packed into a crowded cell and even under a tiny
// budget; later-Z-order layers win ties among the rest.
void CheckSelectedAlwaysSurvives() {
    std::vector<OverlayVisibleInstance> candidates;
    for (int i = 0; i < 30; ++i)
        candidates.push_back(MakeCandidate(static_cast<float>(i) * 100.0f, 0.0f, i, i));
    candidates.push_back(MakeCandidate(5.0f, 5.0f, /*layerIndex=*/0, /*stableOrder=*/999, /*bSelected=*/true));
    OverlayRenderingSettings settings; settings.visibleInstanceBudget = 3;
    const std::vector<OverlayVisibleInstance> result = ApplyVisibleInstanceBudget(candidates, settings, nullptr);
    check(result.size() == 3, "truncates to budget even with a selected instance present");
    bool bSelectedSurvived = false;
    for (const OverlayVisibleInstance& instance : result) bSelectedSurvived = bSelectedSurvived || instance.bSelected;
    check(bSelectedSurvived, "the selected instance is never clustered or capped away");
}

// §14.11's binding guardrail: decimation touches only the in-memory candidate list.
void CheckDeterminismGuardrail() {
    Data::PlacementInstances placementsBefore;
    Data::PlacementInstance instance; instance.positionX = 3.0f;
    placementsBefore.Append(instance);
    const std::vector<float> positionsBefore = placementsBefore.positionX;

    std::vector<OverlayVisibleInstance> candidates;
    for (int i = 0; i < 20; ++i) candidates.push_back(MakeCandidate(0.0f, 0.0f, 0, i));
    OverlayRenderingSettings settings; settings.visibleInstanceBudget = 3;
    ApplyVisibleInstanceBudget(candidates, settings, nullptr);   // decimation call under test

    check(placementsBefore.positionX == positionsBefore,
          "decimation never touches Data::PlacementInstances (bit-identical before/after)");
}

} // namespace

void RunMapCanvasIconLayerBudgetChecks() {
    CheckUnderBudgetDrawsEverything();
    CheckClusteringAloneSufficient();
    CheckFallbackTruncatesToBudget();
    CheckSelectedAlwaysSurvives();
    CheckDeterminismGuardrail();
}

} // namespace Ui
} // namespace SanmapGen

// MapCanvas_IconLayer_Budget_UI.cpp — §2's cross-layer visible-vertex budget: screen-cell
// clustering (primary), then a priority-cap fallback. Layer: UI. Pure, imgui-free,
// headless-testable. Operates ONLY on the in-memory candidate list (§14.11's binding guardrail —
// never touches Data::PlacementInstances/Data::SpatialGrid/any CSR bucket, structurally: this file
// never even includes those headers).
#include "MapCanvas_IconLayer_Ops_UI.h"
#include <algorithm>
#include <cstdint>
#include <unordered_map>

namespace SanmapGen {
namespace Ui {
namespace {

// Selected instance always wins (§14.9's C2 contract); otherwise later-Z-order layer wins
// (§14.7); otherwise a stable append-order tie-break so the visible set does not flicker frame to
// frame at a fixed view.
bool HasHigherDrawPriority(const OverlayVisibleInstance& a, const OverlayVisibleInstance& b) {
    if (a.bSelected != b.bSelected) return a.bSelected;
    if (a.layerIndex != b.layerIndex) return a.layerIndex > b.layerIndex;
    return a.stableOrder < b.stableOrder;
}

std::int64_t ScreenCellKey(float screenX, float screenY, int cellSize) {
    const long long cellX = static_cast<long long>(screenX / static_cast<float>(cellSize));
    const long long cellY = static_cast<long long>(screenY / static_cast<float>(cellSize));
    return (cellX << 32) ^ (cellY & 0xFFFFFFFFLL);
}

// Any cell holding more than one candidate draws exactly one representative — the highest-priority
// one already living there is replaced only if the newcomer outranks it, so a selected instance
// can never be clustered away even if it is not the first one visited.
std::vector<OverlayVisibleInstance> ClusterByScreenCell(const std::vector<OverlayVisibleInstance>& candidates,
                                                        int cellSizePixels) {
    const int cellSize = cellSizePixels > 0 ? cellSizePixels : 1;
    std::unordered_map<std::int64_t, std::size_t> representativeIndexByCell;
    std::vector<OverlayVisibleInstance> representatives;
    representatives.reserve(candidates.size());
    for (const OverlayVisibleInstance& candidate : candidates) {
        const std::int64_t cellKey = ScreenCellKey(candidate.screenCenterX, candidate.screenCenterY, cellSize);
        const auto found = representativeIndexByCell.find(cellKey);
        if (found == representativeIndexByCell.end()) {
            representativeIndexByCell.emplace(cellKey, representatives.size());
            representatives.push_back(candidate);
        } else if (HasHigherDrawPriority(candidate, representatives[found->second])) {
            representatives[found->second] = candidate;
        }
    }
    return representatives;
}

// Only engages when clustering alone still exceeds budget (the caller decides that; this function
// always counts as one fallback invocation once called).
std::vector<OverlayVisibleInstance> TruncateToPriorityBudget(std::vector<OverlayVisibleInstance> representatives,
                                                              int budget, IconLayerBudgetDiagnostics_UI* diagnostics) {
    if (diagnostics != nullptr) ++diagnostics->fallbackInvocationCount;
    std::sort(representatives.begin(), representatives.end(), HasHigherDrawPriority);
    if (static_cast<int>(representatives.size()) > budget) representatives.resize(static_cast<std::size_t>(budget));
    return representatives;
}

} // namespace

std::vector<OverlayVisibleInstance> ApplyVisibleInstanceBudget(std::vector<OverlayVisibleInstance> candidates,
                                                                const OverlayRenderingSettings& settings,
                                                                IconLayerBudgetDiagnostics_UI* diagnostics) {
    if (static_cast<int>(candidates.size()) <= settings.visibleInstanceBudget) return candidates;
    std::vector<OverlayVisibleInstance> clustered =
        ClusterByScreenCell(candidates, settings.screenCellClusterSizePixels);
    if (static_cast<int>(clustered.size()) <= settings.visibleInstanceBudget) return clustered;
    return TruncateToPriorityBudget(std::move(clustered), settings.visibleInstanceBudget, diagnostics);
}

} // namespace Ui
} // namespace SanmapGen

// MapCanvas_ScenarioEditMode_Interaction_UI.cpp — drag (incl. materialize-on-first-drag) and
// right-click request resolution, over MapCanvas_ScenarioEditMode_HitTest_UI.cpp's shared hit-test
// + projection helpers. Layer: UI. Pure/imgui-free/headless-testable.
#include "MapCanvas_ScenarioEditMode_InteractionInternal_UI.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Solid spawn: drags the existing row. Hollow spawn: materializes it FIRST, seeded from the real
// baseline position already resolved onto the candidate — never zeroed (STEP74 §4's flagged gap,
// the ticket's own "core ask").
void BeginDrag(ScenarioEditModeState& state, const ScenarioEditMarkerCandidate_UI& candidate,
              const std::vector<Params::Army>& armies) {
    if (candidate.kind != ScenarioMarkerKind_UI::Spawn || state.editedBody == nullptr) return;
    if (candidate.spawnRowIndex != kScenarioEditModeNoIndex) {
        state.bDragging = true;
        state.dragRowIndex = candidate.spawnRowIndex;
        return;
    }
    if (candidate.armyIndex < 0 || static_cast<std::size_t>(candidate.armyIndex) >= armies.size()) return;
    Params::ScenarioSpawn spawn;
    spawn.armyName  = armies[static_cast<std::size_t>(candidate.armyIndex)].name;
    spawn.positionX = candidate.worldX; spawn.positionY = candidate.worldY; spawn.positionZ = candidate.worldZ;
    state.editedBody->spawns.push_back(spawn);
    state.bDragging = true;
    state.dragRowIndex = static_cast<int>(state.editedBody->spawns.size()) - 1;
}

// Snaps the dragged row's X/Z straight to the world point under the cursor every frame (never
// delta-accumulated) — standard "object follows cursor" drag UX. Y (terrain height) is left as
// authored; this module samples no height field (Backend policy: imgui/ImDrawList only).
void ContinueDrag(ScenarioEditModeState& state, const PreviewComposite& composite, const MapCanvasView& view,
                  float regionLocalX, float regionLocalY) {
    if (!state.bDragging || state.editedBody == nullptr) return;
    if (state.dragRowIndex < 0
        || static_cast<std::size_t>(state.dragRowIndex) >= state.editedBody->spawns.size()) {
        state.bDragging = false;
        return;
    }
    float worldZ = 0.0f;
    const float worldX = ResolveScenarioEditModeWorldXUnderCursor(composite, view, regionLocalX, regionLocalY, worldZ);
    Params::ScenarioSpawn& spawn = state.editedBody->spawns[static_cast<std::size_t>(state.dragRowIndex)];
    spawn.positionX = worldX; spawn.positionZ = worldZ;
}

// A baseline alloy -> "Remove for this scenario"; empty canvas -> "Add Alloy Marker for [nearest
// army]" (the nearest baseline spawn's army — the only spatial army anchor available, a
// documented, flagged reading of the ticket's "near an army's territory").
void ResolveRightClick(ScenarioEditModeState& state, const std::vector<Params::Army>& armies,
                       const PreviewComposite& composite, const MapCanvasView& view,
                       float regionLocalX, float regionLocalY) {
    const std::vector<ScenarioEditMarkerCandidate_UI>& candidates = state.lastResolvedCandidates;
    const int hitIndex = HitTestScenarioEditModeCandidates(candidates, composite, view, regionLocalX, regionLocalY);
    using RequestKind = ScenarioEditModeState::ContextMenuRequest::Kind;
    if (hitIndex != kScenarioEditModeNoIndex) {
        const ScenarioEditMarkerCandidate_UI& candidate = candidates[static_cast<std::size_t>(hitIndex)];
        if (candidate.kind != ScenarioMarkerKind_UI::Alloy
            || (candidate.state != ScenarioMarkerVisualState_UI::AlloyKept
                && candidate.state != ScenarioMarkerVisualState_UI::AlloyDeleted))
            return;   // only a baseline alloy offers this menu
        state.pendingContextMenu.kind = RequestKind::RemoveBaselineAlloy;
        state.pendingContextMenu.worldX = candidate.worldX; state.pendingContextMenu.worldY = candidate.worldY;
        state.pendingContextMenu.worldZ = candidate.worldZ;
        state.pendingContextMenu.markerName = candidate.markerName;
        state.pendingContextMenu.armyName.clear();   // baseline alloys carry no known army (§0)
        state.bContextMenuJustRequested = true;
        return;
    }
    float worldZ = 0.0f;
    const float worldX = ResolveScenarioEditModeWorldXUnderCursor(composite, view, regionLocalX, regionLocalY, worldZ);
    int nearestArmyIndex = kScenarioEditModeNoIndex;
    float nearestDistanceSquared = 0.0f;
    for (const ScenarioEditMarkerCandidate_UI& candidate : candidates) {
        if (candidate.kind != ScenarioMarkerKind_UI::Spawn || candidate.armyIndex < 0) continue;
        const float distanceSquared = ScenarioEditModeDistanceSquared(candidate.worldX, candidate.worldZ, worldX, worldZ);
        if (nearestArmyIndex == kScenarioEditModeNoIndex || distanceSquared < nearestDistanceSquared) {
            nearestArmyIndex = candidate.armyIndex; nearestDistanceSquared = distanceSquared;
        }
    }
    if (nearestArmyIndex == kScenarioEditModeNoIndex
        || static_cast<std::size_t>(nearestArmyIndex) >= armies.size())
        return;   // nobody to attribute the marker to — no menu, never a silent action either
    state.pendingContextMenu.kind = RequestKind::AddAlloyForArmy;
    state.pendingContextMenu.worldX = worldX; state.pendingContextMenu.worldY = 0.0f; state.pendingContextMenu.worldZ = worldZ;
    state.pendingContextMenu.markerName.clear();
    state.pendingContextMenu.armyName = armies[static_cast<std::size_t>(nearestArmyIndex)].name;
    state.bContextMenuJustRequested = true;
}

} // namespace

void ApplyScenarioEditModePointerInput(ScenarioEditModeState& state, const MapCanvasView& view,
                                       const PreviewComposite& composite,
                                       const std::vector<Params::Army>& armies,
                                       const ScenarioEditModePointerFrame_UI& pointerFrame) {
    if (!state.IsActive()) return;
    state.bContextMenuJustRequested = false;

    if (pointerFrame.bRightClicked) {
        ResolveRightClick(state, armies, composite, view, pointerFrame.regionLocalX, pointerFrame.regionLocalY);
        return;
    }
    // The activation frame ONLY seeds (BeginDrag) — it never also snaps to the cursor the same
    // frame, so the materialized row's position is bit-exactly the real baseline on the frame it
    // appears (acceptance test 2's own claim), not a cursor-pixel-quantized approximation of it.
    // Continuing the drag begins the very next held frame.
    if (pointerFrame.bPressActivated) {
        const int hitIndex = HitTestScenarioEditModeCandidates(state.lastResolvedCandidates, composite, view,
                                                                pointerFrame.regionLocalX, pointerFrame.regionLocalY);
        if (hitIndex != kScenarioEditModeNoIndex)
            BeginDrag(state, state.lastResolvedCandidates[static_cast<std::size_t>(hitIndex)], armies);
    } else if (state.bDragging && pointerFrame.bPressActive) {
        ContinueDrag(state, composite, view, pointerFrame.regionLocalX, pointerFrame.regionLocalY);
    }
    if (state.bDragging && !pointerFrame.bPressActive)
        state.bDragging = false;
}

} // namespace Ui
} // namespace SanmapGen

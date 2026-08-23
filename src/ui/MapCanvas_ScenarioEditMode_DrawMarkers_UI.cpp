// MapCanvas_ScenarioEditMode_DrawMarkers_UI.cpp — resolves this frame's candidates and draws each:
// the atlas icon quad underneath (when a pairing/atlas source resolves one) plus the state
// decoration that makes the six states visually distinct even with no atlas content wired
// (Backend policy: `ImDrawList::AddImage` — itself PrimRectUV/PrimReserve+PrimWriteVtx under the
// hood, the SAME §14.9 primitive FlushIconLayerBucket uses, not a second backend; one call per
// marker is entirely proportionate at this module's tens-of-instances cardinality, unlike STEP53's
// own 400k-instance budget that motivates ITS hand-batched bucketing). Layer: UI.
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include "IconAtlasPairing_UI.h"
#include "IconGridWidget_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/Army_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// Constitution §8 — a named on-screen size, never a literal; matches MapCanvas's own
// pickRadiusScreenPixels default posture.
constexpr float kScenarioEditModeIconRadiusScreenPixels = 8.0f;

ImU32 NeutralTint() { return IM_COL32(220, 220, 220, 255); }
ImU32 GreyTint()    { return IM_COL32(120, 120, 120, 255); }

ImU32 ArmyTint(const std::vector<Params::Army>* armies, int armyIndex, ImU32 fallback) {
    if (armies == nullptr || armyIndex < 0 || static_cast<std::size_t>(armyIndex) >= armies->size())
        return fallback;
    const float* color = (*armies)[static_cast<std::size_t>(armyIndex)].armyColor;
    return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
}

ImVec2 ProjectCandidateToScreen(const ScenarioEditMarkerCandidate_UI& candidate, const PreviewComposite& composite,
                                const MapCanvasView& view, float regionOriginX, float regionOriginY) {
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(candidate.worldX, candidate.worldZ);
    const RegionLocalPoint regionLocal = view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOriginX + regionLocal.regionLocalX, regionOriginY + regionLocal.regionLocalY);
}

void DrawAtlasIconQuad(ImDrawList& drawList, const ScenarioEditModeDrawInput& input,
                       const ScenarioEditMarkerCandidate_UI& candidate, ImVec2 screenCenter, ImU32 tint) {
    if (input.pairingLookup == nullptr || input.atlasManifest == nullptr || candidate.templateIdentifier.empty())
        return;
    const IconIdentifierPairing pairing = input.pairingLookup->Resolve(candidate.templateIdentifier);
    if (pairing.thumbnailIconId == kInvalidIconId || pairing.thumbnailIconId >= input.atlasManifest->EntryCount())
        return;   // no atlas content — the decoration below still carries the state on its own
    const IconAtlasEntry& entry = input.atlasManifest->entries[static_cast<std::size_t>(pairing.thumbnailIconId)];
    const float half = kScenarioEditModeIconRadiusScreenPixels;
    drawList.AddImage(static_cast<ImTextureID>(input.atlasManifest->PageTextureIdentifier(entry.atlasPage)),
                      ImVec2(screenCenter.x - half, screenCenter.y - half),
                      ImVec2(screenCenter.x + half, screenCenter.y + half),
                      ImVec2(entry.uvMinimumX, entry.uvMinimumY), ImVec2(entry.uvMaximumX, entry.uvMaximumY), tint);
}

// The six states, made distinct by fill+outline+glyph combination, never color alone (the ticket's
// own "never rely on identical screen position" caution, applied one step further).
void DrawStateDecoration(ImDrawList& drawList, ScenarioMarkerVisualState_UI state, ImVec2 screenCenter, ImU32 tint) {
    const float radius = kScenarioEditModeIconRadiusScreenPixels;
    switch (state) {
        case ScenarioMarkerVisualState_UI::SpawnNoOverride:
            drawList.AddCircle(screenCenter, radius, tint, 0, 2.0f);
            drawList.AddText(ImVec2(screenCenter.x - 4.0f, screenCenter.y - radius - 14.0f), IM_COL32(255, 200, 0, 255), "!");
            break;
        case ScenarioMarkerVisualState_UI::SpawnExplicit:
            drawList.AddCircleFilled(screenCenter, radius, tint);
            break;
        case ScenarioMarkerVisualState_UI::AlloyKept:
            drawList.AddCircleFilled(screenCenter, radius, tint);
            break;
        case ScenarioMarkerVisualState_UI::AlloyDeleted:
            drawList.AddCircleFilled(screenCenter, radius, tint);
            drawList.AddLine(ImVec2(screenCenter.x - radius, screenCenter.y), ImVec2(screenCenter.x + radius, screenCenter.y),
                             IM_COL32(30, 30, 30, 255), 2.0f);
            break;
        case ScenarioMarkerVisualState_UI::AlloyAdded:
            drawList.AddCircleFilled(screenCenter, radius, tint);
            drawList.AddText(ImVec2(screenCenter.x - 4.0f, screenCenter.y - radius - 14.0f), IM_COL32(80, 220, 80, 255), "+");
            break;
        case ScenarioMarkerVisualState_UI::AlloyRemovedGhost:
            drawList.AddCircle(screenCenter, radius, tint, 0, 1.5f);
            drawList.AddLine(ImVec2(screenCenter.x - radius, screenCenter.y - radius),
                             ImVec2(screenCenter.x + radius, screenCenter.y + radius), IM_COL32(220, 30, 30, 220), 2.0f);
            drawList.AddLine(ImVec2(screenCenter.x - radius, screenCenter.y + radius),
                             ImVec2(screenCenter.x + radius, screenCenter.y - radius), IM_COL32(220, 30, 30, 220), 2.0f);
            break;
    }
}

ImU32 ResolveCandidateTint(const ScenarioEditMarkerCandidate_UI& candidate, const std::vector<Params::Army>* armies) {
    switch (candidate.state) {
        case ScenarioMarkerVisualState_UI::SpawnNoOverride:   return GreyTint();
        case ScenarioMarkerVisualState_UI::SpawnExplicit:     return ArmyTint(armies, candidate.armyIndex, NeutralTint());
        case ScenarioMarkerVisualState_UI::AlloyKept:         return NeutralTint();
        case ScenarioMarkerVisualState_UI::AlloyDeleted:      return GreyTint();
        case ScenarioMarkerVisualState_UI::AlloyAdded:        return ArmyTint(armies, candidate.armyIndex, NeutralTint());
        case ScenarioMarkerVisualState_UI::AlloyRemovedGhost: return IM_COL32(200, 80, 80, 90);   // ghost = low alpha
    }
    return NeutralTint();
}

} // namespace

void DrawScenarioEditModeOverlay(ScenarioEditModeState& state, const ScenarioEditModeDrawInput& input,
                                 ImDrawList& drawList) {
    if (!state.IsActive() || input.composite == nullptr || input.view == nullptr) {
        state.lastResolvedCandidates.clear();
        return;
    }
    ResolveScenarioEditModeCandidates(input.resolveInput, *state.editedBody, state.previewAsSlotPattern,
                                      state.lastResolvedCandidates);
    for (const ScenarioEditMarkerCandidate_UI& candidate : state.lastResolvedCandidates) {
        const ImVec2 screenCenter =
            ProjectCandidateToScreen(candidate, *input.composite, *input.view, input.regionOriginX, input.regionOriginY);
        const ImU32 tint = ResolveCandidateTint(candidate, input.resolveInput.armies);
        DrawAtlasIconQuad(drawList, input, candidate, screenCenter, tint);
        DrawStateDecoration(drawList, candidate.state, screenCenter, tint);
    }
}

} // namespace Ui
} // namespace SanmapGen

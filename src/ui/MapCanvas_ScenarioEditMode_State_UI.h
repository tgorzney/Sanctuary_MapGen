// MapCanvas_ScenarioEditMode_State_UI.h — ScenarioEditModeState, split out of
// MapCanvas_ScenarioEditMode_UI.h (Constitution §1.5 ceiling): the one long-lived, cross-frame
// object Application owns (mirroring MapCanvas/OverlayLayerSettings's own single-instance-on-
// Application posture) and injects into MapCanvas + the Scenarios tab's detail panel.
#pragma once
#include "MapCanvas_ScenarioEditMode_UI.h"

namespace SanmapGen {
namespace Ui {

struct ScenarioEditModeState {
    bool                  bActive    = false;
    Params::ScenarioBody* editedBody = nullptr;         // non-owning
    std::string           previewAsSlotPattern;         // UI-session scratch, never serialized

    // Drag ephemeral (one gesture's worth) state.
    bool bDragging    = false;
    int  dragRowIndex = kScenarioEditModeNoIndex;      // body.spawns index once resolved/seeded

    // This frame's already-resolved candidates — ResolveScenarioEditModeCandidates's own comment.
    std::vector<ScenarioEditMarkerCandidate_UI> lastResolvedCandidates;

    // A pending right-click action, set by the interaction pass, drawn/committed by the chrome
    // pass's popup — see MapCanvas_ScenarioEditMode_Ops_UI.h's own note on why this is data handed
    // across rather than a callback. `bContextMenuJustRequested` is a one-frame "open it now" edge,
    // consumed and reset by the popup draw call the same frame it is set.
    struct ContextMenuRequest {
        enum class Kind : int { None, RemoveBaselineAlloy, AddAlloyForArmy };
        Kind kind = Kind::None;
        float worldX = 0.0f, worldY = 0.0f, worldZ = 0.0f;
        std::string markerName;   // RemoveBaselineAlloy only
        std::string armyName;     // both kinds
    } pendingContextMenu;
    bool bContextMenuJustRequested = false;

    bool IsActive() const { return bActive && editedBody != nullptr; }
    // patternSlotPattern non-null = Tier 1 (defaults to the pattern verbatim); else Tier 2/3,
    // synthesized against `countConditions` (null/empty = "always matches", Tier 3's own posture).
    // _PreviewAs_UI.cpp.
    void Activate(Params::ScenarioBody& body, const std::string* patternSlotPattern,
                 const std::vector<Params::ScenarioCountCondition>* countConditions,
                 int maxArmySlotCount);
    void Deactivate() {
        bActive = false; editedBody = nullptr; bDragging = false;
        dragRowIndex = kScenarioEditModeNoIndex; lastResolvedCandidates.clear();
        pendingContextMenu = ContextMenuRequest(); bContextMenuJustRequested = false;
    }
};

} // namespace Ui
} // namespace SanmapGen

// ArmiesTab_RowLayout_UI.h — one army's own settings, drawn as a single compact row. Layer: UI.
// STEP250. Split out of ArmiesTab_UI.h/.cpp (ARCH §1.5 size ceiling) the same way
// ArmiesTab_Mirror_UI.h/ArmiesTab_Units_UI.h already are aspect siblings of the same tab.
#pragma once
#include <vector>
#include "ArmiesTab_UI.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// One row's own fields (SCOPE NOTE 1: none of it notifies the driver). STEP76: "Name" binds
// `displayName`, never machine-owned `name` (ruling 2). STEP75: also draws the mirror-onto-next-
// army button (ruling 1); its confirm dialog is drawn separately by DrawArmiesTab so it stays
// reachable on a frame this function does not run (DrawPendingDeleteRuleLayerDialog's pattern).
// STEP110: called once per EXPANDED row (armyIndex is that row's own index, not necessarily
// `state.selectedArmyIndex`) rather than once at the bottom for whatever was selected. The Mirror
// button is safe to draw unconditionally per row, unlike LayerEditor_Group_UI's import-path picker
// (STEP104): it owns no persisted per-frame edit buffer, only a single click that stamps
// `armyIndex` into the shared pending-mirror state and opens the (still singly-drawn) confirm
// dialog, so two expanded rows can never fight over a live text edit the way a picker could.
// STEP250: all of the above now draws on ONE line via SameLine() — see STEP250's ticket for the
// per-control ruling that settled the order, the removed Alias box, and the removed RT toggles.
void DrawArmySettings(std::vector<Params::Army>& armies, int armyIndex, ArmiesTabState& state);

} // namespace Ui
} // namespace SanmapGen

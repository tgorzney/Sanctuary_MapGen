// ArmiesTab_Mirror_UI.h — the "mirror this army onto its successor" one-time duplicate action.
// Layer: UI (+ PIPELINE passthrough). STEP75. Split out of ArmiesTab_UI.h/.cpp (ARCH SS1.5 size
// ceiling) the same way ArmiesTab_Units_UI.h already is an aspect sibling of the same tab.
//
// STEP76 amendment (binding): this touches ONLY `groups` -- never `Army::name` (machine-owned
// ARMY_XX identity, minted only by AssignArmyIdentities) or `Army::displayName` for either army.
// `sourceIndex + 1` already has a roster row and therefore an already-correct identity, unchanged
// by this operation; nothing here adds, deletes or reorders an army, so no re-mint is needed.
#pragma once
#include <vector>
#include "ConfirmDialog_UI.h"
#include "../params/Army_PARAMS.h"
#include "../params/Geometry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Even-0-indexed (an odd-numbered army, Army1/3/5...) AND a successor row exists in `armies` --
// the two gating conditions ruling 1 asks `DrawArmySettings` to check before showing the button.
inline bool CanMirrorArmy(const std::vector<Params::Army>& armies, int sourceIndex) {
    if (sourceIndex < 0 || (sourceIndex % 2) != 0) return false;
    return sourceIndex + 1 < static_cast<int>(armies.size());
}

// Recurses one UnitGroup subtree in place: every leaf UnitTransform's position is mirrored 180
// degrees about map center (STEP68's BuildWorldSymmetryOrbit, RotateHalfTurn) and its orientation
// composed with a 180-degree yaw (ApplyHalfTurnYaw); both `units` and `groups` are walked, since a
// group may nest child groups.
void MirrorUnitGroupTree(Params::UnitGroup& group, const Params::Geometry& geometry);

// Pure. Overwrites `armies[sourceIndex + 1]`'s `groups` with a mirrored copy of
// `armies[sourceIndex]`'s `groups` -- a no-op (not a crash) when `sourceIndex` has no successor
// row, so a caller need not re-check CanMirrorArmy defensively. Touches `groups` only; see the
// STEP76 amendment above.
void MirrorArmyGroupsOntoNextArmy(std::vector<Params::Army>& armies, int sourceIndex,
                                  const Params::Geometry& geometry);

// The per-army button (ruling 1): visible only when CanMirrorArmy(armies, selectedArmyIndex).
// Clicking it only RECORDS the request (`outPendingMirrorSourceArmyIndex`, `outConfirmDialogState.
// bOpenRequested`) -- the destructive mutation waits for DrawPendingMirrorArmyConfirmDialog's
// confirm (ruling 3).
void DrawMirrorArmyButton(const std::vector<Params::Army>& armies, int selectedArmyIndex,
                          int& outPendingMirrorSourceArmyIndex,
                          ConfirmDialogState& outConfirmDialogState);

// Drawn every frame a mirror might still be pending (the MarkersTab_RuleLayerSettings_UI.cpp
// DrawPendingDeleteRuleLayerDialog pattern), so its own popup gets to run every frame it may be
// open. The pending index is RE-VALIDATED against `armies`' CURRENT shape before it is trusted
// (Constitution SS6). Reports whether `armies` moved (drives NO preview notify -- ruling 5).
bool DrawPendingMirrorArmyConfirmDialog(std::vector<Params::Army>& armies,
                                        int& pendingMirrorSourceArmyIndex,
                                        ConfirmDialogState& confirmDialogState,
                                        const Params::Geometry& geometry);

} // namespace Ui
} // namespace SanmapGen

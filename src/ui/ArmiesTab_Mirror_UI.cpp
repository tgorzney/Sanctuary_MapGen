// ArmiesTab_Mirror_UI.cpp — see ArmiesTab_Mirror_UI.h for the full rationale and the STEP76
// amendment this must respect (touch `groups` only, never `name`/`displayName`).
#include "ArmiesTab_Mirror_UI.h"
#include "../params/Symmetry_PARAMS.h"
#include "../pipeline/SymmetryOrbitQuery_PIPELINE.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The dialog's own display label for an army -- a local, dialog-only fallback so this file needs
// no reverse dependency on ArmiesTab_UI.h's ArmyRowLabel (kept a leaf aspect, like
// ArmiesTab_Units_UI.cpp already is).
const char* MirrorDialogArmyLabel(const Params::Army& army) {
    if (!army.displayName.empty()) return army.displayName.c_str();
    return army.name.empty() ? "this army" : army.name.c_str();
}

} // namespace

void MirrorUnitGroupTree(Params::UnitGroup& group, const Params::Geometry& geometry) {
    for (Params::UnitTransform& unit : group.units) {
        Pipeline::WorldSymmetryOrbitPoint orbitPoints[2];
        const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(
            geometry, Params::SymmetryAxis::RotateHalfTurn, 0, unit.positionX, unit.positionZ,
            orbitPoints, 2);
        // orbitCount is 1 only when the source sits exactly on map center (its own mirror); either
        // way, the last written point is the correct mirrored position.
        const Pipeline::WorldSymmetryOrbitPoint& mirrored = orbitPoints[orbitCount - 1];
        unit.positionX = mirrored.worldPositionX;
        unit.positionZ = mirrored.worldPositionZ;   // positionY (elevation) untouched by a yaw

        float mirroredRotationX, mirroredRotationY, mirroredRotationZ, mirroredRotationW;
        Pipeline::ApplyHalfTurnYaw(unit.rotationX, unit.rotationY, unit.rotationZ, unit.rotationW,
                                   mirroredRotationX, mirroredRotationY, mirroredRotationZ,
                                   mirroredRotationW);
        unit.rotationX = mirroredRotationX;
        unit.rotationY = mirroredRotationY;
        unit.rotationZ = mirroredRotationZ;
        unit.rotationW = mirroredRotationW;
    }
    for (Params::UnitGroup& childGroup : group.groups) MirrorUnitGroupTree(childGroup, geometry);
}

void MirrorArmyGroupsOntoNextArmy(std::vector<Params::Army>& armies, int sourceIndex,
                                  const Params::Geometry& geometry) {
    if (sourceIndex < 0 || sourceIndex + 1 >= static_cast<int>(armies.size())) return;
    Params::Army& targetArmy = armies[static_cast<std::size_t>(sourceIndex + 1)];
    // STEP76: `groups` only -- `targetArmy.name`/`displayName` are never read or written here.
    targetArmy.groups = armies[static_cast<std::size_t>(sourceIndex)].groups;
    for (Params::UnitGroup& group : targetArmy.groups) MirrorUnitGroupTree(group, geometry);
}

void DrawMirrorArmyButton(const std::vector<Params::Army>& armies, int selectedArmyIndex,
                          int& outPendingMirrorSourceArmyIndex,
                          ConfirmDialogState& outConfirmDialogState) {
    if (!CanMirrorArmy(armies, selectedArmyIndex)) return;
    if (ImGui::Button("Mirror Onto Next Army")) {
        outPendingMirrorSourceArmyIndex = selectedArmyIndex;
        outConfirmDialogState.bOpenRequested = true;
    }
}

bool DrawPendingMirrorArmyConfirmDialog(std::vector<Params::Army>& armies,
                                        int& pendingMirrorSourceArmyIndex,
                                        ConfirmDialogState& confirmDialogState,
                                        const Params::Geometry& geometry) {
    if (pendingMirrorSourceArmyIndex < 0) return false;
    if (!CanMirrorArmy(armies, pendingMirrorSourceArmyIndex)) {
        pendingMirrorSourceArmyIndex = -1;   // the roster moved since the click -- reject, not trust
        return false;
    }
    const Params::Army& sourceArmy = armies[static_cast<std::size_t>(pendingMirrorSourceArmyIndex)];
    const Params::Army& targetArmy = armies[static_cast<std::size_t>(pendingMirrorSourceArmyIndex + 1)];
    char body[192];
    std::snprintf(body, sizeof(body),
                 "Mirror \"%s\"'s units onto \"%s\"? This replaces \"%s\"'s existing units.",
                 MirrorDialogArmyLabel(sourceArmy), MirrorDialogArmyLabel(targetArmy),
                 MirrorDialogArmyLabel(targetArmy));
    ConfirmDialogOptions options;
    options.title    = "Mirror Army Units";
    options.bodyText = body;
    const ConfirmDialogChange change =
        DrawConfirmDialog("mirrorArmyConfirm", confirmDialogState, options);
    if (change.bSecondaryClicked) { pendingMirrorSourceArmyIndex = -1; return false; }
    if (!change.bPrimaryClicked) return false;
    MirrorArmyGroupsOntoNextArmy(armies, pendingMirrorSourceArmyIndex, geometry);
    pendingMirrorSourceArmyIndex = -1;
    return true;
}

} // namespace Ui
} // namespace SanmapGen

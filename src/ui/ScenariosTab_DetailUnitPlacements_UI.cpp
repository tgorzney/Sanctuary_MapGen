// ScenariosTab_DetailUnitPlacements_UI.cpp — STEP261's list editor for `ScenarioBody::unitPlacements`
// (`Params::ScenarioUnitPlacement`, STEP260 / ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md
// §15.14). Layer: UI. Split out of ScenariosTab_Detail_UI.cpp for the ARCH §1.5 file-size ceiling,
// same posture as ScenariosTab_DetailSpawns_UI.cpp/ScenariosTab_DetailAlloys_UI.cpp — called only by
// DrawScenarioBodyFields there.
//
// Army/Template/Position mirror the existing spawn/alloy row idiom verbatim: `DrawArmyNameField` is
// reused unchanged (declared in ScenariosTab_UI.h, defined in ScenariosTab_DetailSpawns_UI.cpp), and
// `templateIdentifier` is a free-text field matching `ScenarioAlloyOverride::markerName`'s own
// honest "NOT a validated picker" posture — no unit-template list exists anywhere in this codebase
// to validate against (STEP261 §1, do not fabricate one).
//
// Rotation: no existing UI in this codebase decomposes an arbitrary quaternion to euler for display
// (STEP261's own work order, grep-confirmed across src/ui) — the only quaternion-adjacent UI logic
// (MarkersTab_BundleNodeBody_UI.cpp:49, the bundle-rotate tool) only ever BUILDS a quaternion FROM a
// yaw-degrees float via Math::YawQuaternion, never the reverse. This file follows that same one-way
// direction rather than inventing decomposition: the yaw slider's own current degrees value lives in
// ImGui's own per-widget scratch storage (`ImGui::GetStateStorage()`, the same mechanism this
// codebase already reaches for transient UI-only state, e.g.
// LayerEditor_InlineSettings_UI_Test.cpp:57's CollapsingHeader open-state poke) — it is NOT derived
// from `rotationY`/`rotationW` and NOT a stored PARAMS field (ARCH_15_14 explicitly rejects a stored
// `facingDegrees` float, STEP261 §1). A placement whose yaw-only rotation was authored by something
// OTHER than this slider (e.g. a future foreign-file importer, STEP264) therefore shows the slider
// at 0 until it is touched in THIS session — an accepted tradeoff of "view/write convenience, not a
// live readout" over inventing real quaternion decomposition, per the work order's own instruction.
// The Advanced fields underneath are the raw, always-authoritative `rotationX/Y/Z/W` storage; the
// yaw slider only ever writes them, and only on its own explicit interaction (never merely because a
// frame rendered) — so an Advanced hand-edited non-yaw-only value is never silently clobbered.
#include "ScenariosTab_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "../math/RigidTransformPivot_MATH.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

// --- pure logic, external linkage, no imgui --------------------------------------------------
// Forward-declared directly by ScenariosTab_DetailUnitPlacements_UI_Test.cpp rather than added to
// ScenariosTab_UI.h — this ticket's own "Files touched" list adds exactly one new declaration there
// (DrawScenarioUnitPlacementsSection). Mirrors MarkersTab_BundleNodeBody_UI.cpp's own
// ApplyMarkerLayerBundleMove/Rotation split: pure, headless-testable functions living beside the
// draw code that calls them, with no header of their own.

// Add-row: a fresh identity-rotation, zero-position, empty-army/template row (STEP261 §3 test 1).
void AppendDefaultScenarioUnitPlacement(std::vector<Params::ScenarioUnitPlacement>& placements) {
    placements.push_back(Params::ScenarioUnitPlacement());
}

// The pure-yaw authoring path (ARCH_15_14 §15.14): the same Math::YawQuaternion call the
// bundle-rotate tool uses. X/Z are zeroed UNCONDITIONALLY — this function IS the pure-yaw path; a
// placement that must keep nonzero X/Z only ever gets there through the Advanced fields, never
// through this one (STEP261 §3 test 2).
void ApplyScenarioUnitPlacementYawDegrees(Params::ScenarioUnitPlacement& placement, float yawDegrees) {
    constexpr float kUnitPlacementYawPi = 3.14159265358979323846f;   // per-file local literal —
                                                                     // established convention
                                                                     // (ScenariosTab_Detail_UI.cpp's
                                                                     // kErrorTextColor, MarkersTab_
                                                                     // BundleNodeBody_UI.cpp's
                                                                     // kBundleRotatePi)
    const float angleRadians = yawDegrees * (kUnitPlacementYawPi / 180.0f);
    float yawX, yawY, yawZ, yawW;
    Math::YawQuaternion(angleRadians, yawX, yawY, yawZ, yawW);
    placement.rotationX = yawX;
    placement.rotationY = yawY;
    placement.rotationZ = yawZ;
    placement.rotationW = yawW;
}

// The exact gate the draw loop below applies — extracted so a headless test can drive "a frame with
// no slider interaction leaves the stored quaternion untouched" without an imgui frame (STEP261 §3
// test 3: an Advanced hand-edited non-yaw-only value must survive verbatim on the next frame).
void ApplyScenarioUnitPlacementYawIfChanged(Params::ScenarioUnitPlacement& placement, float yawDegrees,
                                            bool bSliderValueChanged) {
    if (bSliderValueChanged) ApplyScenarioUnitPlacementYawDegrees(placement, yawDegrees);
}

namespace {

// Duplicated 1-line range constant — same established "each file owns its own copy" precedent as
// ScenariosTab_DetailAlloys_UI.cpp's own ExtendedFieldsWorldPositionRange (ScenariosTab_Detail_UI.cpp's
// ScenarioWorldPositionRange lives in a DIFFERENT translation unit's anonymous namespace, not visible
// here).
ScalarSliderRange UnitPlacementWorldPositionRange() { return ScalarSliderRange{ -8192.0f, 8192.0f, 0.0f }; }

// One placement row: Army / Template / Position X-Y-Z / Rotation (yaw slider + Advanced raw fields).
void DrawScenarioUnitPlacementRow(Params::ScenarioUnitPlacement& placement,
                                  const std::vector<Params::Army>& armies) {
    DrawArmyNameField("Army", placement.armyName, armies);
    TextInputRules templateRules; templateRules.bAllowEmpty = true; templateRules.maximumLength = 48;
    DrawTextInput("Template Identifier", placement.templateIdentifier, templateRules);

    const ScalarSliderRange positionRange = UnitPlacementWorldPositionRange();
    RealtimeToggle xToggle, yToggle, zToggle;
    ImGui::SetNextItemWidth(90.0f);
    DrawSliderScalar("X", placement.positionX, positionRange, xToggle, WidgetStyle(), "%.1f");
    ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
    DrawSliderScalar("Y", placement.positionY, positionRange, yToggle, WidgetStyle(), "%.1f");
    ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
    DrawSliderScalar("Z", placement.positionZ, positionRange, zToggle, WidgetStyle(), "%.1f");

    // Facing: a yaw-only "view/write convenience" (see file header comment) — scratch ImGui
    // per-row storage, never derived from the stored quaternion.
    float* yawDegreesScratch = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("##yawDegreesScratch"), 0.0f);
    RealtimeToggle yawToggle;
    const ScalarSliderRange yawRange{ 0.0f, 360.0f, 0.0f };
    const WidgetChange yawChange = DrawSliderScalar("Facing (degrees, yaw only)", *yawDegreesScratch,
                                                    yawRange, yawToggle, WidgetStyle(), "%.1f");
    ApplyScenarioUnitPlacementYawIfChanged(placement, *yawDegreesScratch, yawChange.bValueChanged);

    if (ImGui::CollapsingHeader("Advanced (raw quaternion)")) {
        const ScalarSliderRange quaternionRange{ -1.0f, 1.0f, 0.0f };
        RealtimeToggle rotXToggle, rotYToggle, rotZToggle, rotWToggle;
        DrawSliderScalar("Rotation X", placement.rotationX, quaternionRange, rotXToggle, WidgetStyle(), "%.3f");
        DrawSliderScalar("Rotation Y", placement.rotationY, quaternionRange, rotYToggle, WidgetStyle(), "%.3f");
        DrawSliderScalar("Rotation Z", placement.rotationZ, quaternionRange, rotZToggle, WidgetStyle(), "%.3f");
        DrawSliderScalar("Rotation W", placement.rotationW, quaternionRange, rotWToggle, WidgetStyle(), "%.3f");
    }
}

} // namespace

void DrawScenarioUnitPlacementsSection(Params::ScenarioBody& body, const std::vector<Params::Army>& armies) {
    int removeIndex = -1;
    for (std::size_t index = 0u; index < body.unitPlacements.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        DrawScenarioUnitPlacementRow(body.unitPlacements[index], armies);
        if (ImGui::SmallButton("X##removeUnitPlacement")) removeIndex = static_cast<int>(index);
        ImGui::Separator();
        ImGui::PopID();
    }
    if (removeIndex >= 0) body.unitPlacements.erase(body.unitPlacements.begin() + removeIndex);
    if (ImGui::Button("+ Add Unit Placement")) AppendDefaultScenarioUnitPlacement(body.unitPlacements);
}

} // namespace Ui
} // namespace SanmapGen

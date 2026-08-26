// MarkersTab_BundleNodeBody_UI.cpp — one Bundle's own inline body (rename/type scope/add-layer-here/
// Move/Rotate/Delete). The aspect-split sibling of MarkersTab_Bundles_UI.cpp (ARCH §1.5 — the
// work-order's single-file draft crossed the 150-line hard ceiling once formatted), both declared by
// MarkersTab_Bundles_UI.h — the same split MarkersTab_RuleLayers_UI.cpp/
// MarkersTab_RuleLayerSettings_UI.cpp already uses.
#include "MarkersTab_Bundles_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "PlacementRuleSections_UI.h"
#include "../math/RigidTransformPivot_MATH.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

constexpr float kBundleRotatePi = 3.14159265358979323846f;   // per-file local literal — established
                                                              // convention (symmetryPi, RadialClearance_MATH's pi)

// Move/Rotate, both scoped to the Bundle's MANUAL-ONLY resolved membership (§19.9) — a Procedural
// Layer under this Bundle contributes zero members here, by design. STEP120: NOT anonymous-namespace
// local — declared in MarkersTab_Bundles_UI.h so MarkersTab_Bundles_UI_Test.cpp can exercise each
// Apply function's own call-boundary behavior directly (pure logic, no imgui frame needed).
void ApplyMarkerLayerBundleMove(int bundleIdentifier, const std::vector<Params::MarkerLayerBundle>& bundles,
                                const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                std::vector<Params::MarkerInstanceGroup>& markers, float offsetX, float offsetZ) {
    const auto members = Params::CollectMarkerLayerBundleRecursiveManualMembers(bundleIdentifier, bundles,
                                                                                instanceLayers, markers);
    for (const auto& member : members) {
        auto& transform = markers[static_cast<std::size_t>(member.first)]
            .transforms[static_cast<std::size_t>(member.second)];
        transform.transform.positionX += offsetX;
        transform.transform.positionZ += offsetZ;
    }
}

void ApplyMarkerLayerBundleRotation(int bundleIdentifier, const std::vector<Params::MarkerLayerBundle>& bundles,
                                    const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers, float degrees) {
    const auto members = Params::CollectMarkerLayerBundleRecursiveManualMembers(bundleIdentifier, bundles,
                                                                                instanceLayers, markers);
    if (members.empty()) return;
    float centroidX = 0.0f, centroidZ = 0.0f;
    for (const auto& member : members) {
        const auto& transform = markers[static_cast<std::size_t>(member.first)]
            .transforms[static_cast<std::size_t>(member.second)];
        centroidX += transform.transform.positionX; centroidZ += transform.transform.positionZ;
    }
    centroidX /= static_cast<float>(members.size()); centroidZ /= static_cast<float>(members.size());
    const float angleRadians = degrees * (kBundleRotatePi / 180.0f);
    float yawX, yawY, yawZ, yawW;
    Math::YawQuaternion(angleRadians, yawX, yawY, yawZ, yawW);
    for (const auto& member : members) {
        auto& transform = markers[static_cast<std::size_t>(member.first)]
            .transforms[static_cast<std::size_t>(member.second)];
        float newX, newZ;
        Math::RotatePointAroundPivot(transform.transform.positionX, transform.transform.positionZ,
                                     centroidX, centroidZ, angleRadians, newX, newZ);
        transform.transform.positionX = newX; transform.transform.positionZ = newZ;
        float newRotX, newRotY, newRotZ, newRotW;
        Math::MultiplyQuaternions(yawX, yawY, yawZ, yawW, transform.transform.rotationX,
                                  transform.transform.rotationY, transform.transform.rotationZ,
                                  transform.transform.rotationW, newRotX, newRotY, newRotZ, newRotW);
        transform.transform.rotationX = newRotX; transform.transform.rotationY = newRotY;
        transform.transform.rotationZ = newRotZ; transform.transform.rotationW = newRotW;
    }
}

// One Bundle's own inline body: rename, type scope, per-Bundle "add a Layer here", Move/Rotate,
// Delete (promotes children, never cascades). Never "selected"-gated (STEP110 posture).
void DrawMarkerLayerBundleNodeBody(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                   std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                   std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   MarkerLayerBundlesState& state, MarkersTabState& rootState,
                                   Pipeline::PreviewDriver* previewDriver) {
    const auto bundleIt = std::find_if(bundles.begin(), bundles.end(),
        [&](const Params::MarkerLayerBundle& candidate) { return candidate.identifier == bundleIdentifier; });
    if (bundleIt == bundles.end()) return;   // Constitution §6 — an id is validated, never trusted
    Params::MarkerLayerBundle& bundle = *bundleIt;

    (void)previewDriver;   // STEP138 — no longer used here (see below); kept as a parameter so this
                            // signature does not ripple, same posture other dormant-notify params use.
    TextInputRules nameRules;
    nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Group";
    DrawTextInput("Name", bundle.name, nameRules);
    // Human's own instruction: no "Marker Type" input anywhere — the Section a Group lives in IS
    // its Marker Type. `bundle.markerTypeName` is still set, once, at creation ("+ Group" seeds it
    // from the Type-section it was added from, MarkersTab_UI.cpp) and never re-editable.

    // STEP138/human's own correction: no per-node "Add Layer"/"Add Manual Layer Here" here either —
    // fully redundant with the Type-section header's own "+ Layer" (which already targets THIS
    // Group once its node is selected, MarkersTab_UI.cpp's ResolveAddLayerParentBundleIdentifier),
    // and a second affordance produced the same confusing double-add STEP138's own "Add Group"
    // duplicate did.

    ImGui::Separator();
    ImGui::TextUnformatted("Move");
    DrawSliderScalar("Offset X", state.moveOffsetX, state.moveOffsetRange, state.moveOffsetXToggle);
    DrawSliderScalar("Offset Z", state.moveOffsetZ, state.moveOffsetRange, state.moveOffsetZToggle);
    if (ImGui::Button("Apply Move"))
        ApplyMarkerLayerBundleMove(bundleIdentifier, bundles, instanceLayers, markers,
                                   state.moveOffsetX, state.moveOffsetZ);
    ImGui::TextUnformatted("Rotate");
    DrawSliderScalar("Degrees", state.rotationDegrees, state.rotationDegreesRange, state.rotationDegreesToggle);
    if (ImGui::Button("Apply Rotation"))
        ApplyMarkerLayerBundleRotation(bundleIdentifier, bundles, instanceLayers, markers, state.rotationDegrees);

    ImGui::Separator();
    if (ImGui::Button("Delete Group (promotes children)")) {
        const int parentIdentifier = bundle.parentBundleIdentifier;
        for (Params::MarkerLayerBundle& candidate : bundles)
            if (candidate.parentBundleIdentifier == bundleIdentifier) candidate.parentBundleIdentifier = parentIdentifier;
        for (Params::MarkerRuleLayer& layer : ruleLayers)
            if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
        for (Params::MarkerInstanceLayer& layer : instanceLayers)
            if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
        bundles.erase(bundleIt);
        if (state.selectedBundleIdentifier == bundleIdentifier) state.selectedBundleIdentifier = -1;
    }
}

} // namespace Ui
} // namespace SanmapGen

// MarkersTab_BundleNodeBody_UI.cpp — ApplyMarkerLayerBundleMove/Rotation (pure logic, still tested
// directly, MarkersTab_Bundles_UI_Test.cpp) and DrawMarkerLayerBundleNodeBody, now intentionally
// empty (STEP140/human's own correction: "I did not ask for anything inside of group" — Name/Move/
// Rotate/Delete all moved out, rename+delete now live in the header-extra slot,
// MarkersTab_BundleHeaderExtras_UI.cpp; Move/Rotate had no replacement asked for and are simply
// gone). The aspect-split sibling of MarkersTab_Bundles_UI.cpp (ARCH §1.5), both declared by
// MarkersTab_Bundles_UI.h.
#include "MarkersTab_Bundles_UI.h"
#include "../math/RigidTransformPivot_MATH.h"

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

// One Bundle's own inline body — intentionally empty (see file header). Params kept so the
// declaration/call site (MarkersTab_Bundles_UI.cpp's `drawNodeBody` callback) need not change again
// if real content returns here later.
void DrawMarkerLayerBundleNodeBody(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                   std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                   std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   MarkerLayerBundlesState& state, MarkersTabState& rootState,
                                   Pipeline::PreviewDriver* previewDriver) {
    (void)bundleIdentifier; (void)bundles; (void)ruleLayers; (void)instanceLayers; (void)markers;
    (void)state; (void)rootState; (void)previewDriver;
}

} // namespace Ui
} // namespace SanmapGen

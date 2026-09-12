// Application_PreviewSetup_UI_Test.cpp — STEP267 acceptance: ConfigureDefaultPreview's Slope
// field layer default domain, and a regression sweep over every other layer's defaults.
// Pure: no imgui frame, no window, no GL context.
#include "Application_Defaults_UI.h"
#include "TerrainOverlayTab_UI.h"
#include "SlopeTab_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool NearlyEqual(float left, float right, float tolerance) {
    const float difference = left - right;
    return (difference < 0.0f ? -difference : difference) <= tolerance;
}

// STEP267: the Slope layer's Steep Angle default moved from 45 degrees (the stale 1.0f magic
// gradient) to 30 degrees, matching the slope ramp's red hazard stop.
void RunSlopeLayerDefaultChecks() {
    PreviewCompositeSettings settings;
    ConfigureDefaultPreview(settings, 512);

    PreviewFieldLayer* const slopeLayer = PreviewFieldLayerOfKind(settings, PreviewLayerKind::Slope);
    Check(slopeLayer != nullptr, "a freshly-configured preview carries a Slope layer");
    if (slopeLayer == nullptr) return;

    Check(NearlyEqual(slopeLayer->domainMinimum, 0.0f, 1.0e-6f),
          "the Flat Angle default is unchanged at 0 degrees");
    Check(NearlyEqual(slopeLayer->domainMaximum, SlopeGradientFromDegrees(30.0f), 1.0e-6f),
          "the Steep Angle default is 30 degrees, expressed in the layer's gradient unit");
    Check(NearlyEqual(SlopeDegreesFromGradient(slopeLayer->domainMaximum), 30.0f, 1.0e-2f),
          "round-tripping the default back through degrees recovers 30");
}

// Regression: every other field layer's construction is untouched by this ticket.
void RunOtherLayerRegressionChecks() {
    PreviewCompositeSettings settings;
    ConfigureDefaultPreview(settings, 512);

    PreviewFieldLayer* const heightLayer = PreviewFieldLayerOfKind(settings, PreviewLayerKind::HeightRamp);
    Check(heightLayer != nullptr, "the height layer exists");
    if (heightLayer != nullptr) {
        Check(heightLayer->blendMode == PreviewBlendMode::Replace, "height stays Replace");
        Check(NearlyEqual(heightLayer->domainMinimum, 0.0f, 1.0e-6f) &&
              NearlyEqual(heightLayer->domainMaximum, 1.0f, 1.0e-6f), "height domain unchanged");
        Check(NearlyEqual(heightLayer->opacity, 1.0f, 1.0e-6f), "height opacity unchanged");
    }

    PreviewFieldLayer* const stratumLayer = PreviewFieldLayerOfKind(settings, PreviewLayerKind::StratumSplat);
    Check(stratumLayer != nullptr, "the stratum splat layer exists");
    if (stratumLayer != nullptr) {
        Check(stratumLayer->blendMode == PreviewBlendMode::AlphaBlend, "stratum splat stays AlphaBlend");
        Check(stratumLayer->gradientRampIndex == -1, "stratum splat still carries no ramp");
        Check(NearlyEqual(stratumLayer->opacity, 0.65f, 1.0e-6f), "stratum splat opacity unchanged");
    }

    PreviewFieldLayer* const waterLayer = PreviewFieldLayerOfKind(settings, PreviewLayerKind::Water);
    Check(waterLayer != nullptr, "the water layer exists");
    if (waterLayer != nullptr) {
        Check(waterLayer->blendMode == PreviewBlendMode::AlphaBlend, "water stays AlphaBlend");
        Check(NearlyEqual(waterLayer->domainMinimum, 0.0f, 1.0e-6f) &&
              NearlyEqual(waterLayer->domainMaximum, 1.0f, 1.0e-6f), "water domain unchanged");
        Check(NearlyEqual(waterLayer->opacity, 1.0f, 1.0e-6f), "water opacity unchanged");
    }

    PreviewFieldLayer* const flowLayer = PreviewFieldLayerOfKind(settings, PreviewLayerKind::Flow);
    Check(flowLayer != nullptr, "the flow layer exists");
    if (flowLayer != nullptr) {
        Check(flowLayer->bAutoDomainFromField, "flow still auto-domains from its baked field");
        Check(NearlyEqual(flowLayer->opacity, 1.0f, 1.0e-6f), "flow opacity unchanged");
    }

    PreviewFieldLayer* const accumulationLayer =
        PreviewFieldLayerOfKind(settings, PreviewLayerKind::Accumulation);
    Check(accumulationLayer != nullptr, "the accumulation layer exists");
    if (accumulationLayer != nullptr) {
        Check(accumulationLayer->bAutoDomainFromField,
              "accumulation still auto-domains from its baked field");
        Check(NearlyEqual(accumulationLayer->opacity, 1.0f, 1.0e-6f), "accumulation opacity unchanged");
    }

    PreviewFieldLayer* const mapAreasLayer = PreviewFieldLayerOfKind(settings, PreviewLayerKind::MapAreas);
    Check(mapAreasLayer != nullptr, "the map areas layer exists");
    if (mapAreasLayer != nullptr) {
        Check(mapAreasLayer->blendMode == PreviewBlendMode::Overlay, "map areas stays Overlay");
        Check(mapAreasLayer->gradientRampIndex == -1, "map areas still carries no ramp");
        Check(NearlyEqual(mapAreasLayer->opacity, 1.0f, 1.0e-6f), "map areas opacity unchanged");
    }
}

} // namespace

int main() {
    RunSlopeLayerDefaultChecks();
    RunOtherLayerRegressionChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

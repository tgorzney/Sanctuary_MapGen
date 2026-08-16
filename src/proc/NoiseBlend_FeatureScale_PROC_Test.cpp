// NoiseBlend_FeatureScale_PROC_Test.cpp — WO B2 acceptance: "Scale Features to Map Size".
// Layer: PROC. Covers (a) the pure frequency rule and its input fences, (b) that the rule reaches
// the ONE `frequency` field both backends read, so the Cpu and Gpu paths cannot disagree about it,
// and (c) that toggling it moves the stage's dirty hash — a cached-noise reuse across the toggle
// would silently show the old landscape.
// Runs the flatten only (no cell loop, no GL), so it needs no window and no context.
#include "NoiseBlend_PROC.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// The rule itself: frequency * (reference / mapSize), and every way a caller can hand it nonsense.
void RunFrequencyRuleChecks() {
    Check(Proc::EffectiveLayerFrequency(0.01f, 256, false, 512.0f) == 0.01f,
          "with the toggle OFF the layer's own frequency is used verbatim");
    Check(Proc::EffectiveLayerFrequency(0.01f, 512, true, 512.0f) == 0.01f,
          "at the reference map size the toggle is the identity");
    Check(Proc::EffectiveLayerFrequency(0.01f, 256, true, 512.0f) == 0.02f,
          "half the reference map doubles the frequency (v1's 512/mapSize)");
    Check(Proc::EffectiveLayerFrequency(0.01f, 1024, true, 512.0f) == 0.005f,
          "twice the reference map halves it");
    Check(Proc::EffectiveLayerFrequency(0.01f, 4096, true, 512.0f) == 0.00125f,
          "and the largest offered map scales by 1/8");

    Check(Proc::EffectiveLayerFrequency(0.01f, 0, true, 512.0f) == 0.01f,
          "a zero map size passes the frequency through, never divides by it");
    Check(Proc::EffectiveLayerFrequency(0.01f, -256, true, 512.0f) == 0.01f,
          "so does a negative map size");
    Check(Proc::EffectiveLayerFrequency(0.01f, 256, true, 0.0f) == 0.01f,
          "and a zero reference collapses to the identity rather than to zero frequency");
}

// A stack whose one layer carries a frequency the scaling will visibly move.
Params::LayerStack OneLayerStack(float frequency) {
    Params::LayerStack layerStack;
    layerStack.geoLayers.resize(1);
    layerStack.geoLayers[0].layers.resize(1);
    layerStack.geoLayers[0].layers[0].frequency = frequency;
    return layerStack;
}

// The flattened configuration is the ONE record both backends consume (NoiseBlend_Kernel_PROC.h),
// so proving the scale lands there proves Cpu/Gpu parity for this feature by construction.
float FlattenedFrequency(Params::Geometry& geometry, Params::LayerStack& layerStack) {
    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, layerStack, fields);
    stage.RunOnCpu();
    return stage.LayerConfigurations().empty() ? -1.0f : stage.LayerConfigurations()[0].frequency;
}

void RunFlattenChecks() {
    Params::LayerStack layerStack = OneLayerStack(0.01f);
    Params::Geometry geometry;
    geometry.mapSize = 256;

    geometry.bScaleFeaturesToMapSize = false;
    Check(FlattenedFrequency(geometry, layerStack) == 0.01f,
          "with the toggle off the kernels are handed the layer's frequency unchanged");

    geometry.bScaleFeaturesToMapSize = true;
    Check(FlattenedFrequency(geometry, layerStack) == 0.02f,
          "with it on the kernels are handed the scaled frequency");
    Check(layerStack.geoLayers[0].layers[0].frequency == 0.01f,
          "and the recipe itself is NOT rewritten - the scale is applied at flatten time");

    geometry.mapSize = 1024;
    Check(FlattenedFrequency(geometry, layerStack) == 0.005f,
          "resizing the map rescales what the kernels get, from the same recipe");
}

// The dirty hash has to see the toggle, or the cached raw noise from before the flip is reused.
void RunDirtyHashChecks() {
    Params::LayerStack layerStack = OneLayerStack(0.01f);
    Params::Geometry geometry;
    geometry.mapSize = 256;
    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, layerStack, fields);

    geometry.bScaleFeaturesToMapSize = false;
    const std::size_t unscaledHash = stage.ComputeParameterHash();
    geometry.bScaleFeaturesToMapSize = true;
    const std::size_t scaledHash = stage.ComputeParameterHash();
    Check(unscaledHash != scaledHash, "toggling the scale moves the stage's parameter hash");

    // At the reference size the toggle is the identity, so it must cost NOTHING — the hash is on
    // the effective frequency, not on the flag, precisely so a no-op edit is free.
    geometry.mapSize = 512;
    geometry.bScaleFeaturesToMapSize = false;
    const std::size_t referenceUnscaled = stage.ComputeParameterHash();
    geometry.bScaleFeaturesToMapSize = true;
    Check(stage.ComputeParameterHash() == referenceUnscaled,
          "at the reference size the toggle is a no-op and does not re-run the stage");

    // The per-layer name is metadata; renaming must not re-roll noise.
    layerStack.geoLayers[0].layers[0].name = "Renamed ridge layer";
    Check(stage.ComputeParameterHash() == referenceUnscaled,
          "renaming a layer does not move the hash (pure metadata, WO B2)");
}

} // namespace

int main() {
    RunFrequencyRuleChecks();
    RunFlattenChecks();
    RunDirtyHashChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

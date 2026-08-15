// Bake_TestScene_PROC.h — shared scaffolding for the bake acceptance test: the pass/fail
// counter, RGBA8 expectations, and the one two-stratum scene both halves (Cpu in
// Bake_PROC_Test.cpp, Gpu in Bake_Gpu_PROC_Test.cpp) bake, so the two backends are compared
// on IDENTICAL inputs. Test-only support; not compiled into the application.
// The scene fills `surfaceStratumWeights` directly — that is the Mask stage's product and the
// only weight field the bake reads (ARCH §7.2).
#pragma once
#include "Bake_PROC.h"
#include <vector>

namespace SanmapGen {
namespace Proc {

inline int bakeTestFailures = 0;

inline void CheckBakeExpectation(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++bakeTestFailures; }
}

inline unsigned int ExpectedTexel(int red, int green, int blue, int alpha) {
    return static_cast<unsigned int>(red) | (static_cast<unsigned int>(green) << 8)
         | (static_cast<unsigned int>(blue) << 16) | (static_cast<unsigned int>(alpha) << 24);
}

inline bool AllTexelsEqual(const std::vector<unsigned int>& texels, unsigned int expected) {
    for (unsigned int texel : texels) if (texel != expected) return false;
    return !texels.empty();
}

inline bool HasVariedTexels(const std::vector<unsigned int>& texels) {
    for (std::size_t index = 1; index < texels.size(); ++index)
        if (texels[index] != texels[0]) return true;
    return false;
}

// The settings + loaded art the stage reads, owned by the test the way the recipe and the
// asset loader own them in the application.
struct BakeSceneInputs {
    std::vector<Params::Stratum>  strata;
    std::vector<Data::StratumArt> stratumArt;

    BakeSceneInputs()
        : strata(static_cast<std::size_t>(Data::MapFields::stratumCount)),
          stratumArt(static_cast<std::size_t>(Data::MapFields::stratumCount)) {}
};

// A red base stratum at surface weight 0.25 and a blue stratum at 0.75, baked at 1:1.
inline void BuildTwoStratumScene(Params::Geometry& geometry, Data::MapFields& fields,
                                 BakeSceneInputs& scene, BakeStage& stage) {
    fields.Resize(geometry.VertexSize(), 0.0f);
    fields.surfaceStratumWeights[0].Fill(0.25f);
    fields.surfaceStratumWeights[1].Fill(0.75f);
    stage.Constants().outputResolutionMultiplier = 1;
    stage.Constants().minimumOutputResolution = 4;
    scene.strata[0].tintRed = 1.0f; scene.strata[0].tintGreen = 0.0f; scene.strata[0].tintBlue = 0.0f;
    scene.strata[1].tintRed = 0.0f; scene.strata[1].tintGreen = 0.0f; scene.strata[1].tintBlue = 1.0f;
}

inline Sys::DispatchPolicy CpuVisualPolicy() {
    Sys::DispatchPolicy policy;
    policy.previewBackend  = Sys::ComputeBackend::Cpu;
    policy.outputBackend   = Sys::ComputeBackend::Cpu;
    policy.previewAccuracy = Sys::AccuracyClass::Visual;
    policy.outputAccuracy  = Sys::AccuracyClass::Visual;
    return policy;
}

} // namespace Proc
} // namespace SanmapGen

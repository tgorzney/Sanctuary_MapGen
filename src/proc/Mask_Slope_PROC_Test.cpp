// Mask_Slope_PROC_Test.cpp — the slope half of the M3-2 acceptance test: the pinned slope unit
// (gradient = rise/run, with terrainMaxHeight read from the map), hard clamp vs smoothstep,
// feather, invert, and gate strength. Expected values are derived from a ramp whose gradient is
// known exactly by hand, not from the kernel headers under test.
#include "Mask_TestSupport_PROC.h"
#include "Mask_PROC.h"
#include <vector>

namespace SanmapGen {
namespace MaskTest {
namespace {

// A pure ramp: every interior AND edge vertex has the same finite-difference gradient.
void FillRampHeightfield(Data::MapFields& fields, int vertexSize, float risePerCell) {
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            fields.heightfield.Set(x, y, static_cast<float>(x) * risePerCell);
}

std::vector<Params::StratumMask> MakeGateSettings(float minimumDegrees, float maximumDegrees,
                                                  bool bSmoothstep, float featherDegrees) {
    Params::StratumMask stratumMask;
    stratumMask.bSlopeGateEnabled = true;
    stratumMask.minimumSlopeDegrees = minimumDegrees;
    stratumMask.maximumSlopeDegrees = maximumDegrees;
    stratumMask.bUseSmoothstep = bSmoothstep;
    stratumMask.slopeFeatherDegreesLow = featherDegrees;
    stratumMask.slopeFeatherDegreesHigh = featherDegrees;
    return std::vector<Params::StratumMask>(Data::MapFields::stratumCount, stratumMask);
}

// Runs the CPU path once and returns the resulting gate weight per cell (output / procedural).
std::vector<float> RunGate(const std::vector<Params::StratumMask>& stratumMasks, int mapSize,
                           float terrainMaxHeight, bool bRamp, float risePerCell) {
    Params::Geometry geometry;
    geometry.mapSize = mapSize;
    geometry.terrainMaxHeight = terrainMaxHeight;
    const int vertexSize = geometry.VertexSize();
    Data::MapFields fields;
    fields.Resize(vertexSize);
    if (bRamp) FillRampHeightfield(fields, vertexSize, risePerCell);
    else       FillTestHeightfield(fields, vertexSize);
    FillTestProceduralMasks(fields, vertexSize);
    Data::MapFields expected = fields;
    Proc::MaskStage stage(geometry, stratumMasks, fields);
    stage.RunOnCpu();
    std::vector<float> gateWeights;
    gateWeights.reserve(static_cast<std::size_t>(vertexSize) * vertexSize);
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            const float procedural = expected.materialMasks[0].Get(x, y);
            gateWeights.push_back(procedural > 0.0f ? fields.materialMasks[0].Get(x, y) / procedural : -1.0f);
        }
    return gateWeights;
}

// 1-2. Pinned unit + terrainMaxHeight. rise 1/128 per cell at terrainMaxHeight 128 => gradient
// exactly 1.0 = 45 degrees, at every vertex including the one-sided edges.
void CheckPinnedSlopeUnit() {
    std::vector<float> weights = RunGate(MakeGateSettings(0.0f, 44.0f, false, 0.0f), 8, 128.0f, true, 1.0f / 128.0f);
    bool bAllOutside = true;
    for (float weight : weights) if (weight > 0.0f) bAllOutside = false;
    Check(bAllOutside, "45-degree ramp is rejected by a [0,44] hard window (slope unit = gradient)");

    weights = RunGate(MakeGateSettings(0.0f, 46.0f, false, 0.0f), 8, 128.0f, true, 1.0f / 128.0f);
    bool bAllInside = true;
    for (float weight : weights) if (weight >= 0.0f && std::fabs(weight - 1.0f) > 1e-5f) bAllInside = false;
    Check(bAllInside, "45-degree ramp passes a [0,46] hard window");

    // 2. terrainMaxHeight comes from the map: doubling it doubles the gradient (45 -> 63.4 deg).
    weights = RunGate(MakeGateSettings(0.0f, 46.0f, false, 0.0f), 8, 256.0f, true, 1.0f / 128.0f);
    bAllOutside = true;
    for (float weight : weights) if (weight > 0.0f) bAllOutside = false;
    Check(bAllOutside, "terrainMaxHeight 256 rescales the same ramp out of the [0,46] window");
}

// 3. Hard clamp is binary; smoothstep is not — they visibly differ on the same terrain.
void CheckSmoothstepVersusHardClamp() {
    const std::vector<float> hardWeights = RunGate(MakeGateSettings(10.0f, 30.0f, false, 8.0f), 64, 128.0f, false, 0.0f);
    const std::vector<float> softWeights = RunGate(MakeGateSettings(10.0f, 30.0f, true, 8.0f), 64, 128.0f, false, 0.0f);
    int hardIntermediate = 0, softIntermediate = 0;
    float largestDifference = 0.0f;
    for (std::size_t index = 0; index < hardWeights.size(); ++index) {
        if (hardWeights[index] < 0.0f) continue;
        if (hardWeights[index] > 1e-4f && hardWeights[index] < 1.0f - 1e-4f) ++hardIntermediate;
        if (softWeights[index] > 1e-4f && softWeights[index] < 1.0f - 1e-4f) ++softIntermediate;
        const float difference = std::fabs(hardWeights[index] - softWeights[index]);
        if (difference > largestDifference) largestDifference = difference;
    }
    Check(hardIntermediate == 0, "hard clamp produces only fully in/out weights");
    Check(softIntermediate > 100, "smoothstep produces feathered intermediate weights");
    Check(largestDifference > 0.05f, "smoothstep and hard clamp visibly differ");
}

// 4. Invert flips the window exactly; strength scales the rejection depth.
void CheckInvertAndStrength() {
    const std::vector<float> invertedWeights = [] {
        std::vector<Params::StratumMask> settings = MakeGateSettings(10.0f, 30.0f, false, 0.0f);
        for (Params::StratumMask& stratumMask : settings) stratumMask.bInvertSlopeGate = true;
        return RunGate(settings, 64, 128.0f, false, 0.0f);
    }();
    const std::vector<float> plainWeights = RunGate(MakeGateSettings(10.0f, 30.0f, false, 0.0f), 64, 128.0f, false, 0.0f);
    bool bComplementary = true;
    for (std::size_t index = 0; index < plainWeights.size(); ++index)
        if (plainWeights[index] >= 0.0f && std::fabs(plainWeights[index] + invertedWeights[index] - 1.0f) > 1e-5f)
            bComplementary = false;
    Check(bComplementary, "invert is the exact complement of the gate");

    const std::vector<float> halfStrength = [] {
        std::vector<Params::StratumMask> settings = MakeGateSettings(10.0f, 30.0f, false, 0.0f);
        for (Params::StratumMask& stratumMask : settings) stratumMask.slopeGateStrength = 0.5f;
        return RunGate(settings, 64, 128.0f, false, 0.0f);
    }();
    bool bHalved = true;
    for (std::size_t index = 0; index < plainWeights.size(); ++index) {
        if (plainWeights[index] < 0.0f) continue;
        const float expectedWeight = 1.0f + (plainWeights[index] - 1.0f) * 0.5f;
        if (std::fabs(halfStrength[index] - expectedWeight) > 1e-5f) bHalved = false;
    }
    Check(bHalved, "gate strength mixes linearly toward no gating");
}

} // namespace

void RunSlopeGateTests() {
    CheckPinnedSlopeUnit();
    CheckSmoothstepVersusHardClamp();
    CheckInvertAndStrength();
}

} // namespace MaskTest
} // namespace SanmapGen

// NoiseBlend_TestSupport_PROC.h — the shared assertions of the M3-1 acceptance test: the pass/
// fail counter, the documented Cpu/Gpu parity tolerance, and the two field comparisons every
// parity check runs. Test support only; no production code includes this.
#pragma once
#include <cstdio>
#include <cmath>
#include "../data/MapFields_DATA.h"

namespace SanmapGen {
namespace Proc {

// Visual-class tolerance (Constitution §4): the backends evaluate the SAME expressions on
// different float hardware, so the bound is on magnitude, not bits. Measured: worst
// |difference| 7.7e-07 on the M3-1 reference stack (256x256, three layers, terracing and
// Levels included) and 3.6e-06 across all 24 NoiseType x FractalType combinations at 128x128 —
// zero cells past the bound in every case. The bound is left ~2.5 orders wider than the
// measurement so a different GPU still passes; the outlier fraction is what catches a real
// algorithmic divergence, and it must stay at zero.
constexpr float parityAbsoluteTolerance = 1.0e-3f;
constexpr float parityOutlierFraction   = 0.001f;

// Max |difference| plus the fraction of cells past the tolerance, so a knife-edge cell
// (floor() on a terrace boundary, pow() on Levels) is reported instead of silently widening
// the bound for the whole field.
inline void CompareFields(const Data::FloatField& cpuField, const Data::FloatField& gpuField,
                          const char* label, void (*check)(bool, const char*)) {
    float maximumDifference = 0.0f;
    std::size_t outlierCount = 0;
    for (std::size_t cell = 0; cell < cpuField.CellCount(); ++cell) {
        const float difference = std::fabs(cpuField.Data()[cell] - gpuField.Data()[cell]);
        if (difference > maximumDifference) maximumDifference = difference;
        if (difference > parityAbsoluteTolerance) ++outlierCount;
    }
    const float outliers = static_cast<float>(outlierCount) / static_cast<float>(cpuField.CellCount());
    std::printf("  parity %-22s maxDifference=%.3e outliers=%.4f%%\n", label,
                static_cast<double>(maximumDifference), static_cast<double>(outliers) * 100.0);
    check(outliers <= parityOutlierFraction, label);
}

// A field of all-identical values would pass any parity bound, so prove there is signal first.
inline void CheckFieldHasSignal(const Data::FloatField& field, const char* label,
                                void (*check)(bool, const char*)) {
    float lowest = field.Data()[0];
    float highest = field.Data()[0];
    for (std::size_t cell = 1; cell < field.CellCount(); ++cell) {
        if (field.Data()[cell] < lowest)  lowest = field.Data()[cell];
        if (field.Data()[cell] > highest) highest = field.Data()[cell];
    }
    check(highest - lowest > 0.05f, label);
}

} // namespace Proc
} // namespace SanmapGen

// Dispatch_SYS.h — the CPU/GPU dispatch contract (ARCH §4).
// Layer: SYS. Defines the dispatch vocabulary (backend / context / accuracy class /
// policy), the backend-resolution rule, and the router seam every stage runs through.
// This is the ONLY place a backend is chosen — no stage reads a raw "UseGPU" bool.
// SYS knows HOW to run a kernel, not WHICH stages exist (the per-stage default policies
// are built by PIPELINE).
#pragma once

namespace SanmapGen {
namespace Sys {

enum class ComputeBackend    { Cpu, Gpu, Automatic };
enum class GenerationContext { Preview, Output };
enum class AccuracyClass     { Exact, Accurate, Visual };
enum class DataResidency     { OnCpu, OnGpu, Either };

struct DispatchPolicy {
    ComputeBackend previewBackend  = ComputeBackend::Gpu;
    ComputeBackend outputBackend   = ComputeBackend::Cpu;
    AccuracyClass  previewAccuracy = AccuracyClass::Visual;
    AccuracyClass  outputAccuracy  = AccuracyClass::Accurate;
    bool           bDeterministic  = false;   // forces Cpu for Exact-class stages
};

inline AccuracyClass ActiveAccuracyClass(const DispatchPolicy& policy, GenerationContext context) {
    return context == GenerationContext::Preview ? policy.previewAccuracy : policy.outputAccuracy;
}

// ARCH §4.3 resolution order.
inline ComputeBackend ResolveBackend(const DispatchPolicy& policy, GenerationContext context,
                                     ComputeBackend globalDefault, DataResidency residency) {
    // 1. Deterministic mode forces Cpu for Exact-class (gameplay-authoritative) stages;
    //    Visual/Accurate stages are exempt and follow the normal rules.
    if (policy.bDeterministic && ActiveAccuracyClass(policy, context) == AccuracyClass::Exact)
        return ComputeBackend::Cpu;
    // 2. The stage's backend for the active context.
    ComputeBackend chosen = context == GenerationContext::Preview ? policy.previewBackend : policy.outputBackend;
    // 3. Automatic -> the global backend setting.
    if (chosen == ComputeBackend::Automatic) chosen = globalDefault;
    // 4. Still Automatic -> resolve by data residency (avoid needless copies; prefer the
    //    Gpu speed path when data is already there or could go either way).
    if (chosen == ComputeBackend::Automatic)
        return residency == DataResidency::OnCpu ? ComputeBackend::Cpu : ComputeBackend::Gpu;
    return chosen;
}

// The router seam: resolve the backend, then run the kernel there. Kernel must provide
// RunOnCpu() and RunOnGpu(). Returns the backend actually used. Every stage dispatches
// through this single function.
template <typename Kernel>
inline ComputeBackend Dispatch(Kernel& kernel, const DispatchPolicy& policy, GenerationContext context,
                               ComputeBackend globalDefault, DataResidency residency) {
    ComputeBackend backend = ResolveBackend(policy, context, globalDefault, residency);
    if (backend == ComputeBackend::Cpu) kernel.RunOnCpu();
    else                                kernel.RunOnGpu();
    return backend;
}

} // namespace Sys
} // namespace SanmapGen

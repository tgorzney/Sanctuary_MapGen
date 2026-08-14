// Dispatch_SYS_Test.cpp — acceptance test for Dispatch_SYS (M0-8).
//   g++ -O2 -std=c++17 Dispatch_SYS_Test.cpp -o t && ./t
// Verifies the ARCH §4.3 resolution matrix and the router seam (mock kernel, no GL).
#include "Dispatch_SYS.h"
#include <cstdio>

using namespace SanmapGen::Sys;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

struct MockKernel {
    bool ranCpu = false, ranGpu = false;
    void RunOnCpu() { ranCpu = true; }
    void RunOnGpu() { ranGpu = true; }
};

int main() {
    // Default policy: Preview=Gpu/Visual, Output=Cpu/Accurate.
    DispatchPolicy defaults;
    check(ResolveBackend(defaults, GenerationContext::Preview, ComputeBackend::Automatic, DataResidency::Either) == ComputeBackend::Gpu, "default preview -> Gpu");
    check(ResolveBackend(defaults, GenerationContext::Output,  ComputeBackend::Automatic, DataResidency::Either) == ComputeBackend::Cpu, "default output -> Cpu");

    // Deterministic forces Cpu for an Exact output stage, even if outputBackend=Gpu.
    DispatchPolicy exactGpu;
    exactGpu.outputBackend = ComputeBackend::Gpu;
    exactGpu.outputAccuracy = AccuracyClass::Exact;
    exactGpu.bDeterministic = true;
    check(ResolveBackend(exactGpu, GenerationContext::Output, ComputeBackend::Gpu, DataResidency::OnGpu) == ComputeBackend::Cpu, "deterministic+Exact -> Cpu");

    // Deterministic does NOT force a Visual stage (exempt) -> follows backend.
    DispatchPolicy visualDet;
    visualDet.previewBackend = ComputeBackend::Gpu;
    visualDet.previewAccuracy = AccuracyClass::Visual;
    visualDet.bDeterministic = true;
    check(ResolveBackend(visualDet, GenerationContext::Preview, ComputeBackend::Cpu, DataResidency::Either) == ComputeBackend::Gpu, "deterministic+Visual -> Gpu (exempt)");

    // Automatic stage falls to the global default.
    DispatchPolicy autoStage;
    autoStage.outputBackend = ComputeBackend::Automatic;
    check(ResolveBackend(autoStage, GenerationContext::Output, ComputeBackend::Cpu, DataResidency::Either) == ComputeBackend::Cpu, "auto stage + global Cpu");
    check(ResolveBackend(autoStage, GenerationContext::Output, ComputeBackend::Gpu, DataResidency::Either) == ComputeBackend::Gpu, "auto stage + global Gpu");

    // Automatic stage + Automatic global -> resolve by residency.
    check(ResolveBackend(autoStage, GenerationContext::Output, ComputeBackend::Automatic, DataResidency::OnCpu) == ComputeBackend::Cpu, "auto+auto residency OnCpu -> Cpu");
    check(ResolveBackend(autoStage, GenerationContext::Output, ComputeBackend::Automatic, DataResidency::OnGpu) == ComputeBackend::Gpu, "auto+auto residency OnGpu -> Gpu");
    check(ResolveBackend(autoStage, GenerationContext::Output, ComputeBackend::Automatic, DataResidency::Either) == ComputeBackend::Gpu, "auto+auto residency Either -> Gpu");

    // Router seam runs the resolved backend exactly once.
    {
        MockKernel kernel;
        ComputeBackend used = Dispatch(kernel, defaults, GenerationContext::Output, ComputeBackend::Automatic, DataResidency::Either);
        check(used == ComputeBackend::Cpu && kernel.ranCpu && !kernel.ranGpu, "router -> Cpu path");
    }
    {
        MockKernel kernel;
        ComputeBackend used = Dispatch(kernel, defaults, GenerationContext::Preview, ComputeBackend::Automatic, DataResidency::Either);
        check(used == ComputeBackend::Gpu && kernel.ranGpu && !kernel.ranCpu, "router -> Gpu path");
    }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}

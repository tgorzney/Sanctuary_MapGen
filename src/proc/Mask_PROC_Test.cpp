// Mask_PROC_Test.cpp — acceptance test for the Mask stage (work-order M3-2 rework). Build with
// MSVC from src/proc (the shader directory defaults to "." so the GLSL twins are found here):
//   cl /EHsc /std:c++17 /O2 /I. Mask_PROC_Test.cpp Mask_Slope_PROC_Test.cpp \
//      Mask_WorldScale_PROC_Test.cpp Mask_Merge_PROC_Test.cpp Mask_Purity_PROC_Test.cpp \
//      Mask_DirtyHash_PROC_Test.cpp Mask_Parity_PROC_Test.cpp Mask_PROC.cpp Mask_Prepare_PROC.cpp \
//      Mask_Apply_PROC.cpp Mask_Gpu_PROC.cpp ../sys/GpuResource_Program_SYS.cpp \
//      ../sys/GpuResource_ProgramParts_SYS.cpp ../sys/GpuResource_Buffer_SYS.cpp \
//      ../sys/GpuGlFunctions_SYS.cpp opengl32.lib gdi32.lib user32.lib
// Covers: the pinned slope unit, cell world-size (one owner: Geometry::worldUnitsPerCell,
// M5-0d), hard-clamp vs smoothstep, feather/invert/strength, the three
// stored-art merge modes on hand-checked values, bilinear (not nearest) resampling, the ONE
// per-stratum remap, the single-writer + idempotence rules (ARCH §7.2/§3.4), CPU/GPU parity,
// and dirty-hash skip/re-run through Generation_PIPELINE.
#include "Mask_TestSupport_PROC.h"

using namespace SanmapGen;

int main(int argc, char** argv) {
    const char* shaderDirectory = (argc > 1) ? argv[1] : ".";

    MaskTest::RunSlopeGateTests();
    MaskTest::RunWorldScaleTests();
    MaskTest::RunMergeTests();
    MaskTest::RunPurityTests();
    MaskTest::RunDirtyHashTests();
    MaskTest::RunParityTests(shaderDirectory);

    if (MaskTest::FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", MaskTest::FailureCount());
    return 1;
}

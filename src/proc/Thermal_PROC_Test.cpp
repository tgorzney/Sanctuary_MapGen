// Thermal_PROC_Test.cpp — acceptance test for the Thermal stage (work-order M3-4): the entry
// point and the core relaxation behaviour. Flat terrain is untouched, a steep spike relaxes TO
// the talus angle (not past it), material is conserved, and the iteration count takes effect.
// The parameter checks live in Thermal_Params_PROC_Test.cpp and the Cpu/Gpu parity check in
// Thermal_Gpu_PROC_Test.cpp (that one needs a GL context).
// Build with MSVC (from src/proc):
//   cl /EHsc /std:c++17 /O2 Thermal_PROC_Test.cpp Thermal_Params_PROC_Test.cpp \
//      Thermal_Gpu_PROC_Test.cpp Thermal_PROC.cpp Thermal_Relax_PROC.cpp \
//      Thermal_Transport_PROC.cpp Thermal_Gpu_PROC.cpp ../sys/GpuResource_Program_SYS.cpp \
//      ../sys/GpuResource_Buffer_SYS.cpp ../sys/GpuGlFunctions_SYS.cpp \
//      opengl32.lib gdi32.lib user32.lib
// Optional argv[1] = shader directory (defaults to ".", i.e. run it from src/proc).
#include "Thermal_Test_PROC.h"

using namespace SanmapGen;
using namespace SanmapGen::ThermalTest;

namespace {

void CheckFlatTerrainUnchanged() {
    Data::MapFields fields;
    fields.Resize(testVertexSize, 0.5f);
    const Params::Geometry geometry = MakeGeometry(testVertexSize);
    Proc::ThermalStage stage(geometry, fields);
    stage.Constants().iterationCount = 10;
    Check(stage.Run() == Sys::ComputeBackend::Cpu, "Output context dispatches to the Cpu accuracy path");
    Check(stage.CompletedIterationCount() == 10, "every requested sweep ran");
    bool bUnchanged = true;
    for (std::size_t cell = 0; cell < fields.heightfield.CellCount(); ++cell)
        if (fields.heightfield.Data()[cell] != 0.5f) bUnchanged = false;
    Check(bUnchanged, "flat terrain is bit-for-bit unchanged");
}

void CheckSpikeRelaxesToTalusAngle() {
    const SpikeResult relaxed = RelaxSpike(35.0f, convergenceIterations, -1);
    std::printf("spike: talus threshold %.6f, relaxed max drop %.6f, mass drift %.3e\n",
                static_cast<double>(relaxed.talusThreshold),
                static_cast<double>(relaxed.maximumDrop), relaxed.massDrift);
    Check(testPeakHeight > relaxed.talusThreshold * 5.0f, "the test spike really does start over-steep");
    Check(relaxed.maximumDrop <= relaxed.talusThreshold + slopeTolerance,
          "no slope exceeds the talus angle once relaxed");
    Check(relaxed.maximumDrop >= relaxed.talusThreshold - slopeTolerance,
          "the spike settles AT the talus angle, not flattened past it");
    Check(relaxed.massDrift < 1.0e-3 && relaxed.massDrift > -1.0e-3,
          "material is conserved (gather form, no scatter races)");
}

void CheckIterationCountTakesEffect() {
    const SpikeResult none = RelaxSpike(35.0f, 0, -1);
    Check(none.maximumDrop == testPeakHeight, "zero iterations leaves the field untouched");
    const SpikeResult single = RelaxSpike(35.0f, 1, -1);
    Check(single.maximumDrop < testPeakHeight, "one iteration already moves material");
    const SpikeResult few = RelaxSpike(35.0f, 20, -1);
    const SpikeResult many = RelaxSpike(35.0f, convergenceIterations, -1);
    Check(few.maximumDrop > many.maximumDrop, "more iterations relax the slope further");
}

} // namespace

int main(int argc, char** argv) {
    const char* shaderDirectory = (argc > 1) ? argv[1] : ".";
    CheckFlatTerrainUnchanged();
    CheckSpikeRelaxesToTalusAngle();
    CheckIterationCountTakesEffect();
    RunThermalParameterChecks();

    const int parityFailures = RunThermalGpuParityChecks(shaderDirectory);
    if (parityFailures < 0) std::printf("SKIP: no GL context available - Cpu/Gpu parity not verified\n");
    else FailureCount() += parityFailures;

    if (FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", FailureCount());
    return 1;
}

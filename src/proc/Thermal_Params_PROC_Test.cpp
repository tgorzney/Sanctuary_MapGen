// Thermal_Params_PROC_Test.cpp — the tweakability half of the Thermal acceptance test (M3-4).
// Proves the talus angle is a real parameter (the relaxed slope lands on whatever angle is
// asked for), that the PER-MATERIAL angle is selected by the material masks rather than a global
// constant, and that every constant feeds the dirty hash PIPELINE keys the stage on.
#include "Thermal_Test_PROC.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace ThermalTest {
namespace {

// Each sweep writes only scratch, so partitioning rows across workers must not change a bit.
void CheckThreadPoolIsBitIdentical() {
    Data::MapFields serialFields, pooledFields;
    BuildRoughField(serialFields, testVertexSize);
    BuildRoughField(pooledFields, testVertexSize);
    const Params::Geometry geometry = MakeGeometry(testVertexSize);
    Sys::ThreadPool threadPool;

    Proc::ThermalStage serialStage(geometry, serialFields);
    serialStage.Constants().iterationCount = 32;
    serialStage.Run();
    Proc::ThermalStage pooledStage(geometry, pooledFields);
    pooledStage.Constants().iterationCount = 32;
    pooledStage.SetThreadPool(&threadPool);
    pooledStage.Run();

    float difference = MaximumFieldDifference(serialFields.heightfield, pooledFields.heightfield);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        difference += MaximumFieldDifference(serialFields.materialProportions[stratum],
                                             pooledFields.materialProportions[stratum]);
    Check(difference == 0.0f, "the thread-pooled Cpu sweep is bit-identical to the serial one");
}

void CheckTalusAngleTakesEffect() {
    const SpikeResult shallow = RelaxSpike(20.0f, convergenceIterations, -1);
    const SpikeResult steep   = RelaxSpike(60.0f, convergenceIterations, -1);
    std::printf("talus 20deg: threshold %.6f drop %.6f | 60deg: threshold %.6f drop %.6f\n",
                static_cast<double>(shallow.talusThreshold), static_cast<double>(shallow.maximumDrop),
                static_cast<double>(steep.talusThreshold), static_cast<double>(steep.maximumDrop));
    Check(shallow.talusThreshold < steep.talusThreshold, "a larger talus angle is a larger threshold");
    Check(shallow.maximumDrop <= shallow.talusThreshold + slopeTolerance
       && shallow.maximumDrop >= shallow.talusThreshold - slopeTolerance,
          "a 20 degree talus angle produces a 20 degree slope");
    Check(steep.maximumDrop <= steep.talusThreshold + slopeTolerance
       && steep.maximumDrop >= steep.talusThreshold - slopeTolerance,
          "a 60 degree talus angle produces a 60 degree slope");
}

void CheckPerMaterialTalusAngle() {
    const SpikeResult material = RelaxSpike(60.0f, convergenceIterations, 4);
    std::printf("per-material (stratum 4 at 60deg, all others %.0fdeg): threshold %.6f drop %.6f\n",
                static_cast<double>(decoyAngleDegrees),
                static_cast<double>(material.talusThreshold), static_cast<double>(material.maximumDrop));
    Check(material.maximumDrop <= material.talusThreshold + slopeTolerance
       && material.maximumDrop >= material.talusThreshold - slopeTolerance,
          "the covering stratum's talus angle wins over the stratum-0 decoy");
}

void CheckParameterHash() {
    Data::MapFields fields;
    fields.Resize(testVertexSize, 0.0f);
    const Params::Geometry geometry = MakeGeometry(testVertexSize);
    Proc::ThermalStage stage(geometry, fields);
    const std::size_t baseline = stage.ComputeParameterHash();
    Check(baseline == stage.ComputeParameterHash(), "the parameter hash is stable");
    stage.Constants().iterationCount += 1;
    Check(baseline != stage.ComputeParameterHash(), "iteration count feeds the dirty hash");
    stage.Constants().iterationCount -= 1;
    stage.Constants().relaxationRate += 0.1f;
    Check(baseline != stage.ComputeParameterHash(), "relaxation rate feeds the dirty hash");
    stage.Constants().relaxationRate -= 0.1f;
    stage.Constants().talusAngleDegrees[3] += 1.0f;
    Check(baseline != stage.ComputeParameterHash(), "per-stratum talus angle feeds the dirty hash");
}

} // namespace

void RunThermalParameterChecks() {
    CheckTalusAngleTakesEffect();
    CheckPerMaterialTalusAngle();
    CheckParameterHash();
    CheckThreadPoolIsBitIdentical();
}

} // namespace ThermalTest
} // namespace SanmapGen

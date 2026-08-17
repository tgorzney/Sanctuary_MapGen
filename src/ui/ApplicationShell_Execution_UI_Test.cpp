// ApplicationShell_Execution_UI_Test.cpp — tab-rebuild WO E acceptance, part 3: the Performance
// panel's controls reach `Sys::DispatchPolicy` — the GPU toggles, WYSIWYG baking and determinism —
// and reach EVERY stage, which SystemTab_UI.h SCOPE NOTE 2 names as the app shell's job.
// Driven through a REAL Ui::Application, so what passes is the shell's fan-out, not a replica.
// Headless: nothing below runs a stage.
#include "ApplicationShell_TestSupport_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// True when every stage the shell fans out to carries the flag — determinism is read off the
// stage's OWN policy by Sys::ResolveBackend, so a stage that never saw it could not honour it.
bool EveryStageIsDeterministic(Application& application, bool bExpected) {
    Pipeline::GenerationAssembler& assembler = application.Assembler();
    return assembler.NoiseBlend().ActiveDispatchPolicy().bDeterministic       == bExpected
        && assembler.Erosion().ActiveDispatchPolicy().bDeterministic          == bExpected
        && assembler.Thermal().ActiveDispatchPolicy().bDeterministic          == bExpected
        && assembler.FlowAccumulation().ActiveDispatchPolicy().bDeterministic == bExpected
        && assembler.Mask().ActiveDispatchPolicy().bDeterministic             == bExpected
        && assembler.Placement().ActiveDispatchPolicy().bDeterministic        == bExpected
        && assembler.Bake().ActiveDispatchPolicy().bDeterministic             == bExpected;
}

void CheckGpuToggles(Application& application) {
    Pipeline::GenerationAssembler& assembler = application.Assembler();
    application.ExecutionSettings().bUseGpuTerrain = false;
    Check(application.ApplyExecutionPolicy(), "clearing Use Gpu Terrain moves a policy");
    Check(assembler.NoiseBlend().ActiveDispatchPolicy().previewBackend == Sys::ComputeBackend::Cpu
              && assembler.Erosion().ActiveDispatchPolicy().previewBackend == Sys::ComputeBackend::Cpu
              && assembler.Thermal().ActiveDispatchPolicy().previewBackend == Sys::ComputeBackend::Cpu,
          "and lands on all three terrain stages");
    Check(assembler.FlowAccumulation().ActiveDispatchPolicy().previewBackend
              != Sys::ComputeBackend::Cpu,
          "while the flow stage keeps its own toggle");
    Check(!application.ApplyExecutionPolicy(), "re-applying an unchanged panel costs nothing");

    application.ExecutionSettings().bUseGpuFlow = false;
    Check(application.ApplyExecutionPolicy(), "clearing Use Gpu Flow moves a policy");
    Check(assembler.FlowAccumulation().ActiveDispatchPolicy().previewBackend
              == Sys::ComputeBackend::Cpu,
          "and lands on the flow stage");
    application.ExecutionSettings().bUseGpuTerrain = true;
    application.ExecutionSettings().bUseGpuFlow    = true;
    application.ApplyExecutionPolicy();
}

// WYSIWYG: the output pass runs what the preview pass ran, so the bake is the image the user
// approved (ARCH §4.4). Releasing it restores the stage's own ARCH §4.2 default — Erosion's output
// pass is Cpu/Exact, and that is what has to come back rather than a shell-side guess.
void CheckWysiwygBaking(Application& application) {
    Proc::ErosionStage& erosion = application.Assembler().Erosion();
    const Sys::ComputeBackend outputBackendBefore = erosion.ActiveDispatchPolicy().outputBackend;
    const Sys::AccuracyClass  outputAccuracyBefore = erosion.ActiveDispatchPolicy().outputAccuracy;

    application.ExecutionSettings().bWysiwygBaking = true;
    Check(application.ApplyExecutionPolicy(), "engaging Wysiwyg baking moves a policy");
    Check(erosion.ActiveDispatchPolicy().outputBackend
              == erosion.ActiveDispatchPolicy().previewBackend
              && erosion.ActiveDispatchPolicy().outputAccuracy
                  == erosion.ActiveDispatchPolicy().previewAccuracy,
          "the output pass now runs the preview pass");

    application.ExecutionSettings().bWysiwygBaking = false;
    Check(application.ApplyExecutionPolicy(), "releasing it moves a policy back");
    Check(erosion.ActiveDispatchPolicy().outputBackend == outputBackendBefore
              && erosion.ActiveDispatchPolicy().outputAccuracy == outputAccuracyBefore,
          "and restores the stage's own output pass, not a guessed default");
}

} // namespace

void RunShellExecutionChecks() {
    Application application;
    PrepareShellForTest(application);
    Check(!application.ApplyExecutionPolicy(),
          "a fresh shell's panel already agrees with every stage");
    Check(EveryStageIsDeterministic(application, false), "and no stage starts deterministic");

    CheckGpuToggles(application);
    CheckWysiwygBaking(application);

    application.TabState().system.bDeterministic = true;
    Check(application.ApplyExecutionPolicy(), "setting Deterministic moves a policy");
    Check(EveryStageIsDeterministic(application, true), "and reaches EVERY stage, not just one");
    Check(ApplySystemTabSettings(application.TabState().system, application.ActiveDispatchPolicy()),
          "the execution tab's own policy carries it too");
    application.TabState().system.bDeterministic = false;
    application.ApplyExecutionPolicy();
    Check(EveryStageIsDeterministic(application, false), "and clearing it reaches every stage back");
}

} // namespace Ui
} // namespace SanmapGen

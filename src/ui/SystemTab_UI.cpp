// SystemTab_UI.cpp — the imgui composition of the execution tab. Layer: UI.
// The library has no widget for a dropdown, a checkbox or a path field and this tab needs no
// scalar control, so it is the one tab with no shared widget in it — and, correspondingly, the
// one with no ImGui::SliderFloat/DragFloat either.
#include "SystemTab_UI.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

const char* const computeBackendNames[]    = { "Cpu", "Gpu", "Automatic" };
const char* const generationContextNames[] = { "Preview", "Output" };

// An execution change is invisible to every stage's parameter hash — the recipe did not move — so
// NotifyParametersChanged() would derive a pointless recolor. RequestMapUpdate is the driver's own
// door for "a change no parameter hash can see" (PreviewDriver_PIPELINE.h), which is exactly what
// switching backend, context or determinism is.
void RequestRegeneration(Pipeline::PreviewDriver* previewDriver) {
    if (previewDriver != nullptr) previewDriver->RequestMapUpdate();
}

void DrawBackendSettings(SystemTabState& state, Pipeline::GenerationAssembler* generationAssembler,
                         Pipeline::PreviewDriver* previewDriver) {
    int backendIndex = static_cast<int>(state.globalBackend);
    if (ImGui::Combo("Global Backend", &backendIndex, computeBackendNames,
                     IM_ARRAYSIZE(computeBackendNames))) {
        state.globalBackend = static_cast<Sys::ComputeBackend>(backendIndex);
        if (generationAssembler != nullptr) generationAssembler->SetGlobalBackend(state.globalBackend);
        RequestRegeneration(previewDriver);
    }
    int contextIndex = static_cast<int>(state.generationContext);
    if (ImGui::Combo("Generation Context", &contextIndex, generationContextNames,
                     IM_ARRAYSIZE(generationContextNames))) {
        state.generationContext = static_cast<Sys::GenerationContext>(contextIndex);
        if (generationAssembler != nullptr)
            generationAssembler->SetGenerationContext(state.generationContext);
        RequestRegeneration(previewDriver);
    }
    ImGui::TextUnformatted("Automatic resolves per stage: policy, then this setting, then residency.");
}

} // namespace

bool DrawSystemTab(SystemTabState& state, Sys::DispatchPolicy* dispatchPolicy,
                   Pipeline::GenerationAssembler* generationAssembler,
                   Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("systemTab");
    DrawBackendSettings(state, generationAssembler, previewDriver);
    ImGui::Separator();

    bool bDeterministic = state.bDeterministic;
    if (ImGui::Checkbox("Deterministic (forces Cpu for Exact-class stages)", &bDeterministic)) {
        state.bDeterministic = bDeterministic;
        if (dispatchPolicy != nullptr) ApplySystemTabSettings(state, *dispatchPolicy);
        RequestRegeneration(previewDriver);
    }
    if (dispatchPolicy == nullptr)
        ImGui::TextUnformatted("No dispatch policy bound: the toggle is held here only.");

    ImGui::Separator();
    ImGui::InputText("Asset Cache Directory", state.assetCacheDirectory,
                     IM_ARRAYSIZE(state.assetCacheDirectory));
    ImGui::TextUnformatted("Execution settings are NOT recipe content and never serialize with it.");

    ImGui::Separator();
    ImGui::TextWrapped("Force a full regeneration for a change no parameter hash can see: a resize, "
                       "a recipe reload, or new stratum art.");
    // STEP96_FootprintBakeAndStalenessCheck_IO.md §3.1 call site 2 — this IS the one discrete,
    // human-clicked regeneration trigger left in the UI. The click fires RequestRegeneration()
    // unconditionally, THEN reports whether it fired at all; the staleness check itself runs one
    // frame later, in the caller, and never gates this line.
    const bool bClicked = ImGui::Button("Force Regenerate");
    if (bClicked) RequestRegeneration(previewDriver);
    if (!state.lastRegenerateStalenessWarning.empty())
        ImGui::TextWrapped("%s", state.lastRegenerateStalenessWarning.c_str());
    ImGui::PopID();
    return bClicked;
}

} // namespace Ui
} // namespace SanmapGen

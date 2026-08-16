// Application_UI.cpp — the shell's construction, its callback wiring, and the ONE step that turns
// a pending dirty flag into a visible image. Layer: UI. No imgui and no GLFW in this file: the
// assembly of the parts is separable from the toolkit that draws them, so the acceptance test can
// drive exactly this wiring with no window and no GL context.
#include "Application_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Applies the stage-owned defaults and hands the assembler straight back. It has to happen in the
// init list, BEFORE `PreviewDriver` is constructed: the driver caches every stage's parameter hash
// on construction, so a default applied afterwards would leave the cache disagreeing with the
// stages and the next edit would be attributed to the wrong stage.
Pipeline::GenerationAssembler& AssemblerWithDefaultStages(Pipeline::GenerationAssembler& assembler) {
    ConfigureDefaultStages(assembler);
    return assembler;
}

} // namespace

// Member init order follows the DECLARATION order in the header: the assembler binds the recipe,
// the composite binds the assembler's baked fields, its resolved markers and the id buffer, and
// the driver binds the assembler. Every one of those is a reference to a member declared earlier.
Application::Application(ApplicationSettings applicationSettings)
    : settings(std::move(applicationSettings)),
      recipe(MakeDefaultMapRecipe()),
      assembler(recipe),
      composite(recipe.geometry, recipe.water, recipe.strata, assembler.Fields(),
                assembler.Placements().markers, entityIdentifiers),
      previewDriver(AssemblerWithDefaultStages(assembler)),
      threadPool(settings.workerThreadCount) {
    ConfigureDefaultPreview(composite.Settings(), settings.previewResolution,
                            assembler.WorldUnitsPerCell());
    assembler.SetThreadPool(&threadPool);
    WireCallbacks();
}

Application::~Application() { Shutdown(); }

// The three injected seams that keep the layer graph downward-only (ARCH §3.1). PIPELINE may not
// know a composite exists, and the canvas may not know a pipeline exists, so the shell — which
// legally sees both — hands each of them a closure over the other.
void Application::WireCallbacks() {
    previewDriver.SetPreviewCompositeCallback([this] { composite.Compose(); });
    canvas.SetRegenerationCallback([this] { previewDriver.RequestMapUpdate(); });
    canvas.SetSelectionChangedCallback([this](std::uint32_t entityIdentifier) {
        lastSelectedEntityIdentifier = entityIdentifier;
    });
    canvas.SetEntityIdentifierBuffer(&entityIdentifiers);
}

// The whole of the shell's generation duty. WHICH tier this is was derived by the driver from the
// stages' own parameter hashes; the shell neither decides it nor knows what ran.
Pipeline::RefreshTier Application::ServiceDirtyTier() {
    const Pipeline::RefreshTier servicedTier = previewDriver.Refresh();
    if (servicedTier != Pipeline::RefreshTier::Nothing) BindCompositeToCanvas();
    return servicedTier;
}

// The canvas draws a SYS-owned texture. The Gpu composite writes one itself; the Cpu twin (no
// context, or a kernel that would not compile — PreviewComposite_Gpu_UI.cpp falls back rather than
// producing nothing) has only texels, so the shell uploads them through the same SYS seam instead
// of leaving the viewport blank. Either way no GL handle enters the UI layer (ARCH §3.2).
void Application::BindCompositeToCanvas() {
    Sys::GpuTextureHandle previewTexture = composite.CompositeTexture();
    if (!composite.LastRunUsedGpu() && gpuResourceManager != nullptr)
        previewTexture = UploadCompositeTexels();
    canvas.SetPreviewTexture(gpuResourceManager.get(), previewTexture, composite.Resolution());
}

Sys::GpuTextureHandle Application::UploadCompositeTexels() {
    const int resolution = composite.Resolution();
    const std::vector<unsigned int>& texels = composite.CompositeTexels();
    const Sys::GpuTextureHandle mirrorTexture =
        gpuResourceManager->EnsureTexture("previewCompositeCpuMirror", resolution, resolution);
    if (mirrorTexture.IsValid() && !texels.empty())
        gpuResourceManager->UploadTexture(mirrorTexture, texels.data(),
                                          texels.size() * sizeof(unsigned int));
    return mirrorTexture;
}

const IconAtlasManifest* Application::ActiveIconManifest() const {
    return iconManifest.EntryCount() > 0 ? &iconManifest : nullptr;
}

} // namespace Ui
} // namespace SanmapGen

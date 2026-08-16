// PreviewComposite_UI.h — the WYSIWYG preview composite: colorize and composite the BAKED
// fields into one RGBA8 image, and write the entity-id buffer while shading.
// Layer: UI. Accuracy class: Visual. Backend: Gpu through `Sys::GpuResourceManager` (no
// private GL pipeline, no hardcoded shader path); the Cpu twin is the parity reference and
// the fallback when no GL context exists.
//
// It SAMPLES `Data::MapFields` and nothing else: height, the nine `surfaceStratumWeights`,
// flow, accumulation, and the Mask stage's baked `slope` (M5-0c). It never recomputes slope,
// never re-filters a marker/prop rule, never re-runs a sim — that is the whole point of M4
// (ARCH §3.2, §5.4, hit-list #4;
// PREVIEW_COMPOSITING_SPEC "the shadow-sim problem"). It therefore takes no sim parameter of
// any kind: changing one without re-baking cannot move a pixel here.
//
// Pass ordering: clear -> one pass per enabled field layer -> overlay -> entity id.
// On the Gpu the image IS a real GL_RGBA8 texture, owned by `Sys::GpuResourceManager` and written
// through an image unit, so `MapCanvas_UI` (M5-5) samples it directly instead of re-uploading a
// buffer every composite. The same pixels are also read back into `CompositeTexels()` as packed
// RGBA8 (`Ui::PackRgba8`) — that is the Cpu twin's output format and the parity reference.
#pragma once
#include <vector>
#include "PreviewComposite_Color_UI.h"
#include "PreviewComposite_Kernel_UI.h"
#include "PreviewComposite_Settings_UI.h"
#include "../data/EntityIdBuffer_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../data/PlacementInstances_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/Stratum_PARAMS.h"
#include "../params/Water_PARAMS.h"
#include "../sys/GpuResource_SYS.h"

namespace SanmapGen {
namespace Ui {

class PreviewComposite {
public:
    PreviewComposite(const Params::Geometry& geometrySettings, const Params::Water& waterSettings,
                     const std::vector<Params::Stratum>& stratumSettings,
                     const Data::MapFields& inputFields,
                     const Data::PlacementInstances& placedInstances,
                     Data::EntityIdBuffer& entityIdentifierOutput);

    PreviewCompositeSettings& Settings() { return settings; }
    const PreviewCompositeSettings& Settings() const { return settings; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }

    // Runs the pass sequence on the Gpu when a resource manager with a live context is
    // available, else on the Cpu twin. Reports which one it used, rather than silently
    // producing nothing.
    void Compose();
    void ComposeOnCpu();   // PreviewComposite_Cpu_UI.cpp
    void ComposeOnGpu();   // PreviewComposite_Gpu_UI.cpp

    // The composited image: `Resolution()` squared packed RGBA8 texels, row-major.
    const std::vector<unsigned int>& CompositeTexels() const { return compositeTexels; }
    // The same image as the GL texture the last Gpu run wrote — what a canvas draws. Invalid
    // until a Gpu compose has run (the Cpu twin has only the texels above).
    Sys::GpuTextureHandle CompositeTexture() const { return compositeTexture; }
    int Resolution() const { return configuration.previewResolution; }
    bool LastRunUsedGpu() const { return bLastRunUsedGpu; }
    // Passes executed in the last run: clear + one per enabled layer + overlay + entity id.
    // Counted identically on both backends, so a parity check compares the same sequence.
    int ExecutedPassCount() const { return executedPassCount; }
    const PreviewCompositeConfiguration& Configuration() const { return configuration; }
    const std::vector<PreviewLayerConfiguration>& LayerConfigurations() const {
        return layerConfigurations;
    }

private:
    // Flattens settings + params into the kernel records, bakes every referenced ramp, resolves
    // the entity pixel positions, and sizes the outputs. Shared by both backends.
    void PrepareRun();                                       // PreviewComposite_UI.cpp
    void BuildConfigurationRecord();                         // PreviewComposite_UI.cpp
    void BuildStratumConfigurations();                       // PreviewComposite_UI.cpp
    void BuildLayerConfigurations();                         // PreviewComposite_Prepare_UI.cpp
    void BuildEntityPoints();                                // PreviewComposite_Prepare_UI.cpp
    const Data::FloatField* LayerSourceField(PreviewLayerKind kind) const;
    bool EnsureGpuResources();                               // PreviewComposite_GpuProgram_UI.cpp
    void PackSurfaceStratumWeights();                        // PreviewComposite_GpuBuffers_UI.cpp
    bool EnsureCompositeTexture(Sys::GpuResourceManager& manager);  // PreviewComposite_GpuBuffers_UI.cpp
    void BindComposeBuffers(Sys::GpuResourceManager& manager);      // PreviewComposite_GpuBuffers_UI.cpp

    // The four passes of the Cpu twin (PreviewComposite_Cpu_UI.cpp).
    void ClearPassCpu();
    void FieldLayerPassCpu(int layerIndex);
    void OverlayPassCpu();
    void EntityIdentifierPassCpu();
    // One layer's color at one pixel — the Cpu twin of the GLSL `layerColorAtPixel`.
    PreviewColor LayerColorAtPixel(const PreviewLayerConfiguration& layerConfiguration,
                                   float sampleX, float sampleY) const;

    const Params::Geometry&             geometry;
    const Params::Water&                water;
    const std::vector<Params::Stratum>& strata;
    const Data::MapFields&              mapFields;
    const Data::PlacementInstances&     instances;
    Data::EntityIdBuffer&               entityIdentifierBuffer;
    PreviewCompositeSettings            settings;

    PreviewCompositeConfiguration            configuration;
    std::vector<PreviewLayerConfiguration>   layerConfigurations;
    std::vector<PreviewStratumConfiguration> stratumConfigurations;
    std::vector<PreviewEntityPoint>          entityPoints;
    std::vector<float>                       gradientLookupTables;  // every ramp, concatenated
    std::vector<float>                       packedSurfaceWeights;  // the 9 fields, concatenated
    std::vector<unsigned int>                compositeTexels;

    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::GpuTextureHandle    compositeTexture;
    bool bLastRunUsedGpu  = false;
    bool bGpuProgramReady = false;
    int  gpuProgramIndex  = -1;
    int  executedPassCount = 0;
};

} // namespace Ui
} // namespace SanmapGen

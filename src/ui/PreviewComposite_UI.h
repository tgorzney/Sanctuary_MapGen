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
// The output image is a packed RGBA8 texel per pixel (`Ui::PackRgba8`): the SYS seam carries
// buffers, not GL images, so the composite writes the exact bytes a GL_RGBA8 upload wants
// and hands them to the caller. Uploading that image as a texture is M4-5/M5.
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

namespace SanmapGen {
namespace Sys { class GpuResourceManager; }

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
    void BindComposeBuffers(Sys::GpuResourceManager& manager);   // PreviewComposite_GpuBuffers_UI.cpp

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
    bool bLastRunUsedGpu  = false;
    bool bGpuProgramReady = false;
    int  gpuProgramIndex  = -1;
    int  executedPassCount = 0;
};

} // namespace Ui
} // namespace SanmapGen

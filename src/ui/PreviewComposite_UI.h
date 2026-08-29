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
#include "../params/MapArea_PARAMS.h"
#include "../params/Stratum_PARAMS.h"
#include "../params/Water_PARAMS.h"
#include "../sys/GpuResource_SYS.h"

namespace SanmapGen {
namespace Ui {

class PreviewComposite {
public:
    // A world-space point on the horizontal plane (positionX/positionZ — positionY is height,
    // PlacementInstance_DATA).
    struct PreviewWorldPoint  { float worldX = 0.0f; float worldZ = 0.0f; };
    // A point in preview-pixel space (the composited image's own pixel grid).
    struct PreviewPixelPoint  { float pixelX = 0.0f; float pixelY = 0.0f; };

    // What one Compose() call must actually do — replaces the old lone `bNeedsTexelReadback` bool
    // (ARCH §14.18 items 6-7). A named struct, not a second positional bool: "two bools in a row at
    // a call site is a legibility trap this ARCH does not accept." Defaults reproduce EVERY existing
    // caller's observed behavior exactly — a full re-upload, texels read back — so this struct alone
    // changes zero observable behavior.
    struct ComposeRequest {
        // When false, skips only the Gpu texture->CPU CompositeTexels() readback (unchanged from the
        // retired `bNeedsTexelReadback` parameter — see Compose()'s own contract note below).
        bool bNeedsTexelReadback = true;
        // ARCH §14.18 item 6 — when false, the composite trusts that PackSurfaceStratumWeights() and
        // the five baked-input uploads (heightfield, flow, accumulation, slope, surface-stratum
        // weights) are BYTE-IDENTICAL to the last compose, and skips re-packing/re-uploading them.
        // Only `Pipeline::PreviewDriver`'s own callback wiring may set this false, and only when it
        // is servicing `RefreshTier::PreviewRender` (no stage ran, so the bake cannot have moved) —
        // see `PreviewDriver_PIPELINE.h`'s own invariant this rests on.
        bool bBakedInputsChanged = true;
    };

    // Gpu-path-specific observability (ARCH §14.18 item 10's benchmark prerequisite; STEP218). Not a
    // second implementation of anything — a pure timing side-channel `ComposeOnGpu` fills in ONLY
    // when a caller hands it a non-null pointer (see `ComposeOnGpu` below). Nested beside
    // `ComposeRequest` for the same reason that struct is nested: this exists only to be
    // `ComposeOnGpu()`'s own vocabulary, not a general-purpose type (STEP216's own nesting
    // precedent, `PreviewComposite_UI.h`'s "Interpretation calls made" §1).
    struct ComposeGpuTiming {
        double bindAndDispatchMillis  = 0.0;  // buffer binds/uploads + all pass dispatches, issue-side
        double fenceWaitMillis        = 0.0;  // WaitForCompletion's spin, isolated
        double texelReadbackMillis    = 0.0;  // only non-zero when request.bNeedsTexelReadback
        double entityIdReadbackMillis = 0.0;  // the unconditional entity-id readback, isolated
    };

    PreviewComposite(const Params::Geometry& geometrySettings, const Params::Water& waterSettings,
                     const std::vector<Params::Stratum>& stratumSettings,
                     const std::vector<Params::MapArea>& mapAreaSettings,
                     const Data::MapFields& inputFields,
                     const Data::PlacementInstances& placedInstances,
                     Data::EntityIdBuffer& entityIdentifierOutput);

    PreviewCompositeSettings& Settings() { return settings; }
    const PreviewCompositeSettings& Settings() const { return settings; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }

    // Preview pixels per one world-space "cell" of the baked field grid — the scale factor world
    // positions are mapped through. Zero if no field grid is baked yet (mirrors Resolution()'s own
    // zero-when-unbaked contract).
    float PixelsPerPreviewCell() const;   // PreviewComposite_Prepare_UI.cpp — mapFields.VertexSize()-derived

    // World (positionX/positionZ — the horizontal plane; positionY is height,
    // PlacementInstance_DATA) -> preview pixel. The exact mapping BuildEntityPoints already bakes
    // marks through; extracted so there is exactly one copy (ARCH_08_03_SpatialGridVsSpacingGrid.md
    // §8.3's "one copy" principle, same class of rule as Data::SpatialGrid::CellIndexAt).
    PreviewPixelPoint WorldToPreviewPixel(float worldX, float worldZ) const;
    // Inverse — preview pixel -> world. New; BuildEntityPoints never needed this direction, STEP48's
    // picking migration does. Exact inverse of WorldToPreviewPixel only when PixelsPerPreviewCell()
    // > 0; on an unbaked composite it answers (0, 0) via ReciprocalOrZero — callers must check
    // PixelsPerPreviewCell() > 0 before trusting a picked world position, same discipline
    // ResolvePreviewPixel's bInsideImage already requires callers to observe.
    PreviewWorldPoint PreviewPixelToWorld(float pixelX, float pixelY) const;

    // Runs the pass sequence on the Gpu when a resource manager with a live context is
    // available, else on the Cpu twin. Reports which one it used, rather than silently
    // producing nothing. See ComposeRequest above for what `request` controls; the Cpu twin never
    // uploads a buffer of any kind, so it takes no request of its own.
    void Compose(ComposeRequest request = ComposeRequest());
    void ComposeOnCpu();   // PreviewComposite_Cpu_UI.cpp -- texels ARE the primary output, always needed.
    // `outTiming` (ARCH §14.18 item 10; STEP218) — Gpu-path-specific observability, same spirit as
    // LastRunUsedGpu()/ExecutedPassCount() already being Gpu-path-only accessors on this class. Null
    // by default: every existing call site (including Compose()'s own forwarding call, unchanged
    // below) pays no cost beyond a handful of `!= nullptr` branch checks — no clock read, no write,
    // no behavior change of any kind when omitted.
    void ComposeOnGpu(ComposeRequest request = ComposeRequest(), ComposeGpuTiming* outTiming = nullptr);   // PreviewComposite_Gpu_UI.cpp

    // The composited image: `Resolution()` squared packed RGBA8 texels, row-major.
    const std::vector<unsigned int>& CompositeTexels() const { return compositeTexels; }
    // The same image as the GL texture the last Gpu run wrote — what a canvas draws. Invalid
    // until a Gpu compose has run (the Cpu twin has only the texels above).
    Sys::GpuTextureHandle CompositeTexture() const { return compositeTexture; }
    int Resolution() const { return configuration.previewResolution; }
    bool LastRunUsedGpu() const { return bLastRunUsedGpu; }
    // ARCH §14.18 item 19 — the ONE place that measures BOTH backends, including the two silent
    // fallbacks to the Cpu twin inside ComposeOnGpu (no GL program, no texture) — those fallbacks
    // are invisible to ComposeGpuTiming by construction, and are exactly the catastrophic per-frame
    // case the Tier-B2 watchdog (AreaRecompositeThrottle_UI.h) exists to catch. Brackets the WHOLE
    // Compose() call, including PrepareRun() — deliberately wider than ComposeGpuTiming's own
    // three phases, closing STEP218's own measured blind spot (item 17). Always-on (two clock reads
    // per compose, ROUGH-ESTIMATE ~40-60ns against a >=1ms compose): a gate here would make the
    // safety floor's own input conditional, which is the exact class of bug item 7 already warns
    // against on a hot path.
    double LastComposeMillis() const { return lastComposeMillis; }
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
    void BuildMapAreaConfigurations();                       // PreviewComposite_Prepare_UI.cpp
    void BuildEntityPoints();                                // PreviewComposite_Prepare_UI.cpp
    const Data::FloatField* LayerSourceField(PreviewLayerKind kind) const;
    bool EnsureGpuResources();                               // PreviewComposite_GpuProgram_UI.cpp
    void PackSurfaceStratumWeights();                        // PreviewComposite_GpuBuffers_UI.cpp
    bool EnsureCompositeTexture(Sys::GpuResourceManager& manager);  // PreviewComposite_GpuBuffers_UI.cpp
    // `bBakedInputsChanged` (ARCH §14.18 item 6) — gates PackSurfaceStratumWeights() and the five
    // baked-input uploads only; every other upload in this function stays unconditional.
    void BindComposeBuffers(Sys::GpuResourceManager& manager, bool bBakedInputsChanged);   // PreviewComposite_GpuBuffers_UI.cpp

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
    const std::vector<Params::MapArea>& areas;
    const Data::MapFields&              mapFields;
    const Data::PlacementInstances&     instances;
    Data::EntityIdBuffer&               entityIdentifierBuffer;
    PreviewCompositeSettings            settings;

    PreviewCompositeConfiguration            configuration;
    std::vector<PreviewLayerConfiguration>   layerConfigurations;
    std::vector<PreviewStratumConfiguration> stratumConfigurations;
    std::vector<PreviewMapAreaRectangle>     mapAreaRectangles;
    std::vector<PreviewEntityPoint>          entityPoints;
    std::vector<float>                       gradientLookupTables;  // every ramp, concatenated
    std::vector<float>                       packedSurfaceWeights;  // the 9 fields, concatenated
    std::vector<unsigned int>                compositeTexels;

    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::GpuTextureHandle    compositeTexture;
    bool bLastRunUsedGpu  = false;
    double lastComposeMillis = 0.0;   // ARCH §14.18 item 19 — see LastComposeMillis() above.
    bool bGpuProgramReady = false;
    int  gpuProgramIndex  = -1;
    int  executedPassCount = 0;
};

} // namespace Ui
} // namespace SanmapGen

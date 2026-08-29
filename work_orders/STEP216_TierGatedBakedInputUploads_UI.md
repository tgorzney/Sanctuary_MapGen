# STEP216 — Tier-gate the composite's baked-input uploads (`ComposeRequest`, `RefreshTier`-aware callback, self-defending `EnsureAndBind`)

**Layer:** UI (+ one PIPELINE header/source pair). **Domain:** `PreviewComposite`'s GPU upload path (`PreviewComposite_GpuBuffers_UI.cpp`), `Compose()`'s public signature, `Pipeline::PreviewDriver`'s compose callback. **Executor:** SanGen Coder. Authored by the SanGen UI Expert, per `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` (§14.18 Part 1, items 1-10) and `ARCH_14_08_DirtyFlagTiers.md`'s new Tier B2 row. Every file cited here was read directly against the live tree while drafting — no forward-looking/not-yet-landed prerequisites. This is ARCH §14.18's dispatchable **Piece A** — pure performance, zero visual change, independently shippable, and it does **not** land Piece C (the one-fill/drop-suppression change, blocked on a GPU benchmark that hasn't run yet).

## Summary
`BindComposeBuffers` currently re-packs (`PackSurfaceStratumWeights()`, a multi-megabyte memcpy) and re-uploads every baked field on **every** compose, whether or not a stage ran — a pre-existing performance debt that file's own header comment already named as owed work. §14.18 items 1-10 rule the fix: `PreviewDriver` already knows, per-refresh, whether any stage ran (`RefreshTier::PreviewRender` means "no stage runs, so nothing re-simulates," `PreviewDriver_PIPELINE.h:5-7`) — this ticket threads that already-existing signal down into the composite's own upload path, inventing no new dirty state. `Compose()` gains a `ComposeRequest` options struct whose defaults reproduce today's behavior byte-for-byte; `EnsureAndBind` self-defends via `GpuResourceManager::EnsureBuffer`'s own reallocation report, so a wrongly-wired gate can only cost a redundant upload, never leave a buffer stale. This ticket changes **zero observable behavior** — every production call site (`Application_UI.cpp`) is the only one that actually flips the new flag to `false`, and it does so only when `PreviewDriver` reports the tier that provably left the bake untouched.

## Required reading
`ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` Part 1 (items 1-10 — read in full; this ticket implements items 6-7 specifically, but 1-5/8-9 explain WHY the upload gate had to exist before Piece C can land, so do not skip them), `ARCH_14_08_DirtyFlagTiers.md` (the Tier B2 row and its "B2 vs C2" note).

---

## 1. Modified: `src/ui/PreviewComposite_UI.h`

**New nested struct** — insert directly after `PreviewPixelPoint` (currently lines 40-42), before the constructor (currently line 44):

Currently:
```cpp
    // A world-space point on the horizontal plane (positionX/positionZ — positionY is height,
    // PlacementInstance_DATA).
    struct PreviewWorldPoint  { float worldX = 0.0f; float worldZ = 0.0f; };
    // A point in preview-pixel space (the composited image's own pixel grid).
    struct PreviewPixelPoint  { float pixelX = 0.0f; float pixelY = 0.0f; };

    PreviewComposite(const Params::Geometry& geometrySettings, const Params::Water& waterSettings,
```

New:
```cpp
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

    PreviewComposite(const Params::Geometry& geometrySettings, const Params::Water& waterSettings,
```

**`Compose`/`ComposeOnGpu` signatures** — replace (currently lines 72-81):

Currently:
```cpp
    // Runs the pass sequence on the Gpu when a resource manager with a live context is
    // available, else on the Cpu twin. Reports which one it used, rather than silently
    // producing nothing.
    // `bNeedsTexelReadback`: when false, skips only the Gpu texture->CPU `CompositeTexels()`
    // readback (the entity-id readback used for click-picking is unaffected either way).
    // Production leaves it true unless it knows nothing reads `CompositeTexels()` this run,
    // since the canvas draws `CompositeTexture()` (the GL handle) directly on the Gpu path.
    void Compose(bool bNeedsTexelReadback = true);
    void ComposeOnCpu();   // PreviewComposite_Cpu_UI.cpp -- texels ARE the primary output, always needed.
    void ComposeOnGpu(bool bNeedsTexelReadback = true);   // PreviewComposite_Gpu_UI.cpp
```

New:
```cpp
    // Runs the pass sequence on the Gpu when a resource manager with a live context is
    // available, else on the Cpu twin. Reports which one it used, rather than silently
    // producing nothing. See ComposeRequest above for what `request` controls; the Cpu twin never
    // uploads a buffer of any kind, so it takes no request of its own.
    void Compose(ComposeRequest request = ComposeRequest());
    void ComposeOnCpu();   // PreviewComposite_Cpu_UI.cpp -- texels ARE the primary output, always needed.
    void ComposeOnGpu(ComposeRequest request = ComposeRequest());   // PreviewComposite_Gpu_UI.cpp
```

**`BindComposeBuffers` declaration** — replace (currently line 111):

Currently:
```cpp
    void BindComposeBuffers(Sys::GpuResourceManager& manager);      // PreviewComposite_GpuBuffers_UI.cpp
```

New:
```cpp
    // `bBakedInputsChanged` (ARCH §14.18 item 6) — gates PackSurfaceStratumWeights() and the five
    // baked-input uploads only; every other upload in this function stays unconditional.
    void BindComposeBuffers(Sys::GpuResourceManager& manager, bool bBakedInputsChanged);   // PreviewComposite_GpuBuffers_UI.cpp
```

## 2. Modified: `src/ui/PreviewComposite_UI.cpp`

Replace `Compose` (currently lines 31-37):

Currently:
```cpp
// Gpu is the composite's backend (ARCH §4.2 "preview color = Gpu / Visual"); the Cpu twin runs
// when no resource manager was handed in, so a headless caller still gets a correct image
// instead of nothing.
void PreviewComposite::Compose(bool bNeedsTexelReadback) {
    if (gpuResourceManager != nullptr) { ComposeOnGpu(bNeedsTexelReadback); return; }
    ComposeOnCpu();
}
```

New:
```cpp
// Gpu is the composite's backend (ARCH §4.2 "preview color = Gpu / Visual"); the Cpu twin runs
// when no resource manager was handed in, so a headless caller still gets a correct image
// instead of nothing. ARCH §14.18 items 6-7 — `request` replaces the old lone `bNeedsTexelReadback`
// bool; its default reproduces every existing caller's behavior exactly (both flags true: a full
// re-upload, texels read back).
void PreviewComposite::Compose(ComposeRequest request) {
    if (gpuResourceManager != nullptr) { ComposeOnGpu(request); return; }
    ComposeOnCpu();
}
```

## 3. Modified: `src/ui/PreviewComposite_Gpu_UI.cpp` (full file)

```cpp
// PreviewComposite_Gpu_UI.cpp — the Gpu path's pass sequence: bind, dispatch clear -> one pass
// per enabled field layer -> overlay -> entity id, fence, read the image + ids back.
// Layer: UI. Everything GL goes through `Sys::GpuResourceManager` (ARCH §3.2, §5.4): no private
// GL pipeline, no shader path here, no GL handle — only the opaque handles the SYS seam
// exposes. Program compilation lives in PreviewComposite_GpuProgram_UI.cpp and buffer packing
// in PreviewComposite_GpuBuffers_UI.cpp, behind the same header.
// With no GL context/manager the composite falls back to its Cpu twin and reports Cpu, rather
// than silently producing nothing.
#include "PreviewComposite_UI.h"
#include "../sys/GpuResource_SYS.h"
#include <thread>

namespace SanmapGen {
namespace Ui {
namespace {

// The fence poll is an early-out, not a correctness requirement (the readback is ordered after
// the dispatch by GL itself), so it yields instead of burning a core.
constexpr int fencePollLimit = 100000;

unsigned TileGroupCount(int extent, int tileExtent) {
    return extent > 0 ? static_cast<unsigned>((extent + tileExtent - 1) / tileExtent) : 0u;
}

void WaitForCompletion(Sys::GpuResourceManager& manager) {
    const Sys::GpuFenceHandle fence = manager.InsertFence();
    for (int poll = 0; poll < fencePollLimit && !manager.IsFenceSignaled(fence); ++poll)
        std::this_thread::yield();
    manager.DeleteFence(fence);
}

} // namespace

void PreviewComposite::ComposeOnGpu(ComposeRequest request) {
    PrepareRun();
    const int resolution = configuration.previewResolution;
    if (resolution <= 0) return;
    if (!EnsureGpuResources()) { ComposeOnCpu(); return; }        // no GL -> the Cpu twin

    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    if (!EnsureCompositeTexture(manager)) { ComposeOnCpu(); return; }   // no image -> the Cpu twin
    BindComposeBuffers(manager, request.bBakedInputsChanged);

    const unsigned pixelGroupsX = TileGroupCount(resolution, Sys::WorkgroupSize::kFieldTileWidth);
    const unsigned pixelGroupsY = TileGroupCount(resolution, Sys::WorkgroupSize::kFieldTileHeight);
    const int threadsPerGroup =
        Sys::WorkgroupSize::kFieldTileWidth * Sys::WorkgroupSize::kFieldTileHeight;
    const unsigned entityGroups = TileGroupCount(configuration.entityCount, threadsPerGroup);

    manager.SetUniformInt(program, "passIndex", CompositePass::kClear);
    manager.Dispatch(program, pixelGroupsX, pixelGroupsY, 1);
    ++executedPassCount;

    manager.SetUniformInt(program, "passIndex", CompositePass::kFieldLayer);
    for (int layerIndex = 0; layerIndex < configuration.layerCount; ++layerIndex) {
        manager.SetUniformInt(program, "layerIndex", layerIndex);
        manager.Dispatch(program, pixelGroupsX, pixelGroupsY, 1);
        ++executedPassCount;
    }

    // The two entity passes run one thread per RESOLVED instance, not per pixel. With no
    // instances they are counted (the sequence is the same) but never dispatched — a zero-group
    // dispatch is not a legal GL call.
    for (int entityPass = CompositePass::kOverlay; entityPass <= CompositePass::kEntityIdentifier;
         ++entityPass) {
        if (entityGroups > 0u) {
            manager.SetUniformInt(program, "passIndex", entityPass);
            manager.Dispatch(program, entityGroups, 1, 1);
        }
        ++executedPassCount;
    }

    WaitForCompletion(manager);
    // The canvas draws the texture itself; this readback exists so the Cpu twin stays the parity
    // reference and a headless caller still gets bytes. RGBA8 texels come back R,G,B,A per pixel,
    // which is exactly the byte order `Ui::PackRgba8` packs into one unsigned int. Skipped when
    // the caller knows nothing will read `CompositeTexels()` this run (e.g. the production
    // hot path, where the canvas samples `CompositeTexture()` directly).
    if (request.bNeedsTexelReadback) {
        manager.ReadbackTexture(compositeTexture, compositeTexels.data(),
                                compositeTexels.size() * sizeof(unsigned int));
    }
    // Unaffected by EITHER `request` flag: `MapCanvas::ApplyClick` reads this unconditionally on
    // both backends for click-picking, on every recomposite (ARCH §14.18 item 7's own note —
    // skipping this readback is a picking-correctness argument this ruling explicitly does NOT
    // make; it is named there as the next lever if the item-10 benchmark misses, not in scope here).
    manager.ReadbackBuffer(CompositeBufferName::kEntityIdentifiers, entityIdentifierBuffer.Data(),
                           entityIdentifierBuffer.CellCount() * sizeof(unsigned int));
    bLastRunUsedGpu = true;
}

} // namespace Ui
} // namespace SanmapGen
```

## 4. Modified: `src/ui/PreviewComposite_GpuBuffers_UI.cpp` (full file)

```cpp
// PreviewComposite_GpuBuffers_UI.cpp — packing and binding the composite's persistent buffers.
// Layer: UI. Every buffer is owned and keyed by `Sys::GpuResourceManager`, which reallocates
// only when a named buffer's byte size changes, so a re-composite at the same resolution costs
// uploads and no allocations. ARCH §14.18 items 6-7 — the baked-input uploads (the weight pack's
// memcpy, heightfield, flow, accumulation, slope, surface-stratum-weights) are now GATED on
// `bBakedInputsChanged`, threaded down from `PreviewComposite::ComposeRequest` through
// `ComposeOnGpu`. Everything else in this file re-uploads unconditionally, every compose, per
// §14.18 item 7's own explicit ruling: `BuildEntityPoints()`'s `bEntitiesEnabled` toggle and the
// ramp/layer/stratum/area records are all presentation state a single-frame edit must move
// immediately, and gating any of them would silently break the feature that edits it.
#include "PreviewComposite_UI.h"
#include "../sys/GpuResource_SYS.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

const float zeroFloat = 0.0f;

// GL will not allocate a zero-byte buffer, so an empty input binds one placeholder word; the
// kernel never reads it, because the count that would index it is zero.
// `bUploadRequested` is normally the caller's own gate (ARCH §14.18 item 6), but this function
// SELF-DEFENDS regardless of what the caller passes: `Sys::GpuResourceManager::EnsureBuffer`
// reports when it just (re)allocated, and a freshly (re)allocated buffer has no contents — so the
// upload runs whenever `bUploadRequested || bReallocated`. A gate wired wrongly can only cost one
// redundant upload; it can never leave a buffer stale.
void EnsureAndBind(Sys::GpuResourceManager& manager, const char* bufferName, const void* data,
                   std::size_t byteSize, unsigned bindingIndex, bool bUploadRequested) {
    if (byteSize == 0) { data = &zeroFloat; byteSize = sizeof(float); }
    const bool bReallocated = manager.EnsureBuffer(bufferName, byteSize);
    if (data != nullptr && (bUploadRequested || bReallocated))
        manager.UploadBuffer(bufferName, data, byteSize);
    manager.BindBuffer(bufferName, bindingIndex);
}

void EnsureAndBindField(Sys::GpuResourceManager& manager, const char* bufferName,
                        const Data::FloatField& field, unsigned bindingIndex, bool bUploadRequested) {
    EnsureAndBind(manager, bufferName, field.IsEmpty() ? nullptr : field.Data(),
                  field.CellCount() * sizeof(float), bindingIndex, bUploadRequested);
}

} // namespace

// The nine baked surface-weight fields, concatenated in stratum order — the same packing the
// bake uses, so both read weight `[stratum * cellCount + cell]`. ARCH §14.18 item 6 — the caller
// (BindComposeBuffers) skips this multi-megabyte memcpy entirely when the baked inputs have not
// changed since the last compose.
void PreviewComposite::PackSurfaceStratumWeights() {
    const std::size_t cellCount = mapFields.surfaceStratumWeights[0].CellCount();
    packedSurfaceWeights.assign(cellCount * Data::MapFields::stratumCount, 0.0f);
    if (cellCount == 0) return;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const Data::FloatField& weights = mapFields.surfaceStratumWeights[stratum];
        if (weights.CellCount() != cellCount) continue;      // unsized stratum stays at zero
        std::memcpy(packedSurfaceWeights.data() + static_cast<std::size_t>(stratum) * cellCount,
                    weights.Data(), cellCount * sizeof(float));
    }
}

// The composited image is a real GL_RGBA8 texture bound to an image unit, not an SSBO of packed
// uints: the canvas samples this exact surface (M5-5), so the composite writes it once instead of
// producing bytes someone else re-uploads. `EnsureTexture` reallocates only when the preview
// resolution changes, and the passes read-modify-write it, so the access is ReadWrite.
bool PreviewComposite::EnsureCompositeTexture(Sys::GpuResourceManager& manager) {
    compositeTexture = manager.EnsureTexture(CompositeTextureName::kCompositeImage,
                                             configuration.previewResolution,
                                             configuration.previewResolution,
                                             Sys::GpuTextureFormat::Rgba8);
    if (!compositeTexture.IsValid()) return false;
    manager.BindTextureImage(compositeTexture, CompositeImageUnit::kCompositeImage,
                             Sys::GpuImageAccess::ReadWrite);
    return true;
}

// Three baked inputs, the weight pack, the ramp tables, the entity points and the three record
// buffers in; the entity ids out (the image goes to the texture above). Binding indices are the
// kernel contract. `bBakedInputsChanged` (ARCH §14.18 item 6) gates ONLY
// PackSurfaceStratumWeights() and the five baked-input uploads named below — every other upload in
// this function is unconditional (item 7's own explicit "what must NOT be gated" list).
void PreviewComposite::BindComposeBuffers(Sys::GpuResourceManager& manager, bool bBakedInputsChanged) {
    if (bBakedInputsChanged) PackSurfaceStratumWeights();
    EnsureAndBind(manager, CompositeBufferName::kEntityIdentifiers, nullptr,
                  entityIdentifierBuffer.CellCount() * sizeof(unsigned int),
                  CompositeBinding::kEntityIdentifiers, /*bUploadRequested=*/true);
    EnsureAndBindField(manager, CompositeBufferName::kHeightfield, mapFields.heightfield,
                       CompositeBinding::kHeightfield, bBakedInputsChanged);
    EnsureAndBindField(manager, CompositeBufferName::kFlow, mapFields.flow, CompositeBinding::kFlow,
                       bBakedInputsChanged);
    EnsureAndBindField(manager, CompositeBufferName::kAccumulation, mapFields.accumulation,
                       CompositeBinding::kAccumulation, bBakedInputsChanged);
    EnsureAndBindField(manager, CompositeBufferName::kSlope, mapFields.slope, CompositeBinding::kSlope,
                       bBakedInputsChanged);
    EnsureAndBind(manager, CompositeBufferName::kSurfaceStratumWeights, packedSurfaceWeights.data(),
                  packedSurfaceWeights.size() * sizeof(float),
                  CompositeBinding::kSurfaceStratumWeights, bBakedInputsChanged);
    EnsureAndBind(manager, CompositeBufferName::kGradientLookupTables, gradientLookupTables.data(),
                  gradientLookupTables.size() * sizeof(float),
                  CompositeBinding::kGradientLookupTables, /*bUploadRequested=*/true);
    EnsureAndBind(manager, CompositeBufferName::kEntityPoints, entityPoints.data(),
                  entityPoints.size() * sizeof(PreviewEntityPoint), CompositeBinding::kEntityPoints,
                  /*bUploadRequested=*/true);
    EnsureAndBind(manager, CompositeBufferName::kConfiguration, &configuration,
                  sizeof(PreviewCompositeConfiguration), CompositeBinding::kConfiguration,
                  /*bUploadRequested=*/true);
    EnsureAndBind(manager, CompositeBufferName::kLayerConfigurations, layerConfigurations.data(),
                  layerConfigurations.size() * sizeof(PreviewLayerConfiguration),
                  CompositeBinding::kLayerConfigurations, /*bUploadRequested=*/true);
    EnsureAndBind(manager, CompositeBufferName::kStratumConfigurations, stratumConfigurations.data(),
                  stratumConfigurations.size() * sizeof(PreviewStratumConfiguration),
                  CompositeBinding::kStratumConfigurations, /*bUploadRequested=*/true);
    EnsureAndBind(manager, CompositeBufferName::kMapAreaRectangles, mapAreaRectangles.data(),
                  mapAreaRectangles.size() * sizeof(PreviewMapAreaRectangle),
                  CompositeBinding::kMapAreaRectangles, /*bUploadRequested=*/true);
}

} // namespace Ui
} // namespace SanmapGen
```

## 5. Modified: `src/pipeline/PreviewDriver_PIPELINE.h`

**`SetPreviewCompositeCallback`** — replace (currently lines 35-37):

Currently:
```cpp
    void SetPreviewCompositeCallback(std::function<void()> composePreview) {
        previewCompositeCallback = std::move(composePreview);
    }
```

New:
```cpp
    // ARCH §14.18 item 6 — the callback now receives the tier it is servicing, so a UI-side
    // composite can gate its own baked-input uploads on it. `RefreshTier::PreviewRender` means "no
    // stage ran, so nothing re-simulates" (this file's own header comment, above) — the invariant
    // this whole wiring rests on: every `MapUpdate` refresh composites immediately after the stages
    // run, so a SUBSEQUENT `PreviewRender` compose is provably looking at byte-identical baked
    // fields.
    void SetPreviewCompositeCallback(std::function<void(RefreshTier)> composePreview) {
        previewCompositeCallback = std::move(composePreview);
    }
```

**Private section** — replace (currently lines 56-64):

Currently:
```cpp
private:
    void CacheStageParameterHashes();
    void RunPreviewComposite();

    GenerationAssembler&     assembler;
    std::function<void()>    previewCompositeCallback;
    std::vector<std::size_t> cachedStageParameterHashes;
    std::vector<std::string> stagesThatRanLastRefresh;
    std::string              owningStageName;
```

New:
```cpp
private:
    void CacheStageParameterHashes();
    void RunPreviewComposite(RefreshTier tier);

    GenerationAssembler&              assembler;
    std::function<void(RefreshTier)>  previewCompositeCallback;
    std::vector<std::size_t>          cachedStageParameterHashes;
    std::vector<std::string>          stagesThatRanLastRefresh;
    std::string                       owningStageName;
```

## 6. Modified: `src/pipeline/PreviewDriver_PIPELINE.cpp` (full file)

```cpp
// PreviewDriver_PIPELINE.cpp — the derivation and the two service paths.
#include "PreviewDriver_PIPELINE.h"

namespace SanmapGen {
namespace Pipeline {

PreviewDriver::PreviewDriver(GenerationAssembler& generationAssembler)
    : assembler(generationAssembler) {
    CacheStageParameterHashes();
}

void PreviewDriver::CacheStageParameterHashes() {
    cachedStageParameterHashes.resize(assembler.StageCount());
    for (std::size_t index = 0; index < cachedStageParameterHashes.size(); ++index)
        cachedStageParameterHashes[index] = assembler.ComputeStageParameterHash(index);
}

// THE derivation (no per-widget list): a parameter belongs to whichever stage's own hash it
// moves, because that hash IS the stage's declaration of what it consumes. The first stage that
// claims the edit is also the stage the dirty-hash conductor will re-run from, so its name is
// reported as well. An edit no stage claims cannot alter a generated field — by construction it
// is presentation (a gradient ramp, a tint, the preview resolution) and the composite alone
// services it.
RefreshTier PreviewDriver::NotifyParametersChanged() {
    owningStageName.clear();
    for (std::size_t index = 0; index < cachedStageParameterHashes.size(); ++index) {
        if (assembler.ComputeStageParameterHash(index) == cachedStageParameterHashes[index])
            continue;
        owningStageName = assembler.StageDescriptions()[index].name;
        bNeedsMapUpdate = true;
        return RefreshTier::MapUpdate;
    }
    bNeedsPreviewRender = true;
    return RefreshTier::PreviewRender;
}

// The map tier wins when both are pending: its own composite already carries the visual edit.
// The cached hashes are refreshed only AFTER the pipeline ran, so an edit made between a notify
// and a refresh is still seen as pending rather than silently swallowed.
RefreshTier PreviewDriver::Refresh() {
    if (bNeedsMapUpdate) {
        stagesThatRanLastRefresh = assembler.Run();
        ++pipelineRunCount;
        CacheStageParameterHashes();
        bNeedsMapUpdate = false;
        bNeedsPreviewRender = false;
        RunPreviewComposite(RefreshTier::MapUpdate);
        return RefreshTier::MapUpdate;
    }
    if (bNeedsPreviewRender) {
        stagesThatRanLastRefresh.clear();   // no stage ran: the bake is reused as-is
        bNeedsPreviewRender = false;
        RunPreviewComposite(RefreshTier::PreviewRender);
        return RefreshTier::PreviewRender;
    }
    return RefreshTier::Nothing;
}

// A headless caller (no composite bound) still resolves its flags correctly — it just has no
// image to show, rather than a driver that refuses to run (Constitution §6). `tier` is exactly
// the tier this Refresh() call is servicing (ARCH §14.18 item 6) — never re-derived here.
void PreviewDriver::RunPreviewComposite(RefreshTier tier) {
    if (!previewCompositeCallback) return;
    previewCompositeCallback(tier);
    ++previewCompositeCount;
}

} // namespace Pipeline
} // namespace SanmapGen
```

## 7. Modified: `src/ui/Application_UI.cpp`

Replace the one line (currently line 75):

Currently:
```cpp
    previewDriver.SetPreviewCompositeCallback([this] { composite.Compose(/*bNeedsTexelReadback=*/false); });
```

New:
```cpp
    // ARCH §14.18 item 6 — the production hot path never needs the texel readback (the canvas
    // samples CompositeTexture() directly), and now also gates the baked-input uploads: a
    // PreviewRender-tier refresh provably left the bake untouched (PreviewDriver_PIPELINE.h's own
    // invariant), so it skips re-packing/re-uploading heightfield/flow/accumulation/slope/
    // surface-stratum-weights. A MapUpdate-tier refresh always re-uploads (the bake just changed).
    previewDriver.SetPreviewCompositeCallback([this](Pipeline::RefreshTier tier) {
        composite.Compose({ /*bNeedsTexelReadback=*/false,
                            /*bBakedInputsChanged=*/tier == Pipeline::RefreshTier::MapUpdate });
    });
```

## 8. Modified: `src/ui/PreviewIntegration_TestScene_UI.h`

Replace the one line (currently line 34):

Currently:
```cpp
        driver.SetPreviewCompositeCallback([this] { composite.Compose(); });
```

New:
```cpp
        // Test scaffolding: always a full-fidelity compose regardless of tier — this fixture tests
        // pipeline<->composite WIRING, not the upload gate (that is PreviewComposite_Gpu_UI_Test.cpp's
        // job, §9 below).
        driver.SetPreviewCompositeCallback([this](Pipeline::RefreshTier) { composite.Compose(); });
```

## 9. Modified: `src/ui/PreviewComposite_Gpu_UI_Test.cpp`

New test function — insert after `CheckAllBlendModesParity` (currently ends at line 183), before the closing `} // namespace`:

```cpp
// ARCH §14.18 items 6-7 — the ONE thing this ticket must prove: a gated recompose over UNCHANGED
// baked fields is byte-identical to a full re-upload. The gate is a pure efficiency change; this
// is its own dedicated acceptance test, exactly as the ruling's own dispatch note calls for
// ("Piece A ... testable on its own by asserting the composite's texels are identical with and
// without the flag").
void CheckGatedUploadParity(Sys::GpuResourceManager& manager) {
    Ui::PreviewTestScene scene;
    BuildVariedScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas,
                                   scene.fields, scene.instances, scene.entityIdentifiers);
    ConfigureVariedSettings(composite.Settings());
    composite.SetGpuResourceManager(&manager);

    composite.Compose();   // default request: full upload, full readback — establishes the baseline
    const std::vector<unsigned int> fullUploadTexels = composite.CompositeTexels();

    // Same composite, same unchanged baked fields, uploads gated OFF: must reproduce the exact
    // same image, never an approximation of it (§14.18 item 1's "no second implementation" law
    // applies just as much to a skipped upload as to a rival draw call).
    composite.Compose({ /*bNeedsTexelReadback=*/true, /*bBakedInputsChanged=*/false });
    check(fullUploadTexels == composite.CompositeTexels(),
          "a gated recompose (bBakedInputsChanged=false) over unchanged baked fields reproduces the "
          "exact same image as a full re-upload — the gate changes zero observable behavior");
}
```

**`main()`** — add the call (currently lines 187-206):

Currently:
```cpp
    Sys::GpuResourceManager manager(shaderDirectory);
    check(manager.Initialize(), "the Gpu resource manager initializes");
    CheckGpuPathAndParity(manager);
    CheckAllBlendModesParity(manager);
```

New:
```cpp
    Sys::GpuResourceManager manager(shaderDirectory);
    check(manager.Initialize(), "the Gpu resource manager initializes");
    CheckGpuPathAndParity(manager);
    CheckAllBlendModesParity(manager);
    CheckGatedUploadParity(manager);
```

No `CMakeLists.txt` change — this is an addition to an already-registered test binary.

---

## ARCH rules invoked
- `ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` items 1-10 (Part 1) — this ticket's entire binding law: the dirty tier is the signal, no new dirty state invented; the `ComposeRequest` shape and its defaults; `EnsureAndBind`'s self-defense; the exact gated-vs-not-gated split.
- `ARCH_14_08_DirtyFlagTiers.md`'s Tier B2 row — names this ticket as its own enabling prerequisite ("Bounded by §14.18 item 10's benchmark gate").
- Constitution §6 — `EnsureAndBind`'s self-defense (`bUploadRequested || bReallocated`) refuses to trust a caller-supplied flag when the buffer's own state disagrees with it.
- ARCH §3.1 — `PreviewDriver` (PIPELINE) still knows nothing about `PreviewComposite` (UI); it only passes the `RefreshTier` it already computed through the existing injected callback. No new PIPELINE→UI or UI→PIPELINE coupling is introduced.

## Explicit out-of-scope
- **Piece C (the one-fill/drop-suppression change, §14.18 items 3/4/8/9)** — not authored here. It is blocked on the item-10 GPU benchmark, which has not run. `mapAreaSuppressedIndex`, `SetMapAreaSuppression`, the border-rule amendment, and the per-moving-frame recomposite request are all untouched by this ticket.
- **Item 7's last bullet — `WaitForCompletion`'s fence spin and the unconditional entity-id readback.** Named explicitly by the ruling as "in scope for a later ticket, not this one." Both stay unconditional in every code block above.
- **The item-10 benchmark itself** — owned by the SanGen Compute Optimization Expert (GPU/upload path) and the SanGen UI Optimization Expert (frame-budget review), not this ticket.
- **No change to `PreviewCompositeSettings::mapAreaSuppressedIndex`, `AreaColorTable_UI.h`, or any Areas-tab file** — that is STEP217's territory (independent, ARCH §14.18's own "(B) may land any time").
- **No change to `PreviewComposite_Cpu_UI.cpp`** — the Cpu twin never uploads a GPU buffer, so `ComposeRequest::bBakedInputsChanged` has nothing to gate there; it takes no request parameter at all (unchanged signature).

## Acceptance test
1. Full `SanGenV2` build stays clean.
2. Every existing test continues to pass **unmodified** (`PreviewComposite_UI_Test.cpp`, `PreviewComposite_Wysiwyg_UI_Test.cpp`, `PreviewComposite_MapAreas_UI_Test.cpp`, `MapCanvas_*_UI_Test.cpp`, `PreviewIntegration_*_UI_Test.cpp`, `ParameterTabs_DirtyTier_UI_Test.cpp`, `ApplicationShell_*_UI_Test.cpp`, etc.) — none of them call `Compose(bool)` positionally or reference the old `std::function<void()>` callback shape, so none require source edits.
3. `PreviewComposite_Gpu_UI_Test.cpp`'s existing `manager.CompileCount() == 1` assertion is unaffected (buffer upload gating never touches program compilation).
4. New `CheckGatedUploadParity` passes: `bBakedInputsChanged=false` over an unchanged bake produces byte-identical `CompositeTexels()` to a full upload.
5. `ALL PASS` from `PreviewComposite_Gpu_UI_Test`.

## Interpretation calls made beyond §14.18's ratified text
1. **`ComposeRequest` is declared as a public nested struct `PreviewComposite::ComposeRequest`**, beside the already-nested `PreviewWorldPoint`/`PreviewPixelPoint`, rather than a free struct at `Ui` namespace scope. The ruling's own text says only "Compose takes an options struct," not where it lives; nesting matches this file's own existing convention for structs that exist only to be `Compose()`'s own vocabulary.
2. **`EnsureAndBind`/`EnsureAndBindField`'s new `bool bUploadRequested` trailing parameter** is a plain bool on an already-`TU`-local, anonymous-namespace helper with zero external callers — not a second struct. The ruling's "two bools in a row is a legibility trap" is about the **public** `Compose()` API; a private helper taking `(..., unsigned bindingIndex, bool bUploadRequested)` is not that trap (there is exactly one bool, not two, and it is the helper's own single reason to exist).
3. **Added `CheckGatedUploadParity`** as a new test function rather than leaving Piece A's acceptance methodology unverified — the ruling's own dispatch note explicitly names texel-identity-with-and-without-the-flag as the way to test this piece independently; this is that test, not an invention beyond ARCH's text.
4. **`Application_UI.cpp`'s lambda** computes `bBakedInputsChanged` as `tier == RefreshTier::MapUpdate` rather than `tier != RefreshTier::PreviewRender` — behaviorally identical today (the callback is only ever invoked for `MapUpdate` or `PreviewRender`, never `Nothing`, per `PreviewDriver::Refresh()`'s own two call sites), but written as an equality-against-the-tier-that-means-"re-upload" rather than an inequality-against-the-tier-that-means-"skip," on the theory that a future third tier should default to the SAFE (full-upload) side of an `==` check, not the unsafe side of a `!=` check.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_14_18_AreaLiveBlendFidelityAndPalette.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_14_08_DirtyFlagTiers.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_14_17_MapAreaFieldLayer.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Gpu_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_GpuBuffers_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Cpu_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Kernel_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Settings_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Prepare_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Gpu_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewIntegration_TestScene_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\pipeline\PreviewDriver_PIPELINE.h`,
`D:\Projects\Sanctuary\Map Generator\src\pipeline\PreviewDriver_PIPELINE.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\sys\GpuResource_SYS.h`,
`D:\Projects\Sanctuary\Map Generator\work_orders\STEP210_AreaCanvasGesture_UI.md` (format/rigor template).

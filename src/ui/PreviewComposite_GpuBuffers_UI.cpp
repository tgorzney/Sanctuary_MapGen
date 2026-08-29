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

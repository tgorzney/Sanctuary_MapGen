// PreviewComposite_GpuBuffers_UI.cpp — packing and binding the composite's persistent buffers.
// Layer: UI. Every buffer is owned and keyed by `Sys::GpuResourceManager`, which reallocates
// only when a named buffer's byte size changes, so a re-composite at the same resolution costs
// uploads and no allocations. Gating those uploads on the two-tier dirty flags (re-upload the
// baked fields only when the bake changed) is M4-5's wiring, not this unit's decision.
#include "PreviewComposite_UI.h"
#include "../sys/GpuResource_SYS.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

const float zeroFloat = 0.0f;

// GL will not allocate a zero-byte buffer, so an empty input binds one placeholder word; the
// kernel never reads it, because the count that would index it is zero.
void EnsureAndBind(Sys::GpuResourceManager& manager, const char* bufferName, const void* data,
                   std::size_t byteSize, unsigned bindingIndex) {
    if (byteSize == 0) { data = &zeroFloat; byteSize = sizeof(float); }
    manager.EnsureBuffer(bufferName, byteSize);
    if (data != nullptr) manager.UploadBuffer(bufferName, data, byteSize);
    manager.BindBuffer(bufferName, bindingIndex);
}

void EnsureAndBindField(Sys::GpuResourceManager& manager, const char* bufferName,
                        const Data::FloatField& field, unsigned bindingIndex) {
    EnsureAndBind(manager, bufferName, field.IsEmpty() ? nullptr : field.Data(),
                  field.CellCount() * sizeof(float), bindingIndex);
}

} // namespace

// The nine baked surface-weight fields, concatenated in stratum order — the same packing the
// bake uses, so both read weight `[stratum * cellCount + cell]`.
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

// Three baked inputs, the weight pack, the ramp tables, the entity points and the three record
// buffers in; the image and the entity ids out. Binding indices are the kernel contract.
void PreviewComposite::BindComposeBuffers(Sys::GpuResourceManager& manager) {
    PackSurfaceStratumWeights();
    EnsureAndBind(manager, CompositeBufferName::kEntityIdentifiers, nullptr,
                  entityIdentifierBuffer.CellCount() * sizeof(unsigned int),
                  CompositeBinding::kEntityIdentifiers);
    EnsureAndBindField(manager, CompositeBufferName::kHeightfield, mapFields.heightfield,
                       CompositeBinding::kHeightfield);
    EnsureAndBindField(manager, CompositeBufferName::kFlow, mapFields.flow, CompositeBinding::kFlow);
    EnsureAndBindField(manager, CompositeBufferName::kAccumulation, mapFields.accumulation,
                       CompositeBinding::kAccumulation);
    EnsureAndBind(manager, CompositeBufferName::kSurfaceStratumWeights, packedSurfaceWeights.data(),
                  packedSurfaceWeights.size() * sizeof(float),
                  CompositeBinding::kSurfaceStratumWeights);
    EnsureAndBind(manager, CompositeBufferName::kGradientLookupTables, gradientLookupTables.data(),
                  gradientLookupTables.size() * sizeof(float),
                  CompositeBinding::kGradientLookupTables);
    EnsureAndBind(manager, CompositeBufferName::kEntityPoints, entityPoints.data(),
                  entityPoints.size() * sizeof(PreviewEntityPoint), CompositeBinding::kEntityPoints);
    EnsureAndBind(manager, CompositeBufferName::kCompositeTexels, nullptr,
                  compositeTexels.size() * sizeof(unsigned int), CompositeBinding::kCompositeTexels);
    EnsureAndBind(manager, CompositeBufferName::kConfiguration, &configuration,
                  sizeof(PreviewCompositeConfiguration), CompositeBinding::kConfiguration);
    EnsureAndBind(manager, CompositeBufferName::kLayerConfigurations, layerConfigurations.data(),
                  layerConfigurations.size() * sizeof(PreviewLayerConfiguration),
                  CompositeBinding::kLayerConfigurations);
    EnsureAndBind(manager, CompositeBufferName::kStratumConfigurations, stratumConfigurations.data(),
                  stratumConfigurations.size() * sizeof(PreviewStratumConfiguration),
                  CompositeBinding::kStratumConfigurations);
}

} // namespace Ui
} // namespace SanmapGen

// PreviewComposite_Cpu_UI.cpp — the Cpu twin of PreviewComposite_UI.glsl: the same four
// passes, in the same order, over the same flattened records. Layer: UI.
// It exists as the parity reference (a GL context is not needed to prove the composite's
// colors) and as the fallback when the caller has no resource manager. Every value it reads is
// a BAKED field sample or a settings-derived record — no slope, no re-filtered rule, no sim.
// Both twins round-trip the image through RGBA8 between passes, so a layer stack composites
// identically on either backend.
#include "PreviewComposite_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

// The disc one entity mark covers, in preview pixels. Shared by the overlay pass and the
// entity-id pass so a pixel can never take a color without taking the matching id.
template <typename WritePixel>
void ForEachMarkedPixel(const PreviewEntityPoint& point, float radiusPixels, int resolution,
                        WritePixel writePixel) {
    if (!(radiusPixels > 0.0f)) radiusPixels = 0.0f;
    const int lowX = static_cast<int>(point.pixelX - radiusPixels);
    const int lowY = static_cast<int>(point.pixelY - radiusPixels);
    const int highX = static_cast<int>(point.pixelX + radiusPixels) + 1;
    const int highY = static_cast<int>(point.pixelY + radiusPixels) + 1;
    const float radiusSquared = radiusPixels * radiusPixels;
    for (int pixelY = lowY < 0 ? 0 : lowY; pixelY <= highY && pixelY < resolution; ++pixelY) {
        for (int pixelX = lowX < 0 ? 0 : lowX; pixelX <= highX && pixelX < resolution; ++pixelX) {
            const float offsetX = static_cast<float>(pixelX) - point.pixelX;
            const float offsetY = static_cast<float>(pixelY) - point.pixelY;
            if (offsetX * offsetX + offsetY * offsetY > radiusSquared) continue;
            writePixel(static_cast<std::size_t>(pixelY) * resolution + pixelX, pixelX, pixelY);
        }
    }
}

} // namespace

void PreviewComposite::ComposeOnCpu() {
    PrepareRun();
    if (configuration.previewResolution <= 0) return;
    ClearPassCpu();
    for (int layerIndex = 0; layerIndex < configuration.layerCount; ++layerIndex)
        FieldLayerPassCpu(layerIndex);
    OverlayPassCpu();
    EntityIdentifierPassCpu();
    bLastRunUsedGpu = false;
}

void PreviewComposite::ClearPassCpu() {
    PreviewColor clearColor;
    clearColor.red = configuration.clearColorRed;
    clearColor.green = configuration.clearColorGreen;
    clearColor.blue = configuration.clearColorBlue;
    clearColor.alpha = configuration.clearColorAlpha;
    const unsigned int clearTexel = PackRgba8(clearColor);
    for (unsigned int& texel : compositeTexels) texel = clearTexel;
    entityIdentifierBuffer.Clear();          // every pixel starts at emptySentinel
    ++executedPassCount;
}

// One colorized field, blended over the image. The pixel -> cell mapping is the same
// pixel-center form the bake uses, so preview and bake sample the same place.
void PreviewComposite::FieldLayerPassCpu(int layerIndex) {
    const PreviewLayerConfiguration& layerConfiguration = layerConfigurations[layerIndex];
    const PreviewBlendMode blendMode = static_cast<PreviewBlendMode>(layerConfiguration.blendMode);
    const int resolution = configuration.previewResolution;
    const float cellsPerPixel = static_cast<float>(configuration.vertexSize - 1)
                              / static_cast<float>(resolution);
    for (int pixelY = 0; pixelY < resolution; ++pixelY) {
        const float sampleY = (static_cast<float>(pixelY) + 0.5f) * cellsPerPixel;
        for (int pixelX = 0; pixelX < resolution; ++pixelX) {
            const float sampleX = (static_cast<float>(pixelX) + 0.5f) * cellsPerPixel;
            const PreviewColor layerColor = LayerColorAtPixel(layerConfiguration, sampleX, sampleY);
            const std::size_t texelIndex = static_cast<std::size_t>(pixelY) * resolution + pixelX;
            const PreviewColor blended =
                BlendPreviewColor(UnpackRgba8(compositeTexels[texelIndex]), layerColor, blendMode,
                                  layerConfiguration.opacity * layerColor.alpha);
            compositeTexels[texelIndex] = PackRgba8(blended);
        }
    }
    ++executedPassCount;
}

PreviewColor PreviewComposite::LayerColorAtPixel(const PreviewLayerConfiguration& layerConfiguration,
                                                 float sampleX, float sampleY) const {
    const float* const lookupTable = layerConfiguration.gradientLookupOffset >= 0
                                   ? gradientLookupTables.data() + layerConfiguration.gradientLookupOffset
                                   : nullptr;
    const int lookupEntryCount = layerConfiguration.gradientLookupEntryCount;
    const PreviewLayerKind layerKind = static_cast<PreviewLayerKind>(layerConfiguration.layerKind);
    if (layerKind == PreviewLayerKind::StratumSplat) {
        float stratumWeights[Data::MapFields::stratumCount];
        for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
            stratumWeights[stratum] =
                mapFields.surfaceStratumWeights[stratum].SampleBilinear(sampleX, sampleY);
        return SplatSurfaceStrata(stratumWeights, stratumConfigurations.data(),
                                  Data::MapFields::stratumCount,
                                  configuration.bNormalizeSplatWeights,
                                  configuration.splatWeightEpsilon);
    }
    if (layerKind == PreviewLayerKind::MapAreas) {
        // ARCH §14.19 (supersedes §14.17 items 5/6) — forward iteration, FIRST containing match
        // wins, early exit: ascending array index is now Z-descending (index 0 = top), so the
        // first hit scanning forward IS the topmost area (the same Z rule §21.8's own body
        // hit-test now implements), so click-to-select and what-you-see can never disagree. The
        // degenerate sentinel fails the first test unconditionally.
        for (const PreviewMapAreaRectangle& rectangle : mapAreaRectangles) {
            if (sampleX < rectangle.minimumX || sampleX > rectangle.maximumX) continue;
            if (sampleY < rectangle.minimumZ || sampleY > rectangle.maximumZ) continue;
            PreviewColor result;
            result.red = rectangle.colorRed; result.green = rectangle.colorGreen;
            result.blue = rectangle.colorBlue; result.alpha = rectangle.colorAlpha;
            return result;
        }
        return PreviewColor();
    }
    if (layerKind == PreviewLayerKind::Water) {
        if (configuration.bWaterEnabled == 0) return PreviewColor();
        const float depth = NormalizedWaterDepth(
            mapFields.heightfield.SampleBilinear(sampleX, sampleY), configuration);
        if (depth < 0.0f) return PreviewColor();          // the baked surface is above the water
        return SampleGradientLookupTable(lookupTable, lookupEntryCount, depth);
    }
    const Data::FloatField* const sourceField = LayerSourceField(layerKind);
    const float value = sourceField->SampleBilinear(sampleX, sampleY);
    return SampleGradientLookupTable(lookupTable, lookupEntryCount,
                                     NormalizeToDomain(value, layerConfiguration.domainMinimum,
                                                       layerConfiguration.domainRangeReciprocal));
}

// The overlay: the resolved entities, drawn. Alpha-blended so the composited terrain shows
// through a translucent mark.
void PreviewComposite::OverlayPassCpu() {
    PreviewColor markColor;
    markColor.red = configuration.entityMarkColorRed;
    markColor.green = configuration.entityMarkColorGreen;
    markColor.blue = configuration.entityMarkColorBlue;
    markColor.alpha = configuration.entityMarkColorAlpha;
    for (const PreviewEntityPoint& point : entityPoints)
        ForEachMarkedPixel(point, configuration.entityMarkRadiusPixels,
                           configuration.previewResolution,
                           [&](std::size_t texelIndex, int, int) {
                               compositeTexels[texelIndex] =
                                   PackRgba8(BlendPreviewColor(UnpackRgba8(compositeTexels[texelIndex]),
                                                               markColor, PreviewBlendMode::AlphaBlend,
                                                               markColor.alpha));
                           });
    ++executedPassCount;
}

// The id written under the mark IS the instance's index in Data::PlacementInstances, which is
// what Picking_UI (M4-4) resolves a click to. Pixels no entity covers keep emptySentinel.
void PreviewComposite::EntityIdentifierPassCpu() {
    for (const PreviewEntityPoint& point : entityPoints)
        ForEachMarkedPixel(point, configuration.entityMarkRadiusPixels,
                           configuration.previewResolution,
                           [&](std::size_t, int pixelX, int pixelY) {
                               entityIdentifierBuffer.Set(pixelX, pixelY, point.entityIdentifier);
                           });
    ++executedPassCount;
}

} // namespace Ui
} // namespace SanmapGen

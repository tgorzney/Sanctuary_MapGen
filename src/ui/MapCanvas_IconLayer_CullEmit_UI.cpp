// MapCanvas_IconLayer_CullEmit_UI.cpp — EmitCandidateIfVisible itself: pairing-lookup resolution
// (a miss draws nothing, §14.3), the two-mode LOD formula (verbatim), world -> screen projection
// (STEP47's forward half composed with MapCanvasView's inverse), and opacity-into-tint (§14.2).
// Layer: UI. Pure, imgui-free, headless-testable. Split out of MapCanvas_IconLayer_CullHelpers_UI.cpp
// to stay inside Constitution §1.5's file-size ceiling.
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "IconAtlasPairing_UI.h"
#include "IconGridWidget_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_UI.h"
#include "../io/WorldFootprintSizeTable_IO.h"

namespace SanmapGen {
namespace Ui {
namespace {

void LogMissOnce(IconLayerCullDiagnostics_UI* diagnostics, const std::string& templateIdentifier) {
    if (diagnostics != nullptr && !diagnostics->HasLoggedMissing(templateIdentifier))
        diagnostics->LogMissingOnce(templateIdentifier);
}

struct ResolvedLod_UI { int iconId = kInvalidIconId; float screenSize = 0.0f; };

// §14.3 formula, verbatim. baseFootprint is the larger of {width, depth} — the spec states no
// width/depth -> scalar combination rule, so this is a documented, reasoned coder choice (never a
// silently-invented replacement for STEP58's table itself, which stays the source of both numbers).
ResolvedLod_UI ResolveLodModeAndIcon(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                                     const IconIdentifierPairing& pairing,
                                     const std::string& templateIdentifier, float instanceScale) {
    const Io::WorldFootprintSize_IO footprint = input.footprintSizeTable->Resolve(templateIdentifier);
    const float baseFootprint = footprint.baseFootprintWidth > footprint.baseFootprintDepth
                                     ? footprint.baseFootprintWidth : footprint.baseFootprintDepth;
    const float worldUnitsPerCell = input.composite->Settings().worldUnitsPerCell;
    const float thumbnailScreenSize = worldUnitsPerCell > 0.0f
        ? (baseFootprint * instanceScale) / worldUnitsPerCell
              * input.composite->PixelsPerPreviewCell() * input.view->ZoomScale()
        : 0.0f;
    ResolvedLod_UI resolved;
    if (thumbnailScreenSize >= layer.thumbnailLodThresholdPixels) {
        resolved.iconId = pairing.thumbnailIconId; resolved.screenSize = thumbnailScreenSize;
    } else {
        resolved.iconId = pairing.strategicIconId; resolved.screenSize = layer.strategicIconScreenSizePixels;
    }
    return resolved;
}

void AppendCandidate(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer, int layerIndex,
                     const ResolvedLod_UI& lod, const IconAtlasEntry& atlasEntry, float worldX, float worldZ,
                     PlacementCollectionKind_UI collection, std::int32_t instanceIndex,
                     float tintColorRed, float tintColorGreen, float tintColorBlue,
                     int* stableOrderCounter, std::vector<OverlayVisibleInstance>& outCandidates,
                     bool bManual) {
    const PreviewComposite::PreviewPixelPoint previewPixel = input.composite->WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        input.view->ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    OverlayVisibleInstance instance;
    instance.screenCenterX = input.regionOriginX + regionLocal.regionLocalX;
    instance.screenCenterY = input.regionOriginY + regionLocal.regionLocalY;
    instance.screenSize = lod.screenSize;
    instance.uvMinimumX = atlasEntry.uvMinimumX; instance.uvMinimumY = atlasEntry.uvMinimumY;
    instance.uvMaximumX = atlasEntry.uvMaximumX; instance.uvMaximumY = atlasEntry.uvMaximumY;
    instance.atlasPage = atlasEntry.atlasPage;
    instance.textureIdentifier = input.atlasManifest->PageTextureIdentifier(atlasEntry.atlasPage);
    instance.tintAlpha = layer.opacity;   // §14.2 — opacity folded into tint, never a second blend path
    instance.tintColorRed = tintColorRed; instance.tintColorGreen = tintColorGreen; instance.tintColorBlue = tintColorBlue;
    instance.layerIndex = layerIndex;
    instance.stableOrder = stableOrderCounter != nullptr ? (*stableOrderCounter)++ : 0;
    instance.instanceKey = OverlayInstanceKey_UI{collection, instanceIndex, true, bManual};   // ARCH §19.25
    instance.bSelected = input.selectedInstanceKey.bValid
                       && OverlayInstanceKeysEqual(instance.instanceKey, input.selectedInstanceKey);
    outCandidates.push_back(instance);
}

} // namespace

void EmitCandidateIfVisible(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                            int layerIndex, const std::string& templateIdentifier,
                            float worldX, float worldZ, float instanceScale,
                            PlacementCollectionKind_UI collection, std::int32_t instanceIndex,
                            float tintColorRed, float tintColorGreen, float tintColorBlue,
                            int* stableOrderCounter, IconLayerCullDiagnostics_UI* diagnostics,
                            std::vector<OverlayVisibleInstance>& outCandidates, bool bManual) {
    if (input.pairingLookup == nullptr || input.atlasManifest == nullptr || input.footprintSizeTable == nullptr
        || input.composite == nullptr || input.view == nullptr)
        return;
    const IconIdentifierPairing pairing = input.pairingLookup->Resolve(templateIdentifier);
    if (pairing.thumbnailIconId == kInvalidIconId) { LogMissOnce(diagnostics, templateIdentifier); return; }
    const ResolvedLod_UI lod = ResolveLodModeAndIcon(input, layer, pairing, templateIdentifier, instanceScale);
    if (lod.iconId < 0 || lod.iconId >= input.atlasManifest->EntryCount()) {
        LogMissOnce(diagnostics, templateIdentifier);   // §14.3: strategic mode with no authored icon yet
        return;
    }
    AppendCandidate(input, layer, layerIndex, lod, input.atlasManifest->entries[static_cast<std::size_t>(lod.iconId)],
                    worldX, worldZ, collection, instanceIndex, tintColorRed, tintColorGreen, tintColorBlue,
                    stableOrderCounter, outCandidates, bManual);
}

} // namespace Ui
} // namespace SanmapGen

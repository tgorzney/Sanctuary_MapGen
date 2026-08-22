// PreviewComposite_Prepare_UI.cpp — the flattening that needs the BAKED fields: which layer
// reads which field, the ramp bake + domain resolution, and the resolved entity pixel
// positions. Layer: UI. It reads fields; it derives nothing from them but their own range.
#include "PreviewComposite_UI.h"
#include "GradientLut_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

// Auto-level: the minimum/maximum of the BAKED field (the legacy AutoLevelPreview Cpu scan).
// It reads the field — it does not recompute or re-derive it.
void FieldRange(const Data::FloatField& field, float& minimum, float& maximum) {
    if (field.IsEmpty()) { minimum = 0.0f; maximum = 1.0f; return; }
    const float* const values = field.Data();
    minimum = values[0];
    maximum = values[0];
    for (std::size_t cell = 1; cell < field.CellCount(); ++cell) {
        if (values[cell] < minimum) minimum = values[cell];
        if (values[cell] > maximum) maximum = values[cell];
    }
}

} // namespace

// Which baked field a layer colorizes. Water reads the heightfield (its depth is measured
// against the water level in game units); the splat reads all nine weight fields, so it names
// none here. Slope names the field the Mask stage baked — it is SAMPLED, never derived (M5-0c).
const Data::FloatField* PreviewComposite::LayerSourceField(PreviewLayerKind kind) const {
    switch (kind) {
        case PreviewLayerKind::Flow:         return &mapFields.flow;
        case PreviewLayerKind::Accumulation: return &mapFields.accumulation;
        case PreviewLayerKind::Slope:        return &mapFields.slope;
        case PreviewLayerKind::StratumSplat: return nullptr;
        default:                             return &mapFields.heightfield;
    }
}

// One record per ENABLED layer, in UI order, each carrying its own ramp table slice. Two layers
// naming the same ramp each get their own slice — the tables are a few hundred floats and a
// shared slice would couple two independent layers' domains.
void PreviewComposite::BuildLayerConfigurations() {
    layerConfigurations.clear();
    gradientLookupTables.clear();
    // Nothing baked yet: the composite clears and stops rather than colorizing an absent field
    // (Constitution §6 — validate, do not read past a field that was never sized).
    if (!mapFields.IsSized()) { gradientLookupTables.push_back(0.0f); configuration.layerCount = 0; return; }
    for (const PreviewFieldLayer& layer : settings.fieldLayers) {
        if (!layer.bEnabled) continue;
        PreviewLayerConfiguration record;
        record.layerKind = static_cast<int>(layer.kind);
        record.blendMode = static_cast<int>(layer.blendMode);
        record.opacity = ClampUnit(layer.opacity);
        float domainMinimum = layer.domainMinimum;
        float domainMaximum = layer.domainMaximum;
        const Data::FloatField* const sourceField = LayerSourceField(layer.kind);
        if (layer.bAutoDomainFromField && sourceField != nullptr)
            FieldRange(*sourceField, domainMinimum, domainMaximum);
        record.domainMinimum = domainMinimum;
        record.domainRangeReciprocal = ReciprocalOrZero(domainMaximum - domainMinimum);
        const int rampIndex = layer.gradientRampIndex;
        if (rampIndex >= 0 && static_cast<std::size_t>(rampIndex) < settings.gradientRamps.size()) {
            const std::vector<float> table = BakeGradientLut(settings.gradientRamps[rampIndex]);
            record.gradientLookupOffset = static_cast<int>(gradientLookupTables.size());
            record.gradientLookupEntryCount =
                static_cast<int>(table.size()) / static_cast<int>(kLookupChannelCount);
            gradientLookupTables.insert(gradientLookupTables.end(), table.begin(), table.end());
        }
        layerConfigurations.push_back(record);
    }
    if (gradientLookupTables.empty()) gradientLookupTables.push_back(0.0f);  // never a 0-byte buffer
    configuration.layerCount = static_cast<int>(layerConfigurations.size());
}

// Preview pixels per one world-space "cell" of the baked field grid. Zero if no field grid is
// baked yet (mirrors Resolution()'s own zero-when-unbaked contract).
float PreviewComposite::PixelsPerPreviewCell() const {
    const int vertexSize = mapFields.VertexSize();
    if (vertexSize < 2) return 0.0f;
    return static_cast<float>(configuration.previewResolution) / static_cast<float>(vertexSize - 1);
}

// World (positionX/positionZ) -> preview pixel. The exact mapping BuildEntityPoints bakes marks
// through; extracted so there is exactly one copy.
PreviewComposite::PreviewPixelPoint PreviewComposite::WorldToPreviewPixel(float worldX, float worldZ) const {
    const float cellsPerWorldUnit = ReciprocalOrZero(settings.worldUnitsPerCell);
    const float pixelsPerCell = PixelsPerPreviewCell();
    PreviewPixelPoint point;
    point.pixelX = worldX * cellsPerWorldUnit * pixelsPerCell - 0.5f;
    point.pixelY = worldZ * cellsPerWorldUnit * pixelsPerCell - 0.5f;
    return point;
}

// Inverse — preview pixel -> world. See the header's contract note: answers (0,0) when the
// composite has not baked yet (PixelsPerPreviewCell() == 0).
PreviewComposite::PreviewWorldPoint PreviewComposite::PreviewPixelToWorld(float pixelX, float pixelY) const {
    const float pixelsPerCell = PixelsPerPreviewCell();
    const float cellReciprocal = ReciprocalOrZero(pixelsPerCell);
    PreviewWorldPoint point;
    point.worldX = (pixelX + 0.5f) * settings.worldUnitsPerCell * cellReciprocal;
    point.worldZ = (pixelY + 0.5f) * settings.worldUnitsPerCell * cellReciprocal;
    return point;
}

// The resolved instances Placement accepted, mapped onto preview pixels. The mark is DRAWN, it
// is never re-tested against a placement rule (ARCH §3.2, the legacy SSBO-6 rule re-filter):
// a marker the bake rejected is simply not in this array, so it cannot paint here.
// The pixel mapping is the exact inverse of the field sampling in the two composite twins.
void PreviewComposite::BuildEntityPoints() {
    entityPoints.clear();
    configuration.entityCount = 0;
    if (!settings.bEntitiesEnabled || PixelsPerPreviewCell() <= 0.0f || instances.IsEmpty()) return;
    entityPoints.reserve(instances.Count());
    for (std::size_t instance = 0; instance < instances.Count(); ++instance) {
        PreviewEntityPoint point;
        // positionY is height; the horizontal plane is X/Z (PlacementInstance_DATA).
        const PreviewPixelPoint pixel = WorldToPreviewPixel(instances.positionX[instance],
                                                             instances.positionZ[instance]);
        point.pixelX = pixel.pixelX;
        point.pixelY = pixel.pixelY;
        point.entityIdentifier = static_cast<unsigned int>(instance);
        entityPoints.push_back(point);
    }
    configuration.entityCount = static_cast<int>(entityPoints.size());
}

} // namespace Ui
} // namespace SanmapGen

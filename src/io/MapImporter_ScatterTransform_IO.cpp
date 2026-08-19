// MapImporter_ScatterTransform_IO.cpp — the `Transform` sub-object every rule-Stack entry carries
// -> `Params::ScatterTransform`. Layer: IO. Body relocated verbatim from the deleted
// MapImporter_Rules_IO.cpp — same reasoning as the header: shared plumbing across 4 sibling files
// now (SANMAP_FORMAT_SPEC Correction 7).
#include "MapImporter_ScatterTransform_IO.h"
#include "../params/ScatterTransform_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadScatterTransformJson(const nlohmann::json& parent, Params::ScatterTransform& transform) {
    if (!parent.contains("Transform") || !parent["Transform"].is_object()) return;
    const nlohmann::json& json = parent["Transform"];
    ReadJsonFloat(json, "ScaleMinimum", transform.scaleMinimum);
    ReadJsonFloat(json, "ScaleMaximum", transform.scaleMaximum);
    ReadJsonFloat(json, "RotationMinimumDegrees", transform.rotationMinimumDegrees);
    ReadJsonFloat(json, "RotationMaximumDegrees", transform.rotationMaximumDegrees);
    ReadJsonBoolean(json, "AlignToTerrainNormal", transform.bAlignToTerrainNormal);
    ReadJsonBoolean(json, "Collidable", transform.bCollidable);
    std::string templateIdentifier;
    if (!ReadJsonText(json, "TemplateIdentifier", templateIdentifier)) return;
    // The buffer is fixed width and must stay NUL-terminated whatever the document claimed.
    const std::size_t capacity = sizeof(transform.templateIdentifier) - 1u;
    const std::size_t copyLength = templateIdentifier.size() < capacity ? templateIdentifier.size()
                                                                        : capacity;
    for (std::size_t index = 0; index < sizeof(transform.templateIdentifier); ++index)
        transform.templateIdentifier[index] = index < copyLength ? templateIdentifier[index] : '\0';
}

} // namespace Io
} // namespace SanmapGen

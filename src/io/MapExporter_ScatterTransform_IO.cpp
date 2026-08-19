// MapExporter_ScatterTransform_IO.cpp — `Params::ScatterTransform` -> the `Transform` sub-object
// every rule-Stack entry (`MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack`) carries.
// Layer: IO. Body relocated verbatim from the deleted MapExporter_Rules_IO.cpp — same reasoning as
// the header: shared plumbing across 4 sibling files now (SANMAP_FORMAT_SPEC Correction 7).
#include "MapExporter_ScatterTransform_IO.h"
#include "../params/ScatterTransform_PARAMS.h"
#include <cstddef>
#include <string>

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildScatterTransformJson(const Params::ScatterTransform& transform) {
    nlohmann::ordered_json json;
    json["ScaleMinimum"]           = transform.scaleMinimum;
    json["ScaleMaximum"]           = transform.scaleMaximum;
    json["RotationMinimumDegrees"] = transform.rotationMinimumDegrees;
    json["RotationMaximumDegrees"] = transform.rotationMaximumDegrees;
    json["AlignToTerrainNormal"]   = transform.bAlignToTerrainNormal;
    json["Collidable"]             = transform.bCollidable;
    // `tpId` is a game-dictated identifier kept verbatim (ARCH §1.1). Stored as a bounded string
    // so a non-terminated buffer can never run off the end (Constitution §6).
    const char* identifier = transform.templateIdentifier;
    std::size_t identifierLength = 0;
    while (identifierLength < sizeof(transform.templateIdentifier) && identifier[identifierLength] != '\0')
        ++identifierLength;
    json["TemplateIdentifier"] = std::string(identifier, identifierLength);
    return json;
}

} // namespace Io
} // namespace SanmapGen

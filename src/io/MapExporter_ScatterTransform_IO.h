// MapExporter_ScatterTransform_IO.h — `BuildScatterTransformJson`, the shared instance-transform
// builder every rule-Stack exporter (Markers/Props/Decals/Units) composes from.
// Layer: IO. Extracted out of the deleted MapExporter_Rules_IO.cpp: it is shared plumbing across
// 4 sibling `MapExporter_*Stack_IO.cpp` files now, not local to one (SANMAP_FORMAT_SPEC Correction 7).
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Params { struct ScatterTransform; }
namespace Io {

nlohmann::ordered_json BuildScatterTransformJson(const Params::ScatterTransform& transform);

} // namespace Io
} // namespace SanmapGen

// MapExporter_Symmetry_IO.cpp — `recipe.globalSymmetryMask`/`radialSymmetryRepeatCount`/
// `symmetryDetection`/`symmetryBlend` -> the top-level `.sanmap` `Symmetry` object.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 4 (STEP16), narrowed by the ARCH Expert scoping ruling
// on that ticket. One flat object, sibling of `armies`/`atmosphere`/`SlopeDefaults`/etc., NOT
// nested in `mapGeneratorData`. `GlobalSymmetryMask` is RELOCATED here out of the legacy
// `mapGeneratorData` blob (see MapExporter_Recipe_IO.cpp), not duplicated.
// `Params::SymAlgorithm` is explicitly OUT OF SCOPE (STEP16 ruling #1) — no field, no JSON key.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildSymmetryJson(const Params::MapRecipe& recipe) {
    const Params::SymmetryDetection& detection = recipe.symmetryDetection;
    const Params::SymmetryBlend& blend = recipe.symmetryBlend;
    nlohmann::ordered_json json;
    json["GlobalSymmetryMask"]        = recipe.globalSymmetryMask;
    json["RadialSymmetryRepeatCount"] = recipe.radialSymmetryRepeatCount;
    json["SnapImperfectSymmetry"]      = detection.bSnapImperfectSymmetry;
    json["SymmetryDetectionTolerance"] = detection.detectionTolerance;
    json["SymSuperpositionBlend"] = blend.superpositionBlend;
    json["SymmetryBlurRadius"]    = blend.blurRadius;
    json["CrossFadeWidth"]        = blend.crossFadeWidth;
    json["CylinderZScale"]        = blend.cylinderZScale;
    json["TorusMajorRadius"]      = blend.torusMajorRadius;
    json["TorusMinorRadius"]      = blend.torusMinorRadius;
    return json;
}

} // namespace Io
} // namespace SanmapGen

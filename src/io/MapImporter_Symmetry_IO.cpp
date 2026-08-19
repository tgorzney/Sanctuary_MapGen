// MapImporter_Symmetry_IO.cpp — the top-level `.sanmap` `Symmetry` object ->
// `recipe.globalSymmetryMask`/`radialSymmetryRepeatCount`/`symmetryDetection`/`symmetryBlend`.
// Layer: IO. The exact inverse of MapExporter_Symmetry_IO.cpp (SANMAP_FORMAT_SPEC Correction 4,
// STEP16). Same tier and calling contract as `areas`/`armies`/`atmosphere`/`SlopeDefaults` above:
// takes the top-level `document` directly and must be called unconditionally, BEFORE the
// `mapGeneratorData` presence gate. A document with no `Symmetry` key (older, or hand-authored)
// simply keeps the recipe's own defaults, including the new `RotateHalfTurn` default mask
// (degrade-gracefully, no migration file needed — a content-shape-only change).
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadSymmetryJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("Symmetry") || !document["Symmetry"].is_object()) return;
    const nlohmann::json& json = document["Symmetry"];
    Params::SymmetryDetection& detection = outRecipe.symmetryDetection;
    Params::SymmetryBlend& blend = outRecipe.symmetryBlend;
    ReadJsonInteger(json, "GlobalSymmetryMask", outRecipe.globalSymmetryMask);
    ReadJsonInteger(json, "RadialSymmetryRepeatCount", outRecipe.radialSymmetryRepeatCount);
    ReadJsonBoolean(json, "SnapImperfectSymmetry", detection.bSnapImperfectSymmetry);
    ReadJsonFloat(json, "SymmetryDetectionTolerance", detection.detectionTolerance);
    ReadJsonFloat(json, "SymSuperpositionBlend", blend.superpositionBlend);
    ReadJsonFloat(json, "SymmetryBlurRadius", blend.blurRadius);
    ReadJsonFloat(json, "CrossFadeWidth", blend.crossFadeWidth);
    ReadJsonFloat(json, "CylinderZScale", blend.cylinderZScale);
    ReadJsonFloat(json, "TorusMajorRadius", blend.torusMajorRadius);
    ReadJsonFloat(json, "TorusMinorRadius", blend.torusMinorRadius);
}

} // namespace Io
} // namespace SanmapGen

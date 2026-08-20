// Symmetry_Migrate_V2_IO.cpp — see the header for the full contract.
#include "Symmetry_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {

void Symmetry_Migrate_V2(nlohmann::json& document) {
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    nlohmann::json& legacy = document["mapGeneratorData"];
    nlohmann::json& section = document["Symmetry"];
    if (!section.is_object()) section = nlohmann::json::object();

    MoveKey(legacy, "GlobalSymmetryMask",        section, "GlobalSymmetryMask");
    MoveKey(legacy, "SnapImperfectSymmetry",     section, "SnapImperfectSymmetry");
    MoveKey(legacy, "SymmetryDetectionTolerance", section, "SymmetryDetectionTolerance");
    MoveKey(legacy, "SymSuperpositionBlend",     section, "SymSuperpositionBlend");
    MoveKey(legacy, "SymmetryBlurRadius",        section, "SymmetryBlurRadius");
    MoveKey(legacy, "CrossFadeWidth",            section, "CrossFadeWidth");
    MoveKey(legacy, "CylinderZScale",            section, "CylinderZScale");
    MoveKey(legacy, "TorusMajorRadius",          section, "TorusMajorRadius");
    MoveKey(legacy, "TorusMinorRadius",          section, "TorusMinorRadius");
}

} // namespace Io
} // namespace SanmapGen

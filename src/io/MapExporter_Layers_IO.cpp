// MapExporter_Layers_IO.cpp — the per-stratum settings, as `mapGeneratorData` JSON.
// Layer: IO. One writer function per PARAMS struct, each a literal field-for-field mirror so a
// reader of MapImporter_Recipe_IO.cpp's `ReadStrataSettingsJson` can diff the two by eye.
// The layer-stack half of this file's old name (`BuildLayerStackJson` + its `BuildLayerJson`/
// `BuildGeoLayerJson` helpers) RELOCATED to MapExporter_HeightmapStack_IO.cpp (SANMAP_FORMAT_SPEC
// Correction 3) — this file now holds only the unrelated Stratum content.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildStratumJson(const Params::Stratum& stratum) {
    // The 8 slope-gate keys (`SlopeGateEnabled` ... `SlopeGateStrength`) RELOCATED to the top-level
    // `StratumGenerationSettings` array (SANMAP_FORMAT_SPEC Correction 12,
    // MapExporter_StratumGeneration_IO.cpp) — not duplicated here anymore.
    nlohmann::ordered_json json;
    json["ImportedMaskMode"]       = static_cast<int>(stratum.importedMaskMode);
    // Genuine Vector4 (ARCH §7.2 item 10) — mirrors the format-native `stratumLayers[].maskRemapMin/
    // Max` shape so the two on-disk representations of the same setting never disagree.
    json["MaskRemapMinimum"] = { {"x", stratum.maskRemapMinimum[0]}, {"y", stratum.maskRemapMinimum[1]},
                                 {"z", stratum.maskRemapMinimum[2]}, {"w", stratum.maskRemapMinimum[3]} };
    json["MaskRemapMaximum"] = { {"x", stratum.maskRemapMaximum[0]}, {"y", stratum.maskRemapMaximum[1]},
                                 {"z", stratum.maskRemapMaximum[2]}, {"w", stratum.maskRemapMaximum[3]} };
    json["Enabled"]                = stratum.bEnabled;
    json["TintRed"]                = stratum.tintRed;
    json["TintGreen"]              = stratum.tintGreen;
    json["TintBlue"]               = stratum.tintBlue;
    json["TileCount"]              = stratum.tileCount;
    return json;
}

} // namespace

nlohmann::ordered_json BuildStrataSettingsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json strata = nlohmann::ordered_json::array();
    for (const Params::Stratum& stratum : recipe.strata) strata.push_back(BuildStratumJson(stratum));
    return strata;
}

} // namespace Io
} // namespace SanmapGen

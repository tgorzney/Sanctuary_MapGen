// MapExporter_StratumLayers_IO.cpp — `Params::MapRecipe::strata` -> the top-level `stratumLayers[9]`
// array. Layer: IO. The exact inverse of `ReadStratumLayersJson` (MapImporter_StratumLayers_IO.cpp),
// key for key — SANMAP_FORMAT_SPEC Correction 13. Split out of MapExporter_Recipe_IO.cpp under the
// ARCH §1.5 size ceiling (STEP29); its declaration stays in that file's header
// (MapExporter_Recipe_IO.h), mirroring MapImporter_StratumLayers_IO.cpp 1:1.
// STEP30_LegacyBlobFieldHoming_IO: also writes `ImportedMaskMode`/`Enabled`, new sibling keys on
// each entry — the legacy `mapGeneratorData.Stratums[].ImportedMaskMode`/`Enabled`
// (MapExporter_Layers_IO.cpp) still writes too and stays authoritative on import when present.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildStratumLayersJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json stratumLayers = nlohmann::ordered_json::array();
    for (int stratumIndex = 0; stratumIndex < sanmapStratumCount; ++stratumIndex) {
        const bool bHasSettings = stratumIndex < static_cast<int>(recipe.strata.size());
        const Params::Stratum stratum = bHasSettings ? recipe.strata[stratumIndex] : Params::Stratum();
        const Params::StratumAppearance& appearance = stratum.appearance;
        nlohmann::ordered_json layer;
        layer["name"]        = "Stratum " + std::to_string(stratumIndex);
        // `name` still writes the generated placeholder, not `appearance.name` — real gap, flagged
        // for a future pass, not this correction (SANMAP_FORMAT_SPEC Correction 13).
        layer["ImportedMaskMode"] = static_cast<int>(stratum.importedMaskMode);
        layer["Enabled"]          = stratum.bEnabled;
        layer["albedo"]      = { { "path", appearance.albedoTexturePath } };
        layer["normal"]      = { { "path", appearance.normalTexturePath } };
        layer["mask"]        = { { "path", appearance.compositeTexturePath } };
        layer["tileSize"]    = { { "x", stratum.tileCount }, { "y", stratum.tileCount } };
        layer["tileSizeFar"] = { { "x", appearance.farTileCount }, { "y", appearance.farTileCount } };
        layer["tileSizeTriplanar"]    = appearance.triplanarTileCount;
        layer["tileSizeFarTriplanar"] = appearance.farTriplanarTileCount;
        layer["normalScale"]          = appearance.normalScale;
        layer["normalScaleFar"]       = appearance.farNormalScale;
        layer["normalFarNearBlend"]   = appearance.normalFarNearBlend;
        layer["heightFarNearBlend"]   = appearance.heightFarNearBlend;
        // `diffuseRemap` is written FROM Stratum::tint* — not `appearance.diffuseRemapColor`, which
        // was deleted (dead, round-tripped nothing; see StratumAppearance_PARAMS.h).
        layer["diffuseRemap"] = { { "r", stratum.tintRed }, { "g", stratum.tintGreen },
                                  { "b", stratum.tintBlue }, { "a", 1.0f } };
        layer["farColorRemap"] = { { "r", appearance.farColorRemapColor[0] },
                                   { "g", appearance.farColorRemapColor[1] },
                                   { "b", appearance.farColorRemapColor[2] },
                                   { "a", appearance.farColorRemapColor[3] } };
        // Real Vector4, matching the C# ground truth `SanMap.Types.cs` (ARCH §7.2 item 10) — not
        // the bare scalar this line used to write.
        layer["maskRemapMin"] = { {"x", stratum.maskRemapMinimum[0]}, {"y", stratum.maskRemapMinimum[1]},
                                  {"z", stratum.maskRemapMinimum[2]}, {"w", stratum.maskRemapMinimum[3]} };
        layer["maskRemapMax"] = { {"x", stratum.maskRemapMaximum[0]}, {"y", stratum.maskRemapMaximum[1]},
                                  {"z", stratum.maskRemapMaximum[2]}, {"w", stratum.maskRemapMaximum[3]} };
        stratumLayers.push_back(layer);
    }
    return stratumLayers;
}

} // namespace Io
} // namespace SanmapGen

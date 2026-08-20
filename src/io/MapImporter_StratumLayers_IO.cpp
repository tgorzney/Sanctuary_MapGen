// MapImporter_StratumLayers_IO.cpp — the top-level `stratumLayers[9]` array -> `Params::Stratum::
// appearance` (+ tileCount/tint*/maskRemap*). Layer: IO. The exact inverse of
// `BuildStratumLayersJson` (MapExporter_Recipe_IO.cpp), key for key — SANMAP_FORMAT_SPEC
// Correction 13. Split out of MapImporter_Recipe_IO.cpp under the ARCH §1.5 size ceiling; its
// declaration stays in that file's header (MapImporter_Recipe_IO.h), matching the precedent of
// MapImporter_Layers_IO.cpp / MapImporter_SlopeDefaults_IO.cpp.
// STEP30_LegacyBlobFieldHoming_IO: also reads `ImportedMaskMode`/`Enabled`. This function runs
// unconditionally (MapImporter_IO.cpp, before the mapGeneratorData gate); the gated legacy
// `ReadStrataSettingsJson` (MapImporter_Recipe_IO.cpp) runs AFTER and still wins on overlap when
// `mapGeneratorData.Stratums` is present, matching the STEP27 water precedent exactly.
// STEP37_StratumAppearanceRoundtrip_IO: also reads `name`/`environmentName`/`materialName` into
// `appearance.*` — no empty-value fallback needed, see the inline note at the read site.
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_IO.h"
#include "MapExporter_IO.h"   // sanmapStratumCount — the shared format-invariant cardinality
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// `{"path": ...}` — the format's TextureLoader wrapper (same shape as Atmosphere's sunCookie/
// skybox — a local helper, not promoted to JsonPrimitives_IO.h, matching that domain's precedent
// of one local copy per file until a second file needs it).
void ReadJsonPathWrapper(const nlohmann::json& parent, const char* key, std::string& destination) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    ReadJsonText(parent[key], "path", destination);
}

// `{"r":.., "g":.., "b":.., "a":..}` — `diffuseRemap`/`farColorRemap`'s shape, distinct from
// `maskRemapMin`/`Max`'s `x`/`y`/`z`/`w` (ReadJsonFloatVector4 stays x/y/z/w-only).
void ReadJsonColorRgba(const nlohmann::json& parent, const char* key, float destination[4]) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    const nlohmann::json& color = parent[key];
    ReadJsonFloat(color, "r", destination[0]);
    ReadJsonFloat(color, "g", destination[1]);
    ReadJsonFloat(color, "b", destination[2]);
    ReadJsonFloat(color, "a", destination[3]);
}

// One `stratumLayers[]` entry -> one `Params::Stratum` (SANMAP_FORMAT_SPEC Correction 13's
// mapping table, verbatim). Total per Constitution §6: a missing/wrong-typed sub-key leaves that
// one field on whatever it already held.
void ReadStratumLayerJson(const nlohmann::json& layerJson, Params::Stratum& stratum) {
    Params::StratumAppearance& appearance = stratum.appearance;
    int maskMode = static_cast<int>(stratum.importedMaskMode);
    if (ReadJsonEnumeration(layerJson, "ImportedMaskMode", 3, maskMode))
        stratum.importedMaskMode = static_cast<Params::ImportedMaskMode>(maskMode);
    ReadJsonBoolean(layerJson, "Enabled", stratum.bEnabled);
    // `name`: no fallback text needed on a missing/old-shaped key — `StratumNameRules()`
    // (StratumsTab_UI.h) sets `bAllowEmpty = true`, so the field's own empty default IS the UI's
    // legal value, unlike STEP25's `mapName`. The header's "Stratum <index>" text is a display-time
    // fallback (`FormatStratumSectionLabel`), never stored into this field.
    ReadJsonText(layerJson, "name", appearance.name);
    ReadJsonText(layerJson, "environmentName", appearance.environmentName);
    ReadJsonText(layerJson, "materialName", appearance.materialName);
    ReadJsonPathWrapper(layerJson, "albedo", appearance.albedoTexturePath);
    ReadJsonPathWrapper(layerJson, "normal", appearance.normalTexturePath);
    ReadJsonPathWrapper(layerJson, "mask", appearance.compositeTexturePath);
    if (layerJson.contains("tileSize") && layerJson["tileSize"].is_object())
        ReadJsonFloat(layerJson["tileSize"], "x", stratum.tileCount);   // y ignored — no
                                                                         // anisotropic tile field
    if (layerJson.contains("tileSizeFar") && layerJson["tileSizeFar"].is_object())
        ReadJsonFloat(layerJson["tileSizeFar"], "x", appearance.farTileCount);
    ReadJsonFloat(layerJson, "tileSizeTriplanar", appearance.triplanarTileCount);
    ReadJsonFloat(layerJson, "tileSizeFarTriplanar", appearance.farTriplanarTileCount);
    ReadJsonFloat(layerJson, "normalScale", appearance.normalScale);
    ReadJsonFloat(layerJson, "normalScaleFar", appearance.farNormalScale);
    ReadJsonFloat(layerJson, "normalFarNearBlend", appearance.normalFarNearBlend);
    ReadJsonFloat(layerJson, "heightFarNearBlend", appearance.heightFarNearBlend);
    if (layerJson.contains("diffuseRemap") && layerJson["diffuseRemap"].is_object()) {
        const nlohmann::json& diffuseRemap = layerJson["diffuseRemap"];
        ReadJsonFloat(diffuseRemap, "r", stratum.tintRed);
        ReadJsonFloat(diffuseRemap, "g", stratum.tintGreen);
        ReadJsonFloat(diffuseRemap, "b", stratum.tintBlue);
        // alpha dropped — Params::Stratum has no tint-alpha field.
    }
    ReadJsonColorRgba(layerJson, "farColorRemap", appearance.farColorRemapColor);
    ReadJsonFloatVector4(layerJson, "maskRemapMin", stratum.maskRemapMinimum);
    ReadJsonFloatVector4(layerJson, "maskRemapMax", stratum.maskRemapMaximum);
}

} // namespace

// Growing only (never shrinking, Constitution §6): a merge-friendly companion to
// `ReadStrataSettingsJson` (MapImporter_Recipe_IO.cpp), which runs later, gated, and preserves
// whatever this pass wrote into `outRecipe.strata[index].appearance` rather than clobbering it.
void ReadStratumLayersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                           MapImportResult& result) {
    if (!document.contains("stratumLayers") || !document["stratumLayers"].is_array()) return;
    const nlohmann::json& stratumLayers = document["stratumLayers"];
    if (static_cast<int>(stratumLayers.size()) != sanmapStratumCount) {
        result.Warn("stratumLayers has " + std::to_string(stratumLayers.size()) + " entries; the "
                    "format expects exactly " + std::to_string(sanmapStratumCount) + ".");
    }
    if (outRecipe.strata.size() < stratumLayers.size())
        outRecipe.strata.resize(stratumLayers.size());
    for (std::size_t index = 0; index < stratumLayers.size(); ++index) {
        if (stratumLayers[index].is_object())
            ReadStratumLayerJson(stratumLayers[index], outRecipe.strata[index]);
    }
}

} // namespace Io
} // namespace SanmapGen

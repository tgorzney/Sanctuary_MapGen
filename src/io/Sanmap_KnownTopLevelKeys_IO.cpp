// Sanmap_KnownTopLevelKeys_IO.cpp — see the header for the full contract. The union of (a) every
// key a `Read*Json` call in `MapImporter::ParseSanmapJsonText` actually reads (ground truth: that
// function's own call list, `MapImporter_IO.cpp`), (b) keys `MapExporter::BuildSanmapJsonText`
// writes but deliberately has no importer for YET (`MapExporter_Recipe_IO.cpp`), and (c) the
// migration runner's own two keys.
#include "Sanmap_KnownTopLevelKeys_IO.h"
#include <unordered_set>

namespace SanmapGen {
namespace Io {
namespace {

const std::unordered_set<std::string>& KnownTopLevelSanmapKeys() {
    static const std::unordered_set<std::string> keys = {
        // (c) runner-owned / partially-consumed by the legacy mapGeneratorData-gated readers
        // (ReadGeometryJson/ReadWaterJson/ReadStrataSettingsJson) — never wholesale unknown.
        "SanGenVersion", "mapGeneratorData",
        // Runner-owned and special-cased in `CaptureUnknownTopLevelKeys` (its own `.contains(
        // "UnknownImport")` seed step, STEP28_UnknownImportNesting_IO) — that explicit check already
        // keeps it out of the generic unknown-capture loop below, so this entry is defensive clarity
        // only, not load-bearing: without it, a future reader would wonder why `UnknownImport` isn't
        // captured as "just another unknown key."
        "UnknownImport",

        // (a) read directly, unconditionally, by ParseSanmapJsonText's own top-level readers.
        "name", "credits", "height", "width",
        "hasWater", "waterLevel", "waterDepth",                // STEP27_WaterTopLevelImport_IO
        "deepWaterDepthMin",                                   // STEP30_LegacyBlobFieldHoming_IO
        "areas", "armies", "markers", "MarkerGroups", "MarkerLayerBundles", "chains",
        "PropGroups", "props", "DecalGroups", "decals",
        "Scenarios",                                            // STEP69_ParamsScenariosRoundTrip_IO
        "MarkersStack", "GlobalMarkerSettings", "PropsStack", "DecalsStack", "UnitsStack",
        "stratumLayers", "StratumGenerationSettings",
        "GeneralMapSettings", "HeightmapStack", "Symmetry",
        "SlopeDefaults", "Flow", "DetailNormal",
        // `Accumulation` is written every export and its own reader (`ReadAccumulationJson`) IS
        // called unconditionally by ParseSanmapJsonText, but reads nothing yet (SANMAP_FORMAT_SPEC
        // Correction 6 has no field list — "TBD" means TBD, MapImporter_FlowAccumulation_IO.cpp).
        // Still a current SANMAP_FORMAT_SPEC section, not genuinely unrecognized data —
        // IO_MIGRATION_SPEC.md §6's Unknown Import passthrough only captures a key that is "not one
        // of SANMAP_FORMAT_SPEC's current sections." Allowlisted here for that reason (this key was
        // missing from the work-order's own confirmed (b) list — flagged, not silently absorbed).
        "Accumulation",
        // ATMOSPHERE_PARAMS_SPEC's ~49 flat top-level keys (MapImporter_Atmosphere_IO.cpp).
        "sunRA", "sunDA", "sunIntensity", "sunTint", "sunTemperature", "sunAngularDiameter",
        "sunVolumetricsMultiplier", "sunVolumetricsShadowDimer", "sunPosition", "sunCookie",
        "sunCookieSize", "skylightIntensity", "skylightTint", "skylightTemperature",
        "exposure", "exposureCompensation", "skybox", "skyboxRotation", "skyboxIntensityMode",
        "skyboxExposure", "skyboxMultiplier", "skyboxLuxValue",
        "fogAttenuationDistance", "fogBaseHeight", "fogMaximumHeight", "fogMaximumDistance",
        "fogAnisotropy",
        "backgroundFogIntensity", "backgroundFogRange", "backgroundFogMinimum",
        "backgroundSkyColorIntensity", "backgroundColor", "backgroundColorIntensity",
        "backgroundColorFadeoutRange", "backgroundColorFadeoutPower",
        "heightFogIntensity", "heightFogRange", "heightFogStart", "heightFogEnd", "heightFogPower",
        "linearFogIntensity", "linearFogStart", "linearFogEnd", "linearFogPower",
        "linearFogCameraIntensity", "linearFogCameraStart", "linearFogCameraEnd",
        "windSpeed", "windDirection",

        // (b) confirmed write-only-by-design TODAY (MapExporter_Recipe_IO.cpp) — allowlisted so
        // they are never double-bagged (the exporter already writes them every time regardless).
        // NOT a claim they are intentionally write-only forever: `name`/`credits` (STEP25) and
        // `hasWater`/`waterLevel`/`waterDepth` (STEP27) used to be on this list and have since
        // moved up to the (a) list above once a real importer landed for them. These 8 remain open,
        // tracked by STEP25_ExportOnlyFieldAudit_IO.md's follow-on scope — a future coder adding an
        // importer for one of them moves it to (a), it does not delete it from here.
        "fileVersion", "mapVersion", "length", "heightmapResolution", "shader",
        "heightTransition", "fadeDistance", "fadeStartDistance",
    };
    return keys;
}

} // namespace

bool IsKnownTopLevelSanmapKey(const std::string& key) {
    return KnownTopLevelSanmapKeys().count(key) != 0;
}

} // namespace Io
} // namespace SanmapGen

// MapExporter_DocumentAssembly_IO.h — orchestration-tier helpers behind
// MapExporter::BuildSanmapJsonText (STEP31_ExporterRecipeOrchestrator_IO). None of these own a
// single top-level `.sanmap` section — they only sequence calls into real per-domain builders that
// already live elsewhere (declared in MapExporter_Recipe_IO.h and its siblings) — so they are a
// genuinely new kind (orchestration helpers), not subject to "one domain per file." Layer: IO.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// The ~17 flat root scalars (`fileVersion` through `fadeStartDistance`) — the document's own
// envelope, not any format-native sub-object.
void BuildDocumentEnvelopeJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document);

// `stratumLayers`, `StratumGenerationSettings`, `areas`, `armies`, `markers`, `chains`, `decals`,
// `props`, `PropGroups`, `DecalGroups` — the recipe's own authored entities.
void AppendEntityDomainsJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document);

// `MarkersStack`, `PropsStack`, `DecalsStack`, `UnitsStack`, `GlobalMarkerSettings` — the
// placement-rule vectors and their shared settings.
void AppendStackDomainsJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document);

// `BuildAtmosphereJson`, `SlopeDefaults`, `GeneralMapSettings`, `HeightmapStack`, `Symmetry`,
// `Flow`, `Accumulation`, `DetailNormal` — all flat top-level objects, siblings of
// `mapGeneratorData` (NOT nested in it), each REPLACING a legacy `mapGeneratorData.*` field its
// own builder's header comment documents in full; short Correction-number references kept per
// call below for traceability, without repeating that full explanation eight times.
void AppendSimulationDomainsJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document);

} // namespace Io
} // namespace SanmapGen

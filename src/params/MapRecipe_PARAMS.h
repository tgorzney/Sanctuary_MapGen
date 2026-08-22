// MapRecipe_PARAMS.h — the complete editable settings for one map (the recipe).
// Layer: PARAMS. This is exactly what `mapGeneratorData` serializes and what the
// deterministic shared-generation mode transmits (settings + seed regenerate the map).
// Aggregates the geometry, layer stack, strata, placement rules, and water. Excludes execution
// concerns (dispatch/backend) — those are not reproducible-recipe content.
#pragma once
#include <string>
#include <vector>
#include "Accumulation_PARAMS.h"
#include "Army_PARAMS.h"
#include "Atmosphere_PARAMS.h"
#include "DetailNormal_PARAMS.h"
#include "Flow_PARAMS.h"
#include "GeneralMapSettings_PARAMS.h"
#include "Geometry_PARAMS.h"
#include "GlobalMarkerSettings_PARAMS.h"
#include "LayerStack_PARAMS.h"
#include "MapArea_PARAMS.h"
#include "MarkerChain_PARAMS.h"
#include "MarkerInstance_PARAMS.h"
#include "MarkerRule_PARAMS.h"
#include "PropInstance_PARAMS.h"
#include "ScatterRule_PARAMS.h"
#include "Scenario_PARAMS.h"
#include "SlopeDefaults_PARAMS.h"
#include "Stratum_PARAMS.h"
#include "Symmetry_PARAMS.h"
#include "Water_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct MapRecipe {
    Geometry               geometry;
    // The document's own free-text identity (SANMAP_FORMAT_SPEC "Base" groups `name`/`credits`
    // alongside `width`/`length`/`height` — plain flat document-root fields, not a nested block).
    // Previously UI-session-only state on `MapExportOptions` with no import path (real data loss:
    // opening a `.sanmap` and re-exporting it silently reset both to these defaults);
    // STEP25_MapNameCredits_IO gives them a real PARAMS home so they round-trip. Same default
    // values `MapExportOptions` used to carry, so a brand-new recipe's export behavior is unchanged.
    std::string             mapName    = "mapdef";
    std::string             mapCredits = "Sanctuary Map Generator";
    // The top-level `GeneralMapSettings` section (SANMAP_FORMAT_SPEC Correction 2): Seed/
    // ScaleFeaturesToMapSize/TerrainMinHeight/WorldUnitsPerCell live ON `geometry` above (relocated
    // OUT of the legacy `mapGeneratorData` blob on the wire, not out of Geometry in memory) —
    // `generalMapSettings` itself holds only the one genuinely new field, `globalGravity`.
    GeneralMapSettings     generalMapSettings;
    LayerStack             layerStack;
    // The ONE per-stratum settings array (ARCH §7.1). A stage reads a span of it; no stage
    // keeps a private per-stratum array. Shorter than MapFields::stratumCount is legal —
    // strata past the end run on their defaults.
    std::vector<Stratum>    strata;
    // The shared-default layer any stratum with `bSlopeUseGlobal == true` resolves its slope
    // gate against (MASKING_SPEC §1.7) — a single global record, not a per-stratum type
    // (ARCH §7.1).
    SlopeDefaults           slopeDefaults;
    std::vector<MarkerRule> markerRules;
    std::vector<PropRule>   propRules;
    std::vector<DecalRule>  decalRules;
    std::vector<UnitRule>   unitRules;
    // Map-wide default icon/color/scale for the three resource marker kinds (ARCH §11, completes
    // SANMAP_FORMAT_SPEC Correction 7) — a flat sibling of `markerRules`, for now (a future
    // MarkersStack Group/Layer wrapper may fold this inside it later; not designed here).
    GlobalMarkerSettings    globalMarkerSettings;
    Water                   water;
    // Sun/sky/fog/wind rendering-presentation recipe (ATMOSPHERE_PARAMS_SPEC) — a flat sibling of
    // `water`, no PROC/PIPELINE stage reads it yet (see the spec's own "Where these land").
    Atmosphere              atmosphere;
    // Reserved homes for the future two-simulation velocity->accumulation model
    // (SANMAP_FORMAT_SPEC Correction 6) — flat siblings of `water`/`atmosphere`. `flow` carries the
    // one real field the spec names (`flowMapColor`, a preview tint); `accumulation` is genuinely
    // empty. No PROC consumer for either yet.
    Flow                    flow;
    Accumulation            accumulation;
    // Reserved home for the future layered-heightmap-delta system (SANMAP_FORMAT_SPEC Correction
    // 8) — a flat sibling of `flow`/`accumulation`. Carries the one live field the spec names
    // (`mapSize`); the layered-heightmap-delta system itself has no PROC consumer yet. NOT wired
    // to `DetailNormalTab_UI.h`'s `detailNormalSizeIndex`, which stays caller-owned tab state.
    DetailNormal            detailNormal;
    int                     globalSymmetryMask = SymmetryAxis::RotateHalfTurn;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `globalSymmetryMask`, NOT nested in a new sub-struct, so every existing
    // `recipe.globalSymmetryMask` call site (Placement_Rules_PROC.cpp/Placement_Hash_PROC.cpp/
    // IO/UI) stays untouched. Consumed by `AppendRadialTurns`/`BuildSymmetryOrbit`
    // (Placement_Symmetry_PROC.h, STEP23) via `ResolveRadialSymmetryRepeatCount`, the same
    // `bSymmetryUseGlobal` switch `ResolveSymmetryMask` already uses for `globalSymmetryMask`.
    // Clamped to `[Params::radialSymmetryRepeatCountMinimum, ...Maximum]` at every IO read site.
    int                     radialSymmetryRepeatCount = 3;
    // The aggregate home `Params::SymmetryDetection` was missing (retires SymmetryTab_UI.h SCOPE
    // NOTE 2's "caller-owned" framing — STEP16). `globalSymmetryMask` above stays the ONE home of
    // the mask (ARCH §7.1); this is a separate concept (what counts as symmetric vs. what
    // symmetry to produce), not a rival store for it.
    SymmetryDetection       symmetryDetection;
    // The six exotic-blend scalars (Symmetry_PARAMS.h) — zero PROC consumer yet, same posture as
    // `symmetryDetection` above.
    SymmetryBlend           symmetryBlend;
    // Hand-placed, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC) — round-trip fidelity
    // through the `.sanmap` `armies`/`areas`/`markers`/`chains` dictionaries is their entire
    // purpose; no PROC stage computes or reinterprets them.
    std::vector<Army>                armies;
    std::vector<MapArea>             areas;
    std::vector<MarkerInstanceGroup> markers;
    std::vector<MarkerChain>         chains;
    // Lobby-resolved spawn/alloy scenario data (ARCH_15_05_ParamsScenariosType.md §15.5, amended by
    // ARCH_15_10 §15.10) — same hand-authored, pass-through posture as armies/areas/markers/chains.
    Params::Scenarios                scenarios;
    // PARAMS types + pure JSON round-trip only (STEP4_PropsDecals_IO) — NOT yet live-wired into
    // BuildSanmapJsonText/ParseSanmapJsonText. See MapExporter_IO.h/MapImporter_IO.h SCOPE NOTES.
    std::vector<PropInstanceGroup>   props;
    std::vector<DecalInstanceGroup>  decals;
    std::vector<PropInstanceLayer>   propLayers;
    std::vector<DecalInstanceLayer>  decalLayers;

    bool IsValid() const { return geometry.IsValid(); }
};

} // namespace Params
} // namespace SanmapGen

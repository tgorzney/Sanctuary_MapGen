// MarkerInstance_PARAMS.h — the hand-placed marker roster: `MarkerTransform`, `MarkerInstanceGroup`.
// Layer: PARAMS. Manually-authored, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC "Scope"),
// same posture as Army_PARAMS.h: round-trip fidelity through the `.sanmap` `markers` two-level
// dictionary is the entire purpose, no PROC stage computes or reinterprets these fields.
// `MarkerTransform` COMPOSES `InstancedTransform` as a member — it does NOT flatten it like
// `UnitTransform` does (the spec's second-session ruling: `UnitTransform` was already shipped flat
// and is deliberately not retrofitted; `MarkerTransform` has no such history). Verbatim from
// ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section.
#pragma once
#include <string>
#include <vector>
#include "InstancedTransform_PARAMS.h"
#include "Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Params {

// STEP60: the manual-layer metadata array, one entry per authored marker layer, indexed by
// MarkerTransform::layerIndex. Same shape as PropInstanceLayer/DecalInstanceLayer (ARCH §12),
// plus the "Id" field/`symmetry` sub-record those two don't carry yet. Wire key is `MarkerGroups`
// (a fresh top-level array, PascalCase, no prior format precedent), a plain sibling of the
// `markers` name-keyed dictionary below, NOT nested inside it.
struct MarkerInstanceLayer {
    std::string name;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float iconScale = 1.0f;
    int   layerId = -1;   // stable id, present from day one — `-1` is the "unassigned" sentinel,
                           // identical convention to PropInstanceLayer::layerId/
                           // DecalInstanceLayer::layerId (STEP56 §1) once those land.
    Params::SymmetrySetting symmetry;   // ARCH_16_01_NewParamsShapes.md §16.1, SANMAP_FORMAT_SPEC
                                         // Correction 16. The layer's own mirror-mask setting; what
                                         // a future "place with symmetry" tool would resolve
                                         // against. No consumer/writer UI exists yet (that's
                                         // separate, unratified-as-a-ticket work) — this ticket
                                         // only gives the field its PARAMS+IO home, same posture as
                                         // `layerIndex` before STEP49's tab existed.
    bool  bSymmetryEnabled = true;      // ARCH §19.24. true (default, every pre-existing/legacy
                                         // layer): `symmetry`'s configured mask is live. false: every
                                         // reader must resolve the EFFECTIVE mask to
                                         // SymmetryAxis::None WITHOUT clearing `symmetry`'s own axis
                                         // configuration — re-enabling recovers it unchanged.
    bool  bLocked = false;                    // STEP106 §1. Blocks drag/reposition/add/remove for
                                               // every marker on this layer.
    bool  bHidden = false;                    // STEP144. Hides every marker on this layer from the
                                               // map preview WITHOUT touching bLocked or removing
                                               // anything — the Manual-side sibling of
                                               // MarkerRuleLayer::bHidden (that one also stays
                                               // "generated"; this one has no generation to keep
                                               // running, so it is a plain preview-visibility flag).
    bool  bGridSnapEnabled = false;            // STEP106 §2. Per-layer, not global (see §2).
    float gridSnapSizeWorldUnits = 1.0f;       // STEP106 §2. World-unit cell size; only meaningful
                                               // while bGridSnapEnabled is true.
    bool  bColorOverrideEnabled = false;       // STEP116. false (struct default — every pre-existing
                                               // and every STEP115-synthesized layer): `color` is
                                               // ignored, every marker on this layer resolves its
                                               // owning group's TYPE-default color instead
                                               // (GlobalMarkerSettings::colorAlloy/colorPlasma/
                                               // colorSpawn), white for an unrecognized group name.
                                               // true: `color` is used verbatim, including a
                                               // deliberately-chosen white.
    int parentBundleIdentifier = -1;   // ARCH §19.3/§19.4 — -1 = root (ungrouped). Additive.
    std::string markerTypeName;   // ARCH §19.13 — free-form, same string space as
                                   // MarkerLayerBundle::markerTypeName (STEP119), NOT MarkerCategory
                                   // (ARCH §19.21). Additive.
    int linkIdentifier = -1;      // ARCH §19.29 — the ACTUAL color/visibility-resolution key
                                   // (ARCH §19.31) — checked directly, never derived by walking up
                                   // parentBundleIdentifier, so a later re-nest of this Layer under
                                   // a different Group never silently changes which Link governs
                                   // its color/visibility. -1 = not Link-bound. Additive.
};

struct MarkerTransform {
    std::string name;                  // folded-in inner dict key — instance name (e.g. "Mex 0")
    InstancedTransform transform;
    std::string alias;                 // SanGen-added, already-ratified SANMAP_FORMAT_SPEC Correction 11
    int layerIndex = 0;                // indexes recipe.markerLayers, plain vector position
    // SanGen-added, already-ratified SANMAP_FORMAT_SPEC Correction 16 (STEP68). 0 = ungrouped, any
    // positive value groups this instance with every other MarkerTransform sharing the same value —
    // the field the future drag-and-follow UI (ARCH_16_MarkerLayerSymmetry.md) writes into.
    int symmetryGroupIdentifier = 0;
    std::string iconNameOverride;      // STEP114. Atlas-manifest NAME key, same shape as
                                        // GlobalMarkerSettings::iconName* (NOT the fixed-8-char tpId
                                        // convention MarkerRule::transform.templateIdentifier uses).
                                        // Empty = use the type default resolved from the owning
                                        // MarkerInstanceGroup::name. NEVER an atlas int index —
                                        // IconGridState::selectedIconId is a volatile per-session scan
                                        // order, never stable across a rescan or a save/load.
    // ARCH §19.16. Stable, GLOBALLY unique across every MarkerInstanceGroup's transforms (not
    // per-group) — never reused, -1 = unassigned. A THIRD bare int alongside layerIndex/
    // symmetryGroupIdentifier, spelled in full per §1.9 to stay unambiguous among the three. Exists
    // solely for stable UI-selection addressing (Ticket C) — carries no round-tripping/export-key
    // role of its own; MakeNamesUnique's existing name-based identity is untouched.
    int instanceIdentifier = -1;
    int linkIdentifier = -1;   // ARCH §19.33 — instance-tier Link membership; -1 = not Link-bound.
};

struct MarkerInstanceGroup {
    std::string name;                        // folded-in outer dict key — marker TYPE name
                                              // (e.g. "Spawn"/"Alloys") — free-form std::string,
                                              // NOT MarkerCategory; see the spec's cardinality ruling
    bool bResource = false;                  // format's `resource`, b-prefixed per §1.1
    std::vector<MarkerTransform> transforms;

    using TransformType = MarkerTransform;   // ARCH §21.9 — lets GroupT-generic consumers
                                              // (ManualInstanceHitTest_UI.h/ManualInstanceDelete_UI.h)
                                              // recover the owned transform type without a per-domain
                                              // trait specialization. Unused until STEP249.
};

// The fixed group name SANMAP_FORMAT_SPEC reserves for the commander-spawn roster (moved from
// UI-only `MarkersTab_Manual_UI.h::kSpawnMarkerGroupName`, ARCH_14_14, so IO/UI/PIPELINE consumers
// share one symbol instead of three independent occurrences of the same literal).
inline constexpr const char* kSpawnMarkerGroupName = "Spawn";

// A real, non-SanGen-authored `.sanmap` names its Alloy/Plasma groups in the PLURAL ("Alloys"/
// "Plasmas") — SanGen's own convention (this file, GlobalMarkerSettings_PARAMS.h,
// ResolveMarkerIconTemplateIdentifier/MapCanvas_IconLayer_CullManual_UI.cpp) is singular. Every
// Type-section-membership comparison needs the SAME alias folding those three sites already apply
// to color/scale/icon resolution, or a plural-named import (human's own bug report — Alloy markers
// vanishing from the Markers tab on a real map import) silently falls outside every Type-section's
// exact-string match. Any other group name (Generic/Expansion/freeform) passes through unchanged —
// this is alias resolution, not a taxonomy.
inline std::string CanonicalMarkerTypeSectionName(const std::string& groupName) {
    if (groupName == kSpawnMarkerGroupName || groupName == "Spawns") return kSpawnMarkerGroupName;
    if (groupName == "Alloy" || groupName == "Alloys")               return "Alloy";
    if (groupName == "Plasma" || groupName == "Plasmas")             return "Plasma";
    return groupName;
}

} // namespace Params
} // namespace SanmapGen

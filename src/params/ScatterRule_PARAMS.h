// ScatterRule_PARAMS.h — prop, decal and unit scatter rules.
// Layer: PARAMS. Settings only (PLACEMENT_SCATTER_SPEC). Scatter math lives in PROC.
// The v1 rules carried density + height/slope only; the spacing (Poisson radius), biome/
// mask gate, edge padding, symmetry and transform ranges below are the "missing capability"
// list from the spec, exposed as first-class tweakables (Constitution §8).
#pragma once
#include "FootprintBakeFingerprint_PARAMS.h"
#include "ScatterTransform_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct PropRule {
    bool  bEnabled  = true;
    float density   = 0.5f;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;
    bool  bAvoidWater = false;
    bool  bNearCliffs = false;
    // template-level: does this rule's placed instances belong to the Reclaim overlay
    // sub-layer partition, not the Props one (§14.2/§14.6)
    bool  bReclaimable = false;

    float spacingMinimum          = 0.0f;   // Poisson-disk radius, in cells
    int   mapEdgePadding          = 0;
    int   maskStratumIndex        = -1;     // -1 = no biome gate; else a surfaceStratumWeights index
    float maskWeightMinimum       = 0.0f;
    float obstacleDistanceMinimum = 0.0f;   // Jump-Flood exclusion
    float nearCliffDistanceMaximum = 0.0f;  // bNearCliffs: max distance to a steep cell

    // The real, ingested ground-plane extent for transform.templateIdentifier, UNSCALED -- the
    // per-template "base" size before ScatterTransform::scaleMinimum/scaleMaximum's per-instance
    // scale multiplier is applied (ARCH_18_02_IngestedDataDeterminism.md §18.2 does not rule on this
    // multiplication; a future PROC ticket that consumes this field must apply the instance's chosen
    // scale itself, not assume this value already includes it). "base" mirrors STEP58's own
    // baseFootprintWidth/baseFootprintDepth naming (WorldFootprintSizeTable_IO.h) for exactly the
    // same reason STEP58 chose it. Ordinary, hand-editable PARAMS value (§18.2 rule 3) -- the bake
    // action (PropsTab_Rules_UI.cpp) only ever fills in a STARTING value; nothing locks it afterward.
    // Default duplicates WorldFootprintSizeTable_IO.h's kDefaultPropFootprintSize{4.0f,4.0f} as a
    // literal, not an include -- Constitution §1 layering is IO -> {DATA, PARAMS}, never the reverse.
    float baseFootprintWidth = 4.0f;
    float baseFootprintDepth = 4.0f;
    // Empty/zeroed (FootprintBakeFingerprint::IsValid() == false) means "never baked" -- an ordinary,
    // non-error state (§18.2 rule 4). Populated only by "Resolve Footprint" (PropsTab_Rules_UI.cpp);
    // compared, never auto-rewritten, by Io::CheckFootprintBakeStaleness.
    FootprintBakeFingerprint footprintBakeFingerprint;

    bool  bSymmetryUseGlobal = true;
    int   symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Consumed by `AppendRadialTurns`/`BuildSymmetryOrbit` via
    // `ResolveRadialSymmetryRepeatCount` (STEP23), the same `bSymmetryUseGlobal` switch
    // `ResolveSymmetryMask` already uses for `symmetryMask`.
    int   radialSymmetryRepeatCount = 3;

    ScatterTransform transform;
};

struct DecalRule {
    bool  bEnabled  = true;
    float density   = 0.5f;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;

    float spacingMinimum   = 0.0f;
    int   mapEdgePadding   = 0;
    int   maskStratumIndex = -1;
    float maskWeightMinimum = 0.0f;

    bool  bSymmetryUseGlobal = true;
    int   symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Consumed by `AppendRadialTurns`/`BuildSymmetryOrbit` via
    // `ResolveRadialSymmetryRepeatCount` (STEP23), the same `bSymmetryUseGlobal` switch
    // `ResolveSymmetryMask` already uses for `symmetryMask`.
    int   radialSymmetryRepeatCount = 3;

    ScatterTransform transform;
};

// Pre-placed army units. Replaces the GUI widget's jittered rows x cols grid + rand()
// (PLACEMENT_SCATTER_SPEC "Unit spawning") with the same seeded scatter every other rule uses.
struct UnitRule {
    bool  bEnabled  = true;
    // A real, actively-maintained POSITIONAL index into recipe.armies (ArmiesTab_UI.h's own header
    // note; renumbered on army delete/reorder by DropUnitRulesForRemovedArmy/
    // RenumberUnitRuleArmyIndicesForReorder) — NOT a Faction value. Stale "Chosen=0, Guard=1, EDA=2"
    // comment corrected by ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-B.
    int   armyIndex = 0;
    int   count     = 0;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;

    float spacingMinimum   = 1.0f;
    int   mapEdgePadding   = 0;
    int   maskStratumIndex = -1;
    float maskWeightMinimum = 0.0f;

    // See PropRule's own comment above -- identical contract, UnitRule's own default duplicates
    // WorldFootprintSizeTable_IO.h's kDefaultUnitFootprintSize{2.0f,2.0f} as a literal.
    float baseFootprintWidth = 2.0f;
    float baseFootprintDepth = 2.0f;
    FootprintBakeFingerprint footprintBakeFingerprint;

    bool  bSymmetryUseGlobal = true;
    int   symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Consumed by `AppendRadialTurns`/`BuildSymmetryOrbit` via
    // `ResolveRadialSymmetryRepeatCount` (STEP23), the same `bSymmetryUseGlobal` switch
    // `ResolveSymmetryMask` already uses for `symmetryMask`.
    int   radialSymmetryRepeatCount = 3;

    ScatterTransform transform;
};

} // namespace Params
} // namespace SanmapGen

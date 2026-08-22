// PlacementRuleSections_UI.h — the rule blocks the four placement tabs repeat. Layer: UI.
// Accuracy class: Visual. Tab-rebuild C4 (Markers / Armies / Props / Areas).
//
// Marker, prop, decal and unit rules are four PARAMS structs carrying the SAME blocks: the
// symmetry axis mask, the biome/edge gate, the instance transform (Params::ScatterTransform) and
// the tpId + icon picker. They are written ONCE here so four tabs cannot drift apart — the v1
// duplicate-per-tab defect this rebuild exists to kill.
//
// Everything above the draw declarations is PURE (WidgetHelpers_UI.h "THE SPLIT"), so the mask
// arithmetic and the transform mirrors are testable with no imgui frame, window or GL context.
#pragma once
#include "IconGridWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "../params/ScatterTransform_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// v2's Params::SymmetryAxis is an OR-able flag set of five real bits (MirrorAcrossX,
// MirrorAcrossZ, RotateHalfTurn, QuarterTurns, Radial). Independent tick boxes are drawn rather
// than Checkbox_UI's exclusive row: an exclusive row cannot express e.g. X|Z, and dropping a
// combination a recipe already holds would be a widget overruling PARAMS. Radial's companion
// count field, `radialSymmetryRepeatCount`, is NOT drawn by this file — a real, separately
// tracked gap (STEP95 "Explicit out-of-scope"), not an oversight of this table.
enum : int { kPlacementSymmetryAxisCount = 5 };
inline const char* const placementSymmetryAxisLabels[kPlacementSymmetryAxisCount] = {
    "Mirror X", "Mirror Z", "Half Turn", "Quarter Turns", "Radial"
};

inline int PlacementSymmetryAxisBit(int axisIndex) {
    switch (axisIndex) {
        case 0:  return Params::SymmetryAxis::MirrorAcrossX;
        case 1:  return Params::SymmetryAxis::MirrorAcrossZ;
        case 2:  return Params::SymmetryAxis::RotateHalfTurn;
        case 3:  return Params::SymmetryAxis::QuarterTurns;
        case 4:  return Params::SymmetryAxis::Radial;
        default: return Params::SymmetryAxis::None;
    }
}

inline bool IsPlacementSymmetryAxisSet(int symmetryMask, int axisIndex) {
    const int axisBit = PlacementSymmetryAxisBit(axisIndex);
    return axisBit != Params::SymmetryAxis::None && (symmetryMask & axisBit) != 0;
}

inline int PlacementSymmetryMaskAfterToggle(int symmetryMask, int axisIndex) {
    const int axisBit = PlacementSymmetryAxisBit(axisIndex);
    if (axisBit == Params::SymmetryAxis::None) return symmetryMask;
    return (symmetryMask & axisBit) != 0 ? (symmetryMask & ~axisBit) : (symmetryMask | axisBit);
}

// The bits a mask may legally carry — how a mask from a hand-edited recipe is repaired rather
// than obeyed (Constitution §6).
inline int ResolvedPlacementSymmetryMask(int symmetryMask) {
    int legalBits = Params::SymmetryAxis::None;
    for (int axisIndex = 0; axisIndex < kPlacementSymmetryAxisCount; ++axisIndex)
        legalBits |= PlacementSymmetryAxisBit(axisIndex);
    return symmetryMask & legalBits;
}

// Caller-owned state for the transform block. Separations are zero: a scale range of exactly
// 1..1 (the PARAMS default, "no random scale") must survive being drawn.
struct PlacementTransformState {
    RangeSliderBounds scaleBounds{ 0.01f, 10.0f, 0.0f };
    RangeSliderBounds rotationBounds{ 0.0f, 360.0f, 0.0f };
    RealtimeToggle    scaleToggle;
    RealtimeToggle    rotationToggle;
    RangeSliderValues scaleValues{ 1.0f, 1.0f };
    RangeSliderValues rotationValues{ 0.0f, 360.0f };
    SectionState      transformSection;
};

inline void LoadPlacementTransformValues(const Params::ScatterTransform& transform,
                                         PlacementTransformState& state) {
    state.scaleValues.minimumValue    = transform.scaleMinimum;
    state.scaleValues.maximumValue    = transform.scaleMaximum;
    state.rotationValues.minimumValue = transform.rotationMinimumDegrees;
    state.rotationValues.maximumValue = transform.rotationMaximumDegrees;
}

// widget mirrors -> transform. Reports whether the recipe actually moved.
inline bool StorePlacementTransformValues(const PlacementTransformState& state,
                                          Params::ScatterTransform& transform) {
    const bool bMoved = state.scaleValues.minimumValue    != transform.scaleMinimum
                     || state.scaleValues.maximumValue    != transform.scaleMaximum
                     || state.rotationValues.minimumValue != transform.rotationMinimumDegrees
                     || state.rotationValues.maximumValue != transform.rotationMaximumDegrees;
    transform.scaleMinimum           = state.scaleValues.minimumValue;
    transform.scaleMaximum           = state.scaleValues.maximumValue;
    transform.rotationMinimumDegrees = state.rotationValues.minimumValue;
    transform.rotationMaximumDegrees = state.rotationValues.maximumValue;
    return bMoved;
}

// Caller-owned state for the biome / edge gate block.
struct PlacementGateState {
    ScalarSliderRange maskStratumIndexRange{ -1.0f, 8.0f, 1.0f };
    ScalarSliderRange maskWeightRange{ 0.0f, 1.0f, 0.0f };
    ScalarSliderRange edgePaddingRange{ 0.0f, 200.0f, 1.0f };
    RealtimeToggle    maskStratumIndexToggle;
    RealtimeToggle    maskWeightToggle;
    RealtimeToggle    edgePaddingToggle;
    SectionState      gateSection;
};

// The ONE thing a C4 tab does with a commit: WHICH dirty tier it becomes is PreviewDriver's
// derivation from the stage parameter hashes, never a call site's decision.
void NotifyPlacementChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver);

void DrawPlacementSymmetryAxes(const char* label, bool& bSymmetryUseGlobal, int& symmetryMask,
                               Pipeline::PreviewDriver* previewDriver);
// Five independent tick boxes over the real bit mask — no "Use Global" wrapper, for callers
// that ARE the global setting itself (unlike DrawPlacementSymmetryAxes's per-rule override use).
// The mask is REPAIRED (Constitution §6) before the boxes are drawn, so both this function and
// DrawPlacementSymmetryAxes give a hand-edited or otherwise-arrived-at mask identical treatment.
void DrawIndependentSymmetryAxes(int& symmetryMask, Pipeline::PreviewDriver* previewDriver);
void DrawPlacementTransformSection(Params::ScatterTransform& transform, PlacementTransformState& state,
                                   Pipeline::PreviewDriver* previewDriver);
void DrawPlacementGateSection(int& maskStratumIndex, float& maskWeightMinimum, int& mapEdgePadding,
                              PlacementGateState& state, Pipeline::PreviewDriver* previewDriver);
// `iconManifest` is nullable: with no resident atlas the picker degrades to the typed tpId.
void DrawPlacementTemplatePicker(Params::ScatterTransform& transform, IconGridState& iconGridState,
                                 float iconGridHeight, const IconAtlasManifest* iconManifest,
                                 Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

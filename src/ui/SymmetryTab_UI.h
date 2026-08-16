// SymmetryTab_UI.h — the Symmetry tab: the global axis mask plus the two promoted detection
// settings. Layer: UI. Accuracy class: Visual (it edits settings; it detects nothing itself).
// TAB_REBUILD_PLAN "1 · Symmetry".
//
// THE AXIS ROW. The plan asks for five exclusive options — Point, X, Z, XY, Radial — but
// `Params::SymmetryAxis` is a real bit mask in which XY is TWO bits (MirrorAcrossX|MirrorAcrossZ).
// So the row is drawn over a presentation word with one bit per OPTION (the batch-A exclusive
// checkbox group) and converted to and from the recipe's mask by the pure pair below. The recipe
// keeps exactly one home for the mask — `Params::MapRecipe::globalSymmetryMask` — and this tab
// never stores a second copy of it.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. The plan's symmetry ALGORITHM group — Algorithm combo {Fold, Blur, CrossFade, Cylinder3D,
//     Torus3D, NativeHash, Superposition}, Blur Radius, Cross-Fade Width, Superposition Blend,
//     Cylinder Z Scale, Torus Major/Minor Radius — is NOT drawn. None of those settings exists in
//     the v2 param tree, and there is no heightfield-symmetry PROC stage for them to drive (the
//     pipeline is NoiseBlend -> Erosion -> Thermal -> FlowAccumulation -> Mask -> Placement ->
//     Bake; `Placement_Symmetry_PROC` mirrors placed ENTITIES, not the field). The plan's "all
//     exist in params, expose all" is true of v1, not of this tree. A symmetry stage plus its
//     settings type is a work-order, not a side effect of a tab.
//  2. `Params::SymmetryDetection` has no aggregate home yet: `Params::MapRecipe` carries the mask
//     but no detection record, so the caller owns the instance and passes it in. Adding one line
//     to `MapRecipe_PARAMS.h` is not C1's to make (one owner per PARAMS file).
#pragma once
#include "Checkbox_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// The five options the row offers, in draw order. One presentation BIT each — deliberately not
// `Params::SymmetryAxis` values, because XY is a combination of two of those.
enum class SymmetryAxisOption : int { Point, MirrorX, MirrorZ, MirrorXZ, Radial, Count };

inline constexpr int kSymmetryAxisOptionCount = static_cast<int>(SymmetryAxisOption::Count);

inline const char* const symmetryAxisOptionLabels[kSymmetryAxisOptionCount] = {
    "Point", "X", "Z", "XY", "Radial"
};

// option -> the recipe's mask. An option outside the row answers None rather than a stray bit.
inline int SymmetryAxisMaskOfOption(int optionIndex) {
    switch (static_cast<SymmetryAxisOption>(optionIndex)) {
        case SymmetryAxisOption::Point:    return Params::SymmetryAxis::RotateHalfTurn;
        case SymmetryAxisOption::MirrorX:  return Params::SymmetryAxis::MirrorAcrossX;
        case SymmetryAxisOption::MirrorZ:  return Params::SymmetryAxis::MirrorAcrossZ;
        case SymmetryAxisOption::MirrorXZ: return Params::SymmetryAxis::MirrorAcrossX
                                                | Params::SymmetryAxis::MirrorAcrossZ;
        case SymmetryAxisOption::Radial:   return Params::SymmetryAxis::QuarterTurns;
        default:                           return Params::SymmetryAxis::None;
    }
}

// The recipe's mask -> the option that produces it, or -1 for None and for any combination the
// row cannot express (a hand-edited recipe). -1 draws no tick rather than snapping the user's
// mask onto a neighbouring option (Constitution §6).
inline int SymmetryAxisOptionOfMask(int axisMask) {
    for (int optionIndex = 0; optionIndex < kSymmetryAxisOptionCount; ++optionIndex)
        if (SymmetryAxisMaskOfOption(optionIndex) == axisMask) return optionIndex;
    return -1;
}

// The presentation word the exclusive checkbox row edits: one bit for the active option, empty
// when the mask names none of them.
inline unsigned int SymmetryOptionBitsOfMask(int axisMask) {
    const int optionIndex = SymmetryAxisOptionOfMask(axisMask);
    return optionIndex < 0 ? 0u : (1u << optionIndex);
}

// ...and back. An empty word is "no symmetry", which is a legal recipe state.
inline int SymmetryAxisMaskOfOptionBits(unsigned int optionBits) {
    for (int optionIndex = 0; optionIndex < kSymmetryAxisOptionCount; ++optionIndex)
        if (optionBits == (1u << optionIndex)) return SymmetryAxisMaskOfOption(optionIndex);
    return Params::SymmetryAxis::None;
}

// Caller-owned tab state. The option word is a MIRROR of the recipe's mask, never a second home.
struct SymmetryTabState {
    ScalarSliderRange detectionToleranceRange{ 0.0f, 0.25f, 0.0f };
    RealtimeToggle    detectionToleranceToggle;
    // One per section, held HERE and never as a function-local in the draw path: a local would
    // reset every frame, so a collapsed section could never stay collapsed (the v1 defect the
    // shared library exists to kill).
    SectionState      globalSymmetrySection;
    SectionState      detectionSection;
    unsigned int      axisOptionBits = 0u;
};

// recipe -> mirror.
inline void LoadSymmetryTabValues(int globalSymmetryMask, SymmetryTabState& state) {
    state.axisOptionBits = SymmetryOptionBitsOfMask(globalSymmetryMask);
}

// mirror -> recipe. Reports whether the mask actually moved.
inline bool StoreSymmetryTabValues(const SymmetryTabState& state, int& globalSymmetryMask) {
    const int axisMask = SymmetryAxisMaskOfOptionBits(state.axisOptionBits);
    if (axisMask == globalSymmetryMask) return false;
    globalSymmetryMask = axisMask;
    return true;
}

// Draws the tab. `previewDriver` may be null. `symmetryDetection` is caller-owned (SCOPE NOTE 2).
void DrawSymmetryTab(Params::MapRecipe& recipe, Params::SymmetryDetection& symmetryDetection,
                     SymmetryTabState& state, Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

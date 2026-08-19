// SymmetryTab_UI.h — the Symmetry tab: the global axis mask plus the two promoted detection
// settings. Layer: UI. Accuracy class: Visual (it edits settings; it detects nothing itself).
// TAB_REBUILD_PLAN "1 · Symmetry".
//
// THE AXIS ROW. `Params::SymmetryAxis` is a real OR-able bit mask (MirrorAcrossX / MirrorAcrossZ /
// RotateHalfTurn / QuarterTurns), so the row edits `recipe.globalSymmetryMask` directly through
// `DrawIndependentSymmetryAxes` (`PlacementRuleSections_UI.h`) — the same four-independent-tick-box
// widget already used by every per-rule symmetry override (Markers/Props/Decals/Units). The recipe
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
//  2. STALE, retired by STEP16_SymmetryGlobalSettings_IO: `Params::SymmetryDetection` now has its
//     aggregate home, `Params::MapRecipe::symmetryDetection` (MapRecipe_PARAMS.h). This note used
//     to read "no aggregate home yet ... the caller owns the instance and passes it in" — that
//     framing is what STEP16 retired. `DrawSymmetryTab`'s signature below still takes
//     `symmetryDetection` BY REFERENCE from the caller rather than reading `recipe.
//     symmetryDetection` directly: STEP16 was PARAMS/IO scope only and explicitly did not rewire
//     this draw call (a separate, already-tracked UI-wiring follow-up).
#pragma once
#include "Checkbox_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// Caller-owned tab state. No mirror of the mask lives here: `DrawAxisRow` edits
// `recipe.globalSymmetryMask` directly through `DrawIndependentSymmetryAxes`.
struct SymmetryTabState {
    ScalarSliderRange detectionToleranceRange{ 0.0f, 0.25f, 0.0f };
    RealtimeToggle    detectionToleranceToggle;
    // One per section, held HERE and never as a function-local in the draw path: a local would
    // reset every frame, so a collapsed section could never stay collapsed (the v1 defect the
    // shared library exists to kill).
    SectionState      globalSymmetrySection;
    SectionState      detectionSection;
};

// Draws the tab. `previewDriver` may be null. `symmetryDetection` is caller-owned (SCOPE NOTE 2).
void DrawSymmetryTab(Params::MapRecipe& recipe, Params::SymmetryDetection& symmetryDetection,
                     SymmetryTabState& state, Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

// Application_HostedSettings_UI.h — the settings the rebuilt tabs edit that have no home in
// `Params::MapRecipe` yet, held by the one object that legally owns them: the shell.
// Layer: UI. A member file of Application_UI.h (ARCH §1.5).
//
// EVERY FIELD HERE IS A SCOPE NOTE ALREADY WRITTEN BY THE TAB THAT NEEDS IT (ARCH §8.4 — a coder
// never invents a missing field, and the host never adds one to a PARAMS file it does not own):
//   symmetryDetection    SymmetryTab_UI.h SCOPE NOTE 2 — `MapRecipe` carries the axis mask but no
//                        detection record, so the CALLER owns the instance and passes it in.
//   the four LayerStacks MaskLayerTab_UI.h SCOPE NOTE 1 — `MapRecipe` carries exactly one
//                        `layerStack` (the Heightmap's GeoLayers); v1's DetailNormal/Tint/Hole/
//                        Smoothness stacks have no v2 counterpart, so each tab takes the stack it
//                        edits from its caller.
// They are held together, in their own file, so that the day `MapRecipe_PARAMS.h` grows the fields
// this struct empties out and nothing else in the shell moves.
#pragma once
#include "../params/LayerStack_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ApplicationHostedSettings {
    Params::SymmetryDetection symmetryDetection;
    Params::LayerStack        detailNormalLayers;
    Params::LayerStack        tintLayers;
    Params::LayerStack        holeLayers;
    Params::LayerStack        smoothnessLayers;
};

} // namespace Ui
} // namespace SanmapGen

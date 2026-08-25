// LayerEditor_BakedImage_UI.h — applies the Layer Editor's two reported-only actions
// (LayerEditor_Action_UI.h's SCOPE NOTE 2) now that STEP99/100/101 give both a real PARAMS/DATA
// target. Layer: UI. NOT header-only like LayerEditor_Action_UI.h: this is the first thing in the
// Layer Editor row-action family that legitimately needs IO (Import RAW reads a file off disk), so
// it is its own small translation unit rather than an exception carved into the IO-free header —
// same precedent as LayerEditor_Erosion_UI.h reaching Pipeline::GenerationAssembler for a
// cross-cutting concern with no PARAMS home of its own.
#pragma once
#include "LayerEditor_Action_UI.h"

namespace SanmapGen {
namespace Pipeline { class GenerationAssembler; }
namespace Ui {

// Applies ImportRawRequested / BakeToggleRequested / RefreshBakeRequested to the named layer.
// False (layer untouched) for every other kind, an unnamed layer, or an Import RAW that
// Io::MapImporter refused (a bad extension already fenced by DrawFilePathPicker, or a file that
// failed to read/decode — logged by MapImportResult, Constitution §6, never a crash). Returns
// true when the RECIPE moved, the same contract ApplyLayerEditorAction uses.
//
// Bake is a REAL TOGGLE, not one-way, and NEVER DESTROYS DATA (STEP151): Off -> On reuses an
// existing Data::BakedLayerImage verbatim (an import, or an earlier snapshot) whenever one already
// has content, and only snapshots CURRENT Proc::NoiseBlendStage::CachedRawNoiseCpu() output for a
// genuine first-ever bake; unbaking simply clears bBaked, resuming live generation from the
// layer's own still-present noise recipe (every field below `layerIdentifier` on Params::Layer).
// Import RAW always ends baked, since an imported file has no live recipe to resume.
//
// RefreshBakeRequested (STEP151) is the only path allowed to deliberately overwrite an EXISTING
// snapshot with the layer's current live noise — only meaningful, and only applied, on an unbaked
// layer that still has a live recipe (`Params::NoiseType::None` has nothing to refresh from). It
// never flips `bBaked` itself.
bool ApplyBakedImageAction(const LayerEditorAction& action, Params::LayerStack& layerStack,
                           Pipeline::GenerationAssembler& generationAssembler);

} // namespace Ui
} // namespace SanmapGen

// MaskLayerTab_UI.h — the shape the four mask-layer tabs share: a show-overlay toggle above one
// hosted Layer Editor. Layer: UI. Accuracy class: Visual (it edits settings; it simulates nothing).
// TAB_REBUILD_PLAN "7 · Detail Normal", "8 · Tint", "9 · Holes", "10 · Smoothness" — four tabs that
// differ only in their label and which stack they edit, so the composition is written ONCE here and
// each tab is a thin, named entry point (DetailNormalTab_UI.h, TintTab_UI.h, HolesTab_UI.h,
// SmoothnessTab_UI.h).
//
// It hosts the batch-B `DrawLayerEditor` verbatim — it re-implements no part of a stack editor.
// Each tab owns its OWN MaskLayerTabState, so two mask stacks on screen cannot share a drag or a
// selection (the v1 function-static bug the widget library exists to kill).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing field; reported, not invented):
//  1. THE STACKS HAVE NO RECIPE HOME. `Params::MapRecipe` carries exactly one `layerStack` (the
//     GeoLayers the Heightmap tab edits); v1's `DetailNormalLayers`, `TintLayers`, `HoleLayers` and
//     `SmoothnessLayers` have no v2 counterpart. So each tab takes the `Params::LayerStack&` it
//     edits from its caller. Adding the four fields to `MapRecipe_PARAMS.h` is a work-order —
//     `MapRecipe` is a shared aggregate and no batch owns it (TABREBUILD conflict rule 4).
//  2. THE SHOW-OVERLAY TOGGLE is preview presentation, not recipe: it belongs to the composite's
//     `PreviewFieldLayer::bEnabled`. It is caller-owned tab state here and the host work-order (E)
//     maps it onto the composite, as it does for the left column's `[O]` toggles.
#pragma once
#include "LayerEditor_UI.h"
#include "Section_UI.h"

namespace SanmapGen {
namespace Params { struct LayerStack; }
namespace Pipeline { class GenerationAssembler; class PreviewDriver; }
namespace Ui {

// Caller-owned state for ONE mask-layer tab.
struct MaskLayerTabState {
    bool             bShowOverlay = true;     // SCOPE NOTE 2
    SectionState     layerSection;
    LayerEditorState layerEditor;
};

// Draws the overlay toggle and the hosted editor. `identifier` scopes the imgui ids so four tabs
// (or one tab beside the Heightmap's editor) cannot collide; `overlayLabel` is the toggle's text.
// Both pipeline pointers are nullable — an editor drawn with no pipeline behind it still edits the
// stack (LayerEditor_UI.h).
void DrawMaskLayerTab(const char* identifier, const char* overlayLabel, const char* sectionLabel,
                      Params::LayerStack& layerStack, MaskLayerTabState& state,
                      Pipeline::GenerationAssembler* generationAssembler,
                      Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

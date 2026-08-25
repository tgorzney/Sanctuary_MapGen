// LayerEditor_Signals_UI_Test.cpp — tab-rebuild B acceptance, part 3: the row actions, the order
// one frame's collected signals are applied in, and the selection fence. All pure — the editor's
// draw path MUTATES NOTHING, so every structural edit is assertable with no window open.
// main() lives in LayerEditor_UI_Test.cpp.
#include "LayerEditor_Draw_UI.h"
#include "LayerEditor_Signals_UI.h"
#include "LayerEditor_TestSupport_UI.h"
#include "LayerEditor_UI.h"
#include <string>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

Params::LayerStack TwoGroupStack() {
    Params::LayerStack layerStack;
    layerStack.geoLayers.resize(2);
    layerStack.geoLayers[0].name = "Bedrock";
    layerStack.geoLayers[0].layers.resize(3);
    layerStack.geoLayers[0].layers[1].stratumIndex = 5;
    layerStack.geoLayers[0].layers[1].name = "Ridges";
    layerStack.geoLayers[1].name = "Mountains";
    layerStack.geoLayers[1].layers.resize(1);
    return layerStack;
}

void RunRowActionChecks() {
    Params::LayerStack layerStack = TwoGroupStack();
    int selectedGroup = 0;
    int selectedLayer = 1;

    LayerEditorAction duplicate;
    RecordLayerEditorAction(duplicate, LayerEditorActionKind::DuplicateLayer, 0, 1);
    CheckLayerEditor(ApplyLayerEditorAction(layerStack, duplicate, selectedGroup, selectedLayer),
                     "Duplicate moves the recipe");
    CheckLayerEditor(layerStack.geoLayers[0].layers.size() == 4u, "and the group grew by one");
    CheckLayerEditor(layerStack.geoLayers[0].layers[1].stratumIndex == 5
                     && layerStack.geoLayers[0].layers[2].stratumIndex == 5,
                     "the copy lands directly above its source");
    CheckLayerEditor(layerStack.geoLayers[0].layers[1].name == "Ridges"
                     && layerStack.geoLayers[0].layers[2].name == "Ridges",
                     "and the duplicate carries the source's name (WO B2)");
    CheckLayerEditor(selectedLayer == 1, "and the selection follows the copy");

    // STEP102: duplicating a BAKED layer must reset the copy's identity (layerIdentifier/bBaked),
    // or source and copy would share one Data::BakedLayerImage cache key (STEP99's flagged bug).
    layerStack.geoLayers[0].layers[1].bBaked          = true;
    layerStack.geoLayers[0].layers[1].bakedImagePath  = "heightmap.raw";
    layerStack.geoLayers[0].layers[1].layerIdentifier = 7;
    LayerEditorAction duplicateBaked;
    RecordLayerEditorAction(duplicateBaked, LayerEditorActionKind::DuplicateLayer, 0, 1);
    CheckLayerEditor(ApplyLayerEditorAction(layerStack, duplicateBaked, selectedGroup, selectedLayer),
                     "Duplicate on a baked layer moves the recipe");
    // "The copy lands directly ABOVE its source" (line 38's own precedent): insert() takes the
    // source's OLD index for the copy and shifts the source itself one slot later.
    CheckLayerEditor(!layerStack.geoLayers[0].layers[1].bBaked
                     && layerStack.geoLayers[0].layers[1].layerIdentifier == -1,
                     "the copy lands unbaked with a distinct (unassigned) identifier");
    CheckLayerEditor(layerStack.geoLayers[0].layers[2].bBaked
                     && layerStack.geoLayers[0].layers[2].layerIdentifier == 7,
                     "and the (shifted) source layer's own baked identity is untouched");
    CheckLayerEditor(layerStack.geoLayers[0].layers[1].bakedImagePath == "heightmap.raw",
                     "while bakedImagePath and every noise field still copy verbatim");

    LayerEditorAction addLayer;
    RecordLayerEditorAction(addLayer, LayerEditorActionKind::AddLayer, 1);
    ApplyLayerEditorAction(layerStack, addLayer, selectedGroup, selectedLayer);
    CheckLayerEditor(layerStack.geoLayers[1].layers.size() == 2u && selectedGroup == 1
                     && selectedLayer == 1, "Add Layer appends and selects the new row");

    LayerEditorAction addGroup;
    RecordLayerEditorAction(addGroup, LayerEditorActionKind::AddGeoLayer, -1);
    ApplyLayerEditorAction(layerStack, addGroup, selectedGroup, selectedLayer);
    CheckLayerEditor(layerStack.geoLayers.size() == 3u && selectedGroup == 2,
                     "Add GeoLayer appends and selects the new group");

    // The two reported-only kinds (no PARAMS home yet) must never touch the stack.
    const std::size_t groupCountBefore = layerStack.geoLayers.size();
    for (LayerEditorActionKind kind : { LayerEditorActionKind::ImportRawRequested,
                                        LayerEditorActionKind::BakeToggleRequested }) {
        LayerEditorAction reported;
        RecordLayerEditorAction(reported, kind, 0, 0);
        CheckLayerEditor(!ApplyLayerEditorAction(layerStack, reported, selectedGroup, selectedLayer),
                         "Import RAW / Bake are reported, never applied");
    }
    CheckLayerEditor(layerStack.geoLayers.size() == groupCountBefore, "and the stack is untouched");

    LayerEditorAction firstWins;
    RecordLayerEditorAction(firstWins, LayerEditorActionKind::AddLayer, 0);
    RecordLayerEditorAction(firstWins, LayerEditorActionKind::DuplicateLayer, 1, 0);
    CheckLayerEditor(firstWins.kind == LayerEditorActionKind::AddLayer,
                     "one frame records at most one action, first wins");
}

// The apply ORDER is the whole point of collecting signals: a group delete would invalidate the
// indices the layer signal is expressed in.
void RunFrameSignalOrderChecks() {
    Params::LayerStack layerStack = TwoGroupStack();
    int selectedGroup = 0;
    int selectedLayer = 0;

    LayerEditorFrameSignals signals;
    signals.layerSignalGroupIndex      = 0;
    signals.layerSignal.kind           = DraggableListSignalKind::Delete;
    signals.layerSignal.sourceRowIndex = 2;
    signals.groupSignal.kind           = DraggableListSignalKind::Delete;
    signals.groupSignal.sourceRowIndex = 1;
    CheckLayerEditor(ApplyLayerEditorFrameSignals(layerStack, signals, selectedGroup, selectedLayer),
                     "a frame carrying both deletes moves the recipe");
    CheckLayerEditor(layerStack.geoLayers.size() == 1u, "the group delete landed");
    CheckLayerEditor(layerStack.geoLayers[0].layers.size() == 2u,
                     "and the inner delete hit the row it named, not a shifted one");

    LayerEditorFrameSignals visibility;
    visibility.groupSignal.kind           = DraggableListSignalKind::ToggleVisibility;
    visibility.groupSignal.sourceRowIndex = 0;
    ApplyLayerEditorFrameSignals(layerStack, visibility, selectedGroup, selectedLayer);
    CheckLayerEditor(!layerStack.geoLayers[0].bEnabled, "a group visibility toggle reaches the recipe");

    LayerEditorFrameSignals lock;
    lock.groupSignal.kind           = DraggableListSignalKind::ToggleLock;
    lock.groupSignal.sourceRowIndex = 0;
    ApplyLayerEditorFrameSignals(layerStack, lock, selectedGroup, selectedLayer);
    CheckLayerEditor(IsGeoLayerLocked(layerStack.geoLayers[0]),
                     "the derived group lock sets every layer (LayersTab_UI reuse, not a copy)");

    selectedGroup = 9;
    selectedLayer = 9;
    ClampLayerEditorSelection(layerStack, selectedGroup, selectedLayer);
    CheckLayerEditor(selectedGroup == 0 && selectedLayer == 1,
                     "the selection is pinned back inside a stack that shrank");

    Params::LayerStack emptyStack;
    ClampLayerEditorSelection(emptyStack, selectedGroup, selectedLayer);
    CheckLayerEditor(selectedGroup == -1 && selectedLayer == -1,
                     "an empty stack selects nothing rather than index zero");
}

// STEP150: the header's Bake/Unbake affordance label and the signal->action mapping it wires
// through are both pure, so both are assertable with no imgui frame — the drawing itself (which
// row gets `describeRow`'s `extraButtonLabel`, and where DrawFilePathPicker/the gated sections sit)
// is "verified by eye against a live frame, never by test", the same posture every other batch-A
// widget's .cpp states for its own drawing half.
void RunBakeToggleRowAffordanceChecks() {
    CheckLayerEditor(std::string(LayerEditorBakeToggleButtonLabel(false)) == "Bake##bakeToggle",
                     "an unbaked layer's header affordance reads Bake");
    CheckLayerEditor(std::string(LayerEditorBakeToggleButtonLabel(true)) == "Unbake##bakeToggle",
                     "and a baked layer's reads Unbake");
    CheckLayerEditor(std::string(LayerEditorBakeToggleButtonLabel(false)).find("##bakeToggle")
                     != std::string::npos
                     && std::string(LayerEditorBakeToggleButtonLabel(true)).find("##bakeToggle")
                     != std::string::npos,
                     "both labels share the SAME id salt, so a click never drops imgui's active id "
                     "mid-press when the label flips (DraggableListWidget_UI.h's own icon precedent)");

    LayerEditorFrameSignals signals;
    DraggableListSignal extraButtonSignal;
    extraButtonSignal.kind           = DraggableListSignalKind::ExtraButton;
    extraButtonSignal.sourceRowIndex = 2;
    RecordBakeToggleFromRowSignal(extraButtonSignal, 5, signals);
    CheckLayerEditor(signals.action.kind == LayerEditorActionKind::BakeToggleRequested,
                     "the header affordance's click records the SAME BakeToggleRequested kind the "
                     "old in-body button used");
    CheckLayerEditor(signals.action.geoLayerIndex == 5 && signals.action.layerIndex == 2,
                     "carrying the row's own group/layer indices");

    // Every other row affordance's signal kind must never be mistaken for the bake toggle.
    for (DraggableListSignalKind otherKind : { DraggableListSignalKind::Delete,
                                               DraggableListSignalKind::ToggleVisibility,
                                               DraggableListSignalKind::ToggleLock,
                                               DraggableListSignalKind::Select,
                                               DraggableListSignalKind::Reorder,
                                               DraggableListSignalKind::None }) {
        LayerEditorFrameSignals untouched;
        DraggableListSignal otherSignal;
        otherSignal.kind           = otherKind;
        otherSignal.sourceRowIndex = 2;
        RecordBakeToggleFromRowSignal(otherSignal, 5, untouched);
        CheckLayerEditor(!untouched.action.bHasAction(),
                         "a non-ExtraButton row signal never records a bake action");
    }

    // First-wins: an action already recorded this frame (e.g. Duplicate) is not clobbered.
    LayerEditorFrameSignals alreadyActed;
    RecordLayerEditorAction(alreadyActed.action, LayerEditorActionKind::DuplicateLayer, 0, 0);
    RecordBakeToggleFromRowSignal(extraButtonSignal, 5, alreadyActed);
    CheckLayerEditor(alreadyActed.action.kind == LayerEditorActionKind::DuplicateLayer,
                     "the bake mapping respects the frame's own first-wins action rule");
}

void RunSelectedLayerChecks() {
    Params::LayerStack layerStack = TwoGroupStack();
    LayerEditorState state;
    state.selectedGeoLayerIndex = 0;
    state.selectedLayerIndex    = 1;
    CheckLayerEditor(SelectedLayerEditorLayer(layerStack, state) == &layerStack.geoLayers[0].layers[1],
                     "the selection resolves to the layer the panels edit");
    state.selectedLayerIndex = 12;
    CheckLayerEditor(SelectedLayerEditorLayer(layerStack, state) == nullptr,
                     "and an out-of-range selection resolves to nothing");
}

} // namespace

void RunLayerEditorSignalChecks() {
    RunRowActionChecks();
    RunFrameSignalOrderChecks();
    RunBakeToggleRowAffordanceChecks();
    RunSelectedLayerChecks();
}

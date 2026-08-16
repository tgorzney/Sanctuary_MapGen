// Application_TabState_UI.h — the caller-owned interaction state of every panel the shell mounts.
// Layer: UI. A MEMBER file of Application_UI.h — exactly the standing MapCanvasView_UI.h has to
// MapCanvas_UI.h — split out only for the ARCH §1.5 ceiling. Nothing outside the shell reaches it.
//
// It invents NO settings type (ARCH §8.4): every field is a state struct one of the M5-6 tabs or
// the M5-3 gradient editor already publishes. The shell owns one instance of each because the
// tabs are deliberately stateless functions over caller-owned state.
#pragma once
#include <vector>
#include "GradientEditorWidget_UI.h"
#include "LayersTab_UI.h"
#include "MarkersTab_UI.h"
#include "PropsTab_UI.h"
#include "SystemTab_UI.h"
#include "TerrainTab_UI.h"
#include "WaterTab_UI.h"

namespace SanmapGen {
namespace Ui {

// Which panel the left pane shows. Declaration order is the switcher's draw order. `Preview` is
// the shell's own panel over `PreviewCompositeSettings` (presentation, not recipe content) — no
// M5-6 tab owns the ramps, and they are not `Params::MapRecipe` content, so their editor lives
// with the object that owns them: the shell.
enum class ApplicationPanel { Terrain, Layers, Water, Markers, Props, Preview, System };

struct ApplicationTabState {
    TerrainTabState terrain;
    LayersTabState  layers;
    WaterTabState   water;
    MarkersTabState markers;
    PropsTabState   props;
    SystemTabState  system;

    // One gradient editor per composite ramp, grown to match on the frame the panel draws.
    std::vector<GradientEditorState> gradientEditors;

    ApplicationPanel activePanel = ApplicationPanel::Terrain;

    // The icon id each picker last reported, so the shell can spot a NEW selection and resolve it
    // back to a template identifier (the atlas-id bridge, Application_Assets_UI.cpp). -1 = none.
    int lastMarkerIconId = -1;
    int lastPropIconId   = -1;
};

} // namespace Ui
} // namespace SanmapGen

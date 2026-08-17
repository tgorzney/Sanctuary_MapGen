// Application_TabState_UI.h — the caller-owned interaction state of every panel the shell mounts.
// Layer: UI. A MEMBER file of Application_UI.h — exactly the standing MapCanvasView_UI.h has to
// MapCanvas_UI.h — split out only for the ARCH §1.5 ceiling. Nothing outside the shell reaches it.
//
// It invents NO settings type (ARCH §8.4): every field is a state struct one of the rebuilt tabs
// already publishes. The shell owns one instance of each because the tabs are deliberately
// stateless functions over caller-owned state, and one instance EACH because two tabs sharing a
// selection or a drag is the v1 function-static defect the widget library exists to kill.
//
// The panel enum, the group headers and the `[O]`/`[ ]` mapping live in Application_Panels_UI.h and
// Application_Visibility_UI.h; the settings with no recipe home in Application_HostedSettings_UI.h.
#pragma once
#include <vector>
#include "AccumulationTab_UI.h"
#include "Application_Panels_UI.h"
#include "Application_Visibility_UI.h"
#include "AreasTab_UI.h"
#include "ArmiesTab_UI.h"
#include "AtmosphereTab_UI.h"
#include "DetailNormalTab_UI.h"
#include "FilesTab_UI.h"
#include "FlowTab_UI.h"
#include "GradientEditorWidget_UI.h"
#include "HeightmapTab_UI.h"
#include "MarkersTab_UI.h"
#include "MaskLayerTab_UI.h"
#include "PropsTab_UI.h"
#include "Section_UI.h"
#include "SlopeTab_UI.h"
#include "StratumsTab_UI.h"
#include "SymmetryTab_UI.h"
#include "SystemTab_UI.h"
#include "WaterTab_UI.h"

namespace SanmapGen {
namespace Ui {

struct ApplicationTabState {
    // --- TERRAIN & LAYERS
    SymmetryTabState     symmetry;
    HeightmapTabState    heightmap;
    SlopeTabState        slope;
    FlowTabState         flow;
    AccumulationTabState accumulation;
    StratumsTabState     stratums;
    DetailNormalTabState detailNormal;
    MaskLayerTabState    tint;
    MaskLayerTabState    holes;
    MaskLayerTabState    smoothness;

    // --- ENVIRONMENT
    WaterTabState        water;
    AtmosphereTabState   atmosphere;
    MarkersTabState      markers;
    ArmiesTabState       armies;
    PropsTabState        props;
    AreasTabState        areas;

    // --- SYSTEM
    SystemTabState       system;
    FilesTabState        files;
    // The Performance panel's own collapsing header. Held here, never as a draw-path local: a
    // local would reset every frame, so a collapsed section could never stay collapsed.
    SectionState         performanceSection;

    // One gradient editor per composite ramp, grown to match on the frame the panel draws. The
    // shell edits the HEIGHT ramp itself because no tab owns it: it is composite presentation, not
    // recipe content, and the Slope/Flow/Accumulation/Water tabs each edit only their own.
    std::vector<GradientEditorState> gradientEditors;

    ApplicationPanel           activePanel = ApplicationPanel::Heightmap;   // v1's first tab
    ApplicationVisibilityState visibility;

    // The icon id each picker last reported, so the shell can spot a NEW selection and resolve it
    // back to a template identifier (the atlas-id bridge, Application_Assets_UI.cpp). -1 = none.
    int lastMarkerIconId = -1;
    int lastPropIconId   = -1;
};

} // namespace Ui
} // namespace SanmapGen

// Application_Panels_UI.h — the left column's catalogue: which panels exist, in which group, in
// which order, which of them carry the v1 `[O]`/`[ ]` preview-visibility toggle, and what that
// toggle drives in the composite. Layer: UI. Accuracy class: Visual.
// A member file of Application_UI.h (ARCH §1.5), exactly as Application_Settings_UI.h is.
//
// It is PURE DATA plus pure lookups — no imgui, no shell member — so the layout the work-order
// specifies (TAB_REBUILD_PLAN "Layout (keep v1 shape)") is asserted headlessly rather than eyeballed
// against a screenshot. The draw path (Application_LeftColumn_UI.cpp) reads this table and invents
// no row of its own; the panel bodies (Application_Panel*_UI.cpp) read the same enum.
#pragma once
#include "PreviewComposite_Settings_UI.h"

namespace SanmapGen {
namespace Ui {

// The three v1 group headers, in draw order.
enum class ApplicationPanelGroup : int { TerrainAndLayers, Environment, System, Count };

inline constexpr int kApplicationPanelGroupCount = static_cast<int>(ApplicationPanelGroup::Count);

inline const char* const applicationPanelGroupLabels[kApplicationPanelGroupCount] = {
    "TERRAIN & LAYERS", "ENVIRONMENT", "SYSTEM"
};

// Every panel the shell hosts. Declaration order IS the left column's order.
enum class ApplicationPanel : int {
    Symmetry, Heightmap, Slope, Flow, Accumulation, Stratums, DetailNormal, Tint, Holes, Smoothness,
    Water, Atmosphere, Markers, Armies, Props, Areas,
    Performance, Files, Count
};

inline constexpr int kApplicationPanelCount = static_cast<int>(ApplicationPanel::Count);

// What a row's `[O]` toggle switches off in the composite. `None` = the panel keeps the v1 toggle
// but the composite carries nothing it can drive (see Application_Visibility_UI.h SCOPE NOTE).
enum class PreviewVisibilityTarget : int { None, FieldLayer, Entities };

struct ApplicationPanelEntry {
    ApplicationPanel        panel;
    const char*             label;
    ApplicationPanelGroup   group;
    bool                    bHasVisibilityToggle;
    bool                    bVisibleByDefault;
    PreviewVisibilityTarget visibilityTarget;
    PreviewLayerKind        fieldLayerKind;   // read only when visibilityTarget == FieldLayer
};

// The catalogue. The overlays (Slope/Flow/Accumulation) start HIDDEN: v1 drew them as exclusive
// preview modes, while v2 composites them over the terrain, so v1's "on" default would paint the
// height ramp out on the first frame. Everything else keeps v1's own default.
inline constexpr ApplicationPanelEntry applicationPanelEntries[kApplicationPanelCount] = {
    { ApplicationPanel::Symmetry,     "Symmetry",      ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Heightmap,    "Heightmap",     ApplicationPanelGroup::TerrainAndLayers,
      true,  true,  PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Slope,        "Slope",         ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::Slope },
    { ApplicationPanel::Flow,         "Flow",          ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::Flow },
    { ApplicationPanel::Accumulation, "Accumulation",  ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::Accumulation },
    { ApplicationPanel::Stratums,     "Stratums",      ApplicationPanelGroup::TerrainAndLayers,
      true,  true,  PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::StratumSplat },
    { ApplicationPanel::DetailNormal, "Detail Normal", ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Tint,         "Tint",          ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Holes,        "Holes",         ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Smoothness,   "Smoothness",    ApplicationPanelGroup::TerrainAndLayers,
      true,  false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Water,        "Water",         ApplicationPanelGroup::Environment,
      true,  true,  PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::Water },
    { ApplicationPanel::Atmosphere,   "Atmosphere",    ApplicationPanelGroup::Environment,
      true,  false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Markers,      "Markers",       ApplicationPanelGroup::Environment,
      true,  true,  PreviewVisibilityTarget::Entities,   PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Armies,       "Armies",        ApplicationPanelGroup::Environment,
      true,  false, PreviewVisibilityTarget::Entities,   PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Props,        "Props",         ApplicationPanelGroup::Environment,
      true,  false, PreviewVisibilityTarget::Entities,   PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Areas,        "Areas",         ApplicationPanelGroup::Environment,
      true,  false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Performance,  "Performance",   ApplicationPanelGroup::System,
      false, false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
    { ApplicationPanel::Files,        "Files",         ApplicationPanelGroup::System,
      false, false, PreviewVisibilityTarget::None,       PreviewLayerKind::HeightRamp },
};

// The row a panel sits on, or -1 for a value outside the enum (Constitution §6).
inline int ApplicationPanelIndexOf(ApplicationPanel panel) {
    const int panelIndex = static_cast<int>(panel);
    if (panelIndex < 0 || panelIndex >= kApplicationPanelCount) return -1;
    return panelIndex;
}

// The catalogue entry for a panel, or null when the value names none.
inline const ApplicationPanelEntry* ApplicationPanelEntryOf(ApplicationPanel panel) {
    const int panelIndex = ApplicationPanelIndexOf(panel);
    return panelIndex < 0 ? nullptr : &applicationPanelEntries[panelIndex];
}

// The group's header text, or an empty string for a value outside the enum — never a null label.
inline const char* ApplicationPanelGroupLabel(ApplicationPanelGroup group) {
    const int groupIndex = static_cast<int>(group);
    if (groupIndex < 0 || groupIndex >= kApplicationPanelGroupCount) return "";
    return applicationPanelGroupLabels[groupIndex];
}

// How many panels a group carries — the count the left column draws under one header.
inline int ApplicationPanelCountOfGroup(ApplicationPanelGroup group) {
    int panelCount = 0;
    for (const ApplicationPanelEntry& entry : applicationPanelEntries)
        if (entry.group == group) ++panelCount;
    return panelCount;
}

} // namespace Ui
} // namespace SanmapGen

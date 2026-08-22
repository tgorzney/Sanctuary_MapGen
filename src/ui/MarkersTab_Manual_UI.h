// MarkersTab_Manual_UI.h — the manual markers editor: the hand-authored roster a designer edits
// directly, unlike the read-only PLACED list the Placement stage resolves procedurally
// (MarkersTab_Placed_UI.h). Layer: UI. Accuracy class: Visual. STEP49 (`BRIEF_MarkersTabUI.md`;
// `ENTITY_AUTHORING_PARAMS_SPEC.md`'s third session ratified the real `Params::
// MarkerInstanceGroup`/`MarkerTransform` types this now edits directly).
//
// Shape mirrors AreasTab_UI.h/.cpp — a small, name-keyed vector, DraggableList select->edit — not
// PropsTab_Manual_UI.h, whose list previews a read-only procedural buffer. `recipe.markers` is
// TWO levels (group -> its own instance roster), so this adds a nested DraggableList scoped to
// whichever group is selected. `MakeNamesUnique` runs on `recipe.markers` (group names, global)
// and separately on the SELECTED group's `transforms` (instance names, scoped PER-GROUP — the
// wire format's inner dictionary only needs uniqueness within its own group). Split across two
// `.cpp` files behind this one header (ARCH §1.5): group-level draw in MarkersTab_Manual_UI.cpp,
// instance-level draw in MarkersTab_ManualInstance_UI.cpp.
//
// SCOPE (ARCH §8.4 — reported, not invented): no `layerIndex`/manual layers or per-marker
// symmetry (`GAP_MarkerLayerAndSymmetry_PARAMS.md`, Gaps 1/2); no rotation/scale editing or
// terrain-height snapping; never notifies `Pipeline::PreviewDriver` — `recipe.markers` feeds no
// PROC stage today, same silent posture `PropsTab_Manual_UI.h` SCOPE NOTE 1 already documents.
#pragma once
#include <string>
#include <vector>
#include "MarkersTab_Rules_UI.h"     // markerCategoryLabels / kMarkerCategoryCount — reused, not duplicated
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/Army_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// The fixed group name SANMAP_FORMAT_SPEC reserves for the commander-spawn roster: its inner
// dictionary is keyed by ARMY name, not a free-form instance name (confirmed live in-game,
// `session_findings_2026-08-17_unit_spawning.md`).
inline constexpr const char* kSpawnMarkerGroupName = "Spawn";

struct ManualMarkersState {
    SectionState   section;
    TextInputRules nameRules;             // shared: group name field, non-Spawn instance name field
    int selectedGroupIndex    = -1;
    int selectedInstanceIndex = -1;
    int addGroupCategoryIndex = 0;        // UI-only "Add Marker Type" picker mirror, no PARAMS home

    // X/Z: set by the caller each frame from `recipe.geometry.mapSize` via
    // `MarkerPositionHorizontalSliderRange` — `DrawManualMarkers` takes no map-size parameter.
    ScalarSliderRange positionHorizontalRange{ -256.0f, 512.0f, 0.0f };
    // Y: no existing world-elevation-range constant exists under `src/params/` — a placeholder
    // Constitution §8 setting, generous enough to clear the default `Geometry::terrainMaxHeight`
    // (128) with headroom, not a literal at the slider call site.
    ScalarSliderRange positionElevationRange{ -64.0f, 512.0f, 0.0f };

    RealtimeToggle positionXToggle;
    RealtimeToggle positionYToggle;
    RealtimeToggle positionZToggle;
};

// ---- pure helpers (headless-testable, no imgui) -----------------------------------------------

inline Params::MarkerInstanceGroup* SelectedMarkerGroup(std::vector<Params::MarkerInstanceGroup>& markers,
                                                         int selectedGroupIndex) {
    if (selectedGroupIndex < 0 || selectedGroupIndex >= static_cast<int>(markers.size())) return nullptr;
    return &markers[static_cast<std::size_t>(selectedGroupIndex)];
}

inline int ResolvedMarkerGroupSelection(int selectedGroupIndex, int groupCount) {
    if (groupCount <= 0 || selectedGroupIndex < 0) return -1;
    return selectedGroupIndex < groupCount ? selectedGroupIndex : groupCount - 1;
}

inline Params::MarkerTransform* SelectedMarkerInstance(std::vector<Params::MarkerTransform>& transforms,
                                                        int selectedInstanceIndex) {
    if (selectedInstanceIndex < 0 || selectedInstanceIndex >= static_cast<int>(transforms.size())) return nullptr;
    return &transforms[static_cast<std::size_t>(selectedInstanceIndex)];
}

// Same shape as `ResolvedMarkerGroupSelection`, scoped one level down (a group's own `transforms`).
inline int ResolvedMarkerInstanceSelection(int selectedInstanceIndex, int instanceCount) {
    if (instanceCount <= 0 || selectedInstanceIndex < 0) return -1;
    return selectedInstanceIndex < instanceCount ? selectedInstanceIndex : instanceCount - 1;
}

// Row labels — never empty (Constitution §6).
inline const char* MarkerGroupRowLabel(const Params::MarkerInstanceGroup& group) {
    return group.name.empty() ? "Marker Type" : group.name.c_str();
}
inline const char* MarkerInstanceRowLabel(const Params::MarkerTransform& transform) {
    if (!transform.alias.empty()) return transform.alias.c_str();
    return transform.name.empty() ? "Marker" : transform.name.c_str();
}

// True when `group` is the reserved commander-spawn roster, whose Name field is an army picker.
inline bool IsSpawnMarkerGroup(const Params::MarkerInstanceGroup& group) {
    return group.name == kSpawnMarkerGroupName;
}

// Seed name for a fresh (non-Spawn) instance, before the per-group uniqueness repair runs.
inline std::string NextMarkerInstanceName(int existingCount) { return NextUniqueLabel("Marker", existingCount); }

// X/Z bounds: `AreasTab_UI.h::AreaOriginSliderRange`'s "one map-width slack each side" reasoning,
// but CONTINUOUS — a marker's position is not fenced to a whole heightfield cell like an area is.
inline ScalarSliderRange MarkerPositionHorizontalSliderRange(int mapSize) {
    const int resolvedMapSize = mapSize > 1 ? mapSize : 1;
    ScalarSliderRange range;
    range.minimumValue = static_cast<float>(-resolvedMapSize);
    range.maximumValue = static_cast<float>(resolvedMapSize * 2);
    range.increment     = 0.0f;
    return range;
}

// The Spawn group's army-roster row for `instanceName`: the army whose `.name` matches it, or -1
// for a stale pick (renamed/removed army) — the combo then shows "<none>", never a wrong row (§6).
inline int ResolvedSpawnMarkerArmyPickIndex(const std::vector<Params::Army>& armies,
                                            const std::string& instanceName) {
    for (std::size_t armyIndex = 0u; armyIndex < armies.size(); ++armyIndex)
        if (armies[armyIndex].name == instanceName) return static_cast<int>(armyIndex);
    return -1;
}

// One army-picker row's label: its alias when it has one (friendlier than the export name), else
// the real name — never empty.
inline const char* ArmyPickerRowLabel(const Params::Army& army) {
    if (!army.alias.empty()) return army.alias.c_str();
    return army.name.empty() ? "Army" : army.name.c_str();
}

// MarkersTab_Manual_UI.cpp — the group stack: Add Marker Type, the group list, the selected
// group's own Name/Resource fields. Returns the selected group, or null.
Params::MarkerInstanceGroup* DrawMarkerGroupSection(std::vector<Params::MarkerInstanceGroup>& markers,
                                                     ManualMarkersState& state);

// MarkersTab_ManualInstance_UI.cpp — the selected group's own instance roster: the list, Add/
// Remove, and the selected instance's Alias/Name/Position editor.
void DrawMarkerInstanceSection(Params::MarkerInstanceGroup& group,
                               const std::vector<Params::Army>& armies, ManualMarkersState& state);

// `markers` is `recipe.markers`, `armies` is `recipe.armies` (read-only here — this block only
// picks an army for a Spawn instance's `name`, never adds/renames one). Never touches
// `Pipeline::PreviewDriver` (SCOPE NOTE 3 above).
void DrawManualMarkers(std::vector<Params::MarkerInstanceGroup>& markers,
                       const std::vector<Params::Army>& armies, ManualMarkersState& state);

} // namespace Ui
} // namespace SanmapGen

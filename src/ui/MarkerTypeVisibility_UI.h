// MarkerTypeVisibility_UI.h — STEP133: the Markers tab's per-Type Hide/Unhide preview filter.
// Layer: UI. Session-only, never serialized, never a PARAMS field — mirrors OverlayLayerSettings's
// own established posture (OverlayLayer_Settings_UI.h's own header comment: "Never serialized into
// `mapGeneratorData`"). Owned by MarkersTabState (MarkersTab_UI.h), NOT OverlayLayerSettings — see
// this ticket's own "Ground truth" section for why extending OverlayLayerSettings directly is the
// wrong precedent here (a 2-way Alloy/SpawnsArmies split with no Plasma row).
//
// `revision` mirrors OverlayLayerSettings::layerSettingsRevision/BumpLayerSettingsRevision's own
// shape exactly, so the C2 icon-layer render cache can combine it into its own invalidation key
// (MapCanvas_IconLayer_Draw_UI.cpp) without this struct or MarkersTab_UI.cpp ever writing directly
// into OverlayLayerSettings — that would be a layering violation (Application-shell-owned session
// state mutated from a different tab's code).
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

namespace SanmapGen {
namespace Ui {

struct MarkerTypeVisibility_UI {
    std::unordered_map<std::string, bool> hiddenByTypeName;   // absent/false = visible (default)
    std::uint64_t revision = 0;

    void SetHidden(const std::string& typeName, bool bHidden) {
        hiddenByTypeName[typeName] = bHidden;
        ++revision;
    }
    bool IsHidden(const std::string& typeName) const {
        const auto it = hiddenByTypeName.find(typeName);
        return it != hiddenByTypeName.end() && it->second;
    }
};

} // namespace Ui
} // namespace SanmapGen

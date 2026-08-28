// GlobalScatterSettings_PARAMS.h — Params::GlobalPropSettings/GlobalDecalSettings: map-wide default
// colors for the Props/Decals Type Sections, mirroring GlobalMarkerSettings (ARCH §11) at the scope
// ARCH §20 ratifies for Props/Decals. Layer: PARAMS. Settings only.
//
// Deliberately NOT a field-for-field mirror of GlobalMarkerSettings: no `iconName*` fields (Props/
// Decals already resolve real icons from `blueprintPath` — a default-icon-name field would be dead
// data), and no `selectColor*` fields yet (no selection-highlight consumer exists for Props/Decals
// today; add them if/when one does, mirroring ARCH §19.17's own precedent for when Markers grew
// theirs). `GlobalDecalSettings` carries exactly one color and needs no name-matching resolver at
// all — Decals has exactly one Type Section (ScatterInstanceLayer_PARAMS.h's header note).
#pragma once
#include <string>

namespace SanmapGen {
namespace Params {

struct GlobalPropSettings {
    float colorProp[4]    = {0.6f, 0.6f, 0.6f, 1.0f};
    float colorReclaim[4] = {0.2f, 0.8f, 0.3f, 1.0f};
};

struct GlobalDecalSettings {
    float colorDecal[4] = {0.6f, 0.4f, 0.2f, 1.0f};
};

// The propTypeName -> GlobalPropSettings-field mapping a manual prop resolves a TYPE-default color
// through, once "no explicit per-layer override" is established by the caller
// (PropInstanceLayer::bColorOverrideEnabled). Mirrors ResolveMarkerGroupTypeTintColor's own
// vocabulary shape (GlobalMarkerSettings_PARAMS.h). Any other/empty propTypeName (a layer authored
// before Type Sections existed) resolves to opaque white — the established "unset" convention.
inline void ResolvePropTypeTintColor(const std::string& propTypeName, const GlobalPropSettings& settings,
                                     float& outRed, float& outGreen, float& outBlue) {
    const float* color = nullptr;
    if (propTypeName == "Prop")         color = settings.colorProp;
    else if (propTypeName == "Reclaim") color = settings.colorReclaim;
    if (color == nullptr) { outRed = outGreen = outBlue = 1.0f; return; }
    outRed = color[0]; outGreen = color[1]; outBlue = color[2];
}

} // namespace Params
} // namespace SanmapGen

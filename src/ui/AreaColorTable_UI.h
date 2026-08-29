// AreaColorTable_UI.h — the UI-only per-area presentation color, and nothing else. Layer: UI.
// Extracted out of AreasTab_List_UI.h (ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9) so
// PreviewComposite_Settings_UI.h — the color table's new single owner
// (PreviewCompositeSettings::areaColors) — never has to depend on a TAB header
// (AreasTab_List_UI.h pulls ColorSwatch_UI.h, RtToggleWidget_UI.h, UniqueNameList_UI.h and
// MapArea_PARAMS.h, none of which the composite settings header needs). Depends on nothing but
// <string>/<vector>.
//
// COLOR HAS NO `_PARAMS` HOME (STEP21 ruling #4, restated by §14.17 item 13) — it is presentation
// state, kept by NAME (not vector position — position drifts under a Reorder for no reason color
// needs to care about, Constitution §6), and this is now its one home: the Areas tab, MapCanvas's
// own drag gesture, and the composite's own field-layer flattening all read/write this SAME table.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

enum : int { kAreaColorChannelCount = 4 };

// The default a freshly-resolved area color takes: Green, blend Overlay (ARCH §14.17 item 10) — RGB
// changed from Piece A's white-parity placeholder; the pre-existing 0.35 fill alpha is kept verbatim
// (v1 parity, `AreasTab_List_UI.h`'s own prior default).
inline constexpr float kDefaultAreaColor[kAreaColorChannelCount] = { 0.0f, 1.0f, 0.0f, 0.35f };

// A UI-only color, keyed by area NAME (STEP21 ruling #4).
struct AreaColorEntry {
    std::string name;
    float color[kAreaColorChannelCount] = { kDefaultAreaColor[0], kDefaultAreaColor[1],
                                            kDefaultAreaColor[2], kDefaultAreaColor[3] };
};

// Finds the color entry for `areaName`, or appends a fresh default-colored one on first touch — the
// same linear-scan idiom `NameIsTakenBefore` already uses. Returns the channel array directly so a
// caller can hand it straight to `DrawColorSwatch` or flatten it into a composite record.
inline float* ResolveAreaColor(std::vector<AreaColorEntry>& areaColors, const std::string& areaName) {
    for (AreaColorEntry& entry : areaColors)
        if (entry.name == areaName) return entry.color;
    AreaColorEntry entry;
    entry.name = areaName;
    areaColors.push_back(entry);
    return areaColors.back().color;
}

} // namespace Ui
} // namespace SanmapGen

// AreaColorTable_UI.h — the UI-only per-area presentation color, the 16-entry distinct-color
// palette new areas draw from, and the one pinned reserved name (`kPlayableAreaName`). Layer: UI.
// Extracted out of AreasTab_List_UI.h (ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9) so
// PreviewComposite_Settings_UI.h — the color table's new single owner
// (PreviewCompositeSettings::areaColors) — never has to depend on a TAB header
// (AreasTab_List_UI.h pulls ColorSwatch_UI.h, RtToggleWidget_UI.h, UniqueNameList_UI.h and
// MapArea_PARAMS.h, none of which the composite settings header needs). Depends on nothing but
// <string>/<vector>. ARCH_14_18_AreaLiveBlendFidelityAndPalette.md item 12 moves `kPlayableAreaName`
// down into this SAME file, for the identical reason (`ResolveAreaColor` needs to pin it without
// reaching up into a tab header), and preserves the no-new-#include rule exactly.
//
// COLOR HAS NO `_PARAMS` HOME (STEP21 ruling #4, restated by §14.17 item 13) — it is presentation
// state, kept by NAME (not vector position — position drifts under a Reorder for no reason color
// needs to care about, Constitution §6), and this is now its one home: the Areas tab, MapCanvas's
// own drag gesture, and the composite's own field-layer flattening all read/write this SAME table.
//
// ARCH §14.18 items 11-16 — new areas no longer default to a flat Green. `ResolveAreaColor`'s lazy
// append now assigns from the 16-entry `kAreaPaletteColors` below, by a fixed-stride cycle keyed on
// ordinal (the count of already-resolved non-PlayableArea entries). This is the ONE funnel every
// area reaches — the tab's swatch, the canvas, and `PreviewComposite::BuildMapAreaConfigurations`
// all call it — so created, imported, and migrated areas are all covered by this one
// implementation, never two (item 13's correction over assigning at the two creation call sites,
// which an imported `.sanmap` area never reaches).
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

enum : int { kAreaColorChannelCount = 4 };

// The one area the engine requires. Moved here from AreasTab_List_UI.h (ARCH §14.18 item 12) so
// ResolveAreaColor can pin its color without depending on a tab header. AreasTab_List_UI.h
// re-exports this name by inclusion, so `IsPlayableArea`/`EnsurePlayableArea`/every other existing
// call site keeps compiling with zero changes.
inline constexpr const char* kPlayableAreaName = "PlayableArea";

// ARCH §14.18 item 16 — the single flat `kDefaultAreaColor` no longer means one thing, so it is
// retired and split in two: the pinned reserved color, and the fill alpha every area (palette or
// Playable) still shares.
inline constexpr float kPlayableAreaColor[kAreaColorChannelCount] = { 0.0f, 1.0f, 0.0f, 0.35f };
inline constexpr float kDefaultAreaFillAlpha = 0.35f;

// ARCH §14.18 item 11 — 16 hand-authored entries, RGB only (the fill alpha is
// kDefaultAreaFillAlpha above, not per-entry). Every entry has at least one channel at exactly
// 0.00 and one at exactly 1.00: under Overlay (the MapAreas layer's default blend mode), a source
// channel of 0.5 is the blend IDENTITY (d <= 0.5 -> 2*d*0.5 = d; d > 0.5 -> 1-2(1-d)(1-0.5) = d) —
// a mid-gray area would be literally invisible, so no entry sits anywhere near gray. The ±30°
// neighbourhood of pure green (120°) is excluded and reserved for PlayableArea; the 16 hues run
// 153.75° -> 75.00° in +18.75° steps, spanning the remaining 300° of the wheel. Full saturation,
// full value. Spectrum order, for a human auditing the table — the ASSIGNMENT cycle below does
// NOT walk this in order (item 13).
inline constexpr int kAreaPaletteEntryCount = 16;
inline constexpr float kAreaPaletteColors[kAreaPaletteEntryCount][3] = {
    { 0.00f, 1.00f, 0.56f },   //  0 Spring Aqua   153.75
    { 0.00f, 1.00f, 0.88f },   //  1 Turquoise     172.50
    { 0.00f, 0.81f, 1.00f },   //  2 Cyan          191.25
    { 0.00f, 0.50f, 1.00f },   //  3 Azure         210.00
    { 0.00f, 0.19f, 1.00f },   //  4 Blue          228.75
    { 0.13f, 0.00f, 1.00f },   //  5 Indigo        247.50
    { 0.44f, 0.00f, 1.00f },   //  6 Violet        266.25
    { 0.75f, 0.00f, 1.00f },   //  7 Purple        285.00
    { 1.00f, 0.00f, 0.94f },   //  8 Magenta       303.75
    { 1.00f, 0.00f, 0.63f },   //  9 Fuchsia       322.50
    { 1.00f, 0.00f, 0.31f },   // 10 Rose          341.25
    { 1.00f, 0.00f, 0.00f },   // 11 Red             0.00
    { 1.00f, 0.31f, 0.00f },   // 12 Vermilion      18.75
    { 1.00f, 0.63f, 0.00f },   // 13 Orange         37.50
    { 1.00f, 0.94f, 0.00f },   // 14 Yellow         56.25
    { 0.75f, 1.00f, 0.00f },   // 15 Lime           75.00
};

// ARCH §14.18 item 13 — gcd(7, 16) == 1, so the cycle visits all 16 entries before repeating, and
// consecutively CREATED areas land at least 112.5° apart (versus the 18.75° a naive in-order walk
// would give) — the whole reason the stride exists. 16 (not e.g. 12) is chosen specifically so the
// wrap below is a compile-time bitmask, never a runtime division (Constitution §3).
inline constexpr int kAreaPaletteStride = 7;

// A UI-only color, keyed by area NAME (STEP21 ruling #4). The member initializer is an INERT
// placeholder (ARCH §14.18 item 16) — never a palette entry and never Green — so a construction
// path that forgot to route through ResolveAreaColor is not silently plausible.
struct AreaColorEntry {
    std::string name;
    float color[kAreaColorChannelCount] = { 0.0f, 0.0f, 0.0f, kDefaultAreaFillAlpha };
};

// Finds the color entry for `areaName`, or appends a fresh one on first touch — the same
// linear-scan idiom `NameIsTakenBefore` already uses. This is the ONE funnel every area reaches
// (the tab's swatch, the canvas, and PreviewComposite::BuildMapAreaConfigurations), so the
// palette-assignment rule lives here, once, covering created, imported, and migrated areas alike
// (ARCH §14.18 item 13).
inline float* ResolveAreaColor(std::vector<AreaColorEntry>& areaColors, const std::string& areaName) {
    for (AreaColorEntry& entry : areaColors)
        if (entry.name == areaName) return entry.color;
    AreaColorEntry entry;
    entry.name = areaName;
    if (areaName == kPlayableAreaName) {
        // The pinned reserved area consumes NO ordinal — it cannot collide with any palette entry
        // by construction (item 11's ±30° exclusion), so it needs no collision check either.
        entry.color[0] = kPlayableAreaColor[0]; entry.color[1] = kPlayableAreaColor[1];
        entry.color[2] = kPlayableAreaColor[2]; entry.color[3] = kPlayableAreaColor[3];
    } else {
        // The ordinal is DERIVED, never stored: no counter, no serialization, no new state — the
        // table's own contents answer it. A deleted area leaves its entry behind, so the next area
        // gets a FRESH ordinal rather than reusing the dead color — the desirable direction. No
        // "scan for an unused color" pass: it buys nothing before area 17 and costs a rule.
        int ordinal = 0;
        for (const AreaColorEntry& existing : areaColors)
            if (existing.name != kPlayableAreaName) ++ordinal;
        const float* const paletteColor =
            kAreaPaletteColors[(ordinal * kAreaPaletteStride) & (kAreaPaletteEntryCount - 1)];
        entry.color[0] = paletteColor[0]; entry.color[1] = paletteColor[1];
        entry.color[2] = paletteColor[2]; entry.color[3] = kDefaultAreaFillAlpha;
    }
    areaColors.push_back(entry);
    return areaColors.back().color;
}

} // namespace Ui
} // namespace SanmapGen

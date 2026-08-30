// AreaVisibilityTable_UI.h — the UI-only per-area VISIBILITY, and nothing else. Layer: UI.
// Mirrors AreaColorTable_UI.h's ownership (NOT AreaLockTable_UI.h's): unlike lock, visibility has
// a real composite-side reader (PreviewComposite::BuildMapAreaConfigurations skips a hidden area's
// fill entirely), so its single owner is PreviewCompositeSettings::areaVisibility — the same
// category areaColors already occupies — never AreasTabState. STEP222.
//
// Every area defaults VISIBLE on first resolve, with no "just created vs. pre-existing"
// distinction (unlike AreaLockEntry's bDefaultLocked) — a freshly created area is exactly as
// visible as any other first-touch area, so there is no second creation-time override anywhere.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

struct AreaVisibilityEntry {
    std::string name;
    bool        bVisible = true;
};

// Finds the visibility entry for `areaName`, or appends one (default visible) on first touch —
// the same linear-scan-then-lazy-append idiom `ResolveAreaColor`/`ResolveAreaLocked` already use.
inline bool* ResolveAreaVisible(std::vector<AreaVisibilityEntry>& areaVisibility,
                                const std::string& areaName) {
    for (AreaVisibilityEntry& entry : areaVisibility)
        if (entry.name == areaName) return &entry.bVisible;
    AreaVisibilityEntry entry;
    entry.name = areaName;
    areaVisibility.push_back(entry);
    return &areaVisibility.back().bVisible;
}

} // namespace Ui
} // namespace SanmapGen

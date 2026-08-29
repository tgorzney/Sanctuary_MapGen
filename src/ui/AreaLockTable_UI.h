// AreaLockTable_UI.h — the UI-only per-area LOCK, and nothing else. Layer: UI.
// Mirrors AreaColorTable_UI.h's exact shape (STEP21 ruling #4's precedent extended to a second
// presentation-state concern, STEP212): a small side table keyed by MapArea::name, depending on
// nothing but <string>/<vector>. UNLIKE AreaColorTable_UI.h, this table has NO composite-side
// reader at all — lock never affects what the GPU composite draws (that is color/enabled-layer's
// job alone, ARCH_14_17_MapAreaFieldLayer.md), it only gates whether MapCanvas's own gesture code
// accepts a click/drag for a given area. Its single owner therefore stays `AreasTabState::areaLocks`
// (AreasTab_UI.h) — never `PreviewCompositeSettings` — with MapCanvas holding a plain pointer to
// that same vector (SetManualAreaDragSource), the identical ownership shape
// `AreasTabState::bAreasLocked` (the single global bool this table replaces) already used.
//
// Given its own minimal header regardless (rather than living inline in AreasTab_List_UI.h, which
// remains this domain's "pure lifecycle rules" file — PlayableArea, unique names, Set to Map Size —
// not a catch-all for every presentation-state side table) so `MapCanvas_ManualDragSources_UI.h` can
// depend on it exactly as minimally as it already depends on `AreaColorTable_UI.h`, without pulling
// in `AreasTab_List_UI.h`'s own heavier `ColorSwatch_UI.h`/`RtToggleWidget_UI.h`/
// `UniqueNameList_UI.h`/`MapArea_PARAMS.h` chain — the same footprint discipline that file's own
// existing include list already practices.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

// A UI-only per-area LOCK, keyed by area NAME — the exact same side-table pattern as
// `AreaColorEntry` (AreaColorTable_UI.h). Areas default LOCKED (matches the retired global
// `AreasTabState::bAreasLocked`'s own default), EXCEPT a freshly created one (via the tab's own
// "Add New Area" button or the canvas's own `CreateAreaFromDrag`), which is inserted UNLOCKED
// explicitly at creation time, before this table's own lazy default would otherwise apply.
struct AreaLockEntry {
    std::string name;
    bool        bLocked = true;
};

// Finds the lock entry for `areaName`, or appends one on first touch using `bDefaultLocked` — the
// same linear-scan-then-lazy-append idiom `ResolveAreaColor` already uses. `bDefaultLocked` is
// `true` for every ordinary lazy resolve (a pre-existing area encountered for the first time — an
// imported project, or the engine-required PlayableArea, which is never "just created" by the user
// in practice); the two creation call sites (AreasTab_UI.cpp's "Add New Area",
// MapCanvas_AreaDragDispatch_UI.cpp's CreateAreaFromDrag) pass `false` explicitly, inserting their
// own entry UNLOCKED before this resolver's own default would otherwise apply. Once an entry
// already exists, a LATER resolve's own `bDefaultLocked` argument is irrelevant — the existing
// value always wins, never silently re-defaulted.
inline bool* ResolveAreaLocked(std::vector<AreaLockEntry>& areaLocks, const std::string& areaName,
                               bool bDefaultLocked = true) {
    for (AreaLockEntry& entry : areaLocks)
        if (entry.name == areaName) return &entry.bLocked;
    AreaLockEntry entry;
    entry.name    = areaName;
    entry.bLocked = bDefaultLocked;
    areaLocks.push_back(entry);
    return &areaLocks.back().bLocked;
}

} // namespace Ui
} // namespace SanmapGen

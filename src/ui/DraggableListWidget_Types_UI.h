// DraggableListWidget_Types_UI.h — the wire types DraggableList<T> and its caller share: the
// signal it returns, the per-row description the caller supplies, and the row-layout choice
// (STEP200). Split out of DraggableListWidget_UI.h to keep every file under the ARCH §1.5 ceiling
// — the small header both DraggableListWidget_RowLayout_UI.h (the row drawing) and
// DraggableListWidget_UI.h (the structural-signal rule) build on, so neither of those depends on
// the other.
#pragma once

namespace SanmapGen {
namespace Ui {

enum class DraggableListSignalKind : int {
    None = 0, Reorder, Delete, ToggleVisibility, ToggleLock, Select, ExtraButton
};
// One frame produces at most ONE signal (first wins): every kind changes what the next frame draws.
struct DraggableListSignal {
    DraggableListSignalKind kind = DraggableListSignalKind::None;
    int sourceRowIndex = -1;   // the row the signal is about; the DRAGGED row for a reorder
    int targetRowIndex = -1;   // reorder only: the row it was dropped onto
    bool bHasSignal() const { return kind != DraggableListSignalKind::None; }
};
// What the caller says about one row. Strings are borrowed for the call, never retained.
struct DraggableListRow {
    const char* label    = "";
    bool        bVisible = true;
    bool        bLocked  = false;
    // OPTIONAL extra per-row affordance, right of the delete `X` — null (default) draws nothing,
    // so a consumer that never sets it (Props/Decals/Markers/the GeoLayer list) is unaffected.
    // STEP150's Bake/Unbake header button is the first user (a click reports `ExtraButton`); keep
    // the "##" id salt fixed across a changing label, same discipline as the icons below.
    const char* extraButtonLabel = nullptr;
};
// STEP200: `Collapsible` (default, unchanged) keeps every existing DraggableList consumer's
// CollapsingHeader row untouched. `Flat` is the View popup's opt-in single-line row: no
// disclosure arrow, the name IS the drag handle, the body is drawn inline every frame — there is
// no expand/collapse state to desync from a growing popup.
enum class DraggableListRowLayout { Collapsible, Flat };

// Affordance strip width — a NAMED constant, not the legacy bare `- 60`. Layout styling is a
// Constitution §8 tweakable that moves into the shared UI style settings with the tabs (M5-6).
enum : int { kAffordanceStripWidthPixels = 76 };
// STEP200: the Flat row's fixed name-column width — the row body (blend combo / opacity slider)
// and the affordance strip always start at the same offset, frame to frame, which is what keeps a
// Flat-mode popup from ever auto-growing.
enum : int { kFlatRowNameWidthPixels = 140 };

} // namespace Ui
} // namespace SanmapGen

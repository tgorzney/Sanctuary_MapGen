// MapCanvas_SelectionSet_UI.h — the ordered multi-select set behind the canvas's selection surface
// (ARCH §21.1). Layer: UI. Pure, imgui-free, testable with no window — same posture as
// InstanceDragGesture_UI.h. `OverlayInstanceKeySet_UI` is an ordered list, not a hash set: "primary"
// (the derived single-key reading every pre-existing procedural-only caller keeps reading) is
// defined as the LAST element — an MRU-stack invariant every mutator below maintains, never computed
// some other way — and authoring-scale sizing ("tens, not tens of thousands," the same posture
// `MapCanvas_MarkerHitTest_UI.cpp`'s own header comment already states for a linear manual-roster
// scan) makes a linear scan for containment/dedup entirely legitimate, no hash/tree needed.
#pragma once
#include <vector>
#include "MapCanvas_IconLayer_UI.h"   // OverlayInstanceKey_UI, OverlayInstanceKeysEqual

namespace SanmapGen {
namespace Ui {

struct OverlayInstanceKeySet_UI {
    std::vector<OverlayInstanceKey_UI> keys;
};

// The MRU-stack "primary" — the LAST element, or a default (invalid, bValid=false) key when empty.
const OverlayInstanceKey_UI& PrimaryOfSelectionSet(const OverlayInstanceKeySet_UI& set);

bool SelectionSetContains(const OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key);

// Element-wise, ORDER-sensitive equality — a reorder (e.g. an already-selected key becoming the new
// primary) counts as a change, matching `MapCanvas::SetSelection`'s own pre-§21.1 no-op-on-unchanged
// guard extended to the whole ordered set, not just a single key.
bool SelectionSetsEqual(const OverlayInstanceKeySet_UI& a, const OverlayInstanceKeySet_UI& b);

// Plain click / plain marquee: `keys` becomes the WHOLE set, in the given order — the primary is
// therefore whatever the caller's own list ends with (trivial for a single-key click; for a
// marquee, whatever order the region query enumerated its hits in — an incidental, deterministic
// tie-break, never a designer-meaningful choice).
void ReplaceSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys);

// Ctrl-click (single key only — never called with a marquee batch): present -> erase (primary
// becomes the new back(), or the set empties); absent -> append (becomes primary).
void ToggleInSelectionSet(OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key);

// Ctrl-marquee (batch counterpart to the single-key `ToggleInSelectionSet` above, never called with
// a single key): each key in `keys`, in turn, in order — present -> erase, absent -> append (becomes
// primary in turn, so the LAST key from `keys` that ends up present is the final primary; if the
// last key's own toggle erases it, the primary falls back to the set's new back(), same as
// `ToggleInSelectionSet`'s erase case). This is Toggle applied per element, NOT a hybrid of
// Toggle+Union. Duplicate-key caveat: if `keys` itself contains the same key more than once (a
// marquee region query is not expected to, but this function does not assume it and performs no
// de-duplication), each repeat re-toggles that key's presence in place — two repeats net to a
// no-op for that key, an odd count of repeats nets to the same single toggle a lone occurrence
// would. Callers passing an already-deduplicated `keys` never observe this; it is documented so a
// coder who feeds a raw/undeduplicated hit list is not surprised by it.
void ToggleEachInSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys);

// Shift-click / Shift-marquee: every key in `keys`, in order, not already present is appended
// (becomes primary in turn — the LAST newly-appended key is the final primary). An already-present
// key keeps its existing position, never re-touched/reordered by a union. If every key in `keys`
// was already present, the primary is unchanged.
void UnionIntoSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys);

} // namespace Ui
} // namespace SanmapGen

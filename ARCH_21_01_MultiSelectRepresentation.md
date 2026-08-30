[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.1. **Only the ARCH Expert writes this file.**

### 21.1 Multi-select representation — `OverlayInstanceKeySet_UI`, the widened `MapCanvas` selection surface

**Ratified as designed**, with the exact ordering/primary/callback contract resolved below — the
relayed design names the mechanism but not its precise semantics, resolved here rather than left
for a coder to invent (Constitution §6).

New file `MapCanvas_SelectionSet_UI.h`/`.cpp`:
```cpp
struct OverlayInstanceKeySet_UI { std::vector<OverlayInstanceKey_UI> keys; };

// "Primary" = the LAST element — an MRU-stack invariant every mutator below maintains, not a
// property computed some other way. Returns a default (invalid, bValid=false) key when empty.
const OverlayInstanceKey_UI& PrimaryOfSelectionSet(const OverlayInstanceKeySet_UI& set);
bool SelectionSetContains(const OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key);

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
```

`MapCanvas` replaces its single `OverlayInstanceKey_UI selectedInstanceKey` member with
`OverlayInstanceKeySet_UI selectedInstanceKeys`; `SelectedEntityIdentifier()`/`HasSelection()` stay
thin reads of `PrimaryOfSelectionSet(selectedInstanceKeys)` — computed on demand (authoring-scale
set, "tens, not tens of thousands," the same sizing posture `MapCanvas_MarkerHitTest_UI.cpp`'s own
header comment already states for a linear manual-roster scan; no caching needed).

`SetSelectionChangedCallback` widens:
```cpp
void SetSelectionChangedCallback(
    std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys)> callback);
```

**The canonical entry point widens from `SetSelection` to `ApplySelectionGesture`, two overloads**
(single-key — click, list-click; batch — marquee), both taking modifier state:
```cpp
void ApplySelectionGesture(const OverlayInstanceKey_UI& touchedKey, bool bCtrlHeld, bool bShiftHeld);
void ApplySelectionGesture(const std::vector<OverlayInstanceKey_UI>& touchedKeys, bool bCtrlHeld, bool bShiftHeld);
```
Each overload resolves its own modifier state to exactly one mutator: neither held -> Replace
(`ReplaceSelectionSet`); Shift held (Ctrl not) -> Union (`UnionIntoSelectionSet`, the single-key
overload passes its one key as a one-element list); Ctrl held -> Toggle —
`ToggleInSelectionSet` for the single-key overload, `ToggleEachInSelectionSet` for the batch
overload (Ctrl-marquee toggles each touched key exactly as Ctrl-click does for one, per
`ToggleEachInSelectionSet` above — it is NOT collapsed into Union). Ctrl wins if both are somehow
held: for the single-key overload this was always a live tie-break (Toggle and Union genuinely
differ); for the batch overload it is equally live now that `ToggleEachInSelectionSet` exists to
differ from Union — still an arbitrary but necessary tie-break, recorded so it is not left to a
coder's guess. Both overloads then update `selectedInstanceKeys`, and fire the widened callback with
`(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys)` — but ONLY when the set
actually changed (mirroring today's `SetSelection`'s own no-op-on-unchanged guard). Every existing
single-target call site (`ApplyClick`'s
procedural/manual branches, `SelectManualMarkerByInstanceIdentifier`,
`SelectProceduralMarkerInstanceByArrayPosition`) calls the single-key overload with
`bCtrlHeld=false, bShiftHeld=false` — an unconditional Replace, byte-identical to today's behavior.
Whether `ApplyClick` itself grows two new parameters or a new wrapper method carries the
modifier-aware entry point is left to the coder work-order — not re-litigated here. Threading real
Ctrl/Shift state from a Markers-tab list click into this same path (converging with
`MarkersTab_ManualInstanceSelection_UI.h`'s own already-shipped, list-local multi-select, STEP141)
is a natural follow-up, explicitly NOT required by this ratification — that file's own header
comment already scopes "multi-instance canvas highlighting" out as "a separate, larger change,"
which this section is.

**`Application::WireCallbacks()`'s closure generalizes by partitioning the full set, not just
reading the primary.** For Markers: filter `selectedKeys` to `{collection == Markers, bManual ==
true}`, in order, into `tabState.markers.selectedManualInstanceIdentifiers` (the ALREADY-EXISTING
plural field, `Application_UI.cpp:90,102` — this ruling changes ITS SOURCE from the single-element
list `{key.instanceIndex}` to the real multi-select set, nothing else); set
`tabState.markers.selectedManualInstanceIdentifier` to the primary's `instanceIndex` if the primary
itself is `{Markers, bManual=true}`, else `-1` (mirrors today's single-key logic, re-derived from
the set's primary instead of from the lone callback argument). The procedural-Markers case
(`lastSelectedEntityIdentifier`) stays primary-only — there is no existing plural field for it to
widen into (the same "no such tab-level plural field yet" posture the design states for Props/
Decals, applying equally here). Props/Decals: no-op, unchanged, per the design's own explicit
forward-compatible-no-op call.

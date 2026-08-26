[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.22. **Only the ARCH Expert writes this file.**

### 19.22 File-size ceiling remediation — `MarkersTab_ManualLayers_UI.h` split, FINAL combined plan

**Supersedes this section's own earlier text (2026-08-26, same day).** The earlier ruling below
this line's original version split ONE fault line (the RowBody pair) and stated Ticket B would add
no further declarations to this header, so no further split was needed. That premise did not hold:
`work_orders/STEP125_MarkersTabTypeSections_UI.md` (Ticket B's actual draft, written without
knowledge of this section — a process gap, not a drafting error) both (a) confirmed the RowBody
split was ruled but never implemented — the live file is still 165 lines, unremediated — and (b)
independently found its OWN new content adds three genuinely new declarations directly to this
header (`DrawLayerList` promoted, `DrawManualMarkerLayerBlockSettings` renamed/promoted from
`DrawLayerSettings`, `DrawManualMarkerLayerListBody` new), plus its own required second split (five
pure helpers relocated to a new `MarkersTab_ManualLayerHelpers_UI.h`, STEP125 §6) to stay under the
150-line hard ceiling given that new content. **Ruling: both splits happen, additively, as ONE
remediation — not a choice between them.** They cut along two different, non-overlapping fault
lines (row-body rendering reused by the Bundle tree vs. pure standalone predicate/utility helpers)
and neither alone gets the post-Ticket-B header under ceiling; together they leave it comfortably
under (see the arithmetic below). This is now the ONE authoritative, buildable shape — a coder
implements all three files below in the same pass, no further judgment calls.

**Confirmed by direct read (2026-08-26, this session) before writing this revision:** the live
`src/ui/MarkersTab_ManualLayers_UI.h` is 165 lines, still carrying `DrawLayerRowBody`'s and
`DrawManualMarkerLayerColorOverrideHeaderControl`'s declarations plus their two paired width
constants (the RowBody split below was never built); `MarkersTab_ManualLayerRowBody_UI.cpp` already
exists and already implements both functions, exactly as this section's original ruling described,
still without its own paired `.h`.

---

#### File 1 (new): `src/ui/MarkersTab_ManualLayerRowBody_UI.h` — the RowBody split, unchanged from this section's original ruling

Moves out of `MarkersTab_ManualLayers_UI.h`, verbatim (declarations + their existing doc comments):
- `bool DrawLayerRowBody(Params::MarkerInstanceLayer&, int, const std::vector<Params::MarkerInstanceLayer>&, std::vector<Params::MarkerInstanceGroup>&, const Params::Geometry&, int, int, Params::MarkerSymmetryFixSettings&, ManualMarkerLayersState&);`
- `void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer&, ManualMarkerLayersState&, bool&);`
- `kMarkerLayerColorOverrideHeaderWidthPixels` / `kMarkerLayerColorOverrideSwatchWidthPixels`.

`#include`s `MarkersTab_ManualLayers_UI.h` (needs `ManualMarkerLayersState`) plus the PARAMS headers
those two signatures need. Consumer `#include` updates (mechanical, no transitive-re-export reliance
— unchanged from the original ruling):
- `MarkersTab_ManualLayerRowBody_UI.cpp` — its own first `#include` becomes this new header.
- `MarkersTab_ManualLayers_UI.cpp` — add this include (its own `DrawLayerList` calls both functions
  AND uses `kMarkerLayerColorOverrideHeaderWidthPixels` directly — confirmed by direct read of the
  live `.cpp`, line 56).
- `MarkersTab_Bundles_UI.cpp` — add this include (calls `DrawLayerRowBody` as the tree's Manual
  leaf-body callback; confirmed by direct read, its current sole include is the parent header).
- `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp` — add this include (confirmed by direct
  read, its current sole include is the parent header).
- No other file calls either function directly (confirmed by grep).

#### File 2 (new): `src/ui/MarkersTab_ManualLayerHelpers_UI.h` — the Helpers split, per STEP125 §6, adopted as-is

Moves out of `MarkersTab_ManualLayers_UI.h`, verbatim (declarations + their existing doc comments),
**exactly the five functions STEP125 §6 names by name — not the "lines 78-124" range its own text
also cites, which loosely oversweeps into `SelectedManualMarkerLayer` (see the explicit carve-out
below)**:
- `IsMarkerInstanceLayerLocked`
- `QuantizeMarkerPositionToLayerGrid`
- `EffectiveManualMarkerLayerColor`
- `ManualMarkerLayerRowLabel`
- `NextMarkerLayerName`

Plus STEP125's own new predicate, placed beside its nearest neighbor exactly as STEP125 §3
specifies:
- `IsMarkerInstanceLayerRowSuppressed` (new, beside `IsMarkerInstanceLayerLocked`)

**Explicit carve-out, resolving STEP125's own imprecise line-range citation: `SelectedManualMarkerLayer`
stays in the parent header, untouched by either split.** It is not one of STEP125's five NAMED
helpers (its own bullet list is authoritative over its own looser prose line-range aside). Confirmed
by grep: it is declared once, in the header, and has zero call sites anywhere in `src/` today — dead
code, out of scope for this remediation to either relocate or remove. Whoever eventually touches it
should flag that separately; this ticket does not silently delete or move unreferenced code it wasn't
asked to.

`#include`s `<cmath>` (for `QuantizeMarkerPositionToLayerGrid`'s `std::round`), `UniqueNameList_UI.h`
(for `NextMarkerLayerName`'s `NextUniqueLabel`), and `MarkersTab_ManualLayers_UI.h` (needs
`ManualMarkerLayersState`, used by `EffectiveManualMarkerLayerColor`) — the same "new header includes
the parent for the state type" pattern File 1 uses. Consumer `#include` updates:
- `MarkersTab_ManualLayers_UI.cpp` — add this include (defines `NextMarkerLayerName`'s call site in
  `DrawLayerListButtons`, `ManualMarkerLayerRowLabel` in `DrawLayerList`, and now
  `IsMarkerInstanceLayerRowSuppressed` per STEP125 §3's `DrawLayerList` row-suppression change).
- `MarkersTab_ManualLayers_UI_Test.cpp` — add this include (exercises `IsMarkerInstanceLayerLocked`/
  `QuantizeMarkerPositionToLayerGrid` by name today; STEP125 extends it to also exercise
  `IsMarkerInstanceLayerRowSuppressed` and `DrawLayerListButtons`'s new parameter — already correctly
  flagged in STEP125's own Verify section).
- Any other file calling one of the five by name: none found by grep beyond the two above.

#### File 3 (survives, shrunk): `src/ui/MarkersTab_ManualLayers_UI.h` — final shape

Keeps, unchanged: the header comment (updated to name all three companion files — RowBody, Helpers,
and the fact that Ticket B's Type-section outer loop lives one level up in
`MarkersTab_TypeSections_UI.h`), the `ManualMarkerLayersState` struct in full, and
`SelectedManualMarkerLayer` (see carve-out above).

Drops: the two RowBody declarations + two width constants (→ File 1); the five Helpers functions
(→ File 2); the retired `DrawManualMarkerLayers` declaration (STEP125 §5(a) retires the function — its
three jobs split into `DrawManualMarkerLayerBlockSettings` / `DrawManualMarkerLayerListBody` / the
Type-section's own `DrawSectionBegin` wrap).

Includes drop `<cmath>` and `UniqueNameList_UI.h` (both only served the now-relocated Helpers
functions); **gains `"DraggableListWidget_UI.h"`** (new requirement — `DrawLayerList`'s promoted
declaration returns `DraggableListSignal`, a type this header previously never needed to name because
the function was anonymous-namespace-local; confirmed this is the same include
`MarkersTab_RuleLayers_UI.h` already uses for the identical reason).

Gains, per STEP125 §3/§4/§5(a), the ticket's own new/changed entry-point declarations (bodies defined
in `MarkersTab_ManualLayers_UI.cpp`, exactly as STEP125 already specifies — this ruling changes
nothing about STEP125's own signatures, only where they land):
```cpp
DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted,
                                  const std::string& markerTypeNameFilter);

void DrawManualMarkerLayerBlockSettings(ManualMarkerLayersState& state);   // renamed/promoted from DrawLayerSettings

void DrawManualMarkerLayerListBody(ManualMarkerLayersState& state, std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                                   int globalSymmetryMask, int globalRadialRepeatCount,
                                   Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                   const std::string& markerTypeNameFilter);

bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer = -1,
                          const std::string& markerTypeNameForNewLayer = "");   // gains 2nd param, STEP125 §4
```

**Resulting size — the arithmetic that resolves the conflict.** Struct (~35 lines) +
`SelectedManualMarkerLayer` (~9 lines incl. comment) + header comment/includes/namespace boilerplate
(~35 lines) + the four entry-point declarations with their doc comments (~30 lines) + closing braces
(~2 lines) lands at roughly **110-115 lines** — comfortably under the 150-line hard ceiling, with
headroom this file has not had since before STEP123. (STEP125 §6's own estimate of "145-150" was
computed WITHOUT the RowBody split — i.e., assuming the two RowBody declarations and two width
constants stayed in this header, unaware §19.22 already ruled them out. Doing both splits together,
per this ruling, is what actually clears the ceiling with margin, not either split alone.) Still over
the 100-line soft target — an already-accepted, pre-existing carry, not a new exception opened here.

---

**Note for the human editing `work_orders/STEP125_MarkersTabTypeSections_UI.md` to match this
ruling** (the ARCH Expert does not edit work-order files): its §6 currently describes ONLY the
Helpers split (unaware the RowBody split was separately ratified and still pending), and its own
"lines 78-124" phrasing loosely oversweeps `SelectedManualMarkerLayer` even though its bullet list of
five names does not include it. Its "Files touched" list names `MarkersTab_ManualLayerHelpers_UI.h`
correctly but does not list `MarkersTab_ManualLayerRowBody_UI.h` (needs to be added as a NEW file
this ticket also delivers, or sequenced as a prerequisite landed just ahead of it — coder's choice,
since the two splits are independent of each other) nor does it flag the RowBody split's own required
consumer-include updates (`MarkersTab_ManualLayers_UI.cpp`, `MarkersTab_Bundles_UI.cpp`,
`MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`) as part of this ticket's own diff.

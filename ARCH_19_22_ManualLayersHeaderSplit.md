[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.22. **Only the ARCH Expert writes this file.**

### 19.22 File-size ceiling remediation — `MarkersTab_ManualLayers_UI.h` split, resolved ahead of Ticket B
STEP123 left `src/ui/MarkersTab_ManualLayers_UI.h` at 165 lines, over
`ARCH_01_05_FileSizeCeilings.md` §1.5's 150-line hard ceiling (it was already at the ceiling —
exactly 150 lines — before STEP123's own ticket explicitly directed new declarations into this
file; confirmed by direct read of the current 165-line file). Since the Type-Sections work's Ticket
B is about to add further content to this file's own UI surface, this is resolved now, ahead of
Ticket B's diff, not deferred into it.

**Ruled: split along the file's own already-established fault line, mirroring
`MarkerLayerIndexRepair_UI.h`'s precedent** (this same file's own header comment, line 10: "the
repair functions themselves live in the sibling header `MarkerLayerIndexRepair_UI.h`... this file's
own divergence from the Props precedent"). Confirmed by direct read: `DrawLayerRowBody` and
`DrawManualMarkerLayerColorOverrideHeaderControl` are ALREADY implemented in their own translation
unit, `MarkersTab_ManualLayerRowBody_UI.cpp` (split out specifically so `MarkersTab_Bundles_UI.cpp`
can reuse `DrawLayerRowBody` unchanged as a `TreeListWidget_UI` leaf-body callback, per that file's
own header comment) — but their DECLARATIONS still live in the parent header instead of a paired
header of their own. The fix: give that `.cpp` the sibling `.h` it should already have had.

**New file: `src/ui/MarkersTab_ManualLayerRowBody_UI.h`.** Moves out of
`MarkersTab_ManualLayers_UI.h`, verbatim (declarations + their existing doc comments):
- `bool DrawLayerRowBody(Params::MarkerInstanceLayer&, int, const std::vector<Params::MarkerInstanceLayer>&, std::vector<Params::MarkerInstanceGroup>&, const Params::Geometry&, int, int, Params::MarkerSymmetryFixSettings&, ManualMarkerLayersState&);`
- `void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer&, ManualMarkerLayersState&, bool&);`
- `kMarkerLayerColorOverrideHeaderWidthPixels` / `kMarkerLayerColorOverrideSwatchWidthPixels`
  (STEP123's own paired reserved-width constants for these two functions — moved together with
  them, not split further, since they parameterize nothing else).

The new header `#include`s `MarkersTab_ManualLayers_UI.h` (needs `ManualMarkerLayersState`, defined
there) plus whatever PARAMS headers those signatures need — the same dependency direction
`MarkersTab_ManualLayerRowBody_UI.cpp` already has on the parent header today, moved up one level
from `.cpp` to `.h`.

**Consumer includes updated, mechanically, at every real call site — not left to silent transitive
include luck:**
- `MarkersTab_ManualLayerRowBody_UI.cpp` — its own first `#include` becomes
  `MarkersTab_ManualLayerRowBody_UI.h` (which itself includes the parent), matching the "a `.cpp`
  includes its own paired `.h` first" convention this codebase already uses everywhere else.
- `MarkersTab_ManualLayers_UI.cpp` — add `#include "MarkersTab_ManualLayerRowBody_UI.h"` (calls both
  functions inside `DrawLayerList`/`DrawManualMarkerLayers`).
- `MarkersTab_Bundles_UI.cpp` — add `#include "MarkersTab_ManualLayerRowBody_UI.h"` (calls
  `DrawLayerRowBody` as the tree's Manual leaf-body callback).
- `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp` — add the same include (exercises
  `DrawManualMarkerLayerColorOverrideHeaderControl` by name).
- No other file calls either function directly (confirmed by grep — the two other hits,
  `MarkersTab_Bundles_UI.h` and `MarkerLayerSymmetrySection_UI.h`, are comment-only mentions, not
  call sites).

**Why not have the parent header re-include the new one for free transitive visibility instead —
rejected.** Every real consumer above already includes `MarkersTab_ManualLayers_UI.h` (directly, or
for `MarkersTab_ManualLayerRowBody_UI.cpp`, will include the new header which itself includes the
parent) for OTHER reasons already (the state struct, the accessor helpers). Adding one explicit
include per consumer costs four one-line diffs and produces a codebase where every file's include
list states what it actually calls, rather than relying on an unstated transitive re-export —
matches this pack's "no layer knows more than it's declared to" spirit (§3.2) applied at the
file-include grain.

**Resulting size.** Removing the two declarations, their doc comments, and the two constants brings
`MarkersTab_ManualLayers_UI.h` to roughly 143 lines — under the 150-line hard ceiling, though still
over the 100-line soft target (already true before this remediation; no new soft-ceiling exception is
opened here, the file was already carrying one). Ticket B's own new content (the Type-section outer
loop, the filtered `TreeListWidget_UI` instantiation, the per-type "Add Group"/"Add Layer" wiring)
lives in `MarkersTab_Bundles_UI.cpp`, `MarkersTab_RuleLayers_UI.cpp`, `MarkersTab_ManualLayers_UI.cpp`,
and a new `MarkersTab_TypeSections_UI.h/.cpp` per the design doc's own delivery split — none of it
adds new declarations to `MarkersTab_ManualLayers_UI.h` itself, so no further split is needed before
Ticket B is drafted.

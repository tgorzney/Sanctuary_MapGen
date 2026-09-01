# STEP248 — Relocate Links Section, hierarchical per-type instance body, shared selection sync

**Layer:** UI. **Domain:** `src/ui/MarkersTab_UI.cpp`, `src/ui/MarkersTab_Links_UI.h/.cpp`,
`src/ui/MarkersTab_LinksHeaderExtras_UI.cpp`, `src/ui/MarkersTab_ManualInstanceSelection_UI.h/.cpp`.
**Sequence:** depends on STEP247 (the corrected `ApplyAddLinkAction`/`DeleteMarkerLink` signatures).

Ratifies `work_orders/DESIGN_MarkerLinkCorrection_R1.md` §3-5. Direct human ruling
(`BRIEF_MarkerLinkCorrection_R1.md`): the Links Section moves to right after the Global section; its
body shows a plain label per represented Marker Type, then that type's Link-tagged instances (grouped
by symmetry group, same convention the base Instances list already uses); clicking an instance row
here uses the SAME full Ctrl/Shift/drag selection as every other instance list in this tab, so
selecting in one place highlights in every other place for free (same shared state, same callback).

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. `MarkersTab_ManualInstanceSelection_UI.h`/`.cpp` — add:
   ```cpp
   // "Every instanceIdentifier currently tagged to Link X, grouped by canonical type name" — sibling
   // of PartitionSelectedManualInstancesByType, keyed by transform.linkIdentifier instead of
   // membership in a live selection set. Returns (groupIndex, transformIndex) pairs, matching
   // DrawBaseSectionManualInstanceList's own item type.
   std::unordered_map<std::string, std::vector<std::pair<int, int>>> PartitionLinkedManualInstancesByType(
           const std::vector<Params::MarkerInstanceGroup>& markers, int linkIdentifier);
   ```
2. `MarkersTab_Links_UI.h` — widen `DrawMarkerLinksSection`'s signature:
   ```cpp
   void DrawMarkerLinksSection(Params::MapRecipe& recipe, MarkerLinksState_UI& state,
                               int& selectedManualInstanceIdentifier,
                               std::vector<int>& selectedManualInstanceIdentifiers,
                               int& manualInstanceSelectionAnchorIdentifier,
                               const std::function<void(int, const std::vector<int>&)>&
                                   selectManualMarkerInstanceCallback,
                               Pipeline::PreviewDriver* previewDriver);
   ```
   Needs `#include "PlacementRuleSections_UI.h"` (declares `NotifyPlacementChange`) and whatever
   forward-declare/include `Pipeline::PreviewDriver*` requires (mirror `MarkersTab_UI.h`'s own).
3. `MarkersTab_LinksHeaderExtras_UI.cpp` — replace `DrawMarkerLinkSummaryBody` with `DrawMarkerLinkBody`:
   per represented Marker Type (from step 1's partition function), a plain `ImGui::TextUnformatted`
   label — NOT a nested `Section` widget, no separate collapse/settings state — followed by that
   type's instances fed through the SAME `rowOrder`/`ManualInstanceRowInteractionContext_UI`/
   `DrawSymmetryClusterInstanceList`/`DrawManualInstanceRow` block `DrawBaseSectionManualInstanceList`
   already uses (`MarkersTab_UI.cpp`, the base "Instances" list) — copy that block's shape verbatim,
   swapping only the item source. If `byType` is empty, show `"(no instances)"` disabled text.
4. `MarkersTab_Links_UI.cpp`, `DrawMarkerLinksSection` — swap the body call from
   `DrawMarkerLinkSummaryBody` to `DrawMarkerLinkBody` with the new selection params; accumulate
   `bAnyCommitted` across the whole per-Link loop into a single `bAnyLinkCommitted`, and call
   `NotifyPlacementChange(bAnyLinkCommitted, previewDriver)` once after the loop finishes (currently
   `bAnyCommitted` is computed and dropped — a pre-existing, independent defect, fix it here since
   it's the same call site). Update `DeleteMarkerLink`'s call to pass `recipe.markers` (STEP247's new
   parameter).
5. `MarkersTab_UI.cpp`, `DrawMarkersTab` — move the `DrawMarkerLinksSection(...)` call from its
   current position (last statement in the function) to immediately after
   `DrawMarkersTabGlobals(state.globals);`, passing the widened parameter set — all of
   `state.selectedManualInstanceIdentifier`/`selectedManualInstanceIdentifiers`/
   `manualInstanceSelectionAnchorIdentifier`/`selectManualMarkerInstanceCallback`/`previewDriver` are
   already local/parameters in scope at that point.

**"+Link" button stays in each Type-section header, unchanged** — do not duplicate or move it into
the new Links Section (settled in `DESIGN_MarkerLinkCorrection_R1.md` §4: the button acts on
whatever's currently selected, which is overwhelmingly unlinked instances in a Type-section body; a
one-frame lag between minting a Link and seeing it appear in the now-earlier-drawn Links Section is
cosmetic and self-resolving, the same class every other immediate-mode click-then-see-next-frame case
in this tab already has).

## Verify

- Links Section renders immediately after the Global section, before any Marker-Type section, in the
  live tab.
- A Link's body shows one label per Marker Type actually represented among its tagged instances (no
  label for a type with zero tagged instances), each followed by that type's instances, symmetry-
  clustered the same way the base Instances list clusters them.
- Click an instance row inside a Link's body: it becomes selected/highlighted in the canvas AND in its
  own Marker-Type section's own instance row, without any new plumbing beyond what this ticket wires
  (same `selectedManualInstanceIdentifiers`/callback round-trip already used everywhere else). Ctrl-
  click and Shift-range-select both work identically to the base Instances list.
- Click an instance row inside its Marker-Type section: it becomes selected/highlighted inside the
  Links-Section row too, if that instance is Link-tagged.
- Toggling a Link's Hidden/Locked/Color/Grid/Symmetry header controls now trips `NotifyPlacementChange`
  (confirm via whatever mechanism other `NotifyPlacementChange` call sites are tested with — a dirty-
  flag/regenerate-triggered assertion, not a UI-only check).
- Extend `MarkersTab_Links_UI_Test.cpp`, add coverage for `PartitionLinkedManualInstancesByType`.
  Full `MarkersTab_UI_Test` suite stays green.

## Out of scope

- The shared cross-domain drag-gesture/hit-test/delete generic widening (STEP249) — instance rows
  drawn by this ticket support click/Ctrl/Shift selection today via the existing
  `DrawManualInstanceRow` plumbing (unaffected by STEP249), but do NOT depend on STEP249 landing first;
  drag-from-within-the-Links-Section-body is not a requirement of this ticket (instance rows in lists
  elsewhere in this tab are not draggable either — dragging is a canvas-only gesture).
- Any further resolver change beyond what STEP246 already landed.

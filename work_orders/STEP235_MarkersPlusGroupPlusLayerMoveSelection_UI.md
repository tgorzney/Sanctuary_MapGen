# STEP235 — "+ Group" / "+ Layer" move current same-type selection into the new container

**Layer:** UI. **Domain:** `MarkersTab_UI.cpp` (`bAddGroupClicked`/`bAddManualLayerClicked`
branches), `MarkersTab_ManualInstanceSelection_UI.h` (new predicate). **Sequence:** independent —
no file overlap with STEP234/STEP236.

Ratifies `work_orders/DESIGN_MarkerLink_R1.md` §2.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above, not just once at ticket
start.

## Problem

Both buttons always create an empty Group/Layer and never touch the current marker selection, even
when the selection is entirely one Marker Type and reassigning it is unambiguous.

## Fix

1. New predicate in `MarkersTab_ManualInstanceSelection_UI.h`:
   ```cpp
   bool IsManualInstanceSelectionEntirelyType(const std::vector<Params::MarkerInstanceGroup>& markers,
                                              const std::vector<int>& selectedIdentifiers,
                                              const std::string& typeName);
   ```
   True only if every selected identifier resolves to a transform whose own group (folded through
   `Params::CanonicalMarkerTypeSectionName`) equals `typeName`. Empty selection returns `false`.
2. `bAddGroupClicked` branch (`MarkersTab_UI.cpp`): after constructing the new `MarkerLayerBundle`,
   if `IsManualInstanceSelectionEntirelyType(...)` is true, also create the Group's first
   `MarkerInstanceLayer` (same convention already used elsewhere for a newly-created Group's first
   Layer) and reassign the selection into it via `ReassignManualInstanceLayers`.
3. `bAddManualLayerClicked` branch: after constructing the new `MarkerInstanceLayer`, if the
   predicate is true, `ReassignManualInstanceLayers(recipe.markers,
   state.selectedManualInstanceIdentifiers, newLayerIndex)`.
4. Mixed-type or empty selection: both buttons behave exactly as today (empty container, no move,
   no error/dialog).

## Verify

- New tests: same-type selection ends up moved into the new Group/Layer's transforms; mixed-type
  selection leaves the new container empty and every instance's `layerIndex` unchanged; empty
  selection unchanged from today's behavior.
- Existing `MarkersTab_UI_Test`, `MarkersTab_ManualInstanceSelection_UI_Test` stay green.

## Out of scope

- Cross-type selections — that's the Link mechanic (`DESIGN_MarkerLink_R1.md` §3), a separate
  ticket blocked on ARCH ratification.

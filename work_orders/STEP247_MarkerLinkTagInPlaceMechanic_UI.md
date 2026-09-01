# STEP247 — "+Link" tags instances in place; no-op guard; Delete-Link clears the instance tag

**Layer:** UI. **Domain:** `src/ui/MarkersTab_Links_UI.h/.cpp`,
`src/ui/MarkersTab_ManualInstanceSelection_UI.h/.cpp`. **Sequence:** depends on STEP244; revises
STEP239's landed `ApplyAddLinkAction`/`DeleteMarkerLink`.

Ratifies `ARCH_19_33_LinkMembershipInstanceTierCorrection.md`'s "No-op guard and `ApplyAddLinkAction`/
`DeleteMarkerLink`" section. Direct human ruling, `work_orders/BRIEF_MarkerLinkCorrection_R1.md`:
"+Link" must stop minting any Group/Layer and stop moving instances — existing layering/grouping
stays completely untouched; a Link becomes a pure per-instance tag. If ANY selected instance already
belongs to ANY existing Link, do nothing (no new Link, no tagging) — proceeding would silently break
that instance's existing Link membership.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. `MarkersTab_ManualInstanceSelection_UI.h`/`.cpp` — add two new functions, siblings of the existing
   `ReassignManualInstanceLayers`/`IsManualInstanceSelectionEntirelyType`:
   ```cpp
   // Tags every selected instance directly — mirrors ReassignManualInstanceLayers's exact walk,
   // one field over (linkIdentifier instead of layerIndex).
   void TagManualInstancesWithLink(std::vector<Params::MarkerInstanceGroup>& markers,
                                   const std::vector<int>& taggedIdentifiers, int linkIdentifier);

   // True the moment ANY resolved selected identifier already carries linkIdentifier >= 0. An
   // unresolved/stale identifier is skipped (Constitution §6), never itself a reason to block.
   bool IsAnyManualInstanceSelectionAlreadyLinked(const std::vector<Params::MarkerInstanceGroup>& markers,
                                                  const std::vector<int>& selectedIdentifiers);
   ```
2. `MarkersTab_Links_UI.cpp` — rewrite `ApplyAddLinkAction`:
   ```cpp
   void ApplyAddLinkAction(Params::MapRecipe& recipe, const std::vector<int>& selectedManualInstanceIdentifiers) {
       if (selectedManualInstanceIdentifiers.empty()) return;
       if (IsAnyManualInstanceSelectionAlreadyLinked(recipe.markers, selectedManualInstanceIdentifiers)) return;

       Params::MarkerLink link;
       link.identifier = NextMarkerLinkId(recipe.markerLinks);
       link.name       = "Link " + std::to_string(link.identifier);
       recipe.markerLinks.push_back(link);

       TagManualInstancesWithLink(recipe.markers, selectedManualInstanceIdentifiers, link.identifier);
   }
   ```
   **Delete the Bundle/Layer-minting and `ReassignManualInstanceLayers` call entirely** — no
   `Params::MarkerLayerBundle`, no `Params::MarkerInstanceLayer` is created by this action any more.
   `PartitionSelectedManualInstancesByType` is no longer called here — it becomes a pure UI-display
   helper only, reused by STEP248's Links-Section body (do not delete it).
3. `MarkersTab_Links_UI.cpp`/`.h` — `DeleteMarkerLink` gains a third walk and a new `markers` parameter:
   ```cpp
   void DeleteMarkerLink(int linkIdentifier, std::vector<Params::MarkerLink>& links,
                         std::vector<Params::MarkerInstanceGroup>& markers,             // NEW
                         std::vector<Params::MarkerLayerBundle>& bundles,               // kept, legacy fallback
                         std::vector<Params::MarkerInstanceLayer>& markerLayers);       // kept, legacy fallback
   ```
   Add, as the FIRST walk (before the two existing Bundle/Layer walks): clear
   `transform.linkIdentifier` to `-1` for every `MarkerTransform` across `markers` matching
   `linkIdentifier`. **Keep the two existing Bundle/Layer-tier clearing walks — do not remove them.**
   They are dead-write/live-read backward compat for any `.sanmap` still carrying pre-correction
   Layer-exclusive Link data (`ARCH_19_33`'s explicit backward-compat ruling — no migration, no
   special-casing, the old walks simply become a no-op for every Link this ticket's own
   `ApplyAddLinkAction` mints going forward, while staying load-bearing for legacy data).
   Update the one call site in `DrawMarkerLinksSection` to pass `recipe.markers`.

## Verify

- "+Link" on a fresh, unlinked selection: mints exactly one `Params::MarkerLink`, tags every selected
  instance's `MarkerTransform::linkIdentifier`, creates ZERO new `MarkerLayerBundle`/
  `MarkerInstanceLayer` entries, and leaves every selected instance's `layerIndex`
  unchanged (confirm against its value both before and after the action).
- "+Link" on a selection where at least one instance already has `linkIdentifier >= 0` (from a prior
  Link): confirm NOTHING happens — `recipe.markerLinks` unchanged in size, no instance's
  `linkIdentifier` written, including the ones that WERE unlinked in that same selection.
- "+Link" on a selection entirely already in the SAME existing Link: confirm no-op (same check, same
  outcome).
- Delete-Link on a Link minted by this ticket's own `ApplyAddLinkAction`: every tagged instance's
  `linkIdentifier` resets to `-1`; no Bundle/Layer is touched (none exist to touch).
- Delete-Link on a Link with pre-existing (legacy, hand-authored-for-test) Bundle/Layer-tier tags and
  no transform-tier tags: confirm the existing two walks still clear those — a regression test for
  the "kept, not removed" requirement above.
- Extend `MarkersTab_Links_UI_Test.cpp` to cover all of the above; existing tests for the OLD
  mint-and-move behavior should be replaced, not left asserting the retired behavior.

## Out of scope

- `DrawMarkerLinksSection`'s own signature/body/call-site relocation (STEP248).
- Resolver wiring (STEP246) — already landed by the time this ticket runs, but not this ticket's job
  to touch further.

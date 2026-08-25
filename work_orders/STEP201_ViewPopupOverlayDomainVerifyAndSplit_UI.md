# STEP201 — VIEW popup per-marker-type toggles: verify post-STEP200, split Spawns/Armies

**Layer:** UI. **Domain:** `src/ui/Application_ViewLayersPopup_UI.cpp`, `Application_OverlaySetup_UI.cpp`,
`Application_OverlaySetup_Seed_UI.cpp`, `OverlayLayer_Settings_UI.h` (reuse only, no redesign expected
beyond the one enum-value split below).

## Root problem
The human asked for "preview layers for each TYPE of marker (Alloy, Units, Props, Reclaim, Spawns,
etc.) on the VIEW button." Read-only research (confirmed by direct code reading, not guessed) found
**this already exists and is fully wired end-to-end** — this is not a missing feature, it's an
under-discovered one, plus one literal gap against the exact wording:

1. `OverlayDomainKind_UI` (`src/ui/OverlayLayer_Settings_UI.h:18`) already enumerates
   `{ Alloy, SpawnsArmies, Units, Props, Reclaim, Decals }` — 6 domains, seeded as `"Alloy"`,
   `"Spawns/Armies"`, `"Units"`, `"Props"`, `"Reclaim"`, `"Decals"` in
   `Application_OverlaySetup_UI.cpp:24-47`.
2. `DrawOverlaySection()` (`Application_ViewLayersPopup_UI.cpp:88-107`) already lists all 6 as rows in
   the VIEW popup via `DraggableList<OverlayLayer_UI>`, each with a working `[o]/[-]` visibility
   affordance wired to `OverlayLayer_UI::bEnabled` → `ApplyViewLayerSignal`'s `ToggleVisibility` case
   (`Application_ViewLayersPopup_UI.h:23-29`) → `BumpLayerSettingsRevision()`
   (`Application_ViewLayersPopup_UI.cpp:124` at time of research) → cache invalidation.
3. The icon-layer cull pass already honors it: `MapCanvas_IconLayer_Cull_UI.cpp:89`
   (`if (!layer.bEnabled) continue;`, also `:122` for the click-picker path) — toggling a domain off
   in the popup already hides every marker/prop routed to that domain at seed time
   (`Application_OverlaySetup_Seed_UI.cpp:35-58` Spawn-category → SpawnsArmies, `:87` reclaimable
   props → Reclaim vs. Props, `:66-72` army units → Units).
4. **The one literal gap**: the human's wording lists "Spawns" as its own item; the current
   implementation combines it with "Armies" into a single `SpawnsArmies` domain/row. Splitting that is
   the only concrete enum/UI change this ticket proposes — everything else is verification, not new
   plumbing.
5. STEP200 (already committed, `dbb7357`) changed `Application_ViewLayersPopup_UI.cpp` to route both
   `DrawTerrainSection()` and `DrawOverlaySection()` through the new `DraggableListRowLayout::Flat`
   mode — this ticket must confirm the overlay rows still render/toggle correctly post-STEP200 (they
   were not individually acceptance-tested for the overlay case in STEP200, only terrain rows were).

## Fix approach
1. **Verify, don't rebuild.** Confirm by direct testing (extend
   `Application_ViewLayersPopup_UI_Test.cpp` or its sibling test files) that each of the 6 (soon 7)
   overlay domain rows renders as a flat, non-collapsible, fixed-width row with a working left-side (or
   wherever STEP200 placed it — check current code) visibility toggle, and that toggling one off
   actually removes only that domain's markers from `ResolveVisibleCandidates`
   (`MapCanvas_IconLayer_Cull_UI.cpp:89`) while leaving other domains and the underlying marker data
   untouched.
2. **Split `SpawnsArmies` into `Spawns` and `Armies`.** Add the new enumerator to
   `OverlayDomainKind_UI` (`OverlayLayer_Settings_UI.h:18`), update the seed list in
   `Application_OverlaySetup_UI.cpp:24-47` to two rows (`"Spawns"`, `"Armies"`), and update the routing
   in `Application_OverlaySetup_Seed_UI.cpp:35-58` so Spawn-category markers route to `Spawns` and army
   unit instances route to `Armies` (read the current routing logic fresh — determine whether the
   existing split between "Spawn-category markers" and "army units" already cleanly maps to two
   groups, or whether some instances are ambiguously classified today; document whichever is true).
3. Do not touch `Params::MarkerCategory` (`MarkerRule_PARAMS.h:18`) or
   `Params::MarkerInstanceGroup::name` (`MarkerInstance_PARAMS.h:62-68`) — those are separate systems
   (procedural marker rules and free-form hand-placed group names respectively), unrelated to this
   popup's overlay-domain toggles. Do not conflate them.

## Explicit out-of-scope
- Any finer-than-domain toggle (e.g. per individual `MarkerInstanceGroup`, or per procedural
  `MarkerRule`/`MarkerCategory`) — STEP111 confirmed `Data::PlacementInstance::category` is available
  at cull/emit time if this is wanted later, but it is not requested here.
- Any change to marker color/icon rendering (STEP111/112/114 territory) or drag/tab-selection gating
  (STEP113 territory) — this ticket only touches domain-level visibility toggling.
- Renaming or restructuring any other `OverlayDomainKind_UI` value beyond the one Spawns/Armies split.

## Acceptance test
Extend the overlay-section coverage in `Application_ViewLayersPopup_UI_Test.cpp` (or add a sibling
test file matching STEP200's convention): all 7 domain rows render as flat/non-collapsible with a
working visibility toggle; toggling `Spawns` off hides only spawn markers and leaves `Armies` markers
visible (and vice versa); toggling any domain off does not affect `Data::PlacementInstance` data, only
the cull pass's visibility decision (assert on `ResolveVisibleCandidates` output count, not on
underlying instance data).

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.

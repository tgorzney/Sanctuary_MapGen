# STEP202 — Per-army Units overlay rows, procedural-unit routing, army tint, default color palette

**Layer:** UI/PARAMS/IO. **Domain:** `src/ui/Application_OverlaySetup_UI.cpp`,
`Application_OverlaySetup_Seed_UI.cpp`, `ArmiesTab_UI.cpp`, `OverlayLayer_Settings_UI.h`,
`src/params/Army_PARAMS.h`, `src/io/MapImporter_Armies_IO.cpp`, plus whatever draws the Units
icon-overlay tint (find the real current call site — likely in `MapCanvas_IconLayer_CullManual_UI.cpp`
or `MapCanvas_IconLayer_Draw_UI.cpp`, verify before editing).

**ARCH rules invoked:** `ARCH_14_16_PerArmyUnitsOverlayRows.md` §14.16 (A/B/C/D below map 1:1 to that
section's ruling — this work order does not re-derive any design decision, only sequences
implementation). Amends §14.5's closing line per §14.16-C.

## Root problem
The human wants the VIEW popup's single "Units" overlay row replaced with one row per army (named
`ARMY_XX`/displayName, off real sanmap army data), independently toggleable, with each army's unit
icons tinted that army's own color. ARCH ratified this as feasible with near-zero architectural
change (§14.16) — `overlayLayers` is already an open vector, `DraggableList` Flat mode (STEP200) and
the icon-culling gate already handle N rows per domain with zero code change. The ARCH ruling also
found the feature would ship visually inert: every army defaults to plain white today, and nothing —
Add Army, import, export — ever assigns a distinct default color (v1 had an 8-color rotating palette
never ported to v2/v3). Fix approach below implements ARCH §14.16's four ruled pieces in dependency
order (D before C, since C is inert without it).

## Fix approach
1. **(D) Default color palette — land first.** Add `inline constexpr float kDefaultArmyColors[8][4]`
   to `src/params/Army_PARAMS.h` beside `Army`, exact values from ARCH §14.16-D (Red, Pink, Orange,
   Purple, Blue, Teal, Green, LimeGreen — copy verbatim from the ARCH file, do not retype by hand).
   Two mint sites, both read this one constant:
   - **Add Army** (`ArmiesTab_UI.cpp:71-77`): before `push_back`, assign
     `army.armyColor = kDefaultArmyColors[armies.size() % 8]` alongside the existing `displayName`
     seed.
   - **Import backfill** (`MapImporter_Armies_IO.cpp`): change the discriminator from "is the color
     white" to "was the `armyColor` JSON key present at all" — `ReadArmyColorJson` already
     early-returns on absent key (`:88` at ARCH-research time, re-verify), change it to report
     presence (return `bool`, matching `ReadJsonFloat`'s existing pattern in this file) and have the
     caller backfill `kDefaultArmyColors[rosterPosition % 8]` when absent. Roster position is
     confirmed stable at parse time (`NormalizeArmyIdentities` renumbers `name` in place, never
     reorders the vector) — read `MapImporter_ArmyIdentityNormalize_IO.cpp` fresh to confirm before
     relying on this. Do not touch `MapExporter_Armies_IO.cpp` — it already writes `armyColor`
     unconditionally regardless of source.
2. **(A) Per-army row seeding.** In `ConfigureDefaultOverlayLayers`
   (`Application_OverlaySetup_UI.cpp:30-33,46-47`), replace the single shared
   `OverlayLayer_UI{domainKind=Units}` construction with a loop over `recipe.armies`, one row per
   army, named via `ArmyRowLabel(army)` (`ArmiesTab_UI.h:82-85` — reuse verbatim, do not reimplement
   the displayName-falls-back-to-name-falls-back-to-"Army" rule). No change to `OverlayDomainKind_UI`
   or any other domain's seeding.
3. **Sub-layer seeding follows the row split.** `SeedUnitsManualSubLayers`
   (`Application_OverlaySetup_Seed_UI.cpp:66-72`) keeps `ResolveUnitsManualSubLayer`'s existing global
   flat-index formula over `recipe.armies[*].groups` completely unchanged (do not touch that
   function) — only change which row each resolved `(armyIndex, groupIndex)` pair's
   `OverlaySubLayerRef_UI{Manual, flatIndex}` gets pushed into: the one row whose army matches,
   mirroring the existing `SeedMarkerDomains`/`SeedPropReclaimDomains` "push into whichever row owns
   it" pattern.
4. **(B) Procedural-unit routing — real `armyIndex`, no separate bucket.** For each
   `recipe.unitRules[i]`, bounds-check `0 <= armyIndex < recipe.armies.size()` and push
   `OverlaySubLayerRef_UI{ProceduralRule, i}` into `overlayLayers[armyIndex]`'s row. An out-of-range
   `armyIndex` (corrupt/hand-edited data) drops the ref silently — same defensive floor
   `ResolveUnitsManualSubLayer` already applies to Manual refs, do not throw or assert. ARCH §14.16-B
   confirmed `UnitRule::armyIndex` is a real, actively-maintained positional index into
   `recipe.armies` (not the stale "Faction value" comment at `ScatterRule_PARAMS.h:89`) — fix that
   stale comment as part of this step (housekeeping, not a shape change), but do not change the
   field's type or the PROC passthrough (`Placement_Rules_PROC.cpp:56`,
   `Data::PlacementInstance::armyIndex`).
5. **(C) Per-army tint.** Find the real current Units icon-draw/tint call site (read
   `MapCanvas_IconLayer_CullManual_UI.cpp` and `MapCanvas_IconLayer_Draw_UI.cpp` fresh — do not assume
   stale line numbers) and change it to read `recipe.armies[armyIndex].armyColor` directly instead of
   `OverlaySessionAppearance::unitsAppearance.color`. `unitsAppearance.iconScale` is NOT retired —
   keep it as the one shared icon-scale default across every per-army Units row. You may drop
   `unitsAppearance`'s now-dead `color` sub-field (`OverlayLayer_Settings_UI.h:53,62`) as a small
   cleanup, or leave it unused if removal touches more call sites than expected — either is
   acceptable, note which you did.

## Explicit out-of-scope
- Forking `unitsAppearance.iconScale` per-army — stays one shared UI-session default, per ARCH
  §14.16-C. Separate future ask if ever wanted.
- Any change to `OverlayDomainKind_UI`, `DraggableList`, or the icon-culling gate's switch
  statements — ARCH confirmed both already handle N rows per domain with zero code change.
- The still-pending literal "Spawns/Armies → Spawns/Armies split" question from STEP201 — resolved
  separately (STEP201 follow-up already renamed the row to "Spawns"; no further split needed, that
  domain never contained army data).
- Any other `OverlayDomainKind_UI` value's seeding, naming, or routing.
- Re-deriving `ResolveUnitsManualSubLayer`'s flat-index formula — reuse it unchanged.

## Acceptance test
Extend the overlay-domain test coverage (`Application_ViewLayersPopup_OverlayDomainAcceptance_UI_Test.cpp`,
`MapCanvas_IconLayer_Cull_OverlayDomainToggle_UI_Test.cpp`, and an `ArmiesTab`/`MapImporter_Armies_IO`
test for the palette backfill — find or create matching this suite's file-per-concern convention).
Cover: `ConfigureDefaultOverlayLayers` on a recipe with N armies produces exactly N Units-domain rows
named via `ArmyRowLabel`; toggling one army's row hides only that army's manual AND procedural units
from `ResolveVisibleCandidates`, leaving other armies' units and every other domain untouched; a
procedural `UnitRule` with an out-of-range `armyIndex` is silently dropped, not attached to any row,
not crashing; Add Army assigns `kDefaultArmyColors[armies.size() % 8]`, not white; importing a
`.sanmap` with no `armyColor` key backfills from the same palette by roster position, while a
`.sanmap` that explicitly authored white stays white (key-presence discriminator, not a value
comparison); a Units icon renders tinted with its owning army's `armyColor`.

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.

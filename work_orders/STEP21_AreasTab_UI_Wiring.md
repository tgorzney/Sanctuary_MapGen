# Work-Order — Step 21: wire the Areas tab to `Params::MapArea`

*Constitution §7. Executor: SanGen Coder. Companion to `work_orders/STEP20_ArmiesTab_UI_Wiring.md`
(same UI Expert consult) — retires `AreasTab_UI.h` SCOPE NOTE 1 by editing the real
`Params::MapArea` (shipped `work_orders/STEP2_ArmiesAreas_IO.md`) instead of the UI-only
`MapAreaRectangle`.*

## Root problem
`MapAreaRectangle` (`AreasTab_List_UI.h`) is caller-owned UI-only state. `Params::MapArea` exists
and round-trips through `.sanmap` (Step 2), but the tab still edits the disconnected presentation
type. `AreasTab_UI`'s `recipe` parameter is currently `const Params::MapRecipe&` — read-only,
confirming the tab has never actually written to the real recipe.

## Ruled by this ticket (UI Expert consult — binding, shared with Step 20)
1. **`MapAreaRectangle` is DELETED.** Every pure lifecycle rule in `AreasTab_List_UI.h`
   (`IsPlayableArea`, `IsAreaRemovable`, `AreaRowLabel`, `ResolvedAreaMapSize`, `SetAreaToMapSize`,
   `NextAreaName`, `AreaNameIsTakenBefore`, `MakeAreaNamesUnique`, `EnsurePlayableArea`) retypes
   mechanically onto `Params::MapArea` — same logic, same tests, just the element type
   (`originX`/`originZ`/`width`/`length` are already verbatim field names on `Params::MapArea`).
   **If `MakeAreaNamesUnique`/`AreaNameIsTakenBefore`/`NextAreaName` are genericized as part of
   Step 20 (the ARCH-Expert-placed shared helper), this ticket calls that shared version instead
   of keeping a local copy — coordinate with Step 20's actual landing, don't duplicate.**
2. **`AreasTab_UI.h`'s `recipe` parameter becomes non-const.** `AreasTabState.areas` is deleted;
   the tab edits `recipe.areas` directly. SCOPE NOTE 1 is retired.
3. **The four real scalars use a single shared toggle set**, not per-row — same reasoning as
   Step 20 (`originXToggle`/`originZToggle`/`widthToggle`/`lengthToggle` become tab-state
   singletons; this already matches how `AreasTabState`/`AreasTab_UI.cpp` behave today for their
   single-selection editor, just moving off the per-row struct).
4. **Color has NO home on `Params::MapArea`** (confirmed: the ratified spec never gave Area a
   color field, unlike `Army.armyColor` which Correction 11 explicitly added) — it must stay
   UI-only. Since its VALUE must persist per-area across selection changes (a single shared
   scratch value would let switching the selected area clobber its neighbor's color), keep it as
   a small UI-only side table, **keyed by `MapArea::name`, not vector position** (position drifts
   under Reorder for no reason color needs to care about):
   ```cpp
   struct AreaColorEntry { std::string name; float color[kColorSwatchChannelCount] = {1,1,1,0.35f}; };
   std::vector<AreaColorEntry> areaColors;   // AreasTabState; find-or-create, same linear-scan
                                              // idiom AreaNameIsTakenBefore already uses
   RealtimeToggle colorToggle;               // single instance — only the selected area draws
   ```
   `DrawAreaSettings` resolves via `ResolveAreaColor(state.areaColors, area.name)` (finds by name,
   appends a default entry on first touch).
5. **Renaming an area must retarget its color entry, not orphan it.** Capture the area's name
   BEFORE calling `DrawTextInput` for Name; if the commit changed it, retarget the matching
   `AreaColorEntry.name` to the new value. Without this, renaming a colored area silently reverts
   its color to default next frame — a real regression from today (color lives on the row object
   itself today, immune to renames). Orphaned entries from a DELETED area need no pruning
   (never serialized, never a perf concern at this scale).
6. **`EnsurePlayableArea`'s default color** (today `{1,1,1,0.35f}` on `MapAreaRectangle`'s field
   default) moves to `AreaColorEntry`'s default — same value.
7. **"Add New Area" must explicitly seed width/length.** `Params::MapArea`'s own defaults are
   `width=0, length=0` (correct for "absent from an imported file degrades to nothing" —
   Constitution §6) but wrong for a fresh authored row (invisible, unusable). Keep the existing
   UX: after `recipe.areas.push_back(Params::MapArea())`, explicitly set `.width = 100.0f;
   .length = 100.0f;` — mirrors the handler already explicitly setting `.name`. This is a
   UI-authoring-convenience decision, not a PARAMS-default change.
8. **Dirty-flag posture: unchanged, stays notifying.** Unlike Army, an area rectangle IS drawn as
   a preview overlay — `NotifyPlacementChange(bAreasMoved, previewDriver)` staying wired exactly
   as today is correct; recompositing after an area edit is real, visible work.

## Target files
- `src/ui/AreasTab_List_UI.h` — delete `MapAreaRectangle`; retype every pure rule onto
  `Params::MapArea`; add `AreaColorEntry`/`ResolveAreaColor`.
- `src/ui/AreasTab_UI.h` — `recipe` param non-const; `AreasTabState.areas` deleted; add
  `areaColors`/`colorToggle`; add the four singleton toggles for the real scalars; retire SCOPE
  NOTE 1.
- `src/ui/AreasTab_UI.cpp` — `DrawAreaSettings` binds directly to `Params::MapArea` fields plus
  the resolved color; the rename-retargets-color-entry fix; the explicit width/length seed on Add
  New Area.
- `src/ui/AreasTab_UI_Test.cpp` — retype every test onto `Params::MapArea`;
  `AreasTabState.areas`-based tests become local-`std::vector<Params::MapArea>`-based; add a
  color-rename-retargeting test (rename a colored area, assert its color survives, not reset).

## Layer & accuracy class
UI. Accuracy class: Visual/Exact for the four real scalars (now real recipe content); color stays
purely Visual (no format home, by design).

## Backend policy
N/A — pure UI/imgui composition.

## ARCH rules invoked
- `ENTITY_AUTHORING_PARAMS_SPEC.md` — the ratified `Params::MapArea` shape, verbatim.
- Constitution §6 — `AreaColorEntry` keyed by name (not position) is itself an application of
  "don't let an unrelated operation (reorder) silently corrupt state that has no reason to move."

## Explicit out-of-scope
- **Adding a color field to `Params::MapArea`** — explicitly ruled out; color has no format home
  and stays UI-only by design, not by omission.
- **Any change to `Params::MapArea`'s PARAMS shape** — fields/defaults/JSON keys unchanged.
- **The Playable Area's specific behavior** (`IsPlayableArea`/`IsAreaRemovable`/
  `EnsurePlayableArea`) beyond the mechanical element-type retype — its logic is unchanged.

## Acceptance test
`AreasTab_UI_Test.exe` passes with: `Params::MapArea`-typed fixture data throughout; a color
survives a rename (the retargeting fix); a fresh "Add New Area" has visible (non-zero) width/
length; the Playable Area's un-removability and map-size-follow behavior are unchanged. Full
`SanGenV2` build stays clean.

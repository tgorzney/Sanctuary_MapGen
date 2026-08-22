# Work-Order — Step 20: wire the Armies tab to `Params::Army`

*Constitution §7. Executor: SanGen Coder. Retires `ArmiesTab_UI.h` SCOPE NOTE 1 by editing the real
`Params::Army` (shipped `work_orders/STEP2_ArmiesAreas_IO.md`) instead of the UI-only
`ArmyPresentation`. Design from a dedicated UI Expert consult (binding — this ticket surfaces and
fixes two real bugs the consult found, not just a mechanical retype).*

## Root problem
`ArmyPresentation` (`ArmiesTab_UI.h`) is caller-owned UI-only state explicitly flagged as
temporary by its own SCOPE NOTE 1. `Params::Army` now exists and round-trips through `.sanmap`
(Step 2), but the tab still edits the disconnected presentation type — nothing a designer types in
the Armies tab today is ever saved.

## Ruled by this ticket (UI Expert consult — binding)
1. **`ArmyPresentation` is DELETED, not kept as a mirror.** The tab edits `recipe.armies`
   (`std::vector<Params::Army>&`) directly, matching the established pattern `ArmyUnitListState`/
   `LayerEditorState` already use for a single-selection editor over a real PARAMS vector (one
   shared `RealtimeToggle` set for the currently-selected row, not per-row state — per-row toggle
   arrays are only needed when multiple rows can be mid-drag in the SAME frame, which doesn't apply
   here since only the selected army's detail section draws).
2. **`armyFactionLabels` is WRONG and must be fixed as part of this ticket** — currently
   `{"UEF","Cybran","Aeon"}` (Supreme Commander names, a v1 leftover), harmless while
   `factionIndex` was a meaningless UI-only int. `Params::Faction` is the ratified `{Chosen,
   Guard, EDA}` (`UNIT_PROP_MARKER_DATA_SPEC.md`). Once cast into the real enum and round-tripped,
   a mislabeled combo actively lies to the designer. Fix: relabel to `{"Chosen","Guard","EDA"}`,
   same order as the enum.
3. **`Params::Army::alias` has NO control anywhere today** — a real, already-ratified field
   (Correction 11) that's unreachable. Add a `DrawTextInput("Alias", army.alias, ...)` — costs one
   call, no new state.
4. **A real, pre-existing bug this ticket must fix, not preserve: dragging an army row doesn't
   renumber `UnitRule::armyIndex`.** `ApplyArmyListSignal` only repairs `armyIndex` on Delete
   (`DropUnitRulesForRemovedArmy`), never on Reorder — today this is latent (armies were
   unserialized), but the moment `recipe.armies` is what gets saved, a drag-reorder silently
   desyncs every unit rule's army ownership. Add `RenumberUnitRuleArmyIndicesForReorder` (exact
   implementation given below) and wire it into `ApplyArmyListSignal`'s Reorder branch, called
   BEFORE `ApplyDraggableListSignal(recipe.armies, signal)` runs (needs the pre-move array size).
5. **A real data-loss bug: no army-name-uniqueness protection exists.** `armies` exports as a JSON
   object keyed by `Army::name` (same shape as `areas`, Step 2 finding 1) — two blank-named armies
   (trivially produced: click "Add Army" twice) silently collide on export, one clobbering the
   other. Areas already has this exact protection (`MakeAreaNamesUnique`/`AreaNameIsTakenBefore`/
   `NextAreaName`, `AreasTab_List_UI.h`) — Armies needs the equivalent.

   **RESOLVED (ARCH Expert ruling): new file `src/ui/UniqueNameList_UI.h`.** Not
   `WidgetHelpers_UI.h` (scoped to widget-VALUE math, a different concept), not folded into
   `DraggableListWidget_UI.h` (that file's genericization pairs with the drag widget itself; this
   pairs with no widget), not left permanently attached to `AreasTab_List_UI.h` (a cross-entity
   template doesn't belong owned by one entity's file). Headless, no imgui include, same posture
   as `WidgetHelpers_UI.h`/`DraggableListWidget_UI.h`:
   ```cpp
   template<typename T>
   bool NameIsTakenBefore(const std::vector<T>& rows, std::size_t rowIndex, const std::string& name);
   template<typename T>
   bool MakeNamesUnique(std::vector<T>& rows);
   inline std::string NextUniqueLabel(const char* baseLabel, int existingCount);
   ```
   Constrained only to "has a `std::string name` member" — both `Params::MapArea` and
   `Params::Army` satisfy this trivially (verbatim `name` fields, confirmed).
   **THIS TICKET MUST DELETE the three local functions from `AreasTab_List_UI.h` and redirect its
   callers (`AreasTab_UI.cpp`, `AreasTab_UI_Test.cpp`) to the new shared template IN THE SAME
   DIFF — replace, do not wrap/forward "for now."** This closes the divergence window (a moment
   where a local Areas copy and a new shared Armies version both exist) by construction, not by
   careful sequencing. `AreasTab_List_UI.h` keeps a one-line domain wrapper,
   `NextAreaName(count) { return NextUniqueLabel("NewArea", count); }`; Armies gets its own
   one-line `NextArmyName` analogously, or pushes a blank name and calls `MakeNamesUnique`
   directly — either is a mechanical Coder choice.
   `work_orders/STEP21_AreasTab_UI_Wiring.md` (the Areas ticket) is written to keep working
   correctly against this shared version once it lands — no coordination needed beyond doing this
   ticket first (ruled sequential, not parallel).
6. **Faction combo has no `RealtimeToggle`** (matches today's behavior — commits immediately, no
   deferred-drag concern for a combo): use a same-frame local `int` mirror,
   `static_cast<int>(army.faction)` in, `static_cast<Params::Faction>(factionIndex)` out.
7. **Dirty-flag posture: unchanged, stays silent.** `DrawArmySettings` calls
   `NotifyPlacementChange` for NOTHING today — nothing hashes or previews an army's own fields.
   Confirmed still true (checked `PreviewComposite_UI.cpp`). Keep it exactly as-is; do not add
   notification calls just because the fields are "real" now — nothing consumes them yet. Update
   SCOPE NOTE 1's wording (it now has a `_PARAMS` home) but keep this substance.
8. **Hand-placed `Army.groups`/`UnitGroup`/`UnitTransform` UI is explicitly OUT OF SCOPE.** This
   ticket wires the army's OWN fields (name/alias/faction/color/resources) to their real home. The
   recursive hand-placement tree is a net-new UI feature (canvas placement or manual entry —
   undesigned) with its own widget questions; bundling it here blows the ticket's blast radius.
   `recipe.armies[i].groups` stays reachable only by hand-editing/import until a dedicated,
   separate ticket designs that authoring UI.

## Target files
- `src/ui/ArmiesTab_UI.h` — delete `ArmyPresentation`; `ArmiesTabState.armies` deleted; add the
  single shared toggle set (`armyColorToggle`/`alloysToggle`/`energyToggle`); relabel
  `armyFactionLabels`; `SelectedArmy` becomes `Params::Army* SelectedArmy(std::vector<Params::
  Army>&, int)`; `ArmyRowLabel` retypes mechanically; retire SCOPE NOTE 1's "no home" framing.
- `src/ui/ArmiesTab_UI.cpp` — `DrawArmySettings` binds directly to `Params::Army` fields (name,
  new alias field, armyColor via `DrawColorSwatch`, faction via the int-mirror pattern, alloys/
  energy via `DrawSliderScalar`); `ApplyArmyListSignal` gains the Reorder-renumbering call and the
  name-uniqueness repair; "Add Army" pushes a `Params::Army()` and assigns it a unique name via
  the new shared name-uniqueness helper (not a blank name).
- New: `src/ui/UniqueNameList_UI.h` (ARCH-ruled location/shape, ruling #5) — the genericized
  name-uniqueness template. `src/ui/AreasTab_List_UI.h` loses its three local functions in this
  same diff, redirected to the new shared template.
- `src/ui/ArmiesTab_UI_Test.cpp` — retype `RunArmyPresentationChecks` onto `Params::Army`; add
  reorder-renumbering test cases (source below target, source above target, source==target no-op);
  add the name-uniqueness-repair test (two blank "Add Army"s produce distinct names).
- `src/ui/ArmiesTab_Units_UI.h`/`.cpp` — **no change**, confirmed by the consult: already edits
  `recipe.unitRules` directly, never touched `ArmyPresentation`.

## Layer & accuracy class
UI. Accuracy class: Visual/Exact (the underlying data is now real recipe content, even though the
widget presentation is visual).

## Backend policy
N/A — pure UI/imgui composition.

## ARCH rules invoked
- `ENTITY_AUTHORING_PARAMS_SPEC.md` — the ratified `Params::Army` shape this ticket wires to,
  verbatim, no retyping of the PARAMS type itself.
- Constitution §6 — the name-uniqueness repair prevents silent data loss on export, same principle
  Areas' existing repair already embodies.
- ARCH_01_05_FileSizeCeilings.md §1.5 — the shared name-uniqueness helper's file placement is an open call for the ARCH
  Expert, not invented by this ticket.

## Solution — `RenumberUnitRuleArmyIndicesForReorder` (exact implementation, UI-Expert-provided)
```cpp
// Keeps every rule's armyIndex correct after `recipe.armies` is reordered from source to target
// (the exact same erase-then-insert move ApplyDraggableListSignal performs on the armies vector
// itself) — the Reorder-signal counterpart to DropUnitRulesForRemovedArmy's Delete-signal repair.
inline bool RenumberUnitRuleArmyIndicesForReorder(std::vector<Params::UnitRule>& unitRules,
                                                   int sourceArmyIndex, int targetArmyIndex,
                                                   int armyCount) {
    if (sourceArmyIndex < 0 || sourceArmyIndex >= armyCount) return false;
    int clampedTarget = targetArmyIndex;
    if (clampedTarget < 0) clampedTarget = 0;
    if (clampedTarget > armyCount - 1) clampedTarget = armyCount - 1;
    if (clampedTarget == sourceArmyIndex) return false;
    bool bRecipeMoved = false;
    for (Params::UnitRule& rule : unitRules) {
        if (rule.armyIndex == sourceArmyIndex) { rule.armyIndex = clampedTarget; bRecipeMoved = true; }
        else if (sourceArmyIndex < clampedTarget && rule.armyIndex > sourceArmyIndex
                 && rule.armyIndex <= clampedTarget) { --rule.armyIndex; bRecipeMoved = true; }
        else if (sourceArmyIndex > clampedTarget && rule.armyIndex >= clampedTarget
                 && rule.armyIndex < sourceArmyIndex) { ++rule.armyIndex; bRecipeMoved = true; }
    }
    return bRecipeMoved;
}
```

## Explicit out-of-scope
- **Hand-placed `Army.groups`/`UnitGroup`/`UnitTransform` authoring UI** — ruling #8.
- **`UnitRule::armyIndex`'s fundamental design** (whether it should be name-based instead of
  positional) — this ticket fixes the reorder bug within the EXISTING positional-index design
  (already committed to by the wire format, `MapExporter_UnitsStack_IO.cpp`), it does not
  redesign the linkage mechanism itself.
- **Any change to `Params::Army`/`Params::UnitRule`'s PARAMS shape** — fields, defaults, and JSON
  keys are unchanged; this is UI wiring only.

## Acceptance test
`ArmiesTab_UI_Test.exe` passes with: `Params::Army`-typed fixture data throughout; the reorder
case correctly renumbers `unitRules` for source-below-target, source-above-target, and no-op;
two "Add Army" clicks produce two distinct names; a renamed/reordered army's `unitRules` still
resolve to the correct army after both operations combined. Full `SanGenV2` build stays clean;
`ArmiesTab_Units_UI_Test` (if it exists) is unaffected.

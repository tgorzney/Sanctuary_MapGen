# DESIGN — Scenarios Tab + Runtime Lua Editor (R1)

*Authored by the SanGen UI Expert, 2026-08-21. UI-layer design consult. No PARAMS/ARCH/IO
ratified here — every new type is flagged, not invented (Constitution §7 / ARCH_08_04_CoderScopeLaw.md §8.4).
Grounded in `MAP_SCENARIO_SPEC.md`, `ARCH_14_PreviewOverlayLayering.md` §14, `PREVIEW_COMPOSITING_SPEC.md`'s overlay
section, `UI_FRAMEWORK_SPEC.md`, and the existing v2 tree (`AreasTab_UI.h`, `ArmiesTab_UI.h`,
`MarkersTab_*_UI.h`, `DraggableListWidget_UI.h`, `ConfirmDialog_UI.h`, `FilePathPicker_UI.h`).
Companion reading: `DESIGN_MarkerLayerSymmetry_R1.md`, `SEQUENCE_PreviewOverlayLayering.md`.*

> **Resolved since authoring:** the §8 open question this design flagged (item 9) is now
> SETTLED by the human in the affirmative — SanGen owns parameterized scenario data and
> renders Lua on export, never parsing Lua back in. The design's core assumption holds.

## 0. Where it lives

New top-level tab, `ScenariosTab_UI.h/.cpp`, mirroring `AreasTab_UI`/`ArmiesTab_UI`'s shape
(list management + single-selection detail editor). Added to `ApplicationPanel` and
`ApplicationTabState`, grouped near Armies/Markers/Areas since it references all three. The
Lua editor is a collapsed-by-default `Section_UI` **inside the same tab** ("Runtime Script
(advanced)"), not a separate tab — it is the interpreter for the data this tab authors.

## 1. Data model (flagged for ARCH + Format — nothing here is ratified)

Tiers are modeled as **three distinct collections**, not one flat list with a tier tag —
mirrors the reference Lua's own split, keeps "reorder" meaningful exactly where it is
load-bearing (Tier 2), and makes it structurally impossible to drag a pattern scenario into
count-priority position (the same cross-section-drop-fails trick ARCH_14_07_ViewToolbar.md §14.7 uses).

```cpp
struct Scenario {
    std::string name;                 // log/debug id only — NOT a dictionary key (spec §5)
    Params::MapArea area;             // reuse MapArea — no new rect type
    bool navy = false;
    ScenarioAlloyMode alloyMode = ScenarioAlloyMode::Explicit;

    ScenarioSpawnsPolicy spawnsPolicy = ScenarioSpawnsPolicy::NotSet;  // NEW — see §4
    std::vector<ScenarioArmySpawn>  spawns;   // meaningful only if Explicit
    std::vector<ScenarioArmyAlloys> alloys;   // shape depends on alloyMode (§5)
};
struct ScenarioArmySpawn  { std::string armyName; float positionX, positionY, positionZ; };
struct ScenarioArmyAlloys {
    std::string armyName;
    std::vector<MarkerTransform> add;     // explicit / delta.add
    std::vector<std::string>     remove;  // delta.remove only
};

enum class ScenarioAlloyMode { Explicit, Occupancy, KeepAll, Delta };
enum class ScenarioSpawnsPolicy { NotSet, Explicit, AcknowledgedInherit };  // SanGen intent flag

struct ScenarioCountClause { ScenarioCountField field; ScenarioCountComparator comparator; int value; };
enum class ScenarioCountField { Total, Human, Ai };
enum class ScenarioCountComparator { Equal, NotEqual, Less, LessOrEqual, Greater, GreaterOrEqual };
struct ScenarioCountPredicate { std::vector<ScenarioCountClause> clauses; };  // AND-only, v1

struct ScenarioPatternEntry { std::string slotPattern; Scenario scenario; };        // Tier 1
struct ScenarioCountEntry   { ScenarioCountPredicate predicate; Scenario scenario; }; // Tier 2, order = priority

struct ScenarioSettings {
    int maxArmySlotCount = 8;                           // governs slotPattern length, §2
    std::vector<ScenarioPatternEntry> patternScenarios; // Tier 1 — order NOT load-bearing
    std::vector<ScenarioCountEntry>   countScenarios;   // Tier 2 — order IS load-bearing
    Scenario defaultScenario;                           // Tier 3 — mandatory singleton
    std::string runtimeScriptText;                      // the runtime Lua buffer, §7
};
// MapRecipe gains: ScenarioSettings scenarios;
```

> ⚠️ **SUPERSEDED on `maxArmySlotCount` by `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 (ratified 2026-08-21).** The field is
> **not** a UI-side type's member — it lives on **`Params::Scenarios`** proper (top-level, map-wide,
> sibling of `patternScenarios`/`countScenarios`/`defaultScenario`), and its default is **16**, not
> 8. 16 matches the live reference and is deliberately the map's *real* slot ceiling, not the lobby
> UI's currently-exposed limit of 8 — that limit is not guaranteed permanent, and authoring below
> the true ceiling silently truncates valid slot occupancy off the end of the pattern.
>
> **This also answers this document's own open question 7** ("is `maxArmySlotCount` correctly
> decoupled from `recipe.armies.size()`?"): decoupled in the *permissive* direction — it may freely
> exceed the authored army count, which is the normal case (the live map authors 4 armies with a
> ceiling of 16) — but **coupled in the safety direction**: `maxArmySlotCount < recipe.armies.size()`
> is a loud, logged, non-blocking export-time warning naming the specific armies that can never
> appear in any `slotPattern`. Never clamped, never auto-raised, never blocks the export.
>
> Also ratified alongside it (§15.10): slot-pattern *construction* moves out of the hand-authored
> `<MapName>_data.lua` and into the SanGen-owned runtime file, so the value has exactly one
> authoritative home and nothing to hand-sync. The rest of this struct's shape is unaffected.

Reuses `Params::MapArea` and `Params::MarkerTransform` — no new rect/transform types.
`NextUniqueLabel`/`MakeNamesUnique<T>` reused for naming, but **soft** — `Scenario::name` is
log-only per spec §5, so uniqueness is UX convenience, not a format rule. Flag this so a
future coder doesn't over-enforce it.

## 2. Scenario list management

Three stacked sections, each its own `DraggableList`:

- **"Exact Slot Patterns" (Tier 1)** — reorder is cosmetic only (exact-match tier). Label
  that explicitly in the section header so nobody wastes effort ordering it.
- **"Composition Rules" (Tier 2)** — every row label carries a live priority badge
  (`"1st checked"`, `"2nd checked"`, …), not just position. Position and label always agree,
  and reordering never silently does nothing.
- **"Default (always matches)" (Tier 3)** — one fixed non-collapsible panel, not a list.

Create/duplicate/delete/rename via standard `DraggableList` signals. **Duplicate** is new (no
existing tab has it) — deep-copies immediately **below** the source in the same tier so
relative Tier-2 priority is obvious, then runs `MakeNamesUnique`. Default is exempt from
delete/duplicate.

**Reachability annotation** — each Tier-2 row and the Default row gets a computed badge:
"⚠ Unreachable (shadowed by \<earlier rule\>)" when every composition it could match is already
claimed earlier. Driven by the same evaluator as §3's matrix, so badge and matrix can't disagree.

## 3. Match-rule authoring + live composition preview

**Tier 1 editor**: `maxArmySlotCount` (1–16, shared across `ScenarioSettings`, defaults to
`recipe.armies.size()`, independently overridable) drives a row of three-state toggle buttons
(click cycles `h`→`A`→`-`), each tinted by the corresponding `Params::Army::armyColor` where a
real army exists, grey beyond. Small bespoke `ImDrawList` row, no new shared widget.

**Tier 2 editor**: a compact clause table — one row per clause (`Combo_UI` field · `Combo_UI`
comparator · integer stepper), "and" as static text between rows, `+`/`x` to add/remove.
**AND-of-clauses only** in v1; OR-groups explicitly not built, flagged as a named future
extension. Below it, an auto-generated read-only summary (`"Matches when: total == 3 and
human == 2"`).

**Live composition matrix**: a small triangular grid (rows = total, columns = human, ai
implied), bespoke `ImDrawList`. Each cell colored by whichever Tier-2/Default scenario
resolves it first; hover shows the name. Cells with a registered Tier-1 pattern get a **hatch
overlay, not a solid fill** — a (total,human,ai) triple maps to many slot arrangements, only
some of which may have a Tier-1 match, so the matrix is precise for Tier 2/3 and only
"may pre-empt" for Tier 1. State that on-screen.

**Evaluator placement (flagged for ARCH)**: the pure
`bool Matches(const ScenarioCountPredicate&, int total, int human, int ai)` driving both the
matrix and the reachability badges needs a UI-legal home. Recommend MATH, per the
`BuildSymmetryOrbit` precedent — recommended, not asserted. Note it does **not** need to be
literally shared with the Lua renderer for correctness: one clause list, two independent
readers can't drift as long as both walk the same array. Not a shadow-sim risk.

## 4. Mandatory-`spawns` warning

`ScenarioSpawnsPolicy` is a SanGen-authored tri-state capturing *authoring intent* (the
exporter renders it as the Lua comment spec §6 calls for):

- `NotSet` (default on a fresh scenario) — risk state.
- `Explicit` — the `spawns` editor is populated and authoritative.
- `AcknowledgedInherit` — designer explicitly said "share the baseline on purpose."

Three tiers of visibility so the risk can't be missed at any zoom level:

1. **List-row badge** — ⚠ glyph next to any `NotSet` scenario while merely scanning the list.
   This is literally what would have caught the live `2h1ai` bug before export.
2. **Detail-panel banner** — persistent amber banner: *"No explicit spawn positions. This
   scenario will use whatever the .sanmap's shared baseline spawn currently is — which changes
   if ANY other scenario's baseline edit touches it."* Two buttons acting on `spawnsPolicy`:
   **[Set Explicit Spawns]** (seeds `spawns` from current baseline) / **[I understand, inherit
   baseline]**.
3. **Export-time gate** — `ConfirmDialog_UI`, warn-never-block precedent: exporting with ≥1
   `NotSet` scenario names every affected scenario, **[Export Anyway]**/**[Cancel]**.
   `AcknowledgedInherit` never triggers it — that's the point of the third state.

## 5. `alloyMode` selection

`Combo_UI` over four values, paired with an always-visible consequence card (a Constitution §8
label table like `markerCategoryLabels`), never the dropdown alone:

- `explicit` — "You list every army's alloys below. Any army NOT listed loses its alloy markers entirely."
- `occupancy` — "Uses the map's own baked alloy positions. Empty army slots lose their markers; filled slots keep them."
- `keepAll` — "Uses the map's own baked alloy positions. Nothing is ever deleted, even for empty slots."
- `delta` — "⚠ Reserved — not yet used by any shipped scenario. Only listed Adds/Removes apply."

The strongest legibility mechanism is not prose but §6's canvas preview re-resolving live the
instant `alloyMode` changes.

## 6. Interactive marker editing per scenario (the core ask)

**Does not duplicate the overlay renderer.** Reuses ARCH_14_PreviewOverlayLayering.md §14's existing baseline
Alloy/SpawnsArmies overlay layers unchanged, desaturated, as a ghost baseline. On top, a
dedicated **Scenario Edit Mode draw pass** — deliberately not squeezed into the generic
`OverlayLayer_UI` machinery because (a) it is transient/single-scenario, not a stackable
View-toolbar layer; (b) its source is neither `Data::PlacementInstances` nor a `recipe.*Layer`
array; (c) it needs per-state visuals (ghost / override / deleted / added), not per-layer
uniform ones. It **does** reuse §14.9's primitives: bulk vertex writes, the atlas, and
`MapCanvasView`'s world↔screen projection once landed.

Cardinality is tens of entries — **explicitly does not need** `Data::SpatialGrid`/`Picking_UI`
O(1) machinery; a linear screen-rect hit test is correct here. Flagged so nobody over-engineers.

**Distinguishing "no override" from "override = baseline"** — never rely on identical screen
position (two markers can coincide exactly):

| State | Visual |
|---|---|
| Spawn, no override | hollow/dashed icon + ⚠ badge at baseline position |
| Spawn, explicit override | solid filled icon, even if numerically identical to baseline |
| Alloy, kept | baseline icon, normal color |
| Alloy, deleted (occupancy empty slot, or omitted under `explicit`) | greyed + strike-through/X |
| Alloy, added (`explicit` list or `delta.add`) | baseline-style icon in an "added" tint |
| Alloy, `delta.remove` | ghost icon with red X — distinct from `explicit`'s plain omission |

**Per-army grouping**: tinted by existing `Params::Army::armyColor`. Zero new PARAMS. A legend
strip lists army name/color.

**"Preview As" control**: `occupancy`/`keepAll` only make sense against a concrete composition,
so a scratch slot-pattern editor (reusing §3's toggle row) defaults to the scenario's own
pattern if Tier 1, else a synthesized composition satisfying the Tier-2 predicate. UI-session
scratch state only, never written back to the recipe.

**Interaction**:
- Left-drag a solid (explicit) spawn: moves it, writes `ScenarioArmySpawn`.
- Left-drag a hollow (inherited) spawn: **first drag materializes it** — seeds from baseline,
  flips `spawnsPolicy` to `Explicit`, continues the drag live. Operationalizes §4 at the point
  of highest leverage: touching the canvas can't leave a scenario silently inheriting.
- Right-click a baseline alloy → "Remove for this scenario". **Disabled + tooltipped, not
  silently no-op**, when `alloyMode == keepAll`.
- Right-click empty canvas near an army's territory → "Add Alloy Marker for [Army]".

**Mode entry/exit**: explicit opt-in toggle on the open scenario's detail panel, default off —
browsing the list must never hijack the canvas. Takes exclusive canvas-interaction ownership
while active; auto-exits when the panel closes or the toggle flips off.

## 7. The runtime Lua editor

New shared widget `LuaCodeEditorWidget_UI` (ImGuiColorTextEdit-backed, dark pastel Lua theme).
Follows "THE SPLIT" (`ColorSwatch_UI.cpp`): plain settings/state struct in the header,
third-party editor code isolated to the `.cpp`. Lives in the tab's collapsed "Runtime Script
(advanced)" section, editing `ScenarioSettings::runtimeScriptText` directly.

**Compile-only validation** — a narrow primitive `CompileLuaSource(text) -> {bSuccess,
errorLine, errorMessage}` wrapping `luaL_loadstring` and immediately discarding the chunk;
**never executes**. Exact layer/suffix flagged for ARCH. Runs on focus-loss and an explicit
"Validate Now" button — **not per-keystroke** (avoids thrashing an immediate-mode loop for a
file edited in bursts). Errors surface as an inline gutter marker (ImGuiColorTextEdit supports
this natively, parsed from Lua's `chunkname:line: message`) plus a one-line status bar.

**Export gate — a deliberate divergence from warn-never-block, named so it doesn't read as an
inconsistency**: a Lua syntax error is not a judgment call the designer might legitimately
override; the exported file is provably broken and the game cannot load it. Export is
**hard-disabled** while `CompileLuaSource` fails, tooltip pointing at the error.

**"Reset to bundled default"** — SanGen ships a canonical runtime template as a build resource;
button reverts the buffer, gated by `ConfirmDialog_UI` (destructive).

**Newer-bundled-than-local-edits** — tracked by comparing the buffer against the bundled text
at load time, **not** a version-number scheme. When the buffer has real local edits *and* the
bundled text has changed, show a non-blocking dismissible banner: *"A newer bundled Scenario
Script is available. Your current script has local edits."* → **[View Bundled Default]**
(read-only side panel, no auto-merge — Lua text merging is out of scope) / **[Keep My Version]**
/ **[Dismiss]**. Never modal.

## 8. Export flow UX

New `FilesTabAction::ExportScenarioScript`, a sibling action on the **existing Files tab** —
not a bespoke button on the Scenarios tab. This is how "must never block the normal map export"
is structurally guaranteed for free: every `FilesTabAction` is already independently
triggerable with its own success/failure path into the same `debugLog`.

`gameInstallRoot` modeled exactly like `ArmiesTabState::gamedataDirectory` /
`FilesTabState::exportFolderPath` — a machine-local path in `FilesTabState`, **not**
`Params::MapRecipe` (a recipe must stay portable; a game-install path is host-specific).
Persisted in app-level settings, asked once, remembered across maps.

Failure states:
- **Unset/invalid** — the action row relabels to "Locate Game Install…" (reusing
  `FilePathPicker_UI`'s browse seam) rather than a dead-end error. A redirect, not a failure.
- **Set but not a real install** (no `LJ/lua/maps`) — inline red validation, Export disabled,
  tooltip naming exactly what's missing.
- **Valid install, first export for this map** — not an error; auto-create the folder and log it.
- **Write/IO failure** — surfaces in the existing `debugLog`, no new UI.

Both gates reuse existing primitives: §4's mandatory-spawns `ConfirmDialog_UI` (warn,
proceed-allowed) and §7's compile check (hard block, no override).

## Flagged for other experts (nothing here is ratified)

**ARCH Expert**
1. New PARAMS family: `Scenario`, `ScenarioPatternEntry`, `ScenarioCountEntry`,
   `ScenarioCountPredicate`/`Clause`, `ScenarioSettings`, `ScenarioAlloyMode`,
   `ScenarioSpawnsPolicy`, `ScenarioArmySpawn`, `ScenarioArmyAlloys`; new `MapRecipe::scenarios`.
2. Placement of the pure `Matches` evaluator (recommend MATH, per `BuildSymmetryOrbit`).
3. New third-party deps: ImGuiColorTextEdit + embedded Lua (compile-only). SanGen links no Lua today.
4. New narrow primitive `CompileLuaSource` — layer/suffix undecided; stateless, side-effect-free,
   never executes Lua.
5. `ScenarioSpawnsPolicy` is a SanGen-authored intent flag with no Lua-data equivalent (it drives
   an exported *comment*) — confirm this is acceptable, same class as `Army::alias`/`armyColor`.

**Format Expert** — ✅ **all three ANSWERED 2026-08-21, kept for the audit trail:**
6. ~~Is `DEFAULT_SCENARIO` mandatory-always-present?~~ → **Mandatory, singleton.**
   `FindMatchingScenario` returns it unconditionally with no nil-guard, and `ApplyScenario`
   dereferences it immediately — absent, the first dereference throws. **But its *content* is
   correctly just "the `.sanmap` baseline wrapped"**: the live default is `area` = the map's own
   baked `PlayableArea`, `alloyMode = "occupancy"`, `navy = false`, and no `spawns` override at
   all. UI follow-up: seed a fresh `defaultScenario` from those values, and set its
   `spawnsPolicy` to `AcknowledgedInherit` (**not** `NotSet`) — inheriting the baseline is the
   *intended* Tier-3 behavior, so flagging it would be a false-positive nag on every map.
7. ~~Is `maxArmySlotCount` correctly decoupled from `recipe.armies.size()`?~~ → **Yes,
   permissively; see the ⚠️ superseded note in §1.** Now lives on `Params::Scenarios`, default 16
   (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10). May exceed the army count freely; below it is a loud export-time warning.
8. ~~Confirm `Scenario::name` is log-only, not a dictionary key.~~ → **Confirmed log-only.**
   Exhaustive grep of the live reference: both uses are `Log()` calls. TIER 1 matches on
   `.pattern`, TIER 2 on the predicate, TIER 3 is a bare singleton — **no table anywhere is keyed
   by name.** Duplicates break nothing functional; uniqueness stays a UX nicety, so this
   document's soft-uniqueness assumption (§1) holds. Note the only consumer surfaces through the
   F1 console, whose own reliability is flagged unresolved in `MODDING_SCRIPTING_SPEC.md` —
   which strengthens rather than weakens the ruling.

**IO Architecture Expert**
9. ~~Parameterized-render vs. literal round-trip~~ — **RESOLVED: parameterized render, export-only.**
10. Does `ScenarioSettings` (incl. `runtimeScriptText`) persist inside the `.sanmap`, or only via
    the script-tree export path? (This design assumes inside the `.sanmap`, for survive-reopen.)
11. `gameInstallRoot` storage location — confirm against how machine-local settings persist today.

**Generator Expert** — none. Touches no PROC stage; `recipe.scenarios` is pure pass-through,
same posture as `recipe.markers`.

## ❓ Open questions
- OR-of-clause-groups for Tier 2 — out of v1 scope; revisit if a real map needs it.
- Can Tier-1 slot-pattern length vary per scenario, vs. one shared `maxArmySlotCount`?
  (Assumed shared.)
- Should "Duplicate" work *across* tiers (promote a Tier-2 rule to a Tier-1 pattern)? Not
  designed; today it stays within its source tier.

## ⚠️ Risks
- The Scenario Edit Mode canvas pass depends on `STEP47_WorldScreenProjection_UI.md`
  (**DRAFTED**, not landed) for world↔screen math — sequence after Phase 1 of
  `SEQUENCE_PreviewOverlayLayering.md`, not before.

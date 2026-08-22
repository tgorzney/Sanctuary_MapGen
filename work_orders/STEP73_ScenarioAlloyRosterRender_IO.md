# STEP73 — `ScenarioAlloyRosterRender_IO`: deriving and rendering `ARMY_ID_TO_NAME` / `KNOWN_ALLOY_MARKERS`

**Layer:** IO. **Domain:** extends STEP70's Map Scenario Lua-rendering leg
(`ScenarioScript_DataLua_IO`) — **no new PARAMS field, no new `.sanmap` key.** One small additive
primitive to STEP63's `LuaTableWriter_IO.h`. **Sequence:** closes the blocker flagged in
`work_orders/STEP72_ScenarioRuntimeResource_IO.md`'s "Problems/gaps flagged" item 1, which is
itself `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 3's explicit scope note (`KNOWN_ALLOY_MARKERS`/`ARMY_ID_TO_NAME`'s
per-map rendering shape is "IO/Format-Expert-owned rendering detail, out of scope for this
ruling"). Depends on **STEP69** (`Params::Scenarios`/`MapRecipe::armies`), **STEP70**
(`ScenarioScript_DataLua_IO.cpp`/`.h` — the file this ticket edits), **STEP63**
(`LuaTableWriter_IO.h` — one primitive added). Consumed by **STEP72**'s already-shipped reader
contract — unchanged by this ticket, satisfied by it.

## 0. The ownership question, answered from evidence — no ratification needed

Both tables are **fully derivable** from data already in `Params::MapRecipe`. Neither needs a new
authored PARAMS field, a new `.sanmap` key, an ARCH ratification, or a Format Correction.

**`ARMY_ID_TO_NAME` (slot index → army name).** `Pandemonium Isthmus_data.lua` (the live
orchestrator) states it directly, lines 51–54:

> `playerInfo[i].armyID` is the same field `common/gameUtils.lua`'s `CreateArmies()` correlates
> against `mapStartSlotIndex` (**1 = ARMY_01, 2 = ARMY_02, ... alphabetically sorted army names**),
> ... confirmed real fields read directly from `common/gameUtils.lua:221-258`.

So: sort `MapRecipe::armies` by `Army::name` ascending, 1-based index = slot ID. `Army_PARAMS.h`'s
`Army` struct has **no `armyID`/slot field at all** (confirmed by reading it), consistent with this
being a *derived* mapping rather than stored data.

✅ **CONFIRMED BY THE HUMAN, 2026-08-21:** "the engine itself orders armies by alphabetizing, so
armies always need to be output as `ARMY_XX`." The alphabetical-sort rule is authoritative, not an
inherited guess — the ⚠️ flag previously on this point is retired.

### ⚠️ Load-bearing consequence — army names MUST be zero-padded two-digit `ARMY_XX`

Because slot order comes from a **string** sort, the zero padding is functional, not cosmetic:

| Names | Alphabetical sort → slot order | Correct? |
|---|---|---|
| `ARMY_01`, `ARMY_02`, … `ARMY_10` | 01, 02, … 10 | ✅ |
| `ARMY_1`, `ARMY_2`, `ARMY_10` | `ARMY_1`, `ARMY_10`, `ARMY_2` | ❌ slot 2 becomes ARMY_10 |

Unpadded names are correct up to 9 armies and **silently wrong from 10 onward** — the wrong army
spawns in the wrong lobby slot, its alloy markers get deleted for the wrong occupancy, with no
error anywhere. Since `maxArmySlotCount` defaults to 16 (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10), this is inside the
supported range, not a theoretical edge.

`Army_PARAMS.h` imposes **no format constraint on `Army::name`** — it is a free-form authored
string — so nothing currently prevents a SanGen user from producing `"Bob"` or `"Army 3"` and
silently corrupting slot assignment. The live `.sanmap` already uses the correct convention (its
`armies` dict is keyed `"ARMY_01"`/`"ARMY_02"`, verified), so this is a constraint the game's data
already follows and SanGen must not break.

**Required: an export-time validation warning** (same loud/non-blocking posture as STEP70 §2b's
`maxArmySlotCount` check, Constitution §6). When any `Army::name` in `recipe.armies` does not match
`ARMY_` followed by exactly two digits, warn by name:

> `SANGEN: army "<name>" does not follow the ARMY_XX convention (ARMY_ plus exactly two digits).
> The engine assigns lobby slots by ALPHABETICAL name order, so non-padded or non-conforming names
> silently map armies to the wrong slots once a map has 10 or more armies. Scenario spawn positions
> and alloy occupancy will be assigned to the wrong armies.`

**Never auto-rename, never auto-pad** — silently rewriting an authored name is the failure mode
Constitution §6 forbids in the other direction. Warn and export.

❓ **Placement is the same open question as STEP70 §2b's warning** — `BuildScenarioDataLuaText`
returns a bare `std::string` with nowhere to put diagnostics. Same recommended default: put it in
STEP71's `ScenarioScript_Export_IO`, which already owns a `ScenarioExportResult` with a `debugLog`.
Whichever home is chosen, both warnings should live together rather than split across layers.

**`KNOWN_ALLOY_MARKERS` (army name → its alloy marker names).** Cross-checked the live reference's
hand-authored table against its own `COUNT_SCENARIOS` block:

```
KNOWN_ALLOY_MARKERS.ARMY_01 = { "AlloyMarker_219", "AlloyMarker_237", "AlloyMarker_97" }
```

— **exactly** the marker set the `"1v1"` scenario's `alloys.ARMY_01` entry lists. Same exact match
for `ARMY_02` (`"1v1"`) and `ARMY_03`/`ARMY_04` (`"4human"`/`"1h3ai"`). The roster is nothing but
the **union, deduplicated, of every `(armyName, markerName)` pair appearing anywhere in the map's
own authored scenario data**.

`Params::ScenarioAlloyOverride`/`ScenarioAlloyRemoval` are the **only** place in the entire data
model an alloy marker name is ever associated with an army name (confirmed:
`MarkerInstance_PARAMS.h`'s `MarkerTransform`/`MarkerInstanceGroup` carry `name`/`transform`/`alias`
only — the `.sanmap`'s `markers.Alloys.transforms` dict truly has no army field). `Params::Scenarios`
already carries every such pair, so this is a pure computation over already-round-tripped data.

⚠️ **Flagged, not solved:** the alphabetical-sort claim is sourced from a comment describing
*external engine code* (`common/gameUtils.lua`) this pack cannot read directly — inherited, not
independently re-verified. Same posture as STEP72 §0's `MapUtils.GetMapName()` flag; see the
call-site comment (§2) and Open Question 1.

## Fix — three parts

### 1. New primitive: `AppendArrayOfQuotedStrings` (`src/io/LuaTableWriter_IO.h`, EDIT — additive)

Neither `AppendKeyValueLine` (one scalar) nor `AppendArrayOfTables` (array of `{ row }` tables)
covers "flat array of plain strings" — `KNOWN_ALLOY_MARKERS[armyName]` is the first caller of this
shape anywhere in the pack.

```cpp
// Renders `<indent>key = { "s1", "s2", ... },\n` on ONE line -- a flat array of raw strings, each
// independently escaped/quoted via QuotedLuaString. The scalar-array counterpart to
// AppendArrayOfTables (which wraps each row in `{ ... }`). Empty rawStrings still renders
// `key = {},\n` -- never omitted (Constitution §6, matches AppendArrayOfTables's own empty-table
// posture). First caller: KNOWN_ALLOY_MARKERS (ScenarioScript_DataLua_IO, STEP73).
inline void AppendArrayOfQuotedStrings(std::string& out, int indentLevel, const std::string& key,
                                        const std::vector<std::string>& rawStrings) {
    out += LuaIndent(indentLevel) + key + " = {";
    for (std::size_t i = 0; i < rawStrings.size(); ++i) {
        out += (i == 0 ? " " : ", ") + QuotedLuaString(rawStrings[i]);
    }
    out += rawStrings.empty() ? "}" : " }";
    out += ",\n";
}
```

Add matching coverage to `src/io/LuaTableWriter_IO_Test.cpp` (STEP63's existing file): empty vector
→ `"key = {},\n"`; one element; multiple elements comma-joined on one line; an element needing
escaping runs through `QuotedLuaString` correctly.

### 2. Two new private helpers + wiring (`src/io/ScenarioScript_DataLua_IO.cpp`, EDIT — STEP70's file)

`#include <algorithm>` added for `std::sort`/`std::find`.

```cpp
namespace {

using AlloyRosterEntry = std::pair<std::string, std::string>;  // (armyName, markerName)

// Every (armyName, markerName) pair a ScenarioBody's alloy fields reference -- alloys/alloysToAdd/
// alloysToRemove ALL carry both fields (`ARCH_15_05_ParamsScenariosType.md` §15.5), so all three contribute to the roster.
void CollectScenarioBodyAlloyRosterEntries(const Params::ScenarioBody& body,
                                            std::vector<AlloyRosterEntry>& outEntries) {
    for (const auto& row : body.alloys)         outEntries.emplace_back(row.armyName, row.markerName);
    for (const auto& row : body.alloysToAdd)    outEntries.emplace_back(row.armyName, row.markerName);
    for (const auto& row : body.alloysToRemove) outEntries.emplace_back(row.armyName, row.markerName);
}

// ⚠️ ATTENTION -- EXTERNAL ENGINE BEHAVIOR, NOT INDEPENDENTLY VERIFIED (STEP73 §0). mapStartSlotIndex
// (what player.armyID is matched against, common/gameUtils.lua's CreateArmies()) is claimed -- by a
// comment in the live reference's own _data.lua (lines 51-54) -- to assign 1..N to this map's
// authored armies IN ALPHABETICAL NAME ORDER. This pack cannot read gameUtils.lua directly; the
// claim is inherited from that comment, not re-derived. IF IN-GAME ARMY SLOT ASSIGNMENT EVER LOOKS
// WRONG (the wrong army spawns in the wrong lobby slot), THIS IS THE FIRST PLACE TO LOOK.
std::string BuildArmyIdToNameTable(const std::vector<Params::Army>& armies) {
    std::vector<std::string> sortedNames;
    sortedNames.reserve(armies.size());
    for (const auto& army : armies) sortedNames.push_back(army.name);
    std::sort(sortedNames.begin(), sortedNames.end());

    std::string out;
    OpenTable(out, 0, "ARMY_ID_TO_NAME");
    for (std::size_t i = 0; i < sortedNames.size(); ++i) {
        AppendKeyValueLine(out, 1, "[" + std::to_string(i + 1) + "]", QuotedLuaString(sortedNames[i]));
    }
    CloseTable(out, 0, false);
    return out;
}

// KNOWN_ALLOY_MARKERS is the union, per army, of every alloy marker name recipe.scenarios itself
// already references (STEP73 §0) -- NOT authored anywhere separately. Collected in a FIXED,
// deterministic order (pattern tier, then count tier -- each in its own vector order -- then the
// single default), matching BuildScenarioDataLuaText's own tier-emission order, so output is
// reproducible byte-for-byte from the same recipe with no staging container that could reorder
// (same discipline `ARCH_15_06_CountScenariosOrdering.md` §15.6 requires of CountScenarios itself).
std::string BuildKnownAlloyMarkersTable(const Params::Scenarios& scenarios) {
    std::vector<AlloyRosterEntry> allEntries;
    for (const auto& entry : scenarios.patternScenarios) CollectScenarioBodyAlloyRosterEntries(entry.body, allEntries);
    for (const auto& entry : scenarios.countScenarios)   CollectScenarioBodyAlloyRosterEntries(entry.body, allEntries);
    CollectScenarioBodyAlloyRosterEntries(scenarios.defaultScenario, allEntries);

    // Group by armyName (first-seen order), dedup markerName within each group (first-seen order).
    // Linear scan, not a hash map -- typical roster sizes are tiny, and this sidesteps any
    // container-iteration-order question outright.
    std::vector<std::string> armyOrder;
    std::vector<std::vector<std::string>> markersPerArmy;
    for (const auto& [armyName, markerName] : allEntries) {
        std::size_t armyIndex = 0;
        for (; armyIndex < armyOrder.size(); ++armyIndex) {
            if (armyOrder[armyIndex] == armyName) break;
        }
        if (armyIndex == armyOrder.size()) {
            armyOrder.push_back(armyName);
            markersPerArmy.emplace_back();
        }
        std::vector<std::string>& markers = markersPerArmy[armyIndex];
        if (std::find(markers.begin(), markers.end(), markerName) == markers.end()) {
            markers.push_back(markerName);
        }
    }

    std::string out;
    OpenTable(out, 0, "KNOWN_ALLOY_MARKERS");
    for (std::size_t i = 0; i < armyOrder.size(); ++i) {
        // Bracket-string key (["ARMY_01"] = {...}), never a bare identifier -- Army::name is a
        // free-form authored string (Army_PARAMS.h imposes no identifier-safety constraint on it),
        // so this is the one form guaranteed valid regardless of what characters the name contains.
        // Functionally identical for pairs()/[] lookup either way -- STEP72's runtime reads
        // KNOWN_ALLOY_MARKERS[armyName], which works against either syntax.
        AppendArrayOfQuotedStrings(out, 1, "[" + QuotedLuaString(armyOrder[i]) + "]", markersPerArmy[i]);
    }
    CloseTable(out, 0, false);
    return out;
}

}  // anonymous namespace
```

**Wiring into `BuildScenarioDataLuaText`** — inserted between item 3 (`MAX_ARMY_SLOT_COUNT`) and
item 4 (`BuildPatternScenariosTable`) in STEP70 §2's composition list:

```cpp
// 3b. ARMY_ID_TO_NAME -- derived from recipe.armies (STEP73 §0).
result += BuildArmyIdToNameTable(recipe.armies) + "\n";
// 3c. KNOWN_ALLOY_MARKERS -- derived from recipe.scenarios' own alloy rows (STEP73 §0).
result += BuildKnownAlloyMarkersTable(recipe.scenarios) + "\n";
```

Both are map-wide, non-tiered globals like `MAX_ARMY_SLOT_COUNT` — grouped with it, ahead of the
three scenario tables, matching STEP72's own reading order.

### 3. `src/io/ScenarioScript_DataLua_IO.h` header comment (EDIT)

Add a note beside `BuildScenarioDataLuaText`'s existing comment: the rendered output now also
includes `ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS`, both **derived, not authored** — a future reader
must not add a corresponding `Params::Scenarios` field "to match"; the derivation is the permanent
design, not a placeholder.

## Answers to the open specification items

- **Where the rendering lands.** `ScenarioScript_DataLua_IO` (STEP70's file) — the natural home;
  STEP72 itself named it as such.
- **Exact Lua output shape.** `ARMY_ID_TO_NAME = { [1] = "ARMY_01", [2] = "ARMY_02", ... }`;
  `KNOWN_ALLOY_MARKERS = { ["ARMY_01"] = { "AlloyMarker_219", ... }, ... }`. Both satisfy STEP72's
  reader verbatim (`pairs(ARMY_ID_TO_NAME)` yields `armyId, armyName`;
  `KNOWN_ALLOY_MARKERS[armyName]` is `ipairs`-able).
- **`bAlloyRosterAvailable` — recommend KEEP, do not retire.** After this ticket both globals are
  *always* rendered (even as empty tables) on every real export, so the guard should never fire
  going forward — but it remains cheap defense-in-depth against a stale pre-STEP73
  `_Scenarios_Data.lua`, a hand-corrupted file, or a future regression that drops the render call.
  Not this ticket's file to edit; flagged for whoever applies STEP72 that the guard's *expected*
  trigger rate becomes "should never fire," not "known gap."
- **`.sanmap` `Scenarios` section (Correction 17) — no matching key needed.** Both tables are
  render-time computations over data the section already carries in full
  (`Alloys`/`AlloysToAdd`/`AlloysToRemove`'s existing `ArmyName`/`MarkerName` fields, plus the
  top-level `armies` dict). **Not the Format Expert's action item** — answered here so it does not
  get independently re-opened.

## Files touched
- EDIT `src/io/LuaTableWriter_IO.h` — `AppendArrayOfQuotedStrings` (§1).
- EDIT `src/io/LuaTableWriter_IO_Test.cpp` — new primitive coverage.
- EDIT `src/io/ScenarioScript_DataLua_IO.h` — header comment note (§3).
- EDIT `src/io/ScenarioScript_DataLua_IO.cpp` — two new helpers + wiring (§2).
- EDIT `src/io/ScenarioScript_DataLua_IO_Test.cpp` — new acceptance assertions.
- No `CMakeLists.txt` change — no new translation unit, only edits to already-registered files.

## Backend policy
N/A — pure CPU-side string building, at most once per map export, on data already in memory. No
compute dispatch, no SIMD, no GPU handle.

## ARCH rules invoked
- `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 3 — the scope note this ticket fulfills verbatim.
- `ARCH_15_05_ParamsScenariosType.md` §15.5 — `ScenarioAlloyOverride`/`ScenarioAlloyRemoval`'s `armyName`/`markerName` fields,
  read as-is, never reinterpreted or extended.
- `ARCH_15_07_OwnershipSplit.md` §15.7 — IO owns rendering shape; confirmed by this ticket's finding that no PARAMS or
  format change is required.
- `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 — export-only; both tables are rendered, never parsed back.
- Constitution §6 — total, never-throwing render; empty inputs still render valid, non-omitted
  empty tables; the derivation only surfaces (army, marker) pairs the human already authored
  elsewhere in `recipe.scenarios` — it never fabricates a pairing.

## Explicit out-of-scope
- Any `Params::Scenarios`/`Army_PARAMS.h` shape change — none needed (confirmed, §0).
- Any `.sanmap` `Scenarios`-section key addition — none needed (confirmed above).
- STEP72's runtime reader logic and `bAlloyRosterAvailable` itself — unchanged; this ticket
  satisfies the existing contract without touching the file declaring the guard.
- STEP70's existing `MAX_ARMY_SLOT_COUNT`/tier-table rendering and its §2b export-time warning —
  untouched, orthogonal.

## Acceptance test
New assertions in `src/io/ScenarioScript_DataLua_IO_Test.cpp` (extends STEP70's suite):

1. **Alphabetical-order proof.** Armies authored out of order — `"ARMY_03"`, `"ARMY_01"`,
   `"ARMY_02"` — assert `find("[1] = \"ARMY_01\"") < find("[2] = \"ARMY_02\"") <
   find("[3] = \"ARMY_03\"")`.
2. **Global declaration.** Text contains `"ARMY_ID_TO_NAME = {"` and `"KNOWN_ALLOY_MARKERS = {"`;
   contains **neither** `"local ARMY_ID_TO_NAME"` nor `"local KNOWN_ALLOY_MARKERS"`.
3. **Empty armies list** → `"ARMY_ID_TO_NAME = {\n}"` present verbatim (empty but not omitted).
4. **Union-across-tiers, dedup proof.** `"AlloyMarker_X"` for `ARMY_01` appearing in one
   `PatternScenario`'s `alloys` **and** a different `CountScenario`'s `alloysToAdd` → the rendered
   `["ARMY_01"]` list contains it exactly once.
5. **`alloysToRemove`-only army included.** `ARMY_02` appearing **only** in
   `defaultScenario.alloysToRemove` still appears in `KNOWN_ALLOY_MARKERS` with that marker.
6. **Zero-reference army absent, not empty.** An authored army never mentioned in any scenario's
   alloy fields does **not** appear as `["ARMY_0N"] = {},` — proves the table lists only armies
   with ≥1 derived marker, per STEP72's `KNOWN_ALLOY_MARKERS[armyName] or {}` fallback contract.
7. **Bracket-string key form.** Rendered `["ARMY_01"] = { ... },`, never bare `ARMY_01 = {...},`.
8. **Live-reference parity self-check.** A fixture hand-transcribing Pandemonium Isthmus's real
   4-army roster (3 markers each, from its live `_Scenarios_Script.lua`) renders a
   `KNOWN_ALLOY_MARKERS`/`ARMY_ID_TO_NAME` semantically equivalent to the live hand-authored table
   (same army→marker-set membership, same 1..4 index order) — the closest proof short of a live
   game load.
9. **Position in file.** Both new globals render between `"MAX_ARMY_SLOT_COUNT"` and
   `"PATTERN_SCENARIOS"` (substring-position assertions, matching STEP70's test 9b style).
10. **Self-check via STEP65.** Extend STEP70's full-fixture `Sys::CheckLuaSyntax` test (test 10) to
    a fixture exercising both new tables — `bSucceeded == true`.
11. `AppendArrayOfQuotedStrings` coverage in `LuaTableWriter_IO_Test.cpp`: empty vector, one
    element, multiple elements, one element requiring escaping.
12. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green, both edited test
    targets pass.

## Verify
- `ScenarioScript_DataLua_IO_Test` and `LuaTableWriter_IO_Test` both pass with the new assertions.
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files broken.
- Confirm the rendered `_Scenarios_Data.lua`, run through `Sys::CheckLuaSyntax`, still succeeds with
  both new tables present.

## ❓ Open questions
1. The alphabetical-sort = `mapStartSlotIndex` claim (§0) is sourced from a comment describing
   external engine code this pack cannot read directly — not independently re-verified. First place
   to check if in-game army-slot assignment ever looks wrong; flagged at the call site.
2. Whether an authored army with zero derived alloy-marker references deserves its own export-time
   warning (symmetric to STEP70 §2b's) is left **unmandated** — a map using only
   `"keepAll"`/baked-default `"occupancy"` may legitimately never author an explicit alloy row for
   some army, and a mandatory warning risks becoming noise Constitution §6 does not call for. No
   warning is emitted; open for the human/ARCH to override if real exports show this guess wrong.
3. `resources/lua/SanGenScenarioRuntime.lua`'s "⚠️ GAP, FLAGGED NOT INVENTED" comment block
   (STEP72 Part 1) becomes stale prose once this ticket lands — needs a follow-up edit noting the
   gap is closed. That file belongs to STEP72; flagged for whoever applies it.

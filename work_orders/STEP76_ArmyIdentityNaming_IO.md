# STEP76 — `ARMY_XX` engine identity vs. `displayName`: SanGen owns the army key outright

**Layer:** PARAMS + IO (+ two UI edits). **Domain:** `Params::Army`, the `.sanmap` `armies`
dictionary key, and the `markers["Spawn"].transforms` key references that mirror it.
**Sequence:** closes the defect STEP73 §0 flagged as a warning-only mitigation, and reconciles
STEP74's flagged positional assumption (see the STEP74 amendment appended by this ticket).
Depends on nothing undone. **Blocks nothing, but everything army-keyed is silently wrong until it
lands** — scenarios (STEP69/70/73/74), spawn markers, and lobby slot assignment all key off the
string this ticket takes ownership of.

**⚠️ THE HUMAN'S RULING IS SETTLED LAW FOR THIS TICKET.** Points 1–5 in §0 below were ruled
2026-08-21 and are not reopened by the Coder, by a reviewer, or by a later ticket. Only §4's
migration policy carries a `⚠️ ASSUMPTION` flag, and it is flagged in place.

---

## Root problem — verified live, not inherited

Three independent readings of the tree confirm the same two-line defect:

1. **`src/ui/ArmiesTab_UI.h:77`** —
   ```cpp
   inline std::string NextArmyName(int armyCount) { return NextUniqueLabel("Army", armyCount); }
   ```
   `NextUniqueLabel` (`src/ui/UniqueNameList_UI.h:47-50`) returns
   `baseLabel + "_" + std::to_string(existingCount)`. **The literal names SanGen actually mints
   are `Army_0`, `Army_1`, `Army_2`, … — underscore-separated and ZERO-based**, not `Army1`/`Army2`
   as earlier sessions reported. Correcting the record matters for §4: the migration must not be
   written against a legacy spelling that never shipped.
2. **`src/io/MapExporter_Armies_IO.cpp:75`** — `armies[army.name] = armyJson;`. The authored string
   becomes the `.sanmap` JSON key verbatim. Nothing between the text box
   (`src/ui/ArmiesTab_UI.cpp:83`, `DrawTextInput("Name", army.name, nameRules)`) and this line
   normalizes, validates, or even inspects it.
3. **`src/params/Army_PARAMS.h:44`** — `std::string name;` carries no format constraint of any
   kind. A user can type `"Bob"`, `"army 3"`, or a single space, and it ships.

Real shipped maps use `ARMY_01` … `ARMY_06` (Pandemonium Isthmus, verified). Three things break:

- **Lobby slots.** `common/gameUtils.lua`'s `CreateArmies()` sorts army names **alphabetically** to
  derive `mapStartSlotIndex` (STEP73 §0, human-confirmed). Because the sort is on the *string*, an
  unpadded roster is correct up to 9 armies and **silently wrong from 10 onward** — maps support 16
  slots, so this is inside the real range. `Army_0 … Army_10` sorts `Army_0, Army_1, Army_10,
  Army_2, …`.
- **Spawn markers.** `markers["Spawn"].transforms` is keyed by army name. A mismatch means the
  spawn transform never resolves and the army gets no commander, with no error anywhere.
- **The entire scenario system** keys off `armyName` (`ScenarioSpawn`/`ScenarioAlloyOverride`/
  `ScenarioAlloyRemoval`, `SANMAP_FORMAT_SPEC` Correction 17; `ARMY_ID_TO_NAME`/
  `KNOWN_ALLOY_MARKERS`, STEP73).

**Origin:** `work_orders/STEP20_ArmiesTab_UI_Wiring.md` explicitly left army naming as "a mechanical
Coder choice", so the Coder reused the house `Area`/`NewArea` seed pattern. This is not a Coder
error — it is a work-order that declined to rule on something load-bearing. STEP76 rules on it.

## 0. The ruling (binding — do not re-litigate)

1. The **engine-facing army identity is ALWAYS `ARMY_XX`** — the literal prefix `ARMY_`, then the
   1-based roster position zero-padded to **at least two digits** (16 slots supported; two digits
   covers the full range, and the "at least" clause exists only so a future >99-army ceiling widens
   cleanly rather than silently truncating). It is **auto-generated and maintained by SanGen**.
2. It is **NEVER human-settable.** No text box, no override, no import path, no "advanced" escape
   hatch. A user cannot type it, edit it, or override it.
3. The human-authored army name becomes **DISPLAY-ONLY** — a SanGen-internal organization label.
   It **must never reach the engine-facing `armies` JSON key.**
4. **Ordering is the point:** alphabetical sort of the `ARMY_XX` set MUST equal roster order. This
   is the invariant every acceptance test in §7 exists to protect.
5. Migration policy — see **§4**, flagged `⚠️ ASSUMPTION` in place.

**Consequence for STEP73 §0's warning.** STEP73 specified a loud, non-blocking export-time warning
that names the offending armies and **never auto-renames**. That posture was correct *while
`Army::name` was human-authored* — silently rewriting a string a designer typed is the
Constitution §6 failure mode in the other direction. Ruling 1 changes the premise: SanGen now owns
the identity outright, so **auto-generation IS the correct behaviour** and no warning is needed for
the normal path. The never-silently-discard principle does not evaporate — it **transfers to the
DISPLAY label** (§4), which is the thing a human actually authored. STEP73's warning text survives
only as §3's export-time *assertion* backstop, which should fire zero times in a healthy build.

---

## 1. PARAMS shape — `src/params/Army_PARAMS.h` (EDIT)

**Which existing field keeps which role — stated exactly, because getting this backwards is the
one way to make the blast radius enormous:**

- **`Army::name` KEEPS its role as the folded-in `armies[key]` dictionary key.** It does not change
  meaning, does not change type, does not move. What changes is *who writes it*: it becomes
  **machine-owned**, never bound to a text input. It now always holds `ARMY_XX`.
- **The display label is a NEW field**, `displayName`.

This direction is not a preference — it is what ARCH_01_08_ParamsFieldNamingByKind.md §1.8 already ratified: *"A
`Dictionary<string, X>` becomes `std::vector<X>` with the dictionary key folded in as a `name`
field on `X`."* `Army::name` **is** the folded dictionary key by binding law, and every other
folded-key type in the tree spells it `name` (`MapArea`, `UnitGroup`, `UnitTransform`,
`MarkerTransform`, `MarkerInstanceGroup`). Repurposing `name` as the display label would make
`Army` the single type in the codebase where `name` is *not* the dictionary key.

It is also the minimum-blast-radius choice. Every existing consumer of `army.name` is *already*
consuming the engine identity and stays correct with **zero edits**:
`MapExporter_Armies_IO.cpp:75`, STEP73's `BuildArmyIdToNameTable` (sorts `Army::name` ascending —
now provably correct rather than warned-about), the STEP49 marker army-picker convention
(`SANMAP_FORMAT_SPEC` ~line 872), and every `armyName` field in Correction 17.

```cpp
struct Army {
    // ENGINE IDENTITY — `ARMY_XX`, the folded-in `armies[key]` (ARCH_01_08_ParamsFieldNamingByKind.md §1.8). MACHINE-OWNED as of
    // STEP76: minted and re-minted by AssignArmyIdentities (Sanmap_ArmyIdentity_IO.h) from this
    // army's 1-based roster position, so an ALPHABETICAL sort of the roster's names equals roster
    // order — the property `common/gameUtils.lua`'s CreateArmies() relies on to assign lobby
    // slots. NEVER bound to a text input, NEVER user-settable, NEVER read from a `.sanmap` as
    // authoritative (the importer re-mints it; see MapImporter_ArmyIdentityNormalize_IO).
    // To rename what a human sees, edit `displayName` — not this.
    std::string name;

    // DISPLAY ONLY — the human-authored organization label. SanGen-internal; it reaches the
    // `.sanmap` as the lowerCamelCase `displayName` sibling (Correction 18) and is NEVER the
    // `armies` dictionary key. Free-form, may be empty, NOT required to be unique (two armies may
    // both be called "North" — they are still ARMY_01 and ARMY_02 to the engine).
    std::string displayName;

    Faction     faction = Faction::Chosen;
    float       alloys  = 500.0f;      // SANMAP_FORMAT_SPEC's confirmed export default
    float       energy  = 500.0f;      // same
    std::vector<UnitGroup> groups;     // recursive pre-placed unit tree

    float       armyColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // SanGen-added, Correction 11
    std::string alias;                                       // SanGen-added, Correction 11
};
```

**Naming justification (ARCH_01_08_ParamsFieldNamingByKind.md §1.8, ARCH_01_01_LiteralNames.md §1.1).** `displayName` is SanGen-added data with no format
spelling to inherit, so §1.8's "format's own spelling by default" clause has nothing to defer to;
it falls to §1.1 — literal, fully spelled, no abbreviation. `displayName`, never `dispName` /
`label` / `friendlyName`. It is a plain `std::string`, so no `b` prefix applies.

**`displayName` is a NEW field and does NOT reuse `alias`.** `alias` is already-shipped, already-
ratified (Correction 11), already human-editable, and already consumed by the STEP49 marker
army-picker convention. Folding the migrated legacy name into `alias` would **overwrite a label a
human authored** on any map that already uses `alias` — precisely the discard ruling 5 forbids.
Two distinct concepts, two distinct fields.

**File-size check (ARCH_01_05_FileSizeCeilings.md §1.5):** `Army_PARAMS.h` is **56 lines** today; this edit lands it near 70.
Well inside the soft 100 ceiling. No exception needed.

---

## 2. FORMAT ANSWER — where the display name lives in the `.sanmap`

### The answer, plainly

**`displayName` is a lowerCamelCase field merged directly into the format-native `armies[<ARMY_XX>]`
object, alongside `armyColor` and `alias`. It is NOT a new top-level section, and it is NOT a side
table.**

```jsonc
"armies": {
  "ARMY_01": {
    "faction": 0,
    "alloys": 500.0,
    "energy": 500.0,
    "groups": { },
    "armyColor": { "r": 1.0, "g": 0.2, "b": 0.2, "a": 1.0 },
    "alias": "",
    "displayName": "North Ridge"      // <-- STEP76
  }
}
```

This follows ARCH_01_06_SanmapKeyCasing.md §1.6 ("a field merged into an existing format-native collection stays
lowerCamelCase to match its siblings — `armyColor`, `alias`, never `\"Army Color\"`") and §1.8's
direct-injection branch ("a format-native object gains a small SanGen field by direct injection
when the field is genuinely novel information with no competing home"). A per-army display label
is a novel scalar with no format-native home — the same test `armyColor`, `alias`, and
`layerIndex` all already passed.

### The tolerance question, reasoned from ground truth — and where the evidence stops

The prompt's live-verified finding is correct but is about a **different seam** than the one that
governs here. Two distinct consumers read a `.sanmap`, and they answer this question differently:

**(a) The Lua engine — settled, no risk.** `LoadMapData` builds `GameInfo.MapData` from an explicit
**top-level** whitelist (`props, decals, areas, armies, markers, chains, groups`) and silently drops
unrecognized top-level sections (live-verified 2026-08-20, `ARCH_15_MapScenarioSystem.md` §15.3/§15.9,
restated in `SANMAP_FORMAT_SPEC` Correction 17). `armies` **is** whitelisted, so the whole army
dictionary passes through as-is. The whitelist has no per-army-object equivalent, and Lua tables are
open — an unrecognized key inside an army table is simply an inert extra field nothing indexes.
**Definitively safe on this path.**

**(b) The C# `EM.Map` deserializer — safe, but by inference plus production evidence, NOT by direct
proof. Stating the limit explicitly, per my charter's no-guessing rule.**

What the authoritative ground truth in `D:\Projects\Sanctuary\Sanmap File Format\` **does**
establish (read in full: `SanMap.cs`, `SanMap.Types.cs`, `Types.cs`):

- `EM.Map.Army` (`SanMap.Types.cs:78-85`) declares **exactly four** fields — `faction`, `alloys`,
  `energy`, `groups`. There is no `displayName` member for a key to bind to.
- The class carries **no `[JsonObject(MissingMemberHandling = ...)]` attribute** and **no
  `[JsonExtensionData]` catch-all**. It is a bare `[Serializable]` POCO. It therefore does not
  opt into strict member binding at the type level, and Newtonsoft.Json's **default
  `MissingMemberHandling` is `Ignore`** — unrecognized JSON properties are skipped silently.
- The same shape holds one level up: `SanMap` itself is a fixed-field class with no extension data,
  which is exactly consistent with the observed "parsed then dropped" behaviour for unknown
  top-level sections.

What it **does not** establish: `grep -rn "JsonSerializerSettings|MissingMemberHandling|
JsonExtensionData|DeserializeObject|JsonConvert"` across all five files returns only the
`StringEnumConverter` on `skyboxIntensityMode` (`SanMap.cs:132`) and a **commented-out** `LoadJson`
call (`SanMap.cs:165`). **The deserialization call site and its `JsonSerializerSettings` are not in
this folder.** A caller *could* in principle pass `MissingMemberHandling.Error`. I cannot rule that
out from the vendored ground truth, and I will not pretend otherwise.

**What closes the gap is production evidence, not inference.** `armyColor` and `alias` are
**already** written into `armies[key]` today — `MapExporter_Armies_IO.cpp:72-74`, ratified as
`SANMAP_FORMAT_SPEC` Correction 11, read back at `MapImporter_Armies_IO.cpp:106-107`. Two
unrecognized keys already ship inside every SanGen-exported army object and maps load. Whatever the
serializer settings are, they demonstrably tolerate extra keys in exactly this position.
**`displayName` is the third instance of a settled, in-production pattern — it introduces no new
risk class.** If `MissingMemberHandling.Error` were in play, SanGen's army export would already be
broken today, for reasons having nothing to do with this ticket.

### The safest alternative, named but NOT recommended

If the human ever wants the residual risk driven to literally zero: move the labels to a top-level
SanGen-owned PascalCase section, `ArmyDisplayNames`, a flat `{ "ARMY_01": "North Ridge", ... }` map
— which `LoadMapData`'s whitelist is **proven** to drop, and which the C# `SanMap` class has no
member for either. **Do not build this by default.** It splits one entity's data across two places
for a risk already disproven in production, contradicts §1.8's direct-injection branch, and creates
a second key-synchronization surface (`ArmyDisplayNames` keys would have to be re-mapped on every
re-mint in §3 — a whole extra failure mode invented to avoid a non-failure). Recorded here only so
the fallback is on the shelf, pre-designed, if the human ever calls for it.

### Required follow-up (NOT part of this ticket's file set)

`SANMAP_FORMAT_SPEC.md` needs a **Correction 18** recording `displayName` as a Correction-11-class
merge into `armies[key]`, and recording ruling 1 (`ARMY_XX` is machine-owned format truth, not a
convention). The Format Expert owns that text; this ticket may write only its own file plus the
STEP74 amendment, so the Correction is called out here rather than landed here. **The Coder does
not need Correction 18 to execute STEP76** — this document is self-contained on the wire shape.

---

## 3. Generation and enforcement — where `ARMY_XX` is minted

### 3a. The single canonical helper — NEW `src/io/Sanmap_ArmyIdentity_IO.h`

One pure, headless, testable header. **Zero includes beyond `<string>`/`<vector>`/`<cstddef>` and
`../params/Army_PARAMS.h`; no imgui, no nlohmann.**

**Why IO and not UI or PARAMS.** The identity *is* the `.sanmap` dictionary key — format truth,
which is the IO/BRIDGE layer's own subject. It cannot live in UI, because the export-time guard
(§3c) is IO and the Constitution forbids IO depending upward. `UI → IO` is established practice
(`FilesTab_UI.h:25-26` includes `MapExporter_IO.h`/`MapImporter_IO.h`), so the UI call sites in §3b
reach it legally.

```cpp
// The `.sanmap` `armies` dictionary key for a 1-based roster position. `ARMY_` + at least two
// digits, zero-padded, so an ALPHABETICAL sort of a roster's keys equals roster order — the
// property common/gameUtils.lua's CreateArmies() relies on to assign lobby slots (STEP73 §0).
// The padding is FUNCTIONAL, not cosmetic: ARMY_1/ARMY_2/ARMY_10 sorts 1, 10, 2.
// Positions past 99 widen to three digits rather than truncating; the sort stays correct within
// each width band and no supported map reaches it (16 slots).
inline std::string ArmyIdentityForRosterPosition(int oneBasedPosition);

// True when `name` is exactly `ARMY_` followed by two-or-more digits, all digits. Used by the
// export guard (§3c) and by tests. Does NOT check that the number matches a position.
inline bool IsArmyIdentityWellFormed(const std::string& name);

// THE enforcement point. Rewrites every `armies[i].name` to ArmyIdentityForRosterPosition(i + 1),
// unconditionally and idempotently. Reports whether anything moved, so a UI caller can skip
// downstream work on a no-op frame. Total: an empty roster is a no-op, never a crash.
inline bool AssignArmyIdentities(std::vector<Params::Army>& armies);
```

`AssignArmyIdentities` is deliberately **positional and total** — it does not inspect the current
name, does not pattern-match, and does not try to preserve anything. That is what makes it correct
across add / delete / reorder with one call and no per-operation logic: after ANY roster mutation,
run it, and ruling 4's invariant holds by construction. Each function is far under the 40-line cap;
the header lands ~45 lines, well inside ARCH_01_05_FileSizeCeilings.md §1.5.

### 3b. Keeping it correct across add / delete / reorder — `src/ui/ArmiesTab_UI.{h,cpp}` (EDIT)

The tab already has the exact hook. `ArmiesTab_UI.cpp:127-129`:

```cpp
// The export keys armies by NAME, so the duplicate repair runs on the frames a name settled —
// not every frame, which would rename a row mid-typing (STEP20 ruling #5).
if (bArmiesMoved) MakeNamesUnique(recipe.armies);
```

- **Replace `MakeNamesUnique(recipe.armies)` with `AssignArmyIdentities(recipe.armies)`**, and
  update the comment: the export keys armies by a name SanGen now owns, so this re-mints rather
  than de-duplicates. **`MakeNamesUnique<T>` itself is NOT deleted** — `areas` still uses it and it
  is a shared template (`UniqueNameList_UI.h`).
- **Widen the trigger.** `bArmiesMoved` is currently set by Add and by a *name commit*. It must now
  also be set on **Reorder and Delete**, which today return through `ApplyArmyListSignal` without
  touching it (`ArmiesTab_UI.cpp:114-116`) — a reorder changes every position and therefore every
  identity. Simplest correct form: call `AssignArmyIdentities` whenever the roster was added to OR
  a `DraggableListSignal` fired, i.e. `if (bArmiesMoved || signal.bHasSignal())`. Hoist `signal`
  out of the `if (DrawSectionBegin(...))` block, or set `bArmiesMoved` inside it — either is fine;
  what matters is that no roster mutation can reach the end of the frame un-re-minted.
  (`AssignArmyIdentities` is idempotent, so an over-eager call is free. Under-calling is the only
  real failure.)
- **`DrawArmySettings` (`ArmiesTab_UI.cpp:78-104`) — the text input rebinds to `displayName`.**
  ```cpp
  TextInputRules displayNameRules;
  displayNameRules.maximumLength = 48;
  displayNameRules.bAllowEmpty   = true;    // display-only; blank is legal, ArmyRowLabel falls back
  DrawTextInput("Name", army.displayName, displayNameRules);
  ```
  The `bAllowEmpty = false` / `fallbackText = "Army"` rules that guarded the old `name` are no
  longer needed to protect the export key — the key is machine-minted and can never be blank.
  **`DrawArmySettings` no longer needs to report `bNameCommitted`** (nothing downstream repairs a
  display name); simplify its return to `void` and drop the `|| bArmiesMoved` at
  `ArmiesTab_UI.cpp:121`, or keep the signature and always return `false` — the Coder picks
  whichever keeps the diff smaller, but **must not** leave a stale comment claiming the name commit
  drives a uniqueness repair.
- **Show the identity, read-only, right below it** — so the user always knows what the engine calls
  this army and never wonders where `ARMY_XX` went:
  ```cpp
  ImGui::TextDisabled("Engine ID: %s", army.name.c_str());   // machine-owned, ruling 2 — no input
  ```
  **Do not** add a text input, a right-click rename, a context menu, or any other path to `name`.
  Ruling 2 is absolute.
- **`ArmyRowLabel` (`ArmiesTab_UI.h:70-72`) shows the display name, falling back to the identity.**
  Never empty (Constitution §6):
  ```cpp
  inline const char* ArmyRowLabel(const Params::Army& army) {
      if (!army.displayName.empty()) return army.displayName.c_str();
      return army.name.empty() ? "Army" : army.name.c_str();
  }
  ```
- **`NextArmyName` (`ArmiesTab_UI.h:77`) is repointed, not deleted.** It now seeds the *display*
  name of a fresh row (`ArmiesTab_UI.cpp:66` becomes `army.displayName = NextArmyName(...)`), which
  keeps the row label non-empty on Add. Rename it **`NextArmyDisplayName`** so no future reader
  thinks it still touches the export key, and update its comment — it no longer feeds "the shared
  uniqueness repair", because display names are not required to be unique.

**File-size check (ARCH_01_05_FileSizeCeilings.md §1.5):** `ArmiesTab_UI.h` is **130 lines** — already past the soft 100
ceiling, under the hard 150. These edits are net-neutral (±3 lines). **The new helper deliberately
goes in its own header partly for this reason.** The Coder must not grow `ArmiesTab_UI.h` past 150
lines; if a change would, split rather than ratchet.

### 3c. The export-time guard — `src/io/MapExporter_Armies_IO.cpp` (EDIT)

By §3b the identity is always correct before export. The guard exists for the paths §3b does not
cover: a recipe built by a test, by a future headless/CLI export, or by any caller that mutates
`recipe.armies` without going through the tab.

In `BuildArmiesJson` (`MapExporter_Armies_IO.cpp:63-78`), **before** the write loop:

```cpp
// SanGen owns the army key outright (STEP76 ruling 1). Re-mint defensively: any caller that
// mutated recipe.armies without going through ArmiesTab_UI is corrected here rather than shipping
// a map whose lobby slots silently mis-assign. Idempotent — a no-op on the normal path.
AssignArmyIdentities(const_cast<std::vector<Params::Army>&>(recipe.armies));
```

⚠️ **`BuildArmiesJson` takes `const Params::MapRecipe&`, and a `const_cast` in the exporter is not
acceptable.** Two clean options; **the Coder picks (i) unless it forces a wider signature churn
than (ii):**

  - **(i) Preferred — re-mint one level up, where the recipe is already non-const.** Call
    `AssignArmyIdentities(recipe.armies)` once in the export entry point before document assembly
    (`MapExporter_DocumentAssembly_IO` / `MapExporter::BuildSanmapJsonText`'s caller), and keep
    `BuildArmiesJson` pure and const. Cleanest: one call, one place, no casts.
  - **(ii) Fallback — verify-and-warn instead of correct.** Keep `BuildArmiesJson` const and have it
    check `IsArmyIdentityWellFormed(army.name)` plus positional agreement, warning per offending
    army without rewriting. Reuse STEP73 §0's wording shape:

    > `SANGEN: army "<name>" (roster position <n>) is not the expected engine identity "<ARMY_XX>".
    > The engine assigns lobby slots by ALPHABETICAL name order, so a non-conforming name silently
    > maps armies to the wrong slots once a map has 10 or more armies. Scenario spawn positions and
    > alloy occupancy will be assigned to the wrong armies.`

    **Loud, non-blocking, names every offending army, never refuses the export** — the same posture
    Constitution §6 mandates and STEP73 §0/STEP70 §2b already use.

Under (ii) the warning is a genuine "this should never happen" assertion: in a healthy build it
fires zero times, because §3b and §4 both re-mint. **Do not implement both (i) and (ii)** — one
correction point, not two.

❓ **Same open placement question STEP73 §0 and STEP70 §2b already carry:** the export path's
diagnostics home. `MapExportResult` (`src/io/MapExporter_IO.h`) is the natural sink if it has a
warn channel; if it does not, this ticket does **not** invent one. Choosing (i) sidesteps the
question entirely — a further reason to prefer it.

---

## 4. Import-side normalization + spawn-marker key rewrite

### ⚠️ ASSUMPTION — confirm with human

**This whole section rests on a policy the consolidation session inferred from the human's own
stated workflow ("import the map, then export to get a proper result"), not on a direct ruling.**
It is flagged here, in place, so it is visible at implementation time. Everything in §0–§3 is
settled; §4 is not. If the human rules differently, §4 changes and §0–§3 stand unaffected.

The assumed policy:
1. On import, legacy names normalize to `ARMY_XX`.
2. The original authored string is **PRESERVED as `displayName`** — never discarded.
3. Spawn-marker key references (`markers["Spawn"].transforms[...].name`) are rewritten to match.
4. A **loud, non-blocking** report names exactly what was renamed.

### 4a. The normalizer — NEW `src/io/MapImporter_ArmyIdentityNormalize_IO.{h,cpp}`

**Its own file, not folded into `MapImporter_Armies_IO.cpp`** (which is **125 lines** — adding this
would breach the hard 150 ceiling, ARCH_01_05_FileSizeCeilings.md §1.5), and not a `<Domain>_Migrate_V<N>_IO` unit.

**Why not a migration unit.** `IO_MIGRATION_SPEC`'s `<Domain>_Migrate_V<N>_IO` law addresses
*version-correlated* reshaping. This defect is **not version-correlated**: SanGen shipped
`Army_0`-style names *under `SanGenVersion` 3*, the current version. A version-gated migration would
skip exactly the documents that need fixing. It must run **unconditionally on every import**, which
is a different mechanism. It is also idempotent, so running it on an already-clean v3 document costs
one string compare per army and reports nothing. (The structural file-shape call here is the IO
Architecture Expert's surface; the *"must not be version-gated"* half is format truth and is mine.
Flagging the split rather than silently deciding both.)

```cpp
void NormalizeArmyIdentities(Params::MapRecipe& outRecipe, MapImportResult& result);
```

Algorithm — **positional and total**, exactly mirroring §3a so import and UI can never disagree:

1. For each `armies[i]`, compute `expected = ArmyIdentityForRosterPosition(i + 1)`.
2. If `armies[i].name == expected`, **do nothing and report nothing** (the healthy path).
3. Otherwise:
   - Record the pair `(oldName, expected)` for step 4 and the report.
   - **If `armies[i].displayName` is EMPTY, set it to the old `name`.** This is the never-discard
     rule. **If `displayName` is already non-empty, LEAVE IT ALONE** — it is a label the human
     authored deliberately, and clobbering it with a machine-minted `Army_3` would be the exact
     discard ruling 5 forbids. Report the old identity in the log either way, so nothing is lost
     silently even in the leave-alone branch.
   - Set `armies[i].name = expected`.
4. **Rewrite spawn-marker key references.** For the `Params::MarkerInstanceGroup` in
   `outRecipe.markers` whose `name == "Spawn"` (`recipe.markers` is
   `std::vector<MarkerInstanceGroup>`, `MapRecipe_PARAMS.h:101`; the group name is the folded outer
   dict key, `MarkerInstance_PARAMS.h:24`), walk `transforms` and for every entry whose `name`
   equals a recorded `oldName`, set it to the paired `expected`.
   - Build the old→new mapping **first, across the whole roster, then apply it in one pass.** Do
     **not** rename army-by-army-then-markers, which can chain a rename through two mappings
     (`Army_1 → ARMY_02`, then a later army renames `ARMY_02 → ARMY_03`, corrupting a marker that
     was already correct).
   - Match on `MarkerTransform::name` only. Do **not** touch `MarkerTransform::alias`.
   - The `"Spawn"` group absent, empty, or a marker matching no army: **not an error, not a
     warning here.** A missing spawn marker is **STEP82**'s subject
     (`work_orders/STEP82_ArmySpawnMarkerValidation_IO.md`, concurrently authored) — cite it and
     move on. This ticket rewrites keys it can match and is silent about ones it cannot.
5. Report (Constitution §6 — loud, non-blocking, never a refusal):
   ```cpp
   result.Warn("SANGEN: army \"" + oldName + "\" renamed to engine identity \"" + expected +
               "\" (SanGen owns the armies key; the engine assigns lobby slots by alphabetical "
               "name order). The original name was preserved as this army's display name.");
   ```
   One line per renamed army, **naming both strings** — the human must be able to read the import
   log and see exactly what moved. When `displayName` was already occupied, swap the final sentence
   for `The original name was NOT preserved as a display name because this army already had one ("<displayName>").`
   Then one summary line naming the spawn-marker count:
   ```cpp
   result.Log("SANGEN: rewrote " + std::to_string(rewrittenCount) +
              " Spawn marker key reference(s) to match the new army identities.");
   ```

### 4b. Wiring — `src/io/MapImporter_ParseDocument_IO.cpp` (EDIT)

`ParseEntityDomainsJson` (line 61) already takes `MapImportResult& result`, so **no signature
change is needed** and `ReadArmiesJson`'s own signature (`MapImporter_Recipe_IO.h:55`, no
`MapImportResult`) stays untouched. Add **one line at the END** of `ParseEntityDomainsJson`, after
`ReadStratumGenerationSettingsJson(...)`:

```cpp
NormalizeArmyIdentities(outRecipe, result);
```

⚠️ **Placement is load-bearing and must be at the end, not next to `ReadArmiesJson`.** The
normalizer rewrites BOTH `recipe.armies` and `recipe.markers`, and `ReadArmiesJson` (line 63) runs
*before* `ReadMarkersJson` (line 65) — calling it early would rewrite armies while `recipe.markers`
was still empty, silently orphaning every spawn marker. Extend that function's header comment to
say so, in the same voice as its existing ordering notes about `*Groups` readers and
`StratumGenerationSettings`.

### 4c. ⚠️ Ordering consequence for legacy 10+-army maps — read before implementing

`ParseSanmapJsonText` parses into **`nlohmann::json`** (`MapImporter_ParseDocument_IO.cpp:114-116`),
not `ordered_json`. `nlohmann::json` is `std::map`-backed, so **object keys iterate in sorted
order**, and `ReadNameKeyedObject`'s `parent[key].items()` loop
(`MapImporter_Armies_IO.cpp:33-38`) therefore fills `recipe.armies` **already sorted alphabetically
by the on-disk key**.

That is a fortunate accident and it is load-bearing: re-minting by roster position after import
**exactly preserves whatever slot assignment the engine was already giving that map.** For a
well-formed `ARMY_01…ARMY_06` map it is a perfect no-op.

**But for a legacy `Army_0 … Army_10` map, alphabetical order is `Army_0, Army_1, Army_10,
Army_2, …` — the corrupted order the engine was already using.** So the migration preserves the
map's *actual observed in-game behaviour*, not the author's presumed intent. It does not silently
"fix" the ordering to what the author probably meant, because SanGen cannot know that, and guessing
would move armies between slots without telling anyone.

This is the honest choice, and the preserved `displayName` values make it recoverable: the user sees
`Army_10` sitting at ARMY_03 in the roster and can drag it where they want. **Do not add
number-extracting "smart" re-sorting.** If the human wants the intent-preserving behaviour instead,
that is a ruling to make, not a Coder inference — it belongs to this section's `⚠️ ASSUMPTION`.

---

## 5. Layer, accuracy class, ARCH rules, backend policy

- **Layer:** PARAMS + IO/BRIDGE, plus two UI edits (§3b).
- **Accuracy class:** **Exact.** The identity string must reproduce the engine's classified
  decision (alphabetical sort → `mapStartSlotIndex`). There is no tolerance and no visual path.
- **Backend policy:** N/A — pure CPU-side string work, at most once per import/export and once per
  roster-mutating UI frame. No compute dispatch, no SIMD, no GPU handle; does not touch
  `Dispatch_SYS`.
- **ARCH rules invoked:** §1.1 (literal, fully-spelled names — `displayName`), §1.5 (file-size
  ceilings — the reason §3a and §4a get their own files), §1.6 (`.sanmap` key casing —
  `displayName` is lowerCamelCase because it merges into a format-native collection), §1.8 (folded
  dictionary key is `name`; direct field injection for a novel scalar), Constitution §6 (validate
  input; loud non-blocking reporting; never silently discard authored data).

## 6. Out of scope — do not build, do not stub

- **Export-time "Army has no matching Spawn marker" validation — `STEP82_ArmySpawnMarkerValidation_IO`,
  concurrently authored.** §4a step 4 deliberately stays silent when a spawn marker matches no
  army; that check is STEP82's, not this ticket's. Do not add it here, even though the loop is
  right there.
- Any change to `ARMY_ID_TO_NAME` / `KnownAlloyMarkers` rendering (**STEP73**). STEP73's derivation
  sorts `Army::name` ascending and becomes *correct by construction* once this lands — it needs no
  edit, and must not receive one.
- Any Scenarios-tab UI work (**STEP74**), beyond the amendment this ticket appends to that file.
- `SANMAP_FORMAT_SPEC` Correction 18 (§2) — Format Expert's follow-up, not a Coder file.
- Any `<Domain>_Migrate_V<N>_IO` file, any `Sanmap_MigrationManifest_IO` edit, any
  `SanGenVersion` bump. §4a explains why none applies; a Coder must not create
  `ArmyIdentity_Migrate_V3_IO` "to be safe".
- Uniqueness enforcement on `displayName`. It is display-only and duplicates are legal.
- Any user-facing path to edit `Army::name`. Ruling 2.

## 7. Acceptance tests

**NEW `src/io/Sanmap_ArmyIdentity_IO_Test.cpp`** (registered as `ArmyIdentity_IO_Test`):

1. **⚠️ THE load-bearing test — ruling 4.** Build a 16-army roster, run `AssignArmyIdentities`, copy
   the names into a `std::vector<std::string>`, `std::sort` it, and assert the sorted order is
   **index-for-index identical** to roster order. Then repeat at **9, 10, and 11 armies** — 10 is
   the exact size where the old `Army_N` scheme broke. Assert `[9] == "ARMY_10"`.
2. `ArmyIdentityForRosterPosition(1) == "ARMY_01"`, `(9) == "ARMY_09"`, `(10) == "ARMY_10"`,
   `(16) == "ARMY_16"`, `(100) == "ARMY_100"` (widens, never truncates).
3. **Idempotence:** `AssignArmyIdentities` twice in a row — second call returns `false` and changes
   nothing.
4. **Reorder:** 3 armies, swap positions 0 and 2, re-mint — assert `armies[0].name == "ARMY_01"`
   and that `displayName` **followed the row** (the army that was `ARMY_03` is now `ARMY_01` and
   still carries its own display label). This proves identity is positional and display is not.
5. **Delete:** 4 armies, erase index 1, re-mint — names are a contiguous `ARMY_01..ARMY_03` with no
   gap, and the surviving display labels are the right three in the right order.
6. `IsArmyIdentityWellFormed`: `"ARMY_01"`/`"ARMY_100"` true; `"ARMY_1"`, `"Army_01"`, `"ARMY01"`,
   `"ARMY_0X"`, `"Bob"`, `""` all false.
7. `AssignArmyIdentities` on an **empty** roster: returns `false`, does not crash.

**NEW `src/io/MapImporter_ArmyIdentityNormalize_IO_Test.cpp`** (registered as
`ArmyIdentityNormalize_IO_Test`):

8. **Legacy round-trip, end to end.** Hand-build a `.sanmap` document text whose `armies` keys are
   `"Army_0"`/`"Army_1"`/`"Army_2"` and whose `markers.Spawn.transforms` is keyed by the same three
   strings. `ParseSanmapJsonText` it, then assert: names are `ARMY_01`/`ARMY_02`/`ARMY_03`;
   `displayName` values are `Army_0`/`Army_1`/`Army_2`; **every Spawn transform name now matches an
   army name**; `result.warningCount == 3`; and the log text contains both `"Army_0"` and
   `"ARMY_01"` (substring checks — proves the report names both strings, not just one).
9. **Never-discard, occupied branch.** Same fixture but one army already carries
   `"displayName": "North Ridge"`. Assert `displayName` is still `"North Ridge"` (NOT overwritten
   with `"Army_1"`), the name still normalized, and the log names the old identity anyway.
10. **Already-clean document is a silent no-op.** `armies` keyed `ARMY_01`/`ARMY_02` in order →
    zero warnings, zero renames, `displayName` untouched (stays empty), Spawn keys untouched.
11. **Spawn group absent entirely** → armies still normalize, zero crashes, **no warning about the
    missing group** (that is STEP82's, test #11 asserts its absence explicitly so a future Coder
    doesn't "helpfully" add it here).
12. **Chained-rename hazard.** A fixture whose on-disk keys are `"ARMY_02"`, `"ARMY_03"` (so
    position 1 becomes `ARMY_01` — i.e. an old name and a new name collide across rows). Assert
    every Spawn transform ends on the correct army: proves the build-mapping-then-apply-once
    discipline of §4a step 4, which a naive per-army loop fails.
13. **Full `.sanmap` round-trip parity.** `BuildSanmapJsonText` a recipe with 3 armies carrying
    non-empty `displayName`, `ParseSanmapJsonText` it back, assert both `name` and `displayName`
    survive exactly, and inspect the raw JSON to confirm the keys are `ARMY_01`… and that
    `displayName` sits **inside** each army object as a sibling of `armyColor`/`alias` (§2).

**EDIT `src/ui/ArmiesTab_UI_Test.cpp`:** the existing tests set `armies[i].name` directly to
`"Alpha"`/`"Bravo"`/`"Charlie"` (lines 94-96, 99, 109-118, 171-172, 187-190) and assert on it. Those
strings are now **display** labels — retarget those assertions to `displayName`, keeping the
existing `unitRules.armyIndex` checks intact (they are positional and unaffected). Add one new case:
after `ApplyArmyListSignal` performs a Reorder, `armies[i].name` is still `ARMY_01`/`ARMY_02`/… in
order. **These are the only pre-existing test edits this ticket may make**, and they are
mechanical retargeting, not weakening.

## 8. Files touched

- EDIT `src/params/Army_PARAMS.h` — `displayName` member + the two role comments (§1)
- NEW  `src/io/Sanmap_ArmyIdentity_IO.h` — the three pure helpers (§3a)
- EDIT `src/io/MapExporter_Armies_IO.cpp` — `displayName` write + the §3c guard (option (i) may
  instead put the re-mint in `MapExporter_DocumentAssembly_IO.cpp` / the export entry point)
- EDIT `src/io/MapImporter_Armies_IO.cpp` — one `ReadJsonText(armyJson, "displayName", army.displayName);`
  beside the existing `alias` read (line 107). **Nothing else in this file changes.**
- NEW  `src/io/MapImporter_ArmyIdentityNormalize_IO.h` / `.cpp` (§4a)
- EDIT `src/io/MapImporter_ParseDocument_IO.cpp` — one call at the END of `ParseEntityDomainsJson`
  + the ordering comment (§4b)
- EDIT `src/ui/ArmiesTab_UI.h` — `ArmyRowLabel`, `NextArmyName` → `NextArmyDisplayName` (§3b)
- EDIT `src/ui/ArmiesTab_UI.cpp` — re-mint call + trigger widening, `DrawArmySettings` rebind,
  read-only Engine ID line (§3b)
- NEW  `src/io/Sanmap_ArmyIdentity_IO_Test.cpp`
- NEW  `src/io/MapImporter_ArmyIdentityNormalize_IO_Test.cpp`
- EDIT `src/ui/ArmiesTab_UI_Test.cpp` — mechanical retarget to `displayName` + one new case (§7)
- EDIT `CMakeLists.txt` — two new test registrations

`src/params/*.h` and `src/io/*.cpp`/`*.h` are `GLOB_RECURSE`'d into `SANGEN_V2_SOURCES`
(`CMakeLists.txt:142-150`) — the new sources need no explicit list entry, only the test targets do.

## 9. Verify

- Both new tests pass; full solo rebuild + `ctest -C Debug` at 100%, with **no pre-existing test
  file edited except `ArmiesTab_UI_Test.cpp`** (§7, and only as the mechanical retarget described).
- `grep -n "army.name" src/ui/` returns **only** the read-only `Engine ID` display line and the
  `ArmyRowLabel` fallback — **no** `DrawTextInput` anywhere near it. This is the grep that proves
  ruling 2 held.
- `grep -n "NormalizeArmyIdentities" src/io/MapImporter_ParseDocument_IO.cpp` shows the call at the
  **end** of `ParseEntityDomainsJson`, after `ReadStratumGenerationSettingsJson` — the placement
  most likely to be silently "tidied" next to `ReadArmiesJson`, where it would break (§4b).
- `wc -l src/ui/ArmiesTab_UI.h` is **≤ 150** (ARCH_01_05_FileSizeCeilings.md §1.5 hard ceiling; it starts at 130).

---

## Amendment log

*(none yet — new ticket)*

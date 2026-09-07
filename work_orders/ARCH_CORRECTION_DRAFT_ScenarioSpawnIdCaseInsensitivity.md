# DRAFT — RATIFIED 2026-09-05 (with a correction of its own) — correction to ARCH_15_12's reserved-namespace enforcement

**Status: RATIFIED, kept as historical record only — read `ARCH_15_12_ScenarioSpawnIdentity.md`'s
Validator section as the authoritative text, not this file.** The diagnosis below (§15.12's original
negation of `IsArmyIdentityWellFormed` is case-sensitive and misses `"army_01"`/`"Army_01"`) landed
correctly. **This draft's own proposed replacement text, however, contains two factual errors the
ratified text did NOT carry forward — do not treat "The correction" section below as accurate:**
1. It specifies case-folding to **uppercase** (`std::toupper`). The actually-ratified predicate,
   `Io::ResemblesArmyIdentityCaseInsensitive`, folds to **lowercase** (`std::tolower`) instead.
2. It cites the fold idiom as "the same idiom `ScenarioNameValidation_IO.cpp`'s reserved-name check
   **already uses**, STEP251 §1b point 2" — `ScenarioNameValidation_IO.cpp` does not exist yet (STEP251
   is an unratified/unbuilt work order, confirmed by `ls src/io/ScenarioNameValidation_IO.cpp` finding
   nothing), so "already uses" is false; and STEP251 §1b point 2's own text specifies `std::tolower`
   (matching `TemplateSourceScan_IO.cpp:37`'s existing extension-check idiom), not `std::toupper` as
   this draft claims.

Read `ARCH_15_12_ScenarioSpawnIdentity.md`'s Validator section directly for the correct, ratified
wording (lowercase fold, cites `TemplateSourceScan_IO.cpp:37` as the precedent, correctly notes
STEP251 §1b point 2 will use the same `tolower` idiom once built). The body below is preserved
unedited as what was actually proposed at the time — do not silently rewrite it to match the
ratified text; read it as history, and follow §15.12, not this file, when implementing.

**Status (original): UNRATIFIED DRAFT**, produced 2026-09-05. Corrects one clause of
`ARCH_15_12_ScenarioSpawnIdentity.md`, itself ratified 2026-09-04/05 but **not yet implemented**
anywhere in `src/`/`resources/` (confirmed by grep — zero hits for `spawnPoints`/`spawnIds`/
`SCENARIO_SPAWN_POINTS` outside this ARCH text and its own draft). Nothing in
`ARCH_15_12_ScenarioSpawnIdentity.md` or any other file has been touched to produce this draft.

## The defect

§15.12's "RULED" paragraph states: *"Enforced by negating the exact existing well-formedness
predicate already ratified for `ARMY_XX` minting — `Io::IsArmyIdentityWellFormed`
(`src/io/Sanmap_ArmyIdentity_IO.h:34-43`, STEP76) — never a second, reinvented charset rule."*

Read directly, `IsArmyIdentityWellFormed` is **case-sensitive**:

```cpp
inline bool IsArmyIdentityWellFormed(const std::string& name) {
    static const std::string prefix = "ARMY_";
    if (name.compare(0, prefix.size(), prefix) != 0) return false;   // exact-case compare
    ...
```

Negating it as §15.12 specifies means a pool `spawnId` of `"army_01"` or `"Army_01"` is **not**
rejected — `IsArmyIdentityWellFormed("army_01")` returns `false` (wrong case, doesn't match the
literal `"ARMY_"` prefix), so the negation (`invalid if IsArmyIdentityWellFormed(...) == true`)
never fires. The human's own original intent for this rule (per this session's direct
confirmation) was an explicitly **case-insensitive** reserved-namespace exclusion. As worded,
§15.12 does not deliver that.

## Why the fix is a NEW check, not a change to `IsArmyIdentityWellFormed` itself

`IsArmyIdentityWellFormed`'s existing case-sensitivity is **correct for its own, different
purpose** (STEP76 army-name minting / well-formedness auditing) and must not change:
`GameInfo.MapData.markers.Spawn.transforms[armyName]` is an exact-case string key the live game
engine reads directly — an army genuinely named `army_01` (lowercase) would NOT resolve against
any real `ARMY_01` transform, so correctly flagging it as not-well-formed (and eligible for
rename-to-canonical) is exactly right for that call site. Folding case there would let a broken
lowercase army name through as "already fine," a regression in an unrelated, already-correct
predicate — the opposite of what's needed.

The two checks serve different questions and must stay separate:
- `IsArmyIdentityWellFormed(name)` — "does `name` work as a real army identity the engine can key
  on?" (case-sensitive; unchanged by this correction)
- A new, narrower check — "does `name` merely *resemble* `ARMY_XX`, closely enough to confuse a
  human author, regardless of case?" (case-insensitive; new, scoped only to spawn-pool `spawnId`
  validation)

## The correction

Replace §15.12's Validator bullet "RULED — `ARMY_XX`-shaped strings are a reserved namespace..."
with:

```markdown
- **RULED (corrected 2026-09-05) — `ARMY_XX`-shaped strings are a reserved namespace, forbidden
  as pool `spawnId`s, case-insensitively.** Enforced by a NEW, dedicated predicate,
  `Io::ResemblesArmyIdentityCaseInsensitive(name)` — NOT `Io::IsArmyIdentityWellFormed`, which
  stays case-sensitive and unchanged for its own STEP76 army-minting purpose (an army genuinely
  named `army_01` must still be correctly flagged as malformed there, since the engine's
  `markers.Spawn.transforms` lookup is exact-case; folding case in that predicate would be a
  regression, not a fix). The new predicate case-folds `name` to uppercase (ASCII `std::toupper`,
  the same idiom `ScenarioNameValidation_IO.cpp`'s reserved-name check already uses, STEP251 §1b
  point 2) and applies the identical prefix-plus-two-or-more-digits shape
  `IsArmyIdentityWellFormed` checks, on the folded string. A `Scenarios::spawnPoints` row is
  invalid if `ResemblesArmyIdentityCaseInsensitive(row.spawnId)` is true — same enforcement
  points/posture as every sibling rule in this file (UI-authoring time and export time, the
  offending row's write refused/excluded with a named error).
```

## Scope for implementation (pointer only, not a work order)

When §15.12/§15.13 are eventually implemented (no work order exists for them yet — this is
ratified, unbuilt law, same as `ARCH_15_04`/`05` were before `STEP251`), the implementing work
order should add `Io::ResemblesArmyIdentityCaseInsensitive` alongside `Io::IsArmyIdentityWellFormed`
in `src/io/Sanmap_ArmyIdentity_IO.h` (same file, same family, sibling function — not a new file;
this is a two-line addition, well under any size-ceiling concern) and use it exclusively for the
`Scenarios::spawnPoints` validator, never substituting it for `IsArmyIdentityWellFormed`'s existing
call sites.

## Resolves

A correctness gap in `ARCH_15_12`'s own already-ratified text, caught before any implementation
existed to inherit the bug — confirmed via direct code read of `Io::IsArmyIdentityWellFormed`
(`src/io/Sanmap_ArmyIdentity_IO.h:34-43`), not a hypothetical.

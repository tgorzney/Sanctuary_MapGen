# Work-Order — Step 40D: `SlopeDefaults`/`StratumGenerationSettings` V2→V3 migrations

*Constitution §7. Executor: SanGen Coder. Depends on `STEP40A`. IO Architecture Expert consult.
These two migrations MUST be reviewed and shipped together — they share a read source and write
into overlapping array indices; splitting them across separate diffs would make the coupling much
harder to verify.*

## Root problem
Each `mapGeneratorData.Stratums[i]` legacy entry (PascalCase dialect, `SANMAP_FORMAT_SPEC.md`
Correction 5/12) carries 8 slope-gate fields with no current top-level home: `SlopeGateEnabled`,
`MinimumSlopeDegrees`, `MaximumSlopeDegrees`, `SlopeFeatherDegreesLow`, `SlopeFeatherDegreesHigh`,
`UseSmoothstep`, `InvertSlopeGate`, `SlopeGateStrength`. Two things need to happen with this same
data: (1) synthesize ONE global `SlopeDefaults` from all strata's values, and (2) relocate each
stratum's own 8 values verbatim into the new per-stratum `StratumGenerationSettings[i]` array.
Both migrations read the SAME source (`Stratums[i]`, read-only, neither deletes it — the whole
step's shared `legacyKeysToDelete` handles that after both run) and both write into related
destinations that must not clobber each other.

**Known limitation, not this ticket's to fix:** the one real file checked this session
(`World_Domination.sanmap`) uses a THIRD, unrelated `Stratums[]` dialect (lowerCamelCase keys —
`absorptionRate`, `maskMode`, etc.) that doesn't match this PascalCase shape at all — these
migrations are grounded in the spec's own documented old shape (Corrections 5/12), correct for a
file that actually used it, but will be safe no-ops against that specific real file's dialect
(neither migration will find any of its expected keys). This is a separate, already-flagged
importer gap (`ReadStrataSettingsJson` doesn't recognize the third dialect either) — not fixed by
either of these migrations, and not this ticket's scope.

## Ruled by this ticket (IO Architecture Expert)

**`SlopeDefaults_Migrate_V2`'s synthesis rule — exact, not left to the coder to invent:**
- **The 3 booleans** (`SlopeGateEnabled`, `UseSmoothstep`, `InvertSlopeGate`): **mode** (most
  common value across all `Stratums[i]` entries that HAVE the key). **Tie → `false`** (matches
  each field's own `SlopeDefaults_PARAMS.h` hardcoded default).
- **The 5 floats** (`MinimumSlopeDegrees`, `MaximumSlopeDegrees`, `SlopeFeatherDegreesLow/High`,
  `SlopeGateStrength`): **arithmetic mean** across all `Stratums[i]` entries that have the key.
- **N = 0** (no `Stratums` array, or empty): the migration performs NO write to `SlopeDefaults` at
  all — `Params::SlopeDefaults()`'s own PARAMS defaults (already what `ReadSlopeDefaultsJson`
  falls back to when the key is absent) are the correct outcome.
- **Per-stratum `bSlopeUseGlobal`**: exact equality across all 8 fields against the synthesized
  global — `false` (keep using this stratum's own values) if ANY of the 8 disagree exactly with
  the synthesized default, `true` only if all 8 agree exactly. This is deliberate: it guarantees
  the migration never silently changes a stratum's resolved slope behavior. In real multi-stratum
  data this will correctly land `false` for nearly every stratum — expected, not a bug to fuzz
  away with a tolerance.

**`SlopeDefaults_Migrate_V2` runs BEFORE `StratumGenerationSettings_Migrate_V2`** — genuinely
load-bearing (both write into related `StratumGenerationSettings[i]` destinations; see below).

**`StratumGenerationSettings_Migrate_V2`**: relocates each stratum's same 8 fields verbatim into
`document["StratumGenerationSettings"][i]`, index-aligned with `stratumLayers[9]`, padded to
exactly 9 entries if `Stratums[]` has fewer. Soil physics (6 other `StratumGenerationSettings`
fields) has no legacy source anywhere — left at PARAMS defaults, not an omission.

**Additive-write discipline — the single most important implementation detail:** `SlopeDefaults_
Migrate_V2` sets `StratumGenerationSettings[i]["SlopeUseGlobal"]` for `i` in `[0,
Stratums.size())`. `StratumGenerationSettings_Migrate_V2` (running second) must ADDITIVELY set its
own 8 keys onto each per-index object — NEVER `document["StratumGenerationSettings"] =
newArray` wholesale-replace, which would silently discard the sibling's `SlopeUseGlobal` write.
For the padding indices beyond `Stratums.size()` (which `SlopeDefaults_Migrate_V2` never touches),
use `DefaultIfMissing(entry, "SlopeUseGlobal", true)` — never a raw overwrite — so every one of
the 9 final entries ends up with a `SlopeUseGlobal` key without ever clobbering a real computed
value for an in-range index.

## Target files
- New `src/io/SlopeDefaults_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.
- New `src/io/StratumGenerationSettings_Migrate_V2_IO.h/.cpp` + `_Test.cpp` — this test MUST
  include at least one case that runs BOTH migrations in sequence against the same document and
  asserts neither's writes clobber the other's (the additive-write discipline is the actual thing
  under test, not just each migration in isolation).

## Explicit out-of-scope
- Wiring into the manifest, ordering enforcement at the manifest level — `STEP40F`.
- Fixing `ReadStrataSettingsJson`'s inability to recognize the third real-world Stratums dialect —
  separate, unflagged-until-now bug, not this ticket.
- Any other migration — separate tickets.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. `SlopeDefaults_Migrate_V2` against a multi-stratum fixture produces the correct mode (booleans)
   and mean (floats) synthesis, exactly per the rules above — include a boolean tie case (confirms
   tie→false) and a float mean case with non-trivial values (confirms real averaging, not
   first-value).
2. `SlopeDefaults_Migrate_V2` against an empty/absent `Stratums` array performs NO write to
   `SlopeDefaults` at all.
3. `bSlopeUseGlobal` lands `false` for a stratum whose values genuinely differ from the synthesized
   default, `true` for one that exactly matches (construct a fixture where one stratum's 8 values
   are deliberately set to equal the expected synthesized mean/mode).
4. `StratumGenerationSettings_Migrate_V2` alone relocates all 8 fields correctly, index-aligned,
   padded to exactly 9 entries.
5. Running both migrations in sequence (the mandatory joint test) proves neither clobbers the
   other's writes to `StratumGenerationSettings[i]` — this is the ticket's actual point.
6. Full `SanGenV2` build stays clean; every existing test continues to pass.

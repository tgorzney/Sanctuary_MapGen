# Execution Plan — parallel batch + sequential order for the unbuilt backlog

*Checked and re-checked repeatedly on 2026-08-22 — see §0 for the full record (9 numbered
findings across 2 rounds: an already-shipped ticket, a wave-placement error, 2 real compile
blockers, 3 more compile/test gaps, a stale citation, 3 amendments that were themselves
incomplete on first fix, and a 3-way dependency gap). Every round found something; treat this
document as accurate as of its last edit, not as guaranteed final. Companion to
`IMPLEMENTATION_STATUS.md`.*

**Scope: 42 real unbuilt tickets** — `STEP26A`, `STEP26B`, `STEP46`–`STEP97` minus `STEP95`
(shipped this session) and minus `STEP46` (found already shipped during this pass — see §0).
`STEP96`/`STEP97` are excluded from both the batch and the sequence below; neither is
dispatchable yet (STEP96 needs unwritten tickets 85–91; STEP97 needs STEP51+60+66 landed *and*
an open ARCH ruling).

## 0. What the triple-check found and fixed

The first pass of this document (same-day, earlier) grouped tickets into 6 loosely-sequenced
"waves" based on spot-checking only the file-overlaps that looked suspicious. Re-verifying
*every* ticket in the intended first parallel wave — both for self-sufficiency against the
current shipped tree, and for real dependencies, not just claimed ones — found:

1. **STEP46 is already fully implemented.** The exact code it describes (`bNeedsTexelReadback`
   gating) ships today, landed in commit `18c5154`, predating this whole consolidation effort.
   Moved to `work_orders/shipped/`; it never belonged in the backlog count.
2. **STEP54 was wrongly placed in the first parallel wave despite a hard dependency on STEP51**
   ("cannot be dispatched until STEP51 lands `overlayLayers`") — an outright placement error in
   the first pass, not a subtle judgment call. Fixed by excluding it from the parallel batch.
3. **STEP50 and STEP51 will not compile against the current tree at all**, a hidden dependency
   neither ticket's own dependency-claim caught. Both reference `Params::MarkerRuleLayer`/
   `recipe.markerRuleLayers`, a type that doesn't exist until `STEP66` creates it. Both tickets
   cite a "STEP79 confirmation" as if it resolves this — it only validates a *numbering
   assumption* about that future type, not that the type exists yet. Excluded from the parallel
   batch; both now sequenced right after STEP66.
4. **STEP60 silently required `Params::SymmetrySetting`**, a struct that doesn't exist anywhere
   in `src/` and that STEP60's own "Files touched" list never creates — a real compile blocker.
   **Fixed**: amended `STEP60_MarkerInstanceLayer_PARAMS.md` §0 to add the struct definition
   inline (it's also independently specified, byte-identical, in STEP66 — noted so whichever
   lands first defines it once).
5. **STEP68's own code snippet was misleading** — it showed a `layerIndex` field "from STEP60"
   as if already present, but STEP60 hasn't landed. **Fixed**: amended to show the correct
   3-field current struct and add only `symmetryGroupIdentifier`.
6. **STEP72's own test suite will not compile in isolation** — it has a self-declared test-only
   dependency on STEP65 and STEP70, but STEP70 is nowhere near ready (needs STEP63+STEP69 first)
   and won't be reachable from STEP72's isolated worktree. **Fixed**: amended acceptance test
   items 6(a)/6(d) to use a hardcoded literal + a deferred TODO instead of including headers that
   don't exist yet.
7. **STEP69 cited a `SANMAP_FORMAT_SPEC.md` Correction 17 that doesn't exist in the live spec
   file** (despite other docs asserting it landed). Not a compile blocker — the ticket already
   reproduces the full shape inline — but would send a coder on a dead-end search. **Fixed**:
   amended to say so explicitly and point at the ticket's own inline tables instead.

**Second pass (same day)** — re-ran the same rigorous check on the 22-ticket *sequential* list
(which had only gotten the lighter original inventory treatment, not the full grep-against-`src/`
audit), plus a fresh skeptical re-read of the 4 amendments above to confirm they didn't just
relocate their own problem. Found:

8. **3 of the 4 amendments above were themselves incomplete** — each fixed the section it touched
   but left stale, contradictory text elsewhere in the same file. STEP68's acceptance test still
   asserted `layerIndex` as a guaranteed wire sibling; STEP69 still quoted "Correction 17" as a
   real external document 15+ times including one fabricated-sounding verbatim quote; STEP72's
   Backend Policy section still described a `Sys::CheckLuaSyntax` call the corrected test no
   longer makes. **All three fixed properly this time**, swept for every stale reference, not
   just the first one found.
9. **STEP53 has a real 3-way gap**: it assumes STEP52 bundles in STEP58's footprint-table wiring
   ("bundled under this same umbrella per this ticket's own brief"), but STEP52's own
   out-of-scope section explicitly declines to do that, and STEP58's own out-of-scope section
   also explicitly declines, calling it "STEP51's or STEP52's job." No ticket anywhere actually
   wires `Io::WorldFootprintSizeTable` into `Application`. **Fixed**: added a new §0 to STEP53
   itself (the actual consumer) with the missing wiring, mirroring STEP52's own
   `IconAtlasPairingLookup` pattern; added STEP58 as a stated prerequisite.

**Third pass (same day)** — checked the two edits above (§0.8's amendments, §0.9's STEP53 fix)
independently, since fixing something is exactly where the prior round found gaps. Found:

10. **STEP53's own §0 wiring was itself architecturally wrong** — it had the draw pass call
    `Application::WorldFootprintSizeTable()` directly, but `MapCanvas` (where the draw pass
    lives) has no `Application` reference anywhere, confirmed against real `src/`. This codebase
    already ratified the correct pattern for exactly this case (STEP48's own "RESOLVED — ARCH
    ruling": push-in setter/pointer, same mechanism as `SetPreviewComposite`). **Fixed**:
    `MapCanvas` now gets a `SetWorldFootprintSizeTable()` pointer setter, wired once in
    `WireCallbacks()`, matching the ticket's own pre-existing (and correct) "Files touched" line
    for `MapCanvas_UI.h` that this newer text had briefly contradicted. Also swept and fixed 2
    smaller leftovers in the same file: a "§8 below" reference to a section that doesn't exist
    (it's item 8 inside §1), and a "four prerequisites" header left over from before STEP58 was
    added as the 5th. STEP53's own "Files touched" list ends up with **4** newly-touched files
    (`Application_AssetBridge_UI.h`, `Application_Assets_UI.cpp`, `Application_UI.h`,
    `Application_UI.cpp`), not 3 as originally logged here.

**Fourth pass (same day)** — re-verified STEP53's fix once more fresh (clean, all 5 checks
passed), and specifically hunted the same bug *class* (code assuming a direct call/read path to
another UI object that doesn't actually exist) across the 9 other UI-heavy tickets in both lists
(STEP54, 55, 74, 77, 78, 80, 81, 83, 94). None found — every cross-object access in those 9
resolves to a self-member, a function parameter, or a correctly-specified push-in setter. One
trivial wording fix in STEP94 (cited an unlanded precedent as "already" existing rather than "per
its specified shape"). This pass found no new compile blockers or dependency gaps — a first for
this document, suggesting convergence.

Everything below reflects the corrected picture, not the original claims.

## 1. How "no conflicts" is being enforced

Two different things can go wrong when tickets run at the same time, and they need different
fixes:

- **Blocking dependency** — ticket A needs a type/function/field only ticket B produces. This is
  fatal if A and B run at the same time with no coordination: A's coder can't write correct code,
  full stop. **This is the constraint enforced for the parallel batch below — zero tolerance,
  verified per-ticket against the *current shipped tree*, not against each other's promises.**
- **Same-file edit** — two tickets touch the same file (sometimes the same file *and* the same
  function). This does not block either agent if each one works in its own isolated git worktree
  starting from the same clean commit — it only becomes a problem when the two finished branches
  get merged together afterward, and it's fixed by a normal rebase/merge step, not by blocking
  dispatch. **Recommendation: dispatch every ticket in the parallel batch via `Agent` with
  `isolation: "worktree"`.** All same-file touches found below are annotated with a recommended
  merge order for the integration step after the batch finishes.

## 2. THE PARALLEL BATCH — 18 tickets, dispatch simultaneously

Every ticket below has **zero dependency on any other ticket in this backlog** (verified against
current `src/`, not taken on the ticket's own word), and the 3 that had real compile blockers
have been amended (§0). Hand these to 18 individual coder agents now, each in its own isolated
worktree.

`STEP26A · STEP47 · STEP49 · STEP52 · STEP55 · STEP56 · STEP58 · STEP60 (amended) · STEP62 ·
STEP63 · STEP64 · STEP65 · STEP66 · STEP68 (amended) · STEP69 (amended) · STEP72 (amended) ·
STEP76 · STEP84 (Scope A only — Scope B needs a `SANMAP_FORMAT_SPEC` Correction that is not yet
ratified; the ticket already self-gates this, but tell the dispatched coder explicitly)`

### Merge-order plan for the integration step after all 18 finish

None of these are blocking — every group below is a same-file housekeeping merge, verified
low-risk (disjoint or clearly-adjacent edits), not a redesign:

| Files | Tickets | Merge order / note |
|---|---|---|
| `Application_UI.h` | STEP52 (@~L71), STEP64 (@~L124) | Either order — disjoint accessor/member insertions, 50+ lines apart. |
| `PropInstance_PARAMS.h`, `MapExporter_Props_IO.cpp`, `MapImporter_Props_IO.cpp` | STEP56, STEP62 | STEP56 first (larger surface), then STEP62 — different structs/functions entirely, verified zero real overlap. |
| `Symmetry_PARAMS.h` | STEP60, STEP66 | Both add the **identical** `SymmetrySetting` struct (verbatim, by design — see §0.4). Keep one copy, delete the duplicate definition; both tickets' consumers are unaffected either way. |
| `MapRecipe_PARAMS.h` | STEP60 (`markerLayers` @~L107), STEP66 (renames `markerRules`→`markerRuleLayers` @~L56), STEP69 (`scenarios` @~L101) | Any order — three disjoint regions on one struct. |
| `MapExporter_DocumentAssembly_IO.cpp` | STEP60 (@~L61), STEP69 (@~L66, same function `AppendEntityDomainsJson`, 5 lines apart), STEP84 (@~L34, different function) | STEP60 then STEP69 (adjacent, land together to resolve in one pass), STEP84 independent. |
| `MapImporter_ParseDocument_IO.cpp` | STEP60 (@~L65), STEP69 (@~L72, end of `ParseEntityDomainsJson`), STEP76 (@~L72, **same anchor line as STEP69**) | STEP60 → STEP69 → STEP76 last, specifically to resolve the STEP69/STEP76 exact-line collision while it's fresh. |
| `Sanmap_KnownTopLevelKeys_IO.cpp` | STEP60, STEP69 | Either order — two new list entries. |
| `CMakeLists.txt` | STEP52, STEP58, STEP63, STEP64, STEP65, STEP69, STEP72, STEP76, STEP84 | All additive (new `add_sangen_test` lines / vendoring blocks in different spots). Merge in any stable order, run the full `ctest` suite once at the end. STEP65's LuaJIT vendoring block and STEP72's Lua-resource staging block are thematically adjacent (both Lua-related) — worth a manual glance even though both are pure additions. |

## 3. THE SEQUENTIAL AGENT — 22 tickets, run one at a time in this order, after the batch merges

File conflicts don't matter here since it's one ticket at a time — only real dependencies do,
all already satisfied by what precedes each entry:

1. **STEP79** (← STEP66) — same dispatch unit as STEP66, land immediately after merge.
2. **STEP80** (← STEP66+STEP79)
3. **STEP50** (← STEP66 — the hidden dependency found in §0.3, now satisfiable)
4. **STEP51** (← STEP66 — same)
5. **STEP67** (← STEP66)
6. **STEP26B** (← STEP26A)
7. **STEP48** (← STEP47)
8. **STEP81** (← STEP60, STEP49)
9. **STEP53** (← STEP47, STEP50, STEP51, STEP52, STEP58 — STEP58 added 2026-08-22, §0.9)
10. **STEP54** (← STEP51 — remember the amendment already applied to STEP75, not this one; STEP54 itself needed no content fix, only correct sequencing)
11. **STEP57** (← STEP56)
12. **STEP59** (← STEP53, must be *implemented*, not merely merged — verify its test suite is green before dispatching this one)
13. **STEP70** (← STEP63, STEP69)
14. **STEP71** (← STEP64, STEP70, STEP72)
15. **STEP73** (← STEP69, STEP70, STEP63)
16. **STEP74** (← STEP69)
17. **STEP75** (← STEP68, STEP76 — amendment already applied earlier this session, ready as-is)
18. **STEP77** (← STEP74, STEP64, STEP65, STEP71, STEP72)
19. **STEP78** (← STEP47, STEP50, STEP51, STEP52, STEP53 — GATED, re-verify all five actually landed before dispatch, the ticket demands this itself)
20. **STEP82** (← STEP76)
21. **STEP83** (← STEP62 hard; STEP51, STEP53 soft/adaptive — both will already be landed by this point, so STEP83 takes its cleaner "already-landed" code path rather than the fold-in path)
22. **STEP94** (← STEP47, STEP48, STEP68, STEP49, STEP81)

**Not yet schedulable, excluded from both lists above:**
- `STEP96` — needs tickets 85–91 (unwritten) from `DESIGN_SantpFootprintIngestion_R1.md` §7.
- `STEP97` — needs STEP51+STEP60+STEP66 landed (all three will be, by the end of step 5 above)
  *and* a still-open ARCH routing-discriminator ruling. Re-check after step 5 whether that ruling
  has landed; if so, STEP97 becomes real backlog, not a permanent block.

## 4. Verification method

Every ticket in §2 was re-read in full by an independent agent and checked against the *current*
`src/` tree (not against another ticket's claims) for: (a) any code/type it silently needs that
doesn't exist yet, (b) any "verify X before assuming Y" caveat, actually verified, (c) any open
ARCH/Format Expert gap. Every same-file cluster in §2's merge table was independently re-verified
by a second agent reading both tickets' actual insertion points, not just their file lists.

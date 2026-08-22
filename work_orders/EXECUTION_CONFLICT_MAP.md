# Execution Conflict Map — parallel/sequential grouping for the unbuilt backlog

*Companion to `IMPLEMENTATION_STATUS.md` (which confirms 91/93 M0-M5+STEP1-45 tickets are
already shipped). This document scopes only the real remaining backlog — `STEP26A`, `STEP26B`,
and `STEP46`–`STEP97` (41 tickets, `STEP95` excluded, it shipped this session) — 43 tickets total.
`STEP96`/`STEP97` are excluded from the wave plan below; both are explicitly not-yet-dispatchable
(STEP96 needs unwritten tickets 85–91; STEP97 needs STEP51+60+66 plus an open ARCH ruling).*

**Method:** every one of the 43 tickets was read in full by a dedicated agent, extracting its
target-file list and every dependency/sequencing statement the ticket's own text makes. Every
place where two tickets touched the same file **without cross-referencing each other** was then
independently re-verified by a second agent reading both tickets' actual diffs to confirm whether
the overlap is real, and if so, whether order matters. Findings below are tiered by how deep that
verification went — do not treat a "flagged, not re-verified" item as cleared.

## Legend
- **BLOCKING** — one ticket cannot compile/function without the other; hard order, no exceptions.
- **REAL CONFLICT (verified)** — independently confirmed textual/logical overlap in the same
  function or struct; needs serial landing. Order is stated where it matters.
- **MECHANICAL (verified)** — same file, disjoint/adjacent edits; needs serial landing only to
  avoid a literal patch-apply failure, order does not matter.
- **FLAGGED (not independently re-verified)** — same file touched by 2+ tickets, found during
  inventory but not run through the second verification pass. Treat as same-file-serialize by
  default; confirm with a real diff before dispatching either ticket if both are ever scheduled
  close together.
- **CLEARED** — investigated and found safe to run in parallel.

## 1. The one real defect found (not just an ordering issue) — FIXED 2026-08-22

**STEP75 vs STEP76 — REAL CONFLICT, and STEP75's own text instructed a bug. Amendment applied.**
Both rewrite `DrawArmySettings` in `src/ui/ArmiesTab_UI.h`/`.cpp`. STEP76 makes `Army::name`
machine-owned (never human/UI-settable — the `ARMY_XX` law ruled earlier this session) and adds
`displayName` as the UI-editable field. STEP75's current text tells the coder to hand-set the
mirrored army's `name` directly to a computed `ARMY_XX` string — a direct violation of STEP76's
ruling once STEP76 lands. **Land STEP76 first.** STEP75 already depends on STEP68 anyway, so it
cannot go first regardless — but its own drafted instructions should be corrected to write through
whatever STEP76 ships (`AssignArmyIdentities`) rather than setting `army.name` by hand. Recommend
a small amendment to `STEP75_ArmyMirrorSymmetry_UI.md` once STEP76 lands, same pattern already
used for the STEP74/STEP76 amendment. **Done** — turned out simpler than "write through STEP76's
mechanism": mirroring only touches an *existing* army row (never adds/deletes/reorders), so its
`ARMY_XX` identity is already correct and untouched by the operation. The amendment strikes the
old hand-set instruction entirely rather than redirecting it through `AssignArmyIdentities`.

## 2. Other verified conflicts

| Pair | Verdict | Resolution |
|---|---|---|
| STEP60 ↔ STEP68 | REAL CONFLICT | Both insert new `MarkerTransform` members at the same spot in `MarkerInstance_PARAMS.h` and both add sibling reads in `ReadMarkerTransformJson`. No logical dependency, but land STEP60 first — STEP68's own draft already assumes `layerIndex` exists ("from STEP60, unchanged"). |
| STEP54 ↔ STEP55 | REAL CONFLICT (mechanical) | Both patches rewrite the identical `Regenerate` button line in `DrawCanvasWindow`. Order-free, but not simultaneous — second ticket's implementer works from the already-landed file. |
| STEP26B ↔ STEP77 | MECHANICAL | Different enum/fields/branches in `FilesTab_UI.h`/`_Actions_UI.cpp`/`_Draw_UI.cpp`, no logical collision, but same file/struct — land STEP26B first (STEP77 lands much later anyway, gated on STEP64/65/71/72). |
| STEP82 ↔ STEP84 | MECHANICAL | Disjoint line ranges in `MapExporter_IO.cpp`, no struct overlap. Land STEP84 first (zero dependencies, "safe to schedule at any point"), STEP82 rebases. |
| STEP60 ↔ STEP69 ↔ STEP84 | REAL CONFLICT (hotspot) | STEP60 and STEP69 both edit the **same functions** `AppendEntityDomainsJson`/`ParseEntityDomainsJson`; STEP84 also touches both files. Genuine recurring collision point, not coincidence — serialize all three, any order, one at a time. |
| STEP26A ↔ STEP84 | REAL CONFLICT (hotspot) | Both rewrite logic inside `Sanmap_MigrationRunner_IO.cpp`/`.h`. Serialize. |
| STEP83 ↔ STEP97 | CLEARED (for now) | STEP83 only *relocates* `SeedMarkerDomains` into a new file, doesn't change its logic; STEP97 would edit its body later. No real overlap today, and STEP97 isn't dispatchable regardless (blocked on STEP51+60+66 + an open ARCH ruling). Flag for whoever eventually drafts STEP97's implementation: find `SeedMarkerDomains` in `Application_OverlaySetup_Seed_UI.cpp`, not STEP51's original file, if STEP83 has landed by then. |
| STEP49 ↔ STEP80 | CLEARED | Confirmed disjoint — STEP80 never touches STEP49's files; within `MarkersTab_UI.h/.cpp` they edit different functions/state members entirely. Order-independent. |
| STEP57 ↔ STEP50 (CSR bucket-0) | CLEARED | Real defect (manual instances defaulting `ruleIndex=0` would collide with procedural rule 0's bucket), but **already fixed in STEP57's current text** (`instance.ruleIndex = -1`, credited to STEP83 §7). STEP83's own narration is just stale — no action needed. |

## 3. Additional same-file touches — flagged from inventory, not independently re-verified

Treat every row as "check before parallel dispatch," not as cleared:

| File(s) | Tickets | Note |
|---|---|---|
| `PreviewComposite_UI.h` | STEP46, STEP47 | Different sections (signature default-param vs. new methods) — likely low risk, not deep-checked. |
| `Application_UI.cpp` (`WireCallbacks()`) | STEP46, STEP48, STEP55 | Same function, three tickets. |
| `MapCanvas_UI.h`/`.cpp` | STEP48, STEP53, STEP55, STEP78, STEP94 | Large recurring hotspot — every canvas-touching ticket lands here. |
| `Application_UI.h` | STEP51, STEP52, STEP54, STEP64, STEP77 | Shared shell header. |
| `Application_Settings_UI.h` / `ApplicationSettings` struct | STEP48, STEP77 | Different fields presumed, not confirmed. |
| `OverlayLayer_Settings_UI.h` | STEP51, STEP53 (conditionally) | STEP53 only edits it "if STEP51 hasn't already" added the field. |
| `MapRecipe_PARAMS.h` | STEP60, STEP66, STEP69 | Three different fields/renames on one struct file. |
| `Sanmap_KnownTopLevelKeys_IO.cpp` | STEP60, STEP69 | Both add one key each. |
| `Sanmap_MigrationManifest_IO.h`/`.cpp` | STEP26A, STEP67 | Both touch the migration manifest. |
| `CMakeLists.txt` | Nearly every ticket | Universal — additive lines, always needs a final-merge pass; not treated as a real conflict per pair. |

## 4. Execution waves

Each wave's tickets are mutually parallel-safe (no shared file, or explicitly cleared above).
A ticket only appears once its hard dependencies **and** its recommended conflict-resolution
predecessors have landed. Where a same-wave ticket count is large, sub-teams can take one each.

**Wave 1 — no dependencies, no unresolved collisions**
STEP26A · STEP46 · STEP49 · STEP50 · STEP51 · STEP52 · STEP54 · STEP56 · STEP58 · STEP62 ·
STEP63 · STEP64 · STEP65 · STEP69 · STEP76

**Wave 2** — depends on Wave 1 landings
STEP26B (← STEP26A) · STEP47 (after STEP46, file collision) · STEP55 (after STEP54) ·
STEP57 (← STEP56) · STEP60 (after STEP69, hotspot) · STEP82 (← STEP76) ·
STEP70 (← STEP63, STEP69) · STEP73 (← STEP69, STEP63 — also needs STEP70, see Wave 3) ·
STEP74 (← STEP69)

*(STEP73 actually also depends on STEP70, so it really lands in Wave 3 — listed here only to
flag that its STEP69/STEP63 half is satisfied this wave.)*

**Wave 3**
STEP48 (← STEP47) · STEP66+STEP79 (single dispatch unit, land together, one branch — after
STEP60, hotspot) · STEP68 (after STEP60, verified conflict) · STEP81 (← STEP60, STEP49) ·
STEP73 (← STEP70) · STEP84 (after STEP60/STEP69/STEP26A hotspot cluster settles)

**Wave 4**
STEP53 (← STEP47, STEP50, STEP51, STEP52) · STEP67 (← STEP66) · STEP80 (← STEP66+STEP79) ·
STEP75 (← STEP68, and REQUIRES STEP76 already landed — Wave 1 — real conflict fix) ·
STEP71 (← STEP64, STEP70, STEP72 — **STEP72 has not appeared yet: it has no stated dependency
and can run any wave ≥1; place it in Wave 1 alongside STEP64/65/63 for the scenario track to
unblock STEP71/STEP73 on schedule**) · STEP83 (← STEP62 hard; STEP51/STEP53 soft, adaptive)

**Wave 5**
STEP59 (← STEP53, must be IMPLEMENTED not drafted) · STEP94 (← STEP47, STEP48, STEP68, STEP49,
STEP81) · STEP77 (← STEP74, STEP64, STEP65, STEP71, STEP72; also after STEP26B, mechanical)

**Wave 6**
STEP78 (GATED — ← STEP47, STEP50, STEP51, STEP52, STEP53; re-check all five are actually landed
before dispatch, per the ticket's own explicit re-verification instruction)

**Not yet schedulable — blocked on work outside this 43-ticket set**
- STEP96 — needs tickets 85–91 (unwritten) from `DESIGN_SantpFootprintIngestion_R1.md` §7.
- STEP97 — needs STEP51+STEP60+STEP66 landed *and* an open ARCH routing-discriminator ruling.

**Correction: STEP72 placement.** STEP72 has no stated dependency ("No dependency on STEP63")
and only a test-only soft dependency on STEP65/STEP70 — it can and should run in **Wave 1**, not
wait. This unblocks STEP71 one wave earlier than shown above; re-check STEP71's true earliest
wave is `max(STEP64=W1, STEP70=W3, STEP72=W1) + 1 = W4`, not W5 as loosely implied — STEP71 has
been moved to Wave 4's list above; STEP77 (which needs STEP71) becomes dispatchable in Wave 5 as
already shown, one wave sooner than a naive reading would suggest.

## 5. Notes for whoever dispatches these

- The **STEP75/STEP76 fix** (§1) is the one item that should probably become a small ticket
  amendment before dispatch, the same way STEP73/STEP74/STEP76 already amend each other in
  place — otherwise a coder following STEP75's current literal text ships a naming-law violation.
- The **hotspot clusters** (§2/§3) are a structural property of this codebase, not a planning
  mistake: `MapRecipe_PARAMS.h`, `AppendEntityDomainsJson`/`ParseEntityDomainsJson`,
  `Application_UI.cpp`/`.h`, and `MapCanvas_UI.h`/`.cpp` are central assembly points that almost
  every domain ticket touches additively. No amount of re-ordering removes this — the practical
  rule is: never dispatch two hotspot-touching tickets to two different coders in the same wave
  without an explicit rebase step between them, even when this document lists them in the same
  wave for logical-dependency purposes.
- §3's "flagged, not re-verified" rows are the highest-value place to spend a future verification
  pass if more confidence is wanted before actually executing — they were deprioritized here only
  by depth of check, not because they're less likely to be real.

# Work-Order — Step 42: trim `MapImporter_Recipe_IO.h`'s repeated declaration comments

*Constitution §7. Executor: SanGen Coder. IO Architecture Expert's outline from earlier this
session — drafted, never dispatched. Correcting that gap now.*

## Root problem
`src/io/MapImporter_Recipe_IO.h` is 172 lines, over ARCH §1.5's 150-line hard ceiling. Unlike
every other oversized file fixed this session, this one is NOT a logic problem — it's pure
declarations plus one small inline helper (`ReadJsonFloatVector4`, ~10 lines). The overage is
comment duplication: roughly a dozen declaration groups each restate a near-identical 3-8 line
block — "takes the top-level `document` directly... must be called unconditionally, BEFORE the
`mapGeneratorData` presence gate" — with only the specific domain name changed each time. Several
of these same explanations are ALSO independently carried in each function's own `.cpp` file
header comment (confirmed: `MapImporter_StratumLayers_IO.cpp`, `MapImporter_MarkersStack_IO.cpp`
both already restate their own tier/ordering rationale).

## Solution — shape
**Comment-trim only — no function moves, no reordering, no logic touched.** Trim each of the ~13
declaration-group comments down to a 1-2 line pointer: function name(s), one-line summary of the
JSON key/section it reads, and a pointer to its `.cpp` file's own header comment for the full
ordering rationale (matching the terser style `MapExporter_Recipe_IO.h` already uses for
`BuildStratumLayersJson`'s declaration since `STEP29`).

State the shared "unconditional, before the `mapGeneratorData` gate" ordering law ONCE, as a new
SCOPE NOTE in `MapImporter_IO.h` (which already documents `MapImporter`'s calling contract) or at
the top of this file's own banner comment — pick whichever already-established location fits best
— and have every trimmed per-declaration comment point back at it instead of restating it.

Keep `ReadJsonFloatVector4`'s body and its own explanatory comment exactly as-is — its "stays
here, not promoted to `JsonPrimitives_IO.h`" placement is an already-settled decision
(`JsonPrimitives_IO.h`'s own header references this), not something to re-litigate.

Also trim `ClampGeometryBand`'s declaration comment (added by `STEP41`, currently ~8 lines) to
match the same terser style, keeping only what isn't already said in `MapImporter_Recipe_IO.cpp`'s
own implementation comment for it.

## Target files
- `src/io/MapImporter_Recipe_IO.h` — trim all ~13 declaration-group comments plus
  `ClampGeometryBand`'s.
- `src/io/MapImporter_IO.h` — add the one consolidated "ordering law" SCOPE NOTE, if that's the
  chosen home (confirm against the file's existing SCOPE NOTE numbering before adding a new one).

## Explicit out-of-scope
- Any function move, reorder, or signature change — this file's declarations stay exactly where
  they are, in the same order, with the same signatures.
- `ReadJsonFloatVector4`'s implementation or its "not a generic primitive" placement decision.

## Layer & accuracy class
IO only, comment/documentation only. Zero behavior change. Accuracy class: Exact.

## Acceptance test
1. `MapImporter_Recipe_IO.h` lands under 150 lines (target ~105-115, matching the original
   outline's estimate).
2. Every declaration is still present, same signature, same order — a diff of the file's
   non-comment lines against the pre-ticket version is empty.
3. The shared ordering law is stated exactly once, and every trimmed comment correctly points to
   it (spot-check a few, not exhaustive).
4. Full `SanGenV2` build stays clean; every existing test continues to pass (this ticket cannot
   break anything at runtime — it's comments only — so a clean build is the only real proof
   needed).

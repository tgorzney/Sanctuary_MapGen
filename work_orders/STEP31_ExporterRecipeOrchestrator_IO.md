# Work-Order — Step 31: make `MapExporter_Recipe_IO.cpp` a true orchestrator

*Constitution §7. Executor: SanGen Coder. IO Architecture Expert consult, reconciling its own
earlier STEP29 ruling against the human's sharper instruction: this file should contain zero real
logic, only a calling sequence — not just be under the line ceiling.*

## Root problem
`MapExporter_Recipe_IO.cpp` is 149 lines (150-line hard ceiling, zero headroom) after `STEP29`/
`STEP30`. It still holds two unrelated occupants: `BuildSanmapJsonText`'s orchestration + 4
file-private grouping helpers, AND the unrelated `BuildMapGeneratorDataJson` legacy-blob builder
(22 lines of real per-field domain logic). The importer's mirror, `MapImporter_Recipe_IO.cpp` (92
lines), never had this problem — it holds ONLY legacy-blob readers; its own orchestrator lives in
`MapImporter_IO.cpp` entirely. That asymmetry is the root cause.

## Ruled by this ticket
1. **Two new files, split by kind:**
   - `src/io/MapExporter_DocumentAssembly_IO.h`/`.cpp` — the 4 orchestration-tier helpers
     (`BuildDocumentEnvelopeJson`, `AppendEntityDomainsJson`, `AppendStackDomainsJson`,
     `AppendSimulationDomainsJson`). None of these own a single top-level `.sanmap` section — they
     only sequence calls into real per-domain builders that already live elsewhere — so they're not
     subject to "one domain per file," they're a genuinely new kind (orchestration helpers), hence
     their own file.
   - `src/io/MapExporter_MapGeneratorData_IO.cpp` — `BuildMapGeneratorDataJson` alone, **no new
     header** (its existing declaration in `MapExporter_Recipe_IO.h` stays, only the `.cpp`
     implementation moves). This is real per-field domain logic for exactly one top-level section
     (`mapGeneratorData`) — same footing as `BuildAreasJson`/every other domain builder. This
     supersedes STEP29's "keep it bundled, match the importer's file-naming precedent" ruling — that
     ruling was about the FILE NAME being the legacy blob's reserved home when bundling was
     tolerable; it isn't anymore under "zero real logic in the orchestrator."
2. **Real header declarations for the 4 helpers**, in the NEW `MapExporter_DocumentAssembly_IO.h`
   — not `MapExporter_Recipe_IO.h` (already 127 of its own 150-line ceiling; adding these would
   just ratchet the problem sideways). `BuildSanmapJsonText` itself stays in
   `MapExporter_Recipe_IO.cpp` per direct instruction — it becomes a multi-file orchestrator,
   `#include`-ing both `MapExporter_Recipe_IO.h` (for `BuildMapGeneratorDataJson`) and the new
   `MapExporter_DocumentAssembly_IO.h` (for the 4 helpers) — exactly mirroring how
   `MapImporter_IO.cpp` already includes `MapImporter_Recipe_IO.h` for its own cross-file calls.

Expected result:
| File | Est. lines |
|---|---|
| `MapExporter_Recipe_IO.cpp` | ~30-35 — header comment, includes, `BuildSanmapJsonText`'s existing 17-line body, boilerplate. Pure orchestrator. |
| `MapExporter_DocumentAssembly_IO.h` | ~40-45 |
| `MapExporter_DocumentAssembly_IO.cpp` | ~100-110 |
| `MapExporter_MapGeneratorData_IO.cpp` | ~35 |
| `MapExporter_Recipe_IO.h` | unchanged, 127 |

## Target files
- `src/io/MapExporter_Recipe_IO.cpp` — strip to `BuildSanmapJsonText` only, add the 2 new includes.
- New `src/io/MapExporter_DocumentAssembly_IO.h`/`.cpp` — the 4 helpers, moved verbatim with their
  existing comments.
- New `src/io/MapExporter_MapGeneratorData_IO.cpp` — `BuildMapGeneratorDataJson`, moved verbatim.
- No change to `src/io/MapExporter_Recipe_IO.h` (declaration for `BuildMapGeneratorDataJson`
  already there, untouched).
- Build file registration if not glob-based (check `CMakeLists.txt`, matches STEP29's confirmed
  glob-based reality — likely a no-op, verify).

## Explicit out-of-scope
- Any behavior/output change — pure relocation, zero JSON output difference. Same acceptance-test
  discipline as STEP29: diff exported JSON before/after for a non-trivial recipe, byte-identical.
- The legacy blob's actual content/fate — unchanged, still not deleted (per standing instruction).

## Layer & accuracy class
IO only, pure refactor. Accuracy class: Exact.

## Acceptance test
1. `MapExporter_Recipe_IO.cpp` contains zero function bodies except `BuildSanmapJsonText`.
2. All 4 files land at or under their estimated line counts, all under the 150 hard ceiling.
3. Exporting the same non-trivial recipe before and after this refactor produces byte-identical
   JSON output (explicit diff test, not just "tests pass").
4. Full `SanGenV2` build stays clean; every existing test continues to pass.

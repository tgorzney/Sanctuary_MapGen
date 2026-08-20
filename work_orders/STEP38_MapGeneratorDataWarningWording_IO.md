# Work-Order — Step 38: fix the stale "No mapGeneratorData block" warning wording

*Constitution §6. Executor: SanGen Coder. Side effect of `STEP36`: this message was accurate
before schema-v3's top-level relocations; it is now misleading and fires on every normal file.*

## Root problem
`MapImporter_ParseDocument_IO.cpp:130-132`:
```cpp
if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) {
    result.Warn("No mapGeneratorData block: only the map's own dimensions were recovered.");
    return true;
}
```
This `Warn()` is surfaced directly to the designer (`result.debugLog` → `FilesTab_Actions_UI.cpp`'s
`AppendFilesTabLog` → the Files tab's visible log panel, `FilesTab_Draw_UI.cpp`). Since `STEP36`
stopped the exporter from ever writing `mapGeneratorData`, this branch now fires on EVERY normal
reopen of a freshly-exported file — but by that point in the function, every top-level domain
reader (`ParseDocumentEnvelopeJson`/`ParseEntityDomainsJson`/`ParseStackDomainsJson`/
`ParseSimulationDomainsJson`) has already run and recovered everything. "Only the map's own
dimensions were recovered" is now false for the common case, not just imprecise.

## Solution — shape
Reword to reflect current reality, and reconsider severity: absence of `mapGeneratorData` is now
the EXPECTED, NORMAL state for any current-format export — not a degraded-recovery signal. Only a
genuinely old/foreign file (still carrying the legacy blob) benefits from a "block was present,
recovered from it" contrast. Recommend demoting from `Warn()` to `Log()` (informational, not a
warning-level concern) with wording along the lines of: "No legacy mapGeneratorData block present
— nothing needed it (current-format export)." Confirm `MapImportResult::Log()` vs `Warn()`'s
actual UI distinction (if any — check whether the Files tab visually differentiates them) before
finalizing wording/severity.

## Target files
- `src/io/MapImporter_ParseDocument_IO.cpp` — reword and reconsider `Log()` vs `Warn()`.
- Any test asserting on this exact message text (`MapImporter_IO_Test.cpp` per `STEP36`'s own
  report) — update to match.

## Explicit out-of-scope
- Any other warning/log message wording — scoped to this one message only.

## Layer & accuracy class
IO only, wording/severity only. Accuracy class: Exact.

## Acceptance test
1. A normal, freshly-exported (post-`STEP36`) file, on reopen, produces a message that does not
   claim reduced recovery — reads as informational/expected, not alarming.
2. A genuinely old file WITH a real `mapGeneratorData` block still gets whatever message (if any)
   is appropriate for that block's presence — unaffected by this ticket's wording change on the
   absence branch.
3. Full `SanGenV2` build stays clean; every existing test continues to pass.

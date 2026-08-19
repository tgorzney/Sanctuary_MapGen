# Work-Order — Step 28: fix Unknown-Import to nest under one key, not merge flat

*Constitution §6. Executor: SanGen Coder. Corrects a real deviation `STEP24`'s implementation made
from the ratified spec text — human confirmed nested is correct, matching the original design
intent (a single, filterable, deletable container for unrecognized data).*

## Root problem
`IO_MIGRATION_SPEC.md` §6 says unrecognized top-level keys are "preserved verbatim under one
reserved top-level key, `UnknownImport`" — nested. `STEP24`'s shipped implementation instead
merges each unrecognized key straight into the document's top level, indistinguishable from real
data — flat. Flat fails all three of the human's original goals: not lost (technically satisfied,
but), easily filtered, easily ignored, easily deleted — none of those hold without a single
container to point at.

Confirmed live: `Sanmap_MigrationRunner_IO.cpp:38-43` (`CaptureUnknownTopLevelKeys`) copies each
unrecognized key directly into `outUnknownData.unknownTopLevelKeys[key]`; `MapExporter_
UnknownImportMerge_IO.h:20-24` (`MergeUnknownImportKeys`) writes each of those back to
`document[key]` directly. Both need to change.

## Solution — shape
**Export (`MergeUnknownImportKeys`, `MapExporter_UnknownImportMerge_IO.h`):** if the bag is
non-empty, write the WHOLE bag as one nested object:
```cpp
if (unknownData != nullptr && !unknownData->unknownTopLevelKeys.empty())
    document["UnknownImport"] = unknownData->unknownTopLevelKeys;
```
No per-key collision loop needed anymore — a single top-level assignment.

**Import (`CaptureUnknownTopLevelKeys`, `Sanmap_MigrationRunner_IO.cpp`):** stability requirement —
round-tripping a document whose `UnknownImport` was written by a PRIOR export must not accumulate
nesting (`UnknownImport.UnknownImport.UnknownImport...`) on every load/save cycle. Before the
existing per-key loop, seed the bag directly from any incoming `UnknownImport` object's own
children (flattened one level, not wrapped again):
```cpp
if (document.contains("UnknownImport") && document["UnknownImport"].is_object())
    for (const auto& [key, value] : document["UnknownImport"].items())
        outUnknownData.unknownTopLevelKeys[key] = value;
```
Then the existing loop continues to capture any OTHER genuinely-unrecognized top-level keys into
the same bag (unaffected — a key that's already known, or that becomes known in a later build,
still self-heals exactly as originally designed).

**Allowlist (`Sanmap_KnownTopLevelKeys_IO.cpp`, `IsKnownTopLevelSanmapKey`):** add `"UnknownImport"`
itself to the known-keys list, with a comment explaining it's runner-owned and special-cased above
— defensive clarity, not strictly load-bearing (the explicit `document.contains("UnknownImport")`
check above already prevents it from falling into the generic unknown-capture loop), but leaving
it off the allowlist would make a future reader wonder why it isn't captured as "just another
unknown key."

## Target files
- `src/io/MapExporter_UnknownImportMerge_IO.h` — nested single-key write.
- `src/io/Sanmap_MigrationRunner_IO.cpp` — seed-then-capture, as above.
- `src/io/Sanmap_KnownTopLevelKeys_IO.cpp` — add `"UnknownImport"` to the allowlist.
- `src/io/Sanmap_MigrationRunner_IO_Test.cpp`, `src/io/MapImporter_IO_Test.cpp` — every existing
  assertion checking `document[someUnknownKey]` directly must change to
  `document["UnknownImport"][someUnknownKey]`. Add a new round-trip case proving stability: export
  a document with unknown data (producing a nested `UnknownImport`), re-import it, re-export it
  again — the second export's `UnknownImport` object must equal the first's exactly, no additional
  nesting level.

## Explicit out-of-scope
- Nested/recursive capture of unrecognized fields INSIDE a known key (e.g. `mapGeneratorData`'s
  own unrecognized sub-fields) — still out of scope, unchanged from `STEP24`.
- Any other `STEP24` behavior — this ticket only changes the on-disk shape of the passthrough
  container, nothing about when/what gets captured.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. A document with 2+ genuinely-unrecognized top-level keys exports with all of them nested under
   one `document["UnknownImport"]` object, nothing at the document's own top level.
2. Re-importing that exported document recovers the same unrecognized keys into the bag (via the
   seed step, not the generic capture loop).
3. Re-exporting that re-imported recipe produces byte-identical `UnknownImport` content to the
   first export — no accumulated nesting across 2+ round trips.
4. A document with NO unrecognized data never gets a `document["UnknownImport"]` key at all (empty
   bag → no key written, not an empty object).
5. Full `SanGenV2` build stays clean; every existing test continues to pass.

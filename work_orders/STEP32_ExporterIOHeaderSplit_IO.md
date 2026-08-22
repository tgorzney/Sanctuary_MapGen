# Work-Order — Step 32: split `MapExporter_IO.h` under the ARCH_01_05_FileSizeCeilings.md §1.5 ceiling

*Constitution §7. Executor: SanGen Coder. IO Architecture Expert consult.*

## Root problem
`src/io/MapExporter_IO.h` is 173 lines, over the 150-line hard ceiling. It bundles 4 separable
things: (1) `MapExporter`'s own domain (options/result structs + class declaration), (2)
`BlueprintValidationReport`/`ValidatePropAndDecalBlueprintPaths` (a deliberate public-header
exception, UI calls it directly — do not demote its visibility), (3) generic filesystem primitives
(`JoinExportPath`/`EnsureFolderExists`/`WriteBinaryFileBytes`) that are NOT exporter-specific —
confirmed real cross-domain dependents already exist: `AppSettings_IO.cpp` and
`MapImporter_Fields_IO.cpp` both pull in this entire header just to reach these 3 functions, (4)
`QuantizeNormalizedHeightSample`/`QuantizeNormalizedWeightSample`, small, export-write-scoped.

## Ruled by this ticket
1. **New `src/io/FilesystemPrimitives_IO.h`/`.cpp`** — `JoinExportPath`, `EnsureFolderExists`,
   `WriteBinaryFileBytes`. Mirrors the `JsonPrimitives_IO.h` precedent exactly: a generic, reusable
   toolkit header, not a domain file. Their implementations are CURRENTLY already split
   inconsistently across two unrelated `.cpp`s (`EnsureFolderExists` in `MapExporter_IO.cpp`,
   `JoinExportPath`/`WriteBinaryFileBytes` in `MapExporter_Textures_IO.cpp`) — this move
   consolidates both into one real home. `EnsureExportFolderExists(folderPath,
   MapExportResult&)` (the thin logging wrapper that DOES take a `MapExporter`-specific type)
   stays in `MapExporter_IO.h`/`.cpp`. Update every real caller's include:
   `MapExporter_IO.cpp`, `MapExporter_Textures_IO.cpp`, `MapExporter_Image_IO.cpp`,
   `AppSettings_IO.cpp`, `MapImporter_Fields_IO.cpp`, `FilesTab_Actions_UI.cpp` (verify this exact
   list against real includes, don't assume it's complete).
2. **New `src/io/MapExporter_BlueprintValidation_IO.h`** — `BlueprintValidationReport` +
   `ValidatePropAndDecalBlueprintPaths` declaration, sibling to the already-existing
   `MapExporter_BlueprintValidation_IO.cpp` (which currently has NO header of its own — it
   `#include`s `MapExporter_IO.h` just for these). Stays a public header (the documented UI-calls-
   it-directly exception is unchanged) — `FilesTab_Draw_UI.cpp` and any other real consumer now
   includes this smaller, correctly-named header instead of the whole `MapExporter_IO.h` surface.
3. **New `src/io/MapExporter_SampleQuantize_IO.h`** — `QuantizeNormalizedHeightSample`/
   `QuantizeNormalizedWeightSample`, header-only inline functions. Consumers
   (`MapExporter_Image_IO.cpp`, `MapExporter_Textures_IO.cpp`, `MapExporter_IO_Test.cpp`) add the
   include. Do all three splits together in this one ticket — margin matters: 1+2 alone lands
   right at 148 lines, too tight; all three together lands ~135-138 with real headroom.

Everything else stays in `MapExporter_IO.h`: `MapExportFileNames`, `MapExportOptions`,
`MapExportResult`, the format constants (`sanmapFileVersion` etc.), the `class MapExporter`
declaration itself.

## Target files
- New `src/io/FilesystemPrimitives_IO.h`/`.cpp`.
- New `src/io/MapExporter_BlueprintValidation_IO.h`.
- New `src/io/MapExporter_SampleQuantize_IO.h`.
- `src/io/MapExporter_IO.h`/`.cpp` — remove the moved declarations/implementations, add includes
  where still needed.
- `src/io/MapExporter_Textures_IO.cpp`, `MapExporter_Image_IO.cpp`, `AppSettings_IO.cpp`,
  `MapImporter_Fields_IO.cpp`, `FilesTab_Actions_UI.cpp`, `MapExporter_BlueprintValidation_IO.cpp`,
  `MapExporter_IO_Test.cpp` — update includes to the new, narrower headers.

## Explicit out-of-scope
- Any behavior change — pure relocation. `BlueprintValidationReport`'s public-header status is
  preserved exactly, not narrowed.

## Layer & accuracy class
IO only, pure refactor. Accuracy class: Exact.

## Acceptance test
1. `MapExporter_IO.h` lands under 150 lines (target ~135-138) after all 3 splits.
2. Every real caller of the moved filesystem primitives (`AppSettings_IO.cpp`,
   `MapImporter_Fields_IO.cpp` included — confirm these specifically, since they're the cross-
   domain dependents that proved this split's value) compiles against the new narrower header.
3. `BlueprintValidationReport`/`ValidatePropAndDecalBlueprintPaths` remain callable exactly as
   before from `FilesTab_Draw_UI.cpp` — no behavior change to the blueprint-validation gate.
4. Full `SanGenV2` build stays clean; every existing test continues to pass.

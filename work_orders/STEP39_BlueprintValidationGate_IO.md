# Work-Order — Step 39: move blueprintPath validation into the exporter itself

*Constitution §6. Executor: SanGen Coder. Human directive: more export paths are planned (a
per-domain export button for each entity type — armies, units, props, decals, etc.), so the
blueprint-path safety check must live in the actual write function, not just one button's
click-handler.*

## Root problem
`ValidatePropAndDecalBlueprintPaths` (`MapExporter_BlueprintValidation_IO.cpp`) only reports — it
never blocks a write itself. Today it's gated correctly at the ONE existing call site
(`FilesTab_Draw_UI.cpp`'s `PreCheckGatedExport`/`DrawGatedExportButton`/
`DrawPendingBlueprintWarningDialog` triad: click Export → validate → if unresolved paths exist,
show a confirm dialog → only write if the designer clicks "Export Anyway"). `BuildSanmapJsonText`/
`MapExporter::ExportSanmapOnly`/`ExportAll` themselves have zero built-in enforcement — the
`SANMAP_FORMAT_SPEC.md` requirement ("the exporter MUST verify every blueprintPath resolves...
before writing a `.sanmap`" — a real, in-game-confirmed defect: an unresolved path aborts
everything the game parses AFTER props in the file) is currently satisfied only by this one UI
call site's discipline. A future second export path (any per-domain export button) would need to
remember to re-implement this exact gate, or bypass the safety entirely.

## Solution — shape
Move the gate into the IO layer itself, so it's structurally impossible to bypass regardless of
which UI entry point triggers a write:
- `MapExporter::ExportSanmapOnly`/`ExportAll` (or their shared internal write path — check the
  actual call graph) gain a mandatory check: if `assetPack` is available and
  `ValidatePropAndDecalBlueprintPaths` reports any unresolved path, DO NOT silently refuse (that
  would break the existing "designer can knowingly export anyway" UX) — instead, require the
  caller to have already made an explicit choice, surfaced via a new parameter (e.g. `bool
  bBlueprintValidationAcknowledged` or an enum) that defaults to requiring acknowledgment. The IO
  function refuses (returns a failed `MapExportResult`, does not write) unless either (a) all
  paths resolve, or (b) the caller explicitly passed acknowledgment.
- `FilesTab_Draw_UI.cpp`'s existing triad becomes the FIRST caller to supply that acknowledgment
  (after the designer clicks "Export Anyway" in the dialog) — its own behavior does not change for
  the designer, only where the enforcement structurally lives.
- Any FUTURE export path that doesn't explicitly handle blueprint validation now fails loudly by
  default (refuses to write) rather than silently skipping the check — the safe default flips from
  "must remember to add the check" to "must remember to acknowledge it."

## Target files
- `src/io/MapExporter_IO.h`/`.cpp` — add the acknowledgment parameter/enum to `ExportSanmapOnly`/
  `ExportAll` (or their shared write path), refuse-by-default when unresolved paths exist and no
  acknowledgment was given.
- `src/ui/FilesTab_Draw_UI.cpp`/`FilesTab_Actions_UI.cpp` — thread the acknowledgment through from
  the existing confirm-dialog flow so current designer-facing behavior is unchanged.
- Existing tests exercising export success/failure — extend to cover both the new refuse-by-default
  case and the acknowledged-override case.

## Explicit out-of-scope
- Building any of the planned per-domain export buttons themselves — not this ticket, just making
  sure the safety net will already cover them whenever they land.
- Any change to the validation LOGIC itself (`ValidatePropAndDecalBlueprintPaths`'s own field-by-
  field checks) — unchanged, just where its result gets enforced.

## Layer & accuracy class
IO + thin UI wiring. Accuracy class: Exact.

## Acceptance test
1. Calling the export write path directly (bypassing the UI dialog entirely, as a future headless/
   batch caller would) with unresolved blueprint paths and no acknowledgment REFUSES to write —
   confirm no `.sanmap` file is produced.
2. The same call WITH acknowledgment succeeds and writes exactly as before.
3. The existing Files-tab UI flow (click Export → dialog if needed → "Export Anyway") behaves
   identically to today from the designer's perspective — no UX regression.
4. A call with zero unresolved paths succeeds without requiring any acknowledgment.
5. Full `SanGenV2` build stays clean; every existing test continues to pass.

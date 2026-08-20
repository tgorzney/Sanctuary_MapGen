# Work-Order — Step 37: `stratumLayers[]` writes real appearance name/environment/material

*Constitution §6. Executor: SanGen Coder. Found by a round-trip-fidelity audit.*

## Root problem
`stratum.appearance.name`, `.environmentName`, `.materialName` are live, designer-editable fields
(`StratumsTab_Material_UI.cpp:52-57` — a text input and two asset-picker combos on the Stratums
tab), but:
- `MapExporter_StratumLayers_IO.cpp:23` writes a hardcoded placeholder,
  `"Stratum " + std::to_string(stratumIndex)`, instead of the real `appearance.name` value —
  already flagged in-code as a known gap.
- `environmentName`/`materialName` are never written or read anywhere in `src/io/` — total gap,
  confirmed by grep.

Every designer-set stratum name/environment/material selection is silently lost on export today.

## Solution — shape
`MapExporter_StratumLayers_IO.cpp`: `layer["name"] = appearance.name;` (replacing the placeholder
— but see the empty-name handling below). Add `layer["environmentName"] = appearance.environmentName;`
and `layer["materialName"] = appearance.materialName;` as siblings of the existing per-layer keys.

`MapImporter_StratumLayers_IO.cpp`: `ReadJsonText(layerJson, "name", stratum.appearance.name)`,
same for `environmentName`/`materialName`.

**Empty-name handling**: `StratumsTab_Material_UI.cpp:52` uses `StratumNameRules()` for the Name
text input — check what fallback/empty behavior that rule enforces (mirrors `STEP25`'s precedent
of checking the UI's own enforced invariant before wiring the importer) and match it on import if
the field is empty/missing, so old files without this field don't end up with a blank stratum name
where the UI expects a non-empty one.

## Target files
- `src/io/MapExporter_StratumLayers_IO.cpp` — replace the placeholder, add 2 new keys.
- `src/io/MapImporter_StratumLayers_IO.cpp` — add 3 new reads.
- `src/ui/StratumsTab_Material_UI.cpp` — read `StratumNameRules()`'s fallback behavior, don't
  change the UI itself unless the importer needs to match a specific fallback text.
- Test coverage in `MapExporter_IO_Test.cpp`/`MapImporter_IO_Test.cpp` for round-trip of
  non-default values on all 3 fields.

## Explicit out-of-scope
- Any other `StratumAppearance` field — this ticket is scoped to exactly these 3.
- `STEP36`'s legacy blob deletion — independent, may land in either order.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. A recipe with non-default `appearance.name`/`environmentName`/`materialName` on at least one
   stratum round-trips exactly through export → import.
2. A document missing these keys entirely (old-shaped file) imports with sane fallback behavior
   matching the UI's own empty-name rule, not a crash or silent corruption.
3. Full `SanGenV2` build stays clean; every existing test continues to pass.

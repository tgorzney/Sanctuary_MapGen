# Work-Order — Step 13: `MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack` — relocate placement
# rules out of the legacy `mapGeneratorData.PlacementRules` blob

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 7 +
`ARCH_11_GlobalMarkerSettings.md` §11 (`Params::GlobalMarkerSettings`), per a Format Expert ruling obtained for this
ticket resolving two ambiguities the specs left open (see "Ruled by this ticket" below).*

## Root problem
The four placement-rule vectors (`Params::MarkerRule`/`PropRule`/`DecalRule`/`UnitRule`, all
field-complete) round-trip **only** through the legacy `mapGeneratorData.PlacementRules` object
today (`MapExporter_Rules_IO.cpp`/`MapImporter_Rules_IO.cpp`) — confirmed by direct read of both
files and `MapExporter_Recipe_IO.cpp:72`/`MapImporter_IO.cpp:138`. Correction 7 gives each rule
type its own top-level, format-sibling section (matching the same relocation the entity domains
and `StratumGenerationSettings` already got in earlier STEP tickets) and adds map-wide
`GlobalMarkerSettings` (icon/color/scale defaults for the three resource marker kinds), which
currently has **no C++ type or IO surface at all**.

## Ruled by this ticket (resolving spec ambiguity — see the Format Expert consultation this ticket
is built from)
1. **`MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack` are bare top-level JSON arrays** of
   their respective rule type — same shape as `PropGroups`/`DecalGroups`/`StratumGenerationSettings`.
   Correction 7's "flat array... wrapped under the new top-level key" wording governs.
2. **`GlobalMarkerSettings` is its own top-level PascalCase key, a sibling of `MarkersStack`, NOT
   nested inside it.** Correction 7's "sub-key inside `MarkersStack`" phrasing is superseded by
   `ARCH_11_GlobalMarkerSettings.md` §11, which is explicitly framed as completing Correction 7 and states `MapRecipe`
   gains `globalMarkerSettings` as a "flat sibling of `markerRules`, for now." (A JSON array also
   cannot structurally host a nested key, which independently rules out reading Correction 7's
   wording literally.) **Flag to the ARCH Expert as a separate docs-only follow-up:** Correction
   7's own text should be tightened so it stops contradicting §11.
3. **`mapGeneratorData.PlacementRules` is relocated and DELETED in this ticket**, not
   dual-written — same treatment Correction 12/13 gave `Stratums`' slope-gate fields. Its entire
   content (all four rule vectors) moves with nothing left behind, unlike `Stratums` (which kept 5
   unrelated fields for an unrelated, still-open reason). Building the `MarkersStack_Migrate_V2_IO`
   etc. backward-compat migration files for **old, pre-v3 files** that still have the legacy key is
   explicitly **out of scope** — that's `IO_MIGRATION_SPEC`'s domain (IO Architecture Expert), a
   separate dependent follow-up ticket, not this one.

## Target files
**New PARAMS:**
- `src/params/GlobalMarkerSettings_PARAMS.h` (new) — `Params::GlobalMarkerSettings`, exact shape
  from `ARCH_11_GlobalMarkerSettings.md` §11 (verbatim, do not deviate):
  ```cpp
  struct GlobalMarkerSettings {
      std::string iconNameAlloy  = "Alloy";
      std::string iconNamePlasma = "Plasma";
      std::string iconNameSpawn  = "Spawn";
      float colorAlloy[4]  = {0.8f, 0.8f, 0.2f, 1.0f};
      float colorPlasma[4] = {0.2f, 0.8f, 0.8f, 1.0f};
      float colorSpawn[4]  = {0.8f, 0.2f, 0.2f, 1.0f};
      float scaleAlloy  = 0.17f;
      float scalePlasma = 0.17f;
      float scaleSpawn  = 0.17f;
  };
  ```
- `src/params/MarkerRule_PARAMS.h` — add 4 new fields to `Params::MarkerRule` (confirmed absent
  from the type today, and absent from every other current PARAMS type — a genuine addition, not a
  relocation of any live v2 global): `float hydroMultiplier = 1.0f;`, `float reclaimDensity = 0.0f;`,
  `float mexDensity = 0.0f;`, `int spawnPointCount = 0;` (defaults are placeholders — pick sane
  values per Constitution §8; not prescribed by the spec).
- `src/params/MapRecipe_PARAMS.h` — add `GlobalMarkerSettings globalMarkerSettings;`, flat sibling
  of `markerRules` (per ruling #2 above), plus `#include "GlobalMarkerSettings_PARAMS.h"`.

**New IO (mirrors the existing per-domain split — `MapExporter_Props_IO.cpp`/
`MapExporter_Decals_IO.cpp` precedent — one exporter/importer pair per domain, each comfortably
under the ARCH_01_05_FileSizeCeilings.md §1.5 100-line soft ceiling):**
- `src/io/MapExporter_ScatterTransform_IO.cpp`/`.h` (new) — `BuildScatterTransformJson`, extracted
  verbatim from `MapExporter_Rules_IO.cpp` (unchanged body) since it is now shared plumbing across
  4 sibling files, not local to one.
- `src/io/MapImporter_ScatterTransform_IO.cpp`/`.h` (new) — `ReadScatterTransformJson`, extracted
  verbatim from `MapImporter_Rules_IO.cpp` (unchanged body), same reasoning.
- `src/io/MapExporter_MarkersStack_IO.cpp` (new) — `BuildMarkersStackJson` (the array; body =
  `BuildMarkerRuleJson`, relocated verbatim from `MapExporter_Rules_IO.cpp`) +
  `BuildGlobalMarkerSettingsJson`. Named `MarkersStack`, not `Markers`, to avoid colliding with the
  existing `markers[]` instance-array exporter (`BuildMarkersJson`) — same domain, different
  concept, same distinction Correction 7 draws for Props/Decals Groups vs Stacks.
- `src/io/MapImporter_MarkersStack_IO.cpp` (new) — inverse; `ReadMarkerRuleJson` relocated
  verbatim, `ReadGlobalMarkerSettingsJson` new.
- `src/io/MapExporter_PropsStack_IO.cpp` (new) — `BuildPropsStackJson`; `BuildPropRuleJson`
  relocated verbatim.
- `src/io/MapImporter_PropsStack_IO.cpp` (new) — inverse; `ReadPropRuleJson` relocated verbatim.
- `src/io/MapExporter_DecalsStack_IO.cpp` (new) — `BuildDecalsStackJson`; `BuildDecalRuleJson`
  relocated verbatim.
- `src/io/MapImporter_DecalsStack_IO.cpp` (new) — inverse; `ReadDecalRuleJson` relocated verbatim.
- `src/io/MapExporter_UnitsStack_IO.cpp` (new) — `BuildUnitsStackJson`; `BuildUnitRuleJson`
  relocated verbatim.
- `src/io/MapImporter_UnitsStack_IO.cpp` (new) — inverse; `ReadUnitRuleJson` relocated verbatim.

**Deleted (fully superseded, per ruling #3):**
- `src/io/MapExporter_Rules_IO.cpp` — content relocated into the 4 new exporter files above; delete
  the file (including the now-empty `BuildPlacementRulesJson` wrapper — it has no replacement, the
  new top-level keys are written directly).
- `src/io/MapImporter_Rules_IO.cpp` — same, including `ReadPlacementRulesJson`.
- Any header declarations for the two functions above, wherever they live (check
  `MapExporter_Recipe_IO.h`/`MapImporter_Recipe_IO.h` or a dedicated `MapExporter_Rules_IO.h` if
  one exists — not confirmed in this ticket's research, coder must check).

**Wiring:**
- `src/io/MapExporter_Recipe_IO.cpp` — in `BuildMapGeneratorDataJson`, delete
  `generatorData["PlacementRules"] = BuildPlacementRulesJson(recipe);` (line 72). In
  `BuildSanmapJsonText`, add the 5 new top-level keys (`MarkersStack`, `PropsStack`, `DecalsStack`,
  `UnitsStack`, `GlobalMarkerSettings`) at the same tier as `PropGroups`/`DecalGroups`/
  `SlopeDefaults` — siblings of `mapGeneratorData`, not nested in it.
- `src/io/MapImporter_IO.cpp` — delete the `ReadPlacementRulesJson(generatorData, outRecipe);` call
  (line 138, currently inside the `mapGeneratorData`-presence gate). Add the 5 new reads
  **unconditionally, before** the `mapGeneratorData` gate (line ~129) — same tier as every other
  top-level-key ticket this session (`stratumLayers`, `SlopeDefaults`, `PropGroups`/`DecalGroups`).

## Layer & accuracy class
PARAMS (new `GlobalMarkerSettings` type + 4 new `MarkerRule` fields) + IO/BRIDGE. Accuracy class:
Exact.

## Backend policy
CPU only.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 7 — binding, as narrowed by "Ruled by this ticket" above.
- `ARCH_11_GlobalMarkerSettings.md` §11 — binding, controls over Correction 7's "sub-key" wording (see ruling #2).
- Constitution §6 — asset/input validation posture (n/a to new fields directly, but governs the
  rule-array reads via the existing `ReadRuleArray` template's already-established
  default-on-non-object behavior — reuse it, do not reinvent).
- Constitution §8 — total tweakability; the 4 new `MarkerRule` fields must be real, exposed
  settings, not hardcoded.

## Solution — shape
Each Stack is a bare array, e.g.:
```
MarkersStack: [ { Enabled, Hidden, Category, MinSlope, MaxSlope, ..., HydroMultiplier,
                  ReclaimDensity, MexDensity, SpawnPointCount, SymmetryUseGlobal, SymmetryMask,
                  Transform: {...} }, ... ]
PropsStack:   [ { Enabled, Density, MinSlope, ..., Transform: {...} }, ... ]
DecalsStack:  [ { Enabled, Density, MinSlope, ..., Transform: {...} }, ... ]
UnitsStack:   [ { Enabled, ArmyIndex, Count, MinSlope, ..., Transform: {...} }, ... ]
GlobalMarkerSettings: { GlobalIconAlloy, GlobalIconPlasma, GlobalIconSpawn, MarkerColorAlloy,
                        MarkerColorPlasma, MarkerColorSpawn, MarkerScaleAlloy, MarkerScalePlasma,
                        MarkerScaleSpawn }
```
Per-rule field spellings are **unchanged verbatim** from the current `PlacementRules` shape
(`BuildMarkerRuleJson` etc. already use PascalCase, `b`-prefix dropped) — this ticket relocates
the container, not the per-rule field names. `GlobalMarkerSettings`'s wire keys keep the
`Global*`/`Marker*`-prefixed spelling already ratified in `SANMAP_FORMAT_SPEC` Correction 7
(`GlobalIconAlloy`, `MarkerColorAlloy`, `MarkerScaleAlloy`, ...) even though the C++ field names
drop those prefixes (ARCH_11_GlobalMarkerSettings.md §11's naming note — wire spelling and C++ spelling diverge here by
design, same as `layerIndex`/`PropGroups` elsewhere in this format).

**Cardinality:** none of the four Stacks or `GlobalMarkerSettings` have a fixed count — designer-
authored, variable length, same as the current `PlacementRules` arrays. No padding/mismatch logic
needed (unlike `StratumGenerationSettings`'s fixed-9 requirement).

## Explicit out-of-scope
- **Any Group/Layer/LayerType hierarchy for the Stacks.** Correction 7 explicitly defers that
  design; this ticket implements only the flat-array interim shape it authorizes.
- **`MarkersStack_Migrate_V2_IO`/`PropsStack_Migrate_V2_IO`/`DecalsStack_Migrate_V2_IO`/
  `UnitsStack_Migrate_V2_IO`** — the backward-compat migration files that let an old file still
  carrying `mapGeneratorData.PlacementRules` load correctly. `IO_MIGRATION_SPEC` already anticipates
  these by name; building them is IO Architecture Expert / a separate ticket, not this one. Without
  that follow-up, a **pre-this-ticket `.sanmap` file loses its placement rules on import** until the
  migration ticket lands — acceptable for this ticket, but the human should be told this dependency
  exists before shipping without it.
- **Any change to `ScatterTransform`, `MarkerRule`'s existing fields, or `PropRule`/`DecalRule`/
  `UnitRule`'s shape** beyond the 4 new `MarkerRule` fields named above.
- **Wiring `GlobalMarkerSettings` into any UI tab** (icon/color/scale pickers) — PARAMS + IO shape
  only, per ARCH_11_GlobalMarkerSettings.md §11's own "shape only, not wiring" scope note.
- **Tightening Correction 7's contradictory "sub-key" wording in `SANMAP_FORMAT_SPEC.md` itself** —
  flagged to the ARCH Expert as a docs-only follow-up (ruling #2 above), not this ticket's job
  (only the ARCH Expert writes `sangen_arch_pack/`).

## Acceptance test
A `Params::MapRecipe` with non-default entries in all four rule vectors (including non-default
values for the 4 new `MarkerRule` fields) and a non-default `globalMarkerSettings` survives
export→import exactly through the 5 new top-level keys. Confirm `"PlacementRules"` no longer
appears anywhere under `document["mapGeneratorData"]` in an exported document (grep the JSON text,
not just call sites — same verification style STEP12 used). Confirm `MarkersStack`/`PropsStack`/
`DecalsStack`/`UnitsStack`/`GlobalMarkerSettings` appear as top-level siblings of
`mapGeneratorData`, not nested inside it. Confirm the existing `PlacementRules_PARAMS_Test.cpp`
coverage (symmetry override fields on `MarkerRule`/`PropRule`/`UnitRule`) still passes against the
relocated readers. Full `SanGenV2` build stays clean; every existing test continues to pass.

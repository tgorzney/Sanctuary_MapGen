# Work-Order — Step 25: give `mapName`/`mapCredits` a real home so they round-trip

*Constitution §2/§8. Executor: SanGen Coder. Independent of `STEP24_ImportNeverRefuses_IO.md` —
does not touch the migration runner, does not need ARCH ratification, can be dispatched
immediately. Design consult: Format Expert.*

## Root problem
`.sanmap` export (`src/io/MapExporter_Recipe_IO.cpp:87-88`) writes top-level
`document["name"] = options.mapName;` and `document["credits"] = options.mapCredits;`, sourced
from `MapExportOptions` (`src/io/MapExporter_IO.h:45-46`) — export-time-only UI session state
(`src/ui/FilesTab_Draw_UI.cpp:104-105`, `DrawTextInput` bound directly to
`state.exportOptions.mapName`/`mapCredits`). No importer anywhere reads these top-level keys back
(confirmed by grep across all `src/io/MapImporter_*.cpp` — zero top-level matches). Net effect:
opening a previously-exported `.sanmap` and re-exporting it silently resets the map's name and
credits to the `MapExportOptions` defaults (`"mapdef"`/`"Sanctuary Map Generator"`) — real,
designer-visible data loss the human's round-trip-fidelity goal explicitly wants closed.

Found alongside 11 other write-only export fields; those 11 are confirmed NOT real gaps (do not
re-investigate them in this ticket): `hasWater`/`waterLevel`/`waterDepth` duplicate data already
round-tripped via the legacy `mapGeneratorData.Water` block (`ReadWaterJson` reads
`Enabled`/`WaterLevelMax`/`DeepWaterDepthMax`); `length`/`heightmapResolution` are redundant
derivations of `geometry.mapSize` (already round-tripped via top-level `width`);
`fileVersion`/`mapVersion`/`shader`/`heightTransition`/`fadeDistance`/`fadeStartDistance` are
hardcoded literal constants in the exporter with no recipe-data source at all, so there is nothing
to import back even in principle. `name`/`credits` are the only two fields in this set backed by
real, mutable, designer-authored data with no import path.

## Ruled by this ticket (Format Expert consult)
1. **No existing "map metadata" struct** (confirmed: full `src/params/*.h` listing checked;
   `GeneralMapSettings_PARAMS.h` is explicitly scoped to `globalGravity` only, not a catch-all).
   `SANMAP_FORMAT_SPEC.md:12-13` groups `name`/`credits` under "Base," alongside `width`/`length`/
   `height` — plain flat top-level document fields, not a nested block. Add `mapName`/`mapCredits`
   as flat fields directly on `Params::MapRecipe`, siblings of `geometry`. No new header.
2. **Naming: `mapName`/`mapCredits`** — matches `MapExportOptions`'s existing field names 1:1
   (smooth rename-free wiring), no collision risk (`MapRecipe`'s other fields — `geometry`,
   `water`, `atmosphere` — carry no `map`-prefix stutter).
3. **Remove `MapExportOptions::mapName`/`mapCredits`; bind the UI directly to
   `recipe.mapName`/`recipe.mapCredits`.** This is the codebase's own established pattern, not a
   novel call: every comparable free-text metadata field (`Army::name` —
   `ArmiesTab_UI.cpp:83`, `MapArea::name` — `AreasTab_UI.cpp:100`, `GeoLayer::name`,
   `Stratum::appearance.name`) is a `DrawTextInput` bound directly to the live recipe field with
   no separate "session override" staging struct anywhere in `src/ui/`. `DrawExportSection`
   (`FilesTab_Draw_UI.cpp:104-105`) changes `state.exportOptions.mapName` →
   `recipe.mapName`, `state.exportOptions.mapCredits` → `recipe.mapCredits` — same shape as
   `DrawTextInput("Name", army.name, ...)`. `MapExportOptions::MapExportFileNames` stays
   untouched (genuinely export-run-only, never round-trips through the `.sanmap` document
   itself) — only `mapName`/`mapCredits` move.
4. **Import tier: unconditional top-level reads**, same tier as every other top-level reader this
   session (`ReadAreasJson`, `ReadAtmosphereJson`, etc.) — not gated behind `mapGeneratorData`.
   `SANMAP_FORMAT_SPEC.md`'s own "Base" grouping places `name`/`credits` at the same document root
   as `width`/`length`/`height`, already read at this unconditional tier.
5. **Non-empty invariant for `name` must survive the round trip.**
   `FilesTab_Draw_UI.cpp:101-103`'s `TextInputRules` for the Name field
   (`bAllowEmpty = false`, `fallbackText = "mapdef"`) is a UI-enforced invariant, not merely
   cosmetic — a `.sanmap` written by another tool (or hand-edited) with an empty or missing `name`
   key must not leave `recipe.mapName` empty on import. After `ReadJsonText(document, "name",
   outRecipe.mapName)`, if the resulting value is empty, set it to `"mapdef"` (mirroring the UI
   rule's own fallback text exactly, so import and UI agree on what "no name" defaults to).
   **`credits` gets no equivalent treatment** — an empty credits string is legitimate, real content
   (confirmed: every official Supreme Commander demo map's `credits` field is genuinely `""`);
   `ReadJsonText`'s ordinary unconditional overwrite is correct as-is, no fallback logic needed.

## Target files
- `src/params/MapRecipe_PARAMS.h` — add `std::string mapName = "mapdef";` and
  `std::string mapCredits = "Sanctuary Map Generator";` as flat siblings of `geometry` (ruling
  1/2). Keep these exact default values — they match `MapExportOptions`'s current defaults, so a
  brand-new, never-loaded recipe's export behavior is unchanged.
- `src/io/MapExporter_IO.h` — remove `mapName`/`mapCredits` from `MapExportOptions` (ruling 3).
- `src/io/MapExporter_Recipe_IO.cpp:87-88` — read from `recipe.mapName`/`recipe.mapCredits`
  instead of `options.mapName`/`options.mapCredits`.
- `src/io/MapImporter_IO.cpp` (`ParseSanmapJsonText`) — add unconditional
  `ReadJsonText(document, "name", outRecipe.mapName)` / `ReadJsonText(document, "credits",
  outRecipe.mapCredits)` calls, same tier/ordering as the other top-level readers (ruling 4), plus
  the empty-name fallback (ruling 5).
- `src/ui/FilesTab_Draw_UI.cpp:101-105` — rebind `DrawTextInput` calls from
  `state.exportOptions.mapName`/`mapCredits` to `recipe.mapName`/`recipe.mapCredits` (ruling 3);
  keep the existing `TextInputRules` (`bAllowEmpty = false`, `fallbackText = "mapdef"`) attached to
  the Name field, now operating on the recipe field directly.
- Any other call site constructing/reading `MapExportOptions::mapName`/`mapCredits` — grep
  `MapExportOptions` usages across `src/io/` and `src/ui/` to catch every reference (expect
  `FilesTab_Actions_UI.cpp`'s `RunRecipeExport` and test fixtures at minimum).
- `src/io/MapExporter_IO_Test.cpp`, `src/io/MapImporter_IO_Test.cpp`,
  `src/ui/FilesTab_Roundtrip_UI_Test.cpp` (already sets `state.exportOptions.mapName = "mapdef"`
  at line 60 — update to the new recipe-field location) — update/extend for the acceptance test
  below.

## Explicit out-of-scope
- The other 11 write-only fields identified alongside this ticket — confirmed non-issues (see
  Root problem), do not add importers for them.
- `STEP24`'s Unknown-Import bag / never-refuse mechanism — unrelated, independent ticket.
- Any change to `MapExportOptions::MapExportFileNames` — genuinely export-run-only, not document
  content.

## Layer & accuracy class
PARAMS (2 new flat fields) + IO (1 new unconditional reader pair) + UI (rebind 2 text inputs,
delete 2 now-dead struct fields). Accuracy class: Exact.

## Backend policy
N/A — no compute.

## ARCH rules invoked
- Constitution §2 (naming law) — `mapName`/`mapCredits` flat-sibling convention, matching the
  existing `bSymmetryUseGlobal`/`symmetryMask`-style flat-field precedent used throughout this
  session.
- Constitution §8 — settings exist in PARAMS from the moment they're settable; `mapName`/
  `mapCredits` move from UI-only session state to a real PARAMS home.

## Acceptance test
1. A `Params::MapRecipe` with a non-default `mapName`/`mapCredits` (including an empty
   `mapCredits`, matching real official-map content) survives export → import exactly.
2. A hand-authored/synthetic document with a missing or empty top-level `name` key imports with
   `recipe.mapName == "mapdef"` (the fallback), matching the UI's own enforced default.
3. A hand-authored/synthetic document with an empty top-level `credits` key (`""`) imports with
   `recipe.mapCredits == ""` — confirm NO fallback is applied here, unlike `name`.
4. The Files tab's Name/Credits text inputs, after opening a `.sanmap`, show that file's actual
   name/credits (not the old `MapExportOptions` defaults) — verify via
   `FilesTab_Roundtrip_UI_Test.cpp` or equivalent, not manual interaction.
5. Full `SanGenV2` build stays clean; every existing test continues to pass, including
   `MapExporter_IO_Test.cpp`, `MapImporter_IO_Test.cpp`, `FilesTab_Roundtrip_UI_Test.cpp`, and any
   test currently constructing a `MapExportOptions` with `mapName`/`mapCredits` set (must be
   updated to set `recipe.mapName`/`mapCredits` instead — grep for compile breaks, this is a
   removed-field change, not additive).

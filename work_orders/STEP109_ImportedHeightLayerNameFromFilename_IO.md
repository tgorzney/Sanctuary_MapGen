# STEP109 — Imported height layer's name = the map's filename, underscores replaced with spaces

**Layer:** IO. **Domain:** `MapImporter_HeightmapDecomposition_IO.cpp`, `MapImporter_IO.cpp`/`.h`.
**Sequence:** small, independent of STEP105's own scope; revises the same
`DecomposeBakedHeightmapIntoLayers` function STEP105 just landed (commit `aa199f8`).

## Root problem
`DecomposeBakedHeightmapIntoLayers`'s fresh-synthesis branch hardcodes
`importedGroup.name = "Imported Bake";` — every imported map's height layer/group
shows the same generic name in the Layer Editor, with no way to tell which map it
came from at a glance. The human wants the layer's name to be the `.sanmap`'s own
filename (not its `general.name`/JSON `"name"` field — the literal file stem), with
underscores replaced by spaces (e.g. `Pandemonium_Isthmus.sanmap` -> `"Pandemonium
Isthmus"`).

## Fix
`DecomposeBakedHeightmapIntoLayers` currently has no access to the file path it was
loaded from — only `Params::MapRecipe&`/`Data::MapFields&`. Thread the document's
own filename stem through:

1. `MapImporter::LoadSanmap` (`src/io/MapImporter_IO.cpp`) already resolves
   `result.resolvedDocumentPath` (the full `.sanmap` file path) before calling
   `LoadBakedFields(result.resolvedFolderPath, ...)` — `LoadBakedFields` is only
   ever given the FOLDER today, not the document path. Add the document path (or
   just its derived stem) as a new parameter to `LoadBakedFields`
   (`src/io/MapImporter_Fields_IO.cpp`/`MapImporter_IO.h`), threaded from
   `result.resolvedDocumentPath` at the one real call site.
2. `LoadBakedFields` forwards it into `DecomposeBakedHeightmapIntoLayers`
   (`MapImporter_HeightmapDecomposition_IO.h`/`.cpp`), which gains one new
   `const std::string& sourceFileName` parameter (the file's stem — e.g. via
   `std::filesystem::path(documentPath).stem().string()`, computed once at the
   `LoadBakedFields` call site, not duplicated).
3. New small pure helper, `DeriveLayerNameFromFileName` (same file), replaces
   underscores with spaces and falls back to `"Imported Bake"` for an empty/
   whitespace-only stem (never an empty layer name):
   ```cpp
   std::string DeriveLayerNameFromFileName(const std::string& fileStem) {
       std::string name = fileStem;
       for (char& character : name) if (character == '_') character = ' ';
       // trim any resulting leading/trailing space from a stem that started/ended with '_'
       const std::size_t first = name.find_first_not_of(' ');
       if (first == std::string::npos) return "Imported Bake";
       const std::size_t last = name.find_last_not_of(' ');
       return name.substr(first, last - first + 1);
   }
   ```
4. In the FRESH-SYNTHESIS branch only: `importedGroup.name =
   DeriveLayerNameFromFileName(sourceFileName);` (replaces the hardcoded literal).
   The RE-HYDRATION branch (re-opening a file that already has a populated
   `layerStack`) does NOT rename an existing group — a designer may have already
   renamed it, and re-deriving on every re-open would silently clobber that. Only
   the fresh-synthesis path (first import, no existing layer stack) sets the name.

Verify the real current signatures of `LoadBakedFields`
(`MapImporter_Fields_IO.cpp`, already extended twice by STEP101/105) and
`DecomposeBakedHeightmapIntoLayers` (`MapImporter_HeightmapDecomposition_IO.h`/
`.cpp`, revised by STEP105) before editing — this is a small, additive parameter
thread, not a redesign.

## Files touched
- `src/io/MapImporter_HeightmapDecomposition_IO.h`/`.cpp` — new parameter + helper.
- `src/io/MapImporter_Fields_IO.cpp` — `LoadBakedFields` gains + forwards the
  filename stem.
- `src/io/MapImporter_IO.h`/`.cpp` — `LoadBakedFields`'s declaration/call site.

## Explicit out-of-scope
- Renaming an already-populated layer group on re-open (re-hydration branch) —
  never touches an existing name.
- Any change to the `Params::Layer`'s own name (only the `GeoLayer`'s), unless the
  real shipped code turns out to key its Layer-Editor row label off the `Layer`'s
  own `name` field rather than the `GeoLayer`'s — verify which one is actually
  user-visible as "the layer" at implementation time and name the RIGHT struct's
  field; don't assume without checking.

## Acceptance test
Extend `MapImporter_HeightmapDecomposition_IO_Test.cpp`: importing a synthetic
document at a path whose stem is `"Test_Map_One"` produces a fresh-synthesis
`GeoLayer.name == "Test Map One"`. A stem of `"_leading"` produces `"leading"`
(trimmed). An empty/degenerate stem falls back to `"Imported Bake"`. Re-importing
after the group's name was hand-edited to something else does NOT revert it
(re-hydration branch never renames).

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.

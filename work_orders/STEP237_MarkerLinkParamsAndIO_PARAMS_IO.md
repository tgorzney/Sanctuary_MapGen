# STEP237 — `Params::MarkerLink` type, `linkIdentifier` back-references, IO

**Layer:** PARAMS + IO. **Domain:** new `src/params/MarkerLink_PARAMS.h`,
`src/params/MarkerLayerBundle_PARAMS.h`, `src/params/MarkerInstance_PARAMS.h`, new
`src/io/MapExporter_MarkerLink_IO.h/.cpp`, `src/io/MapImporter_MarkerLink_IO.h/.cpp`,
`src/io/MapExporter_Markers_IO.cpp`, `src/io/MapImporter_MarkerLayerBundle_IO.cpp`,
`src/io/MapImporter_MarkerGroups_IO.cpp`, `src/io/Sanmap_KnownTopLevelKeys_IO.cpp` (or equivalent),
`CMakeLists.txt`. **Sequence:** independent of STEP238 (disjoint files). STEP239 depends on this
ticket landing first.

Ratifies `ARCH_19_28_MarkerLinkParamsType.md`, `ARCH_19_29_LinkIdentifierBackReferences.md`,
`ARCH_19_30_MarkerLinksWireShape.md`. See `DESIGN_MarkerLink_R1.md` §3.3/§3.8 for full grounding.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. New `MarkerLink_PARAMS.h`:
   ```cpp
   struct MarkerLink {
       int identifier = -1;
       std::string name;
       bool bColorOverrideEnabled = false;
       float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
   };
   ```
   `MapRecipe` gains `std::vector<MarkerLink> markerLinks`.
2. `MarkerLayerBundle` gains `int linkIdentifier = -1` (organizational — which Link created this
   Group). `MarkerInstanceLayer` gains its own independent `int linkIdentifier = -1` (the actual
   color/visibility resolution key — never derived by walking `parentBundleIdentifier`, so a re-nest
   never silently changes which Link governs the Layer). Neither field on `MarkerRuleLayer` or
   `MarkerTransform`.
3. New `MapExporter_MarkerLink_IO.h/.cpp` / `MapImporter_MarkerLink_IO.h/.cpp` — wire array
   `MarkerLinks: [{ Identifier(int), Name(string), ColorOverrideEnabled(bool), Color({r,g,b,a}) }]`.
   No `SanGenVersion` bump, no migration unit (pure additive).
4. `LinkIdentifier (int)` merged into `MapExporter_Markers_IO.cpp`'s Bundle/Group JSON builders and
   into `MapImporter_MarkerLayerBundle_IO.cpp` / `MapImporter_MarkerGroups_IO.cpp`'s readers.
   Absent/dangling → `-1` ("not Link-bound"), soft, logged, no hard validation.
5. Register `MarkerLinks` in `Sanmap_KnownTopLevelKeys_IO` (or equivalent) so it parses into
   `recipe.markerLinks` instead of round-tripping under `UnknownImport`.
6. Add `MapExporter_MarkerLink_IO_Test.cpp` / `MapImporter_MarkerLink_IO_Test.cpp` to `CMakeLists.txt`
   (standard per-file IO round-trip test convention).

## Verify

- New IO round-trip tests: write then read back a `MarkerLinks` array and both merged
  `LinkIdentifier` fields, byte-for-byte.
- Dangling `LinkIdentifier` (references no `MarkerLink` entry) imports without error, resolves to
  `-1`-equivalent downstream (no IO-level validation needed).
- `UnknownImport` no longer swallows `MarkerLinks` after the `KnownTopLevelKeys` registration.
- Existing `MapExporter_Markers_IO_Test`, `MapImporter_MarkerLayerBundle_IO_Test`,
  `MapImporter_MarkerGroups_IO_Test` stay green.

## Out of scope

- Any UI surface for Links (STEP239, depends on this ticket).
- Propagation logic (read-and-resolve / cascade-write-on-rename) — STEP239, UI-layer only.

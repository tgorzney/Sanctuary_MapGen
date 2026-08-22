# Work-Order M1-6 — `.sanmap` / `mapGeneratorData` round-trip (IO)

*Constitution §7. Milestone M1 (Data model). Executor: **SanGen Coder in Claude Code**
(needs the project's JSON lib + a real `.sanmap` to validate keys — not sandbox-
verifiable). Status: spec ready; NOT yet implemented.*

## Title
Serialize/parse the `MapRecipe` to/from the `.sanmap` `mapGeneratorData` block.

## Root problem
The v2 `MapRecipe` (all the M1 PARAMS structs) must round-trip through the real
`.sanmap` format: load an existing map's `mapGeneratorData` into a `MapRecipe`, and
write a `MapRecipe` back out identically. This is the tiny payload the deterministic
shared-generation mode transmits (settings + seed regenerate the map).

## Target files
- `src/io/SanmapRecipe_IO.h` / `.cpp` (split methods across `SanmapRecipe_*_IO.cpp` if
  the ceiling is approached — this is a large serializer).
- `src/io/SanmapRecipe_IO_Test.cpp`.

## Layer & accuracy
`IO / BRIDGE`. The format seam — loads/saves only, never simulates (ARCH_03_ModuleBoundaries.md §3). GPU-free.

## Key constraints (read the existing code first)
- **Use the project's existing JSON approach.** Read `core/export/Export_Metadata.cpp`
  and `core/MapImporter.cpp` to see how `.sanmap`/`mapGeneratorData` is currently
  read/written and which JSON library is used; match it exactly so this compiles in the
  MSVC build. Do NOT introduce a new JSON dependency without confirming.
- **Match the real keys.** The exact `mapGeneratorData` JSON keys come from real
  `.sanmap` files and `SANMAP_FORMAT_SPEC` — validate against an actual map, not
  guesswork. Preserve unknown/extra keys on round-trip where feasible.
- **Honor the format truths** (`SANMAP_FORMAT_SPEC`): the coordinate flip
  (`world.z = length - z - 1`) on export / inverted on import; entity positions are
  absolute world/game units; `terrainMaxHeight` read from the map, not hardcoded.
- Validate all input (Constitution §6): bad/missing fields fall back to defaults + log,
  never crash.

## Mapping
Serialize/parse every field of `MapRecipe`: `Geometry` (mapSize/seed/terrainMaxHeight),
the `LayerStack` (GeoLayers → their `Layer`s with noise/reshape/blend), the marker/prop/
decal rule vectors, and `Water`. (Atmosphere once `Atmosphere_PARAMS` exists.)

## Acceptance test
1. **Round-trip identity:** a fully-populated `MapRecipe` → JSON → `MapRecipe` compares
   equal field-for-field.
2. **Real file:** load `mapGeneratorData` from a real official `.sanmap`, write it back,
   and confirm the generator settings survive (diff the two, ignoring key order).
3. Missing/corrupt fields fall back to defaults without crashing.
4. Builds clean in MSVC against the project's JSON lib.

## Out of scope
- The terrain `Textures/` payload (heightmap/masks) — that is export of DATA, a later
  work-order; this is the settings block only.
- Unit/prop/marker *instance* export (the resolved entities) — separate.

## Note to executor
This is the format seam; correctness = it round-trips a real map. Model on the existing
export/import; reconcile the v2 `MapRecipe` fields to the real `mapGeneratorData` keys.

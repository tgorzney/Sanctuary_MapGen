# B2 — parity fields (tiny, solo) + deferred-features log

*Runs after B, before the tabs. Single agent. Closes the two cheap v1 gaps that only need a
data field. Also records the features we are deliberately deferring so none are lost.*

## Do (two cheap fields with a real home)
1. **Per-layer `name`** — add `std::string name = "Layer";` to `Params::Layer`. Add a `TextInput`
   for it in the Layer Editor's per-layer header (batch-A `TextInput_UI`). Pure metadata — no
   stage consumes it.
2. **Scale Features to Map Size** — add `bool bScaleFeaturesToMapSize = true;` to
   `Params::Geometry`. Add a `Checkbox` on the Heightmap tab. Wire the noise/blend stage so that
   when set, layer frequency scales with map size (the v1 behavior). This is the only stage touch.

Register/extend tests, build `SanGenV2` + `SanGenV2App`, run `ctest`. Edit only: `Layer_PARAMS.h`,
`Geometry_PARAMS.h`, the noise/blend PROC stage, `LayerEditor_*`/`HeightmapTab_UI`, `CMakeLists.txt`.
Do NOT touch other tabs or `Application_*`.

## Deferred — tracked, NOT cut (need generation-engine work, revisit later)
These were v1 features that ARCH §5.2 deliberately evicted from `Params::Layer`; they are not
sliders and need real pipeline machinery, so they wait for their own work-orders:

- **Import RAW heightmap** — a layer sourced from an imported 16-bit image instead of noise.
  Needs: image-data field/home, the noise-blend stage to sample the image, IO load path.
- **Bake layer** — freeze a layer's computed output and reuse it. Needs a bake cache in the
  pipeline. (Layer Editor already has the dormant button + signal.)
- **Per-layer symmetry override** — a layer overriding the global symmetry mask. Needs the
  symmetry stage to read a per-layer mask, not just `MapRecipe::globalSymmetryMask`.
- **Durable Global Gravity** — currently in-session only (bulk-writes strata erosion gravity).
  Make it a persisted setting once save/load (WO D) exists, so it round-trips.

## Housekeeping (no action here)
- Soil physics binding to `Params::Stratum` (vs `Proc::MaterialPhysics`) — handled in C2, which
  owns `Stratum_PARAMS`.
- Relocating the erosion constants into a params file — later ARCH tidy; they're settable now via
  the pipeline seam.

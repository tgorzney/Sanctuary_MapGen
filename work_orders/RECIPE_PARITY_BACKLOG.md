# Recipe-Parity Backlog — settings that need a data-model home / engine wiring

*Compiled from the C1–C4 + D coder reports. The tabs are built and interactive, but many settings
are currently UI-layer state: they survive the session but do NOT serialize to the `.sanmap` and,
in some cases, no generation stage reads them yet. Nothing here blocks the app RUNNING (host = WO
E). This is the deliberate "wire settings into the recipe + save/load" phase that comes after E.*

## Tier 1 — needs a PARAMS home + serialization (settable now, but won't save/load)
- **Water level minimum** — `Params::Water` has no low bound; the level RangeSlider's low handle
  is tab-local.
- **Atmosphere** — no `Params::Atmosphere` exists; all 49 values are UI-only.
- **Areas** — no `Params::MapArea` type; the area rectangles don't round-trip.
- **Mask-tab stacks** — `MapRecipe` carries only the heightmap `layerStack`; the Detail-Normal /
  Tint / Holes / Smoothness stacks are caller-owned with no recipe home.
- **View/overlay state** — show-overlay toggles, environment-pack path, detail-normal size — no
  PARAMS home.
- **Accumulation per-stratum settings** — not serialized (bulk-write all 9 strata, read slot 0).
- **SymmetryDetection** — `{detectionTolerance, bSnapImperfectSymmetry}` has no aggregate home;
  give it a `MapRecipe` member in this phase (pass-by-ref is the stopgap).

## Tier 2 — missing engine features (need a PROC/pipeline stage, not just a field)
- **Symmetry algorithm group** (Fold / Blur / Cross-Fade / Superposition / Cylinder3D / Torus3D)
  — v1-only; no v2 fields AND no heightfield-symmetry stage to drive them. Whole feature.
- **Flow Slope-Adherence / Flow-Momentum** — no fields in `Proc::FlowAccumulationConstants`;
  bound to nothing today.
- **RAW heightmap import** as a layer source (from B2).
- **Bake layer** (from B2; dormant button exists).
- **Per-layer symmetry override** (from B2).
- **Bind imported heightfields into the pipeline** — importer loads them; no stage consumes them.
- **Entity export/import** — needs the coordinate flip + rotation conversion.
- **SupCom Lua import** — `SupComImporter_IO` module (tab drives an unbound seam today).
- **Durable Global Gravity** (from B2).
- **Soil seam** — the Layer Editor soil panel writes `Proc::MaterialPhysics` directly while
  Stratums writes the recipe; unify via PIPELINE seeding (same numbers, same stratum).

## Tier 3 — add a control (data exists, no UI)
- **Stratum slope-gate fields** — consumed by the Mask stage but reachable from no UI.

## Tier 2 additions (from WO E)
- **Six non-painting overlays** — the `[O]` toggles for Symmetry, Detail Normal, Tint, Holes,
  Smoothness, Atmosphere are held for column parity but drive nothing; `PreviewLayerKind` only
  has {HeightRamp, StratumSplat, Flow, Accumulation, Water, Slope}. Each needs an overlay
  composite stage before its toggle paints.

## Cleanup (from WO E)
- **Dead tabs** — old `LayersTab_UI` / `TerrainTab_UI` are no longer mounted but still compile
  and test. Delete once nothing references them.

## ARCH Expert calls (not blocking)
- **UI → IO dependency** — not in the ARCH §3.1 table, but already practiced (`FilesTab_UI`
  includes IO; `Application_Settings_UI` includes `AssetAtlasCache_IO`). Add an IO entry for UI,
  or state the exception.
- Where the promoted erosion/flow constants ultimately live (`ErosionFlow_PARAMS`) — later tidy.

## Sequencing
CD-int (link fix + register tests) → E (host: app runs) → **this backlog**, Tier 1 first (makes
the sanmap round-trip whole), then Tier 2 by value, Tier 3 last.

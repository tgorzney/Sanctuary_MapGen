[← ARCH index](ARCH.md) · SanGen ARCH §11. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 11. `Params::GlobalMarkerSettings` (ARCH ruling, completes `SANMAP_FORMAT_SPEC` Correction 7)

Fills the C++-shape gap in the already-ratified `.sanmap` `GlobalMarkerSettings` sub-key
(`SANMAP_FORMAT_SPEC` Correction 7, `PLACEMENT_SCATTER_SPEC` "IO wrapping") — map-wide default
icon/color/scale for the three resource marker kinds (Alloy/Plasma/Spawn), distinct in scope
from any single `Params::MarkerRule` (the same global-vs-per-rule distinction as
`Symmetry`/`SlopeDefaults` vs. their per-rule overrides). Confirmed against the legacy
reference `core/Parameters.h:79-87`.

- **New standalone file, `GlobalMarkerSettings_PARAMS.h`, sibling of `MarkerRule_PARAMS.h`**
  — not a member of `MarkerRule`, because it is map-scoped, not per-rule.
- **Shape:**
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
- **Naming:** icon fields are `iconName*`, not `icon*Path` — they are atlas-manifest name
  keys (`name → { page, uv-rect }`, `ASSET_LOADING_SPEC`), not file paths. Color/scale fields
  drop the redundant `Marker` prefix the legacy globals (`MarkerColorAlloy`,
  `MarkerScaleAlloy`, …) carried — the type's own name already scopes them, and the bare
  `color*`/`scale*` reads cleanly as `Params::GlobalMarkerSettings::colorAlloy`.
- **`Plasma` = Energy, a real planned resource type** (already ruled, `SANMAP_FORMAT_SPEC`
  Correction 7) — not the v1 invention `IO_PARITY_REPORT.md` Decision #5 flagged; keep all
  three Plasma-named fields.
- **Shape only, not wiring.** `MapRecipe_PARAMS.h` gaining
  `GlobalMarkerSettings globalMarkerSettings;` (flat sibling of `markerRules`, for now — the
  future `MarkersStack` Group/Layer wrapper, `PLACEMENT_SCATTER_SPEC`/`SANMAP_FORMAT_SPEC`
  Correction 7, may fold this inside it later; not designed here) and the matching `IO`
  round-trip are a separate coder work-order.

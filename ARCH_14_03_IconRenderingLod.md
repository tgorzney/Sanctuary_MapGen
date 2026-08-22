[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.3. **Only the ARCH Expert writes this file.**

### 14.3 Icon rendering — two-mode LOD, not constant-screen-size-only
R1's framing — markers are always constant-screen-size icons — is **wrong and is retired.** Each
layer switches between two draw modes at its own `thumbnailLodThresholdPixels` (default 5px,
tunable, Constitution §8):
1. **Thumbnail mode** (zoomed in enough) — the entity's raster thumbnail at its true
   world-footprint size: `screenSize = (baseFootprint * instance.scale) / worldUnitsPerCell *
   pixelsPerCell * view.ZoomScale()`. Scales with zoom, by design.
2. **Strategic icon mode** — when thumbnail mode would render below the threshold, switch to a
   fixed-size symbolic icon. Constant screen pixels below threshold — this is the only mode R1's
   retired assumption ever actually covered, now correctly scoped to this mode alone.

Real, currently-unsolved gaps, recorded so no coder papers over them with an invented default:
- **No world-footprint-size data exists anywhere in the codebase today** (`InstancedTransform`
  carries a scale *multiplier*, not an absolute size) — needs a new `templateIdentifier ->
  baseFootprintWidth/Depth` table, IO-layer, asset-derived not PARAMS-authored. Buildable now with
  a placeholder default per domain; real mesh-derived bounds are separately-scoped later work
  (§14.13 item 1).
- Today's prop thumbnail (`AssetAtlasCache_PropThumbnail_IO.cpp`) is a placeholder flat-shaded
  stand-in derived from a digest of the model bytes, not a real rendered view — its own header
  already says so; unchanged and out of scope here.
- A strategic icon per entity type is **new authored visual content**, not a second render of
  existing data. **Decided: bespoke per blueprint** — every `templateIdentifier` gets its own
  authored strategic icon, not a generic one-glyph-per-domain fallback. This is real
  authoring/asset-pipeline work, out of this ruling's scope, not a rendering detail.
- `IconAtlasEntry`/`IconAtlasManifest` (`IconGridWidget_UI.h`) stays one `iconId` -> one UV rect;
  do not widen it — its only other consumer, the icon-picker grid, wants exactly one slot. Add a
  **separate pairing lookup** the overlay renderer consumes: `templateIdentifier ->
  {thumbnailIconId, strategicIconId}`, each id still resolving through the existing single-slot
  manifest unchanged. The widget's own header already names this exact seam as anticipated.


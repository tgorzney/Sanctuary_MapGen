# Work-Order — Step 1: `.sanmap` export shipping bugs

*Constitution §7. First step of the ready-now IO-completion sequence. Executor: SanGen Coder.*

## Title
Fix two confirmed export-correctness bugs: `maskRemapMin`/`maskRemapMax` write the wrong
JSON shape, and `height` writes a fractional value into a format field the C# reader
types as `int`.

## Root problem
- `Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` are `float` scalars.
  `ARCH_07_02_MaterialProportionVsSurfaceWeight.md` §7.2 item 10 (ratified) widens these to `float[kStratumColorChannelCount]`
  (4 components), matching the format's real `Vector4` shape confirmed in
  `SanMap.Types.cs`. `src/io/MapExporter_Recipe_IO.cpp::BuildStratumLayersJson`
  currently writes `layer["maskRemapMin"] = stratum.maskRemapMinimum;` — a bare
  scalar where the format requires `{x,y,z,w}`. This is a hard type mismatch in
  every `.sanmap` this codebase exports today.
- `document["height"] = geometry.terrainMaxHeight;` in the same file writes a `float`
  directly into a JSON field the format types as `int` (`SanMap.cs:24`). A fractional
  `terrainMaxHeight` (a legal `Params::Geometry` value) does not survive Newtonsoft's
  coercion into a C# `int`.

## Target files
- `src/params/Stratum_PARAMS.h` — widen the two fields per the ratified ARCH §7.2 item 10.
- `src/io/MapExporter_Recipe_IO.cpp` — fix both write sites.
- `src/io/MapExporter_Recipe_IO_Test.cpp` (or wherever the export round-trip test for
  this file lives — check `MapExporter_IO_Test.cpp`) — extend coverage.

## Layer & accuracy class
IO / BRIDGE. Accuracy class: Exact (format-shape correctness, not a numeric tolerance).

## Backend policy
CPU only — JSON text I/O, not a dispatchable calculation.

## ARCH rules invoked
- `ARCH_07_02_MaterialProportionVsSurfaceWeight.md` §7.2 item 10 (the ratified field-widening — implement exactly as specified:
  `float maskRemapMinimum[kStratumColorChannelCount]` / `maskRemapMaximum[...]`,
  defaults `{0,0,0,0}` / `{1,1,1,1}`, reusing the constant already defined in
  `StratumAppearance_PARAMS.h`, which `Stratum_PARAMS.h` already includes).
- `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md` — confirms the `Vector4` ground truth.
- Constitution §6 — every value written must match the format's real type; no more
  silent scalar/Vector4 mismatch.

## Solution
1. In `Stratum_PARAMS.h`, change the two field declarations to fixed-size 4-float
   arrays with the ratified defaults. Do not touch any other field.
2. In `BuildStratumLayersJson`, write:
   ```cpp
   layer["maskRemapMin"] = { {"x", stratum.maskRemapMinimum[0]}, {"y", stratum.maskRemapMinimum[1]},
                              {"z", stratum.maskRemapMinimum[2]}, {"w", stratum.maskRemapMinimum[3]} };
   layer["maskRemapMax"] = { {"x", stratum.maskRemapMaximum[0]}, {"y", stratum.maskRemapMaximum[1]},
                              {"z", stratum.maskRemapMaximum[2]}, {"w", stratum.maskRemapMaximum[3]} };
   ```
3. Fix the height write:
   ```cpp
   document["height"] = static_cast<int>(std::lround(geometry.terrainMaxHeight));
   ```
   (Round, don't truncate — a designer-set 127.6 should land on 128, not silently drop to 127.)
4. Any other current reader/writer of `maskRemapMinimum`/`maskRemapMaximum` as a scalar
   (grep the whole tree) must be updated to the new array shape — do not leave a
   partially-migrated field.

## Explicit out-of-scope
- Reading `maskRemapMin`/`maskRemapMax` back on import — `stratumLayers` has no
  importer at all yet (a separate, larger fix, `SANMAP_FORMAT_SPEC.md` Correction 13,
  its own later step in the sequence). This work-order is export-side only.
- Any other `stratumLayers` field (albedo/normal/mask paths, tiling, etc.) — Correction 13.
- `StratumGenerationSettings` (Correction 12) — separate step.
- How the Mask stage's runtime kernel consumes the 4 remap channels — explicitly
  deferred to a future ARCH ruling per `ARCH_07_02_MaterialProportionVsSurfaceWeight.md` §7.2 item 10's own text. Not this
  ticket's concern; this ticket only fixes the on-disk shape.

## Acceptance test
Extend the existing export test: build a `Params::Stratum` with distinct, non-default
values in all 4 components of both `maskRemapMinimum` and `maskRemapMaximum`, export,
and assert the JSON document's `stratumLayers[i].maskRemapMin`/`maskRemapMax` are
objects with all 4 keys present and correct — not a bare number. Assert `height` in
the document is a whole number (e.g. via `document["height"].is_number_integer()` or
equivalent) for both an integer and a fractional `terrainMaxHeight` input.

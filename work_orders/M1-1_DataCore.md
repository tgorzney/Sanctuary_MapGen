# Work-Order M1-1 — data-model core: `FloatField_DATA` + `Geometry_PARAMS`

*Constitution §7. Milestone M1 (Data model). Status: implemented + verified (ALL PASS).*

## Root problem
The v2 data model is greenfield. Start the DATA/PARAMS split with the two most-used
pieces: the computed 2D float grid (replaces `Mask2D`/`FloatMask`) and the core map
geometry settings (part of the `GenerationParams` god object being dismembered).

## Target files
- `src/data/FloatField_DATA.h` (+ test) — computed field grid.
- `src/params/Geometry_PARAMS.h` (+ test) — map dimensions + seed + vertical extent.

## Layer & accuracy
`DATA` (computed output) and `PARAMS` (settings). No behavior beyond accessors/derived
helpers; no GPU handles (ARCH_03_ModuleBoundaries.md §3).

## Solution
- `FloatField`: row-major contiguous `float` grid; `Resize/Fill/Get/Set/At/Data`,
  `SampleBilinear` (edge-clamped). SoA-friendly, DATA-pure.
- `Geometry`: `mapSize`, `seed`, `terrainMaxHeight` (game units, read from the map — not
  the hardcoded 128); `VertexSize()`=mapSize+1, `VertexCount()`, `IsValid()`.

## Acceptance — PASSED
FloatField: dimensions/fill/get-set/`At`/row-major-index/bilinear (corner, midpoint,
center, clamp)/resize/empty-safe — **ALL PASS under ASan+UBSan**, 65-line header.
Geometry: VertexSize/Count + validity — ALL PASS.

## Out of scope
Heightfield/mask/flow field wrappers (thin over FloatField), the layer-stack PARAMS, and
the `.sanmap` round-trip — subsequent M1 work-orders.

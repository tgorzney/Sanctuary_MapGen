# Work-Order M1-2 — enums + `Layer_PARAMS` + `MapFields_DATA`

*Constitution §7. Milestone M1 (Data model). Status: implemented + verified (ALL PASS).*

## Root problem
Continue the god-object split: the shared enums, one height layer's settings (the
`NoiseLayer` replacement, settings-only per ARCH_05_GodObjectDismemberment.md §5.2), and the computed-field container
that replaces the scattered cached maps of `GenerationResult`.

## Target files
- `src/params/GenerationEnums_PARAMS.h` — NoiseType/FractalType/HeightBlendMode/
  ImportedMaskMode, fully spelled (`FractionalBrownian`, not `FBm`).
- `src/params/Layer_PARAMS.h` — one layer's noise + reshape + blend settings.
- `src/data/MapFields_DATA.h` (+ test) — heightfield + flow + accumulation + 9 material
  masks as `FloatField`s.

## Layer & accuracy
`PARAMS` (settings) and `DATA` (computed output). No behavior beyond `Resize`. DATA
takes plain ints for sizing (no PARAMS coupling, ARCH_03_ModuleBoundaries.md §3).

## Solution
`Layer` carries only identity/noise/reshape/blend (image-bake, per-layer erosion,
placement, physics tags evicted per §5.2). `MapFields::Resize(vertexSize)` sizes every
field including the 9 stratum masks.

## Acceptance — PASSED
Layer defaults; enum distinctness; `MapFields` starts unsized, `Resize(257)` sizes
heightfield/flow/accumulation and all 9 masks to 257². **ALL PASS under ASan+UBSan**;
files 27/42/35 lines.

## Out of scope
The full layer STACK container + GeoLayer grouping, the marker/prop rule PARAMS, and the
`.sanmap` round-trip — subsequent M1 work-orders.

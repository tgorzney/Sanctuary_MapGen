# Work-Order M1-3 — `GeoLayer_PARAMS` + `LayerStack_PARAMS`

*Constitution §7. Milestone M1 (Data model). Status: implemented + verified (ALL PASS).*

## Root problem
The editable terrain recipe needs its grouping structure: GeoLayers (groups of height
layers with group-level options) and the top-level stack with the Separate/Unified
simulation toggle (LAYER_SYSTEM_SPEC).

## Target files
- `src/params/GenerationEnums_PARAMS.h` — extended with `GeoLayerMode {Material,Shaper}`
  and `SimulationGrouping {Separate,Unified}`.
- `src/params/GeoLayer_PARAMS.h` — a group: name, enabled, mode, erode-below, blend,
  owned stratum, and its `Layer`s.
- `src/params/LayerStack_PARAMS.h` (+ test) — ordered GeoLayers + grouping toggle +
  `GetFlatLayers()` / `TotalLayerCount()`.

## Layer & accuracy
`PARAMS` (settings). No behavior beyond the flatten helper.

## Solution
`GetFlatLayers()` returns the enabled layers of enabled groups, in stack order (a
transient pointer view — valid until the stack is modified). `TotalLayerCount()` counts
all layers regardless of enabled state.

## Acceptance — PASSED
A stack with a disabled layer and a disabled group flattens to exactly the enabled
layers in order; total count includes disabled; default grouping is Unified. **ALL PASS
under ASan+UBSan**; files 25/36 lines.

## Out of scope
Marker/prop/decal rule PARAMS, the water/atmosphere settings, and the `.sanmap` round-
trip — subsequent M1 work-orders.

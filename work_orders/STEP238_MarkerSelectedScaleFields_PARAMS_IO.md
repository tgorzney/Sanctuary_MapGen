# STEP238 — `GlobalMarkerSettings` per-type selected-icon-size fields

**Layer:** PARAMS + IO. **Domain:** `src/params/GlobalMarkerSettings_PARAMS.h`,
`src/io/MapExporter_MarkersStack_IO.cpp`, `src/io/MapImporter_MarkersStack_IO.cpp`. **Sequence:**
independent of STEP237 (disjoint files). STEP240 depends on this ticket landing first (and on
STEP236, already complete).

Ratifies `ARCH_19_32_MarkerSelectedScaleFields.md`. See `DESIGN_MarkerLink_R1.md` §4.3.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

`GlobalMarkerSettings` gains, strict mirror of the existing `scaleAlloy/Plasma/Spawn`:
```cpp
float scaleSelectedAlloy  = 0.50f;
float scaleSelectedPlasma = 0.50f;
float scaleSelectedSpawn  = 0.50f;
```
New resolver `ResolveMarkerGroupSelectedTypeScale(groupName, settings)`, strict mirror of the
existing `ResolveMarkerGroupTypeScale` (unmatched-name fallback `1.0f`, same as the base resolver).

Wire keys: `MarkerScaleSelectedAlloy`/`MarkerScaleSelectedPlasma`/`MarkerScaleSelectedSpawn`
(preserves the established `Marker<Field><Type>` template) in `MapExporter_MarkersStack_IO.cpp` /
`MapImporter_MarkersStack_IO.cpp`, alongside the existing `scaleAlloy/Plasma/Spawn` handling. No
`SanGenVersion` bump.

## Verify

- IO round-trip test for the three new fields.
- `ResolveMarkerGroupSelectedTypeScale` unit test: known type names resolve correctly, unmatched
  name falls back to `1.0f`.
- Existing `MapExporter_MarkersStack_IO_Test`, `MapImporter_MarkersStack_IO_Test` stay green.

## Out of scope

- UI controls for these fields, and the render-consumer site that actually applies the selected
  scale on canvas (§4.4 — exact site TBD by direct read) — STEP240, depends on this ticket.
- `[0.25, 2.0]` clamp — UI-only enforcement, STEP240.

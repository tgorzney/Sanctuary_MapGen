[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.32. **Only the ARCH Expert writes this file.**

### 19.32 New `GlobalMarkerSettings` fields `scaleSelectedAlloy/Plasma/Spawn` — ratified, per-Type-section base+selected pair
Responds to `work_orders/DESIGN_MarkerLink_R1.md` §4.3/§5 item 5. **Ratified as designed —
naming, placement, and the per-Type-section (not tab-wide) reading are all confirmed correct.**

```cpp
// GlobalMarkerSettings (GlobalMarkerSettings_PARAMS.h) gains, strict mirror of
// scaleAlloy/Plasma/Spawn's existing shape/placement — no 4th-field deviation needed here (unlike
// §19.17's selectColorDefault): ResolveMarkerGroupTypeScale's own unmatched-name fallback is
// already 1.0f, a genuine no-op multiplier that works identically for a "selected" scale as it
// does for the base one — no white-vs-white-style ambiguity a COLOR fallback has.
float scaleSelectedAlloy  = 0.50f;   // same default as scaleAlloy — "selected" starts equal to base
float scaleSelectedPlasma = 0.50f;
float scaleSelectedSpawn  = 0.50f;

// New resolver, strict mirror of ResolveMarkerGroupTypeScale, same name-matching vocabulary
// (Spawn/Spawns, Alloy/Alloys, Plasma/Plasmas), unmatched name -> 1.0f, the same multiplicative
// no-op convention already established:
inline float ResolveMarkerGroupSelectedTypeScale(const std::string& groupName,
                                                 const GlobalMarkerSettings& settings);
```

**Per-Type-section base+selected pair confirmed — not a single tab-wide default+selected pair.**
Item 4's own scope is the per-Type-section header row, which is ALREADY per-type
(`scaleAlloy`/`scalePlasma`/`scaleSpawn`, three independent fields, never one shared default) —
the minimal-surprise extension is a same-shaped second field per type, not a new tab-wide concept
the row has no other precedent for. This also lets "selected size" mean something concrete at
render time: *this marker type's* icon grows/shrinks when *an instance of that type* is selected,
not a single global multiplier applied indiscriminately across Alloy/Plasma/Spawn.

**Range clamp `[0.25, 2.0]`** applies to this pair's own `DialRange`/slider-range at the UI call
site — a NEW, separate, narrower range value scoped to just this row, not a change to the shared
`iconScaleRange` field the unrelated per-Layer `iconScale` control still uses at its own, wider
bounds (`{0.1f, 10.0f}`). No PARAMS-level clamp — matches this format's existing "no range
validated on import, UI enforces at the edit surface" posture for every comparable scalar
(`scaleAlloy`/`Plasma`/`Spawn` themselves carry none either).

**Wire spelling — confirmed, preserving the established `Marker<Field><Type>` template, correcting
the design's original strawman.** The shipped precedent for this exact field family is wire
`MarkerScaleAlloy`/`MarkerScalePlasma`/`MarkerScaleSpawn` → C++ `scaleAlloy`/`scalePlasma`/
`scaleSpawn` (the `Marker` prefix drops on the C++ side only, since `GlobalMarkerSettings`'s own
type name already scopes it; wire keeps the full prefix, ARCH §11). The design's own
`SelectedScaleAlloy`-style strawman would have broken that template by moving `Selected` ahead of
`Scale` — rejected:
```
Wire:  MarkerScaleSelectedAlloy / MarkerScaleSelectedPlasma / MarkerScaleSelectedSpawn
C++:   scaleSelectedAlloy / scaleSelectedPlasma / scaleSelectedSpawn
```
`Selected` slots between `Scale` and the type name, matching where `scaleSelected*`'s own C++ name
already places it — the wire and C++ spellings differ only by the `Marker` prefix, exactly as
every other field in this struct already does.

**Additive, no `SanGenVersion` bump** — same precedent class as `selectColorAlloy/Plasma/Spawn`
(§19.17), the most recent addition to this exact struct.

**IO home:** `MapExporter_MarkersStack_IO.cpp`/`MapImporter_MarkersStack_IO.cpp`, alongside the
existing `scaleAlloy/Plasma/Spawn` handling — not a new file, per the IO Architecture Expert's own
advisory ruling (design §3.8), unchanged by this ratification.

**Render-consumer gap — real, flagged, not ruled on here (out of this section's scope).** Where in
the render pipeline `ResolveMarkerGroupSelectedTypeScale`'s result actually gets folded into a
drawn icon's scale is a coder-ticket-time question, not a PARAMS/wire-shape one — the candidate-
scale composition site the design flagged (`MapCanvas_IconLayer_CullManual_UI.cpp:202`) runs
before selection state is known, so the fold-in point is wherever `bSelected` is already resolved
downstream (the same tint-priority stage §19.18/§19.19 occupy). Left for the coder work-order to
confirm by direct read at dispatch time, per Constitution §8.4 — not guessed here.

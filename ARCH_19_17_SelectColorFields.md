[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.17. **Only the ARCH Expert writes this file.**

### 19.17 `GlobalMarkerSettings` select-color fields — `selectColorAlloy/Plasma/Spawn` strict-mirror, `selectColorDefault` a signed-off, explained 4th-field deviation
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s "Per-type select-color PARAMS
field" section. **Ratified as designed, including the deviation.**
```cpp
// GlobalMarkerSettings (GlobalMarkerSettings_PARAMS.h) gains:
float selectColorAlloy[4]   = {1.0f, 1.0f, 0.0f, 1.0f};
float selectColorPlasma[4]  = {1.0f, 1.0f, 0.0f, 1.0f};
float selectColorSpawn[4]   = {1.0f, 1.0f, 0.0f, 1.0f};
float selectColorDefault[4] = {1.0f, 1.0f, 0.0f, 1.0f};   // signed-off deviation, see below
```
`selectColorAlloy/Plasma/Spawn` strictly mirror the existing 3-field shape/placement of
`colorAlloy/Plasma/Spawn` and `iconNameAlloy/Plasma/Spawn` (`GlobalMarkerSettings_PARAMS.h:14-24`)
— no ruling needed beyond "follow the established pattern."

**`selectColorDefault` sign-off.** The existing 3-field pattern's own resolver,
`ResolveMarkerGroupTypeTintColor`, deliberately falls back to opaque white for any unmatched group
name (`GlobalMarkerSettings_PARAMS.h:38`, "the established 'unset' convention," STEP115 ruling #5) —
a genuine no-op/no-opinion convention, correct for "no special type color." That same fallback is
WRONG for a select color: confirmed by direct read, an unmatched-name marker's normal (unselected)
tint already resolves to opaque white via that fallback absent a per-layer color override, so a
select tint that ALSO fell back to white would make "selected" visually indistinguishable from
"unselected" for any Generic/Expansion/free-form group name — a real correctness gap, not a
cosmetic one, for exactly the class of `markerTypeName` values §19.14 just ruled must render
correctly (any free-form name, not only Alloy/Plasma/Spawn). A hardcoded literal fallback (bypassing
PARAMS) was considered and rejected: Constitution §8 requires a user-visible constant of this kind
be a tweakable, not a literal. `selectColorDefault` is ratified as the one deliberate, explained
departure from strict 3-field mirroring in this struct — recorded here as a conscious decision, not
a silently-added 4th field.

New resolver `ResolveMarkerGroupSelectTintColor(groupName, settings, outR, outG, outB)`, co-located
in `GlobalMarkerSettings_PARAMS.h`, mirroring `ResolveMarkerGroupTypeTintColor`'s exact
name-matching vocabulary (Spawn/Spawns, Alloy/Alloys, Plasma/Plasmas) but resolving an unmatched
name to `settings.selectColorDefault` instead of hardcoded white.

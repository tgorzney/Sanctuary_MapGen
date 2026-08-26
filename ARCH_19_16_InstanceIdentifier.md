[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.16. **Only the ARCH Expert writes this file.**

### 19.16 `MarkerTransform::instanceIdentifier` — ratified, applies §1.9, global uniqueness, legacy-backfill mirrors `layerId`'s precedent
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` Open Q2. **Ratified as designed.**
```cpp
// MarkerTransform (MarkerInstance_PARAMS.h) gains:
int instanceIdentifier = -1;   // stable, GLOBALLY unique across every MarkerInstanceGroup's
                                 // transforms (not per-group) — never reused, -1 = unassigned
```
Spelled `instanceIdentifier`, not bare `identifier` — `MarkerTransform` already carries
`layerIndex`/`symmetryGroupIdentifier` (confirmed, `MarkerInstance_PARAMS.h:57-61`), neither a
stable-identity field; a third bare `identifier` alongside two other ints would be ambiguous at the
call site. Correctly spelled per §1.9 from day one — no abbreviation, qualified-noun form.

Wire key **`"InstanceIdentifier"`**. Additive, no `SanGenVersion` bump — same precedent class as
`layerIndex`/`symmetryGroupIdentifier`/`iconNameOverride` merging into
`markers[type].transforms[name]` (§19.11).

Minting: a new `NextMarkerInstanceIdentifier(const std::vector<Params::MarkerInstanceGroup>& markers)`
scanning `max(instanceIdentifier) + 1` across **every** group's transforms — same shape as
`NextMarkerLayerId`, but the walk is two-level (group, then transform), since identity here is
roster-wide, not per-layer.

**Legacy-backfill on import — ratified to mirror `layerId`'s own already-shipped precedent exactly**
(`MapImporter_Markers_IO.cpp:121`, `layer.layerId = static_cast<int>(outRecipe.markerLayers.size())`
— sequential assignment by arrival order when the wire key is absent). For `instanceIdentifier`: when
`"InstanceIdentifier"` is absent on a transform, assign the next value of a running counter starting
at 0, incremented once per transform, threaded across the ENTIRE nested group→transform import walk
(never reset per group) — every legacy transform in the file receives a fresh, globally-unique id in
encounter order. Additive, no version bump, same "no legacy risk, the field never previously
existed" posture §19.4 already applies to `parentBundleIdentifier`/`assemblyIdentifier`.

This is a second, independent numeric identity alongside `MarkerTransform::name` —
`MakeNamesUnique`'s existing name-based repair is untouched, unreplaced, still the wire-key/dictionary
identity. `instanceIdentifier` exists solely for stable UI-selection addressing across frames (the
reason this round needs it) and carries no round-tripping/export-key role of its own.

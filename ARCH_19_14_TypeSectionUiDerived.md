[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.14. **Only the ARCH Expert writes this file.**

### 19.14 The Type-section tier is UI-derived — dynamic enumeration over `markerTypeName`, not a stored `Params` container
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` Open Q1 / item 2. **Ratified as
designed.** No `Params::MarkerTypeSection` struct, no new top-level wire array. The Markers tab
enumerates the section list itself, every frame, from the union of `markerTypeName` values actually
present in the live recipe: `recipe.markerLayerBundles[*].markerTypeName` ∪
`recipe.markerRuleLayers[*].markerTypeName` ∪ `recipe.markerLayers[*].markerTypeName` (deduped) —
**not** `MarkerInstanceGroup::name`, a different axis (manual roster grouping, not Layer/Bundle
type-scoping).

**Why dynamic is the only correct shape.** `markerTypeName` (§19.3) is already ratified as an open,
free-form string space — "same as `MarkerInstanceGroup::name`... NOT `MarkerCategory`" — a different
axis from `GlobalMarkerSettings`, confirmed by direct read to be a genuinely *closed*, fixed 3-field
struct with a hardcoded white/1.0 fallback for anything else (`GlobalMarkerSettings_PARAMS.h:14-24,
32-52`). A hardcoded 3-tab Type-section UI would make any Bundle/Layer whose `markerTypeName` sits
outside {Alloy, Plasma, Spawn} — already legal today per §19.3 — unrenderable in the tab. Dynamic
enumeration is the only shape consistent with `markerTypeName`'s own already-ratified open-set
contract; a closed-set UI over an open-set data field is a real correctness gap, not a simplification.

**Ordering — binding, not left to implementation whim.** Alloy, Plasma, Spawn first, in that fixed
order (matches `GlobalMarkerSettings`' own field order and its resolvers' existing name-matching
vocabulary, `GlobalMarkerSettings_PARAMS.h:35-51`), then every other distinct value present,
alphabetical, then a final `"(Unassigned)"` bucket for `markerTypeName == ""` (every pre-this-round
Bundle/Layer, and any hand-edited file).

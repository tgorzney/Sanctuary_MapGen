[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.21. **Only the ARCH Expert writes this file.**

### 19.21 `MarkerRule::category` vs. `markerTypeName` — two permanently independent concepts, closed
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s own "resolved, do not
re-litigate" framing (its item 2, restated in its ARCH-rulings item 13). **Ratified, closing the
door.** `MarkerRule::category` (`MarkerRule_PARAMS.h:18`,
`enum class MarkerCategory { Generic, Spawn, Alloys, Expansion }`) is closed-enum, per-rule
AI-analysis metadata describing what a procedural rule generates. `markerTypeName` (§19.3, §19.13)
is an open, free-form string, Layer/Bundle-level UI-section-scoping metadata describing how the
Markers tab organizes and displays a Layer or Bundle. They share no storage, no resolution path, and
no cardinality constraint — a `markerTypeName` value need not be one of `MarkerCategory`'s four
names, and nothing enforces that it is, per §19.14's own open-set ruling.

**No future ticket collapses these two concepts into one field or one enum.** A proposal to do so is
out of conformance with this ruling and must come back to ARCH as an explicit amendment, never a
silent merge.

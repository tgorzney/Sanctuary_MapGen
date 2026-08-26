[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.2. **Only the ARCH Expert writes this file.**

### 19.2 Domain-touching-vs-pure-mechanics genericity split — ratified as the general rule for all future Group/Bundle work (Props, Decals, NavMesh)
**Ratified as designed, no correction.** The design's split is exactly right and is promoted from
"this ticket's reasoning" to **standing law governing every future per-domain Bundle**:

- **Domain-touching logic** — anything that reads a real `Params::` field, resolves membership,
  or round-trips through IO — gets **its own per-domain repeated struct and per-domain repeated
  pure-function family.** `MarkerLayerBundle` today; `PropLayerBundle`/`DecalLayerBundle` later,
  independently written, not templated, not sharing a base class, not discriminated by a `domain`
  enum inside one shared table. This mirrors the already-shipped precedent this codebase already
  chose for the identical shape of problem — `PropInstanceLayer`/`DecalInstanceLayer`/
  `MarkerInstanceLayer` (three independent near-identical structs) and
  `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId` (two independent bodies,
  §3.5) — not invented fresh for this ticket.
- **Pure container/graph/UI mechanics with zero domain-field access** — tree render,
  expand/collapse, drag-to-reparent, cycle-detection over bare id/parent-id pairs — gets **one
  shared C++ template or accessor-callback-parameterized function.** `TreeListWidget_UI<T>`
  (§19.7) and the cycle-check shape (§19.8) are the concrete instances; `DraggableList<T>`/
  `ApplyDraggableListSignal<T>` (`DraggableListWidget_UI.h`) is the pre-existing proof this
  pattern is already accepted in this codebase, not a new liberty granted for this ticket.

**Why this is the correct general rule, not just a Markers-specific convenience.** A single
mixed-domain `Groups: [{id, name, parentGroupId, domain}]` table (the alternative the brief's own
"universal, reusable Group mechanism" framing raised as an open question) would need runtime
domain-filtering to reconstruct a domain-scoped tree, breaks the established "array order is the
layer's identity" convention `PropGroups`/`DecalGroups`/`MarkerGroups` already rely on, and cannot
express a `markerTypeName`-equivalent scoping field uniformly across domains without a
union/variant PARAMS shape this codebase uses nowhere else. The per-domain-struct answer costs
more lines (a second, third file later) in exchange for zero cross-domain coupling, zero variant
types, and a straight-line match to §1.5's "one primary type per file" ceiling. Genericity lives
in the mechanism (the widget, the cycle predicate), never in the data shape — this is the
dividing line future Props/Decals/NavMesh Bundle work-orders apply without re-asking.

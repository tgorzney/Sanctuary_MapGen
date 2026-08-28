[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.4. **Only the ARCH Expert writes this file.**

### 20.4 Drag-gesture / selection substrate — **gated on a separate UI Expert design round, not yet done; do not dispatch a coder work-order against this subsection alone**
Do not hand-mirror `MarkerDragGesture_UI`/`MarkerDragGestureState` into `PropDragGesture_UI`/
`DecalDragGesture_UI`. That state machine (one-shot correspondence table, cardinality tracking,
ghost/cascade-delete, Spawn-group-freeze special case) is large and easy to get subtly wrong on
a second and third hand-written copy — unlike `§20.1`'s small PARAMS structs, where duplication
cost is near zero and is the *correct* choice.

`§19.2` lists **"drag-to-reparent"** by name as a pure-mechanics example that gets one shared,
accessor-parameterized template (`TreeListWidget_UI<T>`, and the pre-existing
`DraggableList<T>`/`ApplyDraggableListSignal<T>` reorder-drag, both already proof this pattern
is accepted here). A drag-to-reposition-with-symmetry-correspondence gesture is the structurally
identical class of problem — a mouse-down/move/up state machine over abstract
position/parent-relationship fields — so it should be **redesigned the same way
`TreeListWidget_UI` was built**: genericized via accessor callbacks (get/set position, get
symmetry-group-identifier, get layer-index) so the core algorithm carries **zero concrete
`Params::` type**, moving it into the shared-template bucket instead of the per-domain
hand-written bucket. Under that redesign, Markers' Spawn-army-cardinality-freeze special case
becomes a pluggable predicate/callback, not baked-in Marker policy the generic core has to know
about.

**Explicitly unify with the separately-paused canvas click/box-select initiative.** Building a
generic drag substrate and a generic click/box-select substrate as two independent efforts risks
solving the same underlying problem twice — "resolve which abstract instance the
cursor/selection-box touches, across Markers/Props/Decals/(eventually Units)." Both should be
one UI Expert design-round consult, not two.

**This is a ruling on shape only — the concrete class design is explicitly NOT ruled here.**
It is routed to a UI Expert design round (the same two-round process
`DESIGN_MarkerLayerSymmetry_R1`/`_R2` used before `MarkerDragGesture_UI` itself was ratified),
covering both the drag-reposition substrate and the paused click/box-select work together. No
coder work-order may build Prop/Decal drag or selection UI ahead of that design round landing
and being ratified into its own new ARCH subsection.

**Does not block `§20.1`/`§20.2`/`§20.3`/`§20.5`/`§20.6`.** Shipping the PARAMS+IO fields
(symmetry, grid-snap, color-override) with no consumer/writer UI yet is an already-accepted,
precedented sequencing here — `MarkerInstanceLayer::symmetry`'s own shipped comment states
exactly this posture ("No consumer/writer UI exists yet... this ticket only gives the field its
PARAMS+IO home"). The Props/Decals PARAMS/IO/Type-Section/Bundle-tree work proceeds
independently of this gate.

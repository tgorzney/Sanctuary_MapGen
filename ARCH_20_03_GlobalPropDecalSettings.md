[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.3. **Only the ARCH Expert writes this file.**

### 20.3 `GlobalPropSettings`/`GlobalDecalSettings` — new PARAMS types, scoped to what has a real analog (not a blind mirror of `GlobalMarkerSettings`)
New types, not folded into `GlobalMarkerSettings` — Props/Decals are a structurally different
axis (real, asset-derived blueprints vs. abstract resource markers with no blueprint of their
own). The field set is **scoped to what has a real semantic analog for each domain**, not a
mechanical copy of every `GlobalMarkerSettings` field:

- **No `iconName*` fields on either type.** `GlobalMarkerSettings`'s icon-name fields exist
  because Alloy/Plasma/Spawn are abstract markers with no real game blueprint, so a manual icon
  choice is meaningful data. Every Prop/Decal instance already resolves a real icon from its own
  `blueprintPath`/sanpack asset — a "default icon name" field on either new type would be dead,
  never-branched data (Constitution §8 tweakability does not require inventing a knob nothing
  reads). Omit it from both.
- `GlobalPropSettings`: `float colorProp[4]`, `float colorReclaim[4]` — mirrors
  `colorAlloy`/`colorPlasma`'s shape, one field per the two ratified Type-section values.
  `scaleProp`/`scaleReclaim` are **not** ruled on here — left to the UI Expert to add only if a
  per-type default icon-scale concept proves meaningful; not required to unblock this ticket.
- `GlobalDecalSettings`: **one** field, `float colorDecal[4]`. With exactly one Type-section
  value, a name-matching resolver (`ResolveMarkerGroupTypeTintColor`'s shape) would have zero
  real branches — skip building one at all. The color-override-fallback consumer reads
  `recipe.globalDecalSettings.colorDecal` directly. This is a deliberately cheaper shape for
  cardinality-1, not an inconsistency with `GlobalPropSettings`' resolver-based shape.
- **`selectColor*` fields are deferred**, not added now. `§19.17`'s Marker precedent
  (`selectColorAlloy`/`Plasma`/`Spawn`/`Default`) exists to serve an already-built selection
  mechanism; Props/Decals have no selection mechanism at all yet (§20.4 — gated, not built).
  Adding `selectColorProp`/`selectColorReclaim`/`selectColorDecal` now would be inert
  speculative fields with no near-term consumer, unlike `MarkerLayerBundle::assemblyIdentifier`
  (signed off as inert-until-Assembly specifically because Assembly is a concretely designed,
  near-term feature — Props/Decals selection is not, as of this ruling). A future ticket adds
  these fields alongside whatever selection design §20.4 eventually produces, not before.

**File home:** one new shared file, `GlobalPropDecalSettings_PARAMS.h`, holding both types —
both are small (one to two float[4] fields each), and Props+Decals are being designed together
in this ratification, matching the multi-domain-per-file convention `§20.1` already applies.

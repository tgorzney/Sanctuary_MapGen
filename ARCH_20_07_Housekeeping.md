[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.7. **Only the ARCH Expert writes this file.**

### 20.7 Naming / file-size / `MapRecipe` housekeeping
- No abbreviations; every new stable-identity field is spelled `identifier` or
  `<noun>Identifier` (`§1.9`) — `PropLayerBundle::identifier`/`parentBundleIdentifier`/
  `assemblyIdentifier`, same as `MarkerLayerBundle`'s. No `Id`/`<noun>Id` anywhere in new code.
- `MapRecipe_PARAMS.h` gains six more flat top-level members: `propRuleLayers`,
  `decalRuleLayers`, `propLayerBundles`, `decalLayerBundles`, `globalPropSettings`,
  `globalDecalSettings`. **Flagged, not reversed:** this keeps growing `MapRecipe`'s flat member
  list rather than nesting each domain's authoring data into a per-domain sub-struct, in tension
  with the Constitution's opening-hit-list "dismember the god object" direction. Ruling: stay
  flat for consistency with Markers' own already-shipped five flat members
  (`markerRuleLayers`/`markers`/`chains`/`markerLayers`/`markerLayerBundles`) — retrofitting
  those into a nested shape now, just to keep Props/Decals from joining them, would be a bigger,
  disruptive, out-of-scope change touching every existing Marker call site. Whether `MapRecipe`'s
  overall shape needs a dedicated dismemberment pass is a separate, later decision, not made
  here.
- Every file-split call in `§20.1`/`§20.3` is driven by `§1.5`'s soft-100/hard-150 ceiling as
  estimated against the fields this ruling names — verify actual line counts at authoring/coding
  time, not just the estimate here; a file landing over ceiling still needs the documented
  work-order exception `§1.5` requires, not silent drift.

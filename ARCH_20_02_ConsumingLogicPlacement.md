[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.2. **Only the ARCH Expert writes this file.**

### 20.2 Grid-snap / effective-symmetry resolvers — duplicated per domain, in PARAMS; a Marker-side placement finding
`QuantizeMarkerPositionToLayerGrid`/`ResolveEffectiveMarkerSymmetry` each take a concrete
`const std::vector<Params::MarkerInstanceLayer>&`. By `§3.5`'s mechanical signature test this is
the identical bucket `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId`
(`PropInstance_PARAMS.h`) already occupy — `§19.2` cites that exact pair, by name, as the
precedent for "two independent bodies, not templated." Ruling:

- `QuantizePropPositionToLayerGrid`/`QuantizeDecalPositionToLayerGrid` and
  `ResolveEffectivePropSymmetry`/`ResolveEffectiveDecalSymmetry` are **duplicated, per domain,
  not templated** on the layer type — same signature shape, same bounds-check-defaults-safely
  posture (Constitution §6) as the Marker originals.
- They live in **PARAMS**, co-located with `PropInstanceLayer`/`DecalInstanceLayer` in the new
  file `§20.1` names — matching where `ResolvePropInstanceLayerId` already lives, per `§3.5`'s
  "Any `Params::` type in the signature → PARAMS, co-located with the struct(s) it walks" rule.

**Placement finding, not a mandate to fix now.** By that same `§3.5` text, the two Marker
originals should also live in PARAMS — they do not; they live in UI
(`MarkerDragGesture_UI.h`, `MarkersTab_ManualLayerHelpers_UI.h`). This is a real, citable
inconsistency, distinct from the many UI-local helpers taking `Params::` types that were never
MATH/PARAMS/PROC candidates in the first place (`ManualMarkerLayerRowLabel`,
`IsMarkerInstanceLayerRowSuppressed` — pure UI display policy, `§3.5` is silent on those). It is
recorded here as a **standing, non-blocking observation**: the IO/UI Experts may relocate the
Marker originals to PARAMS on a later ticket for consistency; this ratification does not force
that move, and the new Prop/Decal functions do not need to wait on it — they simply ship
correctly placed from day one.

[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.1. **Only the ARCH Expert writes this file.**

### 20.1 `PropRuleLayer`/`DecalRuleLayer`/`PropLayerBundle`/`DecalLayerBundle` — hand-written per domain, not templated; new file homes
`§19.2` already promoted "domain-touching PARAMS gets its own per-domain struct + per-domain
function family; only zero-domain-field-access mechanics get one shared template" to standing
law **naming Props/Decals explicitly** as the next consumers. This ruling applies it, not
re-derives it:

- `PropRuleLayer`/`DecalRuleLayer` are hand-written structs mirroring `MarkerRuleLayer`
  (`MarkerRule_PARAMS.h`) field-for-field: `name`, `bEnabled`, `bHidden`,
  `Params::SymmetrySetting symmetry`, `int parentBundleIdentifier = -1`, `std::vector<PropRule>
  rules;` / `std::vector<DecalRule> rules;`. `PropRuleLayer` additionally carries
  `propTypeName` (§20.6); `DecalRuleLayer` does not.
- `PropLayerBundle`/`DecalLayerBundle` mirror `MarkerLayerBundle` (`MarkerLayerBundle_PARAMS.h`)
  field-for-field: `identifier`, `name`, `parentBundleIdentifier`, `assemblyIdentifier`.
  `PropLayerBundle` additionally carries `propTypeName`; `DecalLayerBundle` does not (§20.6).
- Each Bundle type gets its **own** hand-written `WouldReparent<Domain>LayerBundleCreateCycle`
  predicate, not a shared one. This is not a relaxation of §19.2's "cycle-detection is pure
  mechanics" example — `§19.8` already ruled that the *shipped* `MarkerLayerBundle` predicate
  is PARAMS-resident and non-generic specifically because its signature takes a concrete
  `const std::vector<MarkerLayerBundle>&` (a `Params::` type), which `§3.5`'s mechanical test
  routes to PARAMS, hand-written, never MATH, never templated. The Prop/Decal predicates take
  the same shape of concrete parameter, so the same result follows mechanically.
- `TreeListWidget_UI<T, LeafKeyT>` (§19.7) needs **zero new widget-library code** — it is
  already accessor-lambda-parameterized with no `Params::` type baked into its own template
  signature, so `TreeListWidget_UI<PropLayerBundle, PropGroupLeafKey_UI>` /
  `TreeListWidget_UI<DecalLayerBundle, DecalGroupLeafKey_UI>` are new instantiations only.

**File homes** (new files; do not grow the existing files past `§1.5`'s ceiling):
- New `ScatterRuleLayer_PARAMS.h` — `PropRuleLayer` + `DecalRuleLayer`, sibling of
  `ScatterRule_PARAMS.h` (already 114/150 lines holding `PropRule`/`DecalRule`/`UnitRule`;
  it must not also absorb two wrapper structs).
- New `ScatterLayerBundle_PARAMS.h` — `PropLayerBundle` + `DecalLayerBundle` structs plus their
  own (duplicated, not shared) cycle-detection predicates. Split a companion
  `ScatterLayerBundleQuery_PARAMS.h` for the recursive-descendant-collection helpers
  pre-emptively if combined size approaches the ceiling — mirrors the STEP119
  `MarkerLayerBundle_PARAMS.h`/`MarkerLayerBundleQuery_PARAMS.h` split, done proactively this
  time instead of reactively.
- `PropInstanceLayer`/`DecalInstanceLayer` move **out of** `PropInstance_PARAMS.h` (currently
  66 lines) into a new sibling file once they gain full field parity (§20.6, plus
  `bSymmetryEnabled`/`bHidden`/`bGridSnapEnabled`/`gridSnapSizeWorldUnits`/
  `bColorOverrideEnabled`/`symmetry`/`parentBundleIdentifier` — the same field set
  `MarkerInstanceLayer` already carries). `PropInstance_PARAMS.h` keeps `PropTransform`/
  `DecalTransform`/`PropInstanceGroup`/`DecalInstanceGroup` only; `ResolvePropInstanceLayerId`/
  `Color` and their Decal counterparts move with the structs they walk (`§3.5` co-location
  rule) into the new file.
- This is the multi-domain-per-file convention this codebase already uses when Prop and Decal
  are designed together (`ScatterRule_PARAMS.h`, `PropInstance_PARAMS.h`), not the
  single-domain-per-file shape Marker's files happen to have only because they predate any
  Prop/Decal sibling.

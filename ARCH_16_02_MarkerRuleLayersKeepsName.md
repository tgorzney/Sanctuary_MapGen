[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.2. **Only the ARCH Expert writes this file.**

### 16.2 `markerRuleLayers` keeps its full name; does not shorten to `markerLayers` like `propLayers` did
`PropInstanceLayer`'s `MapRecipe` field is `propLayers`, dropping "Instance" from the type name —
§12 precedent. Markers cannot follow that exact shortening: `MapRecipe` needs **two** sibling
marker-layer arrays (procedural-rule layers AND manual-instance layers) where Props/Decals today
have only one (procedural Stacks remain flat). Shortening the procedural field to `markerLayers`
would collide with the manual field of the same natural name. `markerRuleLayers` /
`markerLayers` is therefore not an arbitrary naming choice — the "Rule" qualifier is
**structurally required** to keep the two fields distinct, not merely descriptive polish.


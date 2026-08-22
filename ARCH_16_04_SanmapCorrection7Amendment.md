[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.4. **Only the ARCH Expert writes this file.**

### 16.4 `SANMAP_FORMAT_SPEC` Correction 7 amendment — scoped-down one-tier shape accepted; exact key spelling deferred to the Format Expert
The design's proposed wire shape —
```
MarkersStack → [ N x { Name, Enabled, Hidden, SymmetryUseGlobal, SymmetryMask,
                        RadialSymmetryRepeatCount, <rules array> } ]
```
— **is accepted as the correct scope for this ratification**: it closes only the piece of
Correction 7's long-deferred Group/Layer/LayerType hierarchy that layer-scoped marker symmetry
actually requires (one tier, wrapping the existing flat `MarkerRule` array), not the full
deferred multi-level design. This is the same "build only what the concrete need requires, not
the speculative full design" posture already applied at §12 (props/decals got `layerIndex` +
`PropGroups`/`DecalGroups`, not the full deferred hierarchy either) and is consistent with
Correction 7's own text, which already anticipated this: "For this work-order, each Stack's
layers are, for now, a flat array... shape pending the deferred shared Group/Layer/LayerType
design" — this ratification is the first concrete need to finally justify spending part of that
deferral, for `MarkersStack` only. `PropsStack`/`DecalsStack`/`UnitsStack` remain flat rule
arrays; nothing here obligates upgrading them until a similarly concrete need arises for one of
them.

**Exact JSON key spelling for the new nested-array field (what the design left as
`<rules array>`) is explicitly the Format Expert's call, not ruled here** — consistent with how
§1.6's casing law is a naming law this ARCH owns, but its case-by-case application to a specific
new nested collection member key is the Format Expert's domain (the same split already recorded
at §15.7 for `Scenarios`). The PARAMS-side field name is already fixed by §16.1
(`MarkerRuleLayer::rules`); only its `.sanmap` JSON key spelling remains open.


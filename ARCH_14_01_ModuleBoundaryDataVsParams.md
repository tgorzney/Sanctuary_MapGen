[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.1. **Only the ARCH Expert writes this file.**

### 14.1 Module boundary and the DATA-vs-PARAMS split
`OverlayLayer_UI`/`overlayLayers` is `UI`, the same precedent as `PreviewCompositeSettings::
fieldLayers` (`PreviewComposite_Settings_UI.h`) — session-only presentation, not
recipe-serialized (§14.5). Two kinds of sub-layer, never conflated:
- **Procedural sub-layers** reuse the existing DATA columns (`PlacementInstance_DATA.h`'s
  `ruleIndex`/`category`) — no new DATA field.
- **Manual sub-layers** never touch DATA at all — they read `Params::MapRecipe` pass-through
  arrays (`PropInstanceLayer`/`DecalInstanceLayer` §12, `Army.groups` §9,
  `ENTITY_AUTHORING_PARAMS_SPEC`) directly, filtered by their own existing identity field.

GPU-resident overlay draw state (vertex buffers, atlas bindings) routes through the existing
`GpuResource_SYS` (`DISPATCH_INTERFACE_SPEC` §3) — a UI-owned GL pipeline is a named v1 defect
class (§3.2, §5.4) and must not reappear here.


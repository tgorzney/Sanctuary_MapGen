[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.4. **Only the ARCH Expert writes this file.**

### 14.4 Nested `UnitGroup` addressing is flat
Top-level `Army.groups[name]` is one sub-layer; nested `UnitGroup.groups` draw as part of their
top-level parent, never separately addressable. This keeps `OverlaySubLayerRef_UI` uniform across
every domain (no domain-specific recursive-index special case) and avoids the "flattened
pre-order index into a mutable recursive tree" corruption class `ENTITY_AUTHORING_PARAMS_SPEC`
already ruled against elsewhere. Confirmed (Format Expert) this mirrors the official `.sanmap`
format's own `Army.groups`/`UnitGroup.units`/`.groups` tree 1:1 — not a SanGen invention.
Recursive addressing is a legitimate future ask; it needs its own ratification if actually
requested later.


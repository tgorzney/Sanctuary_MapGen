[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.5. **Only the ARCH Expert writes this file.**

### 14.5 View-stack state — split by field, not one blanket policy
- **Order / `bEnabled` / opacity:** session-only UI presentation — same policy already
  governing `PreviewCompositeSettings` (v1's serialized `PreviewLayers` was already a named
  defect to replace, not evidence v2 must re-serialize).
- **`color`/`iconScale`:** **not** a blanket UI-only field. Where a domain already owns a
  recipe-serialized layer-metadata record — Props/Decals (`PropInstanceLayer`/
  `DecalInstanceLayer`, §12) — `OverlayLayer_UI` reads/writes that record directly. No shadow
  copy, no second source of truth. Where no such PARAMS record exists yet (Alloy/SpawnsArmies,
  Units), these stay UI-session defaults until a future ratification gives them a real home —
  mirror the shipped Props/Decals pattern, do not invent a new one now. **Alloy/SpawnsArmies now
  has that PARAMS record — `Params::MarkerInstanceLayer` (ARCH §16) — so this row's guidance for
  that domain is superseded the same way §14.2's table entry is; Units remains UI-session-only,
  unaffected.**


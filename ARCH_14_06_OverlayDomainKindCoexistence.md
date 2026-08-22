[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.6. **Only the ARCH Expert writes this file.**

### 14.6 `OverlayDomainKind_UI` vs `MarkerCategory`/`PlacementResults` — sits alongside, changes neither
`Alloy`/`SpawnsArmies` re-slice the existing `markers` buffer by its existing `category` column.
A UI enum may re-slice an existing DATA collection by its own field without the DATA shape
changing — zero blast radius on `MarkersTab_Rules_UI.h` or marker import/export. ⚠️ Domain-kind
is **asymmetric** versus DATA buckets — it splits markers 2 ways but maps Props/Units/Decals 1:1
— a coder must not assume `domain == DATA-bucket identity`.


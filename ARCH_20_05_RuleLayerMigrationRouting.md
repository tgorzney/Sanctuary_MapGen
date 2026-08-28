[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.5. **Only the ARCH Expert writes this file.**

### 20.5 IO — additive parts confirmed no-bump; the `RuleLayer` wrapping tier is **gated on a separate IO Architecture Expert consult, not yet done**
Three distinct IO questions, only two of which are actually additive:

1. **New fields on existing `PropInstanceLayer`/`DecalInstanceLayer`** (symmetry,
   `bSymmetryEnabled`, grid-snap, color-override, `parentBundleIdentifier`, `propTypeName`) —
   additive, default-constructs safely on an older file, **no `SanGenVersion` bump, no
   `IO_MIGRATION_SPEC` entry**. Confirmed; matches Corrections 14/16/19's own additive-field
   history.
2. **New sibling arrays/files for `PropLayerBundle`/`DecalLayerBundle`** (new top-level
   `PropLayerBundles`/`DecalLayerBundles` wire arrays) — additive, **no bump**, mirrors
   Correction 19's `MarkerLayerBundles` precedent exactly. New file pairs
   `MapExporter_PropLayerBundle_IO.cpp`/`MapImporter_PropLayerBundle_IO.cpp` (and the Decal
   counterparts), mirroring `MapExporter_MarkersStack_IO.cpp`/
   `MapImporter_MarkerLayerBundle_IO.cpp`. Confirmed.
3. **`PropRuleLayer`/`DecalRuleLayer` — NOT confirmed additive. Gated, not ruled here.**
   This requires restructuring `MapRecipe::propRules`/`decalRules` from a **flat**
   `vector<PropRule>`/`vector<DecalRule>` (confirmed live today —
   `MapImporter_PropsStack_IO.cpp`/`MapExporter_PropsStack_IO.cpp`) into a **two-tier**
   Group(`PropRuleLayer`)→Rule(`PropRule`) shape. This is the same *class* of breaking
   wire-format restructuring as Markers' own `markerRules` → `markerRuleLayers` move
   (`§16.1`/`§16.6`) — Markers' own instance of that migration has since **shipped in full**
   (`§16.6`'s "Shipped" note, confirmed 2026-08-27: `MarkersStack_Migrate_V3_IO.h`/`.cpp`,
   registered under `sourceVersion = 3` in `Sanmap_MigrationManifest_IO.cpp`, tested by
   `MarkersStack_Migrate_V3_IO_Test.cpp`). That does not make Props/Decals' own restructuring
   additive by default-analogy — it is still a new, not-yet-designed migration for a different
   pair of arrays; item 3 remains gated on its own consult below. What Markers' precedent does
   establish is the concrete *shape* such a migration can take (a real, working
   `<Domain>_Migrate_V<N>_IO` grouping transform under this exact class of restructuring), which
   the routed consult below should treat as its working precedent rather than starting from a
   blank slate.

**Ruling: route this to the IO Architecture Expert as ONE shared migration-shape consult
covering Markers + Props + Decals together**, not a second, independently-invented Props/Decals
migration for the identical shape of problem — even though Markers' own instance has now shipped,
Props and Decals still need their own version-step built against `PropRuleLayer`/
`DecalRuleLayer`, and doing that as one shared-shape consult (reusing Markers' shipped
`MarkersStack_Migrate_V3_IO` as the template) avoids a second, independently-invented design for
the same class of problem. A plausible shape (not asserted here as decided) is a JSON transform
that wraps a legacy flat rule array into one synthesized default-named `RuleLayer`, loudly logging
any per-rule divergence it must collapse (Constitution §6) — which may not require a
`SanGenVersion` bump even though it is a real recoverable transform, distinct from "no migration
needed at all."

**No coder work-order may build the `PropRuleLayer`/`DecalRuleLayer` IO round-trip, or wire
`MapRecipe::propRules`/`decalRules` into the new two-tier shape, ahead of that consult landing.**
The PARAMS-only shape (`§20.1`) may still be authored and unit-tested in isolation — it is the
live IO wiring and any legacy-file read path that is gated.

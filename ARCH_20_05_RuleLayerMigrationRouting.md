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
   (`§16.1`/`§16.6`) — and that migration is **still open**: `§16.6` ruled a migration is
   required but explicitly left the mechanics to the IO Architecture Expert, and the pack's own
   `sangen_arch_pack/INDEX.md` still records that consult as not done, even though the
   `MarkerRuleLayer` PARAMS+IO code for the *new* shape already shipped. Extending the identical
   restructuring to Props/Decals must not assume "no bump needed" by false analogy to items 1-2
   above — it is not the same precedent class.

**Ruling: route this to the IO Architecture Expert as ONE shared migration-shape consult
covering Markers + Props + Decals together**, not a second, independently-invented Props/Decals
migration for the identical shape of problem Markers already owes one. A plausible shape (not
asserted here as decided) is a JSON transform that wraps a legacy flat rule array into one
synthesized default-named `RuleLayer`, loudly logging any per-rule divergence it must collapse
(Constitution §6) — which may not require a `SanGenVersion` bump even though it is a real
recoverable transform, distinct from "no migration needed at all."

**No coder work-order may build the `PropRuleLayer`/`DecalRuleLayer` IO round-trip, or wire
`MapRecipe::propRules`/`decalRules` into the new two-tier shape, ahead of that consult landing.**
The PARAMS-only shape (`§20.1`) may still be authored and unit-tested in isolation — it is the
live IO wiring and any legacy-file read path that is gated.

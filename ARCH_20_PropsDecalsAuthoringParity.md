[← ARCH index](ARCH.md) · Part of the ratified v2 architecture; the Constitution and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 20. Props/Decals authoring parity with Markers — `RuleLayer`/`InstanceLayer` field parity, the `LayerBundle` tree, Type Sections

Extends `§19`'s Marker-specific Group-above-Layer model to two new domains, per the human's
ratified decision: Markers, Props, and Decals (Units deferred to a later Armies-tab decision)
get the same structures — Type Sections, full `RuleLayer`/`InstanceLayer` field parity, and the
Group/Bundle tree. Props gets two Type-section values ("Prop"/"Reclaim") driven by a
`propTypeName` string tag, the domain's own mirror of `markerTypeName` — not a boolean derived
from `PropInstanceGroup::bReclaimable`. Decals gets the Bundle tree and full field parity but no
Type-section field at all (exactly one implicit type). Responds to a consult ruling (this
session) grounded by direct reads of `MarkerRule_PARAMS.h`, `MarkerInstance_PARAMS.h`,
`MarkerLayerBundle_PARAMS.h`, `PropInstance_PARAMS.h`, `ScatterRule_PARAMS.h`,
`GlobalMarkerSettings_PARAMS.h`, `MapRecipe_PARAMS.h`, `MarkerDragGesture_UI.h`, and
`MarkersTab_ManualLayerHelpers_UI.h`.

**Two items are explicitly gated, not resolved by this ratification — no coder work-order may
build against §20.4 or the live-IO half of §20.5 until their respective follow-on consults land
and are ratified into their own new ARCH subsections:**
- **§20.4** — the Prop/Decal drag-reposition + selection substrate needs a UI Expert design
  round (unified with the separately-paused canvas click/box-select initiative), not a
  hand-mirrored `PropDragGesture_UI`/`DecalDragGesture_UI`.
- **§20.5 item 3** — the `PropRuleLayer`/`DecalRuleLayer` flat-to-two-tier wire restructuring
  needs an IO Architecture Expert consult, sharing one migration shape with Markers' own still-
  open `§16.6` gap, not an independently-invented Props/Decals migration.

Everything else (`§20.1`–`§20.3`, `§20.6`–`§20.8`) is fully ratified and buildable now.

### Subsections of §20

| § | File | Ruling |
|---|---|---|
| §20.1 | [ARCH_20_01_ParamsGenericitySplit.md](ARCH_20_01_ParamsGenericitySplit.md) | `PropRuleLayer`/`DecalRuleLayer`/`PropLayerBundle`/`DecalLayerBundle` — hand-written per domain, not templated; new file homes |
| §20.2 | [ARCH_20_02_ConsumingLogicPlacement.md](ARCH_20_02_ConsumingLogicPlacement.md) | Grid-snap / effective-symmetry resolvers — duplicated per domain, in PARAMS; a Marker-side placement finding |
| §20.3 | [ARCH_20_03_GlobalPropDecalSettings.md](ARCH_20_03_GlobalPropDecalSettings.md) | `GlobalPropSettings`/`GlobalDecalSettings` — scoped to what has a real analog, not a blind mirror |
| §20.4 | [ARCH_20_04_DragGestureSubstrateRouting.md](ARCH_20_04_DragGestureSubstrateRouting.md) | Drag-gesture/selection substrate — **gated on a UI Expert design round, not yet done** |
| §20.5 | [ARCH_20_05_RuleLayerMigrationRouting.md](ARCH_20_05_RuleLayerMigrationRouting.md) | IO — additive parts confirmed no-bump; `RuleLayer` wrapping tier **gated on an IO Architecture Expert consult, not yet done** |
| §20.6 | [ARCH_20_06_TypeSectionReuse.md](ARCH_20_06_TypeSectionReuse.md) | Type Sections — reuse `§19.14`'s mechanism verbatim; field named per domain (`propTypeName`, never `markerTypeName` on a Prop/Decal struct; Decals gets no field at all) |
| §20.7 | [ARCH_20_07_Housekeeping.md](ARCH_20_07_Housekeeping.md) | Naming / file-size / `MapRecipe` flatness housekeeping |
| §20.8 | [ARCH_20_08_DecalsTopLevelTab.md](ARCH_20_08_DecalsTopLevelTab.md) | Decals is a standalone top-level tab — ratifies the already-shipped split (STEP159), closes a dangling forward-reference |

Related law: `sangen_arch_pack/CONSTITUTION.md`; `ARCH_03_ModuleBoundaries.md` §3.5 (the
MATH/PARAMS/PROC placement rule §20.1/§20.2 apply, not re-derive); `ARCH_01_09_IdAbbreviationBan.md`
(the "Id" ban §20.7 restates); `ARCH_19_MarkerLayerBundle.md` (the Marker-side shapes this
section mirrors into two new domains, without reopening any of that section's own fields);
`ARCH_16_MarkerLayerSymmetry.md` §16.6 (the still-open Marker migration §20.5 ties Props/Decals'
own migration need to). Follow-up, not done this session: `PLACEMENT_SCATTER_SPEC.md` and
`ENTITY_AUTHORING_PARAMS_SPEC.md` (`sangen_arch_pack/specs/`) still need narrative updates
naming these new types, once the §20.4/§20.5 gated consults land and the concrete shape settles
further — recorded here so it is not lost, mirroring how `§19`'s own Format Expert follow-ups
were tracked in `sangen_arch_pack/INDEX.md` before they landed.

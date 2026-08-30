[← ARCH index](ARCH.md) · Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 22. Navmap Modifier blockers — formalizing a proven hand-authoring technique, and (2026-08-30) the ratified SanGen-native successor (ratifies `NAVMAP_MODIFIER_BLOCKER_SPEC.md` and, for §22.10-§22.17, four 2026-08-30 design consults)

The game engine's native per-navigation-layer pathing-block primitive — a `NavmapModifierTemplate`,
an axis-aligned world-space rectangle blocking exactly one named layer, with the set of layers a
prefab can ever block fixed forever at prefab-template creation time — has a real, proven
map-authoring technique for reusing it from a per-map `<MapName>_data.lua` orchestrator: reuse the
engine's own singleton `PlayableAreaBarrier` prefab to block **all** layers together (confirmed
working live in-game, twice, 2026-08-29), or build a small purpose-built prefab to block a
**partial** layer subset (also confirmed working live in-game, 2026-08-29). §22.1-§22.9 promote
that hand-authored Lua technique to first-class knowledge-pack law. §22.10-§22.17 (2026-08-30) rule
on a real, separately-scoped design consult (four documents, one per domain expert) and formally
bring the **SanGen-native successor** — mesh-vs-water-plane intersection → rasterize → decompose →
Navmesh Tab authoring — into the owned architecture for the first time, satisfying §22.9's own
"no coder should build toward it without a real, separately-scoped design consult first" gate.

**Status:** the Lua-authoring half — the all-layer technique (§22.3, confirmed twice) and the
partial-layer technique (§22.4, confirmed once, 2026-08-29) — is **confirmed shipped** and entirely
unaffected by §22.10-§22.17, which are purely additive. The SanGen-native mesh-ingestion/mask-
generation half (§22.10-§22.17) is a **ratified design, not yet built** — it authorizes ticket
dispatch, per the layer-membership table in §22.10, for the file/ticket set the four 2026-08-30
design consults enumerate.

**Classification:** §22.1-§22.9's Lua technique remains **game-side Lua, not SanGen C++** (§22.1) —
it does not occupy a slot in the Constitution §1 layer stack. §22.10-§22.17's mesh-ingestion/mask-
generation successor **does** occupy real slots (SYS/IO/MATH/PROC/PARAMS/PIPELINE/UI) — see §22.10's
table — a genuine addition to the layer stack, not a reclassification of §22.1-§22.9's subject.

---

### Subsections of §22

| § | File | Ruling |
|---|---|---|
| §22.1 | [ARCH_22_01_LayerClassification.md](ARCH_22_01_LayerClassification.md) | Layer classification — game-side Lua, not SanGen C++ (mirrors §15.1) |
| §22.2 | [ARCH_22_02_NativePrimitiveGroundTruth.md](ARCH_22_02_NativePrimitiveGroundTruth.md) | The native `NavmapModifierTemplate` primitive — recorded ground truth, binding on both techniques |
| §22.3 | [ARCH_22_03_AllLayerBlockerTechnique.md](ARCH_22_03_AllLayerBlockerTechnique.md) | All-layer blocker via the global `PlayableAreaBarrier` prefab — ratified, confirmed shipped |
| §22.4 | [ARCH_22_04_PartialLayerBlockerTechnique.md](ARCH_22_04_PartialLayerBlockerTechnique.md) | Partial/single-layer blocker requires a purpose-built prefab — confirmed shipped (2026-08-29); promotion-to-shared-helper guidance still open |
| §22.5 | [ARCH_22_05_PerLuaStateExecutionNuance.md](ARCH_22_05_PerLuaStateExecutionNuance.md) | Per-Lua-state execution nuance — distinct from `MAP_UNIT_SPAWNING_SPEC` §2's `Import`-cache double-execution hazard |
| §22.6 | [ARCH_22_06_NewThreadOrderingLaw.md](ARCH_22_06_NewThreadOrderingLaw.md) | Ordering law inside the shared `NewThread` — extends, does not replace, existing ordering law |
| §22.7 | [ARCH_22_07_MaskToRectangleWorkflow.md](ARCH_22_07_MaskToRectangleWorkflow.md) | Mask-to-rectangle authoring workflow — recorded as the (now-superseding-in-progress, §22.10) manual process |
| §22.8 | [ARCH_22_08_CoordinateConventionDistinction.md](ARCH_22_08_CoordinateConventionDistinction.md) | Pixel↔world coordinate convention for mask-derived rectangles — distinct from the `.sanmap` entity-position convention |
| §22.9 | [ARCH_22_09_OwnershipScopeRuling.md](ARCH_22_09_OwnershipScopeRuling.md) | Ownership/scope ruling for the Lua technique — not a SanGen-owned `PARAMS`/`IO`/`UI` construct; its own "real, separately-scoped design consult" requirement is satisfied, for the SanGen-native successor specifically, by §22.10 |
| §22.10 | [ARCH_22_10_MeshIngestionOwnershipRuling.md](ARCH_22_10_MeshIngestionOwnershipRuling.md) | (2026-08-30) Mesh-ingestion + mask-generation formally enter SanGen's owned architecture — the layer-membership table, and confirmation that §22.9's gate is satisfied for this ticket set |
| §22.11 | [ARCH_22_11_MeshIngestionShape.md](ARCH_22_11_MeshIngestionShape.md) | (2026-08-30) Mesh-ingestion shape — SYS `.sanmodel` reader, the `visuals.lods[]` IO fork (additive sibling file), the mesh cache, and the manual-prop `bCollidable` gap closed on `Params::PropInstanceGroup` |
| §22.12 | [ARCH_22_12_MaskGenerationAlgorithmAndScope.md](ARCH_22_12_MaskGenerationAlgorithmAndScope.md) | (2026-08-30) Mask-generation algorithm/accuracy-class confirmed; v1 layer scope ruled Sea + Submarine only, Land/Amphibious/Hover/Air deferred to a future "full silhouette" technique |
| §22.13 | [ARCH_22_13_BakedArtifactStorageAndDeterminism.md](ARCH_22_13_BakedArtifactStorageAndDeterminism.md) | (2026-08-30) Baked rectangle list lives in `Params::`, never a companion `.lua`; the Exact/Deterministic-chain question ruled out of scope by construction; new one-shot PIPELINE bake responsibility class named |
| §22.14 | [ARCH_22_14_GeometryMathAndDispatch.md](ARCH_22_14_GeometryMathAndDispatch.md) | (2026-08-30) MATH primitive placement confirmed; CPU-only dispatch policy ruled for the new PROC stage(s) |
| §22.15 | [ARCH_22_15_NavmeshTabParamsShape.md](ARCH_22_15_NavmeshTabParamsShape.md) | (2026-08-30) `Params::NavLayerKind` closed-enum divergence confirmed correctly reasoned; tagged-vector storage shape closed; origin+extent coordinate convention confirmed binding; symmetry-drop and Move-only Bundle confirmed; `MapAreas`-style compositing direction confirmed |
| §22.16 | [ARCH_22_16_RectangleDragGesturePromotion.md](ARCH_22_16_RectangleDragGesturePromotion.md) | (2026-08-30) `AreaDragGesture_UI`'s rectangle core promoted to an accessor-parameterized `RectangleDragGesture_UI<Accessor>` template, per this pack's "proven twice" discipline |
| §22.17 | [ARCH_22_17_ZOrderBatchInsertCost.md](ARCH_22_17_ZOrderBatchInsertCost.md) | (2026-08-30) Navmesh adopts its own independent size-sorted Z-order convention; batch-insert cost ruled negligible at ingestion scale (rough estimate), no bulk-sort path required |

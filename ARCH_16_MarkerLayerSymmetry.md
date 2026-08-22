[← ARCH index](ARCH.md) · SanGen ARCH §16. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 16. Marker layer-scoped symmetry — `MarkerRuleLayer` / `MarkerInstanceLayer` / `SymmetrySetting` (ARCH ruling, ratifies `work_orders/DESIGN_MarkerLayerSymmetry_R1.md` + `_R2.md`)

Ratifies the UI Expert's two-round consult on the Markers Tab redesign: markers move from v2's
flat, per-rule symmetry (`MarkerRule::bSymmetryUseGlobal`/`symmetryMask`/`radialSymmetryRepeatCount`,
already field-complete and already live-exported) to a **layer-scoped** symmetry setting shared by
both the procedural-rule side and the manual-instance side, driven through one shared orbit
function on both paths — a real improvement over v1, which never had per-layer symmetry for
either side, per the human's binding requirement in `work_orders/BRIEF_MarkersTabUI_R2.md`. This
introduces the first tier of `SANMAP_FORMAT_SPEC` Correction 7's long-deferred Group/Layer
hierarchy for `MarkersStack` specifically (§16.4) — scoped to what layer-scoped symmetry actually
needs, not the full deferred design.


---

### Subsections of §16

| § | File | Ruling |
|---|---|---|
| §16.1 | [ARCH_16_01_NewParamsShapes.md](ARCH_16_01_NewParamsShapes.md) | New PARAMS shapes — ratified, with §16.5's naming amendment folded in |
| §16.2 | [ARCH_16_02_MarkerRuleLayersKeepsName.md](ARCH_16_02_MarkerRuleLayersKeepsName.md) | `markerRuleLayers` keeps its full name; does not shorten to `markerLayers` like `propLayers` did |
| §16.3 | [ARCH_16_03_ModuleBoundaryChain.md](ARCH_16_03_ModuleBoundaryChain.md) | Module boundary — solved via the existing legal `UI → PIPELINE → PROC` chain; no MATH relocation, no new UI→PROC exception |
| §16.4 | [ARCH_16_04_SanmapCorrection7Amendment.md](ARCH_16_04_SanmapCorrection7Amendment.md) | `SANMAP_FORMAT_SPEC` Correction 7 amendment — scoped-down one-tier shape accepted; exact key spelling deferred to the Format Expert |
| §16.5 | [ARCH_16_05_MarkerTransformFields.md](ARCH_16_05_MarkerTransformFields.md) | `MarkerTransform` — `symmetryGroupIdentifier` (not `symmetryGroupId`) + already-ratified `layerIndex` |
| §16.6 | [ARCH_16_06_MigrationRouting.md](ARCH_16_06_MigrationRouting.md) | Migration — a real breaking schema change; routed to the IO Architecture Expert, not ruled on here |
| §16.7 | [ARCH_16_07_NamingConfirmed.md](ARCH_16_07_NamingConfirmed.md) | Naming confirmed — `MarkerRuleLayer` / `MarkerInstanceLayer` |
| §16.8 | [ARCH_16_08_SpawnArmyShrink.md](ARCH_16_08_SpawnArmyShrink.md) | Spawn/Army shrink — confirmed: no new PARAMS type or field needed |
| §16.9 | [ARCH_16_09_NonArchItems.md](ARCH_16_09_NonArchItems.md) | Non-ARCH items — confirmed out of scope for this ratification |
| §16.10 | [ARCH_16_10_ConsultRoutingSummary.md](ARCH_16_10_ConsultRoutingSummary.md) | Consult routing summary — what remains before a coder work-order can be written |

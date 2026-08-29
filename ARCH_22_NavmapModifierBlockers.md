[← ARCH index](ARCH.md) · Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 22. Navmap Modifier blockers — formalizing a proven hand-authoring technique (ratifies `NAVMAP_MODIFIER_BLOCKER_SPEC.md`)

The game engine's native per-navigation-layer pathing-block primitive — a `NavmapModifierTemplate`,
an axis-aligned world-space rectangle blocking exactly one named layer, with the set of layers a
prefab can ever block fixed forever at prefab-template creation time — has a real, proven
map-authoring technique for reusing it from a per-map `<MapName>_data.lua` orchestrator: reuse the
engine's own singleton `PlayableAreaBarrier` prefab to block **all** layers together (confirmed
working live in-game, twice, 2026-08-29), or build a small purpose-built prefab to block a
**partial** layer subset (designed, not yet shipped). This ratification promotes that technique to
first-class knowledge-pack law, following the same process used for the Map Scenario system
(§15): full technical contract in `NAVMAP_MODIFIER_BLOCKER_SPEC.md`, not re-derived here.

**Status:** the all-layer technique (§22.3) is **confirmed shipped**. The partial-layer technique
(§22.4) is **⚠️ designed, not yet shipped** — treat it accordingly. Neither introduces a new
SanGen `PARAMS`/`IO`/`UI` construct today (§22.9); this ratification is knowledge-pack law about a
hand-authored Lua pattern, the same status `MAP_UNIT_SPAWNING_SPEC.md`'s generator-function
patterns already hold.

**Classification:** like the Map Scenario system, this is **game-side Lua, not SanGen C++**
(§22.1) — it does not occupy a slot in the Constitution §1 layer stack.

---

### Subsections of §22

| § | File | Ruling |
|---|---|---|
| §22.1 | [ARCH_22_01_LayerClassification.md](ARCH_22_01_LayerClassification.md) | Layer classification — game-side Lua, not SanGen C++ (mirrors §15.1) |
| §22.2 | [ARCH_22_02_NativePrimitiveGroundTruth.md](ARCH_22_02_NativePrimitiveGroundTruth.md) | The native `NavmapModifierTemplate` primitive — recorded ground truth, binding on both techniques |
| §22.3 | [ARCH_22_03_AllLayerBlockerTechnique.md](ARCH_22_03_AllLayerBlockerTechnique.md) | All-layer blocker via the global `PlayableAreaBarrier` prefab — ratified, confirmed shipped |
| §22.4 | [ARCH_22_04_PartialLayerBlockerTechnique.md](ARCH_22_04_PartialLayerBlockerTechnique.md) | Partial/single-layer blocker requires a purpose-built prefab — ⚠️ designed, not shipped; promotion-to-shared-helper guidance |
| §22.5 | [ARCH_22_05_PerLuaStateExecutionNuance.md](ARCH_22_05_PerLuaStateExecutionNuance.md) | Per-Lua-state execution nuance — distinct from `MAP_UNIT_SPAWNING_SPEC` §2's `Import`-cache double-execution hazard |
| §22.6 | [ARCH_22_06_NewThreadOrderingLaw.md](ARCH_22_06_NewThreadOrderingLaw.md) | Ordering law inside the shared `NewThread` — extends, does not replace, existing ordering law |
| §22.7 | [ARCH_22_07_MaskToRectangleWorkflow.md](ARCH_22_07_MaskToRectangleWorkflow.md) | Mask-to-rectangle authoring workflow — recorded as the current manual (non-SanGen) process |
| §22.8 | [ARCH_22_08_CoordinateConventionDistinction.md](ARCH_22_08_CoordinateConventionDistinction.md) | Pixel↔world coordinate convention for mask-derived rectangles — distinct from the `.sanmap` entity-position convention |
| §22.9 | [ARCH_22_09_OwnershipScopeRuling.md](ARCH_22_09_OwnershipScopeRuling.md) | Ownership/scope ruling — not yet a SanGen-owned `PARAMS`/`IO`/`UI` construct; future-direction note only |

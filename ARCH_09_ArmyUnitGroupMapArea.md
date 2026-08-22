[← ARCH index](ARCH.md) · SanGen ARCH §9. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 9. `Params::Army` / `UnitGroup` / `UnitTransform` / `MapArea` (ARCH ruling, ratifies `ENTITY_AUTHORING_PARAMS_SPEC`)

Fills the gap standing since M1: `MapRecipe` has always held procedural placement RULES
(`MarkerRule`/`PropRule`/`DecalRule`/`UnitRule`, §7.1-adjacent, `PLACEMENT_SCATTER_SPEC`) but
had no PARAMS home for **manually-placed, human-authored** entity data — pre-placed army
units and named areas — even though `.sanmap` has real, live sections for both (`armies`,
`areas`) and v1 round-tripped both. Confirmed by `work_orders/RECIPE_PARITY_BACKLOG.md` Tier 1
and the standing §8.4-compliant scope notes in `ArmiesTab_UI.h` / `AreasTab_UI.h`.

- **Naming derivation:** §1.8 (new naming law this ruling also adds).
- **Full field lists, the recursive-tree structural ruling, the `legacyTypeTag`
  passthrough ruling, and the live-engine out-of-scope note:** `ENTITY_AUTHORING_PARAMS_SPEC`.
- **These are pass-through types, not procedural rules** — no PROC stage computes or
  reinterprets any field on `Army`/`UnitGroup`/`UnitTransform`/`MapArea`; they exist purely
  for round-trip fidelity through `IO` and direct authoring through `UI`. They are therefore
  distinct from, and additive to, `Params::UnitRule` (which remains the procedural scatter
  rule for armies) — both are legal producers into the same `armies[]` roster.
- **Shape only, not wiring.** This ruling and its spec fix the C++ shape of the four new
  types. Adding `std::vector<Army> armies;` / `std::vector<MapArea> areas;` to
  `MapRecipe_PARAMS.h`, the matching `IO` round-trip, and retiring the two UI scope notes
  are a separate coder work-order (`ENTITY_AUTHORING_PARAMS_SPEC` "Where these land").

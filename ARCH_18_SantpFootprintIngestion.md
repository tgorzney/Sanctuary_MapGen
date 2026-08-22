[← ARCH index](ARCH.md) · SanGen ARCH §18. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 18. `.santp`/`.sanprop` template ingestion — sandboxed execution primitive + determinism ruling (ARCH ruling, responds to `work_orders/DESIGN_SantpFootprintIngestion_R1.md`)

Rules on the ARCH-gated questions the Format Expert's design (`DESIGN_SantpFootprintIngestion_R1.md`)
flagged before its proposed ticket sequence (85–93) can be dispatched: whether a NEW sandboxed
LuaJIT *execution* primitive is legal alongside the existing compile-only `LuaSyntaxCheck_SYS`
(§15.8); how ingested real-game footprint data can influence generation without breaking
Constitution §4's determinism guarantee; and where the richer catalog data the same reader surfaces
(tags, etc.) lives. Tickets 85–88 (the sandbox itself, the source scanner, the dialect parser, the
disk cache) proceed under §18.1 alone; ticket 89 (the orchestrator that actually wires ingested data
toward a PROC consumer) additionally requires §18.2; ticket 92 (`bReclaimable` auto-population from
ingested tags) additionally requires §18.3.

**Q1, ruled: option (a), as the design recommended.** `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3
now carries a clarifying sentence, added by this ruling, confirming its "SanGen never calls into a
Lua parser to read this file, **or any scenario content**, back" text is scoped to the Map Scenario
system's own authored content — template ingestion reads a different corpus (game-shipped
`.santp`/`.sanprop` data) in a different direction (never round-tripped back into a `.sanmap` or
scenario file) and is not in tension with §15.3. (A prior draft of this paragraph left §15.3
unamended, relying on this paragraph alone as the confirmation. That was reconsidered: the design
doc's own concern was precisely that a coder reading §15.3 in isolation, without also finding this
paragraph, would misread it as a blanket prohibition — so the clarifying sentence now lives at the
source, §15.3 itself, exactly as the design doc's option (a) asked for.)

---

### Subsections of §18

| § | File | Ruling |
|---|------|--------|
| §18.1 | [ARCH_18_01_SandboxedExecutionPrimitive.md](ARCH_18_01_SandboxedExecutionPrimitive.md) | `LuaTableEvaluate_SYS`/`LuaTableValue_SYS` — a sibling `SYS` primitive to `LuaSyntaxCheck_SYS`, sharing only the vendored LuaJIT library; the execution safety contract, binding |
| §18.2 | [ARCH_18_02_IngestedDataDeterminism.md](ARCH_18_02_IngestedDataDeterminism.md) | Determinism ruling — ingested footprint data may influence generation only after being baked into `PARAMS` at an explicit authoring-time action; never read live by `PROC` |
| §18.3 | [ARCH_18_03_CatalogDataOwnership.md](ARCH_18_03_CatalogDataOwnership.md) | Q3 ruled — richer catalog data (footprint + tags, the two artifacts tickets 89/92 need) stays `IO`-owned, asset-derived, matching `AssetAtlasCache_*`; no new `DATA`-layer catalog type; `economy.harvest`/`collisionInfo`/`displayName` explicitly deferred |

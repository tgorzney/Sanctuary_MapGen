[← ARCH index](ARCH.md) · SanGen ARCH §18. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 18. `.santp`/`.sanprop` template ingestion — sandboxed execution primitive + determinism ruling (ARCH ruling, responds to `work_orders/DESIGN_SantpFootprintIngestion_R1.md`)

Rules on the two ARCH-gated questions the Format Expert's design (`DESIGN_SantpFootprintIngestion_R1.md`)
flagged before its proposed ticket sequence (85–93) can be dispatched: whether a NEW sandboxed
LuaJIT *execution* primitive is legal alongside the existing compile-only `LuaSyntaxCheck_SYS`
(§15.8), and how ingested real-game footprint data can influence generation without breaking
Constitution §4's determinism guarantee. Tickets 85–88 (the sandbox itself, the source scanner, the
dialect parser, the disk cache) proceed under §18.1 alone; ticket 89 (the orchestrator that actually
wires ingested data toward a PROC consumer) additionally requires §18.2.

**Q1, resolved in passing (not a separate ruling):** `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3's
"SanGen never calls into a Lua parser to read this file, **or any scenario content**, back" is
scoped to the Map Scenario system's own authored content (the emphasized phrase names the domain).
Template ingestion reads a different corpus in a different direction (game-shipped `.santp`/`.sanprop`
data, never round-tripped back into a `.sanmap`) and is not in tension with §15.3. No amendment to
§15.3 is needed; this paragraph is the requested confirmation.

---

### Subsections of §18

| § | File | Ruling |
|---|------|--------|
| §18.1 | [ARCH_18_01_SandboxedExecutionPrimitive.md](ARCH_18_01_SandboxedExecutionPrimitive.md) | `LuaTableEvaluate_SYS`/`LuaTableValue_SYS` — a sibling `SYS` primitive to `LuaSyntaxCheck_SYS`, sharing only the vendored LuaJIT library; the execution safety contract, binding |
| §18.2 | [ARCH_18_02_IngestedDataDeterminism.md](ARCH_18_02_IngestedDataDeterminism.md) | Determinism ruling — ingested footprint data may influence generation only after being baked into `PARAMS` at an explicit authoring-time action; never read live by `PROC` |

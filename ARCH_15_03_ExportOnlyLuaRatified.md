[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.3. **Only the ARCH Expert writes this file.**

### 15.3 Design ratified: option (c) — export-only, SanGen never parses Lua back (resolves §15.2's open question / `MAP_SCENARIO_SPEC.md` §8)

The human has settled the design question §15.2 named but left open. **Option (c): SanGen owns
scenario data; it does NOT parse Lua to read it back.** Export-only. **Rejected:** literal Lua
round-trip (parse and regenerate the tiered scenario tables verbatim, preserving hand-authored
comments and the ordering-significant `COUNT_SCENARIOS` array — option A), and reading scenario
data back out of the `.sanmap` at runtime from Lua (option B — live-verified wasteful: the
Pandemonium `.sanmap` is ~965 KB, so option B would JSON-decode roughly 1 MB twice per map load).

- Scenario data is **authored in SanGen and persisted in the `.sanmap`** as a new SanGen-owned
  schema-v3 section (§1.6 casing law — a new PascalCase top-level key, provisionally
  `Scenarios`; exact JSON shape is the Format Expert's follow-up, §15.7).
- On export, SanGen **renders** that data into the small generated
  `<MapName>_Scenarios_Data.lua` (`MAP_SCENARIO_SPEC.md` §2) — a one-directional PARAMS→text
  render, the same shape of operation `IO` already performs for the `.sanmap` JSON document, just
  targeting Lua table-literal text instead of JSON. SanGen never calls into a Lua parser to read
  this file, or any scenario content, back.
- **Live-verified fact grounding this choice:** a custom top-level `.sanmap` section is harmless
  at runtime even though `LoadMapData` (`common/mapUtils.lua`) builds `GameInfo.MapData` from an
  explicit whitelist (`props, decals, areas, armies, markers, chains, groups`) that silently
  drops any unrecognized top-level key — confirmed by injecting a synthetic
  `{"SanGenScenariosTest":{"magic":12345}}` section into the live Pandemonium `.sanmap` and
  running a real 1v1: the map loaded normally and the scenario system applied correctly. The new
  `Scenarios` section is therefore safe to persist in the `.sanmap` today even though nothing in
  the whitelist reads it — it exists for SanGen's own authoring/export round-trip, not for the
  game to consume directly (yet — §15.9).

**Scope note, added in response to `DESIGN_SantpFootprintIngestion_R1.md` §7 Q1 (ARCH §18.1):**
the "SanGen never calls into a Lua parser to read this file, or any scenario content, back" rule
above is scoped to the **Map Scenario system's own authored/exported content** — it does not
extend to reading a different corpus in a different direction, such as game-shipped
`.santp`/`.sanprop` template data via the separate, explicitly sandboxed, execution-capped
`LuaTableEvaluate_SYS` primitive (§18.1). That is read-only ingestion of external game data with no
round-trip back into a `.sanmap` or scenario file, not scenario content, and is not barred by this
section.

**Second scope note — AMENDED 2026-08-29 by [`ARCH_15_11_ForeignScenarioAreaImport.md`](ARCH_15_11_ForeignScenarioAreaImport.md)
§15.11. Do not read the "never … read this file, or any scenario content, back" sentence above in
isolation and conclude "never" without qualification.** §15.11 grants exactly one narrow,
permanently bounded carve-out: a human-triggered, one-shot, **non-executing** extraction of **area
rectangles only** (`Params::MapArea`) from a **foreign** scenario `.lua` — a hand-authored file
SanGen never writes (the legacy `<MapName>_Scenarios_Script.lua`, `<MapName>_data.lua`) —
mechanically enforced by refusing any input carrying `kScenarioGeneratedFileBannerLine` or either
SanGen-owned filename. This section's rule stands **absolutely** for everything else: every
SanGen-generated `.lua`, and every other kind of scenario content (scenario records, `match`,
`pattern`, `spawns`, `alloyMode`, unit spawns, `COUNT_SCENARIOS` ordering) from any `.lua`
whatsoever. Read §15.11's eleven boundary items before acting on this note.

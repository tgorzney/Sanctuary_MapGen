[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.9. **Only the ARCH Expert writes this file.**

### 15.9 Engine-whitelist migration path (recorded as intended future simplification, not built)

`LoadMapData` (`common/mapUtils.lua`) builds `GameInfo.MapData` from an explicit whitelist
(`props, decals, areas, armies, markers, chains, groups`) that silently drops any unrecognized
top-level `.sanmap` key — confirmed by reading the source (§15.3). This is why the generated
`<MapName>_Scenarios_Data.lua` (§15.4) is the transport to Lua **today**: the `Scenarios`
section persisted in the `.sanmap` (§15.3, §15.5) is real, round-trips safely, but is invisible
to the running game until something reads it.

**Recorded, not designed or scheduled:** a future one-line engine change adding `scenarios` to
`LoadMapData`'s whitelist would let `Scenario.ResolveAndApply` read `GameInfo.MapData.scenarios`
directly from the parsed `.sanmap`, at which point the generated `.lua` data file becomes
redundant and could be retired — SanGen would stop rendering `<MapName>_Scenarios_Data.lua` and
keep only the runtime-resource copy plus the `.sanmap` `Scenarios` section. This is an
**engine/game-dev-owned change**, not something SanGen controls or can schedule; nothing in this
ratification depends on it happening, and no coder should build toward it before it is real.


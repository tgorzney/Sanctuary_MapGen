[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.1. **Only the ARCH Expert writes this file.**

### 15.1 Layer classification
This system is **game-side Lua, not SanGen C++** — it runs inside the engine's own script
sandbox at map-load time, on the player's machine, not inside any SanGen process. It therefore
does not occupy a slot in the Constitution §1 `MATH`/`DATA`/`PARAMS`/`PROC`/`PIPELINE`/`IO`/
`UI`/`SYS` layer stack at all — those layers describe SanGen's own binary, and this is map
content the binary produces (or, per §15.2, may in future consume), not code SanGen executes.
Its prerequisite law (the `Import()` global-capture rule, the `LoadMapData()`/`CreateArmies()`/
`RunMapSetup()`/`NewThread` lifecycle) is itself game-engine law, not SanGen architecture, and is
recorded in `MODDING_SCRIPTING_SPEC.md` for that reason — a spec that documents the game's
scripting contract SanGen must respect, distinct from a spec governing SanGen's own module shape.


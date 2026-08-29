[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.1. **Only the ARCH Expert writes this file.**

### 22.1 Layer classification — game-side Lua, not SanGen C++

This technique is **game-side Lua**, executed inside the engine's own script sandbox at map-load
time, on the player's machine — not SanGen C++, not a SanGen process. It therefore does **not**
occupy a slot in the Constitution §1 `MATH`/`DATA`/`PARAMS`/`PROC`/`PIPELINE`/`IO`/`UI`/`SYS` layer
stack, for the exact same reason `ARCH_15_01_LayerClassification.md` §15.1 already gives for the
Map Scenario system: those layers describe SanGen's own binary, and this is map content /
map-authoring technique the binary may eventually help produce (§22.9), not code SanGen executes
today.

Its prerequisite law — the shared single `NewThread` per script, the `Import()` global-capture
rule, `pcall` discipline, the `IsHost`/`IsClient` per-Lua-state exclusivity — is itself game-engine
law, recorded in `MAP_UNIT_SPAWNING_SPEC.md` and `MODDING_SCRIPTING_SPEC.md` for the same reason
§15.1 gives: it documents the game's scripting contract SanGen-authored content and human-authored
content alike must respect, not SanGen's own module shape.

# DESIGN — Map Scenario IO (R1)

*Authored by the SanGen IO Architecture Expert, 2026-08-21. Design only — no code.
Grounded against `MAP_SCENARIO_SPEC.md` (full), `ARCH_15_MapScenarioSystem.md` §15/§1.5–§1.8/§2/§3.1,
`IO_MIGRATION_SPEC.md` (full), and the real `src/io/MapExporter_IO.h`,
`AppSettings_IO.h`, `AppSettingsLocation_IO.h`, `FilesystemPrimitives_IO.h`,
`CMakeLists.txt`. No `Params::*Scenario*` type exists in `src/params/` yet — confirmed.*

> **⚠️ AMENDED 2026-08-28 — the naval-fleet worked examples are corrected in place.**
> As authored, §2's link-mechanism example wired `Scenario.SpawnNavalFleets` alongside
> `Scenario.ResolveAndApply`, and §6's WO6 bullet described porting the live
> `FindMatchingScenario`/`ApplyScenario`/`SpawnNavalFleets` trio into generic form. **Both named a
> function that no longer exists.** The 2026-08-27 rewrite of the live reference script deleted
> `Scenario.SpawnNavalFleets` and every `NAVAL_*` constant, and on 2026-08-28 the vestigial `navy`
> scenario field was removed from the live Lua after being confirmed to have zero readers
> (`Pandemonium Isthmus_Scenarios_Script.lua:182-186`).
> `ARCH_15_05_ParamsScenariosType.md`'s "RETIRED 2026-08-28" section retires the whole
> `ScenarioNavalFleet` family plus `ScenarioBody::navalFleet`/`navy`; the replacement opt-in is a
> plain `ScenarioBody::spawnsUnits` bool. The two examples below are updated; **nothing else in
> this design changes** — the two-IO-surface split (§0), the file set (§1), the overwrite-safety
> rule (§2), the runtime-resource resolution (§3), the Lua syntax check (§4), the export result
> contract (§5), and the WO ordering (§6) are all independent of which spawn function the runtime
> exposes.
>
> Two pre-existing staleness notes still stand and are **not** re-litigated here: the type is
> ratified as `Params::Scenarios`, not `Params::MapScenario` (STEP69's own correction), and the
> ratified on-disk shape is the map-prefixed `<MapName>_Scenarios_Runtime.lua` /
> `<MapName>_Scenarios_Data.lua` pair, not `SanGenScenarioRuntime.lua` +
> `<MapName>_Scenarios_Script.lua` (STEP70's own naming correction, which also retires §2's
> non-prefix exception and the matching ❓ open item below).

## 0. Load-bearing clarification — there are TWO IO surfaces, only one is new

1. **`Params::MapScenario` round-tripped inside the `.sanmap`** — an ordinary domain.
   **Fully reuses** the existing convention: `MapExporter_MapScenario_IO.cpp` /
   `MapImporter_MapScenario_IO.cpp`, `JsonPrimitives_IO`, and is a normal future
   `<Domain>_Migrate_V<N>_IO` candidate if its shape changes. This is how a human
   authors/edits/reloads scenario rules *inside SanGen itself*. This section is
   dropped by the game's `LoadMapData` whitelist today — **zero effect on the live
   game** until leg 2 also runs.
2. **PARAMS → `.lua` rendering into the game's script tree** — export-only, **new**
   convention. This is the actual scope of the human's ruling and of this design.

"Never contaminate the map-export result contract" applies to leg 2 only; leg 1 is an
ordinary domain already covered by `MapExportResult`.

## 1. Revised `src/io/` file set

Naming law: ARCH_01_05_FileSizeCeilings.md §1.5 `Type_Aspect_LAYER`. Type = `ScenarioScript`.

| File | Responsibility |
|---|---|
| `MapExporter_MapScenario_IO.cpp` / `MapImporter_MapScenario_IO.cpp` | Ordinary `.sanmap` JSON leg (§0.1) — `Params::MapScenario` round-trip. Reuses `JsonPrimitives_IO` normally. |
| `ScenarioScript_DataLua_IO.h/.cpp` | Pure, disk-free: `Params::MapScenario` → generated `<MapName>_Scenarios_Script.lua` text. Mirrors `MapExporter_IO::BuildSanmapJsonText`'s pure-builder shape. |
| `ScenarioScript_RuntimeResource_IO.h/.cpp` | Resolves + reads runtime text: bundled default vs. settings override, with loud-fallback contract (§3). |
| `GameInstallLocation_IO.h/.cpp` | `ValidateGameInstallRoot(candidateRoot) -> {bValid, reason}` — checks `<root>/engine/LJ/lua` **and** `<root>/Sanctuary_Data/Maps` both exist. Pure filesystem; no platform API, so unlike `AppSettingsLocation_IO.h` it needs no `_Shell_IO.cpp` split. UI owns the picker; this owns only validation. |
| `LuaTableWriter_IO.h` | Header-only `inline` pure functions — the Lua-literal twin of `JsonPrimitives_IO.h`: string escaping, table-constructor open/close, key=value emission, array-of-tables emission, and rendering of the declarative comparator vocabulary. Paired `LuaTableWriter_IO_Test.cpp`. |
| `LuaSyntaxCheck_IO.h/.cpp` | Compile-only validation, embedded Lua. Contract in §4. |
| `ScenarioScript_Export_IO.h/.cpp` | Orchestrator: given `gameInstallRoot`, `mapName`, `Params::MapScenario`, runtime text — writes the two SanGen-owned files under the overwrite-safety rule (§2), checks `_data.lua` presence (warn-only), returns `ScenarioExportResult` (§5). The one entry point UI calls. |
| `Io::AppSettings` (existing file, new fields) | `std::string gameInstallRoot;` and `std::string scenarioRuntimeOverridePath;` (empty ⇒ bundled default). |

### Not reused from `IO_MIGRATION_SPEC.md`, and why
- **No `<Domain>_Migrate_V<N>_IO`.** Export-only means there is no persisted document
  shape to walk forward — nothing to migrate *from*. If `Params::MapScenario`'s JSON
  shape changes later, that is leg 1's ordinary concern (§0.1).
- **No `Sanmap_MigrationManifest_IO` / `Sanmap_MigrationRunner_IO`.** Same reason.
- **`JsonPrimitives_IO` does not cover this leg.** Lua literal syntax (table
  constructors, escaping, ordered arrays where TIER 2 order is semantically
  load-bearing) is a different grammar. `LuaTableWriter_IO` is the parallel toolkit,
  homed by the same reasoning `JsonPrimitives_IO` used — never merged into it.

## 2. The three files in `LJ/lua/maps/<MapName>/`

| File | Author | SanGen write rule |
|---|---|---|
| `<MapName>_data.lua` | Human, always | **SanGen never writes this path, ever.** Absolute rule, not content-based. If absent at export: `bOrchestratorPresent = false`, loud warning ("scenario files written but inert until `<MapName>_data.lua` exists"), never blocks the other two writes. |
| `SanGenScenarioRuntime.lua` | SanGen (verbatim copy of bundled resource) | **Always overwritten unconditionally.** No legitimate hand-authored variant exists by design — every map's copy is byte-identical to its source at that export. |
| `<MapName>_Scenarios_Script.lua` | SanGen (generated from `Params::MapScenario`) | **Marker-gated overwrite.** Generated files open with a fixed banner (`-- GENERATED BY SANGEN — DO NOT HAND-EDIT...`). Before writing, read the existing first line. Banner present or file absent → overwrite freely. Banner absent → **do not touch it**; write to sibling `<MapName>_Scenarios_Script.sangen-pending.lua`, set `bDataLuaCollisionDetected = true`, log loudly naming both paths. Never a silent skip, never a silent overwrite of unknown content. |

**Key design property — the existing call site never changes.** `_data.lua`'s
`Import("maps/<MapName>/<MapName>_Scenarios_Script.lua").Scenario` stays exactly as-is.
The generated data file keeps the pre-existing filename and exposes the global
`Scenario` table itself; internally it does
`Import("maps/<MapName>/SanGenScenarioRuntime.lua").SanGenScenarioRuntime` and wires
`Scenario.ResolveAndApply = function(...) return SanGenScenarioRuntime.ResolveAndApply(PATTERN_SCENARIOS, COUNT_SCENARIOS, DEFAULT_SCENARIO, ...) end`
(similarly `SpawnUnits`, the generic instruction executor — **amended 2026-08-28**, this example
named `SpawnNavalFleets`, retired; ⚠️ the `SpawnMatchedScenarioUnits` dispatch and its
per-scenario generators are deliberately **not** wired here, because where per-map procedural
scenario Lua lives under the ratified split is unresolved — `ARCH_15_05_ParamsScenariosType.md`
OPEN item 2). This is why the runtime can be a byte-identical,
non-map-prefixed, verbatim copy: it never needs to know which map it is in — the
map-specific data file calls into it, not the reverse.

⚠️ `SanGenScenarioRuntime.lua` is **not** map-name-prefixed — a deliberate narrow
exception to `MAP_SCENARIO_SPEC.md` §2's prefix rule, justified because that rule
predates the runtime/data split and was written for a pair whose *content* varies per
map; the runtime's content deliberately never varies. **This exception must land in the
ARCH/spec amendment, not be asserted only here.**

**Live migration of `Pandemonium Isthmus_Scenarios_Script.lua`:** it is hand-authored
and carries no banner. The first scenario-leg export hits the collision case — writes
`...sangen-pending.lua` beside it, does not touch the live file, logs the conflict. A
human reviews/diffs once and swaps it in manually; every export after is a clean
overwrite. No automatic migration — content equivalence is a human judgment call.

## 3. Bundled runtime resource — location, resolution, override

- **Source-tree home:** new top-level `resources/lua/SanGenScenarioRuntime.lua`, sibling
  to `src/` — it is not program code, it is shipped Lua text. ⚠️ **New top-level tree
  not covered by ARCH_02_LayerDirectoryMap.md §2's layer map — needs ARCH sign-off, not settled here.**
- **Build staging:** identical to the existing `SANGEN_V2_SHADER_DIRECTORY` `.glsl`
  pattern in `CMakeLists.txt` — `configure_file` into
  `${CMAKE_CURRENT_BINARY_DIR}/sangen_lua_resources/`, then a `POST_BUILD` copy beside
  the executable, mirroring `sangen_shaders`. No absolute paths.
- **Resolution (`LoadScenarioRuntimeText`):**
  1. `scenarioRuntimeOverridePath` non-empty and readable → use it, `sourceDescription = "override"`.
  2. Else `<exeDir>/sangen_lua_resources/SanGenScenarioRuntime.lua`, `sourceDescription = "bundled"`.
  3. Override set but unreadable → **degrade to bundled, logged loudly** (Constitution
     §6 — never a silent swap, never a hard failure).
- **"SanGen shipped a newer runtime, user has local edits":** resolved structurally, not
  by diffing. The bundled file is never user-writable in place; a version bump replaces
  its content at the same staged path. User edits live *only* in the override file, a
  physically separate path the in-app editor writes to. Recommended (coder-tier, not
  binding): a first-line `-- SanGenScenarioRuntimeVersion: N` marker so an advisory
  (not a block) can be logged when an in-use override trails the bundled version.

## 4. Compile-only Lua validation — home and contract

**Home: `LuaSyntaxCheck_IO.h/.cpp`, IO layer.** Both the UI editor (validate as the user
types) and `ScenarioScript_Export_IO` (pre-write safety net) need it. UI already depends
on IO headers (`FilesTab_Draw_UI.cpp` includes `MapExporter_IO`), so IO is the only
shared home keeping the dependency downward-only.

```cpp
struct LuaSyntaxCheckResult { bool bSucceeded = false; int lineNumber = 0; std::string message; };
LuaSyntaxCheckResult CheckLuaSyntax(const std::string& luaSourceText);
```

- Wraps `luaL_loadbuffer`/`luaL_loadstring` **only**. The loaded chunk is immediately
  popped and discarded. Doc comment must state: **never call `lua_pcall`/`lua_call` on
  the loaded chunk** — this function loads and nothing else.
- `lineNumber`/`message` come from Lua's own compile-error string; `0`/empty on success.
- The `.h` exposes only this struct + function — no `lua.h` types leak, matching how
  `SanpackReader_IO.h` hides `miniz`.
- **New dependency:** embedded Lua, linked `PRIVATE` to the translation units needing it
  (mirrors `nlohmann_json`'s `PRIVATE` link). SanGen links no Lua today. Vendor choice
  (Lua 5.4 vs LuaJIT compile-only) is coder-tier, deferred.
- ImGuiColorTextEdit is UI Expert's domain; it consumes this contract.

## 5. Export entry-point wiring

**Not folded into `ExportAll`/`ExportSanmapOnly`.** A separate call:

```cpp
ScenarioScript_Export_IO::ExportMapScenario(
    gameInstallRoot, mapName, scenarioParams,
    runtimeResourceDirectory, runtimeOverridePathOrEmpty) -> ScenarioExportResult;

struct ScenarioExportResult {
    bool bDataLuaWritten = false;
    bool bRuntimeCopied = false;
    bool bOrchestratorPresent = false;      // false => warn: files written but inert
    bool bDataLuaCollisionDetected = false; // unrecognized existing file; wrote .sangen-pending sibling
    std::vector<std::string> writtenFilePaths;
    std::string debugLog;
    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
};
```

Mirrors `MapExportResult`'s shape but is a **distinct type, never merged**. The Files tab
calls the two exports independently: `.sanmap`/asset export first, scenario leg second,
gated on a valid `gameInstallRoot` (UI prompts if unset, using
`ValidateGameInstallRoot`). A scenario-leg failure surfaces in its own banner and never
sets `MapExportResult.bSucceeded = false`, or vice versa — this is the literal mechanism
satisfying "must never block the `.sanmap`/asset export."

## 6. Work-order outline (dependency order)

1. **`Params::MapScenario` + ordinary `.sanmap` round-trip** — foundational; every other
   WO consumes this type. Needs the ratified comparator-vocabulary shape first.
2. **`LuaTableWriter_IO`** — no dependencies; parallel with WO1.
3. **`GameInstallLocation_IO` + `Io::AppSettings` new fields** — no dependencies; parallel.
4. **`LuaSyntaxCheck_IO` + CMake Lua wiring** — no dependencies on WO1–3, but establishes
   the new third-party link; do early so WO5/WO6 can call it without a follow-up CMake edit.
5. **`ScenarioScript_DataLua_IO`** — depends on WO1 (PARAMS shape) and WO2 (primitives);
   optionally WO4 for a self-check of its own output.
6. **`ScenarioScript_RuntimeResource_IO` + the bundled `resources/lua/SanGenScenarioRuntime.lua`
   content** (a coder-tier port of the live `FindMatchingScenario`/`ApplyScenario`/`SpawnUnits`
   into generic, tier-table-parameterized form — **amended 2026-08-28**, this bullet named
   `SpawnNavalFleets`, retired; `SpawnUnits` is already generic in the live file and needs no
   parameterization, only relocation) + CMake staging — depends on WO3; **needs ARCH sign-off on
   the `resources/` location first**.
   ⚠️ **Explicitly out of WO6's scope, and blocked:** `Scenario.SpawnMatchedScenarioUnits` and the
   per-scenario generator functions it dispatches to are per-map, per-scenario, AND procedural —
   a category the ratified three-file split has no home for
   (`ARCH_15_05_ParamsScenariosType.md` OPEN item 2). They cannot go in a byte-identical runtime
   resource. Do not port them into this file to make the port "complete."
7. **`ScenarioScript_Export_IO`** — orchestrator; depends on WO3, WO5, WO6.
8. **UI wiring** (Files tab second export call, result surfacing, `gameInstallRoot`
   prompt, ImGuiColorTextEdit editor calling WO4) — UI Expert's work-order, depends on
   WO3 + WO7 contracts.

## Coder briefing staleness (for the human to apply)

`.claude/agents/sangen-coder.md`, after the existing IO-layer conventions bullet, add:

> **Map Scenario Lua export (`MAP_SCENARIO_SPEC.md`, ARCH_15_MapScenarioSystem.md §15)** is a separate,
> non-migration IO surface — `ScenarioScript_*_IO` files render `Params::MapScenario` to
> `.lua` text one-way (export only, never read back); compose `LuaTableWriter_IO`, not
> `JsonPrimitives_IO`. Consult the IO Architecture Expert before treating any
> scenario-Lua file as a `<Domain>_Migrate_V<N>_IO` candidate — it is not one.

## ❓ Open items (flagged, not settled here)
- Exact repo location of `resources/lua/` + CMake staging — new top-level tree, needs ARCH sign-off.
- `SanGenScenarioRuntime.lua`'s non-map-prefixed exception to `MAP_SCENARIO_SPEC.md` §2 —
  must land in the ARCH/spec amendment.
- Exact comparator vocabulary (fields/ops) replacing TIER 2 `match` closures — Format
  Expert content-truth; `LuaTableWriter_IO` only renders whatever shape is ratified.
- Embedded Lua vendor choice (Lua 5.4 vs LuaJIT compile-only) — coder-tier, WO4.

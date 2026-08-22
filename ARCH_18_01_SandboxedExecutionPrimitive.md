[← ARCH index](ARCH.md) · [§18 ARCH_18_SantpFootprintIngestion](ARCH_18_SantpFootprintIngestion.md) · SanGen ARCH §18.1. **Only the ARCH Expert writes this file.**

### 18.1 `LuaTableEvaluate_SYS`/`LuaTableValue_SYS` — sibling `SYS` primitive, sandboxed-execution contract

**Ruled: yes, a sibling primitive — approved as proposed (ticket 85), with the safety contract
below binding.** `DESIGN_SantpFootprintIngestion_R1.md` §2 correctly identifies that
`LuaSyntaxCheck_SYS` (`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8, `STEP65_LuaSyntaxCheck_SYS.md`)
cannot be widened to do this job: its entire contract is compile-only/never-execute, proven by a
killer test (`CheckLuaSyntax("while true do end")` returns in bounded time *because* nothing is ever
run). A `.santp`/`.sanprop` file is a Lua assignment statement, not a data literal — obtaining
`footprint.x` requires **executing** the chunk and reading the resulting global table, which
`luaL_loadbuffer` alone never does. Widening `LuaSyntaxCheck_SYS` to execute would delete the exact
property its own acceptance test exists to defend, for a caller `LuaSyntaxCheck_SYS` was never
designed to serve. **This ruling forbids that widening explicitly, permanently — not just for this
ticket.**

- **What is shared, and only this:** the vendored LuaJIT library and its CMake build wiring
  (`STEP65_LuaSyntaxCheck_SYS.md` §3 — `FetchContent`/`src/third_party/`, `PRIVATE` link). No second
  Lua vendoring, no dialect substitution — this primitive embeds the same LuaJIT build STEP65
  vendors, for the same reason (`LJ/lua/` is the engine's own script root).
- **Home: `SYS`**, same reasoning as §15.8 — a runtime-primitive/library-integration concern, the
  same footing as `GpuResource_SYS`/`ThreadPool_SYS`/`LuaSyntaxCheck_SYS` itself. It must be
  reachable from `IO` (the ingestion orchestrator, `TemplateIngest_IO` and its collaborators) —
  already legal via §15.8's `IO → SYS` correction to `ARCH_03_ModuleBoundaries.md` §3.1; no further
  dependency exception is needed.
- **Files, confirmed as proposed:** `src/sys/LuaTableValue_SYS.h` (an owned plain-C++ value tree —
  nil/boolean/number/text/array/table; no LuaJIT type crosses this header) and
  `src/sys/LuaTableEvaluate_SYS.h`/`.cpp` (the sandbox itself — the only other translation unit
  besides `LuaSyntaxCheck_SYS.cpp` permitted to include LuaJIT headers directly). Naming conforms to
  §1.1 (literal, no abbreviation) and `ARCH_02_LayerDirectoryMap.md`; the design's own budgeted line
  counts are estimates, not binding — §1.5's ceilings are.

**The execution safety contract — binding, extends Constitution §6 to actual execution of untrusted
Lua text (a materially stronger requirement than §15.8's compile-only contract, not a restatement of
it):**

1. **Zero standard libraries opened.** No `luaL_openlibs`, categorically no LuaJIT `ffi` (native
   code execution). Inherited from §15.8 constraint 2 verbatim — now protecting a state that
   actually runs code, which makes this constraint the single most load-bearing one in the set.
2. **An instruction-count debug hook** (`lua_sethook` with `LUA_MASKCOUNT`) that aborts the chunk
   once a configured budget is exceeded. This is the mechanism that makes execution safe at all —
   `LuaSyntaxCheck_SYS` achieved its safety by never executing; this primitive achieves it by
   bounding what execution can do.
3. **A byte-size cap on the source text** before it is ever handed to Lua (Constitution §6's "cap
   file size," applied literally) **and** a cap on total evaluated table nodes, so a pathological
   file cannot exhaust memory building its result tree even within the instruction budget.
4. **`lua_pcall` only. `lua_call` must not appear anywhere in this primitive.** A runtime error is a
   returned failure, never a longjmp through C++ frames — identical reasoning to §15.8's
   `lua_pcall`/`lua_call` prohibition, now enforced on a state that genuinely calls into loaded code.
5. **A fresh `lua_State` per file, closed on every exit path** — including early-return and
   error branches. No shared state between files: no cross-file contamination, no accumulated
   globals, and every file's fan-out (`Sys::ThreadPool`) is embarrassingly parallel with no
   synchronization required, by construction.
6. **The result crosses the header only as an owned `Sys::LuaTableValue` tree; the `lua_State` is
   closed before the call returns.** No LuaJIT type and no pointer into Lua-owned memory outlives
   the call or crosses `LuaTableEvaluate_SYS.h` — the same opaque-header discipline
   `SanpackReader_IO.h` already uses for `miniz`, applied here to LuaJIT.

**Standing constraint, stated so it is never silently violated:** this contract governs
`LuaTableEvaluate_SYS` only. It does not relax, and must never be read as relaxing,
`LuaSyntaxCheck_SYS`'s own never-execute contract (§15.8) — the two primitives share a library, a
layer, and nothing else. A future ticket that skips `LuaTableEvaluate_SYS`'s sandbox constraints
because "the text already passed `CheckLuaSyntax`" is violating this ruling: syntax validity says
nothing about what an executed chunk does within its instruction budget, and `CheckLuaSyntax` gives
no safety guarantee whatsoever once execution is involved.

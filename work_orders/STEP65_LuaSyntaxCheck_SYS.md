# STEP65 — `LuaSyntaxCheck_SYS` + embedded LuaJIT build wiring

**Layer:** SYS (relocated from IO by ARCH ruling — see below). **Domain:** compile-only Lua syntax
validation. **Sequence:** Map Scenario IO track, `work_orders/DESIGN_MapScenarioIO_R1.md` §6,
Work-Order 4 of 8. No dependency on STEP63/STEP64; **introduces the new third-party LuaJIT
dependency every later scenario work-order (WO5–WO8) relies on** — land this before them so no
follow-up CMake edit is needed.

## Root problem
`DESIGN_MapScenarioIO_R1.md` §4 originally proposed `LuaSyntaxCheck_IO.h/.cpp` in the IO layer.
**`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 corrects this: the home is `SYS`, not IO alone**, because the feature has two
call sites — `UI` (live red-squiggle feedback while a designer types in the future
`LuaCodeEditor_UI` editor widget) and `IO` (`ScenarioScript_Export_IO`'s pre-write safety net on the
override-path runtime file) — and `SYS` is the only layer both can legally reach downward to.
`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 also formalizes `IO → SYS` as a legal dependency direction (§3.1 correction), citing
the real, already-shipped precedent `src/io/AssetAtlasCache_IO.cpp` including `../sys/ThreadPool_SYS.h`.

**Dialect ruling (binding, do not substitute):** embed **LuaJIT itself, not vanilla/stock Lua**. The
engine's script tree is rooted at `LJ/lua/` — "LJ" is LuaJIT — so the runtime this validator must
agree with is LuaJIT's own grammar (closer to Lua 5.1 than 5.2+; no 5.2+ `goto`/integer-division
semantics baked in). A vanilla-Lua validator risks both false-accepts and false-rejects relative to
what the real game engine will actually load.

## Fix

### 1. New file: `src/sys/LuaSyntaxCheck_SYS.h`
No `lua.h` type leaks into this header — same discipline `SanpackReader_IO.h` already uses to hide
`miniz` behind an opaque interface.
```cpp
// LuaSyntaxCheck_SYS.h — compile-only Lua syntax validation over an embedded LuaJIT state.
// Layer: SYS (`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 — a runtime-primitive/library-integration concern, same footing as
// GpuResource_SYS/ThreadPool_SYS). Reachable from BOTH UI (live editor feedback) and IO
// (ScenarioScript_Export_IO's pre-write safety net) — SYS is the one shared home keeping the
// dependency downward-only for both call sites.
//
// HARD SAFETY CONTRACT (Constitution §6, "validate all input" applied to embedding a language
// runtime specifically to validate UNTRUSTED text a human is actively typing):
//   1. The Lua state is used EXCLUSIVELY for compilation (luaL_loadbuffer/luaL_loadstring). The
//      loaded chunk is popped and discarded immediately. lua_pcall/lua_call -- or any other
//      execution entry point -- on the loaded chunk is FORBIDDEN. This function's entire purpose
//      is syntax-checking; it must never run what it loads.
//   2. ZERO standard libraries opened (luaL_openlibs is never called -- including, especially,
//      LuaJIT's native `ffi` library). Compilation needs no library table at all.
//   3. Diagnostics come from Lua's own compiler error string (`chunk:line: message`) -- no
//      separate line-tracking reimplementation.
// No `lua.h`/LuaJIT type is declared or forward-declared here -- only std types cross this header,
// matching how SanpackReader_IO.h hides miniz behind an opaque interface.
#pragma once
#include <string>

namespace SanmapGen {
namespace Sys {

struct LuaSyntaxCheckResult {
    bool        bSucceeded = false;
    int         lineNumber = 0;     // 0 on success or when the compiler's message carried none
    std::string message;            // Lua's own compile-error text; empty on success
};

// Compiles (never executes) `luaSourceText` against an embedded LuaJIT state opened with ZERO
// standard libraries. A syntactically valid chunk that references undefined globals (e.g. `os`,
// `print`) still reports bSucceeded == true -- compilation does not resolve globals, only parses.
// Total: never throws, never blocks on anything but the compiler itself, safe to call once per
// keystroke from a UI editor.
LuaSyntaxCheckResult CheckLuaSyntax(const std::string& luaSourceText);

} // namespace Sys
} // namespace SanmapGen
```

### 2. New file: `src/sys/LuaSyntaxCheck_SYS.cpp`
Wraps LuaJIT's C API directly — the ONLY translation unit in `src/` allowed to include LuaJIT
headers (wrap in `extern "C" { ... }` if the vendored `lua.h` does not already guard for C++).
Implementation contract (prose, per Constitution — the coder writes the body):
- `luaL_newstate()` — never `luaL_openlibs()`.
- `luaL_loadbuffer(L, luaSourceText.data(), luaSourceText.size(), "=ScenarioScript")` (or
  `luaL_loadstring`, per the header's own doc comment — either is acceptable, both are covered by
  the "load-only" contract).
- On `LUA_OK`: `lua_pop(L, 1)` to discard the compiled chunk without ever calling it;
  `bSucceeded = true`.
- On failure: read the error string via `lua_tostring(L, -1)`, parse `lineNumber` out of the
  standard `chunk:LINE: message` prefix (best-effort — a malformed prefix still returns the raw
  message text with `lineNumber = 0` rather than crashing), `lua_pop(L, 1)`.
- `lua_close(L)` on every exit path, including early-return branches — no leaked Lua state.
- **`lua_pcall`/`lua_call` must not appear anywhere in this file.** This is the security property of
  the whole feature — flag it in the file's own top comment as loudly as the header does.

### 3. CMake — vendor LuaJIT, link `PRIVATE`
Mirrors `nlohmann_json::nlohmann_json`'s existing `PRIVATE` link (`CMakeLists.txt:202`) and the
`imgui`/`glfw`/`miniz` `FetchContent` precedents (`CMakeLists.txt:13-34,84-93`) — no new
dependency-addition law needed (`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8, §7.3).
- Add a `FetchContent_Declare`/`FetchContent_MakeAvailable` block for **LuaJIT specifically** (not
  vanilla Lua), or vendor an amalgamated/pre-built LuaJIT under `src/third_party/` if a CMake-native
  LuaJIT source is not practical — **LuaJIT's own upstream build system is Makefile/MSVC-batch based,
  not CMake-native; the exact vendoring mechanism (a CMake-wrapped fork via `FetchContent`, vs. a
  pre-built static lib checked into `src/third_party/`, vs. driving `msvcbuild.bat` from a CMake
  custom command) is the coder's call at implementation time, per `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8's own "the
  coder's call, per §7.3's existing precedent" ruling — verify the chosen source is genuine LuaJIT
  before committing to it, not assumed here.**
- `target_link_libraries(SanGenV2 PRIVATE <the resulting LuaJIT target>)` — PRIVATE so no consumer
  of `SanGenV2`'s public headers gains a Lua include-path dependency (`LuaSyntaxCheck_SYS.h` leaks
  none). If a static LuaJIT archive results, CMake propagates it transitively to any executable
  that links `SanGenV2` (the same transitive-static-link behavior `miniz.c`'s direct
  `target_sources` already relies on) — no per-test relink is needed unless a test includes LuaJIT
  headers directly, which none in this ticket do.
- `add_sangen_test(LuaSyntaxCheck_SYS_Test src/sys/LuaSyntaxCheck_SYS_Test.cpp)` near the existing
  SYS test block (`ArenaAllocator_SYS_Test`/`ThreadPool_SYS_Test`, `CMakeLists.txt:268-275`) — no
  extra `target_link_libraries` line needed for the same reason (the test calls only
  `Sys::CheckLuaSyntax(std::string)`, never a LuaJIT symbol directly).

## Files touched
- NEW `src/sys/LuaSyntaxCheck_SYS.h` — the code block above, verbatim.
- NEW `src/sys/LuaSyntaxCheck_SYS.cpp` — per the implementation contract above.
- NEW `src/sys/LuaSyntaxCheck_SYS_Test.cpp`.
- `CMakeLists.txt` — LuaJIT vendoring block, `SanGenV2` `PRIVATE` link, one new `add_sangen_test` line.

## Backend policy
CPU only. Called at most once per user keystroke in a future editor widget (interactive, not a
per-frame hot path) or once per export-time safety check — not a `Dispatch_SYS` concern, no SIMD,
no GPU handle. Embedding a language runtime for compile-only validation is a library-integration
concern, the same category `GpuResource_SYS`/`ThreadPool_SYS` already occupy.

## ARCH rules invoked
- `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 — the full ruling this ticket implements: `SYS` home (not IO alone), LuaJIT
  specifically (not vanilla Lua), the `IO → SYS` §3.1 correction, and every safety constraint above
  verbatim.
- `ARCH_03_ModuleBoundaries.md` §3.1 — `IO`'s allowed-dependency row now includes `SYS` (formalized by this same
  ruling, grounded in the pre-existing `AssetAtlasCache_IO.cpp` → `ThreadPool_SYS.h` precedent).
- Constitution §6 — "validate all input," applied specifically to embedding an interpreter for
  untrusted-text validation: the never-execute / zero-libraries-opened constraints are the concrete
  mechanism, not a restatement of the general rule.
- §7.3 — vendoring precedent (`FetchContent` or `src/third_party/`, coder's call).
- `SanpackReader_IO.h` — the header-hides-the-vendored-library precedent this ticket's `.h` mirrors
  for LuaJIT exactly as that file already does for `miniz`.

## Explicit out-of-scope
- **`ImGuiColorTextEdit`/`LuaCodeEditor_UI`** — `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 homes this in `UI`
  (`src/ui/LuaCodeEditor_UI.h/.cpp`), a separate, later work-order (WO8) that CALLS this contract;
  not built here.
- **Any call site wiring** — neither `ScenarioScript_Export_IO` (WO7) nor any UI editor calls
  `CheckLuaSyntax` yet; this ticket ships the primitive and its build wiring only.
- **Executing loaded Lua in any form** — explicitly and permanently out of scope for this file, not
  a deferred feature; see the hard safety contract above.
- **Vanilla/stock Lua** — rejected by `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8's dialect ruling; do not substitute a more
  convenient Lua 5.4 (or other) build to sidestep LuaJIT's harder-to-vendor build system.

## Acceptance test
New `src/sys/LuaSyntaxCheck_SYS_Test.cpp` (registered in `CMakeLists.txt`):
- **The never-execute property, the killer test.** `CheckLuaSyntax("while true do end")` returns
  `bSucceeded == true` and returns within a bounded wall-clock budget (e.g. under one second,
  measured in the test) — a syntactically valid infinite loop that, if this implementation ever
  called `lua_pcall`/`lua_call` on the loaded chunk, would hang the test process forever. This is
  the concrete, loud, testable proof of the security property named in the header's own top comment.
- **The zero-libraries property, second proof.** `CheckLuaSyntax("os.execute('nothing')")` (or any
  syntactically valid reference to a global from a standard library that was never opened) still
  returns `bSucceeded == true` — compilation succeeds regardless of whether `os` exists as a global,
  because nothing is ever executed to resolve it. If this ever regressed to opening `luaL_openlibs`
  and then, wrongly, executing the chunk, this specific script becomes a live proof of the mistake.
- A syntax error (e.g. an unterminated `function foo(`) returns `bSucceeded == false`, a non-empty
  `message`, and a `lineNumber` matching the injected error's actual line.
- An empty string and a whitespace-only string both compile successfully (`bSucceeded == true`) —
  an empty chunk is valid Lua, never treated as an error.
- A multi-line script with the syntax error on line 3 (not line 1) reports `lineNumber == 3` —
  proves line extraction is real, not hardcoded to the first line.
- Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; the new
  `LuaSyntaxCheck_SYS_Test` target passes; the LuaJIT vendoring block resolves cleanly on a from-
  scratch configure (no manually-pre-fetched dependency required, same "clone and build" bar every
  existing `FetchContent` dependency already meets).

## Verify
- New `src/sys/LuaSyntaxCheck_SYS_Test.cpp` passes, especially the two proof-of-non-execution tests.
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files edited or broken.
- Confirm at implementation time that the vendored LuaJIT build is genuine LuaJIT (check its own
  version banner / `LUAJIT_VERSION` macro), not a mislabeled vanilla-Lua substitute — the dialect
  ruling is load-bearing, not cosmetic.

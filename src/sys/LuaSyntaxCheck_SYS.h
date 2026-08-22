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

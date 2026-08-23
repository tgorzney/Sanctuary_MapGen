// LuaTableEvaluate_SYS.h — sandboxed LuaJIT execution: runs a .santp/.sanprop Lua chunk (a global
// table assignment, e.g. `UnitTemplate = {...}`) and returns every global it set as an owned
// Sys::LuaTableValue tree. Layer: SYS — a SIBLING primitive to LuaSyntaxCheck_SYS
// (ARCH_18_01_SandboxedExecutionPrimitive.md §18.1), sharing ONLY the vendored LuaJIT library and
// its CMake build wiring. LuaSyntaxCheck_SYS's own never-execute contract is UNCHANGED and
// UNRELATED — this is a materially different, STRONGER safety posture (actual execution of
// untrusted text), never a widening of that primitive (§18.1's own standing constraint, binding).
// No lua.h type leaks into this header — same discipline SanpackReader_IO.h uses for miniz.
//
// Deliberately domain-free: no knowledge of "UnitTemplate"/tpId/footprint here — see this ticket's
// own "Deliberate design decision" section. TemplateDialect_IO (ticket 87, IO) does that work.
#pragma once
#include "LuaTableValue_SYS.h"
#include <cstdint>
#include <string>

namespace SanmapGen {
namespace Sys {

// Every cap is a setting with a sane default (Constitution §8) — sized against the real corpus
// this sandbox targets: 546 files, 3.9 KB average, 35.8 KB largest observed
// (DESIGN_SantpFootprintIngestion_R1.md §1.1). Defaults below are generous multiples of that
// observed maximum, not the maximum itself.
struct LuaTableEvaluateLimits {
    std::size_t  maximumSourceByteSize   = 256 * 1024;   // Constitution §6 "cap file size," literal
    std::int64_t maximumInstructionCount = 2'000'000;    // lua_sethook(LUA_MASKCOUNT) budget
    std::size_t  maximumTableNodeCount   = 50'000;       // caps the C++ tree even within budget
};

struct LuaTableEvaluateResult {
    bool          bSucceeded = false;
    std::string   errorMessage;   // populated on ANY failure: oversize source, compile error,
                                   // runtime error, instruction-budget exceeded, node-count exceeded
    LuaTableValue globals;        // Table-kind; every global the chunk set. Valid only when
                                   // bSucceeded == true.
};

// Executes luaSourceText in a FRESH, zero-library lua_State (LuaTableEvaluate_SYS.cpp's hard safety
// contract, ARCH_18_01_SandboxedExecutionPrimitive.md §18.1 verbatim: zero stdlibs, instruction-
// count hook, byte/node caps, lua_pcall only, fresh state per call, closed before return). Total:
// never throws, never blocks past the instruction budget, safe to call once per file from a
// ThreadPool fan-out with NO shared state between calls (a fresh lua_State per call, by contract).
LuaTableEvaluateResult EvaluateLuaTableSource(const std::string& luaSourceText,
                                              const LuaTableEvaluateLimits& limits = {});

} // namespace Sys
} // namespace SanmapGen

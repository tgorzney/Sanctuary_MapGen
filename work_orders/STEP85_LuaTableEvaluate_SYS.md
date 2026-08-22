# STEP85 — `LuaTableEvaluate_SYS` + `LuaTableValue_SYS`: sandboxed LuaJIT table evaluation

**Layer:** SYS. **Domain:** sandboxed execution primitive, sibling to the existing compile-only
`LuaSyntaxCheck_SYS` (STEP65, shipped). **Sequence:** `.santp`/`.sanprop` template ingestion track,
`work_orders/DESIGN_SantpFootprintIngestion_R1.md` §7, ticket 1 of 8 (85–92). **Real dependency:**
STEP65 (`src/sys/LuaSyntaxCheck_SYS.h/.cpp`, real, shipped — confirmed by reading it this session) for
the vendored LuaJIT library and its CMake build wiring only; no dependency on any of ticket 86–92.
Nothing in this ticket depends on the ARCH_18 rulings' *content* beyond §18.1 (the sandbox contract) —
§18.2/§18.3 govern later tickets, not this one.

## Root problem
`ARCH_18_01_SandboxedExecutionPrimitive.md` §18.1 (ARCH ruling, responding to
`DESIGN_SantpFootprintIngestion_R1.md` §2) approves a **new** sandboxed-execution SYS primitive,
explicitly forbidding it be built by widening `LuaSyntaxCheck_SYS` — that primitive's entire contract
is compile-only/never-execute, proven by its own killer test
(`CheckLuaSyntax("while true do end")` returns in bounded time *because* nothing is ever run,
`src/sys/LuaSyntaxCheck_SYS_Test.cpp`). A `.santp`/`.sanprop` file is a Lua **assignment statement**
(e.g. `UnitTemplate = { footprint = {x=1.2,y=1.2}, ... }`), not a data literal —
`luaL_loadbuffer`/`luaL_loadstring` alone yields a compiled function and zero values; obtaining
`footprint.x` requires **executing** the chunk and reading back whatever global it set.

Q1 (does `ARCH_15_03_ExportOnlyLuaRatified.md`'s "SanGen never parses Lua back" forbid this?) is
RESOLVED: `ARCH_18_SantpFootprintIngestion.md` records that §15.3 now carries a clarifying sentence
confirming its rule is scoped to the Map Scenario system's own authored content, not game-shipped
template data read in the opposite direction. Not re-litigated here.

## Deliberate design decision: this primitive is domain-free
`LuaTableEvaluate_SYS` has **zero knowledge** of `UnitTemplate`/`propTemplate`/`tpId`/`footprint` —
it executes the chunk and returns **every global the chunk set**, verbatim, as an owned tree.
Recognizing a root-table name and extracting `footprint`/`tpId`/`tags` is `TemplateDialect_IO`'s job
(ticket 87, IO layer) — SYS must not carry format/domain knowledge
(`ARCH_02_LayerDirectoryMap.md` layering; SYS is a runtime-primitive/library-integration layer, the
same footing as `ThreadPool_SYS`/`GpuResource_SYS`/`LuaSyntaxCheck_SYS` itself). This is why the API
below takes no "expected root table name" parameter.

**⚠️ Correction 2026-08-22 — this is a deliberate deviation from the design doc, not a gap it left
open.** `DESIGN_SantpFootprintIngestion_R1.md:315` explicitly specifies a 3-argument signature,
`EvaluateLuaTableSource(sourceText, rootTableName, limits)` — the design doc is NOT silent on this,
and an earlier draft of this ticket's text incorrectly claimed it was. This ticket's own resolution
overrides that 3-argument sketch on purpose: passing `rootTableName` in would let SYS branch on a
format-domain string, which is exactly the "no format-domain knowledge crosses the layer" line this
ticket draws (consistent with `ARCH_18_01`'s "no LuaJIT type... crosses the header" discipline,
extended one step further). State this to the Coder as an intentional override of the design doc's
own sketch, not as filling a silence — the design doc reviewer should be aware the signature changed.

## Fix

### 1. New file: `src/sys/LuaTableValue_SYS.h`
Header-only, no LuaJIT type — the owned plain-C++ value tree `ARCH_18_01` names.
```cpp
// LuaTableValue_SYS.h — an owned plain-C++ value tree the sandboxed Lua evaluator
// (LuaTableEvaluate_SYS) returns. No LuaJIT type crosses this header
// (ARCH_18_01_SandboxedExecutionPrimitive.md's own "no LuaJIT type ... outlives the call or crosses
// the header" rule, mirroring how SanpackReader_IO.h hides miniz). Layer: SYS.
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace SanmapGen {
namespace Sys {

enum class LuaTableValueKind { Nil, Boolean, Number, Text, Array, Table };

// A single Lua value, recursively. Array is a Lua sequence (1..n integer keys, stored 0-based
// here); Table is every other (string-keyed) field, stored as an ORDERED vector of pairs — never
// std::map, so two evaluations of the SAME source text produce IDENTICAL iteration order (a
// content-hash-based cache, ticket 88, depends on this being stable, not re-sorted by key).
struct LuaTableValue {
    LuaTableValueKind kind = LuaTableValueKind::Nil;
    bool        boolean = false;
    double      number  = 0.0;
    std::string text;
    std::vector<LuaTableValue> array;
    std::vector<std::pair<std::string, LuaTableValue>> table;

    bool IsNil() const { return kind == LuaTableValueKind::Nil; }

    // Table-kind lookup by string key. Returns nullptr on a miss OR when this value is not
    // Table-kind — never asserts; this tree is walked over untrusted, pre-alpha shipped content
    // (Constitution §6).
    const LuaTableValue* Find(const std::string& key) const {
        if (kind != LuaTableValueKind::Table) return nullptr;
        for (const auto& entry : table) if (entry.first == key) return &entry.second;
        return nullptr;
    }

    // Scalar convenience accessors — return the caller-supplied default on any kind mismatch,
    // never throw (a malformed/misspelled field in a shipped .santp is documented reality —
    // UNIT_PROP_MARKER_DATA_SPEC.md's own maxVerrtices/positonOffset misspellings — not exceptional).
    double AsNumber(double defaultValue) const {
        return kind == LuaTableValueKind::Number ? number : defaultValue;
    }
    const std::string& AsText(const std::string& defaultValue) const {
        return kind == LuaTableValueKind::Text ? text : defaultValue;
    }
};

} // namespace Sys
} // namespace SanmapGen
```

### 2. New file: `src/sys/LuaTableEvaluate_SYS.h`
```cpp
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
```

### 3. New file: `src/sys/LuaTableEvaluate_SYS.cpp`
The second (and last) translation unit in `src/` permitted to include LuaJIT headers directly
(`ARCH_18_01` §18.1: "the only other translation unit besides `LuaSyntaxCheck_SYS.cpp`"). Implementation
contract (prose, per Constitution — the Coder writes the body, same convention STEP65 used):

1. **Byte-size cap first, before touching Lua at all.** `luaSourceText.size() > limits.maximumSourceByteSize`
   → immediate failure, `errorMessage = "source exceeds the <N>-byte cap"`, zero Lua state created.
2. `luaL_newstate()` — **never `luaL_openlibs()`**. Categorically no LuaJIT `ffi`.
3. Install the instruction-count hook **before** loading/running the chunk:
   `lua_sethook(luaState, InstructionCountHookFunction, LUA_MASKCOUNT, someGranularity)`, where the
   hook (a `static` file-local function) tracks a running total against `limits.maximumInstructionCount`
   via a counter stashed in the Lua registry (`lua_pushlightuserdata`/`lua_rawseti(L, LUA_REGISTRYINDEX, ...)`
   or an equivalent per-state side channel — never a global/static C++ variable, which would leak
   across the "fresh state per call" boundary under concurrent `ThreadPool` fan-out). On exceeding
   budget: `lua_pushstring(L, "instruction budget exceeded")` + `lua_error(L)` — this is Lua's own
   internal longjmp mechanism, caught cleanly by the surrounding `lua_pcall` below (not a longjmp
   through C++ frames; `ARCH_18_01`'s constraint 4 is about the *caller's* API surface, not Lua's own
   internal error propagation, which is how the safety net inherently works).
4. `luaL_loadbuffer(luaState, luaSourceText.data(), luaSourceText.size(), "=TemplateIngest")` → on a
   load failure, populate `errorMessage` from `lua_tostring`, `lua_close`, return failure.
5. **`lua_pcall(luaState, 0, 0, 0)` — never `lua_call`.** A runtime error (including the instruction
   hook firing) is a returned `LUA_ERRRUN`/`LUA_ERRMEM`, never an escape through C++ frames.
6. On success: enumerate the global environment WITHOUT ever referencing the `_G` name (which is
   never defined — zero libraries opened) — iterate directly over the C API's globals table
   (`LUA_GLOBALSINDEX` in Lua 5.1/LuaJIT) via `lua_pushnil` + `lua_next` in a loop, converting each
   `(key, value)` pair recursively into `Sys::LuaTableValue` nodes (numbers/strings/booleans/nested
   tables/arrays map directly; a function/userdata/thread value — never expected in this corpus per
   the design doc's own empirical scan, but handled per Constitution §6 anyway — is SKIPPED for that
   one key, not treated as a fatal error). Increment a node counter on every visited node; abort with
   failure the instant `limits.maximumTableNodeCount` is exceeded.
7. `lua_close(luaState)` on **every** exit path, including every early-return/error branch above — no
   leaked Lua state (identical discipline to `LuaSyntaxCheck_SYS.cpp`'s own).

### 4. CMake — extend STEP65's existing block, do not re-derive it
`CMakeLists.txt` already vendors genuine LuaJIT (`FetchContent_Declare(luajit, ...)`, a custom
`SanGenLuaJitBuild` target driving LuaJIT's own `msvcbuild.bat`, `add_library(luajit STATIC IMPORTED
GLOBAL)`, `target_link_libraries(SanGenV2 PRIVATE luajit)`) inside an `if(WIN32)` block — confirmed
real and already working, `CMakeLists.txt:219-303`. Two edits only, both inside that same block:
```cmake
    # LuaSyntaxCheck_SYS.cpp AND LuaTableEvaluate_SYS.cpp are the ONLY two translation units in src/
    # allowed to include LuaJIT headers (ARCH_18_01_SandboxedExecutionPrimitive.md §18.1) — the
    # include path is scoped to exactly these two source files via CMake's per-source-file
    # INCLUDE_DIRECTORIES property, not the whole target.
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/sys/LuaSyntaxCheck_SYS.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/sys/LuaTableEvaluate_SYS.cpp"
        PROPERTIES INCLUDE_DIRECTORIES "${luajit_SOURCE_DIR}/src")
```
(This replaces the existing single-file `set_source_files_properties` call — the old comment claiming
"the ONLY translation unit" must be corrected to "the only TWO translation units," verbatim above.)
```cmake
add_sangen_test(LuaTableEvaluate_SYS_Test src/sys/LuaTableEvaluate_SYS_Test.cpp)
```
placed near the existing `add_sangen_test(LuaSyntaxCheck_SYS_Test ...)` line
(`CMakeLists.txt:398`). `SANGEN_V2_SOURCES` is a `file(GLOB_RECURSE ... CONFIGURE_DEPENDS "src/*.cpp"
"src/*.h" ...)` (confirmed, `CMakeLists.txt:157`) — the two new files need no separate registration
into the library target itself, only the two edits above.

## Files touched
- NEW `src/sys/LuaTableValue_SYS.h`
- NEW `src/sys/LuaTableEvaluate_SYS.h`
- NEW `src/sys/LuaTableEvaluate_SYS.cpp`
- NEW `src/sys/LuaTableEvaluate_SYS_Test.cpp`
- `CMakeLists.txt` — extend the `set_source_files_properties` call to name both `.cpp` files; one new
  `add_sangen_test` line.

## Backend policy
CPU only. Called once per template file from a future `ThreadPool` fan-out (ticket 89) — interactive/
batch, not a per-frame hot path. Same library-integration category as `LuaSyntaxCheck_SYS`/
`ThreadPool_SYS`/`GpuResource_SYS`; no `Dispatch_SYS` involvement.

## ARCH rules invoked
- `ARCH_18_01_SandboxedExecutionPrimitive.md` §18.1 — the full ruling this ticket implements: the new
  primitive is approved, the safety contract (6 points) is binding verbatim, and the standing
  constraint that this NEVER relaxes `LuaSyntaxCheck_SYS`'s own contract.
- `ARCH_18_SantpFootprintIngestion.md` — Q1's resolution (§15.3 is scenario-scoped, not a blanket
  prohibition on reading game-template Lua).
- `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 — the LuaJIT-specifically (not vanilla Lua)
  dialect ruling and the `IO → SYS` legal-dependency correction this ticket's downstream consumers
  (ticket 89, IO) rely on; this ticket itself has no IO dependency.
- Constitution §6 — validate all input, applied to actually executing untrusted text: every one of
  the six sandbox constraints is the concrete mechanism, not a restatement.
- `ARCH_02_LayerDirectoryMap.md` — SYS home, no format-domain knowledge crosses into this layer
  (this ticket's own "Deliberate design decision" section).

## Explicit out-of-scope
- **Any format-domain knowledge** (root-table names, `footprint`/`tpId`/`tags` extraction) —
  `TemplateDialect_IO`, ticket 87.
- **Widening `LuaSyntaxCheck_SYS`** in any form — permanently forbidden by `ARCH_18_01`.
- **File discovery / directory walking** — `TemplateSourceScan_IO`, ticket 86.
- **Caching evaluated results** — `TemplateIngestCache_IO`, ticket 88.
- **Vanilla/stock Lua**, or a second LuaJIT vendoring — rejected; this ticket shares STEP65's existing
  vendored copy exactly.

## Acceptance test
New `src/sys/LuaTableEvaluate_SYS_Test.cpp` (registered in `CMakeLists.txt`):
- **The instruction-budget property, the killer test.** A source containing `X = 1; while true do end`
  returns `bSucceeded == false` and returns within a bounded wall-clock budget (e.g. under one second,
  measured in the test) — proof the instruction-count hook actually aborts execution rather than
  hanging, symmetric to `LuaSyntaxCheck_SYS_Test`'s own never-execute killer test but now proving
  *bounded* execution instead of *no* execution.
- **The zero-libraries property.** A source like `X = { a = os.time() }` fails at runtime
  (`bSucceeded == false`, `os` is nil — attempting to index/call it errors, caught by `lua_pcall`) —
  proof no standard library was ever opened.
- **A real literal-table shape evaluates correctly**: a source like
  `UnitTemplate = { footprint = {x=1.2, y=1.2}, general = {tpId = "uca1001"} }` yields
  `bSucceeded == true`, `result.globals.Find("UnitTemplate")->Find("footprint")->Find("x")->AsNumber(0.0) == 1.2`,
  and the nested `general.tpId` text resolves via the same `Find` chain.
- **An empty chunk / a chunk that sets no globals** succeeds with `result.globals.table.empty() == true`
  — proves "sets zero globals" is not itself an error.
- **A syntax error** (unterminated `function`, or any malformed literal) returns `bSucceeded == false`
  with a non-empty `errorMessage`, no crash.
- **The byte-size cap** — a source text larger than `limits.maximumSourceByteSize` fails immediately
  with a cap-specific message, without a `lua_State` ever being created (verifiable by the bounded,
  effectively-zero wall-clock time of the call).
- **The node-count cap** — a deeply/widely nested literal table exceeding `limits.maximumTableNodeCount`
  fails cleanly (`bSucceeded == false`), not a crash or unbounded memory growth.
- **Fresh state per call, no cross-call leakage** — evaluating `X = 1` then evaluating a second,
  unrelated source that never sets `X` confirms the second result's `globals` contains no trace of
  the first call's `X` — proves no shared/static Lua state survives between calls.
- Full solo rebuild + `ctest -C Debug`: previously-passing suite (including
  `LuaSyntaxCheck_SYS_Test`, unedited) stays green; the new target passes.

## Verify
- New `src/sys/LuaTableEvaluate_SYS_Test.cpp` passes, especially the instruction-budget and
  zero-libraries proofs.
- Grep `src/` for `lua_call\b` inside `LuaTableEvaluate_SYS.cpp` — must have zero matches;
  `lua_pcall` only.
- Grep `src/` for `luaL_openlibs` inside `LuaTableEvaluate_SYS.cpp` — must have zero matches.
- Confirm `LuaSyntaxCheck_SYS.cpp`/`LuaSyntaxCheck_SYS_Test.cpp` are byte-for-byte unedited by this
  ticket (its own never-execute contract must not be touched).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.

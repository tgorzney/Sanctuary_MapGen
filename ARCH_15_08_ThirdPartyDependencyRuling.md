[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.8. **Only the ARCH Expert writes this file.**

### 15.8 Third-party dependency ruling — ImGuiColorTextEdit + embedded LuaJIT

Both approved by the human (item 5 of this consult). Ratified as conforming, subject to the
binding constraints below.

- **Vendoring conforms to existing practice — no new dependency-addition law needed.** SanGen
  already pulls comparably-sized third-party libraries (`imgui`, `glfw`, `nlohmann_json`,
  `miniz`, `stb`) via CMake `FetchContent` (`CMakeLists.txt`), and smaller header-only vendored
  code lives under `src/third_party/`, exempt from the naming law (§7.3). ImGuiColorTextEdit and
  the embedded Lua library are added the same way — `FetchContent` for the full library, or
  `src/third_party/` if a single-header build is used; the coder's call, per §7.3's existing
  precedent.
- **Dialect ruling: embed LuaJIT itself, not a vanilla/stock Lua build.** The engine's own script
  tree is rooted at `LJ/lua/` — "LJ" is LuaJIT — so the runtime this validator must agree with is
  LuaJIT's own grammar (closer to Lua 5.1 than to Lua 5.2+; e.g. no 5.2+ `goto`/integer-division
  semantics baked in). Validating edited runtime text against a differently-dialected parser
  risks both false-accepts and false-rejects relative to what the real game engine will actually
  load. This is a correctness finding made during this ratification, not a restatement of
  anything the setup consult specified — flagged accordingly.
- **Home: `SYS`, not `UI` and not `IO` alone.** New file `src/sys/LuaSyntaxCheck_SYS.h/.cpp` — a
  runtime-primitive/library-integration concern, the same kind of thing
  `GpuResource_SYS`/`ThreadPool_SYS` already are. It must be reachable from **both** call sites
  this feature needs: `UI` (live red-squiggle feedback while a designer types in the
  `LuaCodeEditor_UI` editor widget) and `IO` (a compile-check gate on the override-path runtime
  file at export time, independent of whether a UI editing session ever touched it).
- **§3.1 correction (footnote ¹ above): `IO`'s allowed-dependency row gains `SYS`.** This is not
  a new architectural liberty invented for this feature — it **formalizes an existing, real-code
  precedent** found while grounding this ruling: `src/io/AssetAtlasCache_IO.cpp` already
  `#include`s `../sys/ThreadPool_SYS.h`, meaning `IO → SYS` was already true in the shipped tree,
  undeclared in §3.1's table until now.
- **Hard, binding safety constraints on `LuaSyntaxCheck_SYS`** (Constitution §6, "validate all
  input," applied to embedding a language runtime specifically for untrusted-text validation):
  1. The embedded Lua state is used **exclusively** for compilation
     (`luaL_loadstring`/`load()`-equivalent). **`lua_pcall`/`lua_call`, or any other execution
     entry point, on the loaded chunk is forbidden** — this feature's entire purpose is
     syntax-checking text a human is actively editing, and it must never run.
  2. **Zero standard libraries opened** on the validation Lua state (`luaL_openlibs` is never
     called, including — especially — LuaJIT's native `ffi` library). Compilation needs no
     library table at all; opening any is unnecessary attack surface for a feature that never
     executes what it loads.
  3. Diagnostics are extracted from the compiler's own error string (Lua's standard
     `chunk:line: message` compile-error format) — no separate line-tracking reimplementation.
- **`ImGuiColorTextEdit` home: `UI`, a new `UI_FRAMEWORK_SPEC` universal widget.** New file
  `src/ui/LuaCodeEditor_UI.h/.cpp`, wrapping the vendored `TextEditor` type, calling
  `LuaSyntaxCheck_SYS` to drive its error-marker gutter. **It is a single-instance editor widget,
  not a high-cardinality list or picker** — the imgui-bypass/virtualization requirements
  `UI_FRAMEWORK_SPEC` states for 100k-entity widgets do not apply to it; ordinary per-frame ImGui
  usage is correct and expected here.
- **Shape only, not wiring.** The `LuaCodeEditor_UI` widget's full design (layout, where it is
  hosted in the tab structure, the settings-level override-path field item 4 of this consult
  names) is the parallel UI consult's call, not designed here.


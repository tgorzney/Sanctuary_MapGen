# STEP63 — `LuaTableWriter_IO`: header-only pure Lua-literal rendering primitives

**Layer:** IO. **Domain:** generic Lua-text-rendering toolkit (not a `.sanmap` domain — see naming
note below). **Sequence:** Map Scenario IO track, `work_orders/DESIGN_MapScenarioIO_R1.md` §6,
Work-Order 2 of 8. No dependency on any other undone work-order; parallel with STEP64/STEP65.
Consumed later by `ScenarioScript_DataLua_IO` (WO5, `Params::Scenarios` not yet ratified into
`src/params/` — deliberately not built here).

## Root problem
`ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 ratifies export-only Lua rendering: SanGen owns `Params::Scenarios` and renders
`<MapName>_Scenarios_Data.lua` text on export, never parsing Lua back. `DESIGN_MapScenarioIO_R1.md`
§1 names the tool this needs: "the Lua-literal twin of `JsonPrimitives_IO.h`... deliberately never
merged into it." Lua table-constructor syntax, string escaping, and numeric-literal formatting are
a different grammar from JSON — `JsonPrimitives_IO.h`'s primitives operate on `nlohmann::json`
values and do not apply. No Lua-text-rendering code exists anywhere in `src/` today.

**This ticket's scope, precisely:** the generic, domain-free rendering toolkit only — escaping, a
table-constructor open/close pair, key=value-line emission, numeric/boolean literal formatting, and
array-of-tables emission. It does **not** render anything `Params::Scenarios`-shaped (no
`ScenarioComparator`/`ScenarioCountField` enum-to-Lua mapping, no `ScenarioBody` field layout) — that
is domain-specific and belongs to `ScenarioScript_DataLua_IO` (WO5), which composes these primitives
the same way every `<Domain>_Migrate_V<N>_IO` composes `JsonPrimitives_IO`.

## Fix

### 1. New file: `src/io/LuaTableWriter_IO.h`
Header-only, `inline` free functions, no `.cpp` — same precedent `JsonPrimitives_IO.h` already
sets for a domain-free, pure, header-only toolkit. Text-builder style: every function either
returns a `std::string` fragment or appends onto a caller-owned `std::string& out` — there is no
intermediate DOM (unlike `nlohmann::json`; Lua table-literal text is rendered directly, mirroring
how `MapExporter_IO::BuildSanmapJsonText` builds its own document text as a pure function).

```cpp
// LuaTableWriter_IO.h — the Lua-literal twin of JsonPrimitives_IO.h, deliberately never merged
// into it (`ARCH_15_MapScenarioSystem.md` §15.3/§15.4, DESIGN_MapScenarioIO_R1.md §1). Header-only, `inline`, pure text
// rendering: every function is total (never throws) and free of side effects beyond appending to
// the caller-owned `out` string. Renders GENERIC Lua table-constructor syntax only — it carries no
// knowledge of Params::Scenarios or any other domain shape; a domain renderer (e.g. the future
// ScenarioScript_DataLua_IO) composes these the same way a <Domain>_Migrate_V<N>_IO composes
// JsonPrimitives_IO's transform primitives.
#pragma once
#include <charconv>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// Escapes backslash, double-quote, newline, carriage-return, and tab. Returns the escaped BODY
// only, no surrounding quotes. A SINGLE left-to-right character scan (never a sequence of global
// string::replace passes) -- doing backslash-escaping and quote-escaping as two separate passes
// double-escapes the backslashes a prior pass just inserted, corrupting adversarial input that
// itself contains a literal backslash (Constitution §6, "validate all input").
inline std::string EscapeLuaStringBody(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char character : text) {
        switch (character) {
            case '\\': escaped += "\\\\"; break;
            case '"':  escaped += "\\\""; break;
            case '\n': escaped += "\\n";  break;
            case '\r': escaped += "\\r";  break;
            case '\t': escaped += "\\t";  break;
            default:   escaped += character; break;
        }
    }
    return escaped;
}

// EscapeLuaStringBody wrapped in double quotes -- a complete Lua string literal.
inline std::string QuotedLuaString(const std::string& text) {
    return "\"" + EscapeLuaStringBody(text) + "\"";
}

inline std::string RenderLuaBoolean(bool value) { return value ? "true" : "false"; }

inline std::string RenderLuaNumber(int value) { return std::to_string(value); }

// Shortest round-trip decimal text, chars_format::fixed -- NEVER scientific notation (the world-
// space coordinates and counts this toolkit renders never need it, and an "e" exponent is not
// guaranteed to parse identically across Lua 5.1-family dialects). A whole-number value renders
// with NO decimal point ("90", not "90.0") -- matches the live reference's own literal style
// (MAP_SCENARIO_SPEC.md §5.1: NAVAL_SIDE_BIAS_DISTANCE = 90). std::to_chars still computes the
// shortest digit sequence that round-trips to this exact float even in fixed format.
inline std::string RenderLuaNumber(float value) {
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed);
    return std::string(buffer, result.ptr);
}

// `indentLevel` * 4 spaces -- the ONE indent primitive every Lua-writer in the IO layer agrees on.
inline std::string LuaIndent(int indentLevel) { return std::string(static_cast<std::size_t>(indentLevel) * 4, ' '); }

// Appends one `<indent>key = <renderedValue>,\n` line. `renderedValue` is ALREADY a complete Lua
// literal/table-text fragment -- callers compose it from RenderLuaNumber/QuotedLuaString/
// RenderLuaBoolean/a nested table's own already-rendered text; this function never re-derives it.
inline void AppendKeyValueLine(std::string& out, int indentLevel, const std::string& key,
                                const std::string& renderedValue) {
    out += LuaIndent(indentLevel) + key + " = " + renderedValue + ",\n";
}

// Opens `<indent>key = {\n` (or bare `<indent>{\n` when key is empty, for a top-level/anonymous
// table) at indentLevel.
inline void OpenTable(std::string& out, int indentLevel, const std::string& key) {
    out += LuaIndent(indentLevel);
    if (!key.empty()) out += key + " = ";
    out += "{\n";
}

// Closes a table opened by OpenTable at the SAME indentLevel. bTrailingComma controls whether the
// closing brace is followed by a comma (true -- this table is itself one entry of an outer table
// or array) or not (false -- this is the outermost/final table).
inline void CloseTable(std::string& out, int indentLevel, bool bTrailingComma) {
    out += LuaIndent(indentLevel) + (bTrailingComma ? "},\n" : "}");
}

// Renders `<indent>key = {\n<indent>    { <row> },\n...\n<indent>},\n` -- ONE `{ <row> },` line
// per already-rendered row body (e.g. `armyName = "ARMY_1", positionX = 10, positionY = 0`, built
// by the caller joining AppendKeyValueLine-style fragments or a hand-joined comma list). Trailing
// commas after every row -- including the last -- are DELIBERATE and legal Lua syntax: every row's
// emission is then identical regardless of position, with no special-cased "last element" branch.
// An EMPTY `renderedRowBodies` still opens and closes the table (a real, valid empty Lua table --
// e.g. a scenario with zero alloys renders `alloys = {},`, never an omitted key).
inline void AppendArrayOfTables(std::string& out, int indentLevel, const std::string& key,
                                 const std::vector<std::string>& renderedRowBodies) {
    OpenTable(out, indentLevel, key);
    const std::string rowIndent = LuaIndent(indentLevel + 1);
    for (const std::string& rowBody : renderedRowBodies) {
        out += rowIndent + "{ " + rowBody + " },\n";
    }
    CloseTable(out, indentLevel, true);
}

} // namespace Io
} // namespace SanmapGen
```

### 2. Naming-convention note (pre-empting a misapplied IO rule)
This is **not** a `.sanmap`-domain file and composes nothing from `JsonPrimitives_IO` — the
`MapExporter_<Domain>_IO`/`MapImporter_<Domain>_IO` + `<Domain>_Migrate_V<N>_IO` convention governs
`.sanmap` JSON document sections (`IO_MIGRATION_SPEC.md` §1) and does not apply to a Lua-text
toolkit consumed by a structurally distinct export-only surface (`ARCH_15_02_IoScopeRuling.md` §15.2).

## Files touched
- NEW `src/io/LuaTableWriter_IO.h` — the code block above, verbatim.
- NEW `src/io/LuaTableWriter_IO_Test.cpp` — pure unit tests, no file IO.
- `CMakeLists.txt` — one new `add_sangen_test(LuaTableWriter_IO_Test src/io/LuaTableWriter_IO_Test.cpp)`
  line near the other pure IO-primitive tests (e.g. `FileDialog_IO_Test`, line ~468). **No
  `nlohmann_json::nlohmann_json` link needed** — this toolkit touches no JSON type, unlike
  `JsonPrimitives_IO_Test`/`MapExporter_IO_Test`.

## Backend policy
N/A — pure CPU-side string building, called at most once per map export (not per-frame). No
compute dispatch, no SIMD, no GPU handle; does not touch `Dispatch_SYS`.

## ARCH rules invoked
- `ARCH_15_MapScenarioSystem.md` §15.3/§15.4 — export-only Lua rendering; this is the toolkit that render step composes.
- `DESIGN_MapScenarioIO_R1.md` §1 — `LuaTableWriter_IO` named explicitly as the Lua-literal twin of
  `JsonPrimitives_IO.h`, never merged into it.
- Constitution §6 — "validate all input": the single-pass escape scan is the concrete mechanism
  that keeps an adversarial/hand-typed string (one containing a literal backslash or embedded
  quote) from producing corrupt or re-interpretable Lua text.
- §1.5 size ceilings — single header, one tightly-coupled cluster of pure functions, well under the
  soft ceiling.

## Explicit out-of-scope
- **Any `Params::Scenarios`-shaped rendering** — `ScenarioComparator`/`ScenarioCountField` enum text,
  `ScenarioBody` field layout, the `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO` table
  names, the `alloyMode` branch selection. `Params::Scenarios` does not exist in `src/params/` yet
  (WO1, not this ticket) — a coder must not invent it here to "finish the job." That composition
  lands in `ScenarioScript_DataLua_IO` (WO5).
- **`ScenarioScript_DataLua_IO`, `ScenarioScript_RuntimeResource_IO`, `ScenarioScript_Export_IO`,
  `GameInstallLocation_IO`** — separate work-orders (WO5–WO7, STEP64).
- **A Lua parser / round-trip read path** — `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 rules this out entirely; this ticket only
  ever emits text, never reads it back.

## Acceptance test
New `src/io/LuaTableWriter_IO_Test.cpp` (registered in `CMakeLists.txt`):
- `EscapeLuaStringBody("back\\slash")` doubles the single backslash: `"back\\\\slash"`.
- `EscapeLuaStringBody` on a string containing a real embedded newline/tab/carriage-return character
  produces the two-character escape sequences `\n`/`\t`/`\r`, not the raw control character.
- `EscapeLuaStringBody` run TWICE on adversarial input containing both a literal backslash and a
  literal double-quote produces the mathematically correct single-pass result (proves the scan is
  one pass, not two sequential `string::replace` calls that would double-escape).
- `QuotedLuaString("Alice's \"Base\"")` == `"\"Alice's \\\"Base\\\"\""` (apostrophe untouched,
  double-quote escaped, whole literal wrapped in quotes).
- `RenderLuaNumber(0)` == `"0"`; `RenderLuaNumber(-5)` == `"-5"`.
- `RenderLuaNumber(90.0f)` == `"90"` (no decimal point on a whole-number float); `RenderLuaNumber(1.2f)`
  == `"1.2"`; `RenderLuaNumber(18.4f)` == `"18.4"`; `RenderLuaNumber(-3.5f)` == `"-3.5"`;
  `RenderLuaNumber(0.0f)` == `"0"`. None contain `"e"`/`"E"` (no scientific notation).
- `LuaIndent(0)` == `""`; `LuaIndent(2)` == 8 spaces.
- `AppendKeyValueLine` at indentLevel 1 with key `"name"` and value `QuotedLuaString("Foo")` appends
  exactly `"    name = \"Foo\",\n"`.
- `OpenTable`/`CloseTable` round trip: `OpenTable(out, 1, "spawns")` then `CloseTable(out, 1, true)`
  produces `"    spawns = {\n    },\n"`; `OpenTable(out, 0, "")` then `CloseTable(out, 0, false)`
  produces `"{\n}"`.
- `AppendArrayOfTables` with one row body `armyName = "ARMY_1", positionX = 10` at indentLevel 1
  under key `"spawns"` produces:
  `"    spawns = {\n        { armyName = \"ARMY_1\", positionX = 10 },\n    },\n"`.
- `AppendArrayOfTables` with an EMPTY row list still emits `"    alloys = {\n    },\n"` — proves the
  empty-table case is never silently skipped.
- Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; the new
  `LuaTableWriter_IO_Test` target passes.

## Verify
- New `src/io/LuaTableWriter_IO_Test.cpp` passes (assertions above).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files edited or broken.

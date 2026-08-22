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

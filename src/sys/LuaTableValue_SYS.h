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

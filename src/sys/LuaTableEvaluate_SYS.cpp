// LuaTableEvaluate_SYS.cpp — wraps LuaJIT's C API directly. This is the SECOND (and last)
// translation unit in src/ allowed to include LuaJIT headers, alongside LuaSyntaxCheck_SYS.cpp
// (ARCH_18_01_SandboxedExecutionPrimitive.md §18.1). LuaSyntaxCheck_SYS.cpp's own never-execute
// contract is untouched by this file -- this primitive is a materially different, stronger safety
// posture: it actually EXECUTES the chunk, inside a fresh, zero-library, instruction-budgeted
// lua_State, closed on every exit path.
//
// SECURITY PROPERTIES OF THIS ENTIRE FILE:
//   1. The byte-size cap is checked before any lua_State exists.
//   2. luaL_newstate only -- luaL_openlibs is NEVER called (no stdlibs, no `ffi`).
//   3. lua_sethook(LUA_MASKCOUNT) is installed before the chunk is loaded/run; the running count
//      lives in a per-state userdata addressed via the Lua registry, never a C++ global/static --
//      concurrent ThreadPool calls each get an isolated fresh lua_State and counter.
//   4. lua_pcall -- NEVER lua_call -- so a runtime error, including the instruction hook's own
//      lua_error, is a returned error code, never an escape through C++ frames.
//   5. The globals table is enumerated via LUA_GLOBALSINDEX (never the name "_G", which does not
//      exist -- zero libraries opened), converting recursively into Sys::LuaTableValue nodes,
//      capped by a node counter.
#include "LuaTableEvaluate_SYS.h"

#include <algorithm>
#include <new>
#include <utility>
#include <vector>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

namespace SanmapGen {
namespace Sys {

namespace {

// Hook fires every N VM instructions; the running total is compared to the caller's budget.
constexpr int kInstructionHookGranularity = 1000;

struct HookContext {
    std::int64_t instructionCount = 0;
    std::int64_t maximumInstructionCount = 0;
};

// Address-only registry key -- the key itself is never mutated; the actual counter lives inside
// per-state userdata, so this constant is safe to share across concurrent lua_State instances.
int gInstructionCountRegistryKeyAddress = 0;

void InstructionCountHookFunction(lua_State* luaState, lua_Debug* /*debugInfo*/) {
    lua_pushlightuserdata(luaState, &gInstructionCountRegistryKeyAddress);
    lua_rawget(luaState, LUA_REGISTRYINDEX);
    HookContext* context = static_cast<HookContext*>(lua_touserdata(luaState, -1));
    lua_pop(luaState, 1);
    if (context == nullptr) return;
    context->instructionCount += kInstructionHookGranularity;
    if (context->instructionCount > context->maximumInstructionCount) {
        lua_pushstring(luaState, "instruction budget exceeded");
        lua_error(luaState); // Lua's own longjmp, caught by the surrounding lua_pcall -- never a
                              // longjmp through C++ frames.
    }
}

void ConvertLuaTable(lua_State* luaState, int tableIndex, LuaTableValue& outValue,
                      std::size_t& nodeCount, std::size_t maximumNodeCount, bool& bNodeCapExceeded);

// Converts the value at valueIndex into outValue. A function/userdata/thread value is left
// Nil-kind (skipped for that one key), never treated as fatal.
void ConvertLuaValue(lua_State* luaState, int valueIndex, LuaTableValue& outValue,
                      std::size_t& nodeCount, std::size_t maximumNodeCount, bool& bNodeCapExceeded) {
    ++nodeCount;
    if (nodeCount > maximumNodeCount) { bNodeCapExceeded = true; return; }
    switch (lua_type(luaState, valueIndex)) {
        case LUA_TBOOLEAN:
            outValue.kind = LuaTableValueKind::Boolean;
            outValue.boolean = lua_toboolean(luaState, valueIndex) != 0;
            return;
        case LUA_TNUMBER:
            outValue.kind = LuaTableValueKind::Number;
            outValue.number = static_cast<double>(lua_tonumber(luaState, valueIndex));
            return;
        case LUA_TSTRING: {
            std::size_t length = 0;
            const char* data = lua_tolstring(luaState, valueIndex, &length);
            outValue.kind = LuaTableValueKind::Text;
            outValue.text.assign(data, length);
            return;
        }
        case LUA_TTABLE:
            ConvertLuaTable(luaState, valueIndex, outValue, nodeCount, maximumNodeCount,
                             bNodeCapExceeded);
            return;
        default:
            outValue.kind = LuaTableValueKind::Nil; // function/userdata/thread -- not fatal
            return;
    }
}

// True (and integerEntries sorted) only when every key is a positive integer forming a contiguous
// 1..n sequence with no string keys at all -- the only shape LuaTableValueKind::Array represents.
bool IsContiguousIntegerSequence(std::vector<std::pair<lua_Integer, LuaTableValue>>& integerEntries,
                                  bool bHasStringEntries) {
    if (integerEntries.empty() || bHasStringEntries) return false;
    std::sort(integerEntries.begin(), integerEntries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    for (std::size_t index = 0; index < integerEntries.size(); ++index) {
        if (integerEntries[index].first != static_cast<lua_Integer>(index + 1)) return false;
    }
    return true;
}

void ConvertLuaTable(lua_State* luaState, int tableIndex, LuaTableValue& outValue,
                      std::size_t& nodeCount, std::size_t maximumNodeCount, bool& bNodeCapExceeded) {
    std::vector<std::pair<lua_Integer, LuaTableValue>> integerEntries;
    std::vector<std::pair<std::string, LuaTableValue>> stringEntries;

    lua_pushnil(luaState);
    while (lua_next(luaState, tableIndex) != 0) {
        const int valueIndex = lua_gettop(luaState);
        const int keyIndex = valueIndex - 1;

        LuaTableValue convertedValue;
        ConvertLuaValue(luaState, valueIndex, convertedValue, nodeCount, maximumNodeCount,
                         bNodeCapExceeded);
        if (bNodeCapExceeded) { lua_pop(luaState, 2); break; }

        const int keyType = lua_type(luaState, keyIndex);
        if (keyType == LUA_TNUMBER) {
            const lua_Number numericKey = lua_tonumber(luaState, keyIndex);
            const lua_Integer integerKey = static_cast<lua_Integer>(numericKey);
            if (integerKey >= 1 && static_cast<lua_Number>(integerKey) == numericKey) {
                integerEntries.emplace_back(integerKey, std::move(convertedValue));
            }
        } else if (keyType == LUA_TSTRING) {
            std::size_t length = 0;
            const char* data = lua_tolstring(luaState, keyIndex, &length);
            stringEntries.emplace_back(std::string(data, length), std::move(convertedValue));
        }
        // else: a non-string, non-positive-integer key -- skipped, same posture as an unsupported
        // value kind above.
        lua_pop(luaState, 1); // pop value only; lua_next needs the key left on the stack
    }

    if (IsContiguousIntegerSequence(integerEntries, !stringEntries.empty())) {
        outValue.kind = LuaTableValueKind::Array;
        outValue.array.reserve(integerEntries.size());
        for (auto& entry : integerEntries) outValue.array.push_back(std::move(entry.second));
        return;
    }
    outValue.kind = LuaTableValueKind::Table;
    outValue.table = std::move(stringEntries);
    for (auto& entry : integerEntries) {
        outValue.table.emplace_back(std::to_string(entry.first), std::move(entry.second));
    }
}

} // namespace

LuaTableEvaluateResult EvaluateLuaTableSource(const std::string& luaSourceText,
                                              const LuaTableEvaluateLimits& limits) {
    LuaTableEvaluateResult result;

    // 1. Byte-size cap first -- before touching Lua at all.
    if (luaSourceText.size() > limits.maximumSourceByteSize) {
        result.errorMessage = "source exceeds the " + std::to_string(limits.maximumSourceByteSize) +
                               "-byte cap";
        return result;
    }

    // 2. Fresh, zero-library state -- never luaL_openlibs, categorically no ffi.
    lua_State* luaState = luaL_newstate();
    if (luaState == nullptr) {
        result.errorMessage = "LuaTableEvaluate_SYS: luaL_newstate failed (out of memory)";
        return result;
    }

    // 3. Instruction-count hook, installed before load/run. The counter lives in a full-userdata
    // block owned by this lua_State's own memory, addressed via the registry.
    HookContext* hookContext = new (lua_newuserdata(luaState, sizeof(HookContext))) HookContext();
    hookContext->maximumInstructionCount = limits.maximumInstructionCount;
    lua_pushlightuserdata(luaState, &gInstructionCountRegistryKeyAddress);
    lua_pushvalue(luaState, -2);
    lua_rawset(luaState, LUA_REGISTRYINDEX);
    lua_pop(luaState, 1);
    lua_sethook(luaState, InstructionCountHookFunction, LUA_MASKCOUNT, kInstructionHookGranularity);

    // 4-5. Compile, then execute via pcall ONLY -- never lua_call -- so any runtime error
    // (including the instruction hook firing) is a returned error code.
    if (luaL_loadbuffer(luaState, luaSourceText.data(), luaSourceText.size(), "=TemplateIngest") !=
            LUA_OK ||
        lua_pcall(luaState, 0, 0, 0) != LUA_OK) {
        const char* errorText = lua_tostring(luaState, -1);
        result.errorMessage = (errorText != nullptr) ? std::string(errorText) : std::string();
        lua_close(luaState); // 7. close on every exit path
        return result;
    }

    // 6. Enumerate globals directly via LUA_GLOBALSINDEX -- "_G" is never defined (no libs open).
    std::size_t nodeCount = 0;
    bool bNodeCapExceeded = false;
    ConvertLuaTable(luaState, LUA_GLOBALSINDEX, result.globals, nodeCount,
                     limits.maximumTableNodeCount, bNodeCapExceeded);
    lua_close(luaState); // 7. close on every exit path

    if (bNodeCapExceeded) {
        result.globals = LuaTableValue();
        result.errorMessage =
            "globals table exceeds the " + std::to_string(limits.maximumTableNodeCount) + "-node cap";
        return result;
    }

    result.bSucceeded = true;
    return result;
}

} // namespace Sys
} // namespace SanmapGen

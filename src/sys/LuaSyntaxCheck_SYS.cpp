// LuaSyntaxCheck_SYS.cpp — wraps LuaJIT's C API directly. This is the ONLY translation unit in
// src/ allowed to include LuaJIT headers (ARCH_15_08_ThirdPartyDependencyRuling.md §15.8).
//
// SECURITY PROPERTY OF THIS ENTIRE FILE: `lua_pcall`/`lua_call` -- or any other execution entry
// point -- MUST NEVER appear anywhere below. The loaded chunk is compiled and immediately
// discarded (lua_pop), never run. `luaL_openlibs` is likewise never called -- the validation
// state opens with zero standard libraries, including LuaJIT's native `ffi` library.
#include "LuaSyntaxCheck_SYS.h"

#include <cstdlib>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

namespace SanmapGen {
namespace Sys {

namespace {

// Best-effort extraction of the line number out of LuaJIT's standard "chunk:LINE: message"
// compile-error prefix. A malformed/unexpected prefix still returns the raw message text with
// lineNumber left at 0 rather than crashing (Constitution §6 -- never trust the parsed shape).
int ExtractLineNumber(const std::string& errorMessage) {
    const std::size_t firstColon = errorMessage.find(':');
    if (firstColon == std::string::npos) {
        return 0;
    }
    const std::size_t secondColon = errorMessage.find(':', firstColon + 1);
    if (secondColon == std::string::npos || secondColon <= firstColon + 1) {
        return 0;
    }
    const std::string lineSlice =
        errorMessage.substr(firstColon + 1, secondColon - firstColon - 1);
    if (lineSlice.empty()) {
        return 0;
    }
    for (char lineChar : lineSlice) {
        if (lineChar < '0' || lineChar > '9') {
            return 0;
        }
    }
    return std::atoi(lineSlice.c_str());
}

} // namespace

LuaSyntaxCheckResult CheckLuaSyntax(const std::string& luaSourceText) {
    LuaSyntaxCheckResult result;

    lua_State* luaState = luaL_newstate(); // never luaL_openlibs -- see file-top contract
    if (luaState == nullptr) {
        result.bSucceeded = false;
        result.message = "LuaSyntaxCheck_SYS: luaL_newstate failed (out of memory)";
        return result;
    }

    const int loadStatus = luaL_loadbuffer(luaState, luaSourceText.data(), luaSourceText.size(),
                                            "=ScenarioScript");
    if (loadStatus == LUA_OK) {
        lua_pop(luaState, 1); // discard the compiled chunk -- it is NEVER called
        result.bSucceeded = true;
    } else {
        const char* errorText = lua_tostring(luaState, -1);
        result.message = (errorText != nullptr) ? std::string(errorText) : std::string();
        result.lineNumber = ExtractLineNumber(result.message);
        lua_pop(luaState, 1);
        result.bSucceeded = false;
    }

    lua_close(luaState); // every exit path closes the state -- no leaked Lua state
    return result;
}

} // namespace Sys
} // namespace SanmapGen

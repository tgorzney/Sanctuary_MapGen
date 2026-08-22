// LuaSyntaxCheck_SYS_Test.cpp — acceptance test for LuaSyntaxCheck_SYS (STEP65).
// The two proof-of-non-execution tests are the killer tests: if this implementation ever called
// lua_pcall/lua_call, or ever opened luaL_openlibs, one of these would fail loudly (a hang, or a
// resolved-global error) rather than silently.
#include "LuaSyntaxCheck_SYS.h"

#include <chrono>
#include <cstdio>
#include <string>

using namespace SanmapGen::Sys;

int main() {
    int failures = 0;

    // The never-execute property. A syntactically valid infinite loop must compile successfully
    // and return within a bounded wall-clock budget -- if this ever called lua_pcall/lua_call on
    // the loaded chunk, this would hang the test process forever.
    {
        const auto start = std::chrono::steady_clock::now();
        LuaSyntaxCheckResult result = CheckLuaSyntax("while true do end");
        const auto elapsed = std::chrono::steady_clock::now() - start;
        const double elapsedSeconds = std::chrono::duration<double>(elapsed).count();
        if (!result.bSucceeded) {
            std::printf("FAIL never-execute: expected bSucceeded == true, message=%s\n",
                        result.message.c_str());
            ++failures;
        }
        if (elapsedSeconds >= 1.0) {
            std::printf("FAIL never-execute: took %.3fs (>= 1s budget) -- chunk may have run\n",
                        elapsedSeconds);
            ++failures;
        }
    }

    // The zero-libraries property. A syntactically valid reference to a stdlib global that was
    // never opened still compiles -- compilation never resolves globals, only parses.
    {
        LuaSyntaxCheckResult result = CheckLuaSyntax("os.execute('nothing')");
        if (!result.bSucceeded) {
            std::printf("FAIL zero-libraries: expected bSucceeded == true, message=%s\n",
                        result.message.c_str());
            ++failures;
        }
    }

    // A genuine syntax error reports failure, a non-empty message, and the injected error's line.
    {
        LuaSyntaxCheckResult result = CheckLuaSyntax("function foo(");
        if (result.bSucceeded) {
            std::printf("FAIL syntax-error: expected bSucceeded == false\n");
            ++failures;
        }
        if (result.message.empty()) {
            std::printf("FAIL syntax-error: expected a non-empty message\n");
            ++failures;
        }
        if (result.lineNumber != 1) {
            std::printf("FAIL syntax-error: expected lineNumber == 1, got %d\n", result.lineNumber);
            ++failures;
        }
    }

    // Empty and whitespace-only chunks are valid Lua -- never treated as an error.
    {
        LuaSyntaxCheckResult result = CheckLuaSyntax("");
        if (!result.bSucceeded) {
            std::printf("FAIL empty chunk: expected bSucceeded == true\n");
            ++failures;
        }
    }
    {
        LuaSyntaxCheckResult result = CheckLuaSyntax("   \n\t  \n");
        if (!result.bSucceeded) {
            std::printf("FAIL whitespace-only chunk: expected bSucceeded == true\n");
            ++failures;
        }
    }

    // A multi-line script with the error on line 3 -- proves line extraction is real, not
    // hardcoded to the first line.
    {
        const std::string script =
            "local a = 1\n"
            "local b = 2\n"
            "function foo(";
        LuaSyntaxCheckResult result = CheckLuaSyntax(script);
        if (result.bSucceeded) {
            std::printf("FAIL multi-line: expected bSucceeded == false\n");
            ++failures;
        }
        if (result.lineNumber != 3) {
            std::printf("FAIL multi-line: expected lineNumber == 3, got %d\n", result.lineNumber);
            ++failures;
        }
    }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}

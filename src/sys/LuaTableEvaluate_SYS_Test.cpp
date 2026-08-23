// LuaTableEvaluate_SYS_Test.cpp — acceptance test for LuaTableEvaluate_SYS (STEP85).
// The instruction-budget and zero-libraries tests are the killer tests: if this implementation
// ever skipped the hook, opened a stdlib, or used lua_call instead of lua_pcall, one of these
// would fail loudly (a hang, a resolved global, or an uncaught C++ escape) rather than silently.
#include "LuaTableEvaluate_SYS.h"

#include <chrono>
#include <cstdio>
#include <string>

using namespace SanmapGen::Sys;

namespace {
int gFailureCount = 0;

void Check(bool bCondition, const char* scenarioName, const char* message) {
    if (bCondition) return;
    std::printf("FAIL %s: %s\n", scenarioName, message);
    ++gFailureCount;
}

// Walks a two-level Table.Table.key chain, returning nullptr on any miss along the way -- keeps
// the literal-table scenario below from nesting ternaries.
const LuaTableValue* FindNested(const LuaTableValue& root, const char* a, const char* b) {
    const LuaTableValue* first = root.Find(a);
    return first ? first->Find(b) : nullptr;
}
} // namespace

int main() {
    // The instruction-budget property, the killer test: an infinite loop must be aborted by the
    // hook, returning failure within a bounded wall-clock budget -- proof the hook actually stops
    // execution rather than hanging.
    {
        const auto start = std::chrono::steady_clock::now();
        LuaTableEvaluateResult result = EvaluateLuaTableSource("X = 1; while true do end");
        const double elapsedSeconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Check(!result.bSucceeded, "instruction-budget", "expected bSucceeded == false");
        Check(elapsedSeconds < 1.0, "instruction-budget", "took >= 1s -- chunk may still be running");
    }

    // The zero-libraries property: os.time() fails at runtime because `os` was never opened.
    {
        LuaTableEvaluateResult result = EvaluateLuaTableSource("X = { a = os.time() }");
        Check(!result.bSucceeded, "zero-libraries", "expected bSucceeded == false (os should be nil)");
        Check(!result.errorMessage.empty(), "zero-libraries", "expected a non-empty errorMessage");
    }

    // A real literal-table shape evaluates correctly, including nested Find() chains.
    {
        LuaTableEvaluateResult result = EvaluateLuaTableSource(
            "UnitTemplate = { footprint = {x=1.2, y=1.2}, general = {tpId = \"uca1001\"} }");
        Check(result.bSucceeded, "literal-table", result.errorMessage.c_str());
        const LuaTableValue* unitTemplate = result.globals.Find("UnitTemplate");
        const LuaTableValue* footprintX =
            unitTemplate ? FindNested(*unitTemplate, "footprint", "x") : nullptr;
        const LuaTableValue* tpId = unitTemplate ? FindNested(*unitTemplate, "general", "tpId") : nullptr;
        Check(footprintX && footprintX->AsNumber(-1.0) == 1.2, "literal-table",
              "UnitTemplate.footprint.x != 1.2");
        Check(tpId && tpId->AsText("") == "uca1001", "literal-table",
              "UnitTemplate.general.tpId != \"uca1001\"");
    }

    // An empty chunk, and a chunk that only sets locals, both succeed with an empty globals table.
    {
        LuaTableEvaluateResult emptyResult = EvaluateLuaTableSource("");
        Check(emptyResult.bSucceeded && emptyResult.globals.table.empty(), "empty-chunk",
              "expected success with an empty globals table");
        LuaTableEvaluateResult localOnlyResult = EvaluateLuaTableSource("local onlyLocal = 1");
        Check(localOnlyResult.bSucceeded && localOnlyResult.globals.table.empty(), "local-only-chunk",
              "expected success with an empty globals table");
    }

    // A syntax error returns failure with a non-empty message, no crash.
    {
        LuaTableEvaluateResult result = EvaluateLuaTableSource("function foo(");
        Check(!result.bSucceeded, "syntax-error", "expected bSucceeded == false");
        Check(!result.errorMessage.empty(), "syntax-error", "expected a non-empty errorMessage");
    }

    // The byte-size cap rejects oversize source before any lua_State is created -- proven by the
    // cap-specific message and an effectively-zero wall-clock time.
    {
        LuaTableEvaluateLimits limits;
        limits.maximumSourceByteSize = 16;
        const auto start = std::chrono::steady_clock::now();
        LuaTableEvaluateResult result =
            EvaluateLuaTableSource("X = 1 -- this source text is longer than sixteen bytes", limits);
        const double elapsedSeconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Check(!result.bSucceeded, "byte-size-cap", "expected bSucceeded == false");
        Check(result.errorMessage.find("byte") != std::string::npos, "byte-size-cap",
              "expected a cap-specific message");
        Check(elapsedSeconds < 0.1, "byte-size-cap", "took too long for a pre-lua_State rejection");
    }

    // The node-count cap rejects an over-large table cleanly -- not a crash or unbounded growth.
    {
        LuaTableEvaluateLimits limits;
        limits.maximumTableNodeCount = 10;
        LuaTableEvaluateResult result =
            EvaluateLuaTableSource("X = {}\nfor i = 1, 1000 do X[i] = i end", limits);
        Check(!result.bSucceeded, "node-count-cap", "expected bSucceeded == false");
        Check(result.errorMessage.find("node") != std::string::npos, "node-count-cap",
              "expected a cap-specific message");
    }

    // Fresh state per call, no cross-call leakage.
    {
        LuaTableEvaluateResult firstResult = EvaluateLuaTableSource("X = 1");
        LuaTableEvaluateResult secondResult = EvaluateLuaTableSource("Y = 2");
        Check(firstResult.bSucceeded && secondResult.bSucceeded, "fresh-state",
              "expected both calls to succeed");
        Check(secondResult.globals.Find("X") == nullptr, "fresh-state",
              "second call's globals leaked the first call's X");
        Check(secondResult.globals.Find("Y") != nullptr, "fresh-state",
              "second call's globals is missing its own Y");
    }

    if (gFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", gFailureCount);
    return 1;
}

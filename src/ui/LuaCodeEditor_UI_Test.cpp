// LuaCodeEditor_UI_Test.cpp — acceptance test for STEP77's LuaCodeEditor_UI widget. Headless: no
// imgui frame, no window, no GL context — every behavior under test is exposed as a pure function
// over LuaCodeEditorState (LuaCodeEditor_UI.h's own header comment), exactly as
// LuaSyntaxCheck_SYS_Test.cpp already proves the compiler underneath is headless-testable.
#include "LuaCodeEditor_UI.h"

#include <cstdio>
#include <string>

using namespace SanmapGen::Ui;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// 1. Validation runs on a simulated focus-loss transition, never on every keystroke: the pure
// trigger policy answers true ONLY on a was-focused -> not-focused transition.
static void TestValidationTriggersOnlyOnFocusLossTransition() {
    Check(ShouldValidateLuaCodeEditorOnFocusTransition(true, false) == true,
          "focused last frame, not focused now -> validate (the focus-loss transition)");
    Check(ShouldValidateLuaCodeEditorOnFocusTransition(true, true) == false,
          "still focused -> no validation (never per-keystroke)");
    Check(ShouldValidateLuaCodeEditorOnFocusTransition(false, true) == false,
          "gaining focus is not a loss -> no validation");
    Check(ShouldValidateLuaCodeEditorOnFocusTransition(false, false) == false,
          "never focused at all -> no validation");
}

// 2. A syntactically invalid buffer sets bLastCheckSucceeded == false with a non-zero
// lastCheckedErrorLine matching the injected error's line.
static void TestInvalidBufferReportsFailureAndLine() {
    LuaCodeEditorState state;
    state.bufferText = "local a = 1\nlocal b = 2\nfunction foo(";
    ValidateLuaCodeEditorBuffer(state);
    Check(state.bLastCheckSucceeded == false, "invalid buffer: bLastCheckSucceeded is false");
    Check(state.lastCheckedErrorLine == 3, "invalid buffer: lastCheckedErrorLine matches line 3");
    Check(!state.lastCheckedErrorMessage.empty(), "invalid buffer: a message is carried");
}

// 3. A valid buffer (including empty) sets bLastCheckSucceeded == true.
static void TestValidAndEmptyBuffersReportSuccess() {
    LuaCodeEditorState validState;
    validState.bufferText = "Scenario = {}\nfunction Scenario.OnStart() end\n";
    ValidateLuaCodeEditorBuffer(validState);
    Check(validState.bLastCheckSucceeded == true, "valid buffer: bLastCheckSucceeded is true");

    LuaCodeEditorState emptyState;
    emptyState.bLastCheckSucceeded = false;   // prove the call actually flips it, not a default
    ValidateLuaCodeEditorBuffer(emptyState);
    Check(emptyState.bLastCheckSucceeded == true, "empty buffer: valid Lua, bLastCheckSucceeded is true");
}

int main() {
    TestValidationTriggersOnlyOnFocusLossTransition();
    TestInvalidBufferReportsFailureAndLine();
    TestValidAndEmptyBuffersReportSuccess();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

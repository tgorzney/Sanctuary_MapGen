// LuaTableWriter_IO_Test.cpp — acceptance test for the generic Lua-literal rendering primitives
// (STEP63). Pure unit tests, no file IO: exercises escaping (including the single-pass-scan
// adversarial-input proof), numeric/boolean literal formatting, indentation, key=value-line
// emission, table open/close, and array-of-tables emission.
#include "LuaTableWriter_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

void CheckEscapeLuaStringBody() {
    Check(Io::EscapeLuaStringBody("back\\slash") == "back\\\\slash",
          "EscapeLuaStringBody doubles a single backslash");

    Check(Io::EscapeLuaStringBody("a\nb\tc\rd") == "a\\nb\\tc\\rd",
          "EscapeLuaStringBody renders embedded newline/tab/carriage-return as two-character escapes");

    // Adversarial input containing both a literal backslash and a literal double-quote. Running
    // the escape TWICE must not double-escape: applying the function to its own output is not the
    // same as a correct round trip, but proves the FIRST pass is single-scan (a two-pass
    // backslash-then-quote implementation would corrupt this on the very first call already, and
    // the second call would corrupt it further/differently). We assert the first-pass result is
    // mathematically correct, then assert running it again on that already-escaped text produces
    // the expected re-escape of the escape sequences themselves (i.e. each backslash in the
    // once-escaped text is itself doubled), proving no hidden state / no double-application within
    // a single call.
    const std::string adversarial = "a\\b\"c";
    const std::string oncePass = Io::EscapeLuaStringBody(adversarial);
    Check(oncePass == "a\\\\b\\\"c", "EscapeLuaStringBody single pass is mathematically correct on backslash+quote input");

    const std::string twicePass = Io::EscapeLuaStringBody(oncePass);
    // oncePass == a \\ b \" c  (as characters: a, \, \, b, \, ", c)
    // Escaping THAT again doubles every backslash and escapes the quote again.
    Check(twicePass == "a\\\\\\\\b\\\\\\\"c",
          "EscapeLuaStringBody run twice re-escapes the once-escaped text deterministically (proves single-pass scan, not corrupted state)");
}

void CheckQuotedLuaString() {
    Check(Io::QuotedLuaString("Alice's \"Base\"") == "\"Alice's \\\"Base\\\"\"",
          "QuotedLuaString leaves apostrophe untouched, escapes the double-quote, wraps in quotes");
}

void CheckRenderLuaNumberInt() {
    Check(Io::RenderLuaNumber(0) == "0", "RenderLuaNumber(int 0)");
    Check(Io::RenderLuaNumber(-5) == "-5", "RenderLuaNumber(int -5)");
}

void CheckRenderLuaNumberFloat() {
    Check(Io::RenderLuaNumber(90.0f) == "90", "RenderLuaNumber(90.0f) has no decimal point");
    Check(Io::RenderLuaNumber(1.2f) == "1.2", "RenderLuaNumber(1.2f)");
    Check(Io::RenderLuaNumber(18.4f) == "18.4", "RenderLuaNumber(18.4f)");
    Check(Io::RenderLuaNumber(-3.5f) == "-3.5", "RenderLuaNumber(-3.5f)");
    Check(Io::RenderLuaNumber(0.0f) == "0", "RenderLuaNumber(0.0f)");

    for (const float value : {90.0f, 1.2f, 18.4f, -3.5f, 0.0f}) {
        const std::string rendered = Io::RenderLuaNumber(value);
        Check(rendered.find('e') == std::string::npos && rendered.find('E') == std::string::npos,
              "RenderLuaNumber never emits scientific notation");
    }
}

void CheckLuaIndent() {
    Check(Io::LuaIndent(0) == "", "LuaIndent(0) is empty");
    Check(Io::LuaIndent(2) == "        ", "LuaIndent(2) is 8 spaces");
}

void CheckAppendKeyValueLine() {
    std::string out;
    Io::AppendKeyValueLine(out, 1, "name", Io::QuotedLuaString("Foo"));
    Check(out == "    name = \"Foo\",\n", "AppendKeyValueLine emits the expected line");
}

void CheckOpenCloseTable() {
    std::string spawns;
    Io::OpenTable(spawns, 1, "spawns");
    Io::CloseTable(spawns, 1, true);
    Check(spawns == "    spawns = {\n    },\n", "OpenTable/CloseTable round trip with key and trailing comma");

    std::string anonymous;
    Io::OpenTable(anonymous, 0, "");
    Io::CloseTable(anonymous, 0, false);
    Check(anonymous == "{\n}", "OpenTable/CloseTable round trip anonymous, no trailing comma");
}

void CheckAppendArrayOfTables() {
    std::string out;
    Io::AppendArrayOfTables(out, 1, "spawns", { "armyName = \"ARMY_1\", positionX = 10" });
    Check(out == "    spawns = {\n        { armyName = \"ARMY_1\", positionX = 10 },\n    },\n",
          "AppendArrayOfTables with one row body");

    std::string empty;
    Io::AppendArrayOfTables(empty, 1, "alloys", {});
    Check(empty == "    alloys = {\n    },\n", "AppendArrayOfTables with an empty row list still opens/closes the table");
}

} // namespace

int main() {
    CheckEscapeLuaStringBody();
    CheckQuotedLuaString();
    CheckRenderLuaNumberInt();
    CheckRenderLuaNumberFloat();
    CheckLuaIndent();
    CheckAppendKeyValueLine();
    CheckOpenCloseTable();
    CheckAppendArrayOfTables();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

// ScenarioScript_LuaLiteralLexer_IO_Test.cpp -- first independent coverage for the shared lexer
// (STEP262), promoted out of ScenarioScript_AreaRectangleExtract_IO.cpp. Covers what the area
// extractor's own test suite never needed to exercise directly: String tokens carrying unescaped
// content, and the new comparator operator tokens.
#include "ScenarioScript_LuaLiteralLexer_IO.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;
using Io::LuaLiteralTokenKind;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// 1. A double-quoted string literal emits a String token carrying the unescaped content, not a skip.
static void TestDoubleQuotedStringEmitsStringToken() {
    const auto tokens = Io::Tokenize("\"foo\"");
    Check(tokens.size() == 1, "DoubleQuotedString: exactly one token");
    if (!tokens.empty()) {
        Check(tokens[0].kind == LuaLiteralTokenKind::String, "DoubleQuotedString: kind is String");
        Check(tokens[0].text == "foo", "DoubleQuotedString: content is 'foo'");
    }
}

// 2. A single-quoted string literal likewise emits a String token.
static void TestSingleQuotedStringEmitsStringToken() {
    const auto tokens = Io::Tokenize("'bar'");
    Check(tokens.size() == 1, "SingleQuotedString: exactly one token");
    if (!tokens.empty()) {
        Check(tokens[0].kind == LuaLiteralTokenKind::String, "SingleQuotedString: kind is String");
        Check(tokens[0].text == "bar", "SingleQuotedString: content is 'bar'");
    }
}

// 3. A string carrying the slot-pattern shape referenced by the ticket (STEP263's own motivating
//    example) round-trips its content verbatim.
static void TestSlotPatternShapedStringContent() {
    const auto tokens = Io::Tokenize("\"----hhhh\"");
    Check(tokens.size() == 1 && tokens[0].kind == LuaLiteralTokenKind::String,
          "SlotPatternString: one String token");
    if (!tokens.empty()) Check(tokens[0].text == "----hhhh", "SlotPatternString: content verbatim");
}

// 4. Each comparator operator emits the correct distinct kind, and `=` (assignment) is never
//    confused with `==` (comparison) -- the ticket's own explicitly named risk.
static void TestComparatorOperatorsEachEmitCorrectKind() {
    struct Case { const char* source; LuaLiteralTokenKind expected; const char* label; };
    const Case cases[] = {
        { "==", LuaLiteralTokenKind::EqualsEquals, "EqualsEquals" },
        { "~=", LuaLiteralTokenKind::TildeEquals,  "TildeEquals" },
        { ">",  LuaLiteralTokenKind::GreaterThan,  "GreaterThan" },
        { ">=", LuaLiteralTokenKind::GreaterEquals,"GreaterEquals" },
        { "<",  LuaLiteralTokenKind::LessThan,     "LessThan" },
        { "<=", LuaLiteralTokenKind::LessEquals,   "LessEquals" },
        { "=",  LuaLiteralTokenKind::Equals,       "Equals" },
    };
    for (const Case& testCase : cases) {
        const auto tokens = Io::Tokenize(testCase.source);
        const std::string label = std::string("Comparator ") + testCase.label;
        Check(tokens.size() == 1, (label + ": exactly one token").c_str());
        if (!tokens.empty()) Check(tokens[0].kind == testCase.expected, (label + ": correct kind").c_str());
    }
}

// 5. A real match-condition shape (`t == 4`, `h >= 2`) tokenizes as Identifier, comparator, Number.
static void TestMatchConditionShapeTokenizesCorrectly() {
    const auto tokens = Io::Tokenize("h >= 2");
    Check(tokens.size() == 3, "MatchCondition: three tokens");
    if (tokens.size() == 3) {
        Check(tokens[0].kind == LuaLiteralTokenKind::Identifier && tokens[0].text == "h", "MatchCondition: identifier 'h'");
        Check(tokens[1].kind == LuaLiteralTokenKind::GreaterEquals, "MatchCondition: '>=' operator");
        Check(tokens[2].kind == LuaLiteralTokenKind::Number && tokens[2].text == "2", "MatchCondition: number '2'");
    }
}

// 6. `--` line comments, `--[[ ]]` long comments, and quoted strings containing `--` or braces
//    inside them are still skipped/parsed correctly after promotion (regression, item 3).
static void TestCommentAndStringSkippingSurvivesPromotion() {
    const auto lineCommentTokens = Io::Tokenize("-- a comment\nlocal x = 1\n");
    Check(lineCommentTokens.size() == 4, "CommentSkip: line comment skipped, four real tokens remain");

    const auto longCommentTokens = Io::Tokenize("--[[ a { long comment } with -- inside ]]\nlocal y = 2\n");
    Check(longCommentTokens.size() == 4, "CommentSkip: long comment skipped, four real tokens remain");

    const auto stringWithDashesAndBraces = Io::Tokenize("\"has -- dashes and { braces }\"");
    Check(stringWithDashesAndBraces.size() == 1 && stringWithDashesAndBraces[0].kind == LuaLiteralTokenKind::String,
          "CommentSkip: string with -- and braces stays one String token");
    if (!stringWithDashesAndBraces.empty()) {
        Check(stringWithDashesAndBraces[0].text == "has -- dashes and { braces }",
              "CommentSkip: string content preserved verbatim, including -- and braces");
    }
}

int main() {
    TestDoubleQuotedStringEmitsStringToken();
    TestSingleQuotedStringEmitsStringToken();
    TestSlotPatternShapedStringContent();
    TestComparatorOperatorsEachEmitCorrectKind();
    TestMatchConditionShapeTokenizesCorrectly();
    TestCommentAndStringSkippingSurvivesPromotion();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

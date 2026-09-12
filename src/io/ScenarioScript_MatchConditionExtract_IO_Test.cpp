// ScenarioScript_MatchConditionExtract_IO_Test.cpp -- pure-logic acceptance test for the closed
// literal-only Shape 1 grammar (STEP264, ARCH_15_14 Part B). No filesystem, no disk, matches the
// header's own "pure and total" contract.
#include "ScenarioScript_MatchConditionExtract_IO.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool ConditionEquals(const Params::ScenarioCountCondition& actual, Params::ScenarioCountField field,
                            Params::ScenarioComparator comparator, int value, int slotRangeStart = 1, int slotRangeEnd = 1) {
    return actual.field == field && actual.comparator == comparator && actual.value == value
        && actual.slotRangeStart == slotRangeStart && actual.slotRangeEnd == slotRangeEnd;
}

// 1-6. Template A: the six real reference conjunctions, exact byte-verbatim source text from
//      map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua.officialbak (lines 149, 180, 221,
//      256, 262, 268).
static void TestTemplateA_1v1() {
    const std::string source = "match = function(t, h, a, pattern) return t == 2 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "1v1: exactly one condition set");
    Check(result.nearMisses.empty(), "1v1: no near-misses");
    if (result.conditionSets.size() == 1) {
        const auto& conditions = result.conditionSets[0].conditions;
        Check(conditions.size() == 1, "1v1: one conjunct");
        if (conditions.size() == 1)
            Check(ConditionEquals(conditions[0], Params::ScenarioCountField::Total, Params::ScenarioComparator::Equal, 2),
                  "1v1: t == 2");
    }
}

static void TestTemplateA_4human() {
    const std::string source = "match = function(t, h, a, pattern) return t == 4 and h == 4 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "4human: exactly one condition set");
    if (result.conditionSets.size() == 1) {
        const auto& conditions = result.conditionSets[0].conditions;
        Check(conditions.size() == 2, "4human: two conjuncts");
        if (conditions.size() == 2) {
            Check(ConditionEquals(conditions[0], Params::ScenarioCountField::Total, Params::ScenarioComparator::Equal, 4), "4human: t == 4");
            Check(ConditionEquals(conditions[1], Params::ScenarioCountField::HumanCount, Params::ScenarioComparator::Equal, 4), "4human: h == 4");
        }
    }
}

static void TestTemplateA_1h3ai() {
    const std::string source = "match = function(t, h, a, pattern) return t == 4 and h == 1 and a == 3 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "1h3ai: exactly one condition set");
    if (result.conditionSets.size() == 1) {
        const auto& conditions = result.conditionSets[0].conditions;
        Check(conditions.size() == 3, "1h3ai: three conjuncts");
        if (conditions.size() == 3) {
            Check(ConditionEquals(conditions[0], Params::ScenarioCountField::Total, Params::ScenarioComparator::Equal, 4), "1h3ai: t == 4");
            Check(ConditionEquals(conditions[1], Params::ScenarioCountField::HumanCount, Params::ScenarioComparator::Equal, 1), "1h3ai: h == 1");
            Check(ConditionEquals(conditions[2], Params::ScenarioCountField::AiCount, Params::ScenarioComparator::Equal, 3), "1h3ai: a == 3");
        }
    }
}

static void TestTemplateA_6total() {
    const std::string source = "match = function(t, h, a, pattern) return t == 6 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "6total: exactly one condition set");
    if (result.conditionSets.size() == 1) {
        const auto& conditions = result.conditionSets[0].conditions;
        Check(conditions.size() == 1, "6total: one conjunct");
        if (conditions.size() == 1)
            Check(ConditionEquals(conditions[0], Params::ScenarioCountField::Total, Params::ScenarioComparator::Equal, 6), "6total: t == 6");
    }
}

static void TestTemplateA_2hRestAI() {
    const std::string source = "match = function(t, h, a, pattern) return h == 2 and a >= 2 and t >= 4 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "2hRestAI: exactly one condition set");
    if (result.conditionSets.size() == 1) {
        const auto& conditions = result.conditionSets[0].conditions;
        Check(conditions.size() == 3, "2hRestAI: three conjuncts");
        if (conditions.size() == 3) {
            Check(ConditionEquals(conditions[0], Params::ScenarioCountField::HumanCount, Params::ScenarioComparator::Equal, 2), "2hRestAI: h == 2");
            Check(ConditionEquals(conditions[1], Params::ScenarioCountField::AiCount, Params::ScenarioComparator::GreaterOrEqual, 2), "2hRestAI: a >= 2");
            Check(ConditionEquals(conditions[2], Params::ScenarioCountField::Total, Params::ScenarioComparator::GreaterOrEqual, 4), "2hRestAI: t >= 4");
        }
    }
}

static void TestTemplateA_floor169() {
    const std::string source = "match = function(t, h, a, pattern) return t > 2 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "floor169: exactly one condition set");
    if (result.conditionSets.size() == 1) {
        const auto& conditions = result.conditionSets[0].conditions;
        Check(conditions.size() == 1, "floor169: one conjunct");
        if (conditions.size() == 1)
            Check(ConditionEquals(conditions[0], Params::ScenarioCountField::Total, Params::ScenarioComparator::GreaterThan, 2), "floor169: t > 2");
    }
}

// 7. Template B -- the real reference's `slots5to8AnyFilled` (line 142), byte-verbatim.
static void TestTemplateB_slots5to8AnyFilled() {
    const std::string source =
        "match = function(t, h, a, pattern) return pattern:sub(5, 8):find(\"[^-]\") ~= nil end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "TemplateB: exactly one condition set");
    Check(result.nearMisses.empty(), "TemplateB: no near-misses");
    if (result.conditionSets.size() == 1) {
        const auto& conditions = result.conditionSets[0].conditions;
        Check(conditions.size() == 1, "TemplateB: one condition");
        if (conditions.size() == 1)
            Check(ConditionEquals(conditions[0], Params::ScenarioCountField::SlotRangeOccupiedCount,
                                   Params::ScenarioComparator::GreaterOrEqual, 1, 5, 8),
                  "TemplateB: field=SlotRangeOccupiedCount, comparator=GreaterOrEqual, value=1, [5,8]");
    }
}

// 8. Near-miss: an `or`-bearing match function -- zero extracted, exactly one near-miss.
static void TestNearMiss_OrBearingExpression() {
    const std::string source = "match = function(t, h, a, pattern) return t == 2 or h == 1 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "OrBearing: no condition sets extracted");
    Check(result.nearMisses.size() == 1, "OrBearing: exactly one near-miss");
}

// 9. Near-miss: VAR compared to VAR, not a literal -- zero extracted, exactly one near-miss.
static void TestNearMiss_VarComparedToVar() {
    const std::string source = "match = function(t, h, a, pattern) return t == h end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "VarVsVar: no condition sets extracted");
    Check(result.nearMisses.size() == 1, "VarVsVar: exactly one near-miss");
}

// 10. Near-miss: a comparator chained with arithmetic on N -- zero extracted, exactly one near-miss.
static void TestNearMiss_ArithmeticOnLiteral() {
    const std::string source = "match = function(t, h, a, pattern) return t == 2 + 1 end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "Arithmetic: no condition sets extracted");
    Check(result.nearMisses.size() == 1, "Arithmetic: exactly one near-miss");
}

// 11. Near-miss: Template B with a different string literal -- zero extracted, exactly one near-miss.
static void TestNearMiss_TemplateBDifferentString() {
    const std::string source =
        "match = function(t, h, a, pattern) return pattern:sub(1, 2):find(\"abc\") ~= nil end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "TemplateBDifferentString: no condition sets extracted");
    Check(result.nearMisses.size() == 1, "TemplateBDifferentString: exactly one near-miss");
}

// 12. Near-miss: Template B with a different method name -- zero extracted, exactly one near-miss.
static void TestNearMiss_TemplateBDifferentMethod() {
    const std::string source =
        "match = function(t, h, a, pattern) return pattern:sub(1, 2):match(\"[^-]\") ~= nil end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "TemplateBDifferentMethod: no condition sets extracted");
    Check(result.nearMisses.size() == 1, "TemplateBDifferentMethod: exactly one near-miss");
}

// 13. Near-miss: Template B negated form -- zero extracted, exactly one near-miss.
static void TestNearMiss_TemplateBNegatedForm() {
    const std::string source =
        "match = function(t, h, a, pattern) return pattern:sub(5, 8):find(\"[^-]\") == nil end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "TemplateBNegated: no condition sets extracted");
    Check(result.nearMisses.size() == 1, "TemplateBNegated: exactly one near-miss");
}

// 14. Context tagging: the nearest preceding sibling `name = "..."` is carried as the candidate's
//     identifier, purely for human recognition (never interpreted further).
static void TestContextTagFromSiblingName() {
    const std::string source =
        "{ name = \"1v1\", match = function(t, h, a, pattern) return t == 2 end, area = AREA_356 },";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "ContextTag: one condition set");
    if (result.conditionSets.size() == 1)
        Check(result.conditionSets[0].identifier == "1v1", "ContextTag: identifier carries the sibling name");
}

// 15. Caps: a synthetic file exceeding kMaxScenarioMatchConditionExtractionCount stops extraction at
//     the cap and sets the flag, without crashing or unbounded-scanning.
static void TestCandidateCountCapEnforced() {
    std::string source;
    for (std::size_t index = 0; index < Io::kMaxScenarioMatchConditionExtractionCount + 8; ++index) {
        source += "match = function(t, h, a, pattern) return t == 2 end,\n";
    }
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == Io::kMaxScenarioMatchConditionExtractionCount, "CountCap: extraction stops at the cap");
    Check(result.bMatchConditionCountCapExceeded, "CountCap: flag set");
}

// 16. Caps: a single AND-chain exceeding kMaxScenarioMatchConditionConjunctCountPerChain is rejected
//     as a near-miss for that whole match function, not the file-wide cap.
static void TestConjunctCountCapEnforced() {
    std::string source = "match = function(t, h, a, pattern) return t == 1";
    for (std::size_t index = 0; index < Io::kMaxScenarioMatchConditionConjunctCountPerChain + 4; ++index) {
        source += " and t == 1";
    }
    source += " end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "ConjunctCap: no condition set extracted");
    Check(result.nearMisses.size() == 1, "ConjunctCap: exactly one near-miss");
    Check(!result.bMatchConditionCountCapExceeded, "ConjunctCap: file-wide cap flag NOT set by a per-chain rejection");
}

// 17. Optional whole-chain parentheses are tolerated (Part B: "parens around the whole chain
//     tolerated").
static void TestWholeChainParenthesesTolerated() {
    const std::string source = "match = function(t, h, a, pattern) return (t == 4 and h == 4) end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.size() == 1, "WholeChainParens: one condition set");
    if (result.conditionSets.size() == 1)
        Check(result.conditionSets[0].conditions.size() == 2, "WholeChainParens: two conjuncts still parsed");
}

// 18. No Lua execution of any kind: a body that would have a real side effect if it were ever
//     evaluated (rather than pattern-matched as text) is safely rejected as a near-miss -- and the
//     test process is demonstrably still alive to run the next check afterward. Neither .cpp in this
//     ticket includes any Lua/LuaJIT header or calls LuaTableEvaluate_SYS (confirmed by code review
//     when this file was written) -- this is the executable half of that same guarantee.
static void TestNoLuaExecutionOfAnyKind() {
    const std::string source =
        "match = function(t, h, a, pattern) return (function() os.exit(1) end)() end,";
    const Io::ScenarioMatchConditionExtractionResult result = Io::ExtractMatchConditionsFromScenarioScriptText(source);
    Check(result.conditionSets.empty(), "NoExecution: no condition set extracted from an unevaluated side-effecting body");
    Check(result.nearMisses.size() == 1, "NoExecution: exactly one near-miss, the process did not exit");
}

int main() {
    TestTemplateA_1v1();
    TestTemplateA_4human();
    TestTemplateA_1h3ai();
    TestTemplateA_6total();
    TestTemplateA_2hRestAI();
    TestTemplateA_floor169();
    TestTemplateB_slots5to8AnyFilled();
    TestNearMiss_OrBearingExpression();
    TestNearMiss_VarComparedToVar();
    TestNearMiss_ArithmeticOnLiteral();
    TestNearMiss_TemplateBDifferentString();
    TestNearMiss_TemplateBDifferentMethod();
    TestNearMiss_TemplateBNegatedForm();
    TestContextTagFromSiblingName();
    TestCandidateCountCapEnforced();
    TestConjunctCountCapEnforced();
    TestWholeChainParenthesesTolerated();
    TestNoLuaExecutionOfAnyKind();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

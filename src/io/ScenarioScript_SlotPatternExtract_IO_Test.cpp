// ScenarioScript_SlotPatternExtract_IO_Test.cpp -- pure-logic acceptance test for the closed
// literal-only Shape 2 grammar (STEP264, ARCH_15_14 Part B). No filesystem, no disk, matches the
// header's own "pure and total" contract.
#include "ScenarioScript_SlotPatternExtract_IO.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static const Io::ScenarioSlotPatternExtractionEntry* FindEntryByName(
        const Io::ScenarioSlotPatternExtractionResult& result, const char* name) {
    for (const Io::ScenarioSlotPatternExtractionEntry& entry : result.entries)
        if (entry.name == name) return &entry;
    return nullptr;
}

// 1. A PATTERN_SCENARIOS-shaped array with 3 entries extracts 3 {name, slotPattern} pairs verbatim,
//    including a pattern using every allowed character (`-`/`h`/`a`, per the real corpus's own
//    occupancy-string convention: `-` empty, letters mark an occupied slot).
static void TestThreeEntriesExtractedVerbatim() {
    const std::string source =
        "local PATTERN_SCENARIOS = {\n"
        "    { name = \"4human-slots5-8\", pattern = \"----hhhh--------\", area = AREA_169 },\n"
        "    { name = \"2v2-corners\", pattern = \"h--h------h--h--\" },\n"
        "    { name = \"mixed-occupancy\", pattern = \"ha-ha-ha--------\" },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.size() == 3, "ThreeEntries: exactly three entries");
    Check(result.nearMisses.empty(), "ThreeEntries: no near-misses");

    const Io::ScenarioSlotPatternExtractionEntry* first = FindEntryByName(result, "4human-slots5-8");
    Check(first != nullptr, "ThreeEntries: first entry present");
    if (first != nullptr) Check(first->slotPattern == "----hhhh--------", "ThreeEntries: first pattern verbatim");

    const Io::ScenarioSlotPatternExtractionEntry* second = FindEntryByName(result, "2v2-corners");
    Check(second != nullptr, "ThreeEntries: second entry present");
    if (second != nullptr) Check(second->slotPattern == "h--h------h--h--", "ThreeEntries: second pattern verbatim");

    const Io::ScenarioSlotPatternExtractionEntry* third = FindEntryByName(result, "mixed-occupancy");
    Check(third != nullptr, "ThreeEntries: third entry present");
    if (third != nullptr) Check(third->slotPattern == "ha-ha-ha--------", "ThreeEntries: third pattern verbatim");
}

// 2. Field order within an element does not matter -- `pattern` before `name` still extracts.
static void TestFieldOrderWithinElementDoesNotMatter() {
    const std::string source =
        "local PATTERN_SCENARIOS = {\n"
        "    { pattern = \"h-------h-------\", area = AREA_FULL, name = \"reordered\" },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.size() == 1, "FieldOrder: one entry");
    const Io::ScenarioSlotPatternExtractionEntry* entry = FindEntryByName(result, "reordered");
    Check(entry != nullptr, "FieldOrder: entry present despite pattern-before-name ordering");
    if (entry != nullptr) Check(entry->slotPattern == "h-------h-------", "FieldOrder: pattern verbatim");
}

// 3. An element with no `pattern` key at all is not a candidate -- no entry, no near-miss (lets an
//    unrelated COUNT_SCENARIOS-shaped array coexist in the same file without spurious diagnostics).
static void TestElementWithoutPatternKeyIsIgnored() {
    const std::string source =
        "local COUNT_SCENARIOS = {\n"
        "    { name = \"1v1\", match = function(t, h, a, pattern) return t == 2 end, area = AREA_356 },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.empty(), "NoPatternKey: no entries extracted");
    Check(result.nearMisses.empty(), "NoPatternKey: no near-misses (not a candidate at all)");
}

// 4. A `pattern` field that is not a plain string literal is a near-miss, contributing zero entries.
static void TestPatternNotAStringLiteralIsNearMiss() {
    const std::string source =
        "local PATTERN_SCENARIOS = {\n"
        "    { name = \"bad\", pattern = SOME_VARIABLE },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.empty(), "PatternNotString: no entries extracted");
    Check(result.nearMisses.size() == 1, "PatternNotString: exactly one near-miss");
}

// 5. A `pattern` field present without a sibling `name` field is a near-miss.
static void TestPatternWithoutNameIsNearMiss() {
    const std::string source =
        "local PATTERN_SCENARIOS = {\n"
        "    { pattern = \"h---------------\" },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.empty(), "PatternWithoutName: no entries extracted");
    Check(result.nearMisses.size() == 1, "PatternWithoutName: exactly one near-miss");
}

// 6. A keyed sub-table (e.g. `spawns = { ARMY_01 = {...} }`) is never mistaken for a positional
//    array element -- even one whose keyed children happen to contain a `pattern`-named field.
static void TestKeyedSubTableNeverMistakenForElement() {
    const std::string source =
        "local WEIRD = {\n"
        "    spawns = { pattern = { name = \"nope\", pattern = \"should-not-extract\" } },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.empty(), "KeyedSubTable: no entries extracted from a keyed (non-positional) sub-table");
}

// 7. Caps: a synthetic file exceeding kMaxScenarioSlotPatternExtractionCount stops extraction at the
//    cap and sets the flag, without crashing or unbounded-scanning.
static void TestEntryCountCapEnforced() {
    std::string source = "local PATTERN_SCENARIOS = {\n";
    for (std::size_t index = 0; index < Io::kMaxScenarioSlotPatternExtractionCount + 8; ++index) {
        source += "    { name = \"gen" + std::to_string(index) + "\", pattern = \"h---\" },\n";
    }
    source += "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.size() == Io::kMaxScenarioSlotPatternExtractionCount, "EntryCountCap: extraction stops at the cap");
    Check(result.bSlotPatternCountCapExceeded, "EntryCountCap: flag set");
}

// 8. Caps: a pattern string exceeding kMaxScenarioSlotPatternStringLength is rejected as a near-miss,
//    never silently truncated.
static void TestAbsurdLengthPatternRejected() {
    const std::string longPattern(Io::kMaxScenarioSlotPatternStringLength + 8, 'h');
    const std::string source =
        "local PATTERN_SCENARIOS = {\n"
        "    { name = \"toolong\", pattern = \"" + longPattern + "\" },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    Check(result.entries.empty(), "AbsurdLength: no entry extracted");
    Check(result.nearMisses.size() == 1, "AbsurdLength: exactly one near-miss");
}

// 9. No Lua execution of any kind: neither this file's translation unit nor
//    ScenarioScript_SlotPatternExtract_IO.cpp includes any Lua/LuaJIT header or calls
//    LuaTableEvaluate_SYS (confirmed by code review when this file was written); this scan is a
//    string/token match only, never an evaluation, so a string value that LOOKS like Lua code is
//    still extracted completely inertly, verbatim, never interpreted.
static void TestNoLuaExecutionOfAnyKind() {
    const std::string source =
        "local PATTERN_SCENARIOS = {\n"
        "    { name = \"looks-like-code\", pattern = \"os.exit(1)\" },\n"
        "}\n";
    const Io::ScenarioSlotPatternExtractionResult result = Io::ExtractSlotPatternsFromScenarioScriptText(source);
    const Io::ScenarioSlotPatternExtractionEntry* entry = FindEntryByName(result, "looks-like-code");
    Check(entry != nullptr, "NoExecution: entry still extracted (as inert text)");
    if (entry != nullptr) Check(entry->slotPattern == "os.exit(1)", "NoExecution: verbatim, never evaluated, process did not exit");
}

int main() {
    TestThreeEntriesExtractedVerbatim();
    TestFieldOrderWithinElementDoesNotMatter();
    TestElementWithoutPatternKeyIsIgnored();
    TestPatternNotAStringLiteralIsNearMiss();
    TestPatternWithoutNameIsNearMiss();
    TestKeyedSubTableNeverMistakenForElement();
    TestEntryCountCapEnforced();
    TestAbsurdLengthPatternRejected();
    TestNoLuaExecutionOfAnyKind();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

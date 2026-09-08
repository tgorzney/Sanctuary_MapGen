// ScenarioNameValidation_IO_Test.cpp — acceptance test for STEP251's pure charset/reserved-name/
// uniqueness validator over ScenarioBody::name. Modelled on
// MapExporter_ScenarioAreaNameValidation_IO_Test.cpp's own standalone Check()/main() shape. Pure —
// no imgui, no GL, no filesystem.
#include "ScenarioNameValidation_IO.h"
#include "../params/Scenario_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// 1. Empty Scenarios: the default scenario's own default-constructed empty name is itself a
// charset violation (empty string).
static void TestEmptyScenariosFlagsTheDefaultScenariosEmptyName() {
    Params::Scenarios scenarios;
    const Io::ScenarioNameValidationReport report = Io::ValidateScenarioNames(scenarios);
    Check(!report.AllNamesValid(), "an empty default name is flagged");
    Check(report.violations.size() == 1, "exactly one violation (the default scenario)");
    if (!report.violations.empty())
        Check(report.violations[0].reason.find("[A-Za-z0-9_]") != std::string::npos,
              "the violation is the charset reason");
}

// 2. Valid, unique live reference names across all three tiers -- proves the corrected
// (leading-digit-permitting) charset is actually implemented.
static void TestValidUniqueLiveReferenceNamesAcrossAllTiers() {
    Params::Scenarios scenarios;
    Params::PatternScenario pattern;
    pattern.body.name = "1v1";
    scenarios.patternScenarios.push_back(pattern);

    Params::CountScenario countA;
    countA.body.name = "slots5to8AnyFilled";
    scenarios.countScenarios.push_back(countA);
    Params::CountScenario countB;
    countB.body.name = "4human";
    scenarios.countScenarios.push_back(countB);
    Params::CountScenario countC;
    countC.body.name = "2h1ai";
    scenarios.countScenarios.push_back(countC);

    scenarios.defaultScenario.name = "floor169";

    const Io::ScenarioNameValidationReport report = Io::ValidateScenarioNames(scenarios);
    Check(report.AllNamesValid(), "every live reference name passes the corrected charset");
}

// 3. The task's own required examples: "1v1" passes, "My Scenario!" fails (charset reason).
static void TestRequiredExamplesOneVOneVsMyScenarioBang() {
    Params::Scenarios scenarios;
    scenarios.defaultScenario.name = "1v1";
    Params::PatternScenario pattern;
    pattern.body.name = "My Scenario!";
    scenarios.patternScenarios.push_back(pattern);

    const Io::ScenarioNameValidationReport report = Io::ValidateScenarioNames(scenarios);
    Check(report.FindViolationReasonForName("1v1") == nullptr, "\"1v1\" is not flagged");
    const std::string* reason = report.FindViolationReasonForName("My Scenario!");
    Check(reason != nullptr, "\"My Scenario!\" is flagged");
    if (reason != nullptr)
        Check(reason->find("[A-Za-z0-9_]") != std::string::npos,
              "and it is flagged for the charset reason");
}

// 4. Two names differing only by case both fail as duplicates.
static void TestCaseOnlyDifferenceIsFlaggedAsDuplicateOnBoth() {
    Params::Scenarios scenarios;
    Params::PatternScenario pattern;
    pattern.body.name = "FooBar";
    scenarios.patternScenarios.push_back(pattern);
    scenarios.defaultScenario.name = "foobar";

    const Io::ScenarioNameValidationReport report = Io::ValidateScenarioNames(scenarios);
    const std::string* reasonUpper = report.FindViolationReasonForName("FooBar");
    const std::string* reasonLower = report.FindViolationReasonForName("foobar");
    Check(reasonUpper != nullptr, "\"FooBar\" is flagged");
    Check(reasonLower != nullptr, "\"foobar\" is flagged");
    if (reasonUpper != nullptr)
        Check(reasonUpper->find("duplicates") != std::string::npos, "FooBar's reason is duplicate");
    if (reasonLower != nullptr)
        Check(reasonLower->find("duplicates") != std::string::npos, "foobar's reason is duplicate");
}

// 5. Reserved names, case-insensitive: "Runtime"/"runtime"/"RUNTIME"/"Data"/"data" each individually.
static void TestReservedNamesAreFlaggedCaseInsensitively() {
    const char* reservedCandidates[] = { "Runtime", "runtime", "RUNTIME", "Data", "data" };
    for (const char* candidate : reservedCandidates) {
        Params::Scenarios scenarios;
        scenarios.defaultScenario.name = candidate;
        const Io::ScenarioNameValidationReport report = Io::ValidateScenarioNames(scenarios);
        const std::string* reason = report.FindViolationReasonForName(candidate);
        Check(reason != nullptr, (std::string("reserved name flagged: ") + candidate).c_str());
        if (reason != nullptr)
            Check(reason->find("reserved") != std::string::npos,
                  (std::string("and carries the reserved-name reason: ") + candidate).c_str());
    }
}

// 6. First-failing-rule-wins: a charset-invalid name that would ALSO collide (case-insensitively,
// once "cleaned up") with another entry is flagged exactly once, for charset only.
static void TestCharsetFailureIsNeverAlsoFlaggedAsDuplicate() {
    Params::Scenarios scenarios;
    Params::PatternScenario pattern;
    pattern.body.name = "Foo Bar"; // charset-invalid (space)
    scenarios.patternScenarios.push_back(pattern);
    scenarios.defaultScenario.name = "Foo Bar"; // identical string -- would also be a duplicate

    const Io::ScenarioNameValidationReport report = Io::ValidateScenarioNames(scenarios);
    Check(report.violations.size() == 2, "both entries are flagged, but each exactly once");
    for (const Io::ScenarioNameValidationReport::Violation& violation : report.violations)
        Check(violation.reason.find("[A-Za-z0-9_]") != std::string::npos,
              "each violation is the charset reason, never also a duplicate reason");
}

// 7. SummaryText: empty on a clean report, populated with every name/reason on a dirty one.
static void TestSummaryTextShape() {
    Params::Scenarios cleanScenarios;
    cleanScenarios.defaultScenario.name = "clean1";
    Check(Io::ValidateScenarioNames(cleanScenarios).SummaryText().empty(),
          "a clean report's SummaryText is empty");

    Params::Scenarios dirtyScenarios;
    dirtyScenarios.defaultScenario.name = "Bad Name!";
    const std::string summary = Io::ValidateScenarioNames(dirtyScenarios).SummaryText();
    Check(!summary.empty(), "a dirty report's SummaryText is non-empty");
    Check(summary.find("Bad Name!") != std::string::npos, "the summary names the offending name");
    Check(summary.find("[A-Za-z0-9_]") != std::string::npos, "and its reason");
}

// 8. FindViolationReasonForName returns nullptr for a name with zero violations.
static void TestFindViolationReasonForNameReturnsNullForCleanName() {
    Params::Scenarios scenarios;
    scenarios.defaultScenario.name = "clean2";
    const Io::ScenarioNameValidationReport report = Io::ValidateScenarioNames(scenarios);
    Check(report.FindViolationReasonForName("clean2") == nullptr,
          "a name with zero violations returns nullptr");
}

int main() {
    TestEmptyScenariosFlagsTheDefaultScenariosEmptyName();
    TestValidUniqueLiveReferenceNamesAcrossAllTiers();
    TestRequiredExamplesOneVOneVsMyScenarioBang();
    TestCaseOnlyDifferenceIsFlaggedAsDuplicateOnBoth();
    TestReservedNamesAreFlaggedCaseInsensitively();
    TestCharsetFailureIsNeverAlsoFlaggedAsDuplicate();
    TestSummaryTextShape();
    TestFindViolationReasonForNameReturnsNullForCleanName();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

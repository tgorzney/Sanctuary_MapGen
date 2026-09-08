// ScenarioNameValidation_IO.cpp -- `ValidateScenarioNames` and
// `ScenarioNameValidationReport::SummaryText`/`FindViolationReasonForName`. Layer: IO. STEP251.
//
// Three independent checks, evaluated per scenario body, first-failing-rule-wins (charset, then
// reserved-name, then cross-set uniqueness) -- see ScenarioNameValidation_IO.h's own header comment
// for why this runs over the WHOLE Scenarios set regardless of spawnsUnits. No <regex> anywhere in
// this codebase (`grep -rn "#include <regex>" src/` returns nothing) -- charset is a manual
// per-character loop, matching this codebase's minimal-dependency convention.
#include "ScenarioNameValidation_IO.h"
#include "../params/Scenario_PARAMS.h"
#include <cctype>
#include <unordered_map>

namespace SanmapGen {
namespace Io {
namespace {

// Never-blank scenario descriptor, mirroring MapExporter_ScenarioAreaNameValidation_IO.cpp's own
// ScenarioAreaNameDescriptor -- IO owns its own copy (downward-only deps, Constitution §3).
std::string ScenarioNameDescriptor(const Params::ScenarioBody& body, const char* tierLabel, int index) {
    if (!body.name.empty()) return body.name;
    return std::string(tierLabel) + " Scenario #" + std::to_string(index + 1);
}

bool IsCharsetValid(const std::string& name) {
    if (name.empty()) return false;
    for (char character : name) {
        const bool bAllowed = std::isalnum(static_cast<unsigned char>(character)) || character == '_';
        if (!bAllowed) return false;
    }
    return true;
}

// Case-insensitive ASCII lowercasing, the exact idiom TemplateSourceScan_IO.cpp's IsTemplateExtension
// already uses.
std::string ToLowerAscii(const std::string& text) {
    std::string lower = text;
    for (char& character : lower)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    return lower;
}

bool IsReservedName(const std::string& name) {
    const std::string lower = ToLowerAscii(name);
    return lower == "runtime" || lower == "data";
}

struct ScenarioNameEntry {
    std::string descriptor;
    std::string name;
};

void CollectEntries(const Params::Scenarios& scenarios, std::vector<ScenarioNameEntry>& entries) {
    // Tier order: pattern, then count, then default (same tier order this file family already
    // establishes elsewhere, e.g. MapExporter_ScenarioAreaNameValidation_IO.cpp).
    for (std::size_t index = 0u; index < scenarios.patternScenarios.size(); ++index)
        entries.push_back({ScenarioNameDescriptor(scenarios.patternScenarios[index].body, "Pattern",
                                                  static_cast<int>(index)),
                           scenarios.patternScenarios[index].body.name});
    for (std::size_t index = 0u; index < scenarios.countScenarios.size(); ++index)
        entries.push_back({ScenarioNameDescriptor(scenarios.countScenarios[index].body, "Count",
                                                  static_cast<int>(index)),
                           scenarios.countScenarios[index].body.name});
    entries.push_back({ScenarioNameDescriptor(scenarios.defaultScenario, "Default", 0),
                       scenarios.defaultScenario.name});
}

} // namespace

ScenarioNameValidationReport ValidateScenarioNames(const Params::Scenarios& scenarios) {
    ScenarioNameValidationReport report;
    std::vector<ScenarioNameEntry> entries;
    CollectEntries(scenarios, entries);

    // Pass 1: charset, first-failing-rule-wins -- a charset-invalid name is never also
    // reserved-name/duplicate-checked.
    std::vector<bool> bAlreadyFlagged(entries.size(), false);
    for (std::size_t index = 0u; index < entries.size(); ++index) {
        if (IsCharsetValid(entries[index].name)) continue;
        report.violations.push_back({entries[index].descriptor, entries[index].name,
            "empty, or contains a character outside [A-Za-z0-9_]"});
        bAlreadyFlagged[index] = true;
    }

    // Pass 2: reserved name, only among charset-valid survivors.
    for (std::size_t index = 0u; index < entries.size(); ++index) {
        if (bAlreadyFlagged[index]) continue;
        if (!IsReservedName(entries[index].name)) continue;
        report.violations.push_back({entries[index].descriptor, entries[index].name,
            "'" + entries[index].name + "' collides with SanGen's own reserved "
            "<MapName>_Scenarios_Runtime.lua / _Data.lua filenames (case-insensitive) -- rename "
            "this scenario"});
        bAlreadyFlagged[index] = true;
    }

    // Pass 3: cross-set uniqueness, case-insensitive, only among charset-valid non-reserved
    // survivors. Counted first so the emitted violations stay in tier order (matching every other
    // pass above), rather than grouped by an unordered map's own iteration order.
    std::unordered_map<std::string, int> lowerNameOccurrenceCounts;
    for (std::size_t index = 0u; index < entries.size(); ++index) {
        if (bAlreadyFlagged[index]) continue;
        ++lowerNameOccurrenceCounts[ToLowerAscii(entries[index].name)];
    }
    for (std::size_t index = 0u; index < entries.size(); ++index) {
        if (bAlreadyFlagged[index]) continue;
        if (lowerNameOccurrenceCounts[ToLowerAscii(entries[index].name)] <= 1) continue;
        report.violations.push_back({entries[index].descriptor, entries[index].name,
            "duplicates another scenario's name, case-insensitively, elsewhere in this Scenarios set"});
    }

    return report;
}

const std::string* ScenarioNameValidationReport::FindViolationReasonForName(const std::string& name) const {
    for (const Violation& violation : violations)
        if (violation.name == name) return &violation.reason;
    return nullptr;
}

// ONE wording, shared by every call site -- do not restate the phrasing elsewhere.
std::string ScenarioNameValidationReport::SummaryText() const {
    if (AllNamesValid()) return std::string();
    std::string text = std::to_string(violations.size()) + " scenario(s) have an invalid Name:";
    for (const Violation& violation : violations)
        text += "\n  " + violation.scenarioDescriptor + " -> \"" + violation.name + "\": " + violation.reason;
    text += "\nA scenario's name is substituted directly into a game-loaded file path "
            "(<MapName>_Scenarios_<Name>.lua) -- fix it in the Scenarios tab before exporting the "
            "scenario script. Only that scenario's own category-4 generator file write is refused; "
            "the .sanmap and <MapName>_Scenarios_Data.lua exports are unaffected.";
    return text;
}

} // namespace Io
} // namespace SanmapGen

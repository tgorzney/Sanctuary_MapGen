// FootprintBakeStaleness_IO.cpp — the scan + the house warning SummaryText().
// Layer: IO. work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md §3.
#include "FootprintBakeStaleness_IO.h"
#include "FootprintBakeFingerprint_IO.h"
#include "TemplateIngest_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>

namespace SanmapGen {
namespace Io {
namespace {

// The bounded tpId-buffer -> std::string conversion every wire-mapping site already re-implements
// locally (MapExporter_ScatterTransform_IO.cpp's own copy is the closest precedent) -- the buffer's
// last byte need not be a terminator, so this is never a bare strlen/std::string(char*) construction.
std::string BoundedTemplateIdentifierText(const char (&templateIdentifier)[8]) {
    std::size_t length = 0;
    while (length < sizeof(templateIdentifier) && templateIdentifier[length] != '\0') ++length;
    return std::string(templateIdentifier, length);
}

// PLACEMENT_SCATTER_SPEC's planned per-rule `name` field has not landed on PropRule/UnitRule yet
// (this ticket's Open Question 2) -- "rule #<index>" is the specified fallback until it does.
std::string FallbackRuleName(std::size_t ruleIndex) {
    return "rule #" + std::to_string(ruleIndex);
}

// Shared by PropRule/UnitRule -- both carry footprintBakeFingerprint/baseFootprintWidth/
// baseFootprintDepth/transform with identical field names (ScatterRule_PARAMS.h), so one template
// walks both arrays rather than two hand-duplicated copies.
template <typename RuleType>
void ScanRules(const std::vector<RuleType>& rules, const char* ruleKind,
               const TemplateIngestReport& currentReport,
               std::vector<StaleFootprintEntry>& outStaleEntries) {
    for (std::size_t ruleIndex = 0; ruleIndex < rules.size(); ++ruleIndex) {
        const RuleType& rule = rules[ruleIndex];
        if (!rule.footprintBakeFingerprint.IsValid()) continue;   // never baked -- never checked
        const std::string templateIdentifier =
            BoundedTemplateIdentifierText(rule.transform.templateIdentifier);
        const TemplateFootprintRecord* const record =
            currentReport.FindByTemplateIdentifier(templateIdentifier);
        StaleFootprintEntry entry;
        if (record == nullptr) {
            entry.ruleKind             = ruleKind;
            entry.ruleName             = FallbackRuleName(ruleIndex);
            entry.templateIdentifier   = templateIdentifier;
            entry.oldBaseFootprintWidth = rule.baseFootprintWidth;
            entry.oldBaseFootprintDepth = rule.baseFootprintDepth;
            entry.bNoLongerIngestible  = true;
            outStaleEntries.push_back(entry);
            continue;
        }
        if (!FootprintBakeFingerprintIsStale(rule.footprintBakeFingerprint, record->sourceFingerprint))
            continue;
        entry.ruleKind              = ruleKind;
        entry.ruleName              = FallbackRuleName(ruleIndex);
        entry.templateIdentifier    = templateIdentifier;
        entry.oldBaseFootprintWidth = rule.baseFootprintWidth;
        entry.oldBaseFootprintDepth = rule.baseFootprintDepth;
        entry.newBaseFootprintWidth = record->baseFootprintWidth;
        entry.newBaseFootprintDepth = record->baseFootprintDepth;
        outStaleEntries.push_back(entry);
    }
}

} // namespace

std::string FootprintBakeStalenessReport::SummaryText() const {
    if (staleEntries.empty()) return std::string();
    std::string text = std::to_string(staleEntries.size())
        + " baked footprint value(s) are stale (the game's template data has changed since baking):\n";
    char line[256];
    for (const StaleFootprintEntry& entry : staleEntries) {
        if (entry.bNoLongerIngestible) {
            std::snprintf(line, sizeof(line),
                "  %s rule \"%s\" (tpId \"%s\"): baked %.2fx%.2f, template no longer found in the "
                "current game install (renamed, removed, or install changed)\n",
                entry.ruleKind.c_str(), entry.ruleName.c_str(), entry.templateIdentifier.c_str(),
                entry.oldBaseFootprintWidth, entry.oldBaseFootprintDepth);
        } else {
            std::snprintf(line, sizeof(line),
                "  %s rule \"%s\" (tpId \"%s\"): baked %.2fx%.2f, now %.2fx%.2f\n",
                entry.ruleKind.c_str(), entry.ruleName.c_str(), entry.templateIdentifier.c_str(),
                entry.oldBaseFootprintWidth, entry.oldBaseFootprintDepth,
                entry.newBaseFootprintWidth, entry.newBaseFootprintDepth);
        }
        text += line;
    }
    text += "Nothing was changed: SanGen never re-bakes a footprint value for you. Click \"Resolve "
            "Footprint\" on the affected rule to update it, or leave it as authored if the old value "
            "was intentional.";
    return text;
}

FootprintBakeStalenessReport CheckFootprintBakeStaleness(const Params::MapRecipe& recipe,
                                                          const TemplateIngestReport& currentReport) {
    FootprintBakeStalenessReport report;
    // No install / never ingested this session: the check must never require one (§18.2 rule 4 /
    // design doc §6.1). An empty footprint map means "unknown," not "everything was removed," so
    // nothing is reported stale here -- not even a rule baked in an earlier session.
    if (currentReport.footprintByTemplateIdentifier.empty()) return report;
    ScanRules(recipe.propRules, "Prop", currentReport, report.staleEntries);
    ScanRules(recipe.unitRules, "Unit", currentReport, report.staleEntries);
    return report;
}

} // namespace Io
} // namespace SanmapGen

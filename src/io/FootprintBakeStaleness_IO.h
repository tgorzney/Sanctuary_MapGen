// FootprintBakeStaleness_IO.h — detect (never auto-correct) baked-footprint staleness across
// recipe.propRules/unitRules. Layer: IO. ARCH_18_02_IngestedDataDeterminism.md §18.2 rules 2-4.
// work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md §3.
//
// Pure, read-only, touches no disk. Never run from src/proc/ (§18.2 rule 2 forbids PROC from ever
// seeing Io::TemplateIngestReport in any form) and never auto-corrects anything it finds (rule 3).
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

class TemplateIngestReport;

struct StaleFootprintEntry {
    std::string ruleKind;             // "Prop" or "Unit" -- which recipe array it came from
    std::string ruleName;             // PLACEMENT_SCATTER_SPEC's planned per-rule `name` field has
                                       // not landed yet (Open Question 2) -- "rule #<index>" until it does
    std::string templateIdentifier;
    float oldBaseFootprintWidth = 0.0f, oldBaseFootprintDepth = 0.0f;
    float newBaseFootprintWidth = 0.0f, newBaseFootprintDepth = 0.0f;  // 0/0 when bNoLongerIngestible
    bool  bNoLongerIngestible = false;  // FindByTemplateIdentifier returned nullptr THIS time
};

struct FootprintBakeStalenessReport {
    std::vector<StaleFootprintEntry> staleEntries;
    bool AllFresh() const { return staleEntries.empty(); }
    // ONE wording, house shape (STEP73 §0 / STEP82): loud, aggregate, non-blocking, names every
    // offender, never auto-fixes. Empty string when AllFresh().
    std::string SummaryText() const;
};

// Skips any rule whose footprintBakeFingerprint.IsValid() == false (never baked -- §18.2 rule 4, not
// an error, not reported, regardless of what currentReport contains). If currentReport carries no
// ingested footprint data at all (no install configured, or never ingested this session), returns an
// empty report immediately -- this check must never require a game install to run (§18.2 rule 4 /
// design doc §6.1): an unknown ingestion state is reported as "nothing to say," never as "everything
// is stale."
FootprintBakeStalenessReport CheckFootprintBakeStaleness(const Params::MapRecipe& recipe,
                                                          const TemplateIngestReport& currentReport);

} // namespace Io
} // namespace SanmapGen

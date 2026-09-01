// TemplateDialect_IO.cpp — see the header for the contract. Consumes an already-evaluated
// Sys::LuaTableEvaluateResult (ticket 85's output) purely in-memory: no file I/O, no Lua evaluation
// of its own. The header only forward-declares Sys::LuaTableEvaluateResult for its function
// signature; this .cpp needs the COMPLETE type (bSucceeded/errorMessage/globals are dereferenced
// below), so it includes LuaTableEvaluate_SYS.h. That is NOT a call to Sys::EvaluateLuaTableSource
// (this file never calls the evaluator) — just the header defining the already-produced result type.
// See STEP87's own "Correction" note.
#include "TemplateDialect_IO.h"
#include "../sys/LuaTableEvaluate_SYS.h"
#include <filesystem>
#include <unordered_map>

namespace SanmapGen {
namespace Io {

namespace {

// tpId resolution (STEP87 §2.3): prefer a declared, non-empty Text-kind general.tpId; else fall
// back to the filename stem of sourceLogicalPath.
void ResolveTemplateIdentifier(const Sys::LuaTableValue& rootTable, const std::string& sourcePath,
                                TemplateRecord& record) {
    const Sys::LuaTableValue* generalTable = rootTable.Find("general");
    const Sys::LuaTableValue* declared = generalTable != nullptr ? generalTable->Find("tpId") : nullptr;
    if (declared != nullptr && declared->kind == Sys::LuaTableValueKind::Text && !declared->text.empty()) {
        record.templateIdentifier = declared->text;
        record.bTpIdWasDeclared = true;
    } else {
        record.templateIdentifier = DeriveTemplateIdentifierFromPath(sourcePath);
        record.bTpIdWasDeclared = false;
    }
}

// Footprint (STEP87 §2.4, carrying dialects only): footprint.x/.y via AsNumber(0.0); an absent
// footprint field is an anomaly, flagged rather than silently treated as 0x0.
void ExtractFootprint(const Sys::LuaTableValue& rootTable, TemplateRecord& record, std::string& diagnostic) {
    const Sys::LuaTableValue* footprint = rootTable.Find("footprint");
    if (footprint == nullptr) {
        record.bHasFootprint = false;
        diagnostic = "recognized dialect but footprint field is missing";
        return;
    }
    const Sys::LuaTableValue* widthValue = footprint->Find("x");
    const Sys::LuaTableValue* depthValue = footprint->Find("y");
    record.baseFootprintWidth = widthValue != nullptr ? static_cast<float>(widthValue->AsNumber(0.0)) : 0.0f;
    record.baseFootprintDepth = depthValue != nullptr ? static_cast<float>(depthValue->AsNumber(0.0)) : 0.0f;
    record.bHasFootprint = true;
}

// Tags (STEP87 §2.5): top-level `tags` array only, never effects[].tag. Non-text elements are
// skipped silently (Constitution §6 — never crash on a malformed field).
void ExtractTags(const Sys::LuaTableValue& rootTable, TemplateRecord& record) {
    const Sys::LuaTableValue* tagsArray = rootTable.Find("tags");
    if (tagsArray == nullptr || tagsArray->kind != Sys::LuaTableValueKind::Array) return;
    for (const Sys::LuaTableValue& element : tagsArray->array)
        if (element.kind == Sys::LuaTableValueKind::Text) record.tags.push_back(element.text);
}

bool bDialectCarriesFootprint(TemplateDialectKind dialectKind) {
    return dialectKind == TemplateDialectKind::UnitTemplate ||
           dialectKind == TemplateDialectKind::PropTemplateLowercase ||
           dialectKind == TemplateDialectKind::PropTemplateUppercase;
}

} // namespace

// Root-table detection (STEP87 §2.1): checks the five recognized dialect globals in order,
// first hit wins. Returns nullptr (dialectKind left Unrecognized) when none match. Promoted out of
// this file's anonymous namespace (Auto-NavMesh mesh-ingestion work) so TemplateVisualLod_IO — a
// new additive sibling file, never an extension of TemplateRecord's own shipped scope — can reuse
// the exact same dialect detection instead of re-deriving it.
const Sys::LuaTableValue* DetectTemplateRootTable(const Sys::LuaTableValue& globals,
                                                  TemplateDialectKind& dialectKind) {
    const Sys::LuaTableValue* rootTable = nullptr;
    if ((rootTable = globals.Find("UnitTemplate")) != nullptr) dialectKind = TemplateDialectKind::UnitTemplate;
    else if ((rootTable = globals.Find("propTemplate")) != nullptr) dialectKind = TemplateDialectKind::PropTemplateLowercase;
    else if ((rootTable = globals.Find("PropTemplate")) != nullptr) dialectKind = TemplateDialectKind::PropTemplateUppercase;
    else if ((rootTable = globals.Find("ProjectileTemplate")) != nullptr) dialectKind = TemplateDialectKind::ProjectileTemplate;
    else if ((rootTable = globals.Find("MarkerTemplate")) != nullptr) dialectKind = TemplateDialectKind::MarkerTemplate;
    return rootTable;
}

std::string DeriveTemplateIdentifierFromPath(const std::string& path) {
    return std::filesystem::path(path).stem().string();
}

TemplateParseOutcome ParseTemplateSource(const Sys::LuaTableEvaluateResult& evaluated,
                                          const std::string& sourceLogicalPath,
                                          int sourcePriorityRank,
                                          std::uint64_t sourceByteSize,
                                          std::uint64_t sourceModifiedTime,
                                          std::uint64_t sourceContentHash) {
    TemplateParseOutcome outcome;
    if (!evaluated.bSucceeded) {
        outcome.diagnosticMessage = "evaluation failed: " + evaluated.errorMessage;
        return outcome;
    }

    TemplateDialectKind dialectKind = TemplateDialectKind::Unrecognized;
    const Sys::LuaTableValue* rootTable = DetectTemplateRootTable(evaluated.globals, dialectKind);
    if (rootTable == nullptr) {
        outcome.diagnosticMessage = "no recognized root table (possibly non-Lua content, e.g. a JSON .sanprop)";
        return outcome;
    }

    outcome.bRecognized = true;
    TemplateRecord& record = outcome.record;
    record.dialectKind = dialectKind;
    record.sourceLogicalPath = sourceLogicalPath;
    record.sourcePriorityRank = sourcePriorityRank;
    record.sourceByteSize = sourceByteSize;
    record.sourceModifiedTime = sourceModifiedTime;
    record.sourceContentHash = sourceContentHash;

    // tpId resolution applies to all five recognized kinds (Projectile/Marker included, for
    // diagnostics/counting, even though those two are skipped downstream by design).
    ResolveTemplateIdentifier(*rootTable, sourceLogicalPath, record);

    if (bDialectCarriesFootprint(dialectKind)) {
        ExtractFootprint(*rootTable, record, outcome.diagnosticMessage);
        ExtractTags(*rootTable, record);
    } else {
        record.bHasFootprint = false;
    }
    return outcome;
}

// Groups by templateIdentifier, preserving discovery order for both the group list and the source
// paths within each group; reports every group with more than one member. Read-only, decides no
// winner (see the header's own doc comment and STEP87's Q7 resolution).
TpIdCollisionReport DetectTpIdCollisions(const std::vector<TemplateRecord>& records) {
    TpIdCollisionReport report;
    std::vector<TpIdCollision> groups;
    std::unordered_map<std::string, std::size_t> identifierToGroupIndex;

    for (const TemplateRecord& record : records) {
        auto insertResult = identifierToGroupIndex.emplace(record.templateIdentifier, groups.size());
        if (insertResult.second) {
            groups.push_back(TpIdCollision{record.templateIdentifier, {}});
        }
        groups[insertResult.first->second].conflictingSourcePaths.push_back(record.sourceLogicalPath);
    }

    for (TpIdCollision& group : groups)
        if (group.conflictingSourcePaths.size() > 1) report.collisions.push_back(std::move(group));
    return report;
}

} // namespace Io
} // namespace SanmapGen

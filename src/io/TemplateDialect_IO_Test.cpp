// TemplateDialect_IO_Test.cpp — acceptance test for TemplateDialect_IO (STEP87). Builds
// Sys::LuaTableValue fixtures DIRECTLY (no live Lua evaluation) -- proving this file's own "no
// dependency on ticket 85's .cpp" boundary: only LuaTableValue_SYS.h's plain struct shape and
// LuaTableEvaluate_SYS.h's LuaTableEvaluateResult wrapper are needed to drive every scenario.
#include "TemplateDialect_IO.h"
#include "../sys/LuaTableEvaluate_SYS.h"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

Sys::LuaTableValue MakeText(const std::string& text) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Text;
    value.text = text;
    return value;
}

Sys::LuaTableValue MakeNumber(double number) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Number;
    value.number = number;
    return value;
}

Sys::LuaTableValue MakeTable(std::vector<std::pair<std::string, Sys::LuaTableValue>> entries) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Table;
    value.table = std::move(entries);
    return value;
}

Sys::LuaTableValue MakeArray(std::vector<Sys::LuaTableValue> elements) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Array;
    value.array = std::move(elements);
    return value;
}

Sys::LuaTableValue MakeFootprint(double x, double y) {
    return MakeTable({ {"x", MakeNumber(x)}, {"y", MakeNumber(y)} });
}

Sys::LuaTableValue MakeGeneral(const std::string& tpId) {
    return MakeTable({ {"tpId", MakeText(tpId)} });
}

// Wraps a single root-table entry (e.g. {"UnitTemplate", <table>}) into a succeeded evaluation
// result, mirroring what Sys::EvaluateLuaTableSource would have produced from real source text.
Sys::LuaTableEvaluateResult MakeSucceededResult(const std::string& rootTableName, Sys::LuaTableValue rootTable) {
    Sys::LuaTableEvaluateResult result;
    result.bSucceeded = true;
    result.globals = MakeTable({ {rootTableName, std::move(rootTable)} });
    return result;
}

void TestUnitTemplateWithDeclaredTpId() {
    Sys::LuaTableValue root = MakeTable({
        {"general", MakeGeneral("uca1001")},
        {"footprint", MakeFootprint(1.2, 1.2)},
    });
    const Sys::LuaTableEvaluateResult evaluated = MakeSucceededResult("UnitTemplate", std::move(root));
    const Io::TemplateParseOutcome outcome =
        Io::ParseTemplateSource(evaluated, "units/uca1001.santp", 0, 100, 0, 0);
    Check(outcome.bRecognized, "unit-template: bRecognized");
    Check(outcome.record.dialectKind == Io::TemplateDialectKind::UnitTemplate, "unit-template: dialectKind");
    Check(outcome.record.templateIdentifier == "uca1001", "unit-template: templateIdentifier");
    Check(outcome.record.bTpIdWasDeclared, "unit-template: bTpIdWasDeclared");
    Check(outcome.record.bHasFootprint, "unit-template: bHasFootprint");
    Check(outcome.record.baseFootprintWidth == 1.2f && outcome.record.baseFootprintDepth == 1.2f,
          "unit-template: footprint values");
}

void TestMissingDeclaredTpIdFallsBackToPathStem() {
    Sys::LuaTableValue root = MakeTable({ {"footprint", MakeFootprint(2.0, 2.0)} });   // no `general`
    const Sys::LuaTableEvaluateResult evaluated = MakeSucceededResult("UnitTemplate", std::move(root));
    const Io::TemplateParseOutcome outcome =
        Io::ParseTemplateSource(evaluated, "Environment/01_Highlands/Props/edbm0149/edbm0149.santp", 0, 0, 0, 0);
    Check(outcome.bRecognized, "path-fallback: bRecognized");
    Check(outcome.record.templateIdentifier == "edbm0149", "path-fallback: templateIdentifier == stem");
    Check(!outcome.record.bTpIdWasDeclared, "path-fallback: bTpIdWasDeclared false");
}

// Both prop dialects parse correctly by root-table NAME alone, and a `collider`/`collisionInfo`
// field present in the fixture is never inspected (ARCH_18_03 out-of-scope deferral) -- proven by
// bHasFootprint staying true regardless of their presence.
void TestBothPropDialectsIgnoreColliderFields() {
    Sys::LuaTableValue lowercaseRoot = MakeTable({
        {"general", MakeGeneral("edbm0149")},
        {"footprint", MakeFootprint(0.5, 0.5)},
        {"collider", MakeText("ignored-by-this-file")},
    });
    const Sys::LuaTableEvaluateResult lowercaseEvaluated =
        MakeSucceededResult("propTemplate", std::move(lowercaseRoot));
    const Io::TemplateParseOutcome lowercaseOutcome =
        Io::ParseTemplateSource(lowercaseEvaluated, "edbm0149.sanprop", 0, 0, 0, 0);
    Check(lowercaseOutcome.bRecognized, "prop-dialect-A: bRecognized");
    Check(lowercaseOutcome.record.dialectKind == Io::TemplateDialectKind::PropTemplateLowercase,
          "prop-dialect-A: dialectKind");
    Check(lowercaseOutcome.record.bHasFootprint, "prop-dialect-A: bHasFootprint despite collider field");

    Sys::LuaTableValue uppercaseRoot = MakeTable({
        {"general", MakeGeneral("cliff03")},
        {"footprint", MakeFootprint(3.0, 3.0)},
        {"collisionInfo", MakeTable({ {"radius", MakeNumber(1.0)} })},
    });
    const Sys::LuaTableEvaluateResult uppercaseEvaluated =
        MakeSucceededResult("PropTemplate", std::move(uppercaseRoot));
    const Io::TemplateParseOutcome uppercaseOutcome =
        Io::ParseTemplateSource(uppercaseEvaluated, "cliff03.sanprop", 0, 0, 0, 0);
    Check(uppercaseOutcome.bRecognized, "prop-dialect-B: bRecognized");
    Check(uppercaseOutcome.record.dialectKind == Io::TemplateDialectKind::PropTemplateUppercase,
          "prop-dialect-B: dialectKind");
    Check(uppercaseOutcome.record.bHasFootprint, "prop-dialect-B: bHasFootprint despite collisionInfo field");
}

void TestProjectileAndMarkerRecognizedButNoFootprint() {
    Sys::LuaTableValue projectileRoot = MakeTable({ {"general", MakeGeneral("proj001")} });
    const Sys::LuaTableEvaluateResult projectileEvaluated =
        MakeSucceededResult("ProjectileTemplate", std::move(projectileRoot));
    const Io::TemplateParseOutcome projectileOutcome =
        Io::ParseTemplateSource(projectileEvaluated, "proj001.santp", 0, 0, 0, 0);
    Check(projectileOutcome.bRecognized, "projectile: bRecognized");
    Check(projectileOutcome.record.dialectKind == Io::TemplateDialectKind::ProjectileTemplate,
          "projectile: dialectKind");
    Check(!projectileOutcome.record.bHasFootprint, "projectile: bHasFootprint false");

    Sys::LuaTableValue markerRoot = MakeTable({ {"general", MakeGeneral("spawn001")} });
    const Sys::LuaTableEvaluateResult markerEvaluated =
        MakeSucceededResult("MarkerTemplate", std::move(markerRoot));
    const Io::TemplateParseOutcome markerOutcome =
        Io::ParseTemplateSource(markerEvaluated, "spawn001.santp", 0, 0, 0, 0);
    Check(markerOutcome.bRecognized, "marker: bRecognized");
    Check(markerOutcome.record.dialectKind == Io::TemplateDialectKind::MarkerTemplate, "marker: dialectKind");
    Check(!markerOutcome.record.bHasFootprint, "marker: bHasFootprint false");
}

void TestFailedEvaluationIsRecognizedFalse() {
    Sys::LuaTableEvaluateResult evaluated;
    evaluated.bSucceeded = false;
    evaluated.errorMessage = "unexpected symbol near '('";
    const Io::TemplateParseOutcome outcome = Io::ParseTemplateSource(evaluated, "broken.santp", 0, 0, 0, 0);
    Check(!outcome.bRecognized, "failed-evaluation: bRecognized false");
    Check(outcome.diagnosticMessage.find("evaluation failed") != std::string::npos,
          "failed-evaluation: diagnosticMessage mentions evaluation failed");
}

void TestNoRecognizedRootTable() {
    const Sys::LuaTableEvaluateResult evaluated = MakeSucceededResult("SomeOtherGlobal", MakeTable({}));
    const Io::TemplateParseOutcome outcome = Io::ParseTemplateSource(evaluated, "weird.sanprop", 0, 0, 0, 0);
    Check(!outcome.bRecognized, "no-recognized-root: bRecognized false");
    Check(outcome.diagnosticMessage.find("no recognized root table") != std::string::npos,
          "no-recognized-root: diagnosticMessage mentions no recognized root table");
}

void TestRecognizedFixtureMissingFootprint() {
    Sys::LuaTableValue root = MakeTable({ {"general", MakeGeneral("uca9999")} });   // no `footprint`
    const Sys::LuaTableEvaluateResult evaluated = MakeSucceededResult("UnitTemplate", std::move(root));
    const Io::TemplateParseOutcome outcome = Io::ParseTemplateSource(evaluated, "uca9999.santp", 0, 0, 0, 0);
    Check(outcome.bRecognized, "missing-footprint: bRecognized still true");
    Check(!outcome.record.bHasFootprint, "missing-footprint: bHasFootprint false");
}

// Proves tags extraction keys off the top-level `tags` array only, never the redundant
// effects[].tag entries (a real, empirically-confirmed distinction in the actual game data).
void TestTagsExtractionKeysOffTopLevelOnly() {
    Sys::LuaTableValue taggedRoot = MakeTable({
        {"general", MakeGeneral("uca1002")},
        {"footprint", MakeFootprint(1.0, 1.0)},
        {"tags", MakeArray({ MakeText("HARVESTABLE"), MakeText("FLAMMABLE") })},
    });
    const Sys::LuaTableEvaluateResult taggedEvaluated = MakeSucceededResult("UnitTemplate", std::move(taggedRoot));
    const Io::TemplateParseOutcome taggedOutcome = Io::ParseTemplateSource(taggedEvaluated, "uca1002.santp", 0, 0, 0, 0);
    Check(taggedOutcome.record.tags.size() == 2 && taggedOutcome.record.tags[0] == "HARVESTABLE" &&
          taggedOutcome.record.tags[1] == "FLAMMABLE", "tags: top-level tags read correctly");

    Sys::LuaTableValue effectsOnlyRoot = MakeTable({
        {"general", MakeGeneral("uca1003")},
        {"footprint", MakeFootprint(1.0, 1.0)},
        {"effects", MakeArray({ MakeTable({ {"tag", MakeText("HARVESTABLE")} }) })},
    });
    const Sys::LuaTableEvaluateResult effectsOnlyEvaluated =
        MakeSucceededResult("UnitTemplate", std::move(effectsOnlyRoot));
    const Io::TemplateParseOutcome effectsOnlyOutcome =
        Io::ParseTemplateSource(effectsOnlyEvaluated, "uca1003.santp", 0, 0, 0, 0);
    Check(effectsOnlyOutcome.record.tags.empty(),
          "tags: effects[].tag without top-level tags produces an empty vector");
}

void TestDeriveTemplateIdentifierFromPath() {
    Check(Io::DeriveTemplateIdentifierFromPath(
              "Environment/01_Highlands/Props/edbm0149/edbm0149.santp") == "edbm0149",
          "derive-tpid: real filesystem path");
    Check(Io::DeriveTemplateIdentifierFromPath(
              "C:/game/Environment.sanpack!Environment/01_Highlands/Props/edbm0149/edbm0149.santp") == "edbm0149",
          "derive-tpid: synthetic sanpack-entry logical path");
}

// Mirrors the real Cliff_03.sanprop-declares-tpId-"Cliff_02" case: one record's tpId was declared,
// the other's was derived from a differently-named file, both landing on the SAME templateIdentifier.
void TestDetectTpIdCollisions() {
    std::vector<Io::TemplateRecord> records;
    Io::TemplateRecord declared;
    declared.templateIdentifier = "Cliff_02";
    declared.sourceLogicalPath = "Cliff_03.sanprop";
    declared.bTpIdWasDeclared = true;
    records.push_back(declared);

    Io::TemplateRecord derived;
    derived.templateIdentifier = "Cliff_02";
    derived.sourceLogicalPath = "Cliff_02.sanprop";
    derived.bTpIdWasDeclared = false;
    records.push_back(derived);

    Io::TemplateRecord unrelatedFirst;
    unrelatedFirst.templateIdentifier = "Rock_01";
    unrelatedFirst.sourceLogicalPath = "Rock_01.sanprop";
    records.push_back(unrelatedFirst);

    Io::TemplateRecord unrelatedSecond;
    unrelatedSecond.templateIdentifier = "Tree_05";
    unrelatedSecond.sourceLogicalPath = "Tree_05.sanprop";
    records.push_back(unrelatedSecond);

    const Io::TpIdCollisionReport report = Io::DetectTpIdCollisions(records);
    Check(report.AnyCollisions(), "collisions: AnyCollisions true");
    Check(report.collisions.size() == 1, "collisions: exactly one collision group");
    if (report.collisions.size() == 1) {
        const Io::TpIdCollision& collision = report.collisions[0];
        Check(collision.templateIdentifier == "Cliff_02", "collisions: templateIdentifier == Cliff_02");
        Check(collision.conflictingSourcePaths.size() == 2, "collisions: exactly two conflicting sources");
        Check(collision.conflictingSourcePaths[0] == "Cliff_03.sanprop" &&
              collision.conflictingSourcePaths[1] == "Cliff_02.sanprop",
              "collisions: both source paths named in discovery order");
    }
}

} // namespace

int main() {
    TestUnitTemplateWithDeclaredTpId();
    TestMissingDeclaredTpIdFallsBackToPathStem();
    TestBothPropDialectsIgnoreColliderFields();
    TestProjectileAndMarkerRecognizedButNoFootprint();
    TestFailedEvaluationIsRecognizedFalse();
    TestNoRecognizedRootTable();
    TestRecognizedFixtureMissingFootprint();
    TestTagsExtractionKeysOffTopLevelOnly();
    TestDeriveTemplateIdentifierFromPath();
    TestDetectTpIdCollisions();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

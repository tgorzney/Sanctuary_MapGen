// PropReclaimableBake_IO_Test.cpp — acceptance test for STEP92's human-triggered bReclaimable bake
// over ticket 89's Io::TemplateIngestReport::tagsByTemplateIdentifier. Builds TemplateIngestReport
// fixtures directly (no real ingestion run needed — TemplateHasHarvestableTag/the two bake functions
// are pure over an already-produced report), mirroring TemplateDialect_IO_Test.cpp's direct-fixture
// convention.
#include "PropReclaimableBake_IO.h"
#include "../params/ScatterRule_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
#include <cstdio>
#include <cstring>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// Test 1: known templateIdentifier, present vs. absent HARVESTABLE tag.
void TestTemplateHasHarvestableTagKnownIds() {
    Io::TemplateIngestReport report;
    report.tagsByTemplateIdentifier["edbm0101"] = { "HARVESTABLE", "FLAMMABLE" };
    report.tagsByTemplateIdentifier["edbm0102"] = { "FLAMMABLE" };

    Check(Io::TemplateHasHarvestableTag(report, "edbm0101") == true,
          "has-tag: known id with HARVESTABLE present -> true");
    Check(Io::TemplateHasHarvestableTag(report, "edbm0102") == false,
          "has-tag: known id without HARVESTABLE -> false");
}

// Test 2: unknown templateIdentifier (no entry at all) -> false, same return value as test 1's
// "known, absent" case but a distinct code path (proven by the bake functions below).
void TestTemplateHasHarvestableTagUnknownId() {
    Io::TemplateIngestReport report;
    report.tagsByTemplateIdentifier["edbm0101"] = { "HARVESTABLE" };

    Check(Io::TemplateHasHarvestableTag(report, "unknown_id") == false,
          "has-tag: never-ingested id -> false");
}

// Test 3: bake, found, sets true.
void TestBakePropRuleFoundSetsTrue() {
    Io::TemplateIngestReport report;
    report.tagsByTemplateIdentifier["edbm101"] = { "HARVESTABLE" };

    Params::PropRule rule;
    std::memcpy(rule.transform.templateIdentifier, "edbm101", 7u);
    rule.bReclaimable = false;

    const bool bBaked = Io::BakeReclaimableForPropRule(report, rule);
    Check(bBaked == true, "bake propRule found: returns true");
    Check(rule.bReclaimable == true, "bake propRule found: sets bReclaimable true");
}

// Test 4: bake, found, sets false — proves the overwrite is bidirectional, not a one-way ratchet.
void TestBakePropRuleFoundSetsFalse() {
    Io::TemplateIngestReport report;
    report.tagsByTemplateIdentifier["edbm101"] = { "FLAMMABLE" };  // no HARVESTABLE

    Params::PropRule rule;
    std::memcpy(rule.transform.templateIdentifier, "edbm101", 7u);
    rule.bReclaimable = true;

    const bool bBaked = Io::BakeReclaimableForPropRule(report, rule);
    Check(bBaked == true, "bake propRule found (no tag): returns true (a bake happened)");
    Check(rule.bReclaimable == false, "bake propRule found (no tag): overwrites true back to false");
}

// Test 5: bake, not found, target untouched — never clears a hand-authored true.
void TestBakePropRuleNotFoundLeavesUntouched() {
    Io::TemplateIngestReport report;
    report.tagsByTemplateIdentifier["someOtherId"] = { "HARVESTABLE" };

    Params::PropRule rule;
    std::memcpy(rule.transform.templateIdentifier, "edbm101", 7u);
    rule.bReclaimable = true;

    const bool bBaked = Io::BakeReclaimableForPropRule(report, rule);
    Check(bBaked == false, "bake propRule not found: returns false");
    Check(rule.bReclaimable == true, "bake propRule not found: bReclaimable untouched");
}

// Test 6: PropInstanceGroup, tpId derived from blueprintPath.
void TestBakePropInstanceGroupDerivesFromBlueprintPath() {
    Io::TemplateIngestReport report;
    report.tagsByTemplateIdentifier["edbm0101"] = { "HARVESTABLE" };

    Params::PropInstanceGroup group;
    group.blueprintPath = "Environment/01_Highlands/Props/edbm0101/edbm0101.santp";
    group.bReclaimable = false;

    const bool bBaked = Io::BakeReclaimableForPropInstanceGroup(report, group);
    Check(bBaked == true, "bake propInstanceGroup found: returns true");
    Check(group.bReclaimable == true, "bake propInstanceGroup found: sets bReclaimable true");
}

// Test 7: empty templateIdentifier / blueprintPath -> false, untouched, no crash.
void TestBakeEmptyIdentifiersReturnFalseNoCrash() {
    Io::TemplateIngestReport report;
    report.tagsByTemplateIdentifier["edbm0101"] = { "HARVESTABLE" };

    Params::PropRule rule;  // transform.templateIdentifier default-constructed all-zero (empty)
    rule.bReclaimable = true;
    const bool bRuleBaked = Io::BakeReclaimableForPropRule(report, rule);
    Check(bRuleBaked == false, "bake propRule empty id: returns false");
    Check(rule.bReclaimable == true, "bake propRule empty id: bReclaimable untouched");

    Params::PropInstanceGroup group;  // blueprintPath default-constructed empty
    group.bReclaimable = true;
    const bool bGroupBaked = Io::BakeReclaimableForPropInstanceGroup(report, group);
    Check(bGroupBaked == false, "bake propInstanceGroup empty path: returns false");
    Check(group.bReclaimable == true, "bake propInstanceGroup empty path: bReclaimable untouched");
}

} // namespace

int main() {
    TestTemplateHasHarvestableTagKnownIds();
    TestTemplateHasHarvestableTagUnknownId();
    TestBakePropRuleFoundSetsTrue();
    TestBakePropRuleFoundSetsFalse();
    TestBakePropRuleNotFoundLeavesUntouched();
    TestBakePropInstanceGroupDerivesFromBlueprintPath();
    TestBakeEmptyIdentifiersReturnFalseNoCrash();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

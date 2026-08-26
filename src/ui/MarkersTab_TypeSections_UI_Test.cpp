// MarkersTab_TypeSections_UI_Test.cpp — STEP125/STEP128 acceptance for EnumerateMarkerTypeSectionNames
// (ARCH §19.14): the Alloy/Plasma/Spawn-first fixed order (present-only), alphabetical-others,
// cross-collection union+dedup, the all-legacy-data degrade case, and (STEP128 §4) the Unassigned
// bucket's own present-only rule — a genuinely empty recipe now returns {}, not {""}. Pure logic
// only — no imgui frame, no window, no GL context.
#include "MarkersTab_TypeSections_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool NamesEqual(const std::vector<std::string>& actual, const std::vector<std::string>& expected) {
    return actual == expected;
}

// STEP128 §4: empty bundles/ruleLayers/instanceLayers -> exactly {} — the Unassigned bucket is now
// present-only, the SAME test every other name gets; with zero entries of any kind, nothing is
// "genuinely empty-typed" either, so "" does not appear (retires STEP125's own always-appended rule).
void RunEmptyDataBootstrapCheck() {
    const std::vector<Params::MarkerLayerBundle> bundles;
    const std::vector<Params::MarkerRuleLayer> ruleLayers;
    const std::vector<Params::MarkerInstanceLayer> instanceLayers;
    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, {}), "empty everything (zero bundles/layers at all) returns exactly {} — no "
                                  "Unassigned bucket invented when nothing is genuinely empty-typed");
}

// Bundles typed {"Spawn", "Alloy", "Expansion"}, a rule layer typed "Generic", an instance layer
// typed "" (explicit empty) -> {"Alloy", "Spawn", "Expansion", "Generic", ""}.
void RunFixedOrderAlphabeticalUnassignedCheck() {
    std::vector<Params::MarkerLayerBundle> bundles(3);
    bundles[0].markerTypeName = "Spawn";
    bundles[1].markerTypeName = "Alloy";
    bundles[2].markerTypeName = "Expansion";
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    ruleLayers[0].markerTypeName = "Generic";
    std::vector<Params::MarkerInstanceLayer> instanceLayers(1);
    instanceLayers[0].markerTypeName = "";   // explicit empty

    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, { "Alloy", "Spawn", "Expansion", "Generic", "" }),
          "Alloy/Plasma/Spawn-first (Plasma absent, correctly skipped), alphabetical-others "
          "(Expansion before Generic), Unassigned last, cross-collection union (a name on only a "
          "rule layer still shows up)");
}

// The same "Alloy" value present on both a Bundle AND a rule layer produces exactly ONE "Alloy"
// entry — the union is deduped, not per-collection. STEP128 §4: neither entry is empty-typed, so the
// Unassigned bucket correctly does NOT appear either.
void RunCrossCollectionDedupCheck() {
    std::vector<Params::MarkerLayerBundle> bundles(1);
    bundles[0].markerTypeName = "Alloy";
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    ruleLayers[0].markerTypeName = "Alloy";
    const std::vector<Params::MarkerInstanceLayer> instanceLayers;

    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, { "Alloy" }),
          "the same type on two collections produces exactly one entry, and no Unassigned bucket "
          "(nothing is genuinely empty-typed)");
}

// STEP128 §4: a plain data-level assertion, no imgui frame needed — typing a real name into a
// previously-empty row's own markerTypeName (the free-text field's own mutation target) moves it out
// of the Unassigned bucket and into that name's own section, next call.
void RunTypingIntoEmptyRowMovesSectionCheck() {
    std::vector<Params::MarkerLayerBundle> bundles;
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);   // starts genuinely empty-typed
    const std::vector<Params::MarkerInstanceLayer> instanceLayers;

    const std::vector<std::string> before = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(before, { "" }), "before typing, the row sits in the Unassigned bucket alone");

    ruleLayers[0].markerTypeName = "Generic";   // the free-text field's own DrawTextInput mutation target
    const std::vector<std::string> after = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(after, { "Generic" }),
          "typing a real name moves the row's own Type section next enumeration — Unassigned no "
          "longer appears, \"Generic\" now does");
}

// bundles/ruleLayers/instanceLayers each non-empty but EVERY entry has markerTypeName == "" ->
// exactly {""} — the all-legacy-data case degrades to the single Unassigned bucket.
void RunAllLegacyDataDegradesCheck() {
    const std::vector<Params::MarkerLayerBundle> bundles(2);
    const std::vector<Params::MarkerRuleLayer> ruleLayers(2);
    const std::vector<Params::MarkerInstanceLayer> instanceLayers(2);
    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, { "" }),
          "non-empty collections with no named type anywhere still degrade to exactly {\"\"}, no "
          "fixed-name entries invented");
}

} // namespace

int main() {
    RunEmptyDataBootstrapCheck();
    RunFixedOrderAlphabeticalUnassignedCheck();
    RunCrossCollectionDedupCheck();
    RunAllLegacyDataDegradesCheck();
    RunTypingIntoEmptyRowMovesSectionCheck();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

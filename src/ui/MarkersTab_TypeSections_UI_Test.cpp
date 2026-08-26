// MarkersTab_TypeSections_UI_Test.cpp — acceptance for EnumerateMarkerTypeSectionNames (ARCH
// §19.14): the Alloy/Plasma/Spawn-first fixed order, alphabetical-others, cross-collection
// union+dedup, and (human's own instruction) that NO "(Unassigned)" bucket is ever produced, under
// any circumstance, including all-empty-typed data. Pure logic only — no imgui frame, no window,
// no GL context.
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

// Empty bundles/ruleLayers/instanceLayers -> exactly {}.
void RunEmptyDataBootstrapCheck() {
    const std::vector<Params::MarkerLayerBundle> bundles;
    const std::vector<Params::MarkerRuleLayer> ruleLayers;
    const std::vector<Params::MarkerInstanceLayer> instanceLayers;
    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, {}), "empty everything returns exactly {}");
}

// Bundles typed {"Spawn", "Alloy", "Expansion"}, a rule layer typed "Generic", an instance layer
// typed "" (explicit empty, must be silently dropped, never surfaced as its own section) ->
// {"Alloy", "Spawn", "Expansion", "Generic"}.
void RunFixedOrderAlphabeticalCheck() {
    std::vector<Params::MarkerLayerBundle> bundles(3);
    bundles[0].markerTypeName = "Spawn";
    bundles[1].markerTypeName = "Alloy";
    bundles[2].markerTypeName = "Expansion";
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    ruleLayers[0].markerTypeName = "Generic";
    std::vector<Params::MarkerInstanceLayer> instanceLayers(1);
    instanceLayers[0].markerTypeName = "";   // explicit empty — must never appear in the result

    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, { "Alloy", "Spawn", "Expansion", "Generic" }),
          "Alloy/Plasma/Spawn-first (Plasma absent, correctly skipped), alphabetical-others "
          "(Expansion before Generic), no Unassigned entry despite the empty-typed instance layer, "
          "cross-collection union (a name on only a rule layer still shows up)");
}

// The same "Alloy" value present on both a Bundle AND a rule layer produces exactly ONE "Alloy"
// entry — the union is deduped, not per-collection.
void RunCrossCollectionDedupCheck() {
    std::vector<Params::MarkerLayerBundle> bundles(1);
    bundles[0].markerTypeName = "Alloy";
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    ruleLayers[0].markerTypeName = "Alloy";
    const std::vector<Params::MarkerInstanceLayer> instanceLayers;

    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, { "Alloy" }), "the same type on two collections produces exactly one entry");
}

// Human's own instruction: no "(Unassigned)" section exists at all, no exceptions — even when
// EVERY bundle/ruleLayer/instanceLayer is empty-typed (the old all-legacy-data case), the result
// is empty, not {""}. Such data is simply not reachable from the Type-section view.
void RunAllEmptyTypedDataProducesNoSectionsCheck() {
    const std::vector<Params::MarkerLayerBundle> bundles(2);
    const std::vector<Params::MarkerRuleLayer> ruleLayers(2);
    const std::vector<Params::MarkerInstanceLayer> instanceLayers(2);
    const std::vector<std::string> result = EnumerateMarkerTypeSectionNames(bundles, ruleLayers, instanceLayers);
    Check(NamesEqual(result, {}),
          "non-empty collections with no named type anywhere produce NO sections at all -- no "
          "Unassigned bucket, ever");
}

} // namespace

int main() {
    RunEmptyDataBootstrapCheck();
    RunFixedOrderAlphabeticalCheck();
    RunCrossCollectionDedupCheck();
    RunAllEmptyTypedDataProducesNoSectionsCheck();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

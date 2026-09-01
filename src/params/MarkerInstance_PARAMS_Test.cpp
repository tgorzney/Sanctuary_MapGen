// MarkerInstance_PARAMS_Test.cpp — default-value acceptance for MarkerTransform's bare-int
// sentinel fields (STEP244, ARCH §19.33/§21.9). Mirrors PropInstance_PARAMS_Test.cpp's
// RunFieldDefaultChecks/Check/failureCount/main() style. Headers compile standalone: no JSON, no
// CMake link beyond the default (add_sangen_test).
#include "MarkerInstance_PARAMS.h"
#include <cstdio>
#include <type_traits>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunFieldDefaultChecks() {
    Params::MarkerTransform transform;
    Check(transform.instanceIdentifier == -1, "MarkerTransform::instanceIdentifier defaults to -1 (unassigned)");
    Check(transform.symmetryGroupIdentifier == 0, "MarkerTransform::symmetryGroupIdentifier defaults to 0 (ungrouped)");
    Check(transform.linkIdentifier == -1,
          "MarkerTransform::linkIdentifier (ARCH §19.33, instance-tier Link membership) defaults to -1 (not Link-bound)");
}

void RunTransformTypeAliasChecks() {
    // Compile-time only: confirms Params::MarkerInstanceGroup::TransformType (ARCH §21.9) names
    // MarkerTransform, exactly as ManualInstanceHitTest_UI.h/ManualInstanceDelete_UI.h will rely on
    // once STEP249 wires them in. A mismatch fails to compile, not at runtime.
    static_assert(std::is_same<Params::MarkerInstanceGroup::TransformType, Params::MarkerTransform>::value,
                  "MarkerInstanceGroup::TransformType must alias MarkerTransform");
}

} // namespace

int main() {
    RunFieldDefaultChecks();
    RunTransformTypeAliasChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

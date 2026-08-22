// Placement_Symmetry_PROC_Test.cpp — acceptance test for symmetry clones and the basic
// AI-analyzability check (spawns reachable). Build with MSVC from src/proc:
//   cl /EHsc /std:c++17 /O2 Placement_Symmetry_PROC_Test.cpp Placement_PROC.cpp
//      Placement_Hash_PROC.cpp Placement_Rules_PROC.cpp Placement_Fields_PROC.cpp
//      Placement_Metrics_PROC.cpp Placement_Scatter_PROC.cpp Placement_Accept_PROC.cpp
//      Placement_Emit_PROC.cpp Placement_Gpu_PROC.cpp ..\sys\GpuResource_Program_SYS.cpp
//      ..\sys\GpuResource_Buffer_SYS.cpp ..\sys\GpuResource_ProgramParts_SYS.cpp
//      ..\sys\GpuGlFunctions_SYS.cpp opengl32.lib
#include "Placement_PROC.h"
#include "Placement_RuleAppend_PROC.h"
#include "Placement_RuleBuild_PROC.h"
#include "Placement_Test_Terrain.h"
#include <cstdio>
#include <cmath>
#include <chrono>
#include <vector>

using namespace SanmapGen;

static int failures = 0;
static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failures; }
}

// Minimal rule for the flat-seed/hash unit tests below: only the enable/hide gate matters --
// AppendMarkerRules and ComputeParameterHash are exercised directly, no terrain/Scatter pass.
static Params::MarkerRule MakeFlatTestRule(bool bEnabled, bool bHidden) {
    Params::MarkerRule rule;
    rule.bEnabled = bEnabled;
    rule.bHidden  = bHidden;
    rule.transform = PlacementTest::MakeTransform("m002", 1.0f, 1.0f);
    return rule;
}

static Params::MapRecipe MakeSymmetricRecipe(int symmetryMask) {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = PlacementTest::mapSize;
    recipe.geometry.seed = 4242u;
    recipe.geometry.terrainMaxHeight = 128.0f;
    recipe.globalSymmetryMask = symmetryMask;

    Params::MarkerRule spawnRule;
    spawnRule.category = Params::MarkerCategory::Spawn;
    spawnRule.count = 4;
    spawnRule.clearanceSpacing = 24.0f;
    spawnRule.mapEdgePadding = 10;
    spawnRule.minHeight = 0.4f; spawnRule.maxHeight = 0.6f;
    spawnRule.maxSlope = 10.0f;
    spawnRule.bRandomSelection = true;
    spawnRule.transform = PlacementTest::MakeTransform("m002", 1.0f, 1.0f);
    Params::MarkerRuleLayer spawnLayer;
    spawnLayer.symmetry.bSymmetryUseGlobal = true;    // STEP66: the triplet moved up onto the layer
    spawnLayer.rules.push_back(spawnRule);
    recipe.markerRuleLayers.push_back(spawnLayer);
    return recipe;
}

// Is there an instance at (targetX, targetY), within half a cell?
static bool HasInstanceAt(const Data::PlacementInstances& instances, float targetX, float targetY) {
    for (std::size_t index = 0; index < instances.Count(); ++index) {
        const float deltaX = instances.positionX[index] - targetX;
        const float deltaY = instances.positionZ[index] - targetY;
        if (deltaX * deltaX + deltaY * deltaY < 0.25f) return true;
    }
    return false;
}

// The basic AI-analyzability probe (AI_HOSTCLIENT_SPEC §A): flood-fill the pathable region
// (slope under the limit, above water) from the first spawn and require every other spawn to be
// inside it — a map that seals a spawn off is not playable. Full validation is a later work-order.
static bool AllSpawnsReachable(const Data::PlacementInstances& markers,
                               const Data::MapFields& fields, float maxSlopeDegrees) {
    if (markers.Count() == 0) return false;
    const int side = PlacementTest::vertexSize;
    const float tangent = std::tan(maxSlopeDegrees * 3.14159265f / 180.0f);
    const float gradientLimitSquared = tangent * tangent;
    std::vector<unsigned char> bVisited(static_cast<std::size_t>(side) * side, 0u);
    std::vector<int> stack;
    const int startX = static_cast<int>(markers.positionX[0] + 0.5f);
    const int startY = static_cast<int>(markers.positionZ[0] + 0.5f);
    stack.push_back(startY * side + startX);
    bVisited[startY * side + startX] = 1u;
    while (!stack.empty()) {
        const int cell = stack.back(); stack.pop_back();
        const int cellX = cell % side, cellY = cell / side;
        const int offsetsX[4] = { 1, -1, 0, 0 };
        const int offsetsY[4] = { 0, 0, 1, -1 };
        for (int direction = 0; direction < 4; ++direction) {
            const int nextX = cellX + offsetsX[direction], nextY = cellY + offsetsY[direction];
            if (nextX < 0 || nextY < 0 || nextX >= side || nextY >= side) continue;
            const int nextCell = nextY * side + nextX;
            if (bVisited[nextCell] != 0u) continue;
            const float slopeGradient = fields.slope.Get(nextX, nextY);   // the Mask stage's bake
            if (slopeGradient * slopeGradient > gradientLimitSquared) continue;
            bVisited[nextCell] = 1u;
            stack.push_back(nextCell);
        }
    }
    for (std::size_t index = 0; index < markers.Count(); ++index) {
        const int cellX = static_cast<int>(markers.positionX[index] + 0.5f);
        const int cellY = static_cast<int>(markers.positionZ[index] + 0.5f);
        if (bVisited[static_cast<std::size_t>(cellY) * side + cellX] == 0u) return false;
    }
    return true;
}

int main() {
    Data::MapFields fields;
    PlacementTest::BuildTestFields(fields);
    const float extent = static_cast<float>(PlacementTest::vertexSize - 1);

    // --- four-fold symmetry: mirror across both ground axes.
    Params::MapRecipe recipe = MakeSymmetricRecipe(Params::SymmetryAxis::MirrorAcrossX
                                                 | Params::SymmetryAxis::MirrorAcrossZ);
    Data::PlacementResults results;
    Proc::PlacementStage stage(recipe, fields, results);
    stage.Run();
    const Data::PlacementInstances& markers = results.markers;
    std::printf("symmetric markers=%zu\n", markers.Count());
    Check(markers.Count() == 4, "mirror-X|Z produces a 4-member orbit");

    bool bOneSymmetryGroup = markers.Count() > 0;
    for (std::size_t index = 1; index < markers.Count(); ++index)
        if (markers.symmetryIdentifier[index] != markers.symmetryIdentifier[0]) bOneSymmetryGroup = false;
    Check(bOneSymmetryGroup, "clones share one symmetry id");

    bool bClosedUnderMirror = markers.Count() > 0;
    for (std::size_t index = 0; index < markers.Count(); ++index) {
        const float positionX = markers.positionX[index], positionY = markers.positionZ[index];
        if (!HasInstanceAt(markers, extent - positionX, positionY)) bClosedUnderMirror = false;
        if (!HasInstanceAt(markers, positionX, extent - positionY)) bClosedUnderMirror = false;
    }
    Check(bClosedUnderMirror, "clone positions are exact mirrors");

    // A mirrored yaw is pi - yaw, i.e. the quaternion's y and w swap. Orbit order is
    // [source, mirrorX(source), ...], so entry 1 is entry 0 mirrored across X.
    if (markers.Count() == 4) {
        Check(std::fabs(markers.rotationY[1] - markers.rotationW[0]) < 1e-5f
              && std::fabs(markers.rotationW[1] - markers.rotationY[0]) < 1e-5f,
              "clone rotation is the mirrored rotation");
        Check(std::fabs(markers.positionY[1] - markers.positionY[0]) < 1e-4f,
              "clones sit at the same height on symmetric terrain");
    }

    // --- basic AI-analyzability: every spawn is in one connected pathable region.
    Check(AllSpawnsReachable(markers, fields, 20.0f), "all spawns are mutually reachable");

    // --- point symmetry: a 2-member orbit, so a count of 4 yields two groups.
    Data::PlacementResults pointResults;
    Params::MapRecipe pointRecipe = MakeSymmetricRecipe(Params::SymmetryAxis::RotateHalfTurn);
    Proc::PlacementStage pointStage(pointRecipe, fields, pointResults);
    pointStage.Run();
    std::printf("half-turn markers=%zu\n", pointResults.markers.Count());
    Check(pointResults.markers.Count() == 4, "half-turn symmetry fills the requested count");
    bool bClosedUnderRotation = pointResults.markers.Count() > 0;
    for (std::size_t index = 0; index < pointResults.markers.Count(); ++index)
        if (!HasInstanceAt(pointResults.markers, extent - pointResults.markers.positionX[index],
                           extent - pointResults.markers.positionZ[index])) bClosedUnderRotation = false;
    Check(bClosedUnderRotation, "half-turn clones are exact point reflections");
    Check(AllSpawnsReachable(pointResults.markers, fields, 20.0f), "half-turn spawns reachable");

    // ============================================================================================
    // STEP23_RadialSymmetryOrbit_PROC — AppendRadialTurns, the real N-way rotation orbit generator.
    // Direct, IO-bypassing calls into BuildSymmetryOrbit/AppendRadialTurns, mirroring how the
    // dormancy test this block replaces called Proc::BuildSymmetryOrbit directly.
    // ============================================================================================
    const float radialSourceX = 80.0f, radialSourceY = 60.0f;   // a non-center position
    const float radialCenter = extent * 0.5f;

    // --- Acceptance test 1: Radial(N) alone produces exactly N orbit members, each at the
    // correct rotated angle from the source, for two distinct N (a 4-way and a 12-way pinwheel).
    // Spot-checked via distance-from-center (unchanged by a pure rotation) and angular delta, not
    // just count.
    for (int turnCount : { 4, 12 }) {
        Proc::SymmetryOrbitPoint orbit[Params::symmetryOrbitMaximum];
        const int orbitCount = Proc::BuildSymmetryOrbit(Params::SymmetryAxis::Radial, turnCount, extent,
                                                         radialSourceX, radialSourceY, 0.01f,
                                                         orbit, Params::symmetryOrbitMaximum);
        char label[128];
        std::snprintf(label, sizeof(label), "Radial(%d) alone produces exactly %d orbit members",
                     turnCount, turnCount);
        Check(orbitCount == turnCount, label);

        const float sourceOffsetX = radialSourceX - radialCenter;
        const float sourceOffsetY = radialSourceY - radialCenter;
        const float sourceRadius = std::sqrt(sourceOffsetX * sourceOffsetX + sourceOffsetY * sourceOffsetY);
        bool bAllRadiiMatch = true, bAllAnglesCorrect = true;
        for (int index = 0; index < orbitCount; ++index) {
            const float pointOffsetX = orbit[index].positionX - radialCenter;
            const float pointOffsetY = orbit[index].positionY - radialCenter;
            const float radius = std::sqrt(pointOffsetX * pointOffsetX + pointOffsetY * pointOffsetY);
            if (std::fabs(radius - sourceRadius) > 0.02f) bAllRadiiMatch = false;
            const float angle = static_cast<float>(index) * (2.0f * 3.14159265f / static_cast<float>(turnCount));
            const float expectedX = radialCenter + (sourceOffsetX * std::cos(angle) - sourceOffsetY * std::sin(angle));
            const float expectedY = radialCenter + (sourceOffsetX * std::sin(angle) + sourceOffsetY * std::cos(angle));
            if (std::fabs(orbit[index].positionX - expectedX) > 0.02f
                || std::fabs(orbit[index].positionY - expectedY) > 0.02f) bAllAnglesCorrect = false;
        }
        std::snprintf(label, sizeof(label), "Radial(%d) members all sit at the source's radius from center",
                     turnCount);
        Check(bAllRadiiMatch, label);
        std::snprintf(label, sizeof(label), "Radial(%d) members land at their correctly rotated angle", turnCount);
        Check(bAllAnglesCorrect, label);
    }

    // --- Acceptance test 2: Radial(N) | MirrorAcrossX produces exactly 2N members, no missing or
    // duplicate pairs (Generator Expert's primary composition case).
    {
        const int turnCount = 6;
        Proc::SymmetryOrbitPoint orbit[Params::symmetryOrbitMaximum];
        const int orbitCount = Proc::BuildSymmetryOrbit(
            Params::SymmetryAxis::Radial | Params::SymmetryAxis::MirrorAcrossX, turnCount, extent,
            radialSourceX, radialSourceY, 0.01f, orbit, Params::symmetryOrbitMaximum);
        Check(orbitCount == 2 * turnCount, "Radial(6) | MirrorAcrossX produces a 12-member orbit");
        bool bEveryMemberHasMirrorPartner = true;
        for (int index = 0; index < orbitCount; ++index) {
            const float mirrorX = extent - orbit[index].positionX;
            bool bFoundPartner = false;
            for (int other = 0; other < orbitCount; ++other) {
                if (std::fabs(orbit[other].positionX - mirrorX) < 0.05f
                    && std::fabs(orbit[other].positionY - orbit[index].positionY) < 0.05f) {
                    bFoundPartner = true;
                    break;
                }
            }
            if (!bFoundPartner) bEveryMemberHasMirrorPartner = false;
        }
        Check(bEveryMemberHasMirrorPartner,
              "every Radial(6) | MirrorAcrossX member has its mirror-X partner in the orbit, no gaps");
    }

    // --- Acceptance test 3: Radial(N) | QuarterTurns — the double-rotation misconfiguration — is
    // defined behavior only (PLACEMENT_SCATTER_SPEC's ratified addendum: every set bit composes
    // independently; not this ticket's to forbid). No crash/corruption; epsilon-dedup applies; no
    // exact-count assertion required.
    {
        Proc::SymmetryOrbitPoint orbit[Params::symmetryOrbitMaximum];
        const int orbitCount = Proc::BuildSymmetryOrbit(
            Params::SymmetryAxis::Radial | Params::SymmetryAxis::QuarterTurns, 8, extent,
            radialSourceX, radialSourceY, 0.01f, orbit, Params::symmetryOrbitMaximum);
        Check(orbitCount > 0 && orbitCount <= Params::symmetryOrbitMaximum,
              "Radial(8) | QuarterTurns is defined behavior: no crash, stays within the buffer");
    }

    // --- Acceptance test 4: a candidate exactly at map center with Radial(12) set collapses to
    // orbit count 1 via existing epsilon-dedup (named regression test, not a defect — every
    // rotation of the center point IS the center point).
    {
        Proc::SymmetryOrbitPoint orbit[Params::symmetryOrbitMaximum];
        const int orbitCount = Proc::BuildSymmetryOrbit(Params::SymmetryAxis::Radial, 12, extent,
                                                         radialCenter, radialCenter, 0.01f, orbit,
                                                         Params::symmetryOrbitMaximum);
        Check(orbitCount == 1,
              "a candidate exactly at map center with Radial(12) collapses to orbit count 1");
    }

    // --- Acceptance test 5: ruling 6's own "16 x N = 192" arithmetic (for N=12) turns out to be an
    // OVERCOUNT once actually measured — flagged back, not silently worked around (see the coder's
    // report). MirrorAcrossX composed with MirrorAcrossZ IS a 180-degree rotation about the same
    // center (not an independent reflection), and QuarterTurns' own 90-degree rotation subgroup is
    // entirely CONTAINED in Radial(12)'s rotation subgroup (4 divides 12) — so this exact
    // combination's true maximum orbit at N=12 is the dihedral group of order 2*lcm(4,12) = 24, not
    // 192 (confirmed empirically below). The buffer-sizing conclusion (256) is unaffected by this
    // correction — the true worst case in the ratified [2, 12] range is smaller still, reached at an
    // N COPRIME with 4 (avoiding the QuarterTurns/Radial rotation-subgroup overlap entirely): N=11,
    // the largest such N <= 12, giving 2*lcm(4,11) = 88 members — comfortably inside 256, but
    // meaningfully larger than the N=12 case ruling 6 itself pointed at, and the number this
    // acceptance test actually needs to prove the widened cap covers.
    const int worstCaseMask = Params::SymmetryAxis::Radial | Params::SymmetryAxis::MirrorAcrossX
                             | Params::SymmetryAxis::MirrorAcrossZ | Params::SymmetryAxis::QuarterTurns;
    {
        Proc::SymmetryOrbitPoint orbit[Params::symmetryOrbitMaximum];
        const int orbitCount = Proc::BuildSymmetryOrbit(worstCaseMask, 12, extent,
                                                         radialSourceX, radialSourceY, 0.01f,
                                                         orbit, Params::symmetryOrbitMaximum);
        std::printf("Radial(12)|MirrorAcrossX|MirrorAcrossZ|QuarterTurns = %d members (ruling 6 "
                    "predicted 192; the true group-theoretic maximum at N=12 is 24, since "
                    "QuarterTurns' C4 rotation subgroup is already a subset of Radial(12)'s C12)\n",
                    orbitCount);
        Check(orbitCount == 24,
              "Radial(12)|MirrorAcrossX|MirrorAcrossZ|QuarterTurns fills the mathematically correct "
              "24-member dihedral orbit (order 2 * lcm(4,12)), no truncation");

        // The TRUE worst case inside the ratified [2, 12] range: an N coprime with 4 avoids the
        // QuarterTurns/Radial rotation-subgroup overlap entirely. N=11 is the largest such N <= 12.
        Proc::SymmetryOrbitPoint trueWorstOrbit[Params::symmetryOrbitMaximum];
        const int trueWorstCount = Proc::BuildSymmetryOrbit(worstCaseMask, 11, extent,
                                                             radialSourceX, radialSourceY, 0.01f,
                                                             trueWorstOrbit, Params::symmetryOrbitMaximum);
        std::printf("Radial(11)|MirrorAcrossX|MirrorAcrossZ|QuarterTurns (N coprime with 4, the "
                    "TRUE worst case in [2,12]) = %d members\n", trueWorstCount);
        Check(trueWorstCount == 88,
              "the true worst case in the ratified [2, 12] range (N=11) fills exactly the "
              "88-member orbit, comfortably inside the 256 buffer with no truncation");
        Check(Params::symmetryOrbitMaximum == 256, "symmetryOrbitMaximum was raised to 256");
    }

    // --- Acceptance test 7: ruling 2's PROC-level defenses, exercised directly against
    // AppendRadialTurns/BuildSymmetryOrbit (not only via the IO clamp, which a broken PROC
    // implementation could pass while still being exploitable by any direct caller).
    {
        Proc::SymmetryOrbitPoint orbit[Params::symmetryOrbitMaximum];
        const int orbitCountZero = Proc::BuildSymmetryOrbit(Params::SymmetryAxis::Radial, 0, extent,
                                                             radialSourceX, radialSourceY, 0.01f,
                                                             orbit, Params::symmetryOrbitMaximum);
        Check(orbitCountZero == 1, "radialSymmetryRepeatCount = 0 produces zero additional clones "
                                    "(the floor engages with no special-case branch)");
        const int orbitCountOne = Proc::BuildSymmetryOrbit(Params::SymmetryAxis::Radial, 1, extent,
                                                            radialSourceX, radialSourceY, 0.01f,
                                                            orbit, Params::symmetryOrbitMaximum);
        Check(orbitCountOne == 1, "radialSymmetryRepeatCount = 1 produces zero additional clones "
                                   "(the floor engages with no special-case branch)");

        // An absurdly large count against a SMALL maximumPoints must not overrun the buffer: the
        // internal clamp against maximumPoints engages before the append loop runs.
        constexpr int smallBufferSize = 8;
        Proc::SymmetryOrbitPoint smallBuffer[smallBufferSize];
        const int smallBufferCount = Proc::BuildSymmetryOrbit(Params::SymmetryAxis::Radial, 100000, extent,
                                                               radialSourceX, radialSourceY, 0.01f,
                                                               smallBuffer, smallBufferSize);
        Check(smallBufferCount <= smallBufferSize,
              "an absurdly large radialSymmetryRepeatCount (100000) against a small maximumPoints "
              "does not overrun the buffer");
        // Direct AppendRadialTurns call — the same defense, one layer lower.
        Proc::SymmetryOrbitPoint appendBuffer[smallBufferSize];
        appendBuffer[0].positionX = radialSourceX;
        appendBuffer[0].positionY = radialSourceY;
        const int appendCount = Proc::SymmetryDetail::AppendRadialTurns(appendBuffer, 1, smallBufferSize,
                                                                        100000, extent, 0.01f);
        Check(appendCount <= smallBufferSize,
              "AppendRadialTurns itself clamps turnCount against maximumPoints before the loop runs");
    }

    // --- Acceptance test 8: end-to-end (not just unit-level) local-override coverage. Runs the
    // FULL pipeline (BuildRuleConfigurations -> AcceptCandidates -> BuildSymmetryOrbit) with a
    // layer's bSymmetryUseGlobal = false and a layer-local radialSymmetryRepeatCount that DIFFERS
    // from recipe.radialSymmetryRepeatCount — the exact bug STEP16 flagged: "local override ...
    // silently inherit the global N". A unit test of ResolveRadialSymmetryRepeatCount alone cannot
    // catch a broken array-threading implementation between BuildRuleConfigurations and
    // AcceptCandidates.
    {
        Params::MapRecipe overrideRecipe;
        overrideRecipe.geometry.mapSize = PlacementTest::mapSize;
        overrideRecipe.geometry.seed = 777u;
        overrideRecipe.geometry.terrainMaxHeight = 128.0f;
        overrideRecipe.globalSymmetryMask = Params::SymmetryAxis::Radial;
        overrideRecipe.radialSymmetryRepeatCount = 4;    // the GLOBAL count

        Params::MarkerRule overrideRule;
        overrideRule.category = Params::MarkerCategory::Spawn;
        overrideRule.count = 100;                        // high enough that the orbit is never capped
        overrideRule.clearanceSpacing = 4.0f;
        overrideRule.mapEdgePadding = 10;
        overrideRule.minHeight = 0.4f; overrideRule.maxHeight = 0.6f;
        overrideRule.maxSlope = 10.0f;
        overrideRule.bRandomSelection = true;
        overrideRule.transform = PlacementTest::MakeTransform("m002", 1.0f, 1.0f);

        Params::MarkerRuleLayer overrideLayer;
        overrideLayer.symmetry.bSymmetryUseGlobal = false;   // LOCAL override, not the global mask...
        overrideLayer.symmetry.symmetryMask = Params::SymmetryAxis::Radial;
        overrideLayer.symmetry.radialSymmetryRepeatCount = 9; // ...and a LOCAL count distinct from global
        overrideLayer.rules.push_back(overrideRule);
        overrideRecipe.markerRuleLayers.push_back(overrideLayer);

        Data::PlacementResults overrideResults;
        Proc::PlacementStage overrideStage(overrideRecipe, fields, overrideResults);
        overrideStage.Run();
        // Every accepted orbit must be a multiple of the LOCAL count (9), not the global one (4):
        // markers.Count() is the sum of complete orbits, so it must divide evenly by 9 and (since
        // 9 does not divide any positive multiple of 4 below it without also being a multiple of
        // 4) NOT be explainable as a run of 4-member orbits alone.
        std::printf("local-override markers=%zu (local N=9, global N=4)\n", overrideResults.markers.Count());
        Check(overrideResults.markers.Count() > 0 && overrideResults.markers.Count() % 9 == 0,
              "the placed orbit reflects the rule's LOCAL radialSymmetryRepeatCount (9), not the "
              "recipe's global one (4)");
    }

    // --- Acceptance test 9: throughput check, typical vs. worst case (Compute Optimization
    // Expert's explicit ask — AppendPoint's O(n^2) dedup is NOT redesigned in this ticket; this
    // measures whether it needs a follow-up). Simple wall-clock comparison, not a formal benchmark
    // harness.
    {
        Params::MapRecipe typicalRecipe = MakeSymmetricRecipe(Params::SymmetryAxis::Radial);
        typicalRecipe.radialSymmetryRepeatCount = 4;
        typicalRecipe.markerRuleLayers[0].rules[0].count = 40;
        typicalRecipe.markerRuleLayers[0].rules[0].clearanceSpacing = 4.0f;

        Params::MapRecipe worstRecipe = MakeSymmetricRecipe(worstCaseMask);
        worstRecipe.radialSymmetryRepeatCount = 12;
        worstRecipe.markerRuleLayers[0].rules[0].count = 40;
        worstRecipe.markerRuleLayers[0].rules[0].clearanceSpacing = 4.0f;

        Data::PlacementResults typicalResults, worstResults;
        Proc::PlacementStage typicalStage(typicalRecipe, fields, typicalResults);
        Proc::PlacementStage worstStage(worstRecipe, fields, worstResults);

        const auto typicalStart = std::chrono::steady_clock::now();
        typicalStage.Run();
        const auto typicalEnd = std::chrono::steady_clock::now();
        const auto worstStart = std::chrono::steady_clock::now();
        worstStage.Run();
        const auto worstEnd = std::chrono::steady_clock::now();

        const double typicalMillis = std::chrono::duration<double, std::milli>(typicalEnd - typicalStart).count();
        const double worstMillis   = std::chrono::duration<double, std::milli>(worstEnd - worstStart).count();
        std::printf("throughput: typical(Radial(4))=%.3fms worst(Radial(12)|MirrorX|MirrorZ|QuarterTurns)=%.3fms "
                    "ratio=%.1fx\n", typicalMillis, worstMillis,
                    typicalMillis > 0.0 ? worstMillis / typicalMillis : 0.0);
        Check(worstMillis < 5000.0,
              "worst-case symmetry throughput stays well under 5s (record the actual ratio above; "
              "flag a follow-up ticket if it is severe, do not hand-wave it)");
    }

    // --- Acceptance test 10: ResolveRadialSymmetryRepeatCount unit-level coverage, mirroring
    // ResolveSymmetryMask's own contract — bUseGlobal = true uses the GLOBAL count, not the
    // rule's own distinct local one.
    Check(Proc::ResolveRadialSymmetryRepeatCount(true, 9, 4) == 4,
          "ResolveRadialSymmetryRepeatCount(bUseGlobal=true) returns the global count, not the "
          "rule's own distinct local count");
    Check(Proc::ResolveRadialSymmetryRepeatCount(false, 9, 4) == 9,
          "ResolveRadialSymmetryRepeatCount(bUseGlobal=false) returns the rule's own local count");

    // ============================================================================================
    // STEP79_MarkerRuleLayerProcConsumer_PROC — the two-level markerRuleLayers walk. Exercises
    // Proc::AppendMarkerRules / PlacementStage::ComputeParameterHash directly (no terrain, no
    // Scatter/Accept pass needed for these unit-level determinism/hash checks).
    // ============================================================================================

    // --- Acceptance test 11: THE FLAT SEED COUNTER — the determinism guard. ruleIndex/ruleSeed
    // must keep today's exact flat numbering across the layer boundary, including through a
    // fully-suppressed layer.
    {
        Proc::PlacementConstants constants;
        Params::MapRecipe recipe;
        recipe.geometry.seed = 999u;

        // Five rules, flat positions 1 and 3 suppressed (own bEnabled=false, bHidden=false),
        // distributed 2 / 1 / 2 across three enabled layers.
        Params::MarkerRuleLayer layerA, layerB, layerC;
        layerA.rules.push_back(MakeFlatTestRule(true, false));    // flat 0 -- survives
        layerA.rules.push_back(MakeFlatTestRule(false, false));   // flat 1 -- suppressed (rule gate)
        layerB.rules.push_back(MakeFlatTestRule(true, false));    // flat 2 -- survives
        layerC.rules.push_back(MakeFlatTestRule(false, false));   // flat 3 -- suppressed
        layerC.rules.push_back(MakeFlatTestRule(true, false));    // flat 4 -- survives
        recipe.markerRuleLayers.push_back(layerA);
        recipe.markerRuleLayers.push_back(layerB);
        recipe.markerRuleLayers.push_back(layerC);

        std::vector<Proc::ScatterRuleConfiguration> configurations;
        std::vector<Data::TemplateIdentifier> identifiers;
        std::vector<int> radialCounts;
        Proc::AppendMarkerRules(constants, recipe, configurations, identifiers, radialCounts);

        Check(configurations.size() == 3, "3 of 5 rules survive suppression");
        bool bIndicesCorrect = configurations.size() == 3
            && configurations[0].ruleIndex == 0 && configurations[1].ruleIndex == 2
            && configurations[2].ruleIndex == 4;
        Check(bIndicesCorrect, "surviving configurations carry ruleIndex 0, 2, 4 -- not 0, 1, 2");

        bool bSeedsCorrect = configurations.size() == 3
            && configurations[0].ruleSeed == Proc::MakeRuleSeed(constants, recipe.geometry.seed, 0, 0)
            && configurations[1].ruleSeed == Proc::MakeRuleSeed(constants, recipe.geometry.seed, 0, 2)
            && configurations[2].ruleSeed == Proc::MakeRuleSeed(constants, recipe.geometry.seed, 0, 4);
        Check(bSeedsCorrect, "ruleSeed values match MakeRuleSeed(constants, seed, 0, {0,2,4}) exactly");

        // A fourth, fully DISABLED layer holding 2 rules, inserted between layerB and layerC: the
        // counter must advance through it too, so layerC's surviving rule (flat 4 without the
        // gap) is now seeded 6, not 4 -- the counter did not reset or skip the suppressed layer.
        Params::MarkerRuleLayer gapLayer;
        gapLayer.bEnabled = false;
        gapLayer.bHidden  = false;
        gapLayer.rules.push_back(MakeFlatTestRule(true, false));
        gapLayer.rules.push_back(MakeFlatTestRule(true, false));

        Params::MapRecipe recipeWithGap;
        recipeWithGap.geometry.seed = 999u;
        recipeWithGap.markerRuleLayers.push_back(layerA);
        recipeWithGap.markerRuleLayers.push_back(layerB);
        recipeWithGap.markerRuleLayers.push_back(gapLayer);   // inserted in the middle
        recipeWithGap.markerRuleLayers.push_back(layerC);

        std::vector<Proc::ScatterRuleConfiguration> gapConfigurations;
        std::vector<Data::TemplateIdentifier> gapIdentifiers;
        std::vector<int> gapRadialCounts;
        Proc::AppendMarkerRules(constants, recipeWithGap, gapConfigurations, gapIdentifiers, gapRadialCounts);

        Check(gapConfigurations.size() == 3, "3 of 7 rules survive with the disabled layer inserted");
        bool bGapIndexCorrect = gapConfigurations.size() == 3 && gapConfigurations[2].ruleIndex == 6;
        Check(bGapIndexCorrect, "the counter advances through a fully-suppressed layer inserted "
                                "mid-sequence: the surviving rule after it is seeded 6 (shifted by "
                                "the 2-rule gap), not 4");
    }

    // --- Acceptance test 12: suppression matrix. The layer/rule OR-combination, and the
    // hidden-still-generates semantic preserved at BOTH tiers.
    {
        Proc::PlacementConstants constants;
        Params::MapRecipe recipe;
        recipe.geometry.seed = 1234u;

        Params::MarkerRuleLayer disabledVisibleLayer;   // layer disabled, NOT hidden: a real gate
        disabledVisibleLayer.bEnabled = false;
        disabledVisibleLayer.bHidden  = false;
        disabledVisibleLayer.rules.push_back(MakeFlatTestRule(true, false));  // rule itself enabled

        Params::MarkerRuleLayer enabledLayer;           // layer enabled: the rule tier governs
        enabledLayer.rules.push_back(MakeFlatTestRule(false, false)); // disabled, NOT hidden: suppressed
        enabledLayer.rules.push_back(MakeFlatTestRule(false, true));  // disabled but HIDDEN: still generates

        recipe.markerRuleLayers.push_back(disabledVisibleLayer);
        recipe.markerRuleLayers.push_back(enabledLayer);

        std::vector<Proc::ScatterRuleConfiguration> configurations;
        std::vector<Data::TemplateIdentifier> identifiers;
        std::vector<int> radialCounts;
        Proc::AppendMarkerRules(constants, recipe, configurations, identifiers, radialCounts);

        Check(configurations.size() == 1, "only the disabled-but-hidden rule survives suppression");
        bool bHiddenFlagSet = configurations.size() == 1
            && (configurations[0].selectionFlags & Proc::ScatterSelectionFlag::Hidden) != 0;
        Check(bHiddenFlagSet, "the surviving disabled-but-hidden rule still carries "
                              "ScatterSelectionFlag::Hidden");
    }

    // --- Acceptance test 13: dirty-hash reactivity, the silent-failure guard (Placement_Hash_PROC.cpp
    // fails silently, not at compile time, if not migrated in lockstep). Flip ONE symmetry/enable/
    // hidden field at a time and require ComputeParameterHash() to react every time; also confirm
    // layer STRUCTURE (how the same rules are grouped) is itself a hash input.
    {
        auto MakeHashRecipe = []() {
            Params::MapRecipe recipe;
            recipe.geometry.seed = 55u;
            Params::MarkerRuleLayer layer;
            layer.rules.push_back(MakeFlatTestRule(true, false));
            layer.rules.push_back(MakeFlatTestRule(true, false));
            recipe.markerRuleLayers.push_back(layer);
            return recipe;
        };
        auto HashOf = [&](const Params::MapRecipe& recipe) {
            Data::PlacementResults results;
            Proc::PlacementStage stage(recipe, fields, results);
            return stage.ComputeParameterHash();
        };

        const std::size_t baselineHash = HashOf(MakeHashRecipe());

        Params::MapRecipe changedMask = MakeHashRecipe();
        changedMask.markerRuleLayers[0].symmetry.symmetryMask = Params::SymmetryAxis::Radial;
        Check(HashOf(changedMask) != baselineHash, "flipping symmetry.symmetryMask dirties the hash");

        Params::MapRecipe changedUseGlobal = MakeHashRecipe();
        changedUseGlobal.markerRuleLayers[0].symmetry.bSymmetryUseGlobal = false;
        Check(HashOf(changedUseGlobal) != baselineHash, "flipping symmetry.bSymmetryUseGlobal dirties the hash");

        Params::MapRecipe changedRadialCount = MakeHashRecipe();
        changedRadialCount.markerRuleLayers[0].symmetry.radialSymmetryRepeatCount = 9;
        Check(HashOf(changedRadialCount) != baselineHash, "flipping symmetry.radialSymmetryRepeatCount "
                                                          "dirties the hash (closes a pre-existing gap)");

        Params::MapRecipe changedLayerEnabled = MakeHashRecipe();
        changedLayerEnabled.markerRuleLayers[0].bEnabled = false;
        Check(HashOf(changedLayerEnabled) != baselineHash, "flipping layer.bEnabled dirties the hash");

        Params::MapRecipe changedLayerHidden = MakeHashRecipe();
        changedLayerHidden.markerRuleLayers[0].bHidden = true;
        Check(HashOf(changedLayerHidden) != baselineHash, "flipping layer.bHidden dirties the hash");

        // The same rules distributed 2/1 vs 1/2 across layers (identical symmetry settings on
        // every layer) must hash differently -- layer structure is a real input.
        Params::MapRecipe splitTwoOne;
        splitTwoOne.geometry.seed = 77u;
        Params::MarkerRuleLayer twoRuleLayer, oneRuleLayerA;
        twoRuleLayer.rules.push_back(MakeFlatTestRule(true, false));
        twoRuleLayer.rules.push_back(MakeFlatTestRule(true, false));
        oneRuleLayerA.rules.push_back(MakeFlatTestRule(true, false));
        splitTwoOne.markerRuleLayers.push_back(twoRuleLayer);
        splitTwoOne.markerRuleLayers.push_back(oneRuleLayerA);

        Params::MapRecipe splitOneTwo;
        splitOneTwo.geometry.seed = 77u;
        Params::MarkerRuleLayer oneRuleLayerB, twoRuleLayerB;
        oneRuleLayerB.rules.push_back(MakeFlatTestRule(true, false));
        twoRuleLayerB.rules.push_back(MakeFlatTestRule(true, false));
        twoRuleLayerB.rules.push_back(MakeFlatTestRule(true, false));
        splitOneTwo.markerRuleLayers.push_back(oneRuleLayerB);
        splitOneTwo.markerRuleLayers.push_back(twoRuleLayerB);

        Check(HashOf(splitTwoOne) != HashOf(splitOneTwo), "the same rules distributed 2/1 vs 1/2 "
                                                          "across layers hash differently");
    }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}

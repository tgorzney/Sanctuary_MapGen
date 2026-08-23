// MapCanvas_IconLayer_MicrobenchmarkScenarios_UI_Test.cpp — STEP59: the synthetic-instance scene
// builder for the overlay vertex-gen microbenchmark, split out of
// MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp to stay inside Constitution §1.5's size ceilings
// (that file's own §1 comment flags this split as the coder's implementation call). Test-only.
//
// Positions N synthetic Props instances either entirely inside the fixture's real view-world rect
// (the "0%-culled" scenario — everything reaches the vertex-write stage) or across a square ~20x
// that rect's area, centered on it (the "~5%-visible" scenario — a uniformly-random point lands
// inside the view rect roughly 5% of the time). Seeded deterministically (std::mt19937) so runs are
// reproducible run-to-run, per the work-order's own instruction.
//
// Instances are round-robined across several synthetic atlas pages/templateIdentifiers rather than
// one — not an arbitrary choice: STEP53's real FlushIconLayerBucket (MapCanvas_IconLayer_Draw_UI.cpp)
// issues exactly ONE ImDrawList::PrimReserve() per atlas-page bucket (a deliberate bulk-write
// optimization, not a per-quad call), and this project's ImDrawIdx is the vendored imgui default
// (16-bit, imconfig.h's 32-bit override is commented out) — a single bucket at or above 16,384 quads
// would request >= 65,536 vertices in ONE PrimReserve call and trip imgui's own debug-only 16-bit-
// index assert (imgui_draw.cpp's AddDrawListToDrawDataEx) when Render() later inspects it. Spreading
// N instances across enough pages keeps every individual bucket safely under that per-call limit —
// the same shape any real multi-template overlay naturally has — without ever touching
// MapCanvas_IconLayer_Draw_UI.cpp itself. See the MicrobenchmarkFrameOps sibling's BeginHeadlessFrame()
// for the other half of this: enabling the same ImGuiBackendFlags_RendererHasVtxOffset flag STEP53's
// real production renderer backend (imgui_impl_opengl3.cpp, desktop GL 3.2+) already sets.
#include "MapCanvas_IconLayer_TestFixture_UI.h"
#include <imgui.h>
#include <random>
#include <cmath>
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// Safely under 65536/4 = 16384 — see this file's header comment for why this cap exists.
constexpr int kMicrobenchmarkMaxQuadsPerAtlasPage = 8000;

int ComputeSyntheticPageCount(int instanceCount) {
    const int pageCount = (instanceCount + kMicrobenchmarkMaxQuadsPerAtlasPage - 1)
                         / kMicrobenchmarkMaxQuadsPerAtlasPage;
    return pageCount > 0 ? pageCount : 1;
}

// One distinct templateIdentifier + atlas page + icon per synthetic page, all resolving via the
// SAME real IconAtlasPairingLookup/IconAtlasManifest STEP53 consumes — never a hand-rolled stand-in.
std::vector<std::string> SeedSyntheticAtlasPages(IconLayerTestFixture& fixture, int pageCount) {
    std::vector<std::string> templateIdentifiers(static_cast<std::size_t>(pageCount));
    for (int page = 0; page < pageCount; ++page) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "ic%05d", page);   // 7 chars — exactly tpId's limit
        templateIdentifiers[static_cast<std::size_t>(page)] = buffer;
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest,
                       templateIdentifiers[static_cast<std::size_t>(page)], /*iconId=*/page, /*atlasPage=*/page);
    }
    return templateIdentifiers;
}

} // namespace

// bZeroCulledScenario: true = every instance lands inside the real view-world rect (0% culled);
// false = instances spread over a ~20x-larger square centered on it (~5% survive the cull).
void BuildMicrobenchmarkScene(IconLayerTestFixture& fixture, int instanceCount, bool bZeroCulledScenario,
                              unsigned int randomSeed) {
    const float regionSidePixels = 256.0f;   // mirrors IconLayerTestFixture's own ctor/Input() constant
    const ViewWorldRect_UI viewRect = ComputeViewWorldRect(*fixture.composite, fixture.view, regionSidePixels);
    const float viewWidth = viewRect.highWorldX - viewRect.lowWorldX;
    const float viewDepth = viewRect.highWorldZ - viewRect.lowWorldZ;
    const float viewCenterX = (viewRect.lowWorldX + viewRect.highWorldX) * 0.5f;
    const float viewCenterZ = (viewRect.lowWorldZ + viewRect.highWorldZ) * 0.5f;
    const float spreadSide = std::sqrt((viewWidth * viewDepth) / 0.05f);   // area ratio -> ~5% land inside

    std::mt19937 rng(randomSeed);
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);

    const int pageCount = ComputeSyntheticPageCount(instanceCount);
    const std::vector<std::string> templateIdentifiers = SeedSyntheticAtlasPages(fixture, pageCount);

    fixture.placements.props.Reserve(static_cast<std::size_t>(instanceCount));
    for (int index = 0; index < instanceCount; ++index) {
        float worldX, worldZ;
        if (bZeroCulledScenario) {
            worldX = viewRect.lowWorldX + unitDistribution(rng) * viewWidth;
            worldZ = viewRect.lowWorldZ + unitDistribution(rng) * viewDepth;
        } else {
            worldX = viewCenterX + (unitDistribution(rng) - 0.5f) * spreadSide;
            worldZ = viewCenterZ + (unitDistribution(rng) - 0.5f) * spreadSide;
        }
        const std::string& templateIdentifier = templateIdentifiers[static_cast<std::size_t>(index % pageCount)];
        AppendPropInstance(fixture.placements, worldX, worldZ, /*ruleIndex=*/0, templateIdentifier.c_str());
    }
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(),
                                        static_cast<std::int32_t>(fixture.placements.props.Count()), 1);

    OverlayLayer_UI layer;
    layer.domainKind = OverlayDomainKind_UI::Props;
    layer.thumbnailLodThresholdPixels = 0.01f;   // guarantees thumbnail-mode resolution, never a strategic miss
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);
}

// Operation 3 — the explicitly-rejected naive path (STEP53 §0 forbids this from ever appearing in
// MapCanvas_IconLayer_Draw_UI.cpp). Written ONLY here, a throwaway comparison, never touching
// production code. Returns the count actually drawn, for the apples-to-apples check against op2.
std::size_t DrawNaivePerInstanceImages(ImDrawList& drawList, const std::vector<OverlayVisibleInstance>& candidates) {
    for (const OverlayVisibleInstance& instance : candidates) {
        const float half = instance.screenSize * 0.5f;
        const ImVec2 topLeft(instance.screenCenterX - half, instance.screenCenterY - half);
        const ImVec2 bottomRight(instance.screenCenterX + half, instance.screenCenterY + half);
        const ImU32 tint = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, instance.tintAlpha));
        drawList.AddImage(static_cast<ImTextureID>(instance.textureIdentifier), topLeft, bottomRight,
                          ImVec2(instance.uvMinimumX, instance.uvMinimumY),
                          ImVec2(instance.uvMaximumX, instance.uvMaximumY), tint);
    }
    return candidates.size();
}

} // namespace Ui
} // namespace SanmapGen

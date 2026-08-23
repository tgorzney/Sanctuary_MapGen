// MapCanvas_IconLayer_MicrobenchmarkFrameOps_UI_Test.cpp — STEP59: the headless-imgui-frame harness
// (mirrors MapCanvas_Render_UI_Test.cpp's own ImGui::CreateContext()/NewFrame()/Render()/
// GetDrawData() technique) plus operations 2 and 3's timed measurements. Split out of
// MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp to stay inside Constitution §1.5's size ceilings —
// that file's own header comment names this split as the coder's implementation call, same as the
// MapCanvas_IconLayer_MicrobenchmarkScenarios_UI_Test.cpp split. Test-only.
#include "MapCanvas_IconLayer_DrawInternal_UI.h"
#include "MapCanvas_IconLayer_TestFixture_UI.h"
#include <chrono>
#include <cstdio>

namespace SanmapGen {
namespace Ui {

// MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp
struct ScenarioSpec { const char* label; bool bZeroCulled; };
// MapCanvas_IconLayer_MicrobenchmarkScenarios_UI_Test.cpp
std::size_t DrawNaivePerInstanceImages(ImDrawList& drawList, const std::vector<OverlayVisibleInstance>& candidates);

namespace {

double ElapsedMillis(const std::chrono::steady_clock::time_point& start,
                     const std::chrono::steady_clock::time_point& end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// Matches the real production renderer backend (imgui_impl_opengl3.cpp, desktop GL 3.2+): large
// meshes span multiple ImDrawCmds via ImDrawCmd::VtxOffset instead of overflowing 16-bit indices.
// Setting it here is the faithful production configuration, not a workaround invented for this
// benchmark — see the Scenarios sibling's header comment for why it matters at these N values
// (FlushIconLayerBucket issues one PrimReserve() per atlas-page bucket, not per quad).
constexpr unsigned long long kMicrobenchmarkFontAtlasIdentifier = 0xF0000002ull;
void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.DisplaySize = ImVec2(512.0f, 512.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kMicrobenchmarkFontAtlasIdentifier));
    ImGui::NewFrame();
}

ImDrawList& BeginBenchmarkFrame(const char* windowName) {
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(512.0f, 512.0f));
    ImGui::Begin(windowName);
    return *ImGui::GetWindowDrawList();
}

void EndBenchmarkFrame() {
    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();
}

} // namespace

// Operation 2 — STEP53's real §3 FlushIconLayerBucket-shaped bulk write (via the real FlushBuckets/
// BucketByAtlasPage), inside a live headless imgui frame. Clock starts after NewFrame() returns
// (BeginBenchmarkFrame) and stops before Render(), isolating the write loop from per-frame imgui
// overhead unrelated to this pass, per the work-order's own instruction.
void RunOperationTwoBulkWrite(const std::vector<OverlayVisibleInstance>& candidates, int instanceCount,
                              const ScenarioSpec& scenario) {
    const std::vector<AtlasPageBucket> buckets = BucketByAtlasPage(candidates);
    const int quadCount = static_cast<int>(candidates.size());

    ImDrawList& drawList = BeginBenchmarkFrame("IconLayerMicrobenchmarkBulkWindow");
    // Baseline BEFORE the timed call: ImGui::Begin() already wrote its own window-chrome vertices
    // (background rect/border) into this SAME draw list (GetWindowDrawList(), same technique
    // MapCanvas_IconLayer_Draw_UI_Test.cpp uses) -- subtracting the baseline isolates exactly what
    // FlushBuckets() itself emitted, an exact equality check rather than that test's own looser
    // ">=" (its smaller, fixed instance counts never needed to isolate the chrome overhead).
    int baselineIndexCount = 0;
    for (const ImDrawCmd& command : drawList.CmdBuffer) baselineIndexCount += static_cast<int>(command.ElemCount);
    const int baselineVertexCount = drawList.VtxBuffer.Size;

    const auto start = std::chrono::steady_clock::now();
    FlushBuckets(drawList, buckets);
    const auto end = std::chrono::steady_clock::now();

    int totalIndexCount = 0;
    for (const ImDrawCmd& command : drawList.CmdBuffer) totalIndexCount += static_cast<int>(command.ElemCount);
    const int vertexCount = drawList.VtxBuffer.Size - baselineVertexCount;
    const int indexCount = totalIndexCount - baselineIndexCount;
    EndBenchmarkFrame();

    std::printf("[op2 bulk PrimReserve/Write]    N=%7d scenario=%-12s elapsed=%9.3fms quadCount=%9d "
               "vtx=%9d idx=%9d\n", instanceCount, scenario.label, ElapsedMillis(start, end), quadCount,
               vertexCount, indexCount);
    check(vertexCount == quadCount * 4, "operation 2: emitted vertex count equals exactly quadCount*4");
    check(indexCount == quadCount * 6, "operation 2: emitted index count equals exactly quadCount*6");
}

// Operation 3 — the naive per-instance AddImage() comparison (defined in the Scenarios sibling, the
// explicitly-rejected path STEP53 §0 forbids from MapCanvas_IconLayer_Draw_UI.cpp), same instance
// count and headless-frame technique as operation 2, so the two are apples-to-apples.
void RunOperationThreeNaiveAddImage(const std::vector<OverlayVisibleInstance>& candidates, int instanceCount,
                                    const ScenarioSpec& scenario) {
    ImDrawList& drawList = BeginBenchmarkFrame("IconLayerMicrobenchmarkNaiveWindow");
    const auto start = std::chrono::steady_clock::now();
    const std::size_t drawnCount = DrawNaivePerInstanceImages(drawList, candidates);
    const auto end = std::chrono::steady_clock::now();
    EndBenchmarkFrame();

    std::printf("[op3 naive AddImage]            N=%7d scenario=%-12s elapsed=%9.3fms quadCount=%9zu\n",
               instanceCount, scenario.label, ElapsedMillis(start, end), drawnCount);
    check(drawnCount == candidates.size(),
          "operation 3 draws exactly the same visible-instance count as operation 2, at matching N/scenario");
}

} // namespace Ui
} // namespace SanmapGen

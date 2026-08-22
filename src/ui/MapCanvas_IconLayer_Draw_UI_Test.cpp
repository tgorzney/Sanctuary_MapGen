// MapCanvas_IconLayer_Draw_UI_Test.cpp — acceptance test, part 4: §3's atlas-page bucketing + bulk
// write, live headless imgui frame (no GL, no window), mirroring MapCanvas_Render_UI_Test.cpp's own
// technique. One translation unit of the MapCanvas_IconLayer_UI_Test binary.
#include "MapCanvas_IconLayer_DrawInternal_UI.h"
#include "MapCanvas_IconLayer_TestFixture_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

constexpr unsigned long long kFontAtlasIdentifier = 0xF0000001ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 512.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFontAtlasIdentifier));
    ImGui::NewFrame();
}

OverlayVisibleInstance MakeQuad(int atlasPage, std::uint64_t textureIdentifier) {
    OverlayVisibleInstance instance;
    instance.atlasPage = atlasPage; instance.textureIdentifier = textureIdentifier;
    instance.screenCenterX = 10.0f; instance.screenCenterY = 10.0f; instance.screenSize = 4.0f;
    instance.tintAlpha = 1.0f;
    return instance;
}

// Draw-command count equals distinct atlas pages touched, not instance count or layer count;
// generated vertex/index counts equal exactly quadCount*4 / quadCount*6.
void CheckBucketingProducesOnePageOneCommand() {
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("IconLayerDrawTestWindow");
    ImDrawList& drawList = *ImGui::GetWindowDrawList();

    std::vector<OverlayVisibleInstance> instances;
    for (int i = 0; i < 5; ++i) instances.push_back(MakeQuad(0, 100ull));   // page 0, 5 quads
    for (int i = 0; i < 3; ++i) instances.push_back(MakeQuad(1, 200ull));   // page 1, 3 quads
    const std::vector<AtlasPageBucket> buckets = BucketByAtlasPage(instances);
    check(buckets.size() == 2, "two distinct atlas pages bucket into two AtlasPageBuckets");
    FlushBuckets(drawList, buckets);

    ImGui::End();
    ImGui::Render();
    const ImDrawData& drawData = *ImGui::GetDrawData();
    int drawCommandsWithOurTextures = 0;
    int totalVertexCount = 0, totalIndexCount = 0;
    for (int listIndex = 0; listIndex < drawData.CmdListsCount; ++listIndex) {
        const ImDrawList* list = drawData.CmdLists[listIndex];
        for (const ImDrawCmd& command : list->CmdBuffer) {
            const ImTextureID textureIdentifier = command.GetTexID();
            if (textureIdentifier == static_cast<ImTextureID>(100ull)
             || textureIdentifier == static_cast<ImTextureID>(200ull)) {
                ++drawCommandsWithOurTextures;
                totalIndexCount += static_cast<int>(command.ElemCount);
            }
        }
        totalVertexCount += list->VtxBuffer.Size;
    }
    check(drawCommandsWithOurTextures == 2,
          "the icon layer produces exactly one ImDrawCmd per distinct atlas page touched");
    check(totalIndexCount == 8 * 6, "total generated indices equal exactly quadCount*6");
    check(totalVertexCount >= 8 * 4, "total generated vertices are at least quadCount*4");
    ImGui::DestroyContext();
}

// End-to-end: DrawOverlayIconLayers (cull -> budget -> bucket -> flush) actually emits a draw
// command carrying the resolved instance's atlas page texture identifier.
void CheckFullPipelineEmitsADrawCommand() {
    IconLayerTestFixture fixture;
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "propA");
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propA", 0, /*atlasPage=*/0);
    fixture.atlasManifest.pageTextureIdentifiers[0] = 777ull;
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Props;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("IconLayerDrawTestWindow");
    DrawOverlayIconLayers(fixture.Input(), fixture.aabbCache, fixture.frameCache, *ImGui::GetWindowDrawList());
    ImGui::End();
    ImGui::Render();

    bool bFoundOurTexture = false;
    const ImDrawData& drawData = *ImGui::GetDrawData();
    for (int listIndex = 0; listIndex < drawData.CmdListsCount; ++listIndex)
        for (const ImDrawCmd& command : drawData.CmdLists[listIndex]->CmdBuffer)
            if (command.GetTexID() == static_cast<ImTextureID>(777ull)) bFoundOurTexture = true;
    check(bFoundOurTexture, "the full pipeline resolves and draws the in-view instance's atlas page");
    ImGui::DestroyContext();
}

} // namespace

void RunMapCanvasIconLayerDrawChecks() {
    CheckBucketingProducesOnePageOneCommand();
    CheckFullPipelineEmitsADrawCommand();
}

} // namespace Ui
} // namespace SanmapGen

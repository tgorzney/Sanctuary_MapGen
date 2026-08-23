// MapCanvas_IconLayer_DrawChunkTestSupport_UI.h — shared helpers for STEP98's chunking acceptance
// test, split across MapCanvas_IconLayer_DrawChunk_UI_Test.cpp (chunk-range helper + live-path) and
// MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp (cache-path round trip) to stay inside Constitution
// §1.5's file-size ceiling, mirroring MapCanvas_IconLayer_TestFixture_UI.h's own precedent (a
// shared, header-only fixture, not a duplicated setup per translation unit). Test-support only.
#pragma once
#include "MapCanvas_IconLayer_DrawInternal_UI.h"
#include "MapCanvas_IconLayer_TestFixture_UI.h"

namespace SanmapGen {
namespace Ui {

constexpr unsigned long long kIconLayerChunkTestFontAtlasIdentifier = 0xF0000001ull;
constexpr std::uint64_t kIconLayerChunkTestTextureIdentifier = 555ull;

// Sets ImGuiBackendFlags_RendererHasVtxOffset the way the real ImGui_ImplOpenGL3_Init backend does
// (Application_Window_UI.cpp) -- required so ImDrawListFlags_AllowVtxOffset lands on every drawlist
// (imgui.cpp's NewFrame reads it once, at the top) and PrimReserve's own VtxOffset bump can fire;
// without it this headless context has no backend at all and 16,000+-quad buckets trip imgui's
// debug-only 16-bit-index assert even through the fixed, chunked code path.
inline void BeginIconLayerChunkTestHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.DisplaySize = ImVec2(512.0f, 512.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kIconLayerChunkTestFontAtlasIdentifier));
    ImGui::NewFrame();
}

// Each instance gets a distinct, identifiable screenCenterX = quadIndex (screenCenterY = 0), so no
// two quads' geometry is indistinguishable -- what the index-wraparound regression check needs.
inline std::vector<OverlayVisibleInstance> BuildIconLayerChunkTestQuads(int quadCount,
                                                                        std::uint64_t textureIdentifier) {
    std::vector<OverlayVisibleInstance> instances;
    instances.reserve(static_cast<std::size_t>(quadCount));
    for (int quadIndex = 0; quadIndex < quadCount; ++quadIndex) {
        OverlayVisibleInstance instance;
        instance.atlasPage = 0; instance.textureIdentifier = textureIdentifier;
        instance.screenCenterX = static_cast<float>(quadIndex); instance.screenCenterY = 0.0f;
        instance.screenSize = 2.0f;   // half = 1.0f
        instance.tintAlpha = 1.0f;
        instances.push_back(instance);
    }
    return instances;
}

// The correctness assertion the bug demands: gather every ImDrawCmd (optionally restricted to
// onlyFromList) carrying textureIdentifier, confirm exactly ceil(quadCount/chunkCap) of them, exact
// total vertex/index counts, and -- per quad, in emission order -- resolve its four vertices'
// absolute buffer positions (command.VtxOffset + localIndexValue) against its own unique
// screenCenterX +/- half. Before the fix this fails at and beyond quad index 16,384 due to 16-bit
// ImDrawIdx wraparound colliding with an earlier quad's geometry.
inline void VerifyIconLayerChunkTestBucketDrawOutput(const ImDrawData& drawData, std::uint64_t textureIdentifier,
                                                      int quadCount, const ImDrawList* onlyFromList) {
    std::vector<const ImDrawList*> lists;
    std::vector<const ImDrawCmd*> commands;
    for (int listIndex = 0; listIndex < drawData.CmdListsCount; ++listIndex) {
        const ImDrawList* const list = drawData.CmdLists[listIndex];
        if (onlyFromList != nullptr && list != onlyFromList) continue;
        for (const ImDrawCmd& command : list->CmdBuffer)
            if (command.GetTexID() == static_cast<ImTextureID>(textureIdentifier)) {
                lists.push_back(list); commands.push_back(&command);
            }
    }
    const int expectedChunkCount = (quadCount + kIconLayerBucketChunkQuadCap - 1) / kIconLayerBucketChunkQuadCap;
    check(static_cast<int>(commands.size()) == expectedChunkCount,
          "exactly ceil(quadCount / chunkCap) distinct ImDrawCmd entries carry the bucket's texture id");

    long long totalVertexCount = 0, totalIndexCount = 0;
    for (const ImDrawCmd* command : commands) {
        totalIndexCount += command->ElemCount;
        totalVertexCount += (command->ElemCount / 6) * 4;
    }
    check(totalVertexCount == static_cast<long long>(quadCount) * 4, "total ImDrawVert count equals quadCount*4");
    check(totalIndexCount == static_cast<long long>(quadCount) * 6, "total ImDrawIdx count equals quadCount*6");

    constexpr float kHalf = 1.0f;
    bool bAllMatch = true; int quadCursor = 0;
    for (std::size_t c = 0; c < commands.size(); ++c) {
        const ImDrawList& list = *lists[c]; const ImDrawCmd& command = *commands[c];
        for (int q = 0; q < static_cast<int>(command.ElemCount) / 6; ++q) {
            const unsigned int idx = command.IdxOffset + static_cast<unsigned int>(q) * 6u;
            const float cx = static_cast<float>(quadCursor);
            // Local index sequence (0,1,2,0,2,3): sequence positions 0,1,2,5 hold the 4 distinct corners.
            const float x0 = list.VtxBuffer[command.VtxOffset + list.IdxBuffer[idx + 0]].pos.x;
            const float x1 = list.VtxBuffer[command.VtxOffset + list.IdxBuffer[idx + 1]].pos.x;
            const float x2 = list.VtxBuffer[command.VtxOffset + list.IdxBuffer[idx + 2]].pos.x;
            const float x5 = list.VtxBuffer[command.VtxOffset + list.IdxBuffer[idx + 5]].pos.x;
            if (x0 != cx - kHalf || x1 != cx + kHalf || x2 != cx + kHalf || x5 != cx - kHalf) bAllMatch = false;
            ++quadCursor;
        }
    }
    check(bAllMatch && quadCursor == quadCount,
          "every quad's resolved vertex positions match its own unique screenCenterX -- no index "
          "collision from 16-bit ImDrawIdx wraparound");
}

} // namespace Ui
} // namespace SanmapGen

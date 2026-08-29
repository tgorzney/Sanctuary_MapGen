// MapCanvas_Render_UI_Test.cpp — acceptance test, part 1: the composite's texture actually
// renders in the canvas region. One translation unit of the MapCanvas_UI_Test binary.
// It composites on the Gpu (so the image IS a GL texture, M5-5's repoint), hands the canvas the
// texture, runs one imgui frame with no renderer backend, and inspects the produced draw data:
// a draw command carrying the composite's presentation identifier, covering exactly the canvas
// rectangle, sampling exactly the view's texture window. That is "it renders in the region"
// stated as an assertion instead of a screenshot.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   renderPreviewResolution = 32;
constexpr float renderRegionSidePixels  = 256.0f;

bool NearlyEqual(float value, float expected) {
    const float difference = value - expected;
    return difference < 0.01f && difference > -0.01f;
}

// A height gradient, so the composited texture is not one flat color and a blank region cannot
// pass as a rendered one.
void BuildRenderScene(PreviewTestScene& scene) {
    BuildPreviewTestScene(scene);
    for (int cellY = 0; cellY < scene.geometry.VertexSize(); ++cellY)
        for (int cellX = 0; cellX < scene.geometry.VertexSize(); ++cellX)
            scene.fields.heightfield.Set(cellX, cellY, static_cast<float>(cellX + cellY) * 0.1f);
}

// The rectangle and texture-coordinate window one draw command covers.
struct DrawnImageRectangle {
    bool  bFound = false;
    float lowScreenX = 0.0f, lowScreenY = 0.0f, highScreenX = 0.0f, highScreenY = 0.0f;
    float lowTextureCoordinateX = 0.0f, highTextureCoordinateX = 0.0f;
};

DrawnImageRectangle FindDrawnImage(const ImDrawData& drawData, ImTextureID textureIdentifier) {
    DrawnImageRectangle drawn;
    for (int listIndex = 0; listIndex < drawData.CmdListsCount; ++listIndex) {
        const ImDrawList* const drawList = drawData.CmdLists[listIndex];
        for (const ImDrawCmd& command : drawList->CmdBuffer) {
            if (command.ElemCount == 0 || command.GetTexID() != textureIdentifier) continue;
            for (unsigned int element = 0; element < command.ElemCount; ++element) {
                const ImDrawVert& vertex =
                    drawList->VtxBuffer[command.VtxOffset + drawList->IdxBuffer[command.IdxOffset + element]];
                if (!drawn.bFound) {
                    drawn.lowScreenX = drawn.highScreenX = vertex.pos.x;
                    drawn.lowScreenY = drawn.highScreenY = vertex.pos.y;
                    drawn.lowTextureCoordinateX = drawn.highTextureCoordinateX = vertex.uv.x;
                    drawn.bFound = true;
                    continue;
                }
                drawn.lowScreenX = vertex.pos.x < drawn.lowScreenX ? vertex.pos.x : drawn.lowScreenX;
                drawn.lowScreenY = vertex.pos.y < drawn.lowScreenY ? vertex.pos.y : drawn.lowScreenY;
                drawn.highScreenX = vertex.pos.x > drawn.highScreenX ? vertex.pos.x : drawn.highScreenX;
                drawn.highScreenY = vertex.pos.y > drawn.highScreenY ? vertex.pos.y : drawn.highScreenY;
                drawn.lowTextureCoordinateX =
                    vertex.uv.x < drawn.lowTextureCoordinateX ? vertex.uv.x : drawn.lowTextureCoordinateX;
                drawn.highTextureCoordinateX =
                    vertex.uv.x > drawn.highTextureCoordinateX ? vertex.uv.x : drawn.highTextureCoordinateX;
            }
        }
    }
    return drawn;
}

// The font atlas gets an identifier no GL texture name can take, so the text draw commands can
// never be mistaken for the canvas's image (the composite's texture is the first one this context
// creates, i.e. name 1 — a real collision, not a hypothetical one).
constexpr unsigned long long fontAtlasIdentifier = 0xF0000001ull;

// One imgui frame with no renderer backend: the font atlas is built the legacy way and the frame
// is only rendered into draw data, which is all this test reads.
void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1024.0f, 1024.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr;
    int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(fontAtlasIdentifier));
    ImGui::NewFrame();
}

DrawnImageRectangle DrawCanvasFrame(MapCanvas& canvas) {
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(600.0f, 600.0f));
    ImGui::Begin("MapCanvasTestWindow");
    canvas.Draw("mapCanvas", renderRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return FindDrawnImage(*ImGui::GetDrawData(),
                          static_cast<ImTextureID>(canvas.PresentationIdentifier()));
}

} // namespace

void RunMapCanvasRenderChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildRenderScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = renderPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();
    check(composite.LastRunUsedGpu(), "the composite ran on the Gpu");
    check(composite.CompositeTexture().IsValid(),
          "the composite emits a real GpuResource_SYS texture, not a packed-uint buffer");

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    // Render-only checks below never click, so no picking source is wired (STEP48: MapCanvas no
    // longer reads the id buffer this scene still bakes for the other composite tests).
    check(canvas.PresentationIdentifier() != 0ull,
          "the canvas has a toolkit identifier for the composite texture");

    ImGui::CreateContext();
    const DrawnImageRectangle drawn = DrawCanvasFrame(canvas);
    check(drawn.bFound, "the composite texture is drawn in the canvas region");
    check(NearlyEqual(drawn.highScreenX - drawn.lowScreenX, renderRegionSidePixels)
       && NearlyEqual(drawn.highScreenY - drawn.lowScreenY, renderRegionSidePixels),
          "the drawn image covers exactly the canvas region");
    check(NearlyEqual(drawn.lowTextureCoordinateX, 0.0f)
       && NearlyEqual(drawn.highTextureCoordinateX, 1.0f),
          "at zoom 1 the whole composite image is sampled");

    canvas.ApplyScroll(renderRegionSidePixels * 0.5f, renderRegionSidePixels * 0.5f, 8.0f);
    const DrawnImageRectangle zoomed = DrawCanvasFrame(canvas);
    check(zoomed.bFound && zoomed.highTextureCoordinateX - zoomed.lowTextureCoordinateX
                         < drawn.highTextureCoordinateX - drawn.lowTextureCoordinateX,
          "zooming in narrows the sampled window of the same texture (no re-upload)");
    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen

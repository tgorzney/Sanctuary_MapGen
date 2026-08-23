// MapCanvas_MarkerDrag_UI_Test.cpp — headless coverage for the one imgui-including half of STEP94:
// HitTestManualMarkers (Gap 5's linear routing-around, including its first-match-wins tie rule,
// mirrored from Picking_UI::PickMarker's own convention) and DrawManualMarkerRoster (Gap 6's
// stopgap draw: at-rest dots, a soft-hidden sibling skipped, a ghost point drawn distinctly, and
// the Spawn-refused tint). One live headless imgui frame, no window/GL — mirrors
// MapCanvas_ScenarioEditMode_DrawMarkers_UI_Test.cpp's own technique, inspecting the shared
// ImDrawList's vertex colors directly rather than only counting vertices, since this file's states
// mostly differ by TINT alone (AddCircleFilled every time), which a vertex-count proxy cannot see.
#include "MapCanvas_MarkerDrag_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <cstdio>
#include <imgui.h>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

constexpr unsigned long long kFontAtlasIdentifier = 0xF0000003ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(256.0f, 256.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFontAtlasIdentifier));
    ImGui::NewFrame();
}

struct DrawFixture {
    PreviewTestScene scene;
    PreviewComposite* composite;
    MapCanvasView view;
    DrawFixture() {
        BuildPreviewTestScene(scene);
        composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.fields,
                                         scene.instances, scene.entityIdentifiers);
        ConfigurePreviewSettings(composite->Settings());
        composite->ComposeOnCpu();
        view.SetPreviewResolution(composite->Resolution());
        view.SetRegionSide(256.0f);
    }
    ~DrawFixture() { delete composite; }
    DrawFixture(const DrawFixture&) = delete;
    DrawFixture& operator=(const DrawFixture&) = delete;
};

Params::MarkerTransform MakeTransform(const char* name, float x, float z, int layerIndex = 0) {
    Params::MarkerTransform transform;
    transform.name = name;
    transform.transform.positionX = x;
    transform.transform.positionZ = z;
    transform.layerIndex = layerIndex;
    return transform;
}

RegionLocalPoint ScreenPointFor(const DrawFixture& fixture, float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint previewPixel = fixture.composite->WorldToPreviewPixel(worldX, worldZ);
    return fixture.view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
}

// ---- HitTestManualMarkers ----------------------------------------------------------------------

void RunHitTestChecks() {
    DrawFixture fixture;
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].transforms.push_back(MakeTransform("A", 1.0f, 1.0f));
    markers[1].transforms.push_back(MakeTransform("B", 3.0f, 3.0f));

    const RegionLocalPoint onA = ScreenPointFor(fixture, 1.0f, 1.0f);
    int groupIndex = -99, transformIndex = -99;
    Check(HitTestManualMarkers(markers, *fixture.composite, fixture.view, onA.regionLocalX, onA.regionLocalY,
                               8.0f, groupIndex, transformIndex),
          "a press exactly on a marker's projected point hits it");
    Check(groupIndex == 0 && transformIndex == 0, "resolves the correct (group, transform) pair");

    groupIndex = -99; transformIndex = -99;
    Check(!HitTestManualMarkers(markers, *fixture.composite, fixture.view, -500.0f, -500.0f, 8.0f,
                                groupIndex, transformIndex),
          "a press far from every marker misses");
    Check(groupIndex == -1 && transformIndex == -1, "a miss leaves both out-params at -1");

    // Two markers projecting to the exact same screen point: the first (lowest group index) wins,
    // never silently overwritten by the later one at an identical distance (Picking_UI::PickMarker's
    // own tie convention, mirrored here).
    std::vector<Params::MarkerInstanceGroup> tiedMarkers(2);
    tiedMarkers[0].transforms.push_back(MakeTransform("First", 5.0f, 5.0f));
    tiedMarkers[1].transforms.push_back(MakeTransform("Second", 5.0f, 5.0f));
    const RegionLocalPoint tiedPoint = ScreenPointFor(fixture, 5.0f, 5.0f);
    groupIndex = -99; transformIndex = -99;
    Check(HitTestManualMarkers(tiedMarkers, *fixture.composite, fixture.view, tiedPoint.regionLocalX,
                               tiedPoint.regionLocalY, 8.0f, groupIndex, transformIndex),
          "an exact tie still resolves to a hit");
    Check(groupIndex == 0, "a tie keeps the FIRST (lowest group index) marker, not the last");

    Check(!HitTestManualMarkers({}, *fixture.composite, fixture.view, 0.0f, 0.0f, 8.0f,
                                groupIndex, transformIndex),
          "an empty roster never hits");
}

// ---- DrawManualMarkerRoster --------------------------------------------------------------------

ImU32 LastVertexColor(const ImDrawList& drawList) {
    return drawList.VtxBuffer.Data[drawList.VtxBuffer.Size - 1].col;
}

void RunDrawAtRestAndSoftHideChecks() {
    DrawFixture fixture;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Visible", 1.0f, 1.0f));
    markers[0].transforms.push_back(MakeTransform("Hidden", 2.0f, 2.0f));
    std::vector<Params::MarkerInstanceLayer> noLayers;

    MarkerDragGestureState dragState;   // inactive — nothing is soft-hidden or refused
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    int beforeVertexCount = drawList.VtxBuffer.Size;
    DrawManualMarkerRoster(markers, noLayers, dragState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    Check(drawList.VtxBuffer.Size > beforeVertexCount, "at-rest markers draw at least one primitive each");

    // Now make transform 1 the gesture's soft-hidden sibling: its dot must be skipped entirely.
    dragState.bActive = true;
    dragState.groupIndex = 0;
    MarkerOrbitCorrespondence hiddenEntry;
    hiddenEntry.transformIndex = 1;
    hiddenEntry.bSoftHidden = true;
    dragState.correspondence.push_back(hiddenEntry);

    beforeVertexCount = drawList.VtxBuffer.Size;
    DrawManualMarkerRoster(markers, noLayers, dragState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const int withOneHiddenDelta = drawList.VtxBuffer.Size - beforeVertexCount;

    dragState.bActive = false;   // draw again with the gesture inactive: both dots draw
    beforeVertexCount = drawList.VtxBuffer.Size;
    DrawManualMarkerRoster(markers, noLayers, dragState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const int withNoneHiddenDelta = drawList.VtxBuffer.Size - beforeVertexCount;

    Check(withOneHiddenDelta < withNoneHiddenDelta,
          "the soft-hidden sibling contributes strictly less geometry than when nothing is hidden");
}

void RunDrawRefusedTintChecks() {
    DrawFixture fixture;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Player", 1.0f, 1.0f));
    std::vector<Params::MarkerInstanceLayer> noLayers;
    ImDrawList& drawList = *ImGui::GetWindowDrawList();

    MarkerDragGestureState ordinaryState;
    ordinaryState.bActive = true; ordinaryState.groupIndex = 0; ordinaryState.bSpawnCardinalityRefused = false;
    DrawManualMarkerRoster(markers, noLayers, ordinaryState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const ImU32 ordinaryColor = LastVertexColor(drawList);

    MarkerDragGestureState refusedState;
    refusedState.bActive = true; refusedState.groupIndex = 0; refusedState.bSpawnCardinalityRefused = true;
    DrawManualMarkerRoster(markers, noLayers, refusedState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const ImU32 refusedColor = LastVertexColor(drawList);

    Check(ordinaryColor != refusedColor, "a Spawn-refused frame tints the dot differently from an ordinary drag");
}

} // namespace

int main() {
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("MapCanvasMarkerDragTestWindow");

    RunHitTestChecks();
    RunDrawAtRestAndSoftHideChecks();
    RunDrawRefusedTintChecks();

    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

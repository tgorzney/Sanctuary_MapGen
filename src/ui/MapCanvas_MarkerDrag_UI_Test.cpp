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
#include <cmath>
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

// AddCircleFilled's anti-aliased fill (on by default) appends, for each path point, a solid
// "inner" vertex immediately followed by a transparent "outer" fringe vertex (same RGB, alpha
// forced to 0 for the smooth edge fade — see ImDrawList::AddConvexPolyFilled). The buffer's very
// last vertex is therefore always that alpha-zeroed fringe vertex, not the shape's actual fill
// tint; the second-to-last vertex is the solid one right before it. Index [-2], not [-1].
ImU32 LastVertexColor(const ImDrawList& drawList) {
    return drawList.VtxBuffer.Data[drawList.VtxBuffer.Size - 2].col;
}

void RunDrawAtRestAndSoftHideChecks() {
    DrawFixture fixture;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Visible", 1.0f, 1.0f));
    markers[0].transforms.push_back(MakeTransform("Hidden", 2.0f, 2.0f));
    std::vector<Params::MarkerInstanceLayer> noLayers;
    std::vector<Params::Army> noArmies;
    Params::GlobalMarkerSettings globalMarkerSettings;

    MarkerDragGestureState dragState;   // inactive — nothing is soft-hidden or refused
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    int beforeVertexCount = drawList.VtxBuffer.Size;
    DrawManualMarkerRoster(markers, noLayers, noArmies, globalMarkerSettings, dragState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    Check(drawList.VtxBuffer.Size > beforeVertexCount, "at-rest markers draw at least one primitive each");

    // Now make transform 1 the gesture's soft-hidden sibling: its dot must be skipped entirely.
    dragState.bActive = true;
    dragState.groupIndex = 0;
    MarkerOrbitCorrespondence hiddenEntry;
    hiddenEntry.transformIndex = 1;
    hiddenEntry.bSoftHidden = true;
    dragState.correspondence.push_back(hiddenEntry);

    beforeVertexCount = drawList.VtxBuffer.Size;
    DrawManualMarkerRoster(markers, noLayers, noArmies, globalMarkerSettings, dragState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const int withOneHiddenDelta = drawList.VtxBuffer.Size - beforeVertexCount;

    dragState.bActive = false;   // draw again with the gesture inactive: both dots draw
    beforeVertexCount = drawList.VtxBuffer.Size;
    DrawManualMarkerRoster(markers, noLayers, noArmies, globalMarkerSettings, dragState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const int withNoneHiddenDelta = drawList.VtxBuffer.Size - beforeVertexCount;

    Check(withOneHiddenDelta < withNoneHiddenDelta,
          "the soft-hidden sibling contributes strictly less geometry than when nothing is hidden");
}

void RunDrawRefusedTintChecks() {
    DrawFixture fixture;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Player", 1.0f, 1.0f));
    std::vector<Params::MarkerInstanceLayer> noLayers;
    std::vector<Params::Army> noArmies;
    Params::GlobalMarkerSettings globalMarkerSettings;
    ImDrawList& drawList = *ImGui::GetWindowDrawList();

    MarkerDragGestureState ordinaryState;
    ordinaryState.bActive = true; ordinaryState.groupIndex = 0; ordinaryState.bSpawnCardinalityRefused = false;
    DrawManualMarkerRoster(markers, noLayers, noArmies, globalMarkerSettings, ordinaryState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const ImU32 ordinaryColor = LastVertexColor(drawList);

    MarkerDragGestureState refusedState;
    refusedState.bActive = true; refusedState.groupIndex = 0; refusedState.bSpawnCardinalityRefused = true;
    DrawManualMarkerRoster(markers, noLayers, noArmies, globalMarkerSettings, refusedState, *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
    const ImU32 refusedColor = LastVertexColor(drawList);

    Check(ordinaryColor != refusedColor, "a Spawn-refused frame tints the dot differently from an ordinary drag");
}

// ---- STEP112: Spawn-group manual markers tint by real army color ------------------------------

void RunSpawnArmyTintChecks() {
    DrawFixture fixture;
    std::vector<Params::MarkerInstanceLayer> noLayers;
    Params::GlobalMarkerSettings globalMarkerSettings;
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    MarkerDragGestureState inactiveDragState;   // inactive — no refused-tint priority in play

    // A Spawn-group transform whose name matches an army renders that army's real color.
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = Params::kSpawnMarkerGroupName;
        markers[0].transforms.push_back(MakeTransform("ARMY_01", 1.0f, 1.0f));
        std::vector<Params::Army> armies(1);
        armies[0].name = "ARMY_01";
        armies[0].armyColor[0] = 0.0f; armies[0].armyColor[1] = 1.0f;
        armies[0].armyColor[2] = 0.0f; armies[0].armyColor[3] = 1.0f;
        DrawManualMarkerRoster(markers, noLayers, armies, globalMarkerSettings, inactiveDragState, *fixture.composite,
                               fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList)
                  == ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)),
              "a Spawn transform whose name matches an army renders that army's real color");
    }

    // An orphaned Spawn slot (no matching army) falls back to the existing layer-color behavior.
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = Params::kSpawnMarkerGroupName;
        markers[0].transforms.push_back(MakeTransform("ARMY_99", 1.0f, 1.0f));
        std::vector<Params::Army> armies(1);
        armies[0].name = "ARMY_01";
        DrawManualMarkerRoster(markers, noLayers, armies, globalMarkerSettings, inactiveDragState, *fixture.composite,
                               fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList) == IM_COL32(220, 220, 220, 255),
              "an orphaned Spawn slot with no matching army falls back to the neutral layer color");
    }

    // A non-Spawn group with a name-colliding transform is unaffected — gated on group identity.
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = "Alloys";
        markers[0].transforms.push_back(MakeTransform("ARMY_01", 1.0f, 1.0f));
        std::vector<Params::Army> armies(1);
        armies[0].name = "ARMY_01";
        armies[0].armyColor[0] = 0.0f; armies[0].armyColor[1] = 1.0f;
        armies[0].armyColor[2] = 0.0f; armies[0].armyColor[3] = 1.0f;
        DrawManualMarkerRoster(markers, noLayers, armies, globalMarkerSettings, inactiveDragState, *fixture.composite,
                               fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList) == IM_COL32(220, 220, 220, 255),
              "a non-Spawn group whose transform name collides with an army name is unaffected");
    }

    // The Spawn-cardinality-refused red tint still wins over army color.
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = Params::kSpawnMarkerGroupName;
        markers[0].transforms.push_back(MakeTransform("ARMY_01", 1.0f, 1.0f));
        std::vector<Params::Army> armies(1);
        armies[0].name = "ARMY_01";
        armies[0].armyColor[0] = 0.0f; armies[0].armyColor[1] = 1.0f;
        armies[0].armyColor[2] = 0.0f; armies[0].armyColor[3] = 1.0f;
        MarkerDragGestureState refusedState;
        refusedState.bActive = true; refusedState.groupIndex = 0; refusedState.bSpawnCardinalityRefused = true;
        DrawManualMarkerRoster(markers, noLayers, armies, globalMarkerSettings, refusedState, *fixture.composite,
                               fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList) == IM_COL32(220, 60, 40, 255),
              "the Spawn-cardinality-refused red tint still wins over army color");
    }
}

// ---- STEP116: dot renderer, layer override / type-default / unrecognized-group resolution ------

void RunTypeDefaultColorChecks() {
    DrawFixture fixture;
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    std::vector<Params::Army> noArmies;
    MarkerDragGestureState inactiveDragState;

    // Override wins over type default: bColorOverrideEnabled = true, color deliberately different
    // from what the "Alloys" group name would otherwise resolve.
    {
        std::vector<Params::MarkerInstanceLayer> markerLayers(1);
        markerLayers[0].bColorOverrideEnabled = true;
        markerLayers[0].color[0] = 0.1f; markerLayers[0].color[1] = 0.2f;
        markerLayers[0].color[2] = 0.3f; markerLayers[0].color[3] = 1.0f;
        Params::GlobalMarkerSettings globalMarkerSettings;
        globalMarkerSettings.colorAlloy[0] = 0.4f; globalMarkerSettings.colorAlloy[1] = 0.5f;
        globalMarkerSettings.colorAlloy[2] = 0.6f; globalMarkerSettings.colorAlloy[3] = 1.0f;
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = "Alloys";
        markers[0].transforms.push_back(MakeTransform("Mex 0", 1.0f, 1.0f));
        DrawManualMarkerRoster(markers, markerLayers, noArmies, globalMarkerSettings, inactiveDragState,
                               *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList)
                  == ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.2f, 0.3f, 1.0f)),
              "an explicit layer color override wins over the group's type-default color");
    }

    // Type default when override is disabled: resolves colorAlloy (RGB) with alpha from layer.color[3].
    {
        std::vector<Params::MarkerInstanceLayer> markerLayers(1);
        markerLayers[0].bColorOverrideEnabled = false;
        Params::GlobalMarkerSettings globalMarkerSettings;
        globalMarkerSettings.colorAlloy[0] = 0.4f; globalMarkerSettings.colorAlloy[1] = 0.5f;
        globalMarkerSettings.colorAlloy[2] = 0.6f; globalMarkerSettings.colorAlloy[3] = 1.0f;
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = "Alloys";
        markers[0].transforms.push_back(MakeTransform("Mex 0", 1.0f, 1.0f));
        DrawManualMarkerRoster(markers, markerLayers, noArmies, globalMarkerSettings, inactiveDragState,
                               *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList)
                  == ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.5f, 0.6f, markerLayers[0].color[3])),
              "with the override disabled, the group's type-default color (colorAlloy) resolves, alpha from layer.color[3]");
    }

    // Unrecognized group name resolves opaque white, proving no bleed-through from non-default
    // colorAlloy/colorSpawn.
    {
        std::vector<Params::MarkerInstanceLayer> markerLayers(1);
        markerLayers[0].bColorOverrideEnabled = false;
        Params::GlobalMarkerSettings globalMarkerSettings;
        globalMarkerSettings.colorAlloy[0] = 0.4f; globalMarkerSettings.colorAlloy[1] = 0.5f;
        globalMarkerSettings.colorAlloy[2] = 0.6f;
        globalMarkerSettings.colorSpawn[0] = 0.7f; globalMarkerSettings.colorSpawn[1] = 0.8f;
        globalMarkerSettings.colorSpawn[2] = 0.9f;
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = "Generic";
        markers[0].transforms.push_back(MakeTransform("Mex 0", 1.0f, 1.0f));
        DrawManualMarkerRoster(markers, markerLayers, noArmies, globalMarkerSettings, inactiveDragState,
                               *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList) == IM_COL32(255, 255, 255, 255),
              "an unrecognized group name resolves opaque white, no bleed-through");
    }

    // Orphaned Spawn slot with a real in-range layer (override disabled) now resolves colorSpawn,
    // not the old flat gray — the WYSIWYG improvement STEP116 delivers for orphaned slots.
    {
        std::vector<Params::MarkerInstanceLayer> markerLayers(1);
        markerLayers[0].bColorOverrideEnabled = false;
        Params::GlobalMarkerSettings globalMarkerSettings;
        globalMarkerSettings.colorSpawn[0] = 0.7f; globalMarkerSettings.colorSpawn[1] = 0.8f;
        globalMarkerSettings.colorSpawn[2] = 0.9f; globalMarkerSettings.colorSpawn[3] = 1.0f;
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = Params::kSpawnMarkerGroupName;
        markers[0].transforms.push_back(MakeTransform("ARMY_ORPHAN", 1.0f, 1.0f));
        std::vector<Params::Army> armies(1);
        armies[0].name = "ARMY_NOT_ORPHAN";   // no match for "ARMY_ORPHAN"
        DrawManualMarkerRoster(markers, markerLayers, armies, globalMarkerSettings, inactiveDragState,
                               *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
        Check(LastVertexColor(drawList)
                  == ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.8f, 0.9f, markerLayers[0].color[3])),
              "an orphaned Spawn slot with a real in-range layer resolves colorSpawn, not flat gray");
    }
}

// ---- STEP122: ManualMarkerDotRadius (Global x per-layer Icon Scale composed into the roster
// dot's radius) --------------------------------------------------------------------------------
//
// ManualMarkerDotRadius lives in this file's own anonymous namespace (mirroring ManualMarkerTint's
// exact posture, Fix section 3), so — unlike ResolveMarkerIconTemplateIdentifier/
// ResolveMarkerCategoryTintColor's sibling-file precedent of a non-anonymous, directly-testable
// resolver — it has internal linkage and cannot be called directly from this separate translation
// unit (this test binary links the SanGenV2 static library rather than compiling
// MapCanvas_MarkerDrag_UI.cpp inline). So this test verifies it the same indirect way this file's
// existing RunTypeDefaultColorChecks verifies ManualMarkerTint: through DrawManualMarkerRoster's
// real ImDrawList output — here, the drawn circle's geometric radius rather than its vertex tint.
// AddCircleFilled's antialiased fill appends, per path point, one "inner" vertex at (radius - half
// the fringe width) and one "outer" fringe vertex at (radius + half the fringe width) from center;
// averaging their distances from the known screen-space center cancels the fringe and recovers the
// true radius.
float DistanceFromCenter(const ImVec2& point, const ImVec2& center) {
    const float deltaX = point.x - center.x;
    const float deltaY = point.y - center.y;
    return std::sqrt(deltaX * deltaX + deltaY * deltaY);
}

float DrawnCircleRadius(const ImDrawList& drawList, const ImVec2& center) {
    const ImVec2& inner = drawList.VtxBuffer.Data[drawList.VtxBuffer.Size - 2].pos;
    const ImVec2& outer = drawList.VtxBuffer.Data[drawList.VtxBuffer.Size - 1].pos;
    return (DistanceFromCenter(inner, center) + DistanceFromCenter(outer, center)) * 0.5f;
}

void RunManualMarkerDotRadiusScaleChecks() {
    DrawFixture fixture;
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    std::vector<Params::Army> noArmies;
    MarkerDragGestureState inactiveDragState;
    const RegionLocalPoint projectedCenter = ScreenPointFor(fixture, 1.0f, 1.0f);
    const ImVec2 screenCenter(projectedCenter.regionLocalX, projectedCenter.regionLocalY);

    // Out-of-range layerIndex (empty markerLayers) + unrecognized group name ("Generic"): both
    // terms are a no-op, radius stays the base 6.0f.
    {
        std::vector<Params::MarkerInstanceLayer> noLayers;   // layerIndex 0 is out of range against this
        Params::GlobalMarkerSettings globalMarkerSettings;
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = "Generic";
        markers[0].transforms.push_back(MakeTransform("Mex 0", 1.0f, 1.0f));
        DrawManualMarkerRoster(markers, noLayers, noArmies, globalMarkerSettings, inactiveDragState,
                               *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
        const float radius = DrawnCircleRadius(drawList, screenCenter);
        Check(radius > 5.0f && radius < 7.0f,
              "out-of-range layerIndex + unrecognized group name draws the unscaled base radius (6.0f)");
    }

    // Valid layerIndex, iconScale = 2.0, unrecognized group name ("Generic"): only the layer term
    // applies -> 6.0f * 2.0f = 12.0f.
    {
        std::vector<Params::MarkerInstanceLayer> markerLayers(1);
        markerLayers[0].iconScale = 2.0f;
        Params::GlobalMarkerSettings globalMarkerSettings;
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = "Generic";
        markers[0].transforms.push_back(MakeTransform("Mex 0", 1.0f, 1.0f));
        DrawManualMarkerRoster(markers, markerLayers, noArmies, globalMarkerSettings, inactiveDragState,
                               *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
        const float radius = DrawnCircleRadius(drawList, screenCenter);
        Check(radius > 11.0f && radius < 13.0f,
              "an unrecognized group name returns the base radius times only the layer term (6.0f * 2.0f = 12.0f)");
    }

    // Valid layerIndex, iconScale = 2.0, recognized group name ("Alloys"), scaleAlloy = 3.0:
    // both terms compose -> 6.0f * 2.0f * 3.0f = 36.0f.
    {
        std::vector<Params::MarkerInstanceLayer> markerLayers(1);
        markerLayers[0].iconScale = 2.0f;
        Params::GlobalMarkerSettings globalMarkerSettings;
        globalMarkerSettings.scaleAlloy = 3.0f;
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].name = "Alloys";
        markers[0].transforms.push_back(MakeTransform("Mex 0", 1.0f, 1.0f));
        DrawManualMarkerRoster(markers, markerLayers, noArmies, globalMarkerSettings, inactiveDragState,
                               *fixture.composite, fixture.view, 0.0f, 0.0f, drawList);
        const float radius = DrawnCircleRadius(drawList, screenCenter);
        Check(radius > 34.0f && radius < 38.0f,
              "layerIconScale(2.0) * scaleAlloy(3.0) composes into a 36.0f dot radius (base 6.0f)");
    }
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
    RunSpawnArmyTintChecks();
    RunTypeDefaultColorChecks();
    RunManualMarkerDotRadiusScaleChecks();

    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

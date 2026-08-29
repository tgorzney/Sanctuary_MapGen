// MapCanvas_ScenarioEditMode_DrawMarkers_UI_Test.cpp — acceptance test 1's draw-call-inspection
// half (STEP53's own convention: no window/GL, one live headless imgui frame, mirroring
// MapCanvas_IconLayer_Draw_UI_Test.cpp's own technique). Each of the six states is driven through
// ONE candidate at a time and its own vertex-count contribution to the shared ImDrawList is
// compared — a structural proxy for "renders distinctly" that catches a state silently sharing
// another's exact geometry (never just relying on color, itself invisible to this proxy).
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include "MapCanvasView_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include "../data/PlacementResults_DATA.h"
#include "../data/RuleBucketIndexSet_DATA.h"
#include "../params/Army_PARAMS.h"
#include <imgui.h>
#include <map>
#include <string>

namespace SanmapGen {
namespace Ui {
namespace {

void Check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr unsigned long long kFontAtlasIdentifier = 0xF0000002ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 512.0f);
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
        composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
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

Data::PlacementInstance MakeMarkerInstance(float worldX, float worldZ, int ruleIndex) {
    Data::PlacementInstance instance;
    instance.positionX = worldX; instance.positionZ = worldZ; instance.ruleIndex = ruleIndex;
    return instance;
}

// One overlaySettings+placements+ruleBucketIndex+armies+body combination that resolves to EXACTLY
// one candidate of the requested state.
struct StateFixture {
    Data::PlacementResults    placements;
    Data::RuleBucketIndexSet  ruleBucketIndex;
    OverlayLayerSettings      overlaySettings;
    std::vector<Params::Army> armies;
    Params::ScenarioBody      body;

    ScenarioEditModeDrawInput DrawInput(const DrawFixture& drawFixture) {
        ScenarioEditModeDrawInput input;
        input.resolveInput.overlayLayerSettings = &overlaySettings;
        input.resolveInput.placements           = &placements;
        input.resolveInput.ruleBucketIndex       = &ruleBucketIndex;
        input.resolveInput.armies                = &armies;
        input.composite = drawFixture.composite;
        input.view      = &drawFixture.view;
        input.regionOriginX = 0.0f; input.regionOriginY = 0.0f;
        return input;
    }
};

StateFixture MakeSpawnFixture(bool bExplicit) {
    StateFixture fixture;
    fixture.armies.push_back(Params::Army()); fixture.armies[0].name = "ARMY_01";
    fixture.placements.markers.Append(MakeMarkerInstance(2.0f, 2.0f, 0));
    const int ruleIndexColumn[1] = {0};
    fixture.ruleBucketIndex.markers.Build(ruleIndexColumn, 1, 1);
    OverlayLayer_UI spawnsLayer; spawnsLayer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
    spawnsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers = {spawnsLayer};
    if (bExplicit) {
        Params::ScenarioSpawn spawn; spawn.armyName = "ARMY_01"; spawn.positionX = 3.0f; spawn.positionZ = 3.0f;
        fixture.body.spawns.push_back(spawn);
    }
    return fixture;
}

StateFixture MakeAlloyFixture(Params::ScenarioAlloyMode alloyMode, bool bBaselineInstance,
                              bool bMatchingRemoval, bool bAddedEntry, const std::string& previewAsSlotPattern) {
    StateFixture fixture;
    fixture.body.alloyMode = alloyMode;
    if (bBaselineInstance) {
        fixture.placements.markers.Append(MakeMarkerInstance(2.0f, 2.0f, 0));
        const int ruleIndexColumn[1] = {0};
        fixture.ruleBucketIndex.markers.Build(ruleIndexColumn, 1, 1);
        OverlayLayer_UI alloyLayer; alloyLayer.domainKind = OverlayDomainKind_UI::Alloy;
        alloyLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
        fixture.overlaySettings.overlayLayers = {alloyLayer};
        if (bMatchingRemoval) {
            Params::ScenarioAlloyRemoval removal; removal.markerName = "alloy_r0_0";
            fixture.body.alloysToRemove.push_back(removal);
        }
    }
    if (bAddedEntry) {
        // Kept well inside the 4-cell test map's visible span — a position outside it would
        // project outside the draw list's clip rect, silently culling AddText's glyph vertices
        // (imgui clips text per-glyph; AddCircleFilled does not) and invalidating this test's own
        // "more geometry" comparison below for reasons that have nothing to do with the state.
        Params::ScenarioAlloyOverride added; added.armyName = "ARMY_01"; added.markerName = "custom_0";
        added.positionX = 2.5f; added.positionZ = 1.5f;
        fixture.body.alloysToAdd.push_back(added);
    }
    (void)previewAsSlotPattern;
    return fixture;
}

// Draws `fixture.body` through the real ScenarioEditModeState/DrawScenarioEditModeOverlay entry
// point and returns how many vertices the shared draw list gained.
int VertexDeltaForState(StateFixture& fixture, const DrawFixture& drawFixture, const std::string& previewAsSlotPattern) {
    ScenarioEditModeState state;
    state.Activate(fixture.body, nullptr, nullptr, 1);
    state.previewAsSlotPattern = previewAsSlotPattern;
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    const int beforeVertexCount = drawList.VtxBuffer.Size;
    DrawScenarioEditModeOverlay(state, fixture.DrawInput(drawFixture), drawList);
    Check(state.lastResolvedCandidates.size() == 1u, "the fixture resolves to exactly one candidate");
    return drawList.VtxBuffer.Size - beforeVertexCount;
}

} // namespace

void RunScenarioEditModeDrawChecks() {
    DrawFixture drawFixture;
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("ScenarioEditModeDrawTestWindow");

    std::map<std::string, int> deltaByLabel;
    StateFixture noOverrideFixture = MakeSpawnFixture(false);
    deltaByLabel["SpawnNoOverride"] = VertexDeltaForState(noOverrideFixture, drawFixture, "");
    StateFixture explicitFixture = MakeSpawnFixture(true);
    deltaByLabel["SpawnExplicit"] = VertexDeltaForState(explicitFixture, drawFixture, "");
    StateFixture keptFixture = MakeAlloyFixture(Params::ScenarioAlloyMode::Delta, true, false, false, "");
    deltaByLabel["AlloyKept"] = VertexDeltaForState(keptFixture, drawFixture, "");
    StateFixture deletedFixture = MakeAlloyFixture(Params::ScenarioAlloyMode::Occupancy, true, false, false, "");
    deltaByLabel["AlloyDeleted"] = VertexDeltaForState(deletedFixture, drawFixture, "");
    StateFixture addedFixture = MakeAlloyFixture(Params::ScenarioAlloyMode::Delta, false, false, true, "");
    deltaByLabel["AlloyAdded"] = VertexDeltaForState(addedFixture, drawFixture, "");
    StateFixture removedFixture = MakeAlloyFixture(Params::ScenarioAlloyMode::Delta, true, true, false, "");
    deltaByLabel["AlloyRemovedGhost"] = VertexDeltaForState(removedFixture, drawFixture, "");

    for (const auto& [label, delta] : deltaByLabel)
        Check(delta > 0, (std::string("state '") + label + "' draws at least one primitive").c_str());
    Check(deltaByLabel["SpawnNoOverride"] > deltaByLabel["SpawnExplicit"],
          "the warning-badge state (text glyph) costs strictly more geometry than the plain filled state");
    Check(deltaByLabel["AlloyAdded"] > deltaByLabel["AlloyKept"],
          "the '+'-badge state costs strictly more geometry than the plain kept state");
    Check(deltaByLabel["AlloyRemovedGhost"] != deltaByLabel["AlloyDeleted"],
          "the ghost+X state and the grey+strike state are geometrically distinct from each other");
    Check(deltaByLabel["AlloyKept"] == deltaByLabel["SpawnExplicit"],
          "both plain filled-circle states share identical geometry (their color, not shape, is what differs)");

    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen

// Application_AssetPanel_UI.cpp — the asset half of the System panel plus the two per-frame asset
// steps: turning a NEW icon-grid pick into the selected rule's template id, and performing the
// requested load. Layer: UI. Behind the same header as Application_Assets_UI.cpp (ARCH §1.5).
//
// LOADING IS SYNCHRONOUS, AND THAT IS A REPORTED LIMIT, NOT A CHOICE. ASSET_LOADING_SPEC asks for
// a background ingest with placeholders until the atlas is ready, but `Sys::ThreadPool::ParallelFor`
// is blocking (M5-4 flagged this itself) and no async SYS seam exists. So the shell does what the
// legacy loop did: announce the load in its own presented frame, then perform it on the next. A
// non-blocking seam is a SYS work-order this one does not own (ARCH §8.4).
#include "Application_UI.h"
#include <cstring>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// Copies a resolved identifier into the fixed 8-byte `tpId` field, TRUNCATING rather than
// overflowing (Constitution §6). Reports whether the field actually moved.
bool StoreTemplateIdentifier(const std::string& identifier, char (&templateIdentifier)[8]) {
    char resolved[8] = { 0 };
    const std::size_t copyCount =
        identifier.size() < sizeof(resolved) - 1 ? identifier.size() : sizeof(resolved) - 1;
    for (std::size_t index = 0; index < copyCount; ++index) resolved[index] = identifier[index];
    if (std::memcmp(resolved, templateIdentifier, sizeof(resolved)) == 0) return false;
    std::memcpy(templateIdentifier, resolved, sizeof(resolved));
    return true;
}

} // namespace

// A pick writes the rule only when the selection is NEW, so re-drawing the same grid every frame
// never overwrites a hand-typed template id.
bool Application::ApplyIconSelection(int selectedIconId, int& lastIconId,
                                     char (&templateIdentifier)[8]) {
    if (selectedIconId < 0 || selectedIconId == lastIconId) return false;
    lastIconId = selectedIconId;
    const std::string identifier = TemplateIdentifierOfIcon(selectedIconId);
    return !identifier.empty() && StoreTemplateIdentifier(identifier, templateIdentifier);
}

// The bridge in use: the grid emits an icon id, the shell resolves it to the template identifier
// and stores it on the rule the tab has selected. The tabs stay unaware that an atlas exists
// beyond the read-only manifest they are handed.
void Application::ResolveIconSelections() {
    bool bRecipeMoved = false;
    Params::MarkerRule* const markerRule = SelectedMarkerRule(recipe.markerRuleLayers, tabState.markers);
    if (markerRule != nullptr)
        bRecipeMoved = ApplyIconSelection(tabState.markers.iconGridState.selectedIconId,
                                          tabState.lastMarkerIconId,
                                          markerRule->transform.templateIdentifier) || bRecipeMoved;
    Params::PropRule* const propRule = SelectedPropRule(recipe.propRules, tabState.props);
    if (propRule != nullptr)
        bRecipeMoved = ApplyIconSelection(tabState.props.iconGridState.selectedIconId,
                                          tabState.lastPropIconId,
                                          propRule->transform.templateIdentifier) || bRecipeMoved;
    if (bRecipeMoved) previewDriver.NotifyParametersChanged();
}

bool Application::ServiceAssetLoadRequest() {
    if (!assetBridge.bAssetLoadRequested) return false;
    if (!assetBridge.bAssetLoadAnnounced) {
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if (ImGui::Begin("assetLoading", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav))
            ImGui::TextUnformatted("Loading assets... this may take a few seconds.");
        ImGui::End();
        assetBridge.bAssetLoadAnnounced = true;
        return true;                 // present this frame, then load on the next
    }
    assetBridge.bAssetLoadRequested = false;
    assetBridge.bAssetLoadAnnounced = false;
    LoadAssetAtlas();
    return false;
}

// The sanpack path and the cache directory are both caller-owned buffers: no PARAMS home exists
// for either (SystemTab_UI.h SCOPE NOTE 1), and creating one needs its own work-order, so the
// shell holds them and passes them to IO at call time rather than inventing a settings type.
void Application::DrawAssetPanel() {
    ImGui::TextUnformatted("ASSET ATLAS");
    ImGui::InputText("Sanpack", assetBridge.sanpackPath, IM_ARRAYSIZE(assetBridge.sanpackPath));
    if (ImGui::Button("Load Sanpack")) assetBridge.bAssetLoadRequested = true;
    ImGui::SameLine();
    ImGui::Text("Icons resident: %d", assetBridge.iconManifest.EntryCount());
    ImGui::TextWrapped("%s", assetBridge.assetStatusMessage.c_str());
}

} // namespace Ui
} // namespace SanmapGen

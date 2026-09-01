// MarkersTab_Links_UI.cpp — see MarkersTab_Links_UI.h. DeleteMarkerLink/ApplyAddLinkAction (the two
// pure Apply functions) and DrawMarkerLinksSection (the outer per-Link loop). The header-extra draw
// functions live in the aspect-split sibling MarkersTab_LinksHeaderExtras_UI.cpp (ARCH §1.5).
#include "MarkersTab_Links_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DeleteMarkerLink(int linkIdentifier, std::vector<Params::MarkerLink>& links,
                      std::vector<Params::MarkerInstanceGroup>& markers,
                      std::vector<Params::MarkerLayerBundle>& bundles,
                      std::vector<Params::MarkerInstanceLayer>& markerLayers) {
    // STEP247/ARCH §19.33 — the primary, instance-tier walk (the only tier "+Link" writes to any
    // more).
    for (Params::MarkerInstanceGroup& group : markers)
        for (Params::MarkerTransform& transform : group.transforms)
            if (transform.linkIdentifier == linkIdentifier) transform.linkIdentifier = -1;
    // Dead-write/live-read backward compat for any .sanmap still carrying pre-correction
    // Layer-exclusive Link data (ARCH §19.33) — KEPT, not removed: no live path mints these any
    // more, but the resolver fallback chain still consults them.
    for (Params::MarkerLayerBundle& bundle : bundles)
        if (bundle.linkIdentifier == linkIdentifier) bundle.linkIdentifier = -1;
    for (Params::MarkerInstanceLayer& layer : markerLayers)
        if (layer.linkIdentifier == linkIdentifier) layer.linkIdentifier = -1;
    for (auto it = links.begin(); it != links.end(); ++it)
        if (it->identifier == linkIdentifier) { links.erase(it); break; }
}

void ApplyAddLinkAction(Params::MapRecipe& recipe, const std::vector<int>& selectedManualInstanceIdentifiers) {
    if (selectedManualInstanceIdentifiers.empty()) return;   // ticket's own gate, mirrored defensively
    // ARCH §19.33's no-op guard — proceeding would silently break an already-linked instance's
    // existing Link membership, so ANY already-linked instance in the selection blocks the WHOLE
    // action: no new Link, no tagging.
    if (IsAnyManualInstanceSelectionAlreadyLinked(recipe.markers, selectedManualInstanceIdentifiers)) return;

    Params::MarkerLink link;
    link.identifier = NextMarkerLinkId(recipe.markerLinks);
    link.name       = "Link " + std::to_string(link.identifier);
    recipe.markerLinks.push_back(link);

    // STEP247/ARCH §19.33 — a pure per-instance tag: no Bundle/Layer minted, no
    // ReassignManualInstanceLayers call, existing layering/grouping stays completely untouched.
    TagManualInstancesWithLink(recipe.markers, selectedManualInstanceIdentifiers, link.identifier);
}

void DrawMarkerLinksSection(Params::MapRecipe& recipe, MarkerLinksState_UI& state,
                            int& selectedManualInstanceIdentifier,
                            std::vector<int>& selectedManualInstanceIdentifiers,
                            int& manualInstanceSelectionAnchorIdentifier,
                            const std::function<void(int, const std::vector<int>&)>&
                                selectManualMarkerInstanceCallback,
                            Pipeline::PreviewDriver* previewDriver) {
    // STEP248 — a pre-existing, independent defect fixed here: bAnyCommitted used to be computed per
    // Link (DrawMarkerLinkHeaderExtra) and dropped, so toggling a Link's own Hidden/Locked/Color/
    // Grid/Symmetry controls never tripped a dirty-flag/regenerate. Accumulate across the whole loop,
    // call NotifyPlacementChange once after it finishes — mirrors the Type-section loop's own
    // "call once after the body" convention (MarkersTab_UI.cpp).
    bool bAnyLinkCommitted = false;
    for (Params::MarkerLink& link : recipe.markerLinks) {
        ImGui::PushID(link.identifier);
        if (DrawSectionBegin(link.name.c_str(), state.sectionStateByLinkIdentifier[link.identifier],
                             LinkSectionHeaderOptions(), LinkSectionHeaderStyle())) {
            bool bAnyCommitted = false;
            DrawMarkerLinkHeaderExtra(link, state, bAnyCommitted);
            if (state.renamingLinkIdentifier != link.identifier)
                DrawMarkerLinkBody(link, recipe, selectedManualInstanceIdentifier,
                                   selectedManualInstanceIdentifiers, manualInstanceSelectionAnchorIdentifier,
                                   selectManualMarkerInstanceCallback);
            bAnyLinkCommitted = bAnyLinkCommitted || bAnyCommitted;
            DrawSectionEnd();
        }
        ImGui::PopID();
    }
    if (state.pendingDeleteLinkIdentifier >= 0) {
        DeleteMarkerLink(state.pendingDeleteLinkIdentifier, recipe.markerLinks, recipe.markers,
                         recipe.markerLayerBundles, recipe.markerLayers);
        state.pendingDeleteLinkIdentifier = -1;
    }
    NotifyPlacementChange(bAnyLinkCommitted, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen

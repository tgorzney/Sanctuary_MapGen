// MarkersTab_MarkerLinkResolvers_UI_Test.cpp — STEP241 acceptance for
// MarkersTab_MarkerLinkResolvers_UI.h's own pure read-and-resolve getters (ARCH §19.31 correction):
// Name at both the Bundle and Layer tiers, bHidden, iconScale, the grid-snap pair, and the symmetry
// pair. Pure logic only — no imgui frame needed, mirroring MarkersTab_Links_UI_Test.cpp's own
// RunEffectiveColorResolverChecks shape for the pre-existing color resolvers.
// STEP242 (ARCH §19.31's same-day follow-up amendment, governed field #7) extends this same file
// with RunEffectiveManualMarkerLayerLockedChecks, mirroring every check shape above exactly.
#include "MarkersTab_MarkerLinkResolvers_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// One Link, fully configured with distinct values on every governed field, reused by every check
// below so a bound resolver's return is unambiguously "the Link's value", never coincidentally
// equal to the struct default a fresh Bundle/Layer already starts with.
Params::MarkerLink MakeTestLink() {
    Params::MarkerLink link;
    link.identifier             = 7;
    link.name                   = "Link Name";
    link.bHidden                = true;
    link.iconScale               = 3.5f;
    link.bGridSnapEnabled        = true;
    link.gridSnapSizeWorldUnits  = 12.5f;
    link.bSymmetryEnabled        = false;
    link.symmetry.bSymmetryUseGlobal        = false;
    link.symmetry.symmetryMask              = Params::SymmetryAxis::MirrorAcrossX;
    link.symmetry.radialSymmetryRepeatCount = 5;
    link.bLocked                 = true;
    return link;
}

void RunEffectiveMarkerLayerBundleNameChecks() {
    const std::vector<Params::MarkerLink> links{ MakeTestLink() };

    Params::MarkerLayerBundle bound;
    bound.name = "Own Bundle Name"; bound.linkIdentifier = 7;
    Check(EffectiveMarkerLayerBundleName(bound, links) == "Link Name",
         "a Link-bound Bundle's effective name resolves from the LINK, not its own field");

    Params::MarkerLayerBundle unbound;
    unbound.name = "Own Bundle Name"; unbound.linkIdentifier = -1;
    Check(EffectiveMarkerLayerBundleName(unbound, links) == "Own Bundle Name",
         "an unbound Bundle's effective name resolves from its OWN field");

    Params::MarkerLayerBundle dangling;
    dangling.name = "Own Bundle Name"; dangling.linkIdentifier = 999;
    Check(EffectiveMarkerLayerBundleName(dangling, links) == "Own Bundle Name",
         "a dangling linkIdentifier (Constitution §6 soft-degrade) resolves from the Bundle's own field");
}

void RunEffectiveManualMarkerLayerNameChecks() {
    const std::vector<Params::MarkerLink> links{ MakeTestLink() };

    Params::MarkerInstanceLayer bound;
    bound.name = "Own Layer Name"; bound.linkIdentifier = 7;
    Check(EffectiveManualMarkerLayerName(bound, links) == "Link Name",
         "a Link-bound Layer's effective name resolves from the LINK, not its own field");
    Check(std::string(ManualMarkerLayerRowLabel(bound, links)) == "Link Name",
         "the two-arg ManualMarkerLayerRowLabel draws the SAME Link-resolved name");

    Params::MarkerInstanceLayer unbound;
    unbound.name = "Own Layer Name"; unbound.linkIdentifier = -1;
    Check(EffectiveManualMarkerLayerName(unbound, links) == "Own Layer Name",
         "an unbound Layer's effective name resolves from its OWN field");

    Params::MarkerInstanceLayer emptyUnbound;
    emptyUnbound.linkIdentifier = -1;
    Check(std::string(ManualMarkerLayerRowLabel(emptyUnbound, links)) == "Marker Layer",
         "an unbound, unnamed Layer's row label still falls back to \"Marker Layer\", never blank");
}

void RunEffectiveManualMarkerLayerHiddenChecks() {
    const std::vector<Params::MarkerLink> links{ MakeTestLink() };

    Params::MarkerInstanceLayer bound;
    bound.bHidden = false; bound.linkIdentifier = 7;
    Check(EffectiveManualMarkerLayerHidden(bound, links) == true,
         "a Link-bound Layer's effective bHidden resolves from the LINK (true), not its own (false) field");

    Params::MarkerInstanceLayer unbound;
    unbound.bHidden = false; unbound.linkIdentifier = -1;
    Check(EffectiveManualMarkerLayerHidden(unbound, links) == false,
         "an unbound Layer's effective bHidden resolves from its own field");
}

void RunEffectiveManualMarkerLayerIconScaleChecks() {
    const std::vector<Params::MarkerLink> links{ MakeTestLink() };

    Params::MarkerInstanceLayer bound;
    bound.iconScale = 1.0f; bound.linkIdentifier = 7;
    Check(EffectiveManualMarkerLayerIconScale(bound, links) == 3.5f,
         "a Link-bound Layer's effective iconScale resolves from the LINK, not its own field");

    Params::MarkerInstanceLayer unbound;
    unbound.iconScale = 1.0f; unbound.linkIdentifier = -1;
    Check(EffectiveManualMarkerLayerIconScale(unbound, links) == 1.0f,
         "an unbound Layer's effective iconScale resolves from its own field");
}

void RunEffectiveManualMarkerLayerGridSnapChecks() {
    const std::vector<Params::MarkerLink> links{ MakeTestLink() };

    Params::MarkerInstanceLayer bound;
    bound.bGridSnapEnabled = false; bound.gridSnapSizeWorldUnits = 1.0f; bound.linkIdentifier = 7;
    Check(EffectiveManualMarkerLayerGridSnapEnabled(bound, links) == true,
         "a Link-bound Layer's effective bGridSnapEnabled resolves from the LINK (true), not its own (false) field");
    Check(EffectiveManualMarkerLayerGridSnapSizeWorldUnits(bound, links) == 12.5f,
         "a Link-bound Layer's effective gridSnapSizeWorldUnits resolves from the LINK, not its own field");

    Params::MarkerInstanceLayer unbound;
    unbound.bGridSnapEnabled = false; unbound.gridSnapSizeWorldUnits = 1.0f; unbound.linkIdentifier = -1;
    Check(EffectiveManualMarkerLayerGridSnapEnabled(unbound, links) == false
          && EffectiveManualMarkerLayerGridSnapSizeWorldUnits(unbound, links) == 1.0f,
         "an unbound Layer's effective grid-snap pair resolves from its own fields");
}

void RunEffectiveManualMarkerLayerSymmetryChecks() {
    const std::vector<Params::MarkerLink> links{ MakeTestLink() };

    Params::MarkerInstanceLayer bound;
    bound.bSymmetryEnabled = true; bound.linkIdentifier = 7;
    Check(EffectiveManualMarkerLayerSymmetryEnabled(bound, links) == false,
         "a Link-bound Layer's effective bSymmetryEnabled resolves from the LINK (false), not its own (true) field");
    const Params::SymmetrySetting& resolvedSymmetry = EffectiveManualMarkerLayerSymmetry(bound, links);
    Check(!resolvedSymmetry.bSymmetryUseGlobal
          && resolvedSymmetry.symmetryMask == Params::SymmetryAxis::MirrorAcrossX
          && resolvedSymmetry.radialSymmetryRepeatCount == 5,
         "a Link-bound Layer's effective symmetry sub-record resolves from the LINK, not its own field");

    Params::MarkerInstanceLayer unbound;
    unbound.bSymmetryEnabled = true; unbound.linkIdentifier = -1;
    Check(EffectiveManualMarkerLayerSymmetryEnabled(unbound, links) == true,
         "an unbound Layer's effective bSymmetryEnabled resolves from its own field");
}

void RunEffectiveManualMarkerLayerLockedChecks() {
    const std::vector<Params::MarkerLink> links{ MakeTestLink() };

    Params::MarkerInstanceLayer bound;
    bound.bLocked = false; bound.linkIdentifier = 7;
    Check(EffectiveManualMarkerLayerLocked(bound, links) == true,
         "a Link-bound Layer's effective bLocked resolves from the LINK (true), not its own (false) field");

    Params::MarkerInstanceLayer unbound;
    unbound.bLocked = false; unbound.linkIdentifier = -1;
    Check(EffectiveManualMarkerLayerLocked(unbound, links) == false,
         "an unbound Layer's effective bLocked resolves from its own field");

    Params::MarkerInstanceLayer dangling;
    dangling.bLocked = false; dangling.linkIdentifier = 999;
    Check(EffectiveManualMarkerLayerLocked(dangling, links) == false,
         "a dangling linkIdentifier (Constitution §6 soft-degrade) resolves from the Layer's own field");
}

} // namespace

int main() {
    RunEffectiveMarkerLayerBundleNameChecks();
    RunEffectiveManualMarkerLayerNameChecks();
    RunEffectiveManualMarkerLayerHiddenChecks();
    RunEffectiveManualMarkerLayerIconScaleChecks();
    RunEffectiveManualMarkerLayerGridSnapChecks();
    RunEffectiveManualMarkerLayerSymmetryChecks();
    RunEffectiveManualMarkerLayerLockedChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

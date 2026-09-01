// MapExporter_MarkerLink_IO.cpp — see the header for the full contract.
#include "MapExporter_MarkerLink_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

// `MarkerLinks` — SanGen-owned cross-Marker-Type grouping/color-override tag, top-level PascalCase
// array (ARCH §19.28/§19.30), a fresh sibling of `MarkerLayerBundles`/`MarkerGroups`. `Identifier`
// spelled in full per §1.9 (matches `MarkerLayerBundles[i].Identifier`'s own spelling, NOT the
// pre-§1.9 `MarkerGroups[i].Id` legacy defect). `Color` serializes as the object `{r,g,b,a}`, the
// same shape every other SanGen-owned color field backing a C++ `float color[4]` already uses
// (PropGroups/DecalGroups/MarkerGroups' own `Color`).
//
// STEP243: the 7 fields STEP241/242 added to `Params::MarkerLink` mirror `MarkerGroups[]`'s own
// identical fields (BuildMarkerGroupsJson in MapExporter_Markers_IO.cpp), spelling included.
// `symmetry` (Params::SymmetrySetting) is NOT wrapped in a `Symmetry` sub-object — it flattens to
// sibling `SymmetryUseGlobal`/`SymmetryMask`/`RadialSymmetryRepeatCount` keys, the same shape
// `MarkerGroups[].symmetry` already uses; `bSymmetryEnabled` is the separate `SymmetryEnabled` key,
// distinct from `symmetry.bSymmetryUseGlobal`.
nlohmann::ordered_json BuildMarkerLinksJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markerLinks = nlohmann::ordered_json::array();
    for (const Params::MarkerLink& link : recipe.markerLinks) {
        nlohmann::ordered_json linkJson;
        linkJson["Identifier"]           = link.identifier;
        linkJson["Name"]                 = link.name;
        linkJson["ColorOverrideEnabled"] = link.bColorOverrideEnabled;
        linkJson["Color"] = { { "r", link.color[0] }, { "g", link.color[1] },
                              { "b", link.color[2] }, { "a", link.color[3] } };
        linkJson["Hidden"] = link.bHidden;
        linkJson["IconScale"] = link.iconScale;
        linkJson["GridSnapEnabled"] = link.bGridSnapEnabled;
        linkJson["GridSnapSizeWorldUnits"] = link.gridSnapSizeWorldUnits;
        linkJson["SymmetryEnabled"] = link.bSymmetryEnabled;
        linkJson["SymmetryUseGlobal"] = link.symmetry.bSymmetryUseGlobal;
        linkJson["SymmetryMask"] = link.symmetry.symmetryMask;
        linkJson["RadialSymmetryRepeatCount"] = link.symmetry.radialSymmetryRepeatCount;
        linkJson["Locked"] = link.bLocked;
        markerLinks.push_back(linkJson);
    }
    return markerLinks;
}

} // namespace Io
} // namespace SanmapGen

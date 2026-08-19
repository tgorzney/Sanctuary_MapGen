// Application_AssetBridge_UI.h — the one unit that legally sees IO and UI at once: sanpack ->
// atlas -> residency -> `Ui::IconAtlasManifest` bridge. Layer: UI. A member file of
// Application_UI.h (ARCH §1.5), matching the exact pattern already used for
// Application_HostedSettings_UI.h.
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "IconGridWidget_UI.h"
#include "../io/AssetAtlasCache_IO.h"
#include "../io/SanpackReader_IO.h"
#include "../sys/AtlasResidency_SYS.h"

namespace SanmapGen {
namespace Io { struct UnknownImportBag; }
namespace Ui {

struct ApplicationAssetBridge {
    Sys::AtlasResidency           atlasResidency;
    Io::AssetAtlasCache           assetAtlasCache;
    // Long-lived blueprintPath reader, SEPARATE from AssetAtlasCache's own transient one. Fed down
    // into `tabState.files.assetPack` ONLY on a successful Open()+ReadCentralDirectoryOnce() for
    // the current sanpackPath (Application_Assets_UI.cpp LoadAssetAtlas() — the load-bearing rule).
    Io::SanpackReader              assetPackReader;
    // STEP24_ImportNeverRefuses_IO ruling 4/6: the Files tab's Unknown-Import passthrough, OWNED
    // here (the same load-edit-save-session lifetime as `recipe`), fed down into `tabState.files.
    // unknownImportData` once, in the constructor (Application_UI.cpp). Pimpl'd behind
    // `std::unique_ptr` to an incomplete type — the SAME pattern `gpuResourceManager` above already
    // uses — because `Io::UnknownImportBag` embeds a real `nlohmann::json`, and this header must
    // stay includable by every UI/App translation unit WITHOUT dragging nlohmann's headers along
    // (several do not link `nlohmann_json` at all — Constitution §1's IO-only JSON homing).
    std::unique_ptr<Io::UnknownImportBag> unknownImportData;
    IconAtlasManifest             iconManifest;
    std::vector<std::string>      iconTemplateIdentifiers;   // iconId -> `tpId` side table
    std::string                   assetStatusMessage = "No sanpack loaded.";
    char                          sanpackPath[260] = { 0 };
    bool bAssetLoadRequested  = false;
    bool bAssetLoadAnnounced  = false;
};

} // namespace Ui
} // namespace SanmapGen

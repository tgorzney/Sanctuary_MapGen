// ApplicationShell_IconBridge_UI_Test.cpp — M5-7 acceptance, part 3: the bridge M5-6 flagged.
// A real sanpack goes through the real M5-4 pipeline inside the SHELL (Application::LoadAssetAtlas),
// and the string-keyed Io::AssetAtlas comes out the other side as the integer-keyed
// Ui::IconAtlasManifest the icon grid consumes — with an id -> template-identifier side table that
// resolves a pick back to the `tpId` a Params::ScatterTransform stores.
// No GL here, so no page is resident and every page identifier is zero: that is the documented
// degradation, and it must not stop the manifest or the id mapping from being correct.
#include "ApplicationShell_TestSupport_UI.h"
#include "../io/AssetPipeline_TestSupport_IO.h"
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace SanmapGen {
namespace Ui {
namespace {

const char* const shellTestSanpackPath   = "sangen_shell_test.sanpack";
const char* const shellTestCacheDirectory = "sangen_shell_test_cache";
constexpr int shellTestAtlasPageSide = 256;

void ConfigureShellForAtlas(Application& application) {
    application.Settings().atlasBuildSettings.pageWidth  = shellTestAtlasPageSide;
    application.Settings().atlasBuildSettings.pageHeight = shellTestAtlasPageSide;
    application.Settings().assetEntryFilter.extensions.push_back(".dds");
    std::snprintf(application.TabState().system.assetCacheDirectory,
                  sizeof(application.TabState().system.assetCacheDirectory), "%s",
                  shellTestCacheDirectory);
    application.SetSanpackPath(shellTestSanpackPath);
}

int IconIdOfTemplate(const Application& application, const char* templateIdentifier) {
    for (int iconId = 0; iconId < application.IconManifest().EntryCount(); ++iconId)
        if (application.TemplateIdentifierOfIcon(iconId) == templateIdentifier) return iconId;
    return -1;
}

void RunManifestShapeChecks(Application& application) {
    const IconAtlasManifest& manifest = application.IconManifest();
    Check(manifest.EntryCount() > 0, "the shell published a manifest the icon grid can draw");
    bool bEveryIdIsItsIndex = true, bEveryPageIsInRange = true;
    for (int iconId = 0; iconId < manifest.EntryCount(); ++iconId) {
        if (manifest.entries[static_cast<std::size_t>(iconId)].iconId != iconId) bEveryIdIsItsIndex = false;
        const int atlasPage = manifest.entries[static_cast<std::size_t>(iconId)].atlasPage;
        if (atlasPage < 0 || atlasPage >= static_cast<int>(manifest.pageTextureIdentifiers.size()))
            bEveryPageIsInRange = false;
    }
    Check(bEveryIdIsItsIndex, "every icon id is its stable index into the atlas entries");
    Check(bEveryPageIsInRange, "every entry names a page the manifest carries");
    Check(!manifest.pageTextureIdentifiers.empty(),
          "the manifest carries one draw identifier per atlas page");
    Check(manifest.PageTextureIdentifier(0) == 0ull,
          "with no GL seam no page is resident, and the grid reads zero as nothing to draw");
    Check(application.TemplateIdentifierOfIcon(-1).empty(),
          "an out-of-range icon id resolves to no template identifier");
}

// The pick, end to end: the grid reports an id, the shell resolves it to the template identifier
// and stores it on the rule the Markers tab has selected — the tab never learns the atlas exists.
void RunSelectionResolutionChecks(Application& application) {
    const int iconId = IconIdOfTemplate(application, "ucl3001");
    Check(iconId >= 0, "the known unit thumbnail resolved to a template identifier");
    if (iconId < 0) return;

    application.TabState().markers.iconGridState.selectedIconId = iconId;
    application.ResolveIconSelections();
    Params::MarkerRule* const rule =
        SelectedMarkerRule(application.Recipe().markerRules, application.TabState().markers);
    Check(rule != nullptr, "the default recipe has a marker rule to write");
    if (rule == nullptr) return;
    Check(std::strcmp(rule->transform.templateIdentifier, "ucl3001") == 0,
          "the picked icon became the rule's template id");
    Check(application.Driver().NeedsMapUpdate(), "and that is a stage-owned edit, so it regenerates");
    Check(application.Driver().OwningStageName() == "Placement", "claimed by the placement stage");

    application.Driver().Refresh();
    application.ResolveIconSelections();
    Check(!application.Driver().NeedsMapUpdate(),
          "re-drawing the same selection writes nothing and trips no flag");
}

} // namespace

void RunShellIconBridgeChecks() {
    AssetPipelineTest::SyntheticSanpack layout;
    layout.sanpackPath = shellTestSanpackPath;
    std::error_code errorCode;
    std::filesystem::remove_all(shellTestCacheDirectory, errorCode);
    Check(AssetPipelineTest::WriteSyntheticSanpack(layout), "the synthetic sanpack was written");

    Application application(TestApplicationSettings());
    ConfigureShellForAtlas(application);
    Check(application.LoadAssetAtlas(), "the shell loaded the sanpack through the M5-4 pipeline");
    RunManifestShapeChecks(application);
    RunSelectionResolutionChecks(application);

    std::filesystem::remove(shellTestSanpackPath, errorCode);
    std::filesystem::remove_all(shellTestCacheDirectory, errorCode);
}

} // namespace Ui
} // namespace SanmapGen

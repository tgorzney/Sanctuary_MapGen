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

// STEP52: prove IconPairingLookup() is actually populated by the real LoadAssetAtlas() path, not
// only unit-testable against a hand-built vector in isolation (IconAtlasPairing_UI_Test.cpp).
void RunIconPairingLookupWiringChecks(Application& application) {
    const int expectedIconId = IconIdOfTemplate(application, "ucl3001");
    Check(expectedIconId >= 0, "the known unit thumbnail resolved to a template identifier");
    if (expectedIconId < 0) return;

    const Ui::IconIdentifierPairing pairing = application.IconPairingLookup().Resolve("ucl3001");
    Check(pairing.thumbnailIconId == expectedIconId,
          "the pairing lookup's thumbnail id matches the manifest's own id for the same template");
    Check(pairing.strategicIconId == Ui::kInvalidIconId,
          "no authored strategic icon exists yet, so it resolves to the invalid sentinel");
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
        SelectedMarkerRule(application.Recipe().markerRuleLayers, application.TabState().markers);
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

// STEP114 — Application::ApplyIconSelectionToStringField, driven through the same public
// ResolveIconSelections()/manual-marker-bridge shape RunSelectionResolutionChecks above already
// exercises for MarkerRule (the function itself is private, same posture as ApplyIconSelection).
// Covers: a fresh, resolvable id writes the string; a repeated id is a no-op; a negative
// (default -1) id is a no-op; an unresolvable id is a no-op — mirroring ApplyIconSelection's own
// no-op-on-miss test case.
void RunManualMarkerIconOverrideBridgeChecks(Application& application) {
    const int iconId = IconIdOfTemplate(application, "ucl3001");
    Check(iconId >= 0, "the known unit thumbnail resolved to a template identifier");
    if (iconId < 0) return;

    Params::MarkerInstanceGroup group;
    group.name = "Alloys";
    Params::MarkerTransform transform;
    group.transforms.push_back(transform);
    application.Recipe().markers.push_back(group);
    application.TabState().markers.manual.selectedGroupIndex = 0;
    application.TabState().markers.manual.selectedInstanceIndex = 0;

    Params::MarkerTransform* const manualMarkerInstance =
        SelectedManualMarkerInstance(application.Recipe().markers, application.TabState().markers.manual);
    Check(manualMarkerInstance != nullptr, "the seeded manual marker instance is reachable");
    if (manualMarkerInstance == nullptr) return;

    // Negative (default -1, no pick made yet): a no-op, the field stays empty.
    application.ResolveIconSelections();
    Check(manualMarkerInstance->iconNameOverride.empty(),
          "a negative selectedIconId (the default, no pick made) writes nothing");

    // Unresolvable: an id past the manifest's own entry count resolves to no template identifier.
    application.TabState().markers.manual.iconOverrideGridState.selectedIconId =
        application.IconManifest().EntryCount() + 1000;
    application.ResolveIconSelections();
    Check(manualMarkerInstance->iconNameOverride.empty(),
          "an unresolvable icon id writes nothing and leaves the field untouched");

    // Fresh, resolvable: writes the string once.
    application.TabState().markers.manual.iconOverrideGridState.selectedIconId = iconId;
    application.ResolveIconSelections();
    Check(manualMarkerInstance->iconNameOverride == "ucl3001",
          "a fresh, resolvable selectedIconId writes the resolved template identifier");

    // Repeated: re-drawing the same selection writes nothing (bRecipeMoved-style no-op).
    manualMarkerInstance->iconNameOverride = "HandTyped";   // prove the repeat truly no-ops
    application.ResolveIconSelections();
    Check(manualMarkerInstance->iconNameOverride == "HandTyped",
          "a repeated (== lastIconId) selectedIconId leaves a hand-typed value untouched");
}

// STEP121 — the Markers tab Global section's three per-category icon bridges, mirroring
// RunManualMarkerIconOverrideBridgeChecks above exactly.
void RunGlobalMarkerIconBridgeChecks(Application& application) {
    const int iconId = IconIdOfTemplate(application, "ucl3001");
    Check(iconId >= 0, "the known unit thumbnail resolved to a template identifier");
    if (iconId < 0) return;

    // Negative (default -1, no pick made yet): a no-op for all three rows.
    application.ResolveIconSelections();
    Check(application.Recipe().globalMarkerSettings.iconNameAlloy == "Alloy"
          && application.Recipe().globalMarkerSettings.iconNamePlasma == "Plasma"
          && application.Recipe().globalMarkerSettings.iconNameSpawn == "Spawn",
          "no pick made yet leaves all three GlobalMarkerSettings icon names at their defaults");

    // Fresh, resolvable pick on the Alloy row (index 0) only.
    application.TabState().markers.globals.scaleRows[0].iconGridState.selectedIconId = iconId;
    application.ResolveIconSelections();
    Check(application.Recipe().globalMarkerSettings.iconNameAlloy == "ucl3001",
          "a fresh pick on row 0 writes iconNameAlloy");
    Check(application.Recipe().globalMarkerSettings.iconNamePlasma == "Plasma"
          && application.Recipe().globalMarkerSettings.iconNameSpawn == "Spawn",
          "and leaves the other two rows' fields untouched");
    // STEP121 coder deviation from the ticket's drafted text: `GlobalMarkerSettings` is not hashed
    // by ANY stage's own ComputeParameterHash (Placement_Hash_PROC.cpp — the only stage close
    // enough to own it never reads it), so per PreviewDriver_PIPELINE.cpp's OWN derivation rule
    // ("an edit no stage claims cannot alter a generated field ... the composite alone services
    // it") this edit trips a PREVIEW-ONLY refresh, not `NeedsMapUpdate()`. Asserting
    // `NeedsMapUpdate()` here would fail against the real dirty-hash model and, if made to pass by
    // wiring GlobalMarkerSettings into a stage hash instead, would recreate the exact "cheap tweak
    // triggers a full regen" defect MarkersTab_Globals_UI.h's own SCOPE NOTE 1 names — the same
    // reason the sibling RunManualMarkerIconOverrideBridgeChecks above asserts no tier at all for
    // its own render-only field.
    Check(application.Driver().NeedsPreviewRender(),
          "a real recipe edit trips a preview-only refresh (GlobalMarkerSettings owns no stage's hash)");

    // Repeated: re-drawing the same selection writes nothing.
    application.Driver().Refresh();
    application.Recipe().globalMarkerSettings.iconNameAlloy = "HandTyped";
    application.ResolveIconSelections();
    Check(application.Recipe().globalMarkerSettings.iconNameAlloy == "HandTyped",
          "a repeated (== lastGlobalAlloyIconId) selection leaves a hand-typed value untouched");

    // Plasma (index 1) and Spawn (index 2) rows bridge independently, same shape.
    application.TabState().markers.globals.scaleRows[1].iconGridState.selectedIconId = iconId;
    application.TabState().markers.globals.scaleRows[2].iconGridState.selectedIconId = iconId;
    application.ResolveIconSelections();
    Check(application.Recipe().globalMarkerSettings.iconNamePlasma == "ucl3001"
          && application.Recipe().globalMarkerSettings.iconNameSpawn == "ucl3001",
          "the Plasma and Spawn rows each bridge to their own GlobalMarkerSettings field");
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
    RunIconPairingLookupWiringChecks(application);
    RunManualMarkerIconOverrideBridgeChecks(application);
    RunGlobalMarkerIconBridgeChecks(application);

    std::filesystem::remove(shellTestSanpackPath, errorCode);
    std::filesystem::remove_all(shellTestCacheDirectory, errorCode);
}

} // namespace Ui
} // namespace SanmapGen

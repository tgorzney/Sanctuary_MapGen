// FilesTab_Roundtrip_UI_Test.cpp — the end-to-end half of the Files / Save acceptance test:
// the injected SupCom seam, and Export All -> Open Sanmap through a real scratch folder. This is
// PARITY_BACKLOG PB-1/PB-2 closed in one check — the tab writes a map and then loads the very
// folder it just wrote. Still headless: no imgui frame, no window, no GL context.
#include "FilesTab_TestSupport_UI.h"
#include "FilesTab_UI.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace FilesTabTest {
namespace {

// The injected seam (FilesTab_UI.h SCOPE NOTE 1): with something bound the row works end to end.
bool StubSupComImport(void* userData, const char* luaFilePath, Params::MapRecipe& outRecipe,
                      std::string& outLog) {
    *static_cast<int*>(userData) += 1;
    outRecipe.geometry.mapSize = 64;
    outLog = std::string("stub imported ") + luaFilePath;
    return true;
}

void CheckTheInjectedSupComSeamIsDriven() {
    Ui::FilesTabState state;
    Params::MapRecipe recipe;
    int callCount = 0;
    state.ImportSupComLua = &StubSupComImport;
    state.supComUserData  = &callCount;
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ImportSupComLua, state, recipe, nullptr),
          "a bound importer with no path is still refused");
    Check(callCount == 0, "and the seam was not called");

    state.supComLuaPath = "D:/maps/scenario_save.lua";
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ImportSupComLua, state, recipe, nullptr),
          "with a path the seam runs");
    Check(callCount == 1 && recipe.geometry.mapSize == 64, "and its recipe reaches the caller");
    Check(state.debugLog.find("stub imported") != std::string::npos,
          "with its own log folded into the panel");
}

void CheckEveryExportActionRuns(Ui::FilesTabState& state, Params::MapRecipe& recipe,
                                Data::MapFields& fields) {
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ExportAll, state, recipe, &fields),
          "Export All writes the map through the tab");
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ExportHeightmapRaw, state, recipe, &fields),
          "and each single-file export runs on its own, creating its Textures folder");
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ExportSlopeImage, state, recipe, &fields),
          "including the slope PNG");
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ExportFlowImage, state, recipe, &fields),
          "the flow PNG");
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ExportStratumMasks, state, recipe, &fields),
          "and the stratum TGAs");
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ExportSanmapOnly, state, recipe, &fields),
          "as does the document-only export");
}

void CheckTheExportThenOpenRoundTrip() {
    Ui::FilesTabState state;
    state.exportFolderPath = ScratchFolderPath("SanGenFilesTabTest");
    state.exportOptions.mapName = "mapdef";

    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 8;
    recipe.geometry.seed = 4242u;
    Data::MapFields fields;
    fields.Resize(recipe.geometry.VertexSize(), 0.0f);
    fields.heightfield.Set(2, 2, 1.0f);
    CheckEveryExportActionRuns(state, recipe, fields);

    Params::MapRecipe loadedRecipe;
    Data::MapFields loadedFields;
    state.sanmapPath = state.exportFolderPath;
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::OpenSanmap, state, loadedRecipe, &loadedFields),
          "and Open Sanmap reads the very folder the tab just wrote");
    Check(loadedRecipe.geometry.mapSize == 8 && loadedRecipe.geometry.seed == 4242u,
          "recovering the recipe");
    Check(loadedFields.IsSized() && loadedFields.heightfield.Get(2, 2) > 0.99f,
          "and the baked heightfield with it");

    state.bLoadBakedFieldsOnImport = false;
    Data::MapFields skippedFields;
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::OpenSanmap, state, loadedRecipe, &skippedFields),
          "with the texture toggle off the recipe still loads");
    Check(!skippedFields.IsSized(), "and no texture was read");
}

} // namespace

void RunRoundTripTests() {
    CheckTheInjectedSupComSeamIsDriven();
    CheckTheExportThenOpenRoundTrip();
}

} // namespace FilesTabTest
} // namespace SanmapGen

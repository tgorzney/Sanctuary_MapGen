// FilesTab_MigrationDialog_Draw_UI.cpp — see the header for the split rationale. Layer: UI.
#include "FilesTab_MigrationDialog_Draw_UI.h"
#include "FilesTab_UI.h"
#include "MigrationReconciliationDialog_UI.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

void DrawFilesTabMigrationReconciliationDialog(FilesTabState& state, Params::MapRecipe& recipe,
                                               Data::MapFields* fields,
                                               Pipeline::PreviewDriver* previewDriver,
                                               std::vector<Data::BakedLayerImage>* outBakedLayerImages,
                                               std::vector<Data::StratumArt>* outStratumArt) {
    const MigrationReconciliationDialogChange change = DrawMigrationReconciliationDialog(
        "filesTabMigrationReconciliation", state.migrationDialogState);
    if (!change.bApplyClicked) return;   // Close: dismiss, no re-read, no mutation (ruling 3)
    const bool bSucceeded = RunSelectiveMigrationImport(
        state, recipe, fields, SelectedMigrationNames(state.migrationDialogState), outBakedLayerImages,
        outStratumArt);
    if (bSucceeded && previewDriver != nullptr) previewDriver->RequestMapUpdate();
}

} // namespace Ui
} // namespace SanmapGen

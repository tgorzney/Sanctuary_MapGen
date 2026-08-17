// ApplicationShell_Visibility_UI_Test.cpp — tab-rebuild WO E acceptance, part 2: the left column's
// `[O]`/`[ ]` state ACTUALLY changes what the preview composites. Every check drives a REAL
// Ui::Application, so what passes is the shell's own wiring: its default composition, its
// construction-time apply, and the mapping in Application_Visibility_UI.h.
// Cpu twin throughout: no window, no GL.
#include "ApplicationShell_TestSupport_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

bool IsLayerEnabled(Application& application, PreviewLayerKind kind) {
    const PreviewFieldLayer* const layer =
        PreviewFieldLayerOfKind(application.Composite().Settings(), kind);
    return layer != nullptr && layer->bEnabled;
}

// The composition has to CARRY a layer per overlay tab, or the tab and the row would both be
// editing nothing (TerrainOverlayTab_UI.h resolves by kind, never by index).
void CheckEveryToggledFieldHasALayer(Application& application) {
    for (const ApplicationPanelEntry& entry : applicationPanelEntries) {
        if (entry.visibilityTarget != PreviewVisibilityTarget::FieldLayer) continue;
        Check(PreviewFieldLayerOfKind(application.Composite().Settings(), entry.fieldLayerKind)
                  != nullptr,
              entry.label);
    }
}

void SetPanelVisible(Application& application, ApplicationPanel panel, bool bVisible) {
    application.TabState().visibility.bPanelVisible[ApplicationPanelIndexOf(panel)] = bVisible;
    ApplyPanelVisibility(application.TabState().visibility, application.Composite().Settings());
}

} // namespace

void RunShellVisibilityChecks() {
    Application application;
    PrepareShellForTest(application);
    CheckEveryToggledFieldHasALayer(application);

    // The construction-time apply already ran: the column and the image agree before a click.
    Check(IsLayerEnabled(application, PreviewLayerKind::HeightRamp),
          "the Heightmap row starts ticked and its layer starts enabled");
    Check(!IsLayerEnabled(application, PreviewLayerKind::Slope),
          "the Slope overlay starts hidden, as its row does");

    SetPanelVisible(application, ApplicationPanel::Heightmap, false);
    Check(!IsLayerEnabled(application, PreviewLayerKind::HeightRamp),
          "clearing the Heightmap row disables the height layer");
    SetPanelVisible(application, ApplicationPanel::Slope, true);
    Check(IsLayerEnabled(application, PreviewLayerKind::Slope),
          "and setting the Slope row enables the slope overlay");

    // The image must actually change, not merely the flag: generate once with the height layer on,
    // once with it off, and compare the composited texels.
    SetPanelVisible(application, ApplicationPanel::Heightmap, true);
    SetPanelVisible(application, ApplicationPanel::Slope, false);
    application.ServiceDirtyTier();
    const unsigned long long checksumWithHeight = CompositeImageChecksum(application);
    SetPanelVisible(application, ApplicationPanel::Heightmap, false);
    application.Driver().NotifyParametersChanged();
    application.ServiceDirtyTier();
    Check(CompositeImageChecksum(application) != checksumWithHeight,
          "hiding a preview layer changes the composited image, not just the flag");

    // The three placement rows share the composite's one entity pass: it is on while ANY of them is.
    SetPanelVisible(application, ApplicationPanel::Markers, false);
    Check(!application.Composite().Settings().bEntitiesEnabled,
          "clearing the last placement row switches the entity marks off");
    SetPanelVisible(application, ApplicationPanel::Props, true);
    Check(application.Composite().Settings().bEntitiesEnabled,
          "and setting any placement row switches them back on");
}

} // namespace Ui
} // namespace SanmapGen

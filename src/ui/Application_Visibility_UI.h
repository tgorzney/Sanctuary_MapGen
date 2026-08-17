// Application_Visibility_UI.h — the left column's `[O]`/`[ ]` state and the ONE pure function that
// pushes it into the preview composite. Layer: UI. Accuracy class: Visual.
// A member file of Application_UI.h (ARCH §1.5). No imgui here: the mapping is pure, so "the toggle
// actually changes what the preview composites" is asserted headlessly.
//
// It stores NO second copy of a composite setting. The flags are the row state the user clicks; the
// apply below writes them onto the single home each one has — `PreviewFieldLayer::bEnabled` for a
// colorized field, `PreviewCompositeSettings::bEntitiesEnabled` for the placement marks.
//
// SCOPE NOTE (ARCH §8.4 — a coder never invents a missing field; reported, not invented): six rows
// keep the v1 toggle but drive nothing, because the composite carries no layer for them — Symmetry,
// Detail Normal, Tint, Holes, Smoothness and Atmosphere have no baked field in `Data::MapFields` and
// no `PreviewLayerKind`. Their `[O]` is held here so the column matches v1 and a future overlay
// stage can bind to it; making one of them paint needs that stage plus its layer kind, which is a
// work-order, not a side effect of the host shell.
#pragma once
#include "Application_Panels_UI.h"
#include "TerrainOverlayTab_UI.h"

namespace SanmapGen {
namespace Ui {

// One flag per left-column row, seeded from the catalogue's own defaults so the shell holds no
// second default list.
struct ApplicationVisibilityState {
    bool bPanelVisible[kApplicationPanelCount];

    ApplicationVisibilityState() {
        for (int panelIndex = 0; panelIndex < kApplicationPanelCount; ++panelIndex)
            bPanelVisible[panelIndex] = applicationPanelEntries[panelIndex].bVisibleByDefault;
    }
};

// The flag a panel's row carries, or false for a value outside the enum.
inline bool IsApplicationPanelVisible(const ApplicationVisibilityState& visibility,
                                      ApplicationPanel panel) {
    const int panelIndex = ApplicationPanelIndexOf(panel);
    return panelIndex < 0 ? false : visibility.bPanelVisible[panelIndex];
}

// True when ANY of the placement rows is ticked. The composite has one entity pass, not three, so
// the marks are drawn while at least one of Markers/Armies/Props asks for them.
inline bool AreEntityPanelsVisible(const ApplicationVisibilityState& visibility) {
    for (int panelIndex = 0; panelIndex < kApplicationPanelCount; ++panelIndex)
        if (applicationPanelEntries[panelIndex].visibilityTarget == PreviewVisibilityTarget::Entities
                && visibility.bPanelVisible[panelIndex]) return true;
    return false;
}

// The column's state -> the composite. Reports whether anything actually moved, so re-applying an
// unchanged column costs no recomposite (the driver only recolors when told the presentation moved).
inline bool ApplyPanelVisibility(const ApplicationVisibilityState& visibility,
                                 PreviewCompositeSettings& compositeSettings) {
    bool bCompositeMoved = false;
    for (int panelIndex = 0; panelIndex < kApplicationPanelCount; ++panelIndex) {
        const ApplicationPanelEntry& entry = applicationPanelEntries[panelIndex];
        if (entry.visibilityTarget != PreviewVisibilityTarget::FieldLayer) continue;
        PreviewFieldLayer* const layer = PreviewFieldLayerOfKind(compositeSettings, entry.fieldLayerKind);
        if (layer == nullptr || layer->bEnabled == visibility.bPanelVisible[panelIndex]) continue;
        layer->bEnabled = visibility.bPanelVisible[panelIndex];
        bCompositeMoved = true;
    }
    const bool bEntitiesVisible = AreEntityPanelsVisible(visibility);
    if (compositeSettings.bEntitiesEnabled != bEntitiesVisible) {
        compositeSettings.bEntitiesEnabled = bEntitiesVisible;
        bCompositeMoved = true;
    }
    return bCompositeMoved;
}

} // namespace Ui
} // namespace SanmapGen

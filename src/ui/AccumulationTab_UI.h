// AccumulationTab_UI.h — the Accumulation tab: the ordered-spillover valley fill, its threshold,
// and the accumulation overlay + ramp. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "5 · Accumulation" (overlay/ramp -> PreviewRender, the rest -> MapUpdate).
//
// WHERE THESE SETTINGS LIVE. v1's "Accurate Simultaneous Accumulation" and "Spillover Threshold"
// are the erosion stage's CPU-only ordered accumulation DAG, and in v2 they sit in
// `Proc::ErosionLayerSettings` — one record PER STRATUM. v1 had one global pair, so this tab
// writes the same value onto every stratum's record and reads slot 0 back, exactly as the
// Heightmap tab's Global Gravity does. That is a BULK WRITE onto the one field, not a second
// store: no other tab exposes these three, so there is no rival control (ARCH §4).
//
// Reached through PIPELINE (`GenerationAssembler::Erosion()`), the interim contract that header
// states until the erosion PARAMS home exists. UI includes no PROC stage of its own.
//
// SCOPE NOTE: the values are not serialized — they live on the stage, like every other erosion
// constant, until `ErosionFlow_PARAMS` lands (ARCH §5.2/§7.1).
#pragma once
#include "SliderScalar_UI.h"
#include "TerrainOverlayTab_UI.h"
#include "GradientEditorWidget_UI.h"
#include "LayerEditor_Erosion_UI.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// The three numbers the accumulation DAG carries, gathered so the bulk write is one call and one
// assertion rather than three of each.
struct AccumulationSpilloverSettings {
    bool  bAccurateSimultaneousAccumulation = false;
    float spilloverThreshold = 0.05f;
    float spilloverShare     = 0.5f;
};

// Slot 0's values — what the tab shows. Every slot carries the same numbers once the tab has
// written them; slot 0 is simply the one it reads back from.
inline AccumulationSpilloverSettings AccumulationSettingsOfAssembler(
        Pipeline::GenerationAssembler& generationAssembler) {
    const Proc::ErosionLayerSettings& erosionSettings = generationAssembler.Erosion().LayerSettings(0);
    AccumulationSpilloverSettings settings;
    settings.bAccurateSimultaneousAccumulation = erosionSettings.bAccurateSimultaneousAccumulation;
    settings.spilloverThreshold = erosionSettings.spilloverThreshold;
    settings.spilloverShare     = erosionSettings.spilloverShare;
    return settings;
}

// Writes the three onto EVERY stratum's erosion record. Reports whether any record moved, so
// re-applying what is already there costs no regeneration.
inline bool ApplyAccumulationSettingsToErosion(const AccumulationSpilloverSettings& settings,
                                               Pipeline::GenerationAssembler& generationAssembler) {
    bool bMoved = false;
    for (int stratumIndex = 0; stratumIndex < kLayerEditorStratumCount; ++stratumIndex) {
        Proc::ErosionLayerSettings& erosionSettings =
            generationAssembler.Erosion().LayerSettings(stratumIndex);
        if (erosionSettings.bAccurateSimultaneousAccumulation
                != settings.bAccurateSimultaneousAccumulation) {
            erosionSettings.bAccurateSimultaneousAccumulation =
                settings.bAccurateSimultaneousAccumulation;
            bMoved = true;
        }
        if (erosionSettings.spilloverThreshold != settings.spilloverThreshold) {
            erosionSettings.spilloverThreshold = settings.spilloverThreshold;
            bMoved = true;
        }
        if (erosionSettings.spilloverShare != settings.spilloverShare) {
            erosionSettings.spilloverShare = settings.spilloverShare;
            bMoved = true;
        }
    }
    return bMoved;
}

// Caller-owned tab state. The settings mirror is refreshed from slot 0 whenever nothing is
// mid-drag, so it is a view of the stage rather than a second home.
struct AccumulationTabState {
    ScalarSliderRange spilloverThresholdRange{ 0.0f, 1.0f, 0.0f };
    ScalarSliderRange spilloverShareRange{ 0.0f, 1.0f, 0.0f };
    RealtimeToggle    spilloverThresholdToggle;
    RealtimeToggle    spilloverShareToggle;
    GradientEditorState gradientEditor;
    AccumulationSpilloverSettings spilloverSettings;
};

void DrawAccumulationTab(PreviewCompositeSettings& compositeSettings, AccumulationTabState& state,
                         Pipeline::GenerationAssembler* generationAssembler,
                         Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

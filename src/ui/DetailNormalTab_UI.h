// DetailNormalTab_UI.h — the Detail Normal tab: a show-overlay toggle, the detail-normal texture
// size, and the detail-normal layer stack. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "7 · Detail Normal".
//
// It is the shared MaskLayerTab plus ONE extra control, so the composition itself is still
// MaskLayerTab_UI's; only the size dropdown lives here.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing field; reported, not invented):
//  1. The stack has no recipe home and the show toggle is presentation — MaskLayerTab_UI.h
//     SCOPE NOTES 1 and 2 apply verbatim.
//  2. THE DETAIL-NORMAL SIZE has no PARAMS home in v2 (v1 kept `DetailNormalMapSize` on the god
//     object; the `.sanmap` writes it under `mapGeneratorData`). It is caller-owned tab state,
//     the same standing as the Heightmap tab's global gravity. A durable setting needs its own
//     work-order — the tab does not put it on someone else's PARAMS file.
#pragma once
#include "MaskLayerTab_UI.h"

namespace SanmapGen {
namespace Ui {

// The texture sizes the tab offers, and their labels in the same order (Combo_UI maps row->value).
// Deliberately its own table rather than the Heightmap tab's map-size one: the two lists agree
// today by coincidence, and a detail-normal texture size is not a map size (ARCH §1.1).
inline constexpr int kDetailNormalSizeCount = 5;
inline constexpr int detailNormalSizeValues[kDetailNormalSizeCount] = { 256, 512, 1024, 2048, 4096 };
inline const char* const detailNormalSizeLabels[kDetailNormalSizeCount] = {
    "256", "512", "1024", "2048", "4096"
};

// The offered size a state currently sits on, or -1 for a size the dropdown does not list.
inline int DetailNormalSizeIndexOf(int detailNormalSize) {
    for (int sizeIndex = 0; sizeIndex < kDetailNormalSizeCount; ++sizeIndex)
        if (detailNormalSizeValues[sizeIndex] == detailNormalSize) return sizeIndex;
    return -1;
}

inline int DetailNormalSizeAtIndex(int sizeIndex) {
    if (sizeIndex < 0 || sizeIndex >= kDetailNormalSizeCount) return 0;
    return detailNormalSizeValues[sizeIndex];
}

// Caller-owned tab state: the shared mask-layer state plus the size dropdown's row (SCOPE NOTE 2).
struct DetailNormalTabState {
    MaskLayerTabState maskLayerTab;
    int detailNormalSizeIndex = 2;            // 1024, the v1 default
};

// The size the dropdown row names, or the v1 default when the row points at nothing.
inline int DetailNormalSizeOf(const DetailNormalTabState& state) {
    const int detailNormalSize = DetailNormalSizeAtIndex(state.detailNormalSizeIndex);
    return detailNormalSize > 0 ? detailNormalSize : 1024;
}

void DrawDetailNormalTab(Params::LayerStack& detailNormalLayers, DetailNormalTabState& state,
                         Pipeline::GenerationAssembler* generationAssembler,
                         Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

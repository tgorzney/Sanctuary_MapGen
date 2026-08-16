// StratumsTab_Options_UI.h — the Stratums tab's pure option/label logic. Layer: UI.
// Accuracy class: Visual. The three things the tab must get right without an imgui frame: which
// dropdown row a stored NAME sits on, what the 3-state mask-mode button does next, and the label a
// stratum's collapsing header shows. All pure, so all headless-testable (WidgetHelpers_UI.h "THE
// SPLIT").
//
// WHY THE OPTION LISTS ARE BORROWED: the environment/material lists come from a `.sanpack`, and
// reading one is IO — a layer the UI may not depend on (ARCH §3.1). The host reads the pack and
// hands this tab a borrowed label table for the frame, exactly as ComboOptions is designed for.
#pragma once
#include <cstdio>
#include <string>
#include "Combo_UI.h"
#include "../params/GenerationEnums_PARAMS.h"
#include "../params/Stratum_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// The borrowed catalogues one frame of the tab draws its two dropdowns from. Null/zero simply
// means "no pack loaded", and the combos show their empty label rather than nothing at all.
struct StratumsTabAssetOptions {
    const char* const* environmentLabels = nullptr;
    int                environmentCount  = 0;
    const char* const* materialLabels    = nullptr;
    int                materialCount     = 0;
};

// The row a stored name sits on, or -1 when the pack no longer offers it — which is exactly what
// ComboOptions calls "nothing picked", so a sanpack swapped under a saved recipe shows an empty
// dropdown instead of silently re-pointing the stratum at a neighbouring material.
inline int StratumOptionIndexOf(const std::string& value, const char* const* labels, int count) {
    if (value.empty() || labels == nullptr) return -1;
    for (int optionIndex = 0; optionIndex < count; ++optionIndex)
        if (labels[optionIndex] != nullptr && value == labels[optionIndex]) return optionIndex;
    return -1;
}

// The label at a row, or an empty string for a row the table does not carry.
inline const char* StratumOptionLabelAt(int optionIndex, const char* const* labels, int count) {
    if (labels == nullptr || optionIndex < 0 || optionIndex >= count) return "";
    return labels[optionIndex] != nullptr ? labels[optionIndex] : "";
}

// --- The 3-state mask mode (Params::ImportedMaskMode), drawn as v1 drew it: one button that
// cycles. The order is the enumerator order, so the button and any dropdown agree.
inline constexpr int kImportedMaskModeCount = 3;

inline const char* const importedMaskModeLabels[kImportedMaskModeCount] = {
    "Mask: Disabled", "Mask: Procedural Start", "Mask: Static Override"
};

inline const char* ImportedMaskModeLabel(Params::ImportedMaskMode mode) {
    const int modeIndex = static_cast<int>(mode);
    if (modeIndex < 0 || modeIndex >= kImportedMaskModeCount) return importedMaskModeLabels[0];
    return importedMaskModeLabels[modeIndex];
}

// Disabled -> ProceduralStart -> StaticOverride -> Disabled. A mode outside the enum lands on
// Disabled rather than cycling off the end (Constitution §6).
inline Params::ImportedMaskMode NextImportedMaskMode(Params::ImportedMaskMode mode) {
    switch (mode) {
        case Params::ImportedMaskMode::Disabled:        return Params::ImportedMaskMode::ProceduralStart;
        case Params::ImportedMaskMode::ProceduralStart: return Params::ImportedMaskMode::StaticOverride;
        case Params::ImportedMaskMode::StaticOverride:  return Params::ImportedMaskMode::Disabled;
        default:                                        return Params::ImportedMaskMode::Disabled;
    }
}

// "Stratum 3 - Grass", or just "Stratum 3" for a stratum nobody has named yet — never an empty
// header (Constitution §6). Writes into the caller's buffer; nothing here allocates per frame.
inline void FormatStratumSectionLabel(int stratumIndex, const Params::Stratum& stratum,
                                      char* outLabel, int labelCapacity) {
    if (outLabel == nullptr || labelCapacity <= 0) return;
    if (stratum.appearance.name.empty())
        std::snprintf(outLabel, static_cast<std::size_t>(labelCapacity), "Stratum %d", stratumIndex);
    else
        std::snprintf(outLabel, static_cast<std::size_t>(labelCapacity), "Stratum %d - %s",
                      stratumIndex, stratum.appearance.name.c_str());
}

} // namespace Ui
} // namespace SanmapGen

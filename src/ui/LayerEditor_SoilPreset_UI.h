// LayerEditor_SoilPreset_UI.h — the Soil Physics preset menu. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "§ Layer Editor / Soil Physics": Presets menu {Bedrock, Rock, Clay, Dirt, Mud,
// Sand}, each a one-click fill of the five soil numbers.
//
// NOT A SETTINGS HOME (ARCH §7.1 — there is exactly one per-stratum settings type). `SoilPresetValues`
// is a five-number PAYLOAD a menu pick copies onto whichever record the caller edits; nothing
// retains it, nothing reads it back, and no stage may take it. The moment a preset lands, the
// values live in the stratum's own soil-physics record and the preset is forgotten — which is
// also why picking one is reported as "the caller's values moved", not as a stored mode.
#pragma once

namespace SanmapGen {
namespace Ui {

enum class SoilPreset : int { Bedrock, Rock, Clay, Dirt, Mud, Sand, Count };

inline constexpr int kSoilPresetCount = static_cast<int>(SoilPreset::Count);

// The menu labels, in enumerator order — the option list a Ui::Combo/menu borrows.
inline const char* const soilPresetLabels[kSoilPresetCount] = {
    "Bedrock", "Rock", "Clay", "Dirt", "Mud", "Sand"
};

// The five numbers one preset fills. Field names match the soil-physics record they are copied
// onto, so the assignment reads literally and cannot transpose two values.
struct SoilPresetValues {
    float hardness           = 0.2f;
    float friction           = 0.8f;
    float cohesion           = 0.5f;
    float capacityMultiplier = 2.0f;
    float absorptionRate     = 0.01f;
};

// The catalogue. Hardness rises from mud to bedrock; the loose materials carry more sediment and
// drink more water. Every number is inside the plan's slider limits, so a preset can never place
// a control off its own track.
inline SoilPresetValues SoilPresetValuesOf(SoilPreset preset) {
    switch (preset) {
        case SoilPreset::Bedrock: return SoilPresetValues{ 0.95f, 0.90f, 0.95f, 0.4f, 0.002f };
        case SoilPreset::Rock:    return SoilPresetValues{ 0.75f, 0.80f, 0.80f, 0.8f, 0.005f };
        case SoilPreset::Clay:    return SoilPresetValues{ 0.45f, 0.65f, 0.70f, 1.6f, 0.020f };
        case SoilPreset::Dirt:    return SoilPresetValues{ 0.25f, 0.55f, 0.45f, 2.2f, 0.040f };
        case SoilPreset::Mud:     return SoilPresetValues{ 0.08f, 0.30f, 0.20f, 3.5f, 0.120f };
        case SoilPreset::Sand:    return SoilPresetValues{ 0.15f, 0.40f, 0.10f, 3.0f, 0.080f };
        default:                  return SoilPresetValues();
    }
}

// True when `presetIndex` names a preset. An index a shrunken menu no longer covers answers false
// rather than being clamped onto a neighbour (Constitution §6).
inline bool IsSoilPresetIndex(int presetIndex) {
    return presetIndex >= 0 && presetIndex < kSoilPresetCount;
}

} // namespace Ui
} // namespace SanmapGen

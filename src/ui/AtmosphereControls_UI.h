// AtmosphereControls_UI.h — the Atmosphere tab's control table: WHICH setting each row edits,
// with WHICH widget, between WHICH limits. Layer: UI. Accuracy class: Visual.
//
// The tab has forty-nine controls across eight sections. Writing forty-nine hand-rolled draw
// calls would blow the ARCH §1.5 size ceilings and scatter the limits across the draw code, so
// the tab is TABLE-DRIVEN: this file states every row once, `AtmosphereTab_UI.cpp` walks it, and
// the acceptance test reads the same table the UI does. Constitution §8 — every limit is a
// stated setting, never a literal at a draw site.
#pragma once
#include "AtmosphereSettings_UI.h"
#include "SliderScalar_UI.h"

namespace SanmapGen {
namespace Ui {

// Which shared widget draws a row. `Vector` is a run of scalar sliders over one float array.
enum class AtmosphereControlKind { Scalar, Color, Vector, Text, Combo };

struct AtmosphereControl {
    const char*                 label       = nullptr;
    AtmosphereControlKind       kind        = AtmosphereControlKind::Scalar;
    float AtmosphereSettings::* scalarValue = nullptr;   // Scalar rows only
    int                         slotIndex   = -1;        // Color / Vector / Text rows only
    ScalarSliderRange           range;                   // Scalar rows and Vector components
    const char*                 valueFormat = "%.2f";
};

// One collapsing section = a contiguous run of the control table, so the drawn order IS the
// table order and no section can silently drop a row.
struct AtmosphereSection {
    const char* label             = nullptr;
    int         firstControlIndex = 0;
    int         controlCount      = 0;
};

inline constexpr int kAtmosphereControlCount = 49;
inline constexpr int kAtmosphereSectionCount = 8;

// The component labels a Vector row draws its sliders under.
inline constexpr int kAtmosphereVectorComponentLimit = 3;
extern const char* const atmosphereVectorComponentLabels[kAtmosphereVectorComponentLimit];

extern const AtmosphereControl atmosphereControls[kAtmosphereControlCount];
extern const AtmosphereSection atmosphereSections[kAtmosphereSectionCount];

// True when a row is inside the table — the fence every walker passes its index through
// (Constitution §6).
inline bool IsAtmosphereControlIndexValid(int controlIndex) {
    return controlIndex >= 0 && controlIndex < kAtmosphereControlCount;
}

// The rows a section covers, clamped to the table so a mis-stated span can never read past it.
inline int AtmosphereSectionLastControlIndex(const AtmosphereSection& section) {
    const int lastIndex = section.firstControlIndex + section.controlCount;
    return lastIndex < kAtmosphereControlCount ? lastIndex : kAtmosphereControlCount;
}

} // namespace Ui
} // namespace SanmapGen

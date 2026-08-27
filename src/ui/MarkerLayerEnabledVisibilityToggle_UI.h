// MarkerLayerEnabledVisibilityToggle_UI.h — STEP144: the Enabled/Disabled (E/D) + Visible/Invisible
// (V/I) coupled toggle behavior for a Procedural (Rule) Layer's header buttons, human's own
// explicit rule set. Reachable states are exactly {Enabled,Visible}, {Enabled,Hidden},
// {Disabled,Hidden} — {Disabled,Visible} never occurs:
//   toggling E/D to Disabled  -> forces Hidden too (nothing to show once generation is off).
//   toggling E/D to Enabled   -> forces Visible too.
//   toggling V/I from either Enabled state -> only ever changes Hidden, Enabled is untouched.
//   toggling V/I from {Disabled,Hidden} toward Visible -> auto-enables (the only way to reach
//     Visible from Disabled, since {Disabled,Visible} is not a real state).
// Pure, no imgui — mirrors MarkerLayerId_UI.h's own single-purpose-file precedent.
#pragma once

namespace SanmapGen {
namespace Ui {

inline void ApplyMarkerRuleLayerEnabledToggle(bool& bEnabled, bool& bHidden) {
    bEnabled = !bEnabled;
    bHidden  = !bEnabled;
}

inline void ApplyMarkerRuleLayerVisibilityToggle(bool& bEnabled, bool& bHidden) {
    bHidden = !bHidden;
    if (!bHidden) bEnabled = true;
}

} // namespace Ui
} // namespace SanmapGen

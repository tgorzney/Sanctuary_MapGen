// AtmosphereTab_UI.h — the Atmosphere tab: Sun, Skylight, Exposure & Skybox, the four fog
// varieties, and the global wind. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "ENVIRONMENT / Atmosphere".
//
// The tab is TABLE-DRIVEN (AtmosphereControls_UI.h): it walks the eight sections and their
// forty-nine rows, so adding an atmosphere setting is one table line and no draw-code edit.
//
// THE TIER IS NOT DECIDED HERE. Every committed edit makes the identical
// `Pipeline::PreviewDriver::NotifyParametersChanged()` call and the driver derives the tier from
// the stage parameter hashes. No stage hashes an atmosphere value today (see the SCOPE NOTE on
// AtmosphereSettings_UI.h), so every edit currently derives a preview recomposite — which is the
// derivation working, not a per-widget flag list this tab is forbidden to carry.
//
// SCOPE NOTE (ARCH §8.4): `AtmosphereSettings` is UI-layer presentation state, not PARAMS — v2
// has no `Params::Atmosphere` and no stage that consumes one. The settings therefore persist for
// the session but do NOT serialize into the recipe. Promoting them needs its own work-order.
#pragma once
#include "AtmosphereControls_UI.h"
#include "RtToggleWidget_UI.h"
#include "Section_UI.h"
#include "TextInput_UI.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// Caller-owned tab state: the settings themselves, one RealtimeToggle per row (a scrub on any
// control defers its own commit and no other's — the v1 shared-static bug this library kills),
// and one open/closed bit per section.
struct AtmosphereTabState {
    AtmosphereSettings settings;
    RealtimeToggle     controlToggles[kAtmosphereControlCount];
    SectionState       sections[kAtmosphereSectionCount];
};

// The two path rows are file paths, so they take a wider cap than the 64-character default the
// shared rules use for names (TextInput_UI.h fences it at the staging buffer either way).
inline TextInputRules AtmospherePathTextRules() {
    TextInputRules rules;
    rules.maximumLength = 200;
    return rules;
}

// `previewDriver` may be null (a tab drawn with no pipeline behind it still edits its settings) —
// every call through it is guarded.
void DrawAtmosphereTab(AtmosphereTabState& state, Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen

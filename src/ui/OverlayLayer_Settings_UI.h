// OverlayLayer_Settings_UI.h — the six-domain screen-space overlay stack: what View-toolbar row
// draws which sub-layer, in what Z order, at what opacity (ARCH_14_02_DataModel.md §14.2). Layer: UI.
// Session-only presentation, the same precedent as `PreviewComposite_Settings_UI.h`'s
// `PreviewCompositeSettings::fieldLayers` (§14.1): never recipe-serialized, no PARAMS home. A
// SEPARATE settings object from `PreviewCompositeSettings` on purpose (see this work-order's
// Fix section item 1) — consumed by the screen-space icon draw pass
// (`MapCanvas_IconLayer_UI.h`/STEP53), not by `PreviewComposite`'s GPU field/terrain compositor.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

// The six confirmed dynamic overlay domains (ARCH_14_PreviewOverlayLayering.md §14 intro). Open/additive — a new domain is a
// new enumerator, never a reshuffled existing one (§14.2).
enum class OverlayDomainKind_UI { Alloy, SpawnsArmies, Units, Props, Reclaim, Decals };

// Manual = hand-authored pass-through data (`Params::MapRecipe`'s own arrays); ProceduralRule =
// a `recipe.*Rules[i]` scatter rule's resolved instances (§14.1).
enum class OverlaySubLayerKind_UI { Manual, ProceduralRule };

// `index` resolves differently per (domainKind, kind) pair — see the §14.2 mapping table and
// `Application_OverlaySetup_UI.cpp`'s seeding below for the per-domain resolution each pairing
// uses.
struct OverlaySubLayerRef_UI { OverlaySubLayerKind_UI kind; int index; bool bEnabled = true; };

// One row of the View toolbar's "Overlays (screen-space)" section. BINDING SHAPE (ARCH_14_02_DataModel.md §14.2) —
// do not add fields here; `color`/`iconScale` deliberately live elsewhere (§14.5, see
// `OverlaySessionAppearance` below).
struct OverlayLayer_UI {
    std::string name;
    OverlayDomainKind_UI domainKind = OverlayDomainKind_UI::Alloy;
    bool bEnabled = true;
    float opacity = 1.0f;                              // layer-wide alpha multiplier (§14.2/§14.8)
    std::vector<OverlaySubLayerRef_UI> subLayers;       // any mix/count of Manual + ProceduralRule
    float thumbnailLodThresholdPixels = 5.0f;           // §14.3, Constitution §8 tunable
    // §14.3's strategic-mode fixed screen size (STEP53) — not yet ratified when §14.2 first shipped
    // this struct; added here, per-layer-tweakable exactly like thumbnailLodThresholdPixels above,
    // rather than a shadow field on the draw pass itself.
    float strategicIconScreenSizePixels = 16.0f;
};

// §14.5's "UI-session defaults" half: color/iconScale for a domain with no recipe-serialized
// layer-metadata record yet. Props/Decals do NOT use this — they read/write
// `Params::PropInstanceLayer`/`DecalInstanceLayer` directly through each Manual sub-layer's own
// `index` (no shadow copy, §14.5, see Fix item 3). One entry per such domain, not per sub-layer:
// Units' `Army.groups` carries no per-group appearance field to mirror, and Alloy/SpawnsArmies'
// Manual sub-layers (STEP97, ARCH_14_14) read/write their `OverlaySubLayerRef_UI::index` against
// `recipe.markerLayers` directly, same no-shadow-copy posture Props/Decals already use — so they
// need no `OverlaySessionAppearance` slot of their own either.
struct OverlaySessionAppearance { float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; float iconScale = 1.0f; };

// The session container itself (§14.1's `overlayLayers: vector<OverlayLayer_UI>`), plus its
// three UI-session appearance slots (§14.5). Never serialized into `mapGeneratorData` — the same
// posture `PreviewCompositeSettings` already states for `fieldLayers`.
struct OverlayLayerSettings {
    std::vector<OverlayLayer_UI> overlayLayers;   // vector order = Z order (§14.2)
    OverlaySessionAppearance alloyAppearance;
    OverlaySessionAppearance spawnsArmiesAppearance;
    OverlaySessionAppearance unitsAppearance;
    // Monotonic, bumped by any overlayLayers mutation (reorder/opacity/enable-toggle/threshold
    // change) — the icon draw pass's (STEP53) C2 cache invalidation key and per-layer AABB-cache
    // rebuild key, mirroring PreviewDriver::NotifyParametersChanged()'s own hash-bump pattern
    // (§14.7). No mutation site exists yet in this sequence (the View toolbar is Phase 4) — this is
    // forward-compatible plumbing, not dead weight; today it simply never advances past 0.
    std::uint64_t layerSettingsRevision = 0;
    void BumpLayerSettingsRevision() { ++layerSettingsRevision; }
};

} // namespace Ui
} // namespace SanmapGen

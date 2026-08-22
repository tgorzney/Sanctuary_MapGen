// Application_Defaults_UI.h — what the shell starts up on, and the one bridge that has to be
// drivable without a window. Layer: UI. A member file of Application_UI.h (ARCH §1.5), split out so
// the class header stays inside the ceiling.
//
// These are FREE functions on purpose: the launch defaults and the atlas-id bridge are the two
// parts of the shell an acceptance test wants to drive with no `Ui::Application` around them.
#pragma once
#include <string>
#include <vector>
#include "IconGridWidget_UI.h"                // IconAtlasManifest, the bridge's output
#include "OverlayLayer_Settings_UI.h"
#include "PreviewComposite_Settings_UI.h"
#include "../io/AssetAtlasCache_Atlas_IO.h"   // Io::AssetAtlas, the bridge's input
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Sys { class AtlasResidency; class GpuResourceManager; }
namespace Pipeline { class GenerationAssembler; }
namespace Ui {

Params::MapRecipe MakeDefaultMapRecipe();                          // Application_Recipe_UI.cpp
void ConfigureDefaultStages(Pipeline::GenerationAssembler& assembler);   // Application_Recipe_UI.cpp
void ConfigureDefaultPreview(PreviewCompositeSettings& previewSettings, int previewResolution,
                             float worldUnitsPerCell);            // Application_PreviewSetup_UI.cpp

// The six-domain overlay stack's launch default (ARCH_14_02_DataModel.md §14.2) — one `OverlayLayer_UI`
// per domain, sub-layers seeded per the §14.2 mapping table. One-shot; no live resync as the recipe
// grows mid-session (Application_OverlaySetup_UI.cpp).
void ConfigureDefaultOverlayLayers(OverlayLayerSettings& overlaySettings,
                                    const Params::MapRecipe& recipe);   // Application_OverlaySetup_UI.cpp
// The one flattening Units' Manual sub-layers need (§14.4): resolves a flat
// `OverlaySubLayerRef_UI::index` back to the owning army + top-level group. Army-major,
// group-minor — the SAME order `ConfigureDefaultOverlayLayers` seeds these refs in; do not
// re-derive a second flattening convention at a future call site (§8.3 "one copy" precedent,
// cited the same way in `STEP47_WorldScreenProjection_UI.md`).
bool ResolveUnitsManualSubLayer(const Params::MapRecipe& recipe, int flatSubLayerIndex,
                                 int& outArmyIndex, int& outGroupIndex);   // Application_OverlaySetup_UI.cpp

// The atlas-id bridge, as a free function so it is drivable without a window: assigns each
// `Io::AtlasEntry` its index in Entries() as the `iconId` the grid emits, carries the uv rect
// across, and fills the id -> template-identifier side table.  (Application_Assets_UI.cpp)
void BuildIconAtlasManifest(const Io::AssetAtlas& atlas, const Sys::AtlasResidency& atlasResidency,
                            Sys::GpuResourceManager* gpuResourceManager,
                            IconAtlasManifest& outManifest,
                            std::vector<std::string>& outTemplateIdentifiers);

} // namespace Ui
} // namespace SanmapGen

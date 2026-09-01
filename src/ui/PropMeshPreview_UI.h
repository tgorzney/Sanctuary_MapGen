// PropMeshPreview_UI.h — the Props tab's "Mesh Preview" section (Auto-NavMesh Phase 0): pick a
// prop from `recipe.props`, resolve its blueprint -> LOD0 `.sanmodel` -> load it via
// Sys::ReadSanmodelMesh, and render it with a simple orbit camera so the reader can be visually
// verified against real assets rather than trusted blind. Layer: UI.
//
// SCOPE: only `recipe.props` (Params::PropInstanceGroup::blueprintPath) is supported, not the
// procedural scatter population (Data::PlacementInstances, keyed by tpId through
// Io::TemplateIngestReport). Deliberate, not an oversight: `recipe.props` is exactly what a REAL
// (non-SanGen) map's props import into (MapImporter_Props_IO.cpp reads the `props` JSON array
// directly; a real map's JSON carries no procedural-rule data at all), so it is also the exact
// population this tool needs to visually confirm prop import is correct end-to-end. Procedural/tpId
// resolution (through Io::TemplateIngestReport::FindByTemplateIdentifier) is a straightforward
// follow-up this ticket does not need.
#pragma once
#include "MeshPreviewCamera_UI.h"
#include "MeshPreviewRasterize_UI.h"
#include "Section_UI.h"
#include "../sys/GpuResource_SYS.h"
#include "../sys/SanmodelMesh_SYS.h"
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct PropInstanceGroup; }
namespace Ui {

struct PropMeshPreviewState {
    SectionState section;
    int          selectedGroupIndex = -1;
    std::string  loadedBlueprintPath;   // the blueprintPath actually resident in `mesh` (cache key)
    std::string  statusMessage;
    bool         bLoadSucceeded = false;

    Sys::SanmodelMesh            mesh;
    MeshPreviewCameraState       camera;
    MeshPreviewRasterizeSettings rasterizeSettings;

    // Rasterize-on-change cache: re-rasterizing every imgui frame regardless of camera movement
    // would burn CPU for nothing while the section just sits open and idle.
    Sys::GpuTextureHandle  textureHandle;
    MeshPreviewCameraState lastRasterizedCamera;
    bool                   bHasRasterizedOnce = false;
};

// Resolves and loads `blueprintPath`'s LOD0 mesh into `state`: blueprintPath -> .santp bytes ->
// Lua-evaluate -> dialect-detect -> visuals.lods[] -> minimum-distance .sanmodel -> bytes -> parse.
// Every failure degrades to `state.bLoadSucceeded = false` with a diagnostic in `statusMessage`
// (Constitution §6) — a `.sanprop`-only/engine-lua template with no `lods[]` is a graceful skip,
// not an error tone. Recenters the camera on the new mesh's bounds on success.
void LoadPropMeshPreviewForBlueprint(const std::string& gameInstallRoot, const std::string& blueprintPath,
                                     PropMeshPreviewState& state);

// `props` is `recipe.props` (read-only: this section only ever selects, never edits).
// `gpuResourceManager` is nullable — with none, the picker/status text still draws but no image
// renders (mirrors this codebase's existing `iconManifest`-nullable posture, e.g. PropsTab_UI.h).
void DrawPropMeshPreviewSection(PropMeshPreviewState& state,
                                const std::vector<Params::PropInstanceGroup>& props,
                                const std::string& gameInstallRoot,
                                Sys::GpuResourceManager* gpuResourceManager);

} // namespace Ui
} // namespace SanmapGen

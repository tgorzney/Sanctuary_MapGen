// PropMeshPreview_UI.cpp — LoadPropMeshPreviewForBlueprint's own IO/SYS orchestration. See the
// header for the contract and the scope note. The imgui composition is a private ARCH §1.5 aspect
// split, PropMeshPreview_Draw_UI.cpp — declared on the same header, no header of its own.
#include "PropMeshPreview_UI.h"
#include "../io/ResolvePackRelativeAsset_IO.h"
#include "../io/TemplateDialect_IO.h"
#include "../io/TemplateVisualLod_IO.h"
#include "../sys/LuaTableEvaluate_SYS.h"
#include "../sys/SanmodelRead_SYS.h"
#include <algorithm>
#include <cstdio>
#include <limits>

namespace SanmapGen {
namespace Ui {
namespace {

void ComputeMeshBounds(const Sys::SanmodelMesh& mesh, float& minX, float& minY, float& minZ,
                       float& maxX, float& maxY, float& maxZ) {
    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = -std::numeric_limits<float>::max();
    for (std::size_t vertex = 0; vertex * 3 + 2 < mesh.positions.size(); ++vertex) {
        const float x = mesh.positions[vertex * 3], y = mesh.positions[vertex * 3 + 1],
                   z = mesh.positions[vertex * 3 + 2];
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
        minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
    }
    if (mesh.positions.empty()) { minX = minY = minZ = maxX = maxY = maxZ = 0.0f; }
}

void FailPreview(PropMeshPreviewState& state, const std::string& message) {
    state.bLoadSucceeded = false;
    state.statusMessage = message;
    state.mesh = Sys::SanmodelMesh();
    state.bHasRasterizedOnce = false;
}

std::string DescribeLoadedMesh(const Sys::SanmodelMesh& mesh, const std::string& modelPath) {
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "Loaded '%s' (%zu vertices, %zu triangles)%s",
                 modelPath.c_str(), mesh.positions.size() / 3, mesh.triangleIndices.size() / 3,
                 mesh.bWasSkinned ? ", skinned (rest pose)" : "");
    return std::string(buffer);
}

} // namespace

void LoadPropMeshPreviewForBlueprint(const std::string& gameInstallRoot, const std::string& blueprintPath,
                                     PropMeshPreviewState& state) {
    state.loadedBlueprintPath = blueprintPath;
    if (gameInstallRoot.empty()) { FailPreview(state, "No game install root configured (System tab)."); return; }

    const Io::PackRelativeAssetResult templateBytes =
        Io::ResolvePackRelativeAssetBytes(gameInstallRoot, blueprintPath);
    if (!templateBytes.bSucceeded) { FailPreview(state, templateBytes.errorMessage); return; }

    const std::string sourceText(templateBytes.bytes.begin(), templateBytes.bytes.end());
    const Sys::LuaTableEvaluateResult evaluated = Sys::EvaluateLuaTableSource(sourceText);
    if (!evaluated.bSucceeded) {
        FailPreview(state, "Not a Lua template (possibly a deprecated .sanprop/JSON file): " + evaluated.errorMessage);
        return;
    }

    Io::TemplateDialectKind dialectKind = Io::TemplateDialectKind::Unrecognized;
    const Sys::LuaTableValue* rootTable = Io::DetectTemplateRootTable(evaluated.globals, dialectKind);
    if (rootTable == nullptr) { FailPreview(state, "No recognized template root table."); return; }

    const std::vector<Io::TemplateVisualLodEntry> lods = Io::ExtractVisualLods(*rootTable);
    const Io::TemplateVisualLodEntry* lowestLod = Io::SelectLowestDistanceLod(lods);
    if (lowestLod == nullptr) {
        FailPreview(state, "Template has no visuals.lods[] (engine-lua/Dialect-B template, or a "
                          "hole-only prop) -- nothing to preview.");
        return;
    }

    const Io::PackRelativeAssetResult modelBytes =
        Io::ResolvePackRelativeAssetBytes(gameInstallRoot, lowestLod->model);
    if (!modelBytes.bSucceeded) {
        FailPreview(state, "Could not resolve model '" + lowestLod->model + "': " + modelBytes.errorMessage);
        return;
    }

    const Sys::SanmodelReadResult meshResult =
        Sys::ReadSanmodelMesh(modelBytes.bytes.data(), modelBytes.bytes.size());
    if (!meshResult.bSucceeded) {
        FailPreview(state, "Failed to parse '" + lowestLod->model + "': " + meshResult.errorMessage);
        return;
    }

    state.mesh = meshResult.mesh;
    state.bLoadSucceeded = true;
    state.statusMessage = DescribeLoadedMesh(state.mesh, lowestLod->model);
    state.bHasRasterizedOnce = false;

    float minX, minY, minZ, maxX, maxY, maxZ;
    ComputeMeshBounds(state.mesh, minX, minY, minZ, maxX, maxY, maxZ);
    FitCameraToBounds(state.camera, minX, minY, minZ, maxX, maxY, maxZ);
}

} // namespace Ui
} // namespace SanmapGen

// PropMeshPreview_Draw_UI.cpp — private ARCH §1.5 aspect split off PropMeshPreview_UI.cpp:
// DrawPropMeshPreviewSection's own imgui composition. Declared on PropMeshPreview_UI.h -- this
// file needs no header of its own.
#include "PropMeshPreview_UI.h"
#include "../params/PropInstance_PARAMS.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

bool CameraChanged(const MeshPreviewCameraState& a, const MeshPreviewCameraState& b) {
    return a.yawRadians != b.yawRadians || a.pitchRadians != b.pitchRadians || a.distance != b.distance
        || a.targetX != b.targetX || a.targetY != b.targetY || a.targetZ != b.targetZ;
}

void DrawPropPicker(PropMeshPreviewState& state, const std::vector<Params::PropInstanceGroup>& props,
                    const std::string& gameInstallRoot) {
    ImGui::TextUnformatted("Select a prop (recipe.props):");
    if (!ImGui::BeginListBox("##propMeshPreviewPicker", ImVec2(-1.0f, 120.0f))) return;
    for (int index = 0; index < static_cast<int>(props.size()); ++index) {
        const Params::PropInstanceGroup& group = props[static_cast<std::size_t>(index)];
        char label[160];
        std::snprintf(label, sizeof(label), "%s (%zu instance(s))",
                     group.blueprintPath.empty() ? "<empty blueprintPath>" : group.blueprintPath.c_str(),
                     group.transforms.size());
        const bool bSelected = state.selectedGroupIndex == index;
        if (ImGui::Selectable(label, bSelected) && !bSelected) {
            state.selectedGroupIndex = index;
            LoadPropMeshPreviewForBlueprint(gameInstallRoot, group.blueprintPath, state);
        }
    }
    ImGui::EndListBox();
}

// An invisible drag/scroll surface drawn UNDER the eventual image (same rectangle, cursor rewound
// afterward) — ImGui::Image is not itself an interactive item, so hover/drag needs a real widget.
void ApplyOrbitInput(MeshPreviewCameraState& camera, ImVec2 imageSize) {
    const ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##propMeshPreviewDragSurface", imageSize);
    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive  = ImGui::IsItemActive();
    ImGui::SetCursorScreenPos(cursorPosition);

    if (bHovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) camera.distance = std::max(0.01f, camera.distance * std::pow(0.9f, wheel));
    }
    if (bActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        camera.yawRadians += delta.x * 0.01f;
        camera.pitchRadians = std::clamp(camera.pitchRadians + delta.y * 0.01f, -1.5f, 1.5f);
    }
}

void DrawRenderedImage(PropMeshPreviewState& state, Sys::GpuResourceManager* gpuResourceManager) {
    if (gpuResourceManager == nullptr) {
        ImGui::TextUnformatted("(no GPU context -- preview image unavailable)");
        return;
    }
    const ImVec2 imageSize(static_cast<float>(state.rasterizeSettings.viewportWidth),
                           static_cast<float>(state.rasterizeSettings.viewportHeight));
    ApplyOrbitInput(state.camera, imageSize);   // may mutate state.camera this frame

    const bool bNeedsRasterize =
        !state.bHasRasterizedOnce || CameraChanged(state.camera, state.lastRasterizedCamera);
    if (bNeedsRasterize) {
        std::vector<unsigned char> pixels;
        RasterizeMeshPreview(state.mesh, state.camera, state.rasterizeSettings, pixels);
        state.textureHandle = gpuResourceManager->EnsureTexture(
            "propMeshPreview", state.rasterizeSettings.viewportWidth, state.rasterizeSettings.viewportHeight);
        gpuResourceManager->UploadTexture(state.textureHandle, pixels.data(), pixels.size());
        state.lastRasterizedCamera = state.camera;
        state.bHasRasterizedOnce = true;
    }

    const unsigned long long presentationIdentifier =
        gpuResourceManager->TexturePresentationIdentifier(state.textureHandle);
    if (presentationIdentifier == 0ull) { ImGui::TextUnformatted("(texture unavailable)"); return; }
    ImGui::Image(static_cast<ImTextureID>(presentationIdentifier), imageSize);
    ImGui::TextUnformatted("Drag to orbit, scroll to zoom.");
}

} // namespace

void DrawPropMeshPreviewSection(PropMeshPreviewState& state,
                                const std::vector<Params::PropInstanceGroup>& props,
                                const std::string& gameInstallRoot,
                                Sys::GpuResourceManager* gpuResourceManager) {
    if (!DrawSectionBegin("Mesh Preview", state.section)) return;
    if (gameInstallRoot.empty())
        ImGui::TextUnformatted("Set a game install root (System tab) to preview prop meshes.");
    DrawPropPicker(state, props, gameInstallRoot);
    if (!state.statusMessage.empty())
        ImGui::TextColored(state.bLoadSucceeded ? ImVec4(0.6f, 0.9f, 0.6f, 1.0f) : ImVec4(0.95f, 0.7f, 0.4f, 1.0f),
                          "%s", state.statusMessage.c_str());
    if (state.bLoadSucceeded) DrawRenderedImage(state, gpuResourceManager);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen

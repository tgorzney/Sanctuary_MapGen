// MapCanvas_UI.h — the map viewport. Layer: UI. Accuracy class: Visual.
// It does exactly three things: DRAW the preview composite's GL texture (M5-5's repoint of
// PreviewComposite_UI) in an imgui image region, PAN/ZOOM that view, and route a click through
// `Picking_UI` (M4-4) to the entity the composite already rendered there.
//
// It never simulates and never spawns (ARCH §3.2, §5.3). The ~720-line `Widget_MapCanvas` it
// replaces owned unit-grid spawning, symmetry spawn, army creation and triangle height
// interpolation; all of that is Placement/PROC reached through PIPELINE, and the ONLY way this
// widget can cause work to happen is the injected regenerate callback
// (`Pipeline::PreviewDriver::SetPreviewCompositeCallback` is the same pattern). Nothing here
// includes a `_PROC` header, and it holds no GL handle: the texture stays behind
// `Sys::GpuResourceManager` and the canvas only carries its opaque presentation identifier.
//
// The pan/zoom math is `MapCanvasView` — pure, imgui-free, so the cursor -> preview-pixel
// contract a click depends on is testable without a UI frame. `Draw` is a thin translator from
// imgui input to the three gesture methods below (MapCanvas_Draw_UI.cpp).
#pragma once
#include <cstdint>
#include <functional>
#include "MapCanvasView_UI.h"
#include "../data/EntityIdBuffer_DATA.h"
#include "../sys/GpuResource_SYS.h"

namespace SanmapGen {
namespace Ui {

class MapCanvas {
public:
    // What is displayed: the composited image texture, owned by SYS, sized `previewResolution`
    // squared. Re-pointing it at a different resolution resets the view.
    void SetPreviewTexture(Sys::GpuResourceManager* manager, Sys::GpuTextureHandle texture,
                           int previewResolution);
    // What a click is resolved against: the composite's per-pixel id buffer, read-only.
    void SetEntityIdentifierBuffer(const Data::EntityIdBuffer* entityIdentifierBuffer) {
        entityIdentifiers = entityIdentifierBuffer;
    }
    // Injected by the caller that owns PIPELINE. The canvas asks; it never generates.
    void SetRegenerationCallback(std::function<void()> requestRegeneration) {
        regenerationCallback = std::move(requestRegeneration);
    }
    void SetSelectionChangedCallback(std::function<void(std::uint32_t)> selectionChanged) {
        selectionChangedCallback = std::move(selectionChanged);
    }

    // One imgui frame; `regionSidePixels` is the square viewport side in screen pixels.
    void Draw(const char* canvasIdentifier, float regionSidePixels);   // MapCanvas_Draw_UI.cpp

    // The three gestures, as pure state transitions — Draw() calls exactly these.
    std::uint32_t ApplyClick(float regionLocalX, float regionLocalY);
    void ApplyDrag(float deltaRegionPixelsX, float deltaRegionPixelsY);
    void ApplyScroll(float regionLocalX, float regionLocalY, float wheelSteps);
    void RequestRegeneration();

    MapCanvasView& View() { return view; }
    const MapCanvasView& View() const { return view; }

    // Presentation state of a viewport: what the user last selected. `emptySentinel` = nothing.
    std::uint32_t SelectedEntityIdentifier() const { return selectedEntityIdentifier; }
    bool HasSelection() const {
        return selectedEntityIdentifier != Data::EntityIdBuffer::emptySentinel;
    }
    const PreviewPixelCoordinate& LastPickedPixel() const { return lastPickedPixel; }
    // The toolkit identifier the image draw uses; zero when nothing has been composited yet.
    unsigned long long PresentationIdentifier() const;
    int RegenerationRequestCount() const { return regenerationRequestCount; }

private:
    void SetSelection(std::uint32_t entityIdentifier);
    // Translates the imgui pointer state over the region into the gestures (MapCanvas_Draw_UI.cpp).
    void ApplyPointerInput(float regionOriginX, float regionOriginY);

    MapCanvasView view;
    Sys::GpuResourceManager*     gpuResourceManager = nullptr;
    Sys::GpuTextureHandle        previewTexture;
    const Data::EntityIdBuffer*  entityIdentifiers = nullptr;
    std::function<void()>                 regenerationCallback;
    std::function<void(std::uint32_t)>    selectionChangedCallback;
    PreviewPixelCoordinate lastPickedPixel;
    std::uint32_t selectedEntityIdentifier = Data::EntityIdBuffer::emptySentinel;
    float         pressTravelPixels        = 0.0f;    // how far the current press has dragged
    bool          bPressActive             = false;
    int           regenerationRequestCount = 0;
};

} // namespace Ui
} // namespace SanmapGen

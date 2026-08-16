// DraggableList_TestScene_UI.h — the four-row layer stack the M5-2 DraggableList acceptance test
// drives, plus the synthetic-pointer helpers. Test-support only; no GL.
//
// The scene is the CALLER's state: DraggableList never writes it, so every assertion about order,
// visibility or lock state is an assertion about what the caller applied from a signal.
#pragma once
#include "DraggableListWidget_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include <vector>

namespace SanmapGen {
namespace Ui {

const ImVec2 kSceneWindowSize = ImVec2(420.0f, 300.0f);
const ImVec2 kMouseAway       = ImVec2(-FLT_MAX, -FLT_MAX);

struct TestLayer {
    const char* name;
    int         identifier;
    bool        bVisible;
    bool        bLocked;
};

struct DraggableScene {
    std::vector<TestLayer> layers;
    DraggableListSignal    signal;
    float                  rowTopY[8] = {};
    float                  rowLeftX   = 0.0f;
};

inline DraggableScene MakeDraggableScene() {
    DraggableScene scene;
    scene.layers = {{"Alpha", 1, true, false}, {"Bravo", 2, true, false},
                    {"Charlie", 3, false, true}, {"Delta", 4, true, false}};
    return scene;
}

// One frame with the synthetic pointer wherever the caller put it. describeRow doubles as the
// geometry probe: it runs immediately before each row header, so the cursor is that row's corner.
inline DraggableListSignal RunSceneFrame(DraggableScene& scene, ImVec2 mousePosition,
                                         bool bLeftButtonDown) {
    HeadlessMouseState mouse;
    mouse.position = mousePosition;
    mouse.bLeftButtonDown = bLeftButtonDown;
    RunHeadlessFrame(mouse, kSceneWindowSize, [&] {
        scene.signal = DraggableList<TestLayer>::Render("LayerStack", scene.layers,
            [&](int rowIndex) {
                const ImVec2 rowCorner = ImGui::GetCursorScreenPos();
                if (rowIndex < 8) scene.rowTopY[rowIndex] = rowCorner.y;
                scene.rowLeftX = rowCorner.x;
                DraggableListRow row;
                row.label    = scene.layers[rowIndex].name;
                row.bVisible = scene.layers[rowIndex].bVisible;
                row.bLocked  = scene.layers[rowIndex].bLocked;
                return row;
            },
            [](int) {});                       // header-only rows keep the geometry stable
    });
    return scene.signal;
}

inline float RowCenterY(const DraggableScene& scene, int rowIndex) {
    return scene.rowTopY[rowIndex] + (scene.rowTopY[1] - scene.rowTopY[0]) * 0.5f;
}

// Hover, press, release — the sequence a real pointer makes. The hover frame is REQUIRED, not
// cosmetic: rows use ImGuiTreeNodeFlags_AllowOverlap so their affordances can sit on top of the
// full-width header, and imgui only grants an overlapping item interaction once it was the hovered
// item in the PREVIOUS frame. The header reports Select on the press and a button reports on the
// release, so the first signal of the pair is the one this click produced.
inline DraggableListSignal ClickAt(DraggableScene& scene, ImVec2 position) {
    RunSceneFrame(scene, position, false);
    const DraggableListSignal pressSignal = RunSceneFrame(scene, position, true);
    const DraggableListSignal releaseSignal = RunSceneFrame(scene, position, false);
    return pressSignal.bHasSignal() ? pressSignal : releaseSignal;
}

inline bool SignalIs(const DraggableListSignal& signal, DraggableListSignalKind kind,
                     int sourceRowIndex) {
    return signal.kind == kind && signal.sourceRowIndex == sourceRowIndex;
}

inline bool OrderIs(const DraggableScene& scene, int first, int second, int third, int fourth) {
    return scene.layers.size() == 4u && scene.layers[0].identifier == first &&
           scene.layers[1].identifier == second && scene.layers[2].identifier == third &&
           scene.layers[3].identifier == fourth;
}

} // namespace Ui
} // namespace SanmapGen

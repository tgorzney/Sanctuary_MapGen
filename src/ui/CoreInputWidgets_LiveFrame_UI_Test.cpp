// CoreInputWidgets_LiveFrame_UI_Test.cpp — the M5-1 draw path exercised in a REAL imgui frame.
// The other three M5-1 tests cover the pure logic and link no imgui at all; this one closes the
// remaining gap — that the ImDrawList geometry and the InvisibleButton hit-tests actually agree
// with that logic — WITHOUT a window, a GL context or a backend: imgui is driven headless with
// synthetic mouse events and its draw data is inspected instead of rendered.
// It is the ONLY SanGen test that links imgui (see CMakeLists.txt).
#include "LabelledDialWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "imgui.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;
static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// The scene under test: two controls and the state their caller owns (never the widgets).
static Ui::RangeSliderValues sliderValues{0.2f, 0.8f};
static Ui::RangeSliderBounds sliderBounds;                 // 0..1, default separation
static Ui::RealtimeToggle    sliderToggle;                 // RT off — the deferring case
static float                 dialValue = 5.0f;
static Ui::RealtimeToggle    dialToggle;

// STEP154 — the compact single-line variant, drawn alongside the three-row DrawRangeSlider above so
// this same headless frame proves its explicit SetCursorScreenPos track placement (Draw
// RangeSliderWidget_UI.cpp) actually lands the hit-test where the control is visually drawn, not
// just that the pure interaction math (shared with DrawRangeSlider, already covered above) is sound.
static Ui::RangeSliderValues compactValues{0.3f, 0.7f};
static Ui::RangeSliderBounds compactBounds;                // 0..1, default separation
static Ui::RealtimeToggle    compactToggle;                // RT off — the deferring case
constexpr float kCompactTrackWidth = 120.0f;
constexpr float kCompactFieldWidth = 40.0f;

struct FrameResult {
    Ui::WidgetChange sliderChange;
    Ui::WidgetChange dialChange;
    Ui::WidgetChange compactChange;
    ImVec2 trackOrigin = ImVec2(0.0f, 0.0f);
    float  trackWidth  = 0.0f;
    float  trackHeight = 0.0f;
    ImVec2 knobOrigin  = ImVec2(0.0f, 0.0f);
    ImVec2 compactTrackOrigin = ImVec2(0.0f, 0.0f);
    int    vertexCount = 0;
};

static FrameResult RunFrame(float cursorX, float cursorY, bool bMouseDown) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(cursorX, cursorY);
    io.AddMouseButtonEvent(0, bMouseDown);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f));
    ImGui::Begin("Widgets", nullptr, ImGuiWindowFlags_NoSavedSettings);

    FrameResult result;
    const ImVec2 windowCursor = ImGui::GetCursorScreenPos();
    result.trackWidth  = ImGui::GetContentRegionAvail().x;
    result.trackHeight = ImGui::GetFrameHeight();
    result.trackOrigin = ImVec2(windowCursor.x,
                                windowCursor.y + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y);
    result.sliderChange = Ui::DrawRangeSlider("Slope range", sliderValues, sliderBounds, sliderToggle);

    Ui::DialRange dialRange;
    dialRange.maximumValue = 10.0f;
    dialRange.pixelsForFullSweep = 200.0f;
    result.knobOrigin = ImGui::GetCursorScreenPos();
    result.dialChange = Ui::DrawLabelledDial("Erosion rate", dialValue, dialRange, dialToggle);

    // The compact row's own track sits immediately after its minimum field — this test computes
    // that same offset independently (fieldWidth + one ItemSpacing) so a drag at the computed
    // position proves DrawRangeSliderCompact's internal SetCursorScreenPos actually put the track's
    // real hit-test there, not just somewhere that happens to look right.
    const ImVec2 compactRowCursor = ImGui::GetCursorScreenPos();
    result.compactChange = Ui::DrawRangeSliderCompact("Compact range", compactValues, compactBounds,
                                                      compactToggle, kCompactTrackWidth, kCompactFieldWidth);
    result.compactTrackOrigin = ImVec2(compactRowCursor.x + kCompactFieldWidth + ImGui::GetStyle().ItemSpacing.x,
                                       compactRowCursor.y);

    ImGui::End();
    ImGui::Render();
    result.vertexCount = ImGui::GetDrawData()->TotalVtxCount;
    return result;
}

// Press the maximum handle, drag it left over three frames, release. With RT off the value must
// track every frame while the commit arrives exactly once, on the release frame.
static void TestSliderDragDefersInALiveFrame(const FrameResult& layout) {
    const float handleWidth = Ui::WidgetStyle().handleWidth;
    const float maximumHandleCenterX = layout.trackOrigin.x + handleWidth * 0.5f +
        Ui::RangeSliderHandleOffset(sliderValues.maximumValue, sliderBounds, layout.trackWidth, handleWidth);
    const float trackCenterY = layout.trackOrigin.y + layout.trackHeight * 0.5f;

    const FrameResult press = RunFrame(maximumHandleCenterX, trackCenterY, true);
    Check(!press.sliderChange.bCommitted, "pressing a handle does not commit");

    int commitsDuringDrag = 0;
    float cursorX = maximumHandleCenterX;
    for (int step = 0; step < 3; ++step) {
        cursorX -= 20.0f;
        if (RunFrame(cursorX, trackCenterY, true).sliderChange.bCommitted) ++commitsDuringDrag;
    }
    Check(sliderValues.maximumValue < 0.79f, "the grabbed handle tracked the drag");
    Check(sliderValues.minimumValue > 0.19f && sliderValues.minimumValue < 0.21f, "its partner held still");
    Check(commitsDuringDrag == 0, "no commit is paid for during the drag");
    Check(sliderToggle.IsCommitDeferred(), "the change is deferred to the release");

    Check(RunFrame(cursorX, trackCenterY, false).sliderChange.bCommitted, "the commit fires on release");
    Check(!RunFrame(cursorX, trackCenterY, false).sliderChange.bCommitted, "and only once");
}

// Grab the knob after the cursor has jumped across the window, then drag up. The grab frame must
// contribute nothing: MouseDelta still carries the jump, and charging it to the dial would snap
// the value on a plain click.
static void TestDialGrabDoesNotSnap(const FrameResult& layout) {
    const float knobCenterX = layout.knobOrigin.x + ImGui::GetFrameHeight();
    const float knobCenterY = layout.knobOrigin.y + ImGui::GetFrameHeight();
    const float valueBeforePress = dialValue;
    RunFrame(knobCenterX, knobCenterY, true);
    Check(dialValue == valueBeforePress, "grabbing the knob does not move the value");
    RunFrame(knobCenterX, knobCenterY - 40.0f, true);                 // 40 px of a 200 px sweep = +2
    Check(dialValue > valueBeforePress + 1.9f && dialValue < valueBeforePress + 2.1f,
          "dragging up raises the value by the swept fraction");
    Check(RunFrame(knobCenterX, knobCenterY - 40.0f, false).dialChange.bCommitted, "the dial commits on release");
}

// STEP154 — drags the compact slider's maximum handle by its computed on-screen position (not the
// row-below layout DrawRangeSlider's own test above uses), proving the explicit
// SetCursorScreenPos/SameLine composition in DrawRangeSliderCompact places the real hit-test where
// the control is visually drawn.
static void TestCompactSliderDragHitsWhereItIsDrawn(const FrameResult& layout) {
    const float handleWidth = Ui::WidgetStyle().handleWidth;
    const float maximumHandleCenterX = layout.compactTrackOrigin.x + handleWidth * 0.5f +
        Ui::RangeSliderHandleOffset(compactValues.maximumValue, compactBounds, kCompactTrackWidth, handleWidth);
    const float trackCenterY = layout.compactTrackOrigin.y + layout.trackHeight * 0.5f;

    RunFrame(maximumHandleCenterX, trackCenterY, true);   // press
    const float valueBeforeDrag = compactValues.maximumValue;
    RunFrame(maximumHandleCenterX - 20.0f, trackCenterY, true);   // drag left
    Check(compactValues.maximumValue < valueBeforeDrag - 0.02f,
         "dragging the compact slider's maximum handle at its computed on-screen position moves it");
    Check(compactValues.minimumValue > 0.29f && compactValues.minimumValue < 0.31f,
         "its partner (drawn to the LEFT of the track, in the minimum numeric field) holds still");
    Check(RunFrame(maximumHandleCenterX - 20.0f, trackCenterY, false).compactChange.bCommitted,
         "the compact slider's own commit fires on release, same deferred-commit contract as the 3-row widget");
}

int main() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.IniFilename = nullptr;                                          // no settings file in a test
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;          // no renderer: own the atlas
    io.Fonts->AddFontDefault();

    const FrameResult layout = RunFrame(-1.0f, -1.0f, false);          // frame 1: learn the item rects
    Check(layout.vertexCount > 0, "the widgets emit ImDrawList geometry");
    Check(layout.trackWidth > 50.0f, "the track has a sane width");

    TestSliderDragDefersInALiveFrame(layout);
    TestDialGrabDoesNotSnap(layout);
    TestCompactSliderDragHitsWhereItIsDrawn(layout);

    ImGui::DestroyContext();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

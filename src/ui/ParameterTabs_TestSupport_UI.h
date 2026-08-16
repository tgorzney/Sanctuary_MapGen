// ParameterTabs_TestSupport_UI.h — test-only scaffolding shared by the four M5-6 acceptance
// translation units: the pass/fail counter and the synthetic pointer sequences that drive the
// widgets' pure interaction functions. Not part of the layer graph; nothing outside a *_Test.cpp
// includes it (the same standing as IconGridWidget_TestSupport_UI.h / PreviewIntegration_TestScene_UI.h).
#pragma once
#include "LabelledDialWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "PreviewComposite_TestScene_UI.h"   // CheckPreviewExpectation + previewTestFailureCount

namespace SanmapGen {
namespace Ui {

inline void Check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

// One frame of a knob drag: +y is DOWN, so a negative delta raises the value.
inline DialPointerInput DialDrag(float dragDeltaY) {
    DialPointerInput input;
    input.bDragInProgress = true;
    input.dragDeltaY      = dragDeltaY;
    return input;
}
// The frame the button comes up: no drag in progress, nothing moved.
inline DialPointerInput DialRelease() { return DialPointerInput(); }

// One frame with a range-slider handle grabbed and the cursor over `pointerValue`.
inline RangeSliderPointerInput GrabRangeHandle(RangeSliderHandle handle, float pointerValue) {
    RangeSliderPointerInput input;
    input.grabbedHandle = handle;
    input.pointerValue  = pointerValue;
    return input;
}
inline RangeSliderPointerInput ReleaseRangeHandle() { return RangeSliderPointerInput(); }

} // namespace Ui
} // namespace SanmapGen

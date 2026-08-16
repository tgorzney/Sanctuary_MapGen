// AtmosphereTab_UI.cpp — the imgui composition of the Atmosphere tab. Layer: UI.
// One walk of the shared control table (AtmosphereControls_UI.h): eight collapsing sections, and
// one switch that picks the shared widget each row asks for. Every control is a batch-A library
// widget; the only raw imgui here is the per-row id scope and the vector row's label.
#include "AtmosphereTab_UI.h"
#include "ColorSwatch_UI.h"
#include "Combo_UI.h"
#include "SliderScalar_UI.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The ONE thing a tab does with a commit. WHICH tier it becomes is the driver's derivation from
// the stage parameter hashes, never this call site's decision.
void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

// A run of scalar sliders over one float array, sharing the row's RealtimeToggle so the whole
// vector commits as one edit rather than three.
WidgetChange DrawAtmosphereVector(const AtmosphereControl& control, AtmosphereSettings& settings,
                                  RealtimeToggle& realtimeToggle) {
    float* const components = AtmosphereVectorAt(settings, control.slotIndex);
    const int componentCount = AtmosphereVectorComponentCount(control.slotIndex);
    WidgetChange rowChange;
    if (components == nullptr || componentCount <= 0) return rowChange;
    ImGui::TextUnformatted(control.label);
    for (int componentIndex = 0; componentIndex < componentCount; ++componentIndex) {
        ImGui::PushID(componentIndex);
        const WidgetChange change = DrawSliderScalar(atmosphereVectorComponentLabels[componentIndex],
                                                     components[componentIndex], control.range,
                                                     realtimeToggle, WidgetStyle(), control.valueFormat);
        ImGui::PopID();
        rowChange.bValueChanged = rowChange.bValueChanged || change.bValueChanged;
        rowChange.bCommitted    = rowChange.bCommitted    || change.bCommitted;
    }
    return rowChange;
}

WidgetChange DrawAtmosphereControlRow(const AtmosphereControl& control, AtmosphereSettings& settings,
                                      RealtimeToggle& realtimeToggle) {
    switch (control.kind) {
        case AtmosphereControlKind::Scalar:
            if (control.scalarValue == nullptr) break;
            return DrawSliderScalar(control.label, settings.*control.scalarValue, control.range,
                                    realtimeToggle, WidgetStyle(), control.valueFormat);
        case AtmosphereControlKind::Color: {
            float* const color = AtmosphereColorAt(settings, control.slotIndex);
            if (color == nullptr) break;
            return DrawColorSwatch(control.label, color, ColorSwatchOptions(), realtimeToggle);
        }
        case AtmosphereControlKind::Vector:
            return DrawAtmosphereVector(control, settings, realtimeToggle);
        case AtmosphereControlKind::Text: {
            std::string* const text = AtmosphereTextAt(settings, control.slotIndex);
            if (text == nullptr) break;
            return DrawTextInput(control.label, *text, AtmospherePathTextRules(), WidgetStyle(), "path");
        }
        case AtmosphereControlKind::Combo: {
            ComboOptions options;
            options.labels = skyboxIntensityModeLabels;
            options.count  = kSkyboxIntensityModeCount;
            return DrawCombo(control.label, settings.skyboxIntensityModeIndex, options);
        }
    }
    return WidgetChange();
}

void DrawAtmosphereSection(int sectionIndex, AtmosphereTabState& state,
                           Pipeline::PreviewDriver* previewDriver) {
    const AtmosphereSection& section = atmosphereSections[sectionIndex];
    if (!DrawSectionBegin(section.label, state.sections[sectionIndex])) return;
    const int lastControlIndex = AtmosphereSectionLastControlIndex(section);
    for (int controlIndex = section.firstControlIndex; controlIndex < lastControlIndex; ++controlIndex) {
        if (!IsAtmosphereControlIndexValid(controlIndex)) continue;
        ImGui::PushID(controlIndex);                      // labels repeat across sections
        const WidgetChange change = DrawAtmosphereControlRow(atmosphereControls[controlIndex],
                                                             state.settings,
                                                             state.controlToggles[controlIndex]);
        ImGui::PopID();
        NotifyChange(change.bCommitted, previewDriver);
    }
    DrawSectionEnd();
}

} // namespace

void DrawAtmosphereTab(AtmosphereTabState& state, Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("atmosphereTab");
    for (int sectionIndex = 0; sectionIndex < kAtmosphereSectionCount; ++sectionIndex)
        DrawAtmosphereSection(sectionIndex, state, previewDriver);
    ImGui::Separator();
    ImGui::TextUnformatted("Atmosphere settings are presentation only until they have a PARAMS home.");
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen

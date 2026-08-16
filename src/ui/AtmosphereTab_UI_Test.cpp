// AtmosphereTab_UI_Test.cpp — tab-rebuild C3 acceptance: the Atmosphere tab carries every v1
// atmosphere setting across its eight sections, states each row's limits exactly once in the
// shared control table, and gives every row its own deferral. Pure checks driven with synthetic
// input — no imgui frame, no window, no GL context.
#include "AtmosphereTab_UI.h"
#include "Combo_UI.h"
#include "SliderScalar_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

ScalarSliderPointerInput GrabScalar(float pointerValue) {
    ScalarSliderPointerInput input;
    input.bHandleGrabbed = true;
    input.pointerValue   = pointerValue;
    return input;
}

// The eight plan sections tile the control table exactly: no row is drawn twice and none is
// orphaned, which is what "every setting is present" means for a table-driven tab.
void RunSectionCoverageChecks() {
    Check(kAtmosphereSectionCount == 8, "Sun, Skylight, Exposure & Skybox, four fogs, and Wind");
    int nextExpectedRow = 0;
    for (int sectionIndex = 0; sectionIndex < kAtmosphereSectionCount; ++sectionIndex) {
        const AtmosphereSection& section = atmosphereSections[sectionIndex];
        Check(section.label != nullptr && section.controlCount > 0, "every section names rows");
        Check(section.firstControlIndex == nextExpectedRow, "the sections tile the table with no gap");
        nextExpectedRow += section.controlCount;
    }
    Check(nextExpectedRow == kAtmosphereControlCount, "and together they cover every row exactly once");
    Check(atmosphereSections[0].controlCount == 11, "the Sun section keeps all eleven v1 controls");
    Check(atmosphereSections[7].controlCount == 2, "global wind keeps its speed and direction");
}

// Every row must be drawable: the widget it asks for has to have something to edit.
void RunControlTableChecks() {
    AtmosphereSettings settings;
    int scalarCount = 0, colorCount = 0, vectorCount = 0, textCount = 0, comboCount = 0;
    for (int controlIndex = 0; controlIndex < kAtmosphereControlCount; ++controlIndex) {
        const AtmosphereControl& control = atmosphereControls[controlIndex];
        Check(control.label != nullptr, "every row is labelled");
        switch (control.kind) {
            case AtmosphereControlKind::Scalar: {
                ++scalarCount;
                Check(control.scalarValue != nullptr, "a scalar row names its setting");
                Check(control.range.minimumValue < control.range.maximumValue,
                      "a scalar row has a real range");
                const float value = settings.*control.scalarValue;
                Check(value == ClampScalarSliderValue(value, control.range),
                      "and its v1 default already sits inside that range");
                break;
            }
            case AtmosphereControlKind::Color:
                ++colorCount;
                Check(AtmosphereColorAt(settings, control.slotIndex) != nullptr, "a color row resolves");
                break;
            case AtmosphereControlKind::Vector: {
                ++vectorCount;
                const int componentCount = AtmosphereVectorComponentCount(control.slotIndex);
                Check(AtmosphereVectorAt(settings, control.slotIndex) != nullptr, "a vector row resolves");
                Check(componentCount > 0 && componentCount <= kAtmosphereVectorComponentLimit,
                      "and its components fit the label table");
                break;
            }
            case AtmosphereControlKind::Text:
                ++textCount;
                Check(AtmosphereTextAt(settings, control.slotIndex) != nullptr, "a text row resolves");
                break;
            case AtmosphereControlKind::Combo: ++comboCount; break;
        }
    }
    Check(scalarCount == 40, "forty scalar rows");
    Check(colorCount == 3 && vectorCount == 3 && textCount == 2 && comboCount == 1,
          "three tints, three vectors, two paths and the one intensity mode");
}

// The slot accessors are the only way the table reaches an array or a string, so a slot the
// settings do not have must answer nothing rather than read off the end.
void RunSlotAccessorChecks() {
    AtmosphereSettings settings;
    Check(AtmosphereColorAt(settings, kSunTintSlot) == settings.sunTint, "the sun tint slot resolves");
    Check(AtmosphereColorAt(settings, kAtmosphereColorSlotCount) == nullptr, "an unknown color slot is null");
    Check(AtmosphereVectorAt(settings, kSunPositionSlot) == settings.sunPosition, "sun position resolves");
    Check(AtmosphereVectorComponentCount(kSunPositionSlot) == 3, "sun position is a three-vector");
    Check(AtmosphereVectorComponentCount(kHeightFogRangeSlot) == 2, "the height fog range is a pair");
    Check(AtmosphereVectorComponentCount(-1) == 0, "an unknown vector slot has no components");
    Check(AtmosphereTextAt(settings, kSkyboxPathSlot) == &settings.skyboxPath, "the skybox path resolves");
    Check(AtmosphereTextAt(settings, kAtmosphereTextSlotCount) == nullptr, "an unknown text slot is null");
}

// One RealtimeToggle per ROW: scrubbing the sun's intensity may not defer the skylight's commit.
void RunPerRowDeferralChecks() {
    AtmosphereTabState state;
    const AtmosphereControl& sunIntensity = atmosphereControls[2];
    Check(sunIntensity.scalarValue == &AtmosphereSettings::sunIntensity, "row 2 is the sun intensity");

    WidgetChange change = StepScalarSliderInteraction(state.controlToggles[2],
                                                      state.settings.*sunIntensity.scalarValue,
                                                      sunIntensity.range, GrabScalar(500.0f));
    Check(change.bValueChanged && !change.bCommitted, "scrubbing moves the value and defers the cost");
    Check(state.settings.sunIntensity == 500.0f, "and the named setting is what moved");
    Check(state.controlToggles[2].IsCommitDeferred(), "the sun row owes a commit");
    Check(!state.controlToggles[11].IsCommitDeferred(), "the skylight row owes nothing");

    change = StepScalarSliderInteraction(state.controlToggles[2], state.settings.sunIntensity,
                                         sunIntensity.range, ScalarSliderPointerInput());
    Check(change.bCommitted, "and releasing pays for it exactly once");
}

// The one dropdown on the tab: the v1 SkyIntensityMode, as rows of the shared combo.
void RunSkyboxIntensityModeChecks() {
    AtmosphereTabState state;
    Check(atmosphereControls[18].kind == AtmosphereControlKind::Combo, "row 18 is the intensity mode");
    ComboOptions options;
    options.labels = skyboxIntensityModeLabels;
    options.count  = kSkyboxIntensityModeCount;
    Check(ComboSelectionLabel(state.settings.skyboxIntensityModeIndex, options) == skyboxIntensityModeLabels[0],
          "it opens on Exposure, the v1 default");

    const WidgetChange change = StepComboInteraction(state.settings.skyboxIntensityModeIndex, options, 2);
    Check(change.bValueChanged && change.bCommitted && state.settings.skyboxIntensityModeIndex == 2,
          "picking Multiplier commits on the same frame");
    StepComboInteraction(state.settings.skyboxIntensityModeIndex, options, 9);
    Check(state.settings.skyboxIntensityModeIndex == 2, "a row the list does not have is ignored");
}

} // namespace

int main() {
    RunSectionCoverageChecks();
    RunControlTableChecks();
    RunSlotAccessorChecks();
    RunPerRowDeferralChecks();
    RunSkyboxIntensityModeChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

// ScenariosTab_Detail_UI.cpp — Fix §5's core `ScenarioBody` fields: name, area, spawnsUnits,
// alloyMode (with its consequence card), the spawnIds list, and authoringNote.
// alloys/alloysToAdd/alloysToRemove are ScenariosTab_DetailAlloys_UI.cpp's half (ARCH §1.5 split).
// Layer: UI. `spawnsUnits` RENAMED 2026-08-28, was `navy` (STEP204, ARCH_15_05 §15.5 amended).
// The spawns editor RESHAPED 2026-09-08 (STEP252, ARCH_15_12_ScenarioSpawnIdentity.md §15.12): a
// per-scenario picker over `body.spawnIds` (bare strings) — the pool editor for
// `Scenarios::spawnPoints` itself lives in its own file, ScenariosTab_SpawnPointPool_UI.cpp
// (Scenarios-level, not per-scenario-body).
//
// Backend policy N/A (no PROC stage reads `recipe.scenarios`): every scalar here uses a THROWAWAY,
// function-local `RealtimeToggle` rather than one persisted in `ScenariosTabState` — the value write
// on drag already happens unconditionally inside `StepScalarSliderInteraction` before the toggle is
// even consulted, so a toggle with no cross-frame memory costs nothing real; only `bCommitted`'s
// exact frame is imprecise, and nothing here reads `bCommitted` (there is no dirty flag to trip).
//
// Constitution §8: alloyMode's four labels each carry a real consequence card, per Fix §5.
#include "ScenariosTab_UI.h"
#include "AreasTab_List_UI.h"
#include "Checkbox_UI.h"
#include "Combo_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "../io/ScenarioNameValidation_IO.h"
#include "imgui.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

// STEP251 -- duplicating FilesTab_ScenarioExportRow_Draw_UI.cpp's exact value (same established
// "each file owns its own copy" precedent this ARCH family already uses repeatedly).
const ImVec4 kErrorTextColor(0.95f, 0.35f, 0.35f, 1.0f);

enum : int { kScenarioAlloyModeCount = 4 };
const char* const scenarioAlloyModeLabels[kScenarioAlloyModeCount] = { "Explicit", "Occupancy", "Keep All", "Delta" };
const char* const scenarioAlloyModeConsequenceCards[kScenarioAlloyModeCount] = {
    "You list every army's alloys below. Any army NOT listed loses its alloy markers entirely.",
    "Uses the map's own baked alloy positions. Empty army slots lose their markers; filled slots keep them.",
    "Uses the map's own baked alloy positions. Nothing is ever deleted, even for empty slots.",
    "\xE2\x9A\xA0 Reserved - not yet used by any shipped scenario. Only listed Adds/Removes apply."
};

// A generous, world-coordinate-scale range: DrawScenarioBodyFields carries no `mapSize` to derive a
// tighter one from (unlike AreasTab_UI's AreaOriginSliderRange), so this stays a fixed constant.
ScalarSliderRange ScenarioWorldPositionRange() { return ScalarSliderRange{ -8192.0f, 8192.0f, 0.0f }; }

// The Combo is NOT DrawArmyNameField's exact shape: empty areaName is a real, permanent, authored
// state here ("this scenario owns its own private rectangle"), unlike DrawArmyNameField's transient
// "not chosen yet" -- so this needs one extra leading sentinel entry DrawArmyNameField does not have
// (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28).
void DrawScenarioAreaFields(Params::ScenarioBody& body, const std::vector<Params::MapArea>& areas) {
    std::vector<const char*> labels;
    labels.reserve(areas.size() + 1u);
    labels.push_back("-- Custom (no Area reference) --");
    int selectedIndex = body.areaName.empty() ? 0 : -1;   // -1 = stale reference, matches
                                                          // DrawArmyNameField's own no-match idiom
    for (std::size_t index = 0u; index < areas.size(); ++index) {
        labels.push_back(AreaRowLabel(areas[index]));
        if (!body.areaName.empty() && areas[index].name == body.areaName)
            selectedIndex = static_cast<int>(index) + 1;
    }
    ComboOptions options; options.labels = labels.data(); options.count = static_cast<int>(labels.size());
    if (DrawCombo("Reference Area", selectedIndex, options).bCommitted) {
        if (selectedIndex == 0) {
            body.areaName.clear();
        } else if (selectedIndex > 0) {
            const Params::MapArea& picked = areas[static_cast<std::size_t>(selectedIndex - 1)];
            body.areaName = picked.name;
            // Live-preview copy: the four rect scalars only -- NEVER picked.name (Scenario_PARAMS.h's
            // own comment: area.name is never populated/read anywhere on the wire; leaving it alone
            // keeps that invariant true after a Combo pick, not just at default-construction).
            body.area.originX = picked.originX;
            body.area.originZ = picked.originZ;
            body.area.width   = picked.width;
            body.area.length  = picked.length;
        }
    }

    // Read-only while referenced (ARCH ruling: rejected alternative was editable-with-silent-clear-
    // on-edit -- a slider nudge silently detaching a scenario from its named Area is worse than a
    // slider that visibly refuses input). Sliders stay VISIBLE (never hidden) so resolved numbers are
    // never a black box -- only interaction is disabled.
    const bool bReferenced = !body.areaName.empty();
    ImGui::BeginDisabled(bReferenced);
    const ScalarSliderRange range = ScenarioWorldPositionRange();
    RealtimeToggle originXToggle, originZToggle, widthToggle, lengthToggle;
    DrawSliderScalar("Area Origin X", body.area.originX, range, originXToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Origin Z", body.area.originZ, range, originZToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Width", body.area.width, range, widthToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Length", body.area.length, range, lengthToggle, WidgetStyle(), "%.1f");
    ImGui::EndDisabled();
}

void DrawScenarioAlloyModeField(Params::ScenarioBody& body) {
    ComboOptions options; options.labels = scenarioAlloyModeLabels; options.count = kScenarioAlloyModeCount;
    int modeIndex = static_cast<int>(body.alloyMode);
    if (DrawCombo("Alloy Mode", modeIndex, options).bCommitted)
        body.alloyMode = static_cast<Params::ScenarioAlloyMode>(modeIndex);
    ImGui::TextWrapped("%s", scenarioAlloyModeConsequenceCards[modeIndex]);
}

// Raw `ImGui::InputTextMultiline` per Fix §5's own text — a multi-line box has no shared-library
// widget yet, so this stages a local buffer exactly as TextInput_UI.cpp does for the single-line one.
void DrawAuthoringNoteField(std::string& authoringNote) {
    char buffer[1024];
    std::size_t writtenLength = authoringNote.size() < sizeof(buffer) - 1u
        ? authoringNote.size() : sizeof(buffer) - 1u;
    std::memcpy(buffer, authoringNote.data(), writtenLength);
    buffer[writtenLength] = '\0';
    if (ImGui::InputTextMultiline("##authoringNote", buffer, sizeof(buffer), ImVec2(-1.0f, 60.0f)))
        authoringNote = buffer;
}

} // namespace

// `scenarios` is the OWNING Scenarios struct `body` is a sub-object of (patternScenarios[i].body /
// countScenarios[i].body / defaultScenario) — a mutable sub-reference alongside a const reference to
// its own owner is an established idiom in this exact file family already (DrawScenarioCountList's
// `scenario.conditions` + `scenarios.maxArmySlotCount`, ScenariosTab_ListMechanics_UI.h). Needed here
// for `scenarios.spawnPoints` (the spawnIds picker's own options) and the live inline warning below.
void DrawScenarioBodyFields(Params::ScenarioBody& body, const std::vector<Params::Army>& armies,
                            const std::vector<Params::MapArea>& areas,
                            const Params::Scenarios& scenarios,
                            const Io::ScenarioNameValidationReport& nameReport) {
    TextInputRules nameRules; nameRules.maximumLength = 64; nameRules.bAllowEmpty = true;
    DrawTextInput("Name", body.name, nameRules);
    // STEP251 -- non-blocking, informational, live inline warning: never auto-corrects/renames/
    // truncates/dedupes (ARCH ruling), just surfaces the same rule the export-time gate enforces.
    if (const std::string* invalidReason = nameReport.FindViolationReasonForName(body.name))
        ImGui::TextColored(kErrorTextColor, "%s", ("\xE2\x9A\xA0 " + *invalidReason).c_str());
    ImGui::SeparatorText("Area");
    DrawScenarioAreaFields(body, areas);
    DrawCheckbox("Spawns Units", body.spawnsUnits);
    // STEP251: this text used to describe the now-superseded if/elseif dispatch. SanGen now scaffolds
    // the category-4 generator file itself (ARCH_15_04_ThreeFileOnDiskShape.md "AMENDED 2026-09-03").
    ImGui::TextWrapped("%s", "\xE2\x9A\xA0 Setting this alone spawns nothing. SanGen scaffolds "
        "<MapName>_Scenarios_<Name>.lua on the next Scenario Script export if it does not exist yet -- "
        "open that file and fill in GenerateScenarioUnits(area) to actually spawn units.");
    ImGui::SeparatorText("Alloys");
    DrawScenarioAlloyModeField(body);
    ImGui::SeparatorText("Spawns");
    // Split out of this file for the ARCH §1.5 ceiling — ScenariosTab_DetailSpawns_UI.cpp (mirrors
    // DrawScenarioBodyExtendedFields' own split for alloys, immediately below).
    DrawScenarioSpawnIdsSection(body, scenarios, armies);
    ImGui::SeparatorText("Authoring Note");
    DrawAuthoringNoteField(body.authoringNote);
    DrawScenarioBodyExtendedFields(body, armies);
}

} // namespace Ui
} // namespace SanmapGen

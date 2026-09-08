// ScenariosTab_MatchRules_SlotRangePicker_UI.h — the Tier 2 SlotRangeOccupiedCount clause's own
// From/To picker + bounds banner (STEP253, ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED
// 2026-09-04), split out of ScenariosTab_MatchRules_UI.cpp purely for the ARCH §1.5 file-size
// ceiling — the same posture ScenariosTab_ListMechanics_UI.h has to ScenariosTab_Lists_UI.cpp.
// PRIVATE to ScenariosTab_MatchRules_UI.cpp — the sole translation unit that includes this header.
// Every symbol is `inline`: nothing here is part of the tab's cross-file API.
#pragma once
#include "ScenariosTab_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

// Only meaningful (and only shown by the caller) when condition.field == SlotRangeOccupiedCount.
// Clamped ONLY on the frame the user actually edits a field (Constitution §6 — a load must not
// mutate what it didn't touch, mirroring DrawSlotPatternToggleRow's own discipline) — a pre-existing
// out-of-range value (import, or a later maxArmySlotCount reduction) is left visible so the
// loud/logged export-time validator (ScenarioSlotRangeValidation_IO) can flag it, rather than this
// widget silently reclamping it away every frame.
inline void DrawScenarioSlotRangePicker(Params::ScenarioCountCondition& condition, int maxArmySlotCount) {
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::InputInt("From", &condition.slotRangeStart, 1)) {
        if (condition.slotRangeStart < 1) condition.slotRangeStart = 1;
        if (condition.slotRangeStart > maxArmySlotCount) condition.slotRangeStart = maxArmySlotCount;
    }
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::InputInt("To", &condition.slotRangeEnd, 1)) {
        if (condition.slotRangeEnd < 1) condition.slotRangeEnd = 1;
        if (condition.slotRangeEnd > maxArmySlotCount) condition.slotRangeEnd = maxArmySlotCount;
    }
    const bool bRangeInvalid = condition.slotRangeStart < 1
        || condition.slotRangeStart > condition.slotRangeEnd
        || condition.slotRangeEnd > maxArmySlotCount;
    if (bRangeInvalid) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.15f, 1.0f));
        ImGui::TextWrapped(
            "Invalid slot range [%d, %d] for maxArmySlotCount (%d) — this condition row will "
            "be refused (skipped) at export, never silently reordered/clamped.",
            condition.slotRangeStart, condition.slotRangeEnd, maxArmySlotCount);
        ImGui::PopStyleColor();
    }
}

} // namespace Ui
} // namespace SanmapGen

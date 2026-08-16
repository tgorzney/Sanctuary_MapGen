// Combo_UI.h — the shared dropdown over a fixed option list. Layer: UI.
// UI_FRAMEWORK_SPEC "Universal widget library": every enum-valued parameter in the plan (symmetry
// algorithm, blend mode, noise type, map size, fog intensity mode, faction, …) is drawn by THIS
// control, so a tab never hand-rolls ImGui::BeginCombo. The popup list is imgui's — it is a
// once-per-click interaction, not a hot path, so the bypass toolkit does not apply to it; the
// closed row is drawn with ImDrawList like the rest of the library.
//
// A dropdown has no drag, so it carries NO Ui::RealtimeToggle: a pick is instantaneous and the
// commit lands on the frame the value changes.
//
// Owns no app state: the caller holds the index (an enum it casts) and reads the WidgetChange back.
#pragma once
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// The option list a combo draws. The labels are borrowed, never copied: a caller passes a static
// table (the enum names) or a table it owns for the frame (materials read from a sanpack).
struct ComboOptions {
    const char* const* labels          = nullptr;
    int                count           = 0;
    const char*        emptyLabel      = "<none>";  // shown when the list is empty or nothing is picked
};

// The index a combo may legally show: inside the list, or -1 for "nothing picked". An index that
// points past a list that has since shrunk — a sanpack swapped under a saved recipe — resolves to
// -1 rather than reading off the end (Constitution §6).
inline int ResolvedComboSelection(int selectedIndex, const ComboOptions& options) {
    if (options.labels == nullptr || options.count <= 0) return -1;
    if (selectedIndex < 0 || selectedIndex >= options.count) return -1;
    return selectedIndex;
}

// The label to draw for a selection, never null: an unpicked or out-of-range index answers
// `emptyLabel`, so the closed row always has something to show.
inline const char* ComboSelectionLabel(int selectedIndex, const ComboOptions& options) {
    const int resolved = ResolvedComboSelection(selectedIndex, options);
    if (resolved < 0 || options.labels[resolved] == nullptr)
        return options.emptyLabel != nullptr ? options.emptyLabel : "";
    return options.labels[resolved];
}

// One frame of interaction. `pickedIndex` is the row the popup reported this frame, or -1 for
// "nothing was picked". The resolve runs even on an idle frame, so an index left dangling by a
// shrunken list is corrected the moment the control is drawn rather than at the next click.
inline WidgetChange StepComboInteraction(int& selectedIndex, const ComboOptions& options, int pickedIndex) {
    WidgetChange change;
    // A pick the list cannot honour is IGNORED, never stored as "nothing picked": a stray index is
    // the caller's bug, and clearing a good selection because of one would lose the user's choice.
    const int pickedSelection = ResolvedComboSelection(pickedIndex, options);
    const int updatedIndex = pickedSelection >= 0 ? pickedSelection
                                                  : ResolvedComboSelection(selectedIndex, options);
    if (updatedIndex != selectedIndex) {
        selectedIndex = updatedIndex;
        change.bValueChanged = true;
        change.bCommitted    = true;
    }
    return change;
}

// Draws the label + the closed row + imgui's popup list, and runs the interaction above.
WidgetChange DrawCombo(const char* label, int& selectedIndex, const ComboOptions& options,
                       const WidgetStyle& style = WidgetStyle());

} // namespace Ui
} // namespace SanmapGen

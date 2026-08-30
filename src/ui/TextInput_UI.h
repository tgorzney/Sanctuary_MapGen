// TextInput_UI.h — the shared single-line text field. Layer: UI.
// UI_FRAMEWORK_SPEC "Universal widget library": every name in the plan — layer names, stratum
// names, area names, marker aliases, blueprint paths — is edited through THIS control, so the
// length cap and the "what is a legal name" rule live in one place instead of per tab. The edit
// box itself is imgui's InputText: text entry is a keyboard interaction, never a hot path, so the
// ImDrawList bypass (toolkit §1) would buy nothing.
//
// It carries NO Ui::RealtimeToggle: a text field already has the two-tier shape built in — typing
// is the live edit, leaving the field is the commit — so an RT button would be a second, rival
// control over the same decision (ARCH §4: never add a rival toggle).
//
// Owns no app state: the caller holds the string and reads the WidgetChange back.
#pragma once
#include <string>
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// The largest staging buffer the draw path will put on the stack. A rules cap above this is
// silently lowered, so no caller can ask for an unbounded field.
inline constexpr int kTextInputBufferCapacity = 256;

// What counts as a legal value — settings, not literals in the edit code (Constitution §8).
struct TextInputRules {
    int         maximumLength           = 64;
    bool        bStripControlCharacters = true;   // tabs/newlines pasted into a one-line field
    bool        bTrimSurroundingSpaces  = true;   // applied on commit only, never mid-typing
    bool        bAllowEmpty             = true;
    const char* fallbackText            = "Unnamed";  // used when an empty value is not allowed
};

// The cap actually enforced: the caller's, held inside the staging buffer.
inline int ResolvedTextInputLength(const TextInputRules& rules) {
    if (rules.maximumLength < 0) return 0;
    return rules.maximumLength < kTextInputBufferCapacity - 1 ? rules.maximumLength
                                                              : kTextInputBufferCapacity - 1;
}

// The LIVE form: what the field may hold WHILE typing — control characters dropped and the length
// capped, but surrounding spaces left alone, because trimming mid-edit would eat the space the
// user just typed.
inline std::string LiveTextInput(const std::string& text, const TextInputRules& rules) {
    const int maximumLength = ResolvedTextInputLength(rules);
    std::string result;
    result.reserve(text.size());
    for (const char character : text) {
        if (rules.bStripControlCharacters && static_cast<unsigned char>(character) < 0x20u) continue;
        if (static_cast<int>(result.size()) >= maximumLength) break;
        result.push_back(character);
    }
    return result;
}

// The SETTLED form: the live form plus the trim and the not-empty fallback. Run when the edit
// ends, so what lands in PARAMS is always a legal name.
inline std::string SanitizeTextInput(const std::string& text, const TextInputRules& rules) {
    std::string result = LiveTextInput(text, rules);
    if (rules.bTrimSurroundingSpaces) {
        std::string::size_type firstKept = result.find_first_not_of(' ');
        if (firstKept == std::string::npos) result.clear();
        else result = result.substr(firstKept, result.find_last_not_of(' ') - firstKept + 1);
    }
    if (result.empty() && !rules.bAllowEmpty)
        result = LiveTextInput(rules.fallbackText != nullptr ? rules.fallbackText : "", rules);
    return result;
}

// One frame of interaction, expressed so it is drivable headless from a synthetic key sequence.
//   bTextEditedThisFrame   — the box's contents changed this frame (imgui's InputText returned true).
//   bEditFinishedThisFrame — the user left the box after editing it (IsItemDeactivatedAfterEdit).
struct TextInputSignal {
    bool bTextEditedThisFrame   = false;
    bool bEditFinishedThisFrame = false;
};

// Applies one frame and returns the live/expensive pair: typing moves the caller's string every
// frame (bValueChanged) while the commit arrives once, when the edit ends — the same two-tier
// contract the RT toggle gives the sliders (UI_FRAMEWORK_SPEC §7), with no button to press.
inline WidgetChange StepTextInputInteraction(std::string& value, const std::string& editedText,
                                             const TextInputRules& rules, const TextInputSignal& signal) {
    WidgetChange change;
    if (signal.bTextEditedThisFrame) {
        const std::string liveText = LiveTextInput(editedText, rules);
        if (liveText != value) { value = liveText; change.bValueChanged = true; }
    }
    if (signal.bEditFinishedThisFrame) {
        const std::string settledText = SanitizeTextInput(value, rules);
        if (settledText != value) { value = settledText; change.bValueChanged = true; }
        change.bCommitted = true;
    }
    return change;
}

// Draws the label + the edit box and runs the interaction above. `hintText` is the greyed
// placeholder shown while the box is empty; null draws none. `bLabelHidden` (default false, every
// existing call site unchanged) drops the visible label text while `label` still salts the imgui id
// — for a caller that draws its own label elsewhere (e.g. an inline rename box in a tree row's own
// header-extra zone, MarkersTab_BundleHeaderExtras_UI.h), mirroring ColorSwatchOptions::bLabelHidden's
// established shape.
// `fixedWidthPixels` (default 0.0f, every existing call site unchanged): <= 0 keeps today's 'fill
// remaining content width' behavior; a positive value fixes the box's own width instead, mirroring
// `ColorSwatchOptions::swatchWidth` — the seam that lets a caller sit this control beside others via
// `SameLine()` instead of always claiming the rest of the line.
WidgetChange DrawTextInput(const char* label, std::string& value,
                           const TextInputRules& rules = TextInputRules(),
                           const WidgetStyle& style = WidgetStyle(), const char* hintText = nullptr,
                           bool bLabelHidden = false, float fixedWidthPixels = 0.0f);

} // namespace Ui
} // namespace SanmapGen

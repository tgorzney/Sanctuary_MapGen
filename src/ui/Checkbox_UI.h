// Checkbox_UI.h — the shared boolean tick box, plus the exclusive ("XOR") bit group. Layer: UI.
// UI_FRAMEWORK_SPEC "Universal widget library": tabs compose this, they never call ImGui::Checkbox
// themselves. Drawn with ImDrawList + one InvisibleButton per the bypass toolkit §1; drawing lives
// in the .cpp, everything here is pure and headless-testable.
//
// A tick box has no drag, so it carries NO Ui::RealtimeToggle: there is nothing to defer — the
// click IS the release, and the commit lands on the same frame as the change (UI_FRAMEWORK_SPEC
// §7 is about scrubbing, which a boolean cannot do).
//
// Owns no app state: the caller holds the bool (or the mask) and reads the WidgetChange back.
#pragma once
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// The most bits an exclusive group may carry. The symmetry axis mask — the widest group in the
// plan — is five (Point/X/Z/XY/Radial); 32 is simply the width of the mask word.
inline constexpr int kMaximumExclusiveCheckboxCount = 32;

// One frame of a tick box. `bClickedThisFrame` is what the hit-test reported; the value flips and
// both flags rise together, because a boolean edit is never deferred.
inline WidgetChange StepCheckboxInteraction(bool& value, bool bClickedThisFrame) {
    WidgetChange change;
    if (bClickedThisFrame) {
        value = !value;
        change.bValueChanged = true;
        change.bCommitted    = true;
    }
    return change;
}

// True when `bitIndex` is set in `mask`. Out-of-range indices answer false rather than shifting by
// an illegal amount (Constitution §6 — validate rather than trust the caller's index).
inline bool IsExclusiveCheckboxBitSet(unsigned int mask, int bitIndex) {
    if (bitIndex < 0 || bitIndex >= kMaximumExclusiveCheckboxCount) return false;
    return (mask & (1u << bitIndex)) != 0u;
}

// Repairs a mask that carries more than one bit — a recipe written before a bit was added, or a
// hand-edited file — by keeping the LOWEST set bit, so the group always draws exactly one tick.
// A mask with no legal bits is left empty; whether that is allowed is the caller's `bAllowNone`.
inline unsigned int ResolvedExclusiveCheckboxMask(unsigned int mask, int bitCount) {
    if (bitCount <= 0) return 0u;
    if (bitCount > kMaximumExclusiveCheckboxCount) bitCount = kMaximumExclusiveCheckboxCount;
    const unsigned int legalBits = bitCount >= kMaximumExclusiveCheckboxCount
        ? ~0u : ((1u << bitCount) - 1u);
    const unsigned int inRange = mask & legalBits;
    if (inRange == 0u) return 0u;
    return inRange & (0u - inRange);                     // lowest set bit only
}

// The mask after clicking one box of an exclusive group: the clicked bit becomes the only one set.
// Clicking the bit that is ALREADY set clears the group when `bAllowNone`, and is a no-op when it
// is not — so a group that must always answer something cannot be emptied by a stray click.
inline unsigned int ExclusiveCheckboxMaskAfterClick(unsigned int mask, int bitCount, int clickedIndex,
                                                    bool bAllowNone) {
    const unsigned int resolved = ResolvedExclusiveCheckboxMask(mask, bitCount);
    if (clickedIndex < 0 || clickedIndex >= bitCount || clickedIndex >= kMaximumExclusiveCheckboxCount)
        return resolved;
    const unsigned int clickedBit = 1u << clickedIndex;
    if (resolved == clickedBit) return bAllowNone ? 0u : resolved;
    return clickedBit;
}

// One frame of an exclusive group. `clickedIndex` is the box the hit-test reported, or -1 for
// "none clicked". Like the single box, a click commits immediately.
inline WidgetChange StepExclusiveCheckboxInteraction(unsigned int& mask, int bitCount, int clickedIndex,
                                                     bool bAllowNone) {
    WidgetChange change;
    const unsigned int updatedMask = clickedIndex < 0
        ? ResolvedExclusiveCheckboxMask(mask, bitCount)
        : ExclusiveCheckboxMaskAfterClick(mask, bitCount, clickedIndex, bAllowNone);
    if (updatedMask != mask) {
        mask = updatedMask;
        change.bValueChanged = true;
        change.bCommitted    = true;
    }
    return change;
}

// Draws the box + its label and runs the interaction above.
WidgetChange DrawCheckbox(const char* label, bool& value, const WidgetStyle& style = WidgetStyle());

// Draws `bitCount` tick boxes on one row — `labels[0..bitCount)` — over a single exclusive mask.
WidgetChange DrawExclusiveCheckboxRow(const char* label, unsigned int& mask, const char* const* labels,
                                      int bitCount, bool bAllowNone = true,
                                      const WidgetStyle& style = WidgetStyle());

} // namespace Ui
} // namespace SanmapGen

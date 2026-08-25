// FilePathPicker_UI.h — the browse button + short-path label. Layer: UI. Accuracy class: Visual.
// UI_FRAMEWORK_SPEC "Universal widget library": every path a tab exposes (sanpack, albedo /
// normal / composite textures, RAW imports, gamedata roots, sanmap open/save) is edited through
// THIS control, so validation and the shortened label are written once.
//
// The widget never opens a platform dialog itself: ARCH §3 keeps the platform seam in IO, and
// UI does not depend on IO. It reports `bBrowseRequested` and — for the caller that would rather
// be handed the answer inline — calls an injected FilePathRequestFunction. Wiring the real native
// picker (FileDialog_IO) is a later work-order; until then a host may inject a stub that answers
// a fixed path string.
//
// Owns no app state: the caller holds the std::string and reads the result back (ARCH §3.2).
#pragma once
#include "FilePathLabel_UI.h"
#include "WidgetHelpers_UI.h"
#include <string>

namespace SanmapGen {
namespace Ui {

// The injected seam. Returns true and writes `outChosenPath` when the user picked something;
// false when the dialog was cancelled (leaving the setting untouched).
using FilePathRequestFunction = bool (*)(void* userData, const char* startingPath, std::string& outChosenPath);

// Per-picker tweakables (Constitution §8).
struct FilePathPickerOptions {
    // STEP153: no longer read by DrawFilePathPicker's rendering — the single button's visible text
    // is always the caller's `label` argument now. Kept on the struct so existing callers that set
    // it still compile; a later ticket may retire the field outright.
    const char* browseButtonLabel       = "Browse...";
    const char* allowedExtensions       = nullptr;  // ';'-separated (".png;.dds"); null = any
    int         maximumLabelCharacterCount = 40;
    bool        bClearButtonShown       = true;     // STEP153: surfaced as a right-click "Clear" menu item
    FilePathRequestFunction RequestFilePath = nullptr;
    void*                   requestUserData = nullptr;
};

// What one picker row did this frame. `change` follows the library contract (WidgetHelpers_UI.h):
// a path pick is a discrete event with no drag to defer, so bValueChanged and bCommitted always
// arrive on the same frame.
struct FilePathPickerResult {
    WidgetChange change;
    bool bBrowseRequested   = false;   // the host should run the platform dialog (FileDialog_IO)
    bool bRejectedExtension = false;   // the chosen path failed the fence; the setting is untouched
};

// Applies a path the dialog (or a test) chose. The extension fence runs FIRST: a rejected path is
// reported and NOT written, so a wrong file can never reach a loader (Constitution §6). Clearing
// the setting is always legal, so an empty choice skips the fence.
inline FilePathPickerResult ApplyChosenFilePath(std::string& filePath, const std::string& chosenPath,
                                                const FilePathPickerOptions& options) {
    FilePathPickerResult result;
    if (!chosenPath.empty() && !HasAllowedFileExtension(chosenPath, options.allowedExtensions)) {
        result.bRejectedExtension = true;
        return result;
    }
    if (chosenPath == filePath) return result;                 // re-picking the same file is free
    filePath = chosenPath;
    result.change.bValueChanged = true;
    result.change.bCommitted    = true;
    return result;
}

// Empties the setting. Reports a change only when there was something to clear.
inline FilePathPickerResult ClearFilePath(std::string& filePath) {
    return ApplyChosenFilePath(filePath, std::string(), FilePathPickerOptions());
}

// True when the stored path is one this picker would accept — how a tab greys out or flags a
// setting loaded from a recipe written against different rules.
inline bool StoredFilePathIsAllowed(const std::string& filePath, const FilePathPickerOptions& options) {
    return filePath.empty() || HasAllowedFileExtension(filePath, options.allowedExtensions);
}

// Draws ONE button (labelled `label`) — STEP153: the current path (or "None") and, when the fence
// would reject the stored value, a warning, both live in the hover tooltip; clearing (when
// `bClearButtonShown`) lives on a right-click context menu on the same button. Runs the injected
// request seam when the button is pressed.
FilePathPickerResult DrawFilePathPicker(const char* label, std::string& filePath,
                                        const FilePathPickerOptions& options = FilePathPickerOptions(),
                                        const WidgetStyle& style = WidgetStyle());

} // namespace Ui
} // namespace SanmapGen

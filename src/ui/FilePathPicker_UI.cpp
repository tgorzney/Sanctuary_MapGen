// FilePathPicker_UI.cpp — the imgui draw path of the browse button + short-path label. Layer: UI.
// All validation, shortening and the change contract are pure and live in the headers
// (WidgetHelpers_UI.h "THE SPLIT"), so this file is only buttons, layout and a tooltip.
#include "FilePathPicker_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Runs the injected seam, if any. With no seam the row only reports the request and the host —
// which owns the platform layer — opens the dialog itself on the next frame.
FilePathPickerResult RequestAndApplyFilePath(std::string& filePath, const FilePathPickerOptions& options) {
    FilePathPickerResult result;
    result.bBrowseRequested = true;
    if (options.RequestFilePath == nullptr) return result;
    std::string chosenPath;
    if (!options.RequestFilePath(options.requestUserData, filePath.c_str(), chosenPath)) return result;
    const FilePathPickerResult applied = ApplyChosenFilePath(filePath, chosenPath, options);
    result.change             = applied.change;
    result.bRejectedExtension = applied.bRejectedExtension;
    return result;
}

} // namespace

FilePathPickerResult DrawFilePathPicker(const char* label, std::string& filePath,
                                        const FilePathPickerOptions& options, const WidgetStyle& style) {
    FilePathPickerResult result;
    ImGui::PushID(label);

    // STEP153: a genuine single button — the browse button's OWN label doubles as this control's
    // only visible element ("ANYTHING that selects a file/s or Folder/s should be the single button
    // widget"); the current path (or the fence-rejected warning) moved to the hover tooltip below,
    // and clearing moved to a right-click context menu on this same button.
    if (ImGui::Button(label)) result = RequestAndApplyFilePath(filePath, options);

    // Reuses the existing shortened-path helper unchanged; the "greyed" fence warning that used to
    // live in a separate always-visible text row is folded into the same tooltip rather than lost.
    const std::string shortLabel = ShortenedFilePathLabel(filePath, options.maximumLabelCharacterCount);
    const bool bStoredPathAllowed = StoredFilePathIsAllowed(filePath, options);
    if (ImGui::IsItemHovered()) {
        if (bStoredPathAllowed) ImGui::SetTooltip("%s", shortLabel.c_str());
        else ImGui::SetTooltip("%s (not accepted by this picker)", shortLabel.c_str());
    }

    if (options.bClearButtonShown && ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Clear") && !filePath.empty())
            result.change = ClearFilePath(filePath).change;
        ImGui::EndPopup();
    }

    (void)style;                     // the row is plain imgui items; nothing to style yet
    ImGui::PopID();
    return result;
}

} // namespace Ui
} // namespace SanmapGen

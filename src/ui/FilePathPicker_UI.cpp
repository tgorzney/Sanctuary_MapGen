// FilePathPicker_UI.cpp — the imgui draw path of the browse button + short-path label. Layer: UI.
// All validation, shortening and the change contract are pure and live in the headers
// (WidgetHelpers_UI.h "THE SPLIT"), so this file is only buttons, layout and a tooltip.
#include "FilePathPicker_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The clear button's width, derived from the current font so it stays square-ish at any scale.
float ResolveClearButtonWidth() { return ImGui::GetFrameHeight(); }

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
    ImGui::TextUnformatted(label);

    const char* const buttonLabel = options.browseButtonLabel != nullptr ? options.browseButtonLabel : "Browse...";
    if (ImGui::Button(buttonLabel)) result = RequestAndApplyFilePath(filePath, options);

    if (options.bClearButtonShown) {
        ImGui::SameLine();
        if (ImGui::Button("x", ImVec2(ResolveClearButtonWidth(), 0.0f)) && !filePath.empty())
            result.change = ClearFilePath(filePath).change;
    }

    // The label is greyed when the stored path would not survive this picker's own fence, so a
    // setting carried in from an older recipe is visible rather than silently trusted.
    const std::string shortLabel = ShortenedFilePathLabel(filePath, options.maximumLabelCharacterCount);
    const bool bStoredPathAllowed = StoredFilePathIsAllowed(filePath, options);
    ImGui::SameLine();
    if (bStoredPathAllowed) ImGui::TextUnformatted(shortLabel.c_str());
    else ImGui::TextDisabled("%s", shortLabel.c_str());
    if (!filePath.empty() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", filePath.c_str());

    (void)style;                     // the row is plain imgui items; nothing to style yet
    ImGui::PopID();
    return result;
}

} // namespace Ui
} // namespace SanmapGen

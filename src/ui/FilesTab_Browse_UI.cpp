// FilesTab_Browse_UI.cpp — the picker row and the native dialog behind it. Layer: UI.
// The only place the Files tab touches `Io::FileDialog`. With no native picker compiled in the
// row still edits its path by hand, so the tab degrades to a typed path rather than to a dead
// button (FileDialog_IO.h, IsNativeFileDialogAvailable).
#include "FilesTab_Browse_UI.h"
#include "FilePathPicker_UI.h"
#include "../io/FileDialog_IO.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The type dropdowns, per kind. "All files" stays last so a map written with an odd extension is
// still reachable rather than invisible.
const Io::FileDialogFilter sanmapDialogFilters[] = {
    { "Sanctuary map", "*.sanmap" }, { "All files", "*.*" } };
const Io::FileDialogFilter supComLuaDialogFilters[] = {
    { "Supreme Commander save", "*.lua" }, { "All files", "*.*" } };

Io::FileDialogRequest BuildDialogRequest(FilesTabBrowseKind kind, const std::string& startingPath) {
    Io::FileDialogRequest request;
    request.startingDirectory = startingPath.empty() ? nullptr : startingPath.c_str();
    if (kind == FilesTabBrowseKind::SanmapDocument) {
        request.title            = "Open Sanmap File";
        request.defaultExtension = ".sanmap";
        request.filters          = sanmapDialogFilters;
        request.filterCount      = 2;
    } else if (kind == FilesTabBrowseKind::SupComLuaDocument) {
        request.title            = "Import Supreme Commander Lua";
        request.defaultExtension = ".lua";
        request.filters          = supComLuaDialogFilters;
        request.filterCount      = 2;
    } else {
        request.title = "Choose the destination map folder";
    }
    return request;
}

bool RunBrowseDialog(FilesTabBrowseKind kind, const std::string& startingPath,
                     std::string& outChosenPath) {
    const Io::FileDialogRequest request = BuildDialogRequest(kind, startingPath);
    if (kind == FilesTabBrowseKind::ExportFolder)
        return Io::FileDialog::SelectDirectoryPath(request, outChosenPath);
    return Io::FileDialog::OpenFilePath(request, outChosenPath);
}

// The extension fence, per kind. A `.sanmap` row is deliberately UNFENCED: the importer resolves
// either the document or the map folder that holds it (MapImporter_IO.h), so an extension test
// would reject a perfectly good folder. The `.lua` row is fenced, since only a save file parses.
FilePathPickerOptions BuildPickerOptions(FilesTabBrowseKind kind) {
    FilePathPickerOptions options;
    options.allowedExtensions = kind == FilesTabBrowseKind::SupComLuaDocument ? ".lua" : nullptr;
    options.browseButtonLabel = "Browse...";
    return options;
}

} // namespace

bool DrawFilesTabPathRow(const char* label, FilesTabBrowseKind kind, std::string& filePath) {
    const FilePathPickerOptions options = BuildPickerOptions(kind);
    FilePathPickerResult result = DrawFilePathPicker(label, filePath, options);
    if (result.bBrowseRequested) {
        std::string chosenPath;
        if (RunBrowseDialog(kind, filePath, chosenPath))
            result = ApplyChosenFilePath(filePath, chosenPath, options);
        else if (!Io::IsNativeFileDialogAvailable())
            ImGui::TextUnformatted("No native file dialog on this platform; type the path instead.");
    }
    if (result.bRejectedExtension)
        ImGui::TextUnformatted("That file type is not accepted here; the setting was left alone.");
    return result.change.bCommitted;
}

} // namespace Ui
} // namespace SanmapGen

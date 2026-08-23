// FilesTab_Browse_UI.h — MODULE-INTERNAL browse plumbing for the Files tab. Layer: UI.
// Split out under the ARCH §1.5 ceilings: FilesTab_Draw_UI.cpp composes the sections, this pair
// owns the one thing every path row shares — the picker row plus the native dialog behind its
// Browse button. Nothing outside the FilesTab module includes this header; it declares no new
// public type (ARCH §8.4).
//
// The row follows FilePathPicker_UI.h's own contract exactly: the WIDGET never opens a dialog, it
// reports `bBrowseRequested`, and the HOST — this file, the Files tab being the tab that drives
// IO — runs `Io::FileDialog`. A cancelled dialog leaves the setting untouched (Constitution §6).
#pragma once
#include <string>

namespace SanmapGen {
namespace Ui {

// What the Browse button should open. Each kind carries its own dialog title, filter set and
// mode; they are settings in FilesTab_Browse_UI.cpp, never literals at a call site.
enum class FilesTabBrowseKind {
    SanmapDocument,      // an EXISTING .sanmap file (the importer also resolves a map folder)
    SupComLuaDocument,   // an EXISTING Supreme Commander `_save.lua`
    ExportFolder,        // the destination map FOLDER — "a map on disk is a folder"
    GameInstallRoot,     // STEP77: the game install FOLDER (GameInstallLocation_IO validates it)
    ScenarioRuntimeOverrideLua,   // STEP77: an EXISTING Runtime Script override `.lua`
};

// Draws one labelled picker row and runs the native dialog when its Browse button is pressed.
// True when `filePath` actually moved this frame.
bool DrawFilesTabPathRow(const char* label, FilesTabBrowseKind kind, std::string& filePath);

// Draws the button itself and, on click, runs the native Open dialog directly — no separate
// Browse step, no picker row. True when the user picked a file (`filePath` was updated).
bool DrawFilesTabOpenButton(const char* label, std::string& filePath);

} // namespace Ui
} // namespace SanmapGen

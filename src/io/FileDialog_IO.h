// FileDialog_IO.h — the native file/folder picker. Layer: IO / BRIDGE — this IS the platform
// seam (Constitution §5, ARCH §3.3): the only place the shell's dialog COM objects are touched,
// so the eventual UE-plugin port re-implements exactly this file and nothing else.
//
// UI never calls a platform dialog itself (FilePathPicker_UI.h says so in as many words): a tab
// reports `bBrowseRequested` and its HOST runs one of the three functions below. The dialog is
// LOADING/SAVING only — it never simulates and never touches GPU state (ARCH §3.1/§3.2).
//
// Cancel is NOT a failure: every entry point answers false and leaves `outChosenPath` untouched,
// so a cancelled browse can never blank a setting (Constitution §6).
#pragma once
#include <string>

namespace SanmapGen {
namespace Io {

// One row of the dialog's type dropdown. `extensionPattern` is the shell's own syntax —
// "*.sanmap", or ";"-joined for several ("*.raw;*.r16") — kept verbatim because it is a
// platform-dictated string (ARCH §1.1 naming exception).
struct FileDialogFilter {
    const char* description      = nullptr;   // "Sanctuary map"
    const char* extensionPattern = nullptr;   // "*.sanmap"
};

// Everything one dialog needs. Every field is optional; a default-constructed request opens a
// plain "any file" picker.
struct FileDialogRequest {
    const char*             title             = nullptr;
    const char*             defaultExtension  = nullptr;   // "sanmap" or ".sanmap" — both accepted
    const char*             startingDirectory = nullptr;
    const FileDialogFilter* filters           = nullptr;
    int                     filterCount       = 0;

    // A request is only usable when its filter array agrees with its count — the one way a
    // caller can hand the shell a wild pointer (Constitution §6).
    bool IsValid() const {
        if (filterCount < 0) return false;
        return filterCount == 0 || filters != nullptr;
    }
};

// The shell wants the default extension WITHOUT its leading dot, while every setting in the tree
// carries one (".sanmap"). Pure, so the normalization is testable without opening a dialog.
inline std::string NormalizedDefaultExtension(const char* defaultExtension) {
    if (defaultExtension == nullptr) return std::string();
    const char* readCursor = defaultExtension;
    while (*readCursor == '.') ++readCursor;
    return std::string(readCursor);
}

// False on a platform with no native picker compiled in; a host then falls back to a typed path
// rather than silently doing nothing.
bool IsNativeFileDialogAvailable();

class FileDialog {
public:
    // Pick an EXISTING file. False = cancelled, unavailable, or an invalid request.
    static bool OpenFilePath(const FileDialogRequest& request, std::string& outChosenPath);

    // Pick a file to WRITE (the shell adds the default extension and prompts on overwrite).
    static bool SaveFilePath(const FileDialogRequest& request, std::string& outChosenPath);

    // Pick a FOLDER — what the .sanmap export targets, since a map on disk is a folder
    // (SANMAP_FORMAT_SPEC "A map on disk is a folder").
    static bool SelectDirectoryPath(const FileDialogRequest& request, std::string& outChosenPath);
};

} // namespace Io
} // namespace SanmapGen

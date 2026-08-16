// FileDialog_IO.cpp — the three public picker entry points. Layer: IO.
// Each validates its request first (Constitution §6) and then hands off to the platform seam;
// on a platform with no seam compiled in they all answer false without touching the caller's
// string, so a host can detect the situation with IsNativeFileDialogAvailable() and fall back
// to a typed path instead of appearing to do nothing.
#include "FileDialog_IO.h"
#include "FileDialog_Shell_IO.h"

namespace SanmapGen {
namespace Io {

#ifdef _WIN32

bool IsNativeFileDialogAvailable() { return true; }

bool FileDialog::OpenFilePath(const FileDialogRequest& request, std::string& outChosenPath) {
    if (!request.IsValid()) return false;
    return RunShellFileDialog(FileDialogMode::OpenFile, request, outChosenPath);
}

bool FileDialog::SaveFilePath(const FileDialogRequest& request, std::string& outChosenPath) {
    if (!request.IsValid()) return false;
    return RunShellFileDialog(FileDialogMode::SaveFile, request, outChosenPath);
}

bool FileDialog::SelectDirectoryPath(const FileDialogRequest& request, std::string& outChosenPath) {
    if (!request.IsValid()) return false;
    return RunShellFileDialog(FileDialogMode::SelectDirectory, request, outChosenPath);
}

#else

bool IsNativeFileDialogAvailable() { return false; }

bool FileDialog::OpenFilePath(const FileDialogRequest&, std::string&) { return false; }
bool FileDialog::SaveFilePath(const FileDialogRequest&, std::string&) { return false; }
bool FileDialog::SelectDirectoryPath(const FileDialogRequest&, std::string&) { return false; }

#endif // _WIN32

} // namespace Io
} // namespace SanmapGen

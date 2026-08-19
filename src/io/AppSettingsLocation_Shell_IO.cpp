// AppSettingsLocation_Shell_IO.cpp — the Win32 SHGetKnownFolderPath plumbing behind
// DefaultAppSettingsDirectory. Layer: IO. The ONE translation unit that resolves the bootstrap
// settings folder; everything above it sees one plain function returning std::string
// (Constitution §5 — the swappable seam), the same shape FileDialog_Shell_IO.cpp already
// establishes for the file-picker seam.
#include "AppSettingsLocation_IO.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>

// Linked here rather than in CMake so a new IO translation unit never forces a build-file edit —
// FileDialog_Shell_IO.cpp's own precedent.
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

namespace SanmapGen {
namespace Io {
namespace {

constexpr const char* kApplicationFolderName = "SanGen";

// UTF-16 -> UTF-8, empty in, empty out, never throws — the read half of the same conversion
// FileDialog_Shell_IO.cpp performs for the dialog seam.
std::string NarrowUtf16Path(const wchar_t* utf16Text) {
    if (utf16Text == nullptr || *utf16Text == L'\0') return std::string();
    const int byteCount = WideCharToMultiByte(CP_UTF8, 0, utf16Text, -1, nullptr, 0, nullptr, nullptr);
    if (byteCount <= 1) return std::string();
    std::string narrowed(static_cast<std::size_t>(byteCount - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16Text, -1, &narrowed[0], byteCount, nullptr, nullptr);
    return narrowed;
}

} // namespace

std::string DefaultAppSettingsDirectory() {
    PWSTR roamingAppDataPath = nullptr;
    std::string resolvedDirectory;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roamingAppDataPath))
        && roamingAppDataPath != nullptr) {
        resolvedDirectory = NarrowUtf16Path(roamingAppDataPath);
    }
    if (roamingAppDataPath != nullptr) CoTaskMemFree(roamingAppDataPath);
    if (resolvedDirectory.empty()) return std::string();
    const char lastCharacter = resolvedDirectory.back();
    if (lastCharacter != '/' && lastCharacter != '\\') resolvedDirectory += '\\';
    return resolvedDirectory + kApplicationFolderName;
}

} // namespace Io
} // namespace SanmapGen

#else // !_WIN32

namespace SanmapGen {
namespace Io {

std::string DefaultAppSettingsDirectory() { return std::string(); }

} // namespace Io
} // namespace SanmapGen

#endif // _WIN32

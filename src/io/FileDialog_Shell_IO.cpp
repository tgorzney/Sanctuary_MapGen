// FileDialog_Shell_IO.cpp — the Win32 shell dialog plumbing behind FileDialog_IO. Layer: IO.
// The ONE translation unit in src/ that talks to the shell's COM dialogs; everything above it
// sees three plain functions taking std::string (Constitution §5 — the swappable seam).
#include "FileDialog_Shell_IO.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>

// Linked here rather than in CMake so a new IO translation unit never forces a build-file edit.
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

namespace SanmapGen {
namespace Io {
namespace {

// The wide filter strings must outlive the COMDLG_FILTERSPEC array that points at them, so the
// caller owns this storage for the whole call.
struct ShellFilterStorage {
    std::vector<std::wstring>     text;
    std::vector<COMDLG_FILTERSPEC> specifications;
};

void BuildShellFilters(const FileDialogRequest& request, ShellFilterStorage& storage) {
    if (request.filters == nullptr || request.filterCount <= 0) return;
    storage.text.reserve(static_cast<std::size_t>(request.filterCount) * 2u);
    for (int filterIndex = 0; filterIndex < request.filterCount; ++filterIndex) {
        const FileDialogFilter& filter = request.filters[filterIndex];
        if (filter.description == nullptr || filter.extensionPattern == nullptr) continue;
        storage.text.push_back(WidenUtf8Text(filter.description));
        storage.text.push_back(WidenUtf8Text(filter.extensionPattern));
    }
    for (std::size_t textIndex = 0; textIndex + 1 < storage.text.size(); textIndex += 2) {
        COMDLG_FILTERSPEC specification;
        specification.pszName = storage.text[textIndex].c_str();
        specification.pszSpec = storage.text[textIndex + 1].c_str();
        storage.specifications.push_back(specification);
    }
}

void ConfigureShellDialog(IFileDialog* dialog, FileDialogMode mode, const FileDialogRequest& request,
                          ShellFilterStorage& storage) {
    DWORD dialogOptions = 0;
    if (SUCCEEDED(dialog->GetOptions(&dialogOptions))) {
        dialogOptions |= FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR;
        if (mode == FileDialogMode::SelectDirectory) dialogOptions |= FOS_PICKFOLDERS;
        if (mode == FileDialogMode::SaveFile)        dialogOptions |= FOS_OVERWRITEPROMPT;
        if (mode == FileDialogMode::OpenFile)        dialogOptions |= FOS_FILEMUSTEXIST;
        dialog->SetOptions(dialogOptions);
    }
    if (request.title != nullptr) dialog->SetTitle(WidenUtf8Text(request.title).c_str());
    if (request.startingDirectory != nullptr && *request.startingDirectory != '\0') {
        IShellItem* startingItem = nullptr;
        const std::wstring startingText = WidenUtf8Text(request.startingDirectory);
        if (SUCCEEDED(SHCreateItemFromParsingName(startingText.c_str(), nullptr, IID_IShellItem,
                                                  reinterpret_cast<void**>(&startingItem)))
            && startingItem != nullptr) {
            dialog->SetFolder(startingItem);
            startingItem->Release();
        }
    }
    const std::string defaultExtension = NormalizedDefaultExtension(request.defaultExtension);
    if (!defaultExtension.empty()) dialog->SetDefaultExtension(WidenUtf8Text(defaultExtension).c_str());
    if (mode != FileDialogMode::SelectDirectory) {
        BuildShellFilters(request, storage);
        if (!storage.specifications.empty())
            dialog->SetFileTypes(static_cast<UINT>(storage.specifications.size()),
                                 storage.specifications.data());
    }
}

// The picked item's filesystem path, or an empty string when the shell answered a virtual item.
std::string ChosenPathOfDialog(IFileDialog* dialog) {
    std::string chosenPath;
    IShellItem* shellItem = nullptr;
    if (FAILED(dialog->GetResult(&shellItem)) || shellItem == nullptr) return chosenPath;
    PWSTR filesystemPath = nullptr;
    if (SUCCEEDED(shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filesystemPath))) {
        chosenPath = NarrowUtf16Text(filesystemPath);
        CoTaskMemFree(filesystemPath);
    }
    shellItem->Release();
    return chosenPath;
}

} // namespace

std::wstring WidenUtf8Text(const std::string& utf8Text) {
    if (utf8Text.empty()) return std::wstring();
    const int characterCount = MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(),
                                                   static_cast<int>(utf8Text.size()), nullptr, 0);
    if (characterCount <= 0) return std::wstring();
    std::wstring widened(static_cast<std::size_t>(characterCount), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), static_cast<int>(utf8Text.size()),
                        &widened[0], characterCount);
    return widened;
}

std::string NarrowUtf16Text(const wchar_t* utf16Text) {
    if (utf16Text == nullptr || *utf16Text == L'\0') return std::string();
    const int byteCount = WideCharToMultiByte(CP_UTF8, 0, utf16Text, -1, nullptr, 0, nullptr, nullptr);
    if (byteCount <= 1) return std::string();
    std::string narrowed(static_cast<std::size_t>(byteCount - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16Text, -1, &narrowed[0], byteCount, nullptr, nullptr);
    return narrowed;
}

bool RunShellFileDialog(FileDialogMode mode, const FileDialogRequest& request,
                        std::string& outChosenPath) {
    const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const CLSID dialogClassId = mode == FileDialogMode::SaveFile ? CLSID_FileSaveDialog
                                                                 : CLSID_FileOpenDialog;
    IFileDialog* dialog = nullptr;
    bool bChosen = false;
    if (SUCCEEDED(CoCreateInstance(dialogClassId, nullptr, CLSCTX_ALL, IID_IFileDialog,
                                   reinterpret_cast<void**>(&dialog))) && dialog != nullptr) {
        ShellFilterStorage filterStorage;
        ConfigureShellDialog(dialog, mode, request, filterStorage);
        if (SUCCEEDED(dialog->Show(nullptr))) {
            const std::string chosenPath = ChosenPathOfDialog(dialog);
            if (!chosenPath.empty()) { outChosenPath = chosenPath; bChosen = true; }
        }
        dialog->Release();
    }
    if (SUCCEEDED(initializeResult)) CoUninitialize();
    return bChosen;
}

} // namespace Io
} // namespace SanmapGen
#endif // _WIN32

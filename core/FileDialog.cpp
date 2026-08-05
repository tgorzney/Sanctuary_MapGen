#include "FileDialog.h"
#include <windows.h>
#include <shlobj.h>
#include <iostream>

namespace SanmapGen {

bool FileDialog::SaveFile(const char* filter, const char* defaultExt, std::string& outPath) {
    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileSaveDialog* pDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pDialog));
    if (SUCCEEDED(hr)) {
        // We could parse the 'filter' string to COMDLG_FILTERSPEC if we wanted, but for simplicity
        // we'll set a generic filter or omit it, allowing any files, unless we want to do the complex parsing.
        // Since the prompt is for modern dialogs, let's keep it simple and just show the dialog.
        
        // Convert defaultExt to wide string
        if (defaultExt != nullptr) {
            int len = MultiByteToWideChar(CP_UTF8, 0, defaultExt, -1, NULL, 0);
            std::wstring wExt(len, 0);
            MultiByteToWideChar(CP_UTF8, 0, defaultExt, -1, &wExt[0], len);
            pDialog->SetDefaultExtension(wExt.c_str());
        }

        if (SUCCEEDED(pDialog->Show(NULL))) {
            IShellItem* pItem;
            if (SUCCEEDED(pDialog->GetResult(&pItem))) {
                PWSTR pszFilePath;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    std::string strTo(size_needed, 0);
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &strTo[0], size_needed, NULL, NULL);
                    if (!strTo.empty() && strTo.back() == '\0') strTo.pop_back();
                    outPath = strTo;
                    
                    CoTaskMemFree(pszFilePath);
                    pItem->Release();
                    pDialog->Release();
                    if (SUCCEEDED(hrInit)) CoUninitialize();
                    return true;
                }
                pItem->Release();
            }
        }
        pDialog->Release();
        if (SUCCEEDED(hrInit)) CoUninitialize();
        return false; // User cancelled
    }
    if (SUCCEEDED(hrInit)) CoUninitialize();

    // Fallback to legacy
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn) == TRUE) {
        outPath = ofn.lpstrFile;
        return true;
    }
    return false;
}

bool FileDialog::OpenFile(const char* filter, std::string& outPath) {
    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileOpenDialog* pDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pDialog));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        if (SUCCEEDED(pDialog->GetOptions(&dwOptions))) {
            pDialog->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);
        }
        if (SUCCEEDED(pDialog->Show(NULL))) {
            IShellItem* pItem;
            if (SUCCEEDED(pDialog->GetResult(&pItem))) {
                PWSTR pszFilePath;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    std::string strTo(size_needed, 0);
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &strTo[0], size_needed, NULL, NULL);
                    if (!strTo.empty() && strTo.back() == '\0') strTo.pop_back();
                    outPath = strTo;
                    
                    CoTaskMemFree(pszFilePath);
                    pItem->Release();
                    pDialog->Release();
                    if (SUCCEEDED(hrInit)) CoUninitialize();
                    return true;
                }
                pItem->Release();
            }
        }
        pDialog->Release();
        if (SUCCEEDED(hrInit)) CoUninitialize();
        return false; // User cancelled
    }
    if (SUCCEEDED(hrInit)) CoUninitialize();

    // Fallback to legacy
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        outPath = ofn.lpstrFile;
        return true;
    }
    return false;
}

bool FileDialog::SelectFolder(std::string& outPath) {
    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IFileOpenDialog* pDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pDialog));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        if (SUCCEEDED(pDialog->GetOptions(&dwOptions))) {
            pDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }
        
        if (SUCCEEDED(pDialog->Show(NULL))) {
            IShellItem* pItem;
            if (SUCCEEDED(pDialog->GetResult(&pItem))) {
                PWSTR pszFilePath;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    std::string strTo(size_needed, 0);
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &strTo[0], size_needed, NULL, NULL);
                    if (!strTo.empty() && strTo.back() == '\0') strTo.pop_back();
                    outPath = strTo;
                    
                    CoTaskMemFree(pszFilePath);
                    pItem->Release();
                    pDialog->Release();
                    if (SUCCEEDED(hrInit)) CoUninitialize();
                    return true;
                }
                pItem->Release();
            }
        }
        pDialog->Release();
    }
    if (SUCCEEDED(hrInit)) CoUninitialize();
    
    // Fallback to legacy
    BROWSEINFOA bi = { 0 };
    bi.lpszTitle = "Select Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != 0) {
        CHAR path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            outPath = path;
            CoTaskMemFree(pidl);
            return true;
        }
        CoTaskMemFree(pidl);
    }
    return false;
}

}

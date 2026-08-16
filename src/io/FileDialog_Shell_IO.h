// FileDialog_Shell_IO.h — MODULE-INTERNAL declarations of FileDialog_IO's shell plumbing.
// Layer: IO. Split out under the ARCH §1.5 ceilings: `FileDialog_IO.cpp` holds the three public
// entry points, this pair holds the COM/wide-string work they share. Nothing outside the
// FileDialog module includes this header — it declares no new public type (ARCH §8.4).
#pragma once
#include "FileDialog_IO.h"

#ifdef _WIN32
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

enum class FileDialogMode { OpenFile, SaveFile, SelectDirectory };

// UTF-8 <-> UTF-16 at the shell boundary. Empty in, empty out; never throws.
std::wstring WidenUtf8Text(const std::string& utf8Text);
std::string  NarrowUtf16Text(const wchar_t* utf16Text);

// Runs one configured dialog to completion. Writes `outChosenPath` and answers true ONLY when
// the user actually picked something; a cancel leaves the caller's string untouched.
bool RunShellFileDialog(FileDialogMode mode, const FileDialogRequest& request,
                        std::string& outChosenPath);

} // namespace Io
} // namespace SanmapGen
#endif // _WIN32

#pragma once
#include <string>

namespace SanmapGen {

class FileDialog {
public:
    // Opens a native "Save File" dialog. Returns true if successful and populates outPath.
    static bool SaveFile(const char* filter, const char* defaultExt, std::string& outPath);

    // Opens a native "Open File" dialog. Returns true if successful and populates outPath.
    static bool OpenFile(const char* filter, std::string& outPath);

    // Opens a native "Select Folder" dialog. Returns true if successful and populates outPath.
    static bool SelectFolder(std::string& outPath);
};

}

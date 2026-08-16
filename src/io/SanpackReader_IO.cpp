// SanpackReader_IO.cpp — open/close lifecycle and the needed-entry filter (ARCH §1.5 aspect
// split; the mapping, the directory parse and the extraction pass live in the sibling
// SanpackReader_*_IO.cpp files behind the one header).
#include "SanpackReader_IO.h"
#include "SanpackReader_Format_IO.h"
#include <iostream>

namespace SanmapGen {
namespace Io {

namespace {

char LowerCaseAscii(char character) {
    return (character >= 'A' && character <= 'Z') ? static_cast<char>(character - 'A' + 'a') : character;
}

bool StartsWith(const std::string& text, const std::string& prefix) {
    if (prefix.size() > text.size()) return false;
    for (std::size_t index = 0; index < prefix.size(); ++index)
        if (LowerCaseAscii(text[index]) != LowerCaseAscii(prefix[index])) return false;
    return true;
}

bool EndsWith(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) return false;
    const std::size_t offset = text.size() - suffix.size();
    for (std::size_t index = 0; index < suffix.size(); ++index)
        if (LowerCaseAscii(text[offset + index]) != LowerCaseAscii(suffix[index])) return false;
    return true;
}

} // namespace

bool SanpackEntryFilter::Accepts(const std::string& entryName) const {
    if (entryName.empty() || entryName.back() == '/') return false;   // directory record
    bool bPrefixAccepted = pathPrefixes.empty();
    for (std::size_t index = 0; index < pathPrefixes.size() && !bPrefixAccepted; ++index)
        bPrefixAccepted = StartsWith(entryName, pathPrefixes[index]);
    if (!bPrefixAccepted) return false;
    bool bExtensionAccepted = extensions.empty();
    for (std::size_t index = 0; index < extensions.size() && !bExtensionAccepted; ++index)
        bExtensionAccepted = EndsWith(entryName, extensions[index]);
    return bExtensionAccepted;
}

bool SanpackReader::Open(const std::string& sanpackPath) {
    Close();
    if (!MapFile(sanpackPath)) {
        std::cerr << "SanpackReader: cannot memory-map '" << sanpackPath << "'.\n";
        return false;
    }
    if (mappedByteSize < ZipFormat::endOfCentralDirectoryByteSize) {
        std::cerr << "SanpackReader: '" << sanpackPath << "' is too small to be an archive.\n";
        Close();
        return false;
    }
    return true;
}

void SanpackReader::Close() {
    UnmapFile();
    mappedData = nullptr;
    mappedByteSize = 0;
    directoryEntries.clear();
    statistics = SanpackIngestStatistics{};
    bDirectoryParsed = false;
}

} // namespace Io
} // namespace SanmapGen

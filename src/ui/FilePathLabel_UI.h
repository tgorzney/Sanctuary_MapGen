// FilePathLabel_UI.h — pure path-text helpers for the file picker. Layer: UI.
// Split out of FilePathPicker_UI.h under the ARCH §1.5 ceilings: the picker's own file holds the
// widget, this one holds the three string functions it (and any tab showing a stored path) needs.
// No imgui, no filesystem call — these look at characters only, so they are headless-testable and
// safe to run on a path that does not exist.
#pragma once
#include <string>

namespace SanmapGen {
namespace Ui {

// The label shown when a path setting is empty. A setting, not a literal in the draw code.
inline const char* EmptyFilePathLabel() { return "(none)"; }

// Case-insensitive ASCII compare of one character. Extensions are ASCII by definition of every
// format we read (.sanmap/.sanpack/.dds/.png/.raw), so no locale is involved.
inline bool CharactersMatchIgnoringCase(char firstCharacter, char secondCharacter) {
    if (firstCharacter >= 'A' && firstCharacter <= 'Z') firstCharacter += ('a' - 'A');
    if (secondCharacter >= 'A' && secondCharacter <= 'Z') secondCharacter += ('a' - 'A');
    return firstCharacter == secondCharacter;
}

// The last path segment. Both separators are honored because a path can arrive from a native
// Windows dialog, a .sanmap written on another machine, or a hand-typed setting.
inline std::string FileNameFromPath(const std::string& filePath) {
    const std::string::size_type lastSeparator = filePath.find_last_of("/\\");
    if (lastSeparator == std::string::npos) return filePath;
    return filePath.substr(lastSeparator + 1);
}

// True when `filePath` ends with one of the ';'-separated extensions in `allowedExtensions`
// (".png;.dds"). A null or empty list accepts anything — the fence is opt-in per control. This is
// the first half of Constitution §6: reject the wrong file BEFORE anything tries to load it.
inline bool HasAllowedFileExtension(const std::string& filePath, const char* allowedExtensions) {
    if (allowedExtensions == nullptr || *allowedExtensions == '\0') return true;
    const std::string extensionList(allowedExtensions);
    std::string::size_type entryStart = 0;
    while (entryStart <= extensionList.size()) {
        std::string::size_type entryEnd = extensionList.find(';', entryStart);
        if (entryEnd == std::string::npos) entryEnd = extensionList.size();
        const std::string::size_type entryLength = entryEnd - entryStart;
        if (entryLength > 0 && filePath.size() >= entryLength) {
            const std::string::size_type suffixStart = filePath.size() - entryLength;
            bool bSuffixMatches = true;
            for (std::string::size_type offset = 0; offset < entryLength && bSuffixMatches; ++offset)
                bSuffixMatches = CharactersMatchIgnoringCase(filePath[suffixStart + offset],
                                                             extensionList[entryStart + offset]);
            if (bSuffixMatches) return true;
        }
        entryStart = entryEnd + 1;
    }
    return false;
}

// The SHORT label beside the browse button: the TAIL of the path — the part that identifies the
// file — preceded by an ellipsis whenever the whole path does not fit the character budget. The
// label never exceeds the budget, so a row's layout cannot be blown out by a deep path. A
// non-positive budget means "no limit" rather than an empty label (Constitution §6).
inline std::string ShortenedFilePathLabel(const std::string& filePath, int maximumCharacterCount) {
    if (filePath.empty()) return std::string(EmptyFilePathLabel());
    if (maximumCharacterCount <= 0) return filePath;
    const std::string::size_type budget = static_cast<std::string::size_type>(maximumCharacterCount);
    if (filePath.size() <= budget) return filePath;
    const std::string ellipsis("...");
    if (budget <= ellipsis.size()) return filePath.substr(filePath.size() - budget);
    return ellipsis + filePath.substr(filePath.size() - (budget - ellipsis.size()));
}

} // namespace Ui
} // namespace SanmapGen

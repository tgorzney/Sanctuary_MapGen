// AssetPipeline_TestDamage_IO_Test.cpp — the two post-processing steps that make the synthetic
// sanpack an ADVERSARIAL fixture rather than a happy-path one.
//  * ReverseCentralDirectoryOrder rewrites the central directory's records back-to-front. A zip
//    does not require directory order to match file-offset order, and miniz's writer always
//    emits them in the same order — so without this step "entries were extracted in offset
//    order" would hold even for a reader that did no sorting at all, and the acceptance check
//    would prove nothing. Reversed, the two orders disagree for every entry.
//  * PatchFileByte flips one byte of a STORED payload, which is what a corrupt entry looks like
//    in the wild: a structurally valid archive whose CRC-32 no longer matches.
#include "AssetPipeline_TestSupport_IO.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace AssetPipelineTest {

namespace {

unsigned int ReadUnsigned16(const unsigned char* at) { return at[0] | (at[1] << 8); }
unsigned int ReadUnsigned32(const unsigned char* at) {
    return at[0] | (at[1] << 8) | (at[2] << 16) | (static_cast<unsigned int>(at[3]) << 24);
}

bool ReadWholeFile(const std::string& filePath, std::vector<unsigned char>& outBytes) {
    std::FILE* file = std::fopen(filePath.c_str(), "rb");
    if (file == nullptr) return false;
    unsigned char readBuffer[4096];
    for (;;) {
        const std::size_t readCount = std::fread(readBuffer, 1, sizeof(readBuffer), file);
        if (readCount == 0) break;
        outBytes.insert(outBytes.end(), readBuffer, readBuffer + readCount);
    }
    std::fclose(file);
    return !outBytes.empty();
}

bool WriteWholeFile(const std::string& filePath, const std::vector<unsigned char>& bytes) {
    std::FILE* file = std::fopen(filePath.c_str(), "wb");
    if (file == nullptr) return false;
    const bool bWritten = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    std::fclose(file);
    return bWritten;
}

} // namespace

bool ReverseCentralDirectoryOrder(const std::string& filePath) {
    std::vector<unsigned char> contents;
    if (!ReadWholeFile(filePath, contents) || contents.size() < 22) return false;
    std::size_t endOfCentralDirectory = contents.size() - 22;
    while (ReadUnsigned32(contents.data() + endOfCentralDirectory) != 0x06054b50u) {
        if (endOfCentralDirectory == 0) return false;
        --endOfCentralDirectory;
    }
    const unsigned int entryCount = ReadUnsigned16(contents.data() + endOfCentralDirectory + 10);
    const unsigned int directoryOffset = ReadUnsigned32(contents.data() + endOfCentralDirectory + 16);

    std::vector<std::vector<unsigned char>> records;
    std::size_t cursor = directoryOffset;
    for (unsigned int index = 0; index < entryCount; ++index) {
        if (cursor + 46 > contents.size()) return false;
        const unsigned char* header = contents.data() + cursor;
        if (ReadUnsigned32(header) != 0x02014b50u) return false;
        const std::size_t recordSize = 46 + ReadUnsigned16(header + 28) + ReadUnsigned16(header + 30) +
                                       ReadUnsigned16(header + 32);
        if (cursor + recordSize > contents.size()) return false;
        records.emplace_back(header, header + recordSize);
        cursor += recordSize;
    }
    // Same bytes, same total size, same offsets — only the record ORDER changes, which is legal
    // and leaves every local header exactly where it was.
    std::size_t writeCursor = directoryOffset;
    for (std::size_t index = records.size(); index-- > 0;) {
        std::memcpy(contents.data() + writeCursor, records[index].data(), records[index].size());
        writeCursor += records[index].size();
    }
    return WriteWholeFile(filePath, contents);
}

bool PatchFileByte(const std::string& filePath, const char* marker, unsigned char replacement) {
    std::vector<unsigned char> contents;
    if (!ReadWholeFile(filePath, contents)) return false;
    const std::size_t markerLength = std::strlen(marker);
    for (std::size_t offset = 0; offset + markerLength <= contents.size(); ++offset) {
        if (std::memcmp(contents.data() + offset, marker, markerLength) != 0) continue;
        contents[offset] = replacement;      // one byte flipped => CRC-32 mismatch
        return WriteWholeFile(filePath, contents);
    }
    return false;
}

} // namespace AssetPipelineTest

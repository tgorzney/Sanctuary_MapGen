// SanpackReader_MemoryMap_IO.cpp — the platform seam of SanpackReader (Constitution §5: the
// OS-specific touchpoint stays thin and behind IO). A ~2 GB sanpack is mapped read-only and
// paged in by the OS as the offset-sorted pass walks it, so peak RSS is the pages actually
// touched — never the archive (ASSET_LOADING_SPEC "never copy 2 GB into RAM"). The handles
// leave this file only as void*, so the header stays free of Windows.h and <sys/mman.h>.
#include "SanpackReader_IO.h"

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <Windows.h>
#else
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace SanmapGen {
namespace Io {

#if defined(_WIN32)

bool SanpackReader::MapFile(const std::string& sanpackPath) {
    HANDLE file = CreateFileA(sanpackPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER fileSize = {};
    if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart <= 0) { CloseHandle(file); return false; }
    HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mapping == nullptr) { CloseHandle(file); return false; }
    const void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (view == nullptr) { CloseHandle(mapping); CloseHandle(file); return false; }
    platformFileHandle = file;
    platformMappingHandle = mapping;
    mappedData = static_cast<const unsigned char*>(view);
    mappedByteSize = static_cast<std::uint64_t>(fileSize.QuadPart);
    return true;
}

void SanpackReader::UnmapFile() {
    if (mappedData != nullptr) UnmapViewOfFile(mappedData);
    if (platformMappingHandle != nullptr) CloseHandle(platformMappingHandle);
    if (platformFileHandle != nullptr) CloseHandle(platformFileHandle);
    platformMappingHandle = nullptr;
    platformFileHandle = nullptr;
}

#else

bool SanpackReader::MapFile(const std::string& sanpackPath) {
    const int fileDescriptor = ::open(sanpackPath.c_str(), O_RDONLY);
    if (fileDescriptor < 0) return false;
    struct stat fileStatus = {};
    if (::fstat(fileDescriptor, &fileStatus) != 0 || fileStatus.st_size <= 0) {
        ::close(fileDescriptor);
        return false;
    }
    void* view = ::mmap(nullptr, static_cast<size_t>(fileStatus.st_size), PROT_READ, MAP_PRIVATE,
                        fileDescriptor, 0);
    if (view == MAP_FAILED) { ::close(fileDescriptor); return false; }
    platformFileHandle = reinterpret_cast<void*>(static_cast<std::intptr_t>(fileDescriptor));
    platformMappingHandle = nullptr;
    mappedData = static_cast<const unsigned char*>(view);
    mappedByteSize = static_cast<std::uint64_t>(fileStatus.st_size);
    return true;
}

void SanpackReader::UnmapFile() {
    if (mappedData != nullptr) ::munmap(const_cast<unsigned char*>(mappedData),
                                        static_cast<size_t>(mappedByteSize));
    if (platformFileHandle != nullptr)
        ::close(static_cast<int>(reinterpret_cast<std::intptr_t>(platformFileHandle)));
    platformFileHandle = nullptr;
    platformMappingHandle = nullptr;
}

#endif

} // namespace Io
} // namespace SanmapGen

// AssetPipeline_TestSupport_IO_Test.cpp — builds the synthetic sanpack the M5-4 acceptance
// checks run against. Uses miniz's writer so the archive is a genuine zip (real local headers,
// real deflate streams, real CRC-32s); the damage is applied afterwards, by patching bytes in
// the finished file, which is how a real corrupted download presents itself.
#include "AssetPipeline_TestSupport_IO.h"
#include <cstdio>
#include <cstring>
#include <miniz.h>

namespace AssetPipelineTest {

namespace {

void AppendUnsigned32(std::vector<unsigned char>& bytes, unsigned int value) {
    for (int shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<unsigned char>(value >> shift));
}

// The 16-byte marker lets the damage step find this payload in the finished archive.
const char* CrcDamageMarker() { return "SANGEN-DAMAGE-1!"; }

std::vector<unsigned char> MakeSurfaceHeader(unsigned int width, unsigned int height,
                                             unsigned int fourCharacterCode) {
    std::vector<unsigned char> header;
    AppendUnsigned32(header, 0x20534444u);       // 'DDS '
    AppendUnsigned32(header, 124u);              // dwSize
    AppendUnsigned32(header, 0x000a1007u);       // dwFlags
    AppendUnsigned32(header, height);
    AppendUnsigned32(header, width);
    while (header.size() < 80) header.push_back(0);
    AppendUnsigned32(header, 0x4u);              // ddspf.dwFlags = FOURCC
    AppendUnsigned32(header, fourCharacterCode);
    while (header.size() < 128) header.push_back(0);
    return header;
}

} // namespace

std::vector<unsigned char> MakeFlatDxt5Surface(int width, int height, unsigned short color565) {
    std::vector<unsigned char> surface = MakeSurfaceHeader(static_cast<unsigned int>(width),
                                                           static_cast<unsigned int>(height), 0x35545844u);
    const int blockColumnCount = (width + 3) / 4;
    const int blockRowCount = (height + 3) / 4;
    for (int block = 0; block < blockColumnCount * blockRowCount; ++block) {
        surface.push_back(255); surface.push_back(255);            // alpha0 == alpha1 == opaque
        for (int index = 0; index < 6; ++index) surface.push_back(0);
        surface.push_back(static_cast<unsigned char>(color565 & 0xff));
        surface.push_back(static_cast<unsigned char>(color565 >> 8));
        surface.push_back(static_cast<unsigned char>(color565 & 0xff));
        surface.push_back(static_cast<unsigned char>(color565 >> 8));
        for (int index = 0; index < 4; ++index) surface.push_back(0);   // every texel -> colour 0
    }
    return surface;
}

std::vector<unsigned char> MakeOversizeSurfaceHeader() {
    return MakeSurfaceHeader(65535u, 65535u, 0x35545844u);
}

bool WriteSyntheticSanpack(const SyntheticSanpack& layout) {
    std::remove(layout.sanpackPath.c_str());
    mz_zip_archive archive;
    std::memset(&archive, 0, sizeof(archive));
    if (!mz_zip_writer_init_file(&archive, layout.sanpackPath.c_str(), 0)) return false;

    const std::vector<unsigned char> validSurface =
        MakeFlatDxt5Surface(layout.iconWidth, layout.iconHeight, layout.validIconColor565);
    const std::vector<unsigned char> secondSurface =
        MakeFlatDxt5Surface(layout.iconWidth, layout.iconHeight, layout.secondIconColor565);
    const std::vector<unsigned char> oversizeSurface = MakeOversizeSurfaceHeader();
    std::vector<unsigned char> damagedSurface =
        MakeFlatDxt5Surface(layout.iconWidth, layout.iconHeight, layout.validIconColor565);
    std::memcpy(damagedSurface.data() + 128, CrcDamageMarker(), 16);
    const std::string notASurface = "this entry is not a .dds at all";
    const std::string propModel = "sanmodel-vertices-and-indices-would-live-here";

    // Written deliberately out of alphabetical order: offset order != name order.
    bool bWritten = true;
    const auto add = [&](const std::string& name, const void* data, std::size_t byteSize, mz_uint level) {
        bWritten = bWritten && mz_zip_writer_add_mem(&archive, name.c_str(), data, byteSize, level);
    };
    add(layout.ignoredName, propModel.data(), propModel.size(), MZ_BEST_COMPRESSION);
    add(layout.secondIconName, secondSurface.data(), secondSurface.size(), MZ_BEST_COMPRESSION);
    add(layout.crcDamagedIconName, damagedSurface.data(), damagedSurface.size(), MZ_NO_COMPRESSION);
    add(layout.validIconName, validSurface.data(), validSurface.size(), MZ_BEST_COMPRESSION);
    add(layout.propModelName, propModel.data(), propModel.size(), MZ_BEST_COMPRESSION);
    add(layout.badHeaderIconName, notASurface.data(), notASurface.size(), MZ_BEST_COMPRESSION);
    add(layout.oversizeIconName, oversizeSurface.data(), oversizeSurface.size(), MZ_BEST_COMPRESSION);

    bWritten = bWritten && mz_zip_writer_finalize_archive(&archive);
    mz_zip_writer_end(&archive);
    // Then make the fixture adversarial: directory order reversed (so an unsorted extraction
    // pass would be visibly wrong) and one stored payload byte flipped (so its CRC fails).
    return bWritten && ReverseCentralDirectoryOrder(layout.sanpackPath) &&
           PatchFileByte(layout.sanpackPath, CrcDamageMarker(), 0x00);
}

} // namespace AssetPipelineTest

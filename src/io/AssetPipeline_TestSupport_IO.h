// AssetPipeline_TestSupport_IO.h — shared harness for the M5-4 acceptance binary. The checks
// span several translation units (ARCH §1.5) and all of them need the SAME synthetic sanpack,
// so the archive builder and the pass/fail counter live here. Test scaffolding only — no
// shipping file includes this header.
//
// The synthetic archive is written with miniz's WRITING api (a real zip, real deflate, real
// CRCs — never a hand-rolled byte blob), then two of its entries are deliberately damaged so
// the reader's validation path is exercised for real (Constitution §6).
#pragma once
#include <cstdio>
#include <string>
#include <vector>

namespace AssetPipelineTest {

inline int& FailureCount() { static int failureCount = 0; return failureCount; }

inline void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL: %s\n", label);
    ++FailureCount();
}

// Names are written in an order that does NOT match their alphabetical order, so an extraction
// pass that walked the directory as-parsed would visibly differ from the offset-sorted one.
struct SyntheticSanpack {
    std::string sanpackPath;
    std::string validIconName        = "UI/UI/Sprites/Icons/Units/ucl3001.dds";
    std::string secondIconName       = "Gameplay/StrategicIcons/air1_t1_aa.dds";
    std::string crcDamagedIconName   = "UI/UI/Sprites/Icons/Units/ucl9999.dds";
    std::string badHeaderIconName    = "UI/UI/Sprites/Icons/Units/ucl0000.dds";
    std::string oversizeIconName     = "UI/UI/Sprites/Icons/Units/ucl0001.dds";
    std::string propModelName        = "Environment/Environment/03_Desert/Props/edbm01/edbm01.sanmodel";
    std::string ignoredName          = "Audio/Audio/music.sanbank";   // filtered out by extension
    int  iconWidth = 64;
    int  iconHeight = 64;
    unsigned short validIconColor565 = 0xf800;   // pure red
    unsigned short secondIconColor565 = 0x001f;  // pure blue
};

// A real .dds: 128-byte header + DXT5 blocks, every block a single flat colour so the decoded
// surface is exactly predictable and the atlas can be pixel-compared.
std::vector<unsigned char> MakeFlatDxt5Surface(int width, int height, unsigned short color565);
// A .dds header claiming a huge surface with no payload — rejected by the dimension cap.
std::vector<unsigned char> MakeOversizeSurfaceHeader();

// Writes the archive, then applies the two adversarial post-processing steps below.
bool WriteSyntheticSanpack(const SyntheticSanpack& layout);

// Defined in AssetPipeline_TestDamage_IO_Test.cpp.
// Rewrites the central directory back-to-front (legal zip) so directory order and file-offset
// order DISAGREE — without it, a reader that never sorted would still look offset-ordered.
bool ReverseCentralDirectoryOrder(const std::string& filePath);
// Flips one byte of a stored payload: a structurally valid archive with a broken CRC-32.
bool PatchFileByte(const std::string& filePath, const char* marker, unsigned char replacement);

// Defined in the sibling test translation units.
void RunSanpackReaderChecks(const SyntheticSanpack& layout);              // SanpackReader_IO_Test.cpp
void RunAtlasCacheChecks(const SyntheticSanpack& layout,
                         const std::string& scratchDirectory);            // AssetAtlasCache_IO_Test.cpp
// STEP5_PropsDecalsValidation_UI: SanpackReader::HasEntry / ValidatePropAndDecalBlueprintPaths /
// the warn-not-block export safety net, all against this SAME synthetic sanpack.
void RunBlueprintValidationChecks(const SyntheticSanpack& layout,
                                  const std::string& scratchDirectory);   // MapExporter_BlueprintValidation_IO_Test.cpp

} // namespace AssetPipelineTest

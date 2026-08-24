// FootprintBakeFingerprint_IO_Test.cpp — acceptance test for
// work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md's §1.1/§3/§5 fingerprint half: the
// Params::FootprintBakeFingerprint <-> JSON wire mapping (never-refuse-on-absence included) and the
// cross-type staleness compare against Io::SourceFingerprint.
#include "FootprintBakeFingerprint_IO.h"
#include "AssetAtlasCache_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

Params::FootprintBakeFingerprint MakeFingerprint() {
    Params::FootprintBakeFingerprint fingerprint;
    fingerprint.sourcePath   = "Templates/Props/rock_01.santp";
    fingerprint.byteSize     = 4096ull;
    fingerprint.modifiedTime = 1700000000ull;
    fingerprint.contentHash  = 123456789ull;
    return fingerprint;
}

void RunIsValidChecks() {
    Params::FootprintBakeFingerprint fresh;
    Check(!fresh.IsValid(), "a default-constructed fingerprint reports never-baked");
    Params::FootprintBakeFingerprint baked = MakeFingerprint();
    Check(baked.IsValid(), "a fully populated fingerprint reports valid");
    Params::FootprintBakeFingerprint zeroByteSize = MakeFingerprint();
    zeroByteSize.byteSize = 0;
    Check(!zeroByteSize.IsValid(), "a zero byteSize (even with a sourcePath) is not valid");
}

void RunBuildReadRoundTripChecks() {
    const Params::FootprintBakeFingerprint original = MakeFingerprint();
    const nlohmann::ordered_json json = Io::BuildFootprintBakeFingerprintJson(original);
    Check(json["SourcePath"] == "Templates/Props/rock_01.santp"
          && json["ByteSize"] == 4096ull && json["ModifiedTime"] == 1700000000ull
          && json["ContentHash"] == 123456789ull,
          "BuildFootprintBakeFingerprintJson writes all four PascalCase members");

    nlohmann::json parent;
    parent["FootprintBakeFingerprint"] = json;
    Params::FootprintBakeFingerprint loaded;
    Io::ReadFootprintBakeFingerprintJson(parent, "FootprintBakeFingerprint", loaded);
    Check(loaded.sourcePath == original.sourcePath && loaded.byteSize == original.byteSize
          && loaded.modifiedTime == original.modifiedTime && loaded.contentHash == original.contentHash
          && loaded.IsValid(),
          "the round trip is byte-for-byte exact and reports valid");
}

// IO_MIGRATION_SPEC.md's never-refuse-on-absence posture: an older .sanmap that predates this
// ticket has no "FootprintBakeFingerprint" key at all -- the PARAMS default (never baked) survives.
void RunNeverRefuseOnAbsenceChecks() {
    nlohmann::json parentWithNoKey = nlohmann::json::object();
    Params::FootprintBakeFingerprint out;
    Io::ReadFootprintBakeFingerprintJson(parentWithNoKey, "FootprintBakeFingerprint", out);
    Check(!out.IsValid(), "a missing key leaves the PARAMS default (never baked) untouched");

    nlohmann::json parentWithWrongType;
    parentWithWrongType["FootprintBakeFingerprint"] = "not an object";
    Params::FootprintBakeFingerprint outWrongType;
    Io::ReadFootprintBakeFingerprintJson(parentWithWrongType, "FootprintBakeFingerprint", outWrongType);
    Check(!outWrongType.IsValid(), "a present-but-wrong-typed key is treated the same as absent");
}

void RunStalenessCompareChecks() {
    const Params::FootprintBakeFingerprint baked = MakeFingerprint();
    Io::SourceFingerprint current;
    current.sourcePath   = baked.sourcePath;
    current.byteSize      = baked.byteSize;
    current.modifiedTime  = baked.modifiedTime;
    current.contentHash   = baked.contentHash;
    Check(!Io::FootprintBakeFingerprintIsStale(baked, current),
          "identical fields on both sides report NOT stale");

    Io::SourceFingerprint changedHash = current;
    changedHash.contentHash = 999ull;
    Check(Io::FootprintBakeFingerprintIsStale(baked, changedHash),
          "a contentHash-only difference is detected");

    Io::SourceFingerprint changedByteSize = current;
    changedByteSize.byteSize = 8192ull;
    Check(Io::FootprintBakeFingerprintIsStale(baked, changedByteSize),
          "a byteSize-only difference is detected");

    Io::SourceFingerprint changedModifiedTime = current;
    changedModifiedTime.modifiedTime = 1800000000ull;
    Check(Io::FootprintBakeFingerprintIsStale(baked, changedModifiedTime),
          "a modifiedTime-only difference is detected");

    Io::SourceFingerprint changedPath = current;
    changedPath.sourcePath = "Templates/Props/rock_02.santp";
    Check(Io::FootprintBakeFingerprintIsStale(baked, changedPath),
          "a sourcePath-only difference is detected");
}

} // namespace

int main() {
    RunIsValidChecks();
    RunBuildReadRoundTripChecks();
    RunNeverRefuseOnAbsenceChecks();
    RunStalenessCompareChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}

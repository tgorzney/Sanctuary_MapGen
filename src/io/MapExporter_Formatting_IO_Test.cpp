// MapExporter_Formatting_IO_Test.cpp — STEP84_SanmapExportFormatting_IO, Scope A (§5 + §6 excluding
// §6.5 + §7 excluding §7.1b). Pins BuildSanmapJsonText's ratified emission contract (R1-R10,
// documented above BuildSanmapJsonText in MapExporter_Recipe_IO.cpp) with a byte-exact comparison
// against a checked-in fixture, so nlohmann's free formatting guarantees and SanGen's own R3/R8/R10
// enforcement cannot silently regress. Exceeds the ARCH §1.5 soft/hard line ceilings on purpose —
// `*_Test.cpp` files across src/io/ already run well past 150 lines (MapImporter_IO_Test.cpp is
// 1600+), so a 10-case formatting acceptance test follows established precedent rather than
// requiring a new exception (work-order §"File-size ceilings" note).
//
// (B)-only cases 11-15 (position-preserving unknown-key passthrough, §6.5) are OUT OF SCOPE for this
// binary: §6.5 is gated on a SANMAP_FORMAT_SPEC Correction that is not yet ratified.
#include "MapExporter_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace SanmapGen;

namespace {

int gFailureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL: %s\n", label);
    ++gFailureCount;
}

// The one recipe every byte-exact/format test drives — authored to exercise every §5/§7.1 property
// at least once: nesting to depth 3+ (props[].transforms[].position.x), an array of objects
// (props), empty [] (e.g. "decals") and empty {} (e.g. "areas"/"Accumulation" — both already
// produced by an otherwise-default recipe), an integral float ("fadeDistance": 128.0, fixed by
// BuildDocumentEnvelopeJson), a fractional float ("heightTransition": 0.5, same), a negative float
// ("waterDepth": -20.0), a true integer ("width": 2048), a boolean ("hasWater": true), and a string
// containing an unescaped '/' (the prop's blueprintPath).
Params::MapRecipe BuildFixtureRecipe() {
    Params::MapRecipe recipe;
    recipe.mapName    = "FormattingReferenceMap";
    recipe.mapCredits = "SanGen Test Fixture";
    recipe.geometry.mapSize = 2048;
    recipe.water.bEnabled = true;
    recipe.water.waterLevelMaximum = 78.0f;         // R8: must print as "waterLevel": 78, not 78.0
    recipe.water.deepWaterDepthMaximum = -20.0f;     // negative float, must print as "waterDepth": -20.0

    Params::PropInstanceGroup propGroup;
    propGroup.blueprintPath = "Environment/Rocks/rock01.sanmodel";
    Params::PropTransform propTransform;
    propTransform.transform.positionX = 10.0f;
    propTransform.transform.positionY = 2.5f;
    propTransform.transform.positionZ = 30.0f;
    propGroup.transforms.push_back(propTransform);
    recipe.props.push_back(propGroup);
    return recipe;
}

std::string ReadFixtureBytes(const std::string& fileName) {
    const std::string fixturePath = std::string(SANGEN_IO_TESTDATA_DIRECTORY) + "/" + fileName;
    std::ifstream inputStream(fixturePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << inputStream.rdbuf();
    return buffer.str();
}

// Test 1 support: on mismatch, report the first differing byte offset and 40 bytes of context on
// each side — a raw "not equal" on a 23 KB string is useless to debug.
void ReportFirstDifference(const std::string& produced, const std::string& expected) {
    const std::size_t shorterLength = produced.size() < expected.size() ? produced.size() : expected.size();
    std::size_t differenceIndex = shorterLength;
    for (std::size_t index = 0; index < shorterLength; ++index) {
        if (produced[index] != expected[index]) { differenceIndex = index; break; }
    }
    std::printf("  first difference at byte %zu (produced=%zu bytes, expected=%zu bytes)\n",
                differenceIndex, produced.size(), expected.size());
    const std::size_t contextStart = differenceIndex > 40 ? differenceIndex - 40 : 0;
    std::printf("  produced: ...%s...\n", produced.substr(contextStart, 80).c_str());
    std::printf("  expected: ...%s...\n", expected.substr(contextStart, 80).c_str());
}

void RunByteExactFixtureMatch() {
    const Params::MapRecipe recipe = BuildFixtureRecipe();
    const std::string producedText = Io::MapExporter::BuildSanmapJsonText(recipe);
    const std::string fixtureBytes = ReadFixtureBytes("FormattingReference.sanmap");
    const bool bMatches = producedText == fixtureBytes;
    if (!bMatches) ReportFirstDifference(producedText, fixtureBytes);
    Check(bMatches, "byte-exact fixture match (test 1)");
}

void RunDeterminism() {
    const Params::MapRecipe recipe = BuildFixtureRecipe();
    const std::string firstText  = Io::MapExporter::BuildSanmapJsonText(recipe);
    const std::string secondText = Io::MapExporter::BuildSanmapJsonText(recipe);
    Check(firstText == secondText, "R9: two calls on the same recipe are byte-identical (test 2)");

    const Params::MapRecipe rebuiltRecipe = BuildFixtureRecipe();
    const std::string thirdText = Io::MapExporter::BuildSanmapJsonText(rebuiltRecipe);
    Check(firstText == thirdText,
         "R9: a recipe rebuilt from scratch reproduces the same bytes (test 2b)");
}

void RunLineEndings(const std::string& producedText) {
    Check(producedText.find('\r') == std::string::npos, "R3: no CR byte anywhere (test 3)");
    Check(producedText.find("\n\n") == std::string::npos, "R3: no blank line (test 3)");
}

void RunNoTrailingNewline(const std::string& producedText) {
    Check(!producedText.empty() && producedText.back() == '}',
         "R4: document ends on '}' with no trailing newline (test 4)");
}

void RunNoTrailingWhitespace(const std::string& producedText) {
    std::size_t lineStart = 0;
    bool bAllLinesClean = true;
    while (lineStart <= producedText.size()) {
        const std::size_t lineEnd = producedText.find('\n', lineStart);
        const std::string line = producedText.substr(
            lineStart, (lineEnd == std::string::npos ? producedText.size() : lineEnd) - lineStart);
        if (!line.empty() && line.back() == ' ') { bAllLinesClean = false; break; }
        if (lineEnd == std::string::npos) break;
        lineStart = lineEnd + 1;
    }
    Check(bAllLinesClean, "R5: no line ends in a space (test 5)");
}

void RunIndentation(const std::string& producedText) {
    bool bAllMultipleOfFour = true;
    bool bSawTwelve = false;
    std::size_t lineStart = 0;
    while (lineStart <= producedText.size()) {
        const std::size_t lineEnd = producedText.find('\n', lineStart);
        const std::string line = producedText.substr(
            lineStart, (lineEnd == std::string::npos ? producedText.size() : lineEnd) - lineStart);
        std::size_t leadingSpaceCount = 0;
        while (leadingSpaceCount < line.size() && line[leadingSpaceCount] == ' ') ++leadingSpaceCount;
        if (leadingSpaceCount % 4 != 0) bAllMultipleOfFour = false;
        if (leadingSpaceCount == 12) bSawTwelve = true;
        if (lineEnd == std::string::npos) break;
        lineStart = lineEnd + 1;
    }
    Check(bAllMultipleOfFour, "R1: every line's leading-space count is a multiple of 4 (test 6)");
    Check(bSawTwelve, "R1: at least one line reaches depth 12 (test 6)");
}

void RunFloatSpelling(const std::string& producedText) {
    Check(producedText.find("\"fadeDistance\": 128.0") != std::string::npos,
         "R7: fadeDistance prints as 128.0, not 128 (test 7)");
}

void RunIntegerSpelling(const std::string& producedText) {
    Check(producedText.find("\"width\": 2048") != std::string::npos,
         "R8: width prints as a bare integer (test 8)");
    Check(producedText.find("\"width\": 2048.0") == std::string::npos,
         "R8: width never prints with a decimal point (test 8)");
    Check(producedText.find("\"waterLevel\": 78") != std::string::npos,
         "R8: waterLevel (from waterLevelMaximum = 78.0f) prints as a bare integer (test 8)");
    Check(producedText.find("\"waterLevel\": 78.0") == std::string::npos,
         "R8: waterLevel never prints with a decimal point (test 8)");
}

void RunInvalidUtf8() {
    Params::MapRecipe recipe = BuildFixtureRecipe();
    recipe.mapName = std::string("Bad") + static_cast<char>(0xFF) + "Name";
    bool bThrew = false;
    std::string producedText;
    try {
        producedText = Io::MapExporter::BuildSanmapJsonText(recipe);
    } catch (...) {
        bThrew = true;
    }
    Check(!bThrew, "R10: invalid UTF-8 never escapes as an exception (test 9)");
    Check(producedText.empty(), "R10: invalid UTF-8 makes BuildSanmapJsonText return empty (test 9)");
}

void RunRoundTripStability() {
    Params::MapRecipe originalRecipe = BuildFixtureRecipe();
    // STEP115_MarkerPropDecalLayerReconciliationOnImport_IO: BuildFixtureRecipe's one
    // PropInstanceGroup deliberately has no matching PropInstanceLayer (propLayers stays empty) —
    // that shape is needed by the OTHER tests in this file (the byte-exact fixture match against the
    // checked-in golden file, determinism). But it is also exactly the "real map, no PropGroups
    // section" shape Io::ReconcilePropLayers now synthesizes a layer for on import, so re-parsing
    // firstText below would gain a PropGroups entry it didn't start with — a real, correct behavior
    // change, but one unrelated to what this test actually checks (JSON formatting stability). Give
    // this LOCAL copy a matching layer so the round trip stays a pure formatting-stability check.
    Params::PropInstanceLayer propLayer;
    propLayer.name = "rock01";
    originalRecipe.propLayers.push_back(propLayer);

    const std::string firstText = Io::MapExporter::BuildSanmapJsonText(originalRecipe);

    Params::MapRecipe reimportedRecipe;
    Io::MapImportResult importResult;
    const bool bParsed = Io::MapImporter::ParseSanmapJsonText(
        firstText, reimportedRecipe, Io::MapImportOptions(), importResult, nullptr);
    Check(bParsed, "round-trip: ParseSanmapJsonText succeeds on our own export (test 10)");

    const std::string thirdText = Io::MapExporter::BuildSanmapJsonText(reimportedRecipe);
    const bool bMatches = firstText == thirdText;
    if (!bMatches) ReportFirstDifference(thirdText, firstText);
    Check(bMatches, "round-trip: Build -> Parse -> Build is byte-identical (test 10)");
}

} // namespace

int main() {
    RunByteExactFixtureMatch();
    RunDeterminism();

    const Params::MapRecipe recipe = BuildFixtureRecipe();
    const std::string producedText = Io::MapExporter::BuildSanmapJsonText(recipe);
    RunLineEndings(producedText);
    RunNoTrailingNewline(producedText);
    RunNoTrailingWhitespace(producedText);
    RunIndentation(producedText);
    RunFloatSpelling(producedText);
    RunIntegerSpelling(producedText);

    RunInvalidUtf8();
    RunRoundTripStability();

    if (gFailureCount == 0) { std::printf("All MapExporter_Formatting_IO_Test cases passed.\n"); return 0; }
    std::printf("%d MapExporter_Formatting_IO_Test case(s) FAILED.\n", gFailureCount);
    return 1;
}

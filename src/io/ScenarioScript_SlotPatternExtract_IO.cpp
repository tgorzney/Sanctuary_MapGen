// ScenarioScript_SlotPatternExtract_IO.cpp -- see the header for the full contract. Tokenization is
// delegated to the shared ScenarioScript_LuaLiteralLexer_IO primitive (STEP262); everything below
// (the array-of-tables recognition, the field scan, the caps) is private to this translation unit
// and specific to the slot-pattern grammar only.
#include "ScenarioScript_SlotPatternExtract_IO.h"
#include "ScenarioScript_LuaLiteralLexer_IO.h"

namespace SanmapGen {
namespace Io {
namespace {

using Token     = LuaLiteralToken;
using TokenKind = LuaLiteralTokenKind;

struct SegmentSpan { std::size_t start; std::size_t end; };

bool IsOtherTokenWithText(const Token& token, const char* text) {
    return token.kind == TokenKind::Other && token.text == text;
}

// Returns the index of the `}` matching the `{` at tokens[openBraceIndex], or tokens.size() if the
// table never closes.
std::size_t FindMatchingCloseBraceIndex(const std::vector<Token>& tokens, std::size_t openBraceIndex) {
    int depth = 0;
    for (std::size_t index = openBraceIndex; index < tokens.size(); ++index) {
        if (tokens[index].kind == TokenKind::LBrace) ++depth;
        else if (tokens[index].kind == TokenKind::RBrace) {
            --depth;
            if (depth == 0) return index;
        }
    }
    return tokens.size();
}

// Splits [start, end) on every top-level comma -- "top-level" meaning outside any nested `{...}` or
// `(...)`, so a sibling field like `spawns = { ARMY_01 = { x = 1, y = 2 } }` stays ONE segment
// instead of being torn apart by its own internal commas.
std::vector<SegmentSpan> SplitTopLevelCommaSegments(const std::vector<Token>& tokens, std::size_t start, std::size_t end) {
    std::vector<SegmentSpan> segments;
    std::size_t segmentStart = start;
    int depth = 0;
    for (std::size_t index = start; index < end; ++index) {
        const Token& token = tokens[index];
        if (token.kind == TokenKind::LBrace || IsOtherTokenWithText(token, "(")) ++depth;
        else if (token.kind == TokenKind::RBrace || IsOtherTokenWithText(token, ")")) --depth;
        else if (token.kind == TokenKind::Comma && depth == 0) {
            segments.push_back({ segmentStart, index });
            segmentStart = index + 1;
        }
    }
    if (segmentStart < end) segments.push_back({ segmentStart, end });
    return segments;
}

// Grades ONE positional `{ ... }` sub-table [bodyStart, bodyEnd) against Shape 2's rule: a `pattern`
// key with no clean string value, or a clean `pattern` string with no sibling clean `name` string, is
// a near-miss; no `pattern` key at all means this element simply is not a candidate for this shape
// (contributes nothing, not even a near-miss -- lets this scan pass safely over unrelated
// arrays-of-tables like COUNT_SCENARIOS elsewhere in the same file).
void ProcessElementCandidate(const std::vector<Token>& tokens, std::size_t bodyStart, std::size_t bodyEnd,
                             ScenarioSlotPatternExtractionResult& result) {
    const std::vector<SegmentSpan> segments = SplitTopLevelCommaSegments(tokens, bodyStart, bodyEnd);

    bool bFoundPatternKey = false, bPatternValid = false;
    bool bFoundNameKey = false;
    std::string nameValue, patternValue;

    for (const SegmentSpan& segment : segments) {
        if (segment.end - segment.start < 3) continue;
        if (tokens[segment.start].kind != TokenKind::Identifier || tokens[segment.start + 1].kind != TokenKind::Equals) continue;

        const std::string& key = tokens[segment.start].text;
        const bool bSingleStringValue = (segment.end - segment.start == 3) && tokens[segment.start + 2].kind == TokenKind::String;

        if (key == "name") {
            bFoundNameKey = bSingleStringValue;
            if (bSingleStringValue) nameValue = tokens[segment.start + 2].text;
        } else if (key == "pattern") {
            bFoundPatternKey = true;
            bPatternValid = bSingleStringValue;
            if (bSingleStringValue) patternValue = tokens[segment.start + 2].text;
        }
    }

    if (!bFoundPatternKey) return;   // not a candidate for this shape -- no near-miss

    if (!bPatternValid) {
        result.nearMisses.push_back({ nameValue, "'pattern' field is not a plain string literal" });
        return;
    }
    if (patternValue.size() > kMaxScenarioSlotPatternStringLength) {
        result.nearMisses.push_back({ nameValue, "'pattern' string exceeds the absurd-length guard" });
        return;
    }
    if (!bFoundNameKey) {
        result.nearMisses.push_back({ std::string(), "'pattern' field present without a sibling 'name' string field" });
        return;
    }

    if (result.entries.size() >= kMaxScenarioSlotPatternExtractionCount) {
        result.bSlotPatternCountCapExceeded = true;
        return;   // caller checks this flag and stops the whole scan
    }
    result.entries.push_back({ nameValue, patternValue });
}

} // namespace

ScenarioSlotPatternExtractionResult ExtractSlotPatternsFromScenarioScriptText(const std::string& sourceText) {
    ScenarioSlotPatternExtractionResult result;
    const std::vector<Token> tokens = Tokenize(sourceText);

    std::size_t index = 0;
    while (index < tokens.size()) {
        std::size_t identifierIndex = index;
        if (tokens[identifierIndex].kind == TokenKind::Identifier && tokens[identifierIndex].text == "local") ++identifierIndex;
        if (identifierIndex >= tokens.size() || tokens[identifierIndex].kind != TokenKind::Identifier) { ++index; continue; }
        const std::size_t equalsIndex = identifierIndex + 1;
        if (equalsIndex >= tokens.size() || tokens[equalsIndex].kind != TokenKind::Equals) { ++index; continue; }
        const std::size_t braceIndex = equalsIndex + 1;
        if (braceIndex >= tokens.size() || tokens[braceIndex].kind != TokenKind::LBrace) { ++index; continue; }

        const std::size_t outerCloseIndex = FindMatchingCloseBraceIndex(tokens, braceIndex);
        if (outerCloseIndex >= tokens.size()) break;   // unterminated outer table -- nothing sane left

        // Scan the outer body for POSITIONAL `{ ... }` sub-tables only -- a `{` immediately preceded
        // by `=` is a KEYED sub-table (e.g. `spawns = { ARMY_01 = {...} }`), skipped whole and never
        // treated as a candidate element for this shape.
        std::size_t scanIndex = braceIndex + 1;
        while (scanIndex < outerCloseIndex) {
            if (tokens[scanIndex].kind == TokenKind::LBrace) {
                const bool bKeyedSubTable = scanIndex > braceIndex + 1 && tokens[scanIndex - 1].kind == TokenKind::Equals;
                const std::size_t elementCloseIndex = FindMatchingCloseBraceIndex(tokens, scanIndex);
                if (elementCloseIndex >= tokens.size()) { scanIndex = outerCloseIndex; break; }   // unterminated -- stop this array
                if (!bKeyedSubTable) {
                    ProcessElementCandidate(tokens, scanIndex + 1, elementCloseIndex, result);
                    if (result.bSlotPatternCountCapExceeded) return result;   // Constitution §6 cap
                }
                scanIndex = elementCloseIndex + 1;
            } else {
                ++scanIndex;
            }
        }

        index = outerCloseIndex + 1;
    }
    return result;
}

} // namespace Io
} // namespace SanmapGen

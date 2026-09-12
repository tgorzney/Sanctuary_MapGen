// ScenarioScript_LuaLiteralLexer_IO.cpp -- see the header for the full contract.
#include "ScenarioScript_LuaLiteralLexer_IO.h"
#include <cctype>
#include <cstdlib>

namespace SanmapGen {
namespace Io {

bool IsIdentifierStartChar(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; }
bool IsIdentifierChar(char c)      { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; }
bool IsDigitChar(char c)           { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

std::size_t SkipWhitespaceAndComments(const std::string& text, std::size_t cursor) {
    for (;;) {
        while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) != 0) ++cursor;
        if (cursor + 1 < text.size() && text[cursor] == '-' && text[cursor + 1] == '-') {
            cursor += 2;
            if (cursor + 1 < text.size() && text[cursor] == '[' && text[cursor + 1] == '[') {
                const std::size_t closeIndex = text.find("]]", cursor + 2);
                cursor = (closeIndex == std::string::npos) ? text.size() : closeIndex + 2;
            } else {
                const std::size_t newlineIndex = text.find('\n', cursor);
                cursor = (newlineIndex == std::string::npos) ? text.size() : newlineIndex + 1;
            }
            continue;
        }
        break;
    }
    return cursor;
}

std::size_t SkipQuotedString(const std::string& text, std::size_t cursor) {
    const char quoteChar = text[cursor];
    ++cursor;
    while (cursor < text.size() && text[cursor] != quoteChar) {
        if (text[cursor] == '\\' && cursor + 1 < text.size()) cursor += 2;
        else ++cursor;
    }
    if (cursor < text.size()) ++cursor;   // consume the closing quote
    return cursor;
}

namespace {

// Content between the quotes of a string starting at text[start], ending at `end` (SkipQuotedString's
// return value). Only a backslash-escaped quote is unescaped (`\"`/`\'` -> the bare quote); every
// other backslash sequence passes through verbatim -- no general Lua escape processing (item 3).
std::string ExtractQuotedStringContent(const std::string& text, std::size_t start, std::size_t end) {
    const char quoteChar = text[start];
    const bool bClosed = end > start && text[end - 1] == quoteChar && end - 1 > start;
    const std::size_t contentEnd = bClosed ? end - 1 : end;
    std::string content;
    std::size_t cursor = start + 1;
    while (cursor < contentEnd) {
        if (text[cursor] == '\\' && cursor + 1 < contentEnd && text[cursor + 1] == quoteChar) { content.push_back(quoteChar); cursor += 2; }
        else { content.push_back(text[cursor]); ++cursor; }
    }
    return content;
}

LuaLiteralToken ReadIdentifierToken(const std::string& text, std::size_t& cursor) {
    const std::size_t start = cursor;
    while (cursor < text.size() && IsIdentifierChar(text[cursor])) ++cursor;
    return { LuaLiteralTokenKind::Identifier, text.substr(start, cursor - start) };
}

LuaLiteralToken ReadNumberToken(const std::string& text, std::size_t& cursor) {
    const std::size_t start = cursor;
    while (cursor < text.size() && IsDigitChar(text[cursor])) ++cursor;
    if (cursor < text.size() && text[cursor] == '.' && cursor + 1 < text.size() && IsDigitChar(text[cursor + 1])) {
        ++cursor;
        while (cursor < text.size() && IsDigitChar(text[cursor])) ++cursor;
    }
    return { LuaLiteralTokenKind::Number, text.substr(start, cursor - start) };
}

// One/two-char operator or punctuation token at text[cursor]; advances cursor past what it consumed.
LuaLiteralToken ReadOperatorOrPunctuationToken(const std::string& text, std::size_t& cursor) {
    const char c = text[cursor];
    const char next = (cursor + 1 < text.size()) ? text[cursor + 1] : '\0';
    LuaLiteralToken token;
    if      (c == '=' && next == '=') token = { LuaLiteralTokenKind::EqualsEquals, "==" };
    else if (c == '~' && next == '=') token = { LuaLiteralTokenKind::TildeEquals, "~=" };
    else if (c == '>' && next == '=') token = { LuaLiteralTokenKind::GreaterEquals, ">=" };
    else if (c == '<' && next == '=') token = { LuaLiteralTokenKind::LessEquals, "<=" };
    else switch (c) {
        case '=': token = { LuaLiteralTokenKind::Equals, "=" }; break;
        case '>': token = { LuaLiteralTokenKind::GreaterThan, ">" }; break;
        case '<': token = { LuaLiteralTokenKind::LessThan, "<" }; break;
        case '{': token = { LuaLiteralTokenKind::LBrace, "{" }; break;
        case '}': token = { LuaLiteralTokenKind::RBrace, "}" }; break;
        case ',': token = { LuaLiteralTokenKind::Comma,  "," }; break;
        case '-': token = { LuaLiteralTokenKind::Minus,  "-" }; break;
        default:  token = { LuaLiteralTokenKind::Other,  std::string(1, c) }; break;
    }
    cursor += (token.text.size() == 2) ? 2 : 1;
    return token;
}

} // namespace

std::vector<LuaLiteralToken> Tokenize(const std::string& text) {
    std::vector<LuaLiteralToken> tokens;
    std::size_t cursor = 0;
    for (;;) {
        cursor = SkipWhitespaceAndComments(text, cursor);
        if (cursor >= text.size()) break;
        const char c = text[cursor];

        if (c == '"' || c == '\'') {
            const std::size_t start = cursor;
            cursor = SkipQuotedString(text, cursor);
            tokens.push_back({ LuaLiteralTokenKind::String, ExtractQuotedStringContent(text, start, cursor) });
        } else if (IsIdentifierStartChar(c)) {
            tokens.push_back(ReadIdentifierToken(text, cursor));
        } else if (IsDigitChar(c)) {
            tokens.push_back(ReadNumberToken(text, cursor));
        } else {
            tokens.push_back(ReadOperatorOrPunctuationToken(text, cursor));
        }
    }
    return tokens;
}

bool TryReadSignedNumberSpanningRange(const std::vector<LuaLiteralToken>& tokens, std::size_t index,
                                      std::size_t end, float& outValue) {
    if (index >= end) return false;
    bool bNegative = false;
    if (tokens[index].kind == LuaLiteralTokenKind::Minus) { bNegative = true; ++index; }
    if (index >= end || tokens[index].kind != LuaLiteralTokenKind::Number) return false;
    const float magnitude = std::strtof(tokens[index].text.c_str(), nullptr);
    ++index;
    if (index != end) return false;   // extra trailing token in this segment -- reject
    outValue = bNegative ? -magnitude : magnitude;
    return true;
}

} // namespace Io
} // namespace SanmapGen

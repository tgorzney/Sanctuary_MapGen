// ScenarioScript_LuaLiteralLexer_IO.h -- the shared, pure, disk-free tokenizer for the closed
// literal-only grammars used to extract data from FOREIGN scenario .lua text (promoted out of
// ScenarioScript_AreaRectangleExtract_IO.cpp -- STEP262 -- once a second/third caller
// (STEP263/STEP264) needed the same comment/string-skipping and numeric/identifier tokenization).
//
// Same "NO Lua execution of any kind" contract as the file this was promoted from
// (`ARCH_15_11_ForeignScenarioAreaImport.md` item 3): not `LuaTableEvaluate_SYS`, not a variant of
// it, ever. This is a hand-rolled tokenizer over a closed literal-only surface -- NOT a general Lua
// parser, and nothing downstream may treat it as one. It takes text, returns tokens, and performs no
// disk access and no execution of any kind.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

enum class LuaLiteralTokenKind {
    Identifier, Number, String, Equals, LBrace, RBrace, Comma, Minus, Other,
    EqualsEquals, TildeEquals, GreaterThan, GreaterEquals, LessThan, LessEquals,
};

struct LuaLiteralToken {
    LuaLiteralTokenKind kind;
    std::string         text;   // Identifier/Number: verbatim source text. String: the UNESCAPED
                                 // content between the quotes (quotes themselves not included).
};

bool IsIdentifierStartChar(char c);
bool IsIdentifierChar(char c);
bool IsDigitChar(char c);

// Skips whitespace, `--` line comments, and `--[[ ]]` long comments. An unterminated long comment
// consumes to end-of-text -- never an infinite loop, never a crash.
std::size_t SkipWhitespaceAndComments(const std::string& text, std::size_t cursor);

// Skips a single/double-quoted string literal starting at text[cursor] (which must be a quote
// character), returning the index just past the closing quote. Handles a backslash-escaped quote.
std::size_t SkipQuotedString(const std::string& text, std::size_t cursor);

// Tokenizes the full text against the closed literal-only surface: identifiers/keywords, decimal
// numbers, quoted strings (emitted as a `String` token carrying the unescaped content), `=`/`==`/
// `~=`/`>`/`>=`/`<`/`<=`, `{`/`}`/`,`/`-`, and a catch-all `Other` for anything else -- comments and
// whitespace are never emitted as tokens.
std::vector<LuaLiteralToken> Tokenize(const std::string& text);

// An optional leading Minus token followed by exactly one Number token, consuming the WHOLE
// [index, end) range -- a trailing stray token (e.g. a second number, an identifier) fails this.
bool TryReadSignedNumberSpanningRange(const std::vector<LuaLiteralToken>& tokens, std::size_t index,
                                      std::size_t end, float& outValue);

} // namespace Io
} // namespace SanmapGen

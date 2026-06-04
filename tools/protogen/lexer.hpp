#pragma once

/// @file
/// @brief protogen tokenizer for the proto3 subset. Comments (// and block) are
///        stripped.

#include <string>
#include <vector>

namespace protogen {

enum class TokKind {
    Identifier,  // [A-Za-z_][A-Za-z0-9_.]*  (dots kept so "google.protobuf.X" is one token)
    Number,      // integer / float literal, optional leading '-'
    String,      // contents of a "..." literal (unescaped minimally)
    Symbol,      // single punctuation char: { } [ ] ( ) = ; , < > .
    End,
};

struct Token {
    TokKind kind = TokKind::End;
    std::string text;
    int line = 0;
};

/// Tokenize proto source. Throws std::runtime_error on a lexical error.
std::vector<Token> lex(const std::string& src, const std::string& filename);

}  // namespace protogen

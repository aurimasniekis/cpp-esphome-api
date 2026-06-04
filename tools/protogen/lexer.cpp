#include "lexer.hpp"

#include <cctype>
#include <stdexcept>

namespace protogen {

namespace {

bool ident_start(const char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}
bool ident_cont(const char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.';
}

[[noreturn]] void lex_error(const std::string& filename, const int line, const std::string& msg) {
    throw std::runtime_error(filename + ":" + std::to_string(line) + ": lexer: " + msg);
}

}  // namespace

std::vector<Token> lex(const std::string& src, const std::string& filename) {
    std::vector<Token> out;
    std::size_t i = 0;
    const std::size_t n = src.size();
    int line = 1;

    while (i < n) {
        const char c = src[i];

        if (c == '\n') {
            ++line;
            ++i;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            ++i;
            continue;
        }

        // Comments.
        if (c == '/' && i + 1 < n && src[i + 1] == '/') {
            i += 2;
            while (i < n && src[i] != '\n')
                ++i;
            continue;
        }
        if (c == '/' && i + 1 < n && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n')
                    ++line;
                ++i;
            }
            if (i + 1 >= n)
                lex_error(filename, line, "unterminated block comment");
            i += 2;
            continue;
        }

        // String literal.
        if (c == '"' || c == '\'') {
            const char quote = c;
            const int start_line = line;
            ++i;
            std::string value;
            while (i < n && src[i] != quote) {
                if (src[i] == '\\' && i + 1 < n) {
                    // Keep escapes simple — proto option strings here are plain.
                    switch (const char esc = src[i + 1]) {
                    case 'n': value.push_back('\n'); break;
                    case 't': value.push_back('\t'); break;
                    case 'r': value.push_back('\r'); break;
                    default: value.push_back(esc); break;
                    }
                    i += 2;
                    continue;
                }
                if (src[i] == '\n')
                    ++line;
                value.push_back(src[i]);
                ++i;
            }
            if (i >= n)
                lex_error(filename, start_line, "unterminated string literal");
            ++i;  // closing quote
            out.push_back(Token{TokKind::String, value, start_line});
            continue;
        }

        // Identifier / keyword.
        if (ident_start(c)) {
            const std::size_t start = i;
            while (i < n && ident_cont(src[i]))
                ++i;
            out.push_back(Token{TokKind::Identifier, src.substr(start, i - start), line});
            continue;
        }

        // Number (optionally signed; floats accepted).
        if (std::isdigit(static_cast<unsigned char>(c)) != 0 ||
            (c == '-' && i + 1 < n && std::isdigit(static_cast<unsigned char>(src[i + 1])) != 0)) {
            const std::size_t start = i;
            if (src[i] == '-')
                ++i;
            while (i < n && (std::isalnum(static_cast<unsigned char>(src[i])) != 0 ||
                             src[i] == '.' || src[i] == '+' || src[i] == '-')) {
                // Stop a trailing sign that is not part of an exponent.
                if ((src[i] == '+' || src[i] == '-') &&
                    !(i > start && (src[i - 1] == 'e' || src[i - 1] == 'E')))
                    break;
                ++i;
            }
            out.push_back(Token{TokKind::Number, src.substr(start, i - start), line});
            continue;
        }

        // Single-character symbol.
        const std::string sym(1, c);
        switch (c) {
        case '{':
        case '}':
        case '[':
        case ']':
        case '(':
        case ')':
        case '=':
        case ';':
        case ',':
        case '<':
        case '>':
        case '.':
            out.push_back(Token{TokKind::Symbol, sym, line});
            ++i;
            continue;
        default:
            lex_error(filename, line, std::string("unexpected character '") + c + "'");
        }
    }

    out.push_back(Token{TokKind::End, "", line});
    return out;
}

}  // namespace protogen

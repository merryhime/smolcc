// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

using FileId = std::size_t;
struct Location {
    Location() = default;
    Location(FileId file)
            : file(file) {}

    FileId file;
    std::size_t line = 1;
    std::size_t col = 1;
    std::size_t index = 0;
    std::size_t length = 0;
};

class CharStream {
public:
    CharStream(FileId file, std::string contents)
            : contents(contents), current_loc(file), next_loc(file) {}

    std::optional<char> peek() const {
        if (next_loc.index >= contents.size())
            return std::nullopt;
        return contents[next_loc.index];
    }

    std::optional<char> get() {
        if (next_loc.index >= contents.size())
            return std::nullopt;

        const char ch = contents[next_loc.index];
        current_loc.length++;
        next_loc.index++;
        next_loc.col++;
        if (ch == '\n') {
            next_loc.line++;
            next_loc.col = 1;
        }
        return ch;
    }

    bool consume_if(char ch) {
        if (peek() == ch) {
            get();
            return true;
        }
        return false;
    }

    Location loc() const { return current_loc; }
    void new_loc() {
        current_loc = next_loc;
    }

private:
    std::string contents;

    Location current_loc;
    Location next_loc;
};

enum class TokenKind {
    EndOfFile,
    IntegerConstant,
    Punctuator,
    Identifier,
};

enum class PunctuatorKind {
    LBracket,  // [
    RBracket,  // ]
    LParen,    // (
    RParen,    // )
    LBrace,    // {
    RBrace,    // }
    Dot,       // .
    Arrow,     // ->

    PlusPlus,    // ++
    MinusMinus,  // --
    And,         // &
    Star,        // *
    Plus,        // +
    Minus,       // -
    Tilde,       // ~
    Not,         // !

    Slash,     // /
    Modulo,    // %
    LLAngle,   // <<
    RRAngle,   // >>
    LAngle,    // <
    RAngle,    // >
    LAngleEq,  // <=
    RAngleEq,  // >=
    EqEq,      // ==
    NotEq,     // !=
    Caret,     // ^
    Or,        // |
    AndAnd,    // &&
    OrOr,      // ||

    Query,      // ?
    Colon,      // :
    Semi,       // ;
    DotDotDot,  // ...

    Eq,         // =
    StarEq,     // *=
    SlashEq,    // /=
    ModuloEq,   // %=
    PlusEq,     // +=
    MinusEq,    // -=
    LLAngleEq,  // <<=
    RRAngleEq,  // >>=
    AndEq,      // &=
    CaretEq,    // ^=
    OrEq,       // |=

    Comma,     // ,
    Hash,      // #
    HashHash,  // ##
};

struct Token {
    TokenKind kind;
    Location loc;

    uintmax_t value;
    PunctuatorKind punctuator;
    std::string payload;

    static Token IntegerConstant(Location loc, uintmax_t value) {
        Token result;
        result.kind = TokenKind::IntegerConstant;
        result.loc = loc;
        result.value = value;
        return result;
    }

    static Token Punctuator(Location loc, PunctuatorKind punctuator) {
        Token result;
        result.kind = TokenKind::Punctuator;
        result.loc = loc;
        result.punctuator = punctuator;
        return result;
    }

    static Token Identifier(Location loc, std::string payload) {
        Token result;
        result.kind = TokenKind::Identifier;
        result.loc = loc;
        result.payload = payload;
        return result;
    }
};

class TokenStream {
public:
    TokenStream(CharStream inner)
            : inner(std::move(inner)) {}

    Token peek() {
        if (current)
            return *current;
        return *(current = tok());
    }

    bool peek(PunctuatorKind punctuator) {
        return peek().kind == TokenKind::Punctuator && peek().punctuator == punctuator;
    }

    bool peek_identifier(std::string_view identifier) {
        return peek().kind == TokenKind::Identifier && peek().payload == identifier;
    }

    bool consume_if(PunctuatorKind punctuator) {
        if (peek(punctuator)) {
            next();
            return true;
        }
        return false;
    }

    bool consume_if_identifier(std::string_view identifier) {
        if (peek_identifier(identifier)) {
            next();
            return true;
        }
        return false;
    }

    Token next() {
        if (!current)
            return tok();
        return *std::exchange(current, std::nullopt);
    }

    Location loc() const {
        return last_loc;
    }

private:
    Token tok();

    std::optional<Token> current;
    Location last_loc;

    CharStream inner;
};

// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>

#include <fmt/core.h>

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

    Location loc() const { return current_loc; }
    void new_loc() {
        current_loc = next_loc;
    }

private:
    std::string contents;

    Location current_loc;
    Location next_loc;
};

bool isspace(std::optional<char> ch) {
    return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\r' || ch == '\n';
}

bool isdecimaldigit(std::optional<char> ch) {
    return ch && (*ch >= '0' && *ch <= '9');
}

enum class TokenTag {
    EndOfFile,
    IntegerConstant,
    Punctuator,
};

enum class PunctuatorType {
    Plus,   // +
    Minus,  // -
};

struct Token {
    TokenTag tag;
    Location loc;

    uintmax_t value;
    PunctuatorType punctuator;

    static Token IntegerConstant(Location loc, uintmax_t value) {
        Token result;
        result.tag = TokenTag::IntegerConstant;
        result.loc = loc;
        result.value = value;
        return result;
    }

    static Token Punctuator(Location loc, PunctuatorType punctuator) {
        Token result;
        result.tag = TokenTag::Punctuator;
        result.loc = loc;
        result.punctuator = punctuator;
        return result;
    }
};

class TokenStream {
public:
    TokenStream(CharStream inner)
            : inner(std::move(inner)) {}

    Token tok() {
        while (inner.peek()) {
            while (isspace(inner.peek())) {
                inner.get();
            }

            if (!inner.peek()) {
                break;
            }

            if (isdecimaldigit(inner.peek())) {
                inner.new_loc();

                std::string value;
                while (isdecimaldigit(inner.peek())) {
                    value += *inner.get();
                }
                return Token::IntegerConstant(inner.loc(), static_cast<uintmax_t>(std::atoll(value.c_str())));
            }

            inner.new_loc();
            switch (*inner.peek()) {
            case '+':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorType::Plus);
            case '-':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorType::Minus);
            }

            std::terminate();  // invalid character
        }

        return Token{
            TokenTag::EndOfFile,
            inner.loc(),
        };
    }

private:
    CharStream inner;
};

#define ASSERT(x) [&] { if (!(x)) fmt::print("failed assert {}\n", #x); }()

int main() {
    CharStream cs{0, "42 + 43 - 44"};
    TokenStream ts{cs};

    Token t;

    t = ts.tok();
    ASSERT(t.tag == TokenTag::IntegerConstant && t.value == 42);
    t = ts.tok();
    ASSERT(t.tag == TokenTag::Punctuator && t.punctuator == PunctuatorType::Plus);
    t = ts.tok();
    ASSERT(t.tag == TokenTag::IntegerConstant && t.value == 43);
    t = ts.tok();
    ASSERT(t.tag == TokenTag::Punctuator && t.punctuator == PunctuatorType::Minus);
    t = ts.tok();
    ASSERT(t.tag == TokenTag::IntegerConstant && t.value == 44);

    return 1;
}

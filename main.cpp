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

struct Token {
    TokenTag tag;
    Location loc;

    uintmax_t value;
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
                return Token{
                    TokenTag::IntegerConstant,
                    inner.loc(),
                    static_cast<uintmax_t>(std::atoll(value.c_str())),
                };
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

int main() {
    CharStream cs{0, "42 43 44"};
    TokenStream ts{cs};

    for (size_t i = 0; i < 3; i++) {
        fmt::print("{}\n", ts.tok().value);
    }
}

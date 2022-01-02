// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

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

int main() {
}

// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <fmt/core.h>

#define ASSERT(x) [&] { if (!(x)) { fmt::print("failed assert {}\n", #x); std::terminate(); } }()

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

enum class TokenKind {
    EndOfFile,
    IntegerConstant,
    Punctuator,
};

enum class PunctuatorKind {
    Plus,    // +
    Minus,   // -
    Star,    // *
    Slash,   // /
    Modulo,  // %
    LParen,  // (
    RParen,  // )
};

struct Token {
    TokenKind kind;
    Location loc;

    uintmax_t value;
    PunctuatorKind punctuator;

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

    bool consume_if(PunctuatorKind punctuator) {
        if (peek().kind == TokenKind::Punctuator && peek().punctuator == punctuator) {
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

private:
    Token tok() {
        while (inner.peek()) {
            while (isspace(inner.peek())) {
                inner.get();
            }

            if (!inner.peek()) {
                break;
            }

            inner.new_loc();
            switch (*inner.peek()) {
            case '0' ... '9': {
                std::string value;
                do {
                    value += *inner.get();
                } while (isdecimaldigit(inner.peek()));
                return Token::IntegerConstant(inner.loc(), static_cast<uintmax_t>(std::atoll(value.c_str())));
            }
            case '+':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Plus);
            case '-':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Minus);
            case '*':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Star);
            case '/':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Slash);
            case '%':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Modulo);
            case '(':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::LParen);
            case ')':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::RParen);
            }

            std::terminate();  // invalid character
        }

        return Token{
            TokenKind::EndOfFile,
            inner.loc(),
        };
    }

    std::optional<Token> current;

    CharStream inner;
};

struct Expr {
    virtual ~Expr() {}
};

using ExprPtr = std::unique_ptr<Expr>;

struct IntegerConstantExpr : public Expr {
    explicit IntegerConstantExpr(uintmax_t value)
            : value(value) {}
    uintmax_t value;
};

enum class BinOpKind {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
};

struct BinOpExpr : public Expr {
    BinOpExpr(BinOpKind op, ExprPtr lhs, ExprPtr rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    BinOpKind op;
    ExprPtr lhs;
    ExprPtr rhs;
};

class Parser {
public:
    Parser(TokenStream inner)
            : inner(std::move(inner)) {}

    ExprPtr primary_expression() {
        if (inner.consume_if(PunctuatorKind::LParen)) {
            ExprPtr result = expression();
            ASSERT(inner.consume_if(PunctuatorKind::RParen));
            return result;
        }

        const Token tok = inner.next();
        ASSERT(tok.kind == TokenKind::IntegerConstant);
        return std::make_unique<IntegerConstantExpr>(tok.value);
    }

    ExprPtr multiplicative_expression() {
        ExprPtr e = primary_expression();
        while (true) {
            if (inner.consume_if(PunctuatorKind::Star)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Multiply, std::move(e), primary_expression());
            } else if (inner.consume_if(PunctuatorKind::Slash)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Divide, std::move(e), primary_expression());
            } else if (inner.consume_if(PunctuatorKind::Modulo)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Modulo, std::move(e), primary_expression());
            } else {
                return e;
            }
        }
    }

    ExprPtr additive_expression() {
        ExprPtr e = multiplicative_expression();
        while (true) {
            if (inner.consume_if(PunctuatorKind::Plus)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Add, std::move(e), multiplicative_expression());
            } else if (inner.consume_if(PunctuatorKind::Minus)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Subtract, std::move(e), multiplicative_expression());
            } else {
                return e;
            }
        }
    }

    ExprPtr expression() {
        return additive_expression();
    }

private:
    TokenStream inner;
};

template<typename T>
T* dyn(const ExprPtr& e) {
    return dynamic_cast<T*>(e.get());
}

void emit_expr(const ExprPtr& expr) {
    if (auto e = dyn<IntegerConstantExpr>(expr)) {
        fmt::print("movz x0, {}\n", e->value & 0xFFFF);
        if ((e->value >> 16) & 0xFFFF)
            fmt::print("movk x0, {}, lsl 16\n", (e->value >> 16) & 0xFFFF);
        if ((e->value >> 32) & 0xFFFF)
            fmt::print("movk x0, {}, lsl 32\n", (e->value >> 32) & 0xFFFF);
        if ((e->value >> 48) & 0xFFFF)
            fmt::print("movk x0, {}, lsl 48\n", (e->value >> 48) & 0xFFFF);
        return;
    }

    if (auto e = dyn<BinOpExpr>(expr)) {
        emit_expr(e->lhs);
        fmt::print("str x0, [sp, -16]!\n");
        emit_expr(e->rhs);
        fmt::print("ldr x1, [sp], 16\n");

        switch (e->op) {
        case BinOpKind::Add:
            fmt::print("add x0, x1, x0\n");
            return;
        case BinOpKind::Subtract:
            fmt::print("sub x0, x1, x0\n");
            return;
        case BinOpKind::Multiply:
            fmt::print("mul x0, x1, x0\n");
            return;
        case BinOpKind::Divide:
            fmt::print("udiv x0, x1, x0\n");  // unsigned divide for now
            return;
        case BinOpKind::Modulo:
            fmt::print("udiv x2, x1, x0\n");  // unsigned for now
            fmt::print("msub x0, x2, x0, x1\n");
            return;
        default:
            ASSERT(!"Unknown binop kind");
        }
    }

    ASSERT(!"Unknown expr kind");
}

int main(int argc, char* argv[]) {
    ASSERT(argc == 2);

    Parser p{TokenStream{CharStream{0, argv[1]}}};

    fmt::print(".text\n");
    fmt::print(".globl _main\n");
    fmt::print(".align 4\n");
    fmt::print("_main:\n");

    ExprPtr e = p.expression();
    emit_expr(std::move(e));

    fmt::print("ret\n");

    return 0;
}

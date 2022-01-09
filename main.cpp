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
            case '[':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::LBracket);
            case ']':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::RBracket);
            case '(':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::LParen);
            case ')':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::RParen);
            case '{':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::LBrace);
            case '}':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::RBrace);
            case '.':
                inner.get();
                if (inner.consume_if('.')) {
                    ASSERT(inner.consume_if('.'));
                    return Token::Punctuator(inner.loc(), PunctuatorKind::DotDotDot);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Dot);
            case '&':
                inner.get();
                if (inner.consume_if('&')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::AndAnd);
                }
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::AndEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::And);
            case '|':
                inner.get();
                if (inner.consume_if('|')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::OrOr);
                }
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::OrEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Or);
            case '^':
                inner.get();
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::CaretEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Caret);
            case '~':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Tilde);
            case '!':
                inner.get();
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::NotEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Not);
            case '+':
                inner.get();
                if (inner.consume_if('+')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::PlusPlus);
                }
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::PlusEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Plus);
            case '-':
                inner.get();
                if (inner.consume_if('-')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::MinusMinus);
                }
                if (inner.consume_if('>')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::Arrow);
                }
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::MinusEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Minus);
            case '*':
                inner.get();
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::StarEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Star);
            case '/':
                inner.get();
                if (inner.consume_if('/')) {
                    ASSERT(!"Handle comment");
                }
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::SlashEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Slash);
            case '%':
                inner.get();
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::ModuloEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Modulo);
            case '<':
                inner.get();
                if (inner.consume_if('<')) {
                    if (inner.consume_if('=')) {
                        return Token::Punctuator(inner.loc(), PunctuatorKind::LLAngleEq);
                    }
                    return Token::Punctuator(inner.loc(), PunctuatorKind::LLAngle);
                }
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::LAngleEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::LAngle);
            case '>':
                inner.get();
                if (inner.consume_if('>')) {
                    if (inner.consume_if('=')) {
                        return Token::Punctuator(inner.loc(), PunctuatorKind::RRAngleEq);
                    }
                    return Token::Punctuator(inner.loc(), PunctuatorKind::RRAngle);
                }
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::RAngleEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::RAngle);
            case '?':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Query);
            case ':':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Colon);
            case ';':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Semi);
            case '=':
                inner.get();
                if (inner.consume_if('=')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::EqEq);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Eq);
            case ',':
                inner.get();
                return Token::Punctuator(inner.loc(), PunctuatorKind::Comma);
            case '#':
                inner.get();
                if (inner.consume_if('#')) {
                    return Token::Punctuator(inner.loc(), PunctuatorKind::HashHash);
                }
                return Token::Punctuator(inner.loc(), PunctuatorKind::Hash);
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

enum class UnOpKind {
    Posate,
    Negate,
};

struct UnOpExpr : public Expr {
    UnOpExpr(UnOpKind op, ExprPtr e)
            : op(op), e(std::move(e)) {}

    UnOpKind op;
    ExprPtr e;
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

    ExprPtr unary_expression() {
        if (inner.consume_if(PunctuatorKind::Plus)) {
            return std::make_unique<UnOpExpr>(UnOpKind::Posate, unary_expression());
        }
        if (inner.consume_if(PunctuatorKind::Minus)) {
            return std::make_unique<UnOpExpr>(UnOpKind::Negate, unary_expression());
        }
        return primary_expression();
    }

    ExprPtr multiplicative_expression() {
        ExprPtr e = unary_expression();
        while (true) {
            if (inner.consume_if(PunctuatorKind::Star)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Multiply, std::move(e), unary_expression());
            } else if (inner.consume_if(PunctuatorKind::Slash)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Divide, std::move(e), unary_expression());
            } else if (inner.consume_if(PunctuatorKind::Modulo)) {
                e = std::make_unique<BinOpExpr>(BinOpKind::Modulo, std::move(e), unary_expression());
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

    if (auto e = dyn<UnOpExpr>(expr)) {
        emit_expr(e->e);

        switch (e->op) {
        case UnOpKind::Posate:
            // do nothing
            return;
        case UnOpKind::Negate:
            fmt::print("neg x0, x0\n");
            return;
        default:
            ASSERT(!"Unknown unop kind");
        }
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

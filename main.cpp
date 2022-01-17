// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <fmt/core.h>

#include "assert.h"
#include "lexer.h"
#include "parser.h"

static int iota() {
    static int value = 1;
    return value++;
}

struct Function {
    StmtVal body;
    std::map<std::string, int> locals;
    int stack_size;
};

Function f{};

template<typename T>
void emit_loc(const T& x) {
    fmt::print(".loc {} {} {}\n", x->loc.file, x->loc.line, x->loc.col);
}

void emit_addr(const ExprVal& expr);
void emit_expr(const ExprVal& expr);
void emit_stmt(const StmtVal& stmt);

void emit_constant(std::string_view reg, std::uint64_t value) {
    fmt::print("movz {}, {}\n", reg, value & 0xFFFF);
    if ((value >> 16) & 0xFFFF)
        fmt::print("movk {}, {}, lsl 16\n", reg, (value >> 16) & 0xFFFF);
    if ((value >> 32) & 0xFFFF)
        fmt::print("movk {}, {}, lsl 32\n", reg, (value >> 32) & 0xFFFF);
    if ((value >> 48) & 0xFFFF)
        fmt::print("movk {}, {}, lsl 48\n", reg, (value >> 48) & 0xFFFF);
}

void emit_addr(const ExprVal& expr) {
    if (auto e = expr.cast<VariableExpr>()) {
        fmt::print("add x0, fp, {}\n", f.locals[e->ident]);
        return;
    }

    if (auto e = expr.cast<UnOpExpr>()) {
        switch (e->op) {
        case UnOpKind::Dereference:
            emit_expr(e->e);
            return;
        default:
            ASSERT(!"Unknown unop kind");
        }
    }

    ASSERT(!"!lvalue");
}

void emit_addsub(BinOpExpr* e) {
    const bool is_add = e->op == BinOpKind::Add;
    const TypeVal lt = e->lhs->type();
    const TypeVal rt = e->rhs->type();
    const bool lp = lt->is_pointer();
    const bool rp = rt->is_pointer();

    if (lp && rp) {
        ASSERT(!is_add && "pointer + pointer is invalid");
        emit_constant("x2", lt.cast<PointerType>()->base->size());
        fmt::print("sub x0, x1, x0\n");
        fmt::print("udiv x0, x0, x2\n");
        return;
    } else if (lp && !rp) {
        emit_constant("x2", lt.cast<PointerType>()->base->size());
        fmt::print("{} x0, x0, x2, x1\n", is_add ? "madd" : "msub");  // x0 = x1 + x0 * x2
        return;
    } else if (!lp && rp) {
        ASSERT(is_add && "integer - pointer is invalid");
        emit_constant("x2", rt.cast<PointerType>()->base->size());
        fmt::print("madd x0, x1, x2, x0\n");  // x0 = x0 + x1 * x2
        return;
    }

    if (is_add) {
        fmt::print("add x0, x1, x0\n");
    } else {
        fmt::print("sub x0, x1, x0\n");
    }
}

void emit_expr(const ExprVal& expr) {
    if (auto e = expr.cast<IntegerConstantExpr>()) {
        emit_loc(expr);
        emit_constant("x0", e->value);
        return;
    }

    if (auto e = expr.cast<VariableExpr>()) {
        fmt::print("ldr x0, [fp, {}]\n", f.locals[e->ident]);
        return;
    }

    if (auto e = expr.cast<UnOpExpr>()) {
        if (e->op == UnOpKind::AddressOf) {
            emit_addr(e->e);
            return;
        }

        emit_expr(e->e);

        emit_loc(expr);
        switch (e->op) {
        case UnOpKind::Dereference:
            fmt::print("ldr x0, [x0]\n");
            return;
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

    if (auto e = expr.cast<BinOpExpr>()) {
        emit_expr(e->lhs);
        fmt::print("str x0, [sp, -16]!\n");
        emit_expr(e->rhs);
        fmt::print("ldr x1, [sp], 16\n");

        emit_loc(expr);
        switch (e->op) {
        case BinOpKind::Add:
        case BinOpKind::Subtract:
            emit_addsub(e);
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
        case BinOpKind::LessThan:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, lt\n");  // signed compare
            return;
        case BinOpKind::GreaterThan:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, gt\n");  // signed compare
            return;
        case BinOpKind::LessThanEqual:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, le\n");  // signed compare
            return;
        case BinOpKind::GreaterThanEqual:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, ge\n");  // signed compare
            return;
        case BinOpKind::Equal:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, eq\n");
            return;
        case BinOpKind::NotEqual:
            fmt::print("cmp x1, x0\n");
            fmt::print("cset x0, ne\n");
            return;
        case BinOpKind::BitAnd:
            fmt::print("and x0, x1, x0\n");
            return;
        case BinOpKind::BitXor:
            fmt::print("eor x0, x1, x0\n");
            return;
        case BinOpKind::BitOr:
            fmt::print("orr x0, x1, x0\n");
            return;
        default:
            ASSERT(!"Unknown binop kind");
        }
    }

    if (auto e = expr.cast<AssignExpr>()) {
        emit_addr(e->lhs);
        fmt::print("str x0, [sp, -16]!\n");
        emit_expr(e->rhs);
        fmt::print("ldr x1, [sp], 16\n");
        fmt::print("str x0, [x1]\n");
        return;
    }

    ASSERT(!"Unknown expr kind");
}

void emit_stmt(const StmtVal& stmt) {
    if (auto s = stmt.cast<CompoundStmt>()) {
        for (auto& i : s->items) {
            emit_stmt(i);
        }
        return;
    }

    if (auto s = stmt.cast<ExprStmt>()) {
        if (s->e)
            emit_expr(s->e);
        return;
    }

    if (auto s = stmt.cast<IfStmt>()) {
        const int i = iota();
        emit_expr(s->cond);
        fmt::print("cmp x0, 0\n");
        fmt::print("b.eq .if{}.else\n", i);
        emit_stmt(s->then_);
        fmt::print("b .if{}.end\n", i);
        fmt::print(".if{}.else:\n", i);
        if (s->else_)
            emit_stmt(s->else_);
        fmt::print(".if{}.end:\n", i);
        return;
    }

    if (auto s = stmt.cast<LoopStmt>()) {
        const int i = iota();
        if (s->init) {
            emit_expr(s->init);
        }
        fmt::print(".loop{}.cond:\n", i);
        if (s->cond) {
            emit_expr(s->cond);
            fmt::print("cmp x0, 0\n");
            fmt::print("b.eq .loop{}.end\n", i);
        }
        emit_stmt(s->then);
        if (s->incr)
            emit_expr(s->incr);
        fmt::print("b .loop{}.cond\n", i);
        fmt::print(".loop{}.end:\n", i);
        return;
    }

    if (auto s = stmt.cast<ReturnStmt>()) {
        if (s->e)
            emit_expr(s->e);

        emit_loc(stmt);
        fmt::print("ret\n");
        return;
    }

    if (auto s = stmt.cast<DeclStmt>()) {
        // TODO: no duplicates
        f.locals[s->ident] = f.stack_size;
        f.stack_size += 8;
        return;
    }

    ASSERT(!"Unknown stmt kind");
}

int main(int argc, char* argv[]) {
    ASSERT(argc == 2);

    Parser p{TokenStream{CharStream{1, argv[1]}}};

    fmt::print(".file 1 \"stdin\"\n");
    fmt::print(".text\n");
    fmt::print(".globl _main\n");
    fmt::print(".align 4\n");
    fmt::print("_main:\n");

    fmt::print("mov fp, sp\n");
    fmt::print("sub sp, sp, 256\n");

    StmtVal s = p.statement();
    emit_stmt(s);

    fmt::print("add sp, sp, 256\n");
    fmt::print("ret\n");

    return 0;
}

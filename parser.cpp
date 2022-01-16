// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#include "parser.h"

#include <memory>

#include "assert.h"
#include "poly_value.h"

template<typename T, typename... Ts>
static ExprVal make_expr(Ts&&... ts) {
    return make_poly_value<Expr, T>(std::forward<Ts>(ts)...);
}

template<typename T, typename... Ts>
static StmtVal make_stmt(Ts&&... ts) {
    return make_poly_value<Stmt, T>(std::forward<Ts>(ts)...);
}

ExprVal Parser::primary_expression() {
    if (inner.consume_if(PunctuatorKind::LParen)) {
        ExprVal result = expression();
        ASSERT(inner.consume_if(PunctuatorKind::RParen));
        return result;
    }

    // TODO other types of primary expression

    const Token tok = inner.next();

    if (tok.kind == TokenKind::IntegerConstant) {
        return make_expr<IntegerConstantExpr>(inner.loc(), tok.value);
    }

    if (tok.kind == TokenKind::Identifier) {
        return make_expr<VariableExpr>(inner.loc(), tok.payload);
    }

    ASSERT(!"unrecognised primary expression");
    return nullptr;
}

ExprVal Parser::postfix_expression() {
    // TODO
    return primary_expression();
}

ExprVal Parser::unary_expression() {
    // TODO complete this
    if (inner.consume_if(PunctuatorKind::And)) {
        return make_expr<UnOpExpr>(inner.loc(), UnOpKind::AddressOf, cast_expression());
    }
    if (inner.consume_if(PunctuatorKind::Star)) {
        return make_expr<UnOpExpr>(inner.loc(), UnOpKind::Dereference, cast_expression());
    }
    if (inner.consume_if(PunctuatorKind::Plus)) {
        return make_expr<UnOpExpr>(inner.loc(), UnOpKind::Posate, cast_expression());
    }
    if (inner.consume_if(PunctuatorKind::Minus)) {
        return make_expr<UnOpExpr>(inner.loc(), UnOpKind::Negate, cast_expression());
    }
    return postfix_expression();
}

ExprVal Parser::cast_expression() {
    // TODO
    return unary_expression();
}

ExprVal Parser::multiplicative_expression() {
    ExprVal e = cast_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Star)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::Multiply, std::move(e), cast_expression());
        } else if (inner.consume_if(PunctuatorKind::Slash)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::Divide, std::move(e), cast_expression());
        } else if (inner.consume_if(PunctuatorKind::Modulo)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::Modulo, std::move(e), cast_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::additive_expression() {
    ExprVal e = multiplicative_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Plus)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::Add, std::move(e), multiplicative_expression());
        } else if (inner.consume_if(PunctuatorKind::Minus)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::Subtract, std::move(e), multiplicative_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::shift_expression() {
    ExprVal e = additive_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::LLAngle)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::LShift, std::move(e), additive_expression());
        } else if (inner.consume_if(PunctuatorKind::RRAngle)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::RShift, std::move(e), additive_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::relational_expression() {
    ExprVal e = shift_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::LAngle)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::LessThan, std::move(e), shift_expression());
        } else if (inner.consume_if(PunctuatorKind::RAngle)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::GreaterThan, std::move(e), shift_expression());
        } else if (inner.consume_if(PunctuatorKind::LAngleEq)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::LessThanEqual, std::move(e), shift_expression());
        } else if (inner.consume_if(PunctuatorKind::RAngleEq)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::GreaterThanEqual, std::move(e), shift_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::equality_expression() {
    ExprVal e = relational_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::EqEq)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::Equal, std::move(e), relational_expression());
        } else if (inner.consume_if(PunctuatorKind::NotEq)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::NotEqual, std::move(e), relational_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::and_expression() {
    ExprVal e = equality_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::And)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::BitAnd, std::move(e), equality_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::exclusive_or_expression() {
    ExprVal e = and_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Caret)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::BitXor, std::move(e), and_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::inclusive_or_expression() {
    ExprVal e = exclusive_or_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::Or)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::BitOr, std::move(e), exclusive_or_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::logical_and_expression() {
    ExprVal e = inclusive_or_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::AndAnd)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::LogicalAnd, std::move(e), inclusive_or_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::logical_or_expression() {
    ExprVal e = logical_and_expression();
    while (true) {
        if (inner.consume_if(PunctuatorKind::OrOr)) {
            e = make_expr<BinOpExpr>(inner.loc(), BinOpKind::LogicalOr, std::move(e), logical_and_expression());
        } else {
            return e;
        }
    }
}

ExprVal Parser::conditional_expression() {
    // TODO
    return logical_or_expression();
}

ExprVal Parser::assignment_expression() {
    ExprVal e = conditional_expression();
    // TODO: constrain to unary-expression
    // TODO: complete
    if (inner.consume_if(PunctuatorKind::Eq)) {
        return make_expr<AssignExpr>(inner.loc(), std::move(e), assignment_expression());
    }
    return e;
}

ExprVal Parser::expression() {
    // TODO comma operator
    return assignment_expression();
}

StmtVal Parser::compound_statement() {
    std::vector<StmtVal> items;

    ASSERT(inner.consume_if(PunctuatorKind::LBrace));
    Location loc = inner.loc();

    while (!inner.consume_if(PunctuatorKind::RBrace)) {
        items.emplace_back(statement());
    }
    return make_stmt<CompoundStmt>(loc, std::move(items));
}

StmtVal Parser::expression_statement() {
    ExprVal e = expression();
    ASSERT(inner.consume_if(PunctuatorKind::Semi));

    return make_stmt<ExprStmt>(e->loc, std::move(e));
}

StmtVal Parser::if_statement() {
    ASSERT(inner.consume_if_identifier("if"));
    const Location loc = inner.loc();

    ASSERT(inner.consume_if(PunctuatorKind::LParen));
    ExprVal e = expression();
    ASSERT(inner.consume_if(PunctuatorKind::RParen));

    StmtVal then_ = statement();

    if (inner.consume_if_identifier("else")) {
        StmtVal else_ = statement();

        return make_stmt<IfStmt>(loc, std::move(e), std::move(then_), std::move(else_));
    }
    return make_stmt<IfStmt>(loc, std::move(e), std::move(then_), nullptr);
}

StmtVal Parser::while_statement() {
    ASSERT(inner.consume_if_identifier("while"));
    const Location loc = inner.loc();

    ASSERT(inner.consume_if(PunctuatorKind::LParen));
    ExprVal cond = expression();
    ASSERT(inner.consume_if(PunctuatorKind::RParen));

    StmtVal then = statement();
    return make_stmt<LoopStmt>(loc, nullptr, std::move(cond), nullptr, std::move(then));
}

StmtVal Parser::for_statement() {
    ASSERT(inner.consume_if_identifier("for"));
    const Location loc = inner.loc();
    ExprVal init, cond, incr;

    ASSERT(inner.consume_if(PunctuatorKind::LParen));
    if (!inner.consume_if(PunctuatorKind::Semi)) {
        init = expression();
        ASSERT(inner.consume_if(PunctuatorKind::Semi));
    }
    if (!inner.consume_if(PunctuatorKind::Semi)) {
        cond = expression();
        ASSERT(inner.consume_if(PunctuatorKind::Semi));
    }
    if (!inner.consume_if(PunctuatorKind::RParen)) {
        incr = expression();
    }
    ASSERT(inner.consume_if(PunctuatorKind::RParen));

    StmtVal then = statement();
    return make_stmt<LoopStmt>(loc, std::move(init), std::move(cond), std::move(incr), std::move(then));
}

StmtVal Parser::return_statement() {
    ASSERT(inner.consume_if_identifier("return"));
    const Location loc = inner.loc();

    if (inner.peek(PunctuatorKind::Semi)) {
        return make_stmt<ReturnStmt>(loc, nullptr);
    }

    ExprVal e = expression();
    ASSERT(inner.consume_if(PunctuatorKind::Semi));

    return make_stmt<ReturnStmt>(loc, std::move(e));
}

StmtVal Parser::statement() {
    // TODO
    if (inner.peek(PunctuatorKind::Semi)) {
        // null statement
        inner.next();
        return make_stmt<ExprStmt>(inner.loc(), nullptr);
    }
    if (inner.peek(PunctuatorKind::LBrace)) {
        return compound_statement();
    }
    if (inner.peek_identifier("if")) {
        return if_statement();
    }
    if (inner.peek_identifier("while")) {
        return while_statement();
    }
    if (inner.peek_identifier("for")) {
        return for_statement();
    }
    if (inner.peek_identifier("return")) {
        return return_statement();
    }
    // TODO: Temporary
    if (inner.peek_identifier("int")) {
        return declaration();
    }
    return expression_statement();
}

StmtVal Parser::declaration() {
    // TODO: Temporary
    ASSERT(inner.consume_if_identifier("int"));
    const Location loc = inner.loc();
    ASSERT(inner.peek().kind == TokenKind::Identifier);
    // TODO: !keyword
    const std::string ident = inner.next().payload;
    ASSERT(inner.consume_if(PunctuatorKind::Semi));
    return make_stmt<DeclStmt>(loc, ident);
}

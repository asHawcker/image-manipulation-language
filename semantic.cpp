#include "semantic.hpp"

Type SemanticAnalyzer::analyze_expr(Expr *expr)
{

    if (!expr)
        return Type::UNK;

    if (auto *int_expr = dynamic_cast<IntExpr *>(expr))
    {
        expr->type = Type::INT;
        return expr->type;
    }

    if (auto *float_expr = dynamic_cast<FloatExpr *>(expr))
    {
        expr->type = Type::FLOAT;
        return expr->type;
    }

    if (auto *bool_expr = dynamic_cast<BoolExpr *>(expr))
    {
        expr->type = Type::BOOL;
        return expr->type;
    }

    if (auto *var_expr = dynamic_cast<VarExpr *>(expr))
    {
        if (type_sym_table.find(var_expr->value) != type_sym_table.end())
        {
            expr->type = type_sym_table[var_expr->value];
            return expr->type;
        }
        throw std::runtime_error("undeclared variable: " + var_expr->value);
    }

    if (auto *bin_expr = dynamic_cast<BinExpr *>(expr))
    {
        Type lType = analyze_expr(bin_expr->left.get());
        Type rType = analyze_expr(bin_expr->right.get());

        if (
            bin_expr->op == TokenType::OP_lt ||
            bin_expr->op == TokenType::OP_gt ||
            bin_expr->op == TokenType::OP_dequal ||
            bin_expr->op == TokenType::OP_gtequal ||
            bin_expr->op == TokenType::OP_ltequal)
        {
            bin_expr->type = Type::BOOL;
            return bin_expr->type;
        }

        if (lType == Type::BOOL || rType == Type::BOOL)
        {
            throw std::runtime_error("Cant perform arithmetic operation on 'bool'");
        }

        if (lType == Type::FLOAT || rType == Type::FLOAT)
        {
            bin_expr->type = Type::FLOAT;
        }
        else
        {
            bin_expr->type = Type::INT;
        }

        return bin_expr->type;
    }
    if (auto *call_expr = dynamic_cast<CallExpr *>(expr))
    {
        for (auto &arg : call_expr->args)
        {
            analyze_expr(arg.get());
        }
        return expr->type;
    }
    return Type::UNK;
}

Type token_to_type(TokenType tok)
{

    switch (tok)
    {
    case TokenType::KW_int:
        return Type::INT;
    case TokenType::KW_float:
        return Type::FLOAT;
    case TokenType::KW_bool:
        return Type::BOOL;
    default:
        return Type::UNK;
    }
}

void SemanticAnalyzer::analyze_stmt(Stmt *stmt)
{
    if (auto *var_dec = dynamic_cast<VarDecStmt *>(stmt))
    {
        Type init_type = analyze_expr(var_dec->value.get());
        Type declared = token_to_type(var_dec->type);

        if (declared == Type::INT && init_type == Type::FLOAT)
        {
            throw std::runtime_error("Type Error: cannot assign float to int variable '" + var_dec->name + "'");
        }

        type_sym_table[var_dec->name] = declared;
    }
    else if (auto *expr_stmt = dynamic_cast<ExprStmt *>(stmt))
    {
        analyze_expr(expr_stmt->expression.get());
    }
    else if (auto *block_stmt = dynamic_cast<BlockStmt *>(stmt))
    {
        for (auto &s : block_stmt->stmts)
        {
            analyze_stmt(s.get());
        }
    }
    else if (auto *if_stmt = dynamic_cast<IfStmt *>(stmt))
    {
        Type cond_type = analyze_expr(if_stmt->cond.get());
        analyze_stmt(if_stmt->branch_true.get());
        if (if_stmt->branch_false)
        {
            analyze_stmt(if_stmt->branch_false.get());
        }
    }
}

void SemanticAnalyzer::analyze_program(const std::vector<std::unique_ptr<Stmt>> &program)
{
    for (const auto &stmt : program)
    {
        analyze_stmt(stmt.get());
    }
}
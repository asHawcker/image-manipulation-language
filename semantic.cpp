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
        Type t = get_var_type(var_expr->value);
        if (t != Type::UNK)
        {
            expr->type = t;
            return t;
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

        if (lType == Type::STRING || rType == Type::STRING)
        {
            throw std::runtime_error("Cant perform arithmetic operation on 'string'");
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
        if (func_table.find(call_expr->name) == func_table.end())
        {
            throw std::runtime_error("Type Error: call to undeclared function " + call_expr->name);
        }

        FunctionSignature expected_sig = func_table[call_expr->name];

        if (call_expr->args.size() != expected_sig.param_types.size())
        {
            throw std::runtime_error("Type Error: Expected " +
                                     std::to_string(expected_sig.param_types.size()) +
                                     " arguments for '" + call_expr->name + "', but got " +
                                     std::to_string(call_expr->args.size()));
        }

        for (size_t i = 0; i < call_expr->args.size(); i++)
        {
            Type arg_type = analyze_expr(call_expr->args[i].get());
            if (arg_type != expected_sig.param_types[i])
            {
                throw std::runtime_error("Type Error: Argument type mismatch in call to '" + call_expr->name + "'");
            }
        }

        expr->type = expected_sig.return_type;
        return expr->type;
    }

    if (auto *str_expr = dynamic_cast<StrExpr *>(expr))
    {
        expr->type = Type::STRING;
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
    case TokenType::KW_str:
        return Type::STRING;
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

        declare_var(var_dec->name, declared);
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
        if (cond_type != Type::BOOL)
        {
            throw std::runtime_error("Type Error: condition must be a boolean expression");
        }
        analyze_stmt(if_stmt->branch_true.get());
        if (if_stmt->branch_false)
        {
            analyze_stmt(if_stmt->branch_false.get());
        }
    }
    else if (auto *while_stmt = dynamic_cast<WhileStmt *>(stmt))
    {
        Type cond_type = analyze_expr(while_stmt->cond.get());
        if (cond_type != Type::BOOL)
        {
            throw std::runtime_error("Type Error: condition must be a boolean expression");
        }
        analyze_stmt(while_stmt->body.get());
    }
    else if (auto *assign_stmt = dynamic_cast<AssignStmt *>(stmt))
    {
        if (get_var_type(assign_stmt->name) == Type::UNK)
        {
            throw std::runtime_error("Type Error: cannot assign to undeclared variable " + assign_stmt->name);
        }
        Type expr_type = analyze_expr(assign_stmt->value.get());
        Type var_type = get_var_type(assign_stmt->name);

        if (var_type == Type::INT && expr_type == Type::FLOAT)
        {
            throw std::runtime_error("Type Error: cannot assign float to int variable");
        }
    }
    else if (auto *fnc_dec_stmt = dynamic_cast<FncDecStmt *>(stmt))
    {
        Type expected_ret = token_to_type(fnc_dec_stmt->ret_type);
        FunctionSignature sig;
        sig.return_type = expected_ret;
        for (auto &param : fnc_dec_stmt->params)
        {
            sig.param_types.push_back(token_to_type(param.type));
        }
        func_table[fnc_dec_stmt->name] = sig;

        Type previous_ret_type = curr_func_ret_type;
        curr_func_ret_type = expected_ret;

        push_scope();

        for (auto &param : fnc_dec_stmt->params)
        {
            declare_var(param.name, token_to_type(param.type));
        }

        analyze_stmt(fnc_dec_stmt->body.get());

        pop_scope();
        curr_func_ret_type = previous_ret_type;
    }
    else if (auto *ret_stmt = dynamic_cast<RetStmt *>(stmt))
    {
        Type return_expr_type = Type::UNK;

        if (ret_stmt->value)
        {
            return_expr_type = analyze_expr(ret_stmt->value.get());
        }

        if (return_expr_type != curr_func_ret_type)
        {
            throw std::runtime_error("Type Error: return type mismatch");
        }
    }
    else if (auto *extern_dec_stmt = dynamic_cast<ExternDeclStmt *>(stmt))
    {
        Type expected_ret = token_to_type(extern_dec_stmt->ret_type);
        FunctionSignature sig;
        sig.return_type = expected_ret;
        for (auto &param : extern_dec_stmt->arg_types)
        {
            sig.param_types.push_back(token_to_type(param));
        }
        func_table[extern_dec_stmt->name] = sig;
    }
}

void SemanticAnalyzer::analyze_program(const std::vector<std::unique_ptr<Stmt>> &program)
{
    for (const auto &stmt : program)
    {
        analyze_stmt(stmt.get());
    }
}
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.hpp"
#include <map>
#include <string>

struct FunctionSignature
{
    Type return_type;
    std::vector<Type> param_types;
};

class SemanticAnalyzer
{
private:
    std::vector<std::map<std::string, Type>> scopes; // symbol table
    std::map<std::string, FunctionSignature> func_table;

    Type curr_func_ret_type = Type::UNK;

    void push_scope() { scopes.push_back({}); }
    void pop_scope() { scopes.pop_back(); }

    void declare_var(std::string name, Type type)
    {
        scopes.back()[name] = type;
    }

    Type get_var_type(std::string name)
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); it--)
        {
            if (it->find(name) != it->end())
            {
                return it->at(name);
            }
        }
        return Type::UNK;
    }

public:
    SemanticAnalyzer() { push_scope(); }
    void analyze_program(const std::vector<std::unique_ptr<Stmt>> &program);
    void analyze_stmt(Stmt *stmt);
    Type analyze_expr(Expr *expr);
};

#endif
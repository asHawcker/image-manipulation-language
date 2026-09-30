#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.hpp"
#include <map>
#include <string>

class SemanticAnalyzer
{
private:
    std::map<std::string, Type> type_sym_table; // symbol table

public:
    void analyze_program(const std::vector<std::unique_ptr<Stmt>> &program);
    void analyze_stmt(Stmt *stmt);
    Type analyze_expr(Expr *expr);
};

#endif
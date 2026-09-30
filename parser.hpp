#ifndef PARSER_H
#define PARSER_H

#include "ast.hpp"

class Parser
{
private:
    Lexer lexer;
    Token curr_token;

public:
    Parser(Lexer lexer) : lexer(lexer)
    {
        advance();
    }
    void advance();
    bool match(TokenType type);
    Token consume(TokenType type, const std::string &err_msg);

    // parsing methods
    std::unique_ptr<Stmt> parse_stmt();
    std::unique_ptr<Stmt> parse_var_dec_stmt(TokenType tokentype);
    std::unique_ptr<Stmt> parse_expr_stmt();

    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Expr> parse_int_expr();
    std::unique_ptr<Expr> parse_var_expr();
    std::unique_ptr<Expr> parse_bin_expr();
    std::unique_ptr<Expr> parse_call_expr();

    std::vector<std::unique_ptr<Stmt>> parse_program();

    // helpers
    std::unique_ptr<Expr> parse_primary(); // for parse_expr
};

#endif
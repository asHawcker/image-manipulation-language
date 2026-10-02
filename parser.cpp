#include "parser.hpp"
#include <stdexcept>

void Parser::advance()
{
    curr_token = lexer.next_token();
}

bool Parser::match(TokenType type)
{
    if (type == curr_token.type)
        return true;
    return false;
}

Token Parser::consume(TokenType type, const std::string &err_msg)
{
    if (match(type))
    {
        Token tok = curr_token;
        advance();
        return tok;
    }
    else
    {
        throw std::runtime_error(err_msg);
    }
}

std::unique_ptr<Stmt> Parser::parse_stmt()
{
    if (match(TokenType::KW_int))
        return parse_var_dec_stmt(TokenType::KW_int);
    if (match(TokenType::KW_float))
        return parse_var_dec_stmt(TokenType::KW_float);
    if (match(TokenType::KW_bool))
        return parse_var_dec_stmt(TokenType::KW_bool);
    if (match(TokenType::KW_if))
        return parse_if_stmt();
    if (match(TokenType::KW_while))
        return parse_while_stmt();
    if (match(TokenType::KW_extern))
        return parse_extern_stmt();
    if (match(TokenType::Identifier))
    {
        if (lexer.peek_token().type == TokenType::OP_equal)
        {
            return parse_assign_stmt();
        }
    }
    return parse_expr_stmt();
}

std::unique_ptr<Stmt> Parser::parse_var_dec_stmt(TokenType tokentype)
{
    advance();
    Token identifier = consume(TokenType::Identifier, "expected Identifier ");
    Token op = consume(TokenType::OP_equal, "expected '=' equals operator ");
    std::unique_ptr<Expr> val = std::move(parse_expr());
    consume(TokenType::Semicolon, "expected ';' semicolon");
    return std::make_unique<VarDecStmt>(identifier.lexeme, tokentype, std::move(val));
}

std::unique_ptr<Stmt> Parser::parse_block_stmt()
{
    std::vector<std::unique_ptr<Stmt>> block;

    if (match(TokenType::Brace_left))
    {
        advance();
        while (!match(TokenType::Brace_right) && !match(TokenType::Eof))
        {
            block.push_back(parse_stmt());
        }
        consume(TokenType::Brace_right, "expected '}'");
    }
    else
    {
        block.push_back(parse_stmt());
    }
    return std::make_unique<BlockStmt>(std::move(block));
}

std::unique_ptr<Stmt> Parser::parse_if_stmt()
{
    advance();
    consume(TokenType::Paren_left, "expected '('");
    std::unique_ptr<Expr> cond_ = parse_expr();
    consume(TokenType::Paren_right, "expected ')'");
    std::unique_ptr<Stmt> branch_true = parse_block_stmt();
    std::unique_ptr<Stmt> branch_false;
    if (match(TokenType::KW_else))
    {
        advance();
        branch_false = parse_block_stmt();
    }
    return std::make_unique<IfStmt>(std::move(cond_), std::move(branch_true), std::move(branch_false));
}

std::unique_ptr<Stmt> Parser::parse_while_stmt()
{
    advance();
    consume(TokenType::Paren_left, "expected '(' recieved " + curr_token.lexeme);
    std::unique_ptr<Expr> cond = std::move(parse_expr());
    consume(TokenType::Paren_right, "expected ')' recieved " + curr_token.lexeme);
    std::unique_ptr<Stmt> body = parse_block_stmt();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

int Parser::get_token_precedence(TokenType type)
{
    switch (type)
    {
    case TokenType::OP_dequal:
    case TokenType::OP_lt:
    case TokenType::OP_gt:
    case TokenType::OP_ltequal:
    case TokenType::OP_gtequal:
        return 10;
    case TokenType::OP_plus:
    case TokenType::OP_minus:
        return 20;
    case TokenType::OP_star:
    case TokenType::OP_slash:
    case TokenType::OP_mod:
        return 30;
    default:
        return 0;
    }
}

std::unique_ptr<Expr> Parser::parse_primary()
{
    if (match(TokenType::Paren_left))
    {
        advance();
        std::unique_ptr<Expr> expr = parse_expr(0);
        consume(TokenType::Paren_right, "expected ')' after expression");
        return expr;
    }

    if (match(TokenType::Lit_int))
    {
        Token val = consume(TokenType::Lit_int, "expected an integer literal ");
        return std::make_unique<IntExpr>(std::stoi(val.lexeme));
    }
    if (match(TokenType::Lit_float))
    {
        Token val = consume(TokenType::Lit_float, "expected a float literal ");
        return std::make_unique<FloatExpr>(std::stof(val.lexeme));
    }
    if (match(TokenType::KW_true))
    {
        consume(TokenType::KW_true, "");
        return std::make_unique<BoolExpr>(true);
    }
    if (match(TokenType::KW_false))
    {
        consume(TokenType::KW_false, "");
        return std::make_unique<BoolExpr>(false);
    }

    if (match(TokenType::Identifier))
    {
        Token identifier = consume(TokenType::Identifier, "expected identifier");
        if (match(TokenType::Paren_left))
        {
            consume(TokenType::Paren_left, "");
            std::vector<std::unique_ptr<Expr>> args;
            while (!match(TokenType::Paren_right))
            {
                args.push_back(parse_expr(0));
                if (match(TokenType::Comma))
                    consume(TokenType::Comma, "");
            }
            consume(TokenType::Paren_right, "");
            return std::make_unique<CallExpr>(identifier.lexeme, std::move(args));
        }
        return std::make_unique<VarExpr>(identifier.lexeme);
    }

    throw std::runtime_error("Unexpected token in expression: " + curr_token.lexeme);
}

std::unique_ptr<Expr> Parser::parse_expr(int min_precedence)
{
    std::unique_ptr<Expr> left = parse_primary();

    while (true)
    {
        int precedence = get_token_precedence(curr_token.type);
        if (precedence <= min_precedence)
        {
            break;
        }

        Token op = curr_token;
        advance();

        // Left-associative binary expression parsing
        std::unique_ptr<Expr> right = parse_expr(precedence);
        left = std::make_unique<BinExpr>(op.type, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<Stmt> Parser::parse_expr_stmt()
{
    std::unique_ptr<Expr> val = std::move(parse_expr());
    consume(TokenType::Semicolon, "expected ';' semicolon, received " + curr_token.lexeme);
    return std::make_unique<ExprStmt>(std::move(val));
}

std::vector<std::unique_ptr<Stmt>> Parser::parse_program()
{
    std::vector<std::unique_ptr<Stmt>> program;

    while (curr_token.type != TokenType::Eof)
    {
        std::unique_ptr<Stmt> statement = std::move(parse_stmt());
        program.push_back(std::move(statement));
    }
    return program;
}

std::unique_ptr<Stmt> Parser::parse_assign_stmt()
{
    Token identifier = consume(TokenType::Identifier, "expected identifier");
    consume(TokenType::OP_equal, "expected '='");
    std::unique_ptr<Expr> val = std::move(parse_expr());
    consume(TokenType::Semicolon, "expected ';'");
    return std::make_unique<AssignStmt>(identifier.lexeme, std::move(val));
}

std::unique_ptr<Stmt> Parser::parse_extern_stmt()
{
    advance();
    Token ret_tok = consume(curr_token.type, "expected return type");
    Token name_tok = consume(TokenType::Identifier, "expected function name");

    consume(TokenType::Paren_left, "expected '('");
    std::vector<TokenType> args;
    while (!match(TokenType::Paren_right))
    {
        args.push_back(curr_token.type);
        advance();
        if (match(TokenType::Comma))
            advance();
    }
    consume(TokenType::Paren_right, "expected ')'");
    consume(TokenType::Semicolon, "expected ';'");

    return std::make_unique<ExternDeclStmt>(name_tok.lexeme, std::move(args), ret_tok.type);
}
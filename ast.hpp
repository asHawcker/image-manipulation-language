#ifndef AST_H
#define AST_H

#include "lexer.hpp"
#include <memory>
#include <vector>
#include <llvm/IR/Value.h>

enum class Type
{
    INT,
    FLOAT,
    BOOL,
    UNK
};

struct CodeGenContext;

class Expr
{
public:
    Type type = Type::UNK;
    virtual ~Expr() = default;
    virtual llvm::Value *codegen(CodeGenContext &context) = 0;
};

class Stmt
{
public:
    virtual ~Stmt() = default;
    virtual void codegen(CodeGenContext &context) = 0;
};

class IntExpr : public Expr
{
public:
    int value;
    IntExpr(int value) : value(value) { type = Type::INT; }
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class FloatExpr : public Expr
{
public:
    float value;
    FloatExpr(float value) : value(value) { type = Type::FLOAT; }
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class BoolExpr : public Expr
{
public:
    bool value;
    BoolExpr(bool value) : value(value) { type = Type::BOOL; }
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class VarExpr : public Expr
{
public:
    std::string value;
    VarExpr(std::string value) : value(std::move(value)) { type = Type::UNK; }
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class BinExpr : public Expr
{
public:
    TokenType op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    BinExpr(TokenType op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right) : op(op), left(std::move(left)), right(std::move(right)) {}
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class CallExpr : public Expr
{
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args) : name(std::move(name)), args(std::move(args)) {}
    virtual llvm::Value *codegen(CodeGenContext &context) override;
};

class VarDecStmt : public Stmt
{
public:
    std::string name;
    TokenType type;
    std::unique_ptr<Expr> value;
    VarDecStmt(std::string name, TokenType type, std::unique_ptr<Expr> value) : name(std::move(name)), type(type), value(std::move(value)) {}
    virtual void codegen(CodeGenContext &context) override;
};

class ExprStmt : public Stmt
{
public:
    std::unique_ptr<Expr> expression;
    ExprStmt(std::unique_ptr<Expr> expression) : expression(std::move(expression)) {}
    virtual void codegen(CodeGenContext &context) override;
};

class BlockStmt : public Stmt
{
public:
    std::vector<std::unique_ptr<Stmt>> stmts;
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts) : stmts(std::move(stmts)) {}
    virtual void codegen(CodeGenContext &context) override;
};

class IfStmt : public Stmt
{
public:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> branch_true;
    std::unique_ptr<Stmt> branch_false;

    IfStmt(std::unique_ptr<Expr> cond,
           std::unique_ptr<Stmt> branch_true,
           std::unique_ptr<Stmt> branch_false = nullptr) : cond(std::move(cond)),
                                                           branch_true(std::move(branch_true)),
                                                           branch_false(std::move(branch_false)) {}
    virtual void codegen(CodeGenContext &context) override;
};

class WhileStmt : public Stmt
{
public:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> cond,
              std::unique_ptr<Stmt> body) : cond(std::move(cond)),
                                            body(std::move(body)) {}
    virtual void codegen(CodeGenContext &context) override;
};

#endif
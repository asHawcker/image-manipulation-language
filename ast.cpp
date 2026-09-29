#include "ast.hpp"
#include "codegen.hpp"

void ExprStmt::codegen(CodeGenContext &context)
{
    expression->codegen(context);
}

llvm::Value *IntExpr::codegen(CodeGenContext &context)
{
    return llvm::ConstantInt::get(context.context, llvm::APInt(32, value, true));
}

llvm::Value *BinExpr::codegen(CodeGenContext &context)
{
    llvm::Value *L = left->codegen(context);
    llvm::Value *R = right->codegen(context);

    if (L == nullptr || R == nullptr)
    {
        return nullptr;
    }

    switch (op)
    {
    case TokenType::OP_plus:
        return context.builder.CreateAdd(L, R, "addtmp");
    case TokenType::OP_minus:
        return context.builder.CreateSub(L, R, "subtmp");
    case TokenType::OP_star:
        return context.builder.CreateMul(L, R, "multmp");
    case TokenType::OP_slash:
        return context.builder.CreateSDiv(L, R, "divtmp");
    default:
        return nullptr;
    }
}

llvm::Value *VarExpr::codegen(CodeGenContext &context)
{
    if (context.named_values.find(value) != context.named_values.end())
    {
        return context.builder.CreateLoad(llvm::Type::getInt32Ty(context.context), context.named_values[value], value.c_str());
    }
    return nullptr;
}

void VarDecStmt::codegen(CodeGenContext &context)
{
    llvm::Value *init_value = value->codegen(context);
    if (!init_value)
        return;
    llvm::AllocaInst *alloca = context.builder.CreateAlloca(llvm::Type::getInt32Ty(context.context), nullptr, name);
    context.builder.CreateStore(init_value, alloca);
    context.named_values[name] = alloca;
}

llvm::Value *CallExpr::codegen(CodeGenContext &context)
{
    llvm::Function *callee = context.module->getFunction(name);
    if (!callee)
    {
        std::runtime_error("unknown function referenced ");
    }

    std::vector<llvm::Value *> argument_values;
    for (auto &arg : args)
    {
        llvm::Value *tmp = arg->codegen(context);
        if (!tmp)
            return nullptr;
        argument_values.push_back(tmp);
    }

    return context.builder.CreateCall(callee, argument_values, "calltmp");
}
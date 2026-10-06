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
llvm::Value *FloatExpr::codegen(CodeGenContext &context)
{
    return llvm::ConstantFP::get(context.context, llvm::APFloat(value));
}
llvm::Value *BoolExpr::codegen(CodeGenContext &context)
{
    return llvm::ConstantInt::getBool(context.context, value);
}

llvm::Value *BinExpr::codegen(CodeGenContext &context)
{
    llvm::Value *L = left->codegen(context);
    llvm::Value *R = right->codegen(context);
    if (!L || !R)
        return nullptr;

    bool is_float = (left->type == Type::FLOAT || right->type == Type::FLOAT);

    if (is_float)
    {
        if (left->type == Type::INT)
            L = context.builder.CreateSIToFP(L, llvm::Type::getFloatTy(context.context), "castL");
        if (right->type == Type::INT)
            R = context.builder.CreateSIToFP(R, llvm::Type::getFloatTy(context.context), "castR");

        switch (op)
        {
        case TokenType::OP_plus:
            return context.builder.CreateFAdd(L, R, "faddtmp");
        case TokenType::OP_minus:
            return context.builder.CreateFSub(L, R, "fsubtmp");
        case TokenType::OP_star:
            return context.builder.CreateFMul(L, R, "fmultmp");
        case TokenType::OP_slash:
            return context.builder.CreateFDiv(L, R, "fdivtmp");
        case TokenType::OP_lt:
            return context.builder.CreateFCmpOLT(L, R, "cmplttmp");
        case TokenType::OP_gt:
            return context.builder.CreateFCmpOGT(L, R, "cmpgttmp");
        case TokenType::OP_ltequal:
            return context.builder.CreateFCmpOLE(L, R, "cmpltetmp");
        case TokenType::OP_gtequal:
            return context.builder.CreateFCmpOGE(L, R, "cmpgtetmp");
        case TokenType::OP_dequal:
            return context.builder.CreateFCmpOEQ(L, R, "cmpeqtmp");
        default:
            return nullptr;
        }
    }
    else
    {
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
        case TokenType::OP_lt:
            return context.builder.CreateICmpSLT(L, R, "cmplttmp");
        case TokenType::OP_gt:
            return context.builder.CreateICmpSGT(L, R, "cmpgttmp");
        case TokenType::OP_ltequal:
            return context.builder.CreateICmpSLE(L, R, "cmpltetmp");
        case TokenType::OP_gtequal:
            return context.builder.CreateICmpSGE(L, R, "cmpgtetmp");
        case TokenType::OP_dequal:
            return context.builder.CreateICmpEQ(L, R, "cmpeqtmp");
        default:
            return nullptr;
        }
    }
}

llvm::Value *VarExpr::codegen(CodeGenContext &context)
{
    if (context.get_var(value) != nullptr)
    {
        llvm::Value *ptr = context.get_var(value);
        llvm::AllocaInst *alloca = llvm::cast<llvm::AllocaInst>(ptr);
        return context.builder.CreateLoad(alloca->getAllocatedType(), ptr, value.c_str());
    }
    return nullptr;
}

void VarDecStmt::codegen(CodeGenContext &context)
{
    llvm::Value *init_value = value->codegen(context);
    if (!init_value)
        return;
    if (type == TokenType::KW_float && init_value->getType()->isIntegerTy())
        init_value = context.builder.CreateSIToFP(init_value, llvm::Type::getFloatTy(context.context), "casttmp");

    llvm::AllocaInst *alloca;
    if (type == TokenType::KW_int)
        alloca = context.builder.CreateAlloca(llvm::Type::getInt32Ty(context.context), nullptr, name);
    else if (type == TokenType::KW_float)
        alloca = context.builder.CreateAlloca(llvm::Type::getFloatTy(context.context), nullptr, name);
    else if (type == TokenType::KW_bool)
        alloca = context.builder.CreateAlloca(llvm::Type::getInt1Ty(context.context), nullptr, name);
    else if (type == TokenType::KW_str || type == TokenType::KW_img)
        alloca = context.builder.CreateAlloca(llvm::PointerType::getUnqual(context.context), nullptr, name);
    context.builder.CreateStore(init_value, alloca);
    context.set_var(name, alloca);
}

llvm::Value *CallExpr::codegen(CodeGenContext &context)
{
    llvm::Function *callee = context.module->getFunction(name);
    if (!callee)
    {
        throw std::runtime_error("unknown function referenced ");
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

llvm::Value *StrExpr::codegen(CodeGenContext &context)
{
    return context.builder.CreateGlobalString(value, "tmpstr");
}

llvm::Value *ImgExpr::codegen(CodeGenContext &context)
{
    llvm::Function *malloc_func = context.module->getFunction("malloc");
    if (!malloc_func)
    {
        llvm::FunctionType *malloc_type = llvm::FunctionType::get(
            llvm::PointerType::getUnqual(context.context),
            {llvm::Type::getInt64Ty(context.context)},
            false);
        malloc_func = llvm::Function::Create(
            malloc_type,
            llvm::Function::ExternalLinkage,
            "malloc",
            context.module.get());
    }

    const llvm::DataLayout &dl = context.module->getDataLayout();
    uint64_t struct_size = dl.getTypeAllocSize(context.img_struct_type);

    llvm::Value *img_struct_ptr = context.builder.CreateCall(
        malloc_func,
        {llvm::ConstantInt::get(llvm::Type::getInt64Ty(context.context), struct_size)},
        "img_struct");

    int total_pixels = width * height * channels;
    llvm::Value *data_ptr = nullptr;

    if (data && total_pixels > 0)
    {
        uint64_t data_bytes = total_pixels * sizeof(int);
        data_ptr = context.builder.CreateCall(
            malloc_func,
            {llvm::ConstantInt::get(llvm::Type::getInt64Ty(context.context), data_bytes)},
            "img_data");

        for (int i = 0; i < total_pixels; i++)
        {
            llvm::Value *elem_ptr = context.builder.CreateGEP(
                llvm::Type::getInt32Ty(context.context),
                data_ptr,
                {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context.context), i)},
                "pixel_ptr");
            context.builder.CreateStore(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context.context), data[i]),
                elem_ptr);
        }
    }
    else
    {
        data_ptr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context.context));
    }

    llvm::Value *w_ptr = context.builder.CreateStructGEP(context.img_struct_type, img_struct_ptr, 0, "w_ptr");
    context.builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context.context), width), w_ptr);

    llvm::Value *h_ptr = context.builder.CreateStructGEP(context.img_struct_type, img_struct_ptr, 1, "h_ptr");
    context.builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context.context), height), h_ptr);

    llvm::Value *c_ptr = context.builder.CreateStructGEP(context.img_struct_type, img_struct_ptr, 2, "c_ptr");
    context.builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context.context), channels), c_ptr);

    llvm::Value *d_ptr = context.builder.CreateStructGEP(context.img_struct_type, img_struct_ptr, 3, "d_ptr");
    context.builder.CreateStore(data_ptr, d_ptr);

    return img_struct_ptr;
}

llvm::Value *PropAccessExpr::codegen(CodeGenContext &context)
{
    llvm::Value *img_ptr = context.get_var(var_name);
    if (!img_ptr)
    {
        throw std::runtime_error("Undeclared image variable: " + var_name);
    }
    llvm::Value *img_val = context.builder.CreateLoad(llvm::PointerType::getUnqual(context.context), img_ptr, "img_val");

    int field_idx = 0;
    if (property == "width")
        field_idx = 0;
    else if (property == "height")
        field_idx = 1;
    else if (property == "channels")
        field_idx = 2;
    else
        throw std::runtime_error("Unknown image property");

    llvm::Value *field_ptr = context.builder.CreateStructGEP(context.img_struct_type, img_val, field_idx, "imggep");

    return context.builder.CreateLoad(llvm::Type::getInt32Ty(context.context), field_ptr, "propertytmp");
}

void BlockStmt::codegen(CodeGenContext &context)
{
    context.push_scope();
    for (auto &stmt : stmts)
    {
        stmt->codegen(context);
    }
    context.pop_scope();
}

void IfStmt::codegen(CodeGenContext &context)
{
    llvm::Value *condition = cond->codegen(context);
    if (!condition)
        return;

    llvm::Function *parent_func = context.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *true_block = llvm::BasicBlock::Create(context.context, "then", parent_func, nullptr);
    llvm::BasicBlock *false_block = llvm::BasicBlock::Create(context.context, "else");
    llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(context.context, "continue");

    context.builder.CreateCondBr(condition, true_block, false_block);

    // then block code
    context.builder.SetInsertPoint(true_block);
    branch_true->codegen(context);
    if (!context.builder.GetInsertBlock()->getTerminator())
    {
        context.builder.CreateBr(merge_block);
    }

    // else block code
    parent_func->insert(parent_func->end(), false_block);
    context.builder.SetInsertPoint(false_block);
    if (branch_false)
    {
        branch_false->codegen(context);
    }
    if (!context.builder.GetInsertBlock()->getTerminator())
    {
        context.builder.CreateBr(merge_block);
    }

    // merge
    parent_func->insert(parent_func->end(), merge_block);
    context.builder.SetInsertPoint(merge_block);
}

void WhileStmt::codegen(CodeGenContext &context)
{

    llvm::Function *parent_func = context.builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *cond_block = llvm::BasicBlock::Create(context.context, "whilecond", parent_func);
    llvm::BasicBlock *body_block = llvm::BasicBlock::Create(context.context, "whilebody");
    llvm::BasicBlock *end = llvm::BasicBlock::Create(context.context, "whileend");

    context.builder.CreateBr(cond_block);
    context.builder.SetInsertPoint(cond_block);
    llvm::Value *condition = cond->codegen(context);
    if (!condition)
        return;
    context.builder.CreateCondBr(condition, body_block, end);

    parent_func->insert(parent_func->end(), body_block);
    context.builder.SetInsertPoint(body_block);
    body->codegen(context);
    if (!context.builder.GetInsertBlock()->getTerminator())
    {
        context.builder.CreateBr(cond_block);
    }

    parent_func->insert(parent_func->end(), end);
    context.builder.SetInsertPoint(end);
}

void AssignStmt::codegen(CodeGenContext &context)
{
    llvm::Value *val = value->codegen(context);
    if (!val)
        return;

    if (context.get_var(name) == nullptr)
    {
        throw std::runtime_error("Undeclared variable assignment");
    }
    llvm::Value *ptr = context.get_var(name);

    context.builder.CreateStore(val, ptr);
}

void ExternDeclStmt::codegen(CodeGenContext &context)
{
    std::vector<llvm::Type *> args;
    for (auto t : arg_types)
    {
        if (t == TokenType::KW_int)
            args.push_back(llvm::Type::getInt32Ty(context.context));
        else if (t == TokenType::KW_float)
            args.push_back(llvm::Type::getFloatTy(context.context));
        else if (t == TokenType::KW_bool)
            args.push_back(llvm::Type::getInt1Ty(context.context));
        else if (t == TokenType::KW_str || t == TokenType::KW_img)
            args.push_back(llvm::PointerType::getUnqual(context.context));
    }

    llvm::Type *ret = llvm::Type::getVoidTy(context.context);
    if (ret_type == TokenType::KW_int)
        ret = llvm::Type::getInt32Ty(context.context);
    else if (ret_type == TokenType::KW_float)
        ret = llvm::Type::getFloatTy(context.context);
    else if (ret_type == TokenType::KW_bool)
        ret = llvm::Type::getInt1Ty(context.context);
    else if (ret_type == TokenType::KW_str || ret_type == TokenType::KW_img)
        ret = llvm::PointerType::getUnqual(context.context);

    llvm::FunctionType *func_type = llvm::FunctionType::get(ret, args, false);

    llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, context.module.get());
}

void FncDecStmt::codegen(CodeGenContext &context)
{
    std::vector<llvm::Type *> param_types;
    for (auto &p : params)
    {
        if (p.type == TokenType::KW_int)
            param_types.push_back(llvm::Type::getInt32Ty(context.context));
        if (p.type == TokenType::KW_float)
            param_types.push_back(llvm::Type::getFloatTy(context.context));
        if (p.type == TokenType::KW_bool)
            param_types.push_back(llvm::Type::getInt1Ty(context.context));
        if (p.type == TokenType::KW_str || p.type == TokenType::KW_img)
            param_types.push_back(llvm::PointerType::getUnqual(context.context));
    }

    llvm::Type *return_type = llvm::Type::getVoidTy(context.context);
    if (ret_type == TokenType::KW_int)
        return_type = llvm::Type::getInt32Ty(context.context);
    else if (ret_type == TokenType::KW_float)
        return_type = llvm::Type::getFloatTy(context.context);
    else if (ret_type == TokenType::KW_bool)
        return_type = llvm::Type::getInt1Ty(context.context);
    else if (ret_type == TokenType::KW_str || ret_type == TokenType::KW_img)
        return_type = llvm::PointerType::getUnqual(context.context);

    llvm::FunctionType *fn_type = llvm::FunctionType::get(return_type, param_types, false);
    llvm::Function *fnc = llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage, name, context.module.get());

    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context.context, "entry", fnc);

    llvm::BasicBlock *prev_insert_point = context.builder.GetInsertBlock();
    context.builder.SetInsertPoint(entry);

    context.push_scope();

    size_t index = 0;
    for (auto &arg : fnc->args())
    {
        std::string arg_name = params[index].name;
        arg.setName(arg_name);

        llvm::AllocaInst *alloca = context.builder.CreateAlloca(arg.getType(), nullptr, arg_name);
        context.builder.CreateStore(&arg, alloca);

        context.set_var(arg_name, alloca);
        index++;
    }

    body->codegen(context);

    if (!context.builder.GetInsertBlock()->getTerminator())
    {
        if (return_type->isVoidTy())
        {
            context.builder.CreateRetVoid();
        }
        else
        {
            context.builder.CreateRet(llvm::UndefValue::get(return_type));
        }
    }

    context.pop_scope();
    if (prev_insert_point)
    {
        context.builder.SetInsertPoint(prev_insert_point);
    }
}

void RetStmt::codegen(CodeGenContext &context)
{
    if (value)
    {
        llvm::Value *ret_val = value->codegen(context);
        context.builder.CreateRet(ret_val);
    }
    else
    {
        context.builder.CreateRetVoid();
    }
}
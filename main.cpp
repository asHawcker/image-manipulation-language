#include "parser.hpp"
#include "codegen.hpp"

int main()
{
    const std::string source_code = R"(
    int x = 2;
    int y= 3;
    print(x+y);
    )";

    Lexer lexer(source_code);
    Parser parser(lexer);

    auto program = parser.parse_program();

    CodeGenContext context;

    llvm::FunctionType *mainType = llvm::FunctionType::get(llvm::Type::getVoidTy(context.context), false);

    llvm::Function *mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", context.module.get());

    llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(context.context, "entry_block", mainFunc);

    context.builder.SetInsertPoint(entry_block);

    llvm::FunctionType *printType = llvm::FunctionType::get(llvm::Type::getVoidTy(context.context), {llvm::Type::getInt32Ty(context.context)}, false);

    llvm::Function::Create(printType, llvm::Function::ExternalLinkage, "print", context.module.get());

    for (auto &stmt : program)
    {
        stmt->codegen(context);
    }

    context.builder.CreateRetVoid();

    context.module->print(llvm::outs(), nullptr);
    return 0;
}
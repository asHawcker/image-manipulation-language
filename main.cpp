#include "parser.hpp"
#include "codegen.hpp"

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/TargetParser/Triple.h>

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

    llvm::Function *mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "iml_main", context.module.get());

    llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(context.context, "entry_block", mainFunc);

    context.builder.SetInsertPoint(entry_block);

    llvm::FunctionType *printType = llvm::FunctionType::get(llvm::Type::getVoidTy(context.context), {llvm::Type::getInt32Ty(context.context)}, false);

    llvm::Function::Create(printType, llvm::Function::ExternalLinkage, "print", context.module.get());

    for (auto &stmt : program)
    {
        stmt->codegen(context);
    }

    context.builder.CreateRetVoid();

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::ModuleAnalysisManager mam;
    llvm::CGSCCAnalysisManager cam;

    llvm::PassBuilder PB;

    PB.registerModuleAnalyses(mam);
    PB.registerFunctionAnalyses(fam);
    PB.crossRegisterProxies(lam, fam, cam, mam);

    llvm::FunctionPassManager fpm;
    fpm.addPass(llvm::PromotePass());
    fpm.run(*mainFunc, fam);

    context.module->print(llvm::outs(), nullptr);

    // object-file output

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    llvm::Triple targetTriple(llvm::sys::getDefaultTargetTriple());
    context.module->setTargetTriple(targetTriple);

    std::string Error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);

    if (!target)
    {
        llvm::errs() << Error;
        return 1;
    }

    auto CPU = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    auto TargetMachine = target->createTargetMachine(targetTriple, CPU, features, opt, llvm::Reloc::PIC_);

    context.module->setDataLayout(TargetMachine->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest("output.o", ec, llvm::sys::fs::OF_None);

    if (ec)
    {
        llvm::errs() << "could not open file" << ec.message();
    }

    llvm::legacy::PassManager pass;

    auto FileType = llvm::CodeGenFileType::ObjectFile;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
    {
        llvm::errs() << "Target machine can't emit a file of this type";
        return 1;
    }

    pass.run(*context.module);
    dest.flush();

    return 0;
}
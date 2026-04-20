#include <cstdlib>
#include <iostream>
#include <memory>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>

int main(int argc, char** argv) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  llvm::ExitOnError exit_on_err;

  auto jit = exit_on_err(llvm::orc::LLJITBuilder().create());

  auto context = std::make_unique<llvm::LLVMContext>();
  auto module = std::make_unique<llvm::Module>("jit_module", *context);
  module->setDataLayout(jit->getDataLayout());

  llvm::IRBuilder<> builder(*context);
  llvm::FunctionType* function_type = llvm::FunctionType::get(
      builder.getInt32Ty(), {builder.getInt32Ty()}, false);
  llvm::Function* add_one = llvm::Function::Create(
      function_type, llvm::Function::ExternalLinkage, "add1", module.get());

  llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", add_one);
  builder.SetInsertPoint(entry);

  llvm::Value* input = add_one->getArg(0);
  llvm::Value* one = llvm::ConstantInt::get(builder.getInt32Ty(), 1);
  llvm::Value* sum = builder.CreateAdd(input, one, "sum");
  builder.CreateRet(sum);

  if (llvm::verifyFunction(*add_one, &llvm::errs()) ||
      llvm::verifyModule(*module, &llvm::errs())) {
    std::cerr << "Verification failed" << std::endl;
    return 1;
  }

  exit_on_err(jit->addIRModule(
      llvm::orc::ThreadSafeModule(std::move(module), std::move(context))));

  llvm::orc::ExecutorAddr symbol = exit_on_err(jit->lookup("add1"));
  using AddOneFn = int (*)(int);
  auto* fn = symbol.toPtr<AddOneFn>();

  int value = 41;
  if (argc > 1) {
    value = std::atoi(argv[1]);
  }

  const int result = fn(value);
  std::cout << "add1(" << value << ") = " << result << std::endl;

  return 0;
}

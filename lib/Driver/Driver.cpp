#include "eclair/Driver/Driver.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

llvm::cl::opt<OptLevel> OptimizationLevel(llvm::cl::desc("Choose optimization level:"),
  llvm::cl::values(
    clEnumValN(NoOptimizations, "O0", "No optimizations"),
    clEnumVal(O1, "Enable trivial optimizations"),
    clEnumVal(O2, "Enable default optimizations"),
    clEnumVal(O3, "Enable expensive optimizations")
  )
);

namespace llvm {
namespace orc {
Expected<ThreadSafeModule> EclairJIT::optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R) {
    TSM.withModuleDo([](Module &M) {
      auto FPM = std::make_unique<legacy::FunctionPassManager>(&M);

      FPM->doInitialization();

      for (auto &F : M) {
        FPM->run(F);
      }
    });

    return std::move(TSM);
  }
}
}

namespace eclair {

static std::unique_ptr<llvm::orc::EclairJIT> TheJIT;
static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<SLContext> TheSLContext;
static ExitOnError ExitOnErr;

LLVMContext& getGlobalContext() {
  return *TheContext.get();
}

SLContext& getSLContext() {
  return *TheSLContext.get();
}

llvm::orc::EclairJIT& getJIT() {
  return *TheJIT.get();
}

void initJIT() {
  llvm::DebugFlag = false;
  TheJIT = ExitOnErr(llvm::orc::EclairJIT::Create());

  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("eclair", *TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());

  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  TheSLContext = std::make_unique<SLContext>();

  TheSLContext->TheTarget = &TheModule->getDataLayout();
  TheSLContext->TheModule = TheModule.get();
  TheSLContext->TheBuilder = Builder.get();
}

llvm::JITTargetAddress getJITMain() {
  auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));
  auto H = TheJIT->addModule(std::move(TSM));

  auto Sym = ExitOnErr(getJIT().lookup("main"));

  return Sym.getAddress();
}

}
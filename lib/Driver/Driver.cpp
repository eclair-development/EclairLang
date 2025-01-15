#include <eclair/Driver/Driver.h>
#include <llvm/Support/Debug.h>
#include <iostream>

using namespace llvm;

llvm::cl::opt<EclairOpts> Optimization(llvm::cl::desc("Choose optimization level:"),
  llvm::cl::values(
    clEnumValN(NoOptimizations, "O0", "No optimizations"),
    clEnumVal(OR, "optimization for recursive functions"),
    clEnumVal(OL, "optimizations for loops"),
    clEnumVal(OA, "optimizations for array access"),
    clEnumVal(ORL, "optimization for recursive functions and loops"),
    clEnumVal(OLA, "optimizations for loops and array access"),
    clEnumVal(ORA, "optimizations for recursive functions and array access"),
    clEnumVal(ORLA, "optimization for recursive functions, loops and array access")
  )
);

namespace llvm {
namespace orc {
void EclairJIT::optimizeRecursions(legacy::FunctionPassManager &FPM) {
    FPM.add(llvm::createTailCallEliminationPass()); 
    FPM.add(llvm::createConstraintEliminationPass());
    FPM.add(llvm::createInstructionCombiningPass());
    FPM.add(llvm::createGVNPass());
}
void EclairJIT::optimizeLoops(legacy::FunctionPassManager &FPM) {
    FPM.add(llvm::createLoopSimplifyCFGPass());
}
void EclairJIT::optimizeArrayAccess(legacy::FunctionPassManager &FPM) {
    FPM.add(llvm::createMemCpyOptPass());
}
Expected<ThreadSafeModule> EclairJIT::optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R) {
    TSM.withModuleDo([](Module &M) {
      auto FPM = legacy::FunctionPassManager(&M);
      
      switch(Optimization){
        case OR : {
            optimizeRecursions(FPM);
        }
        case OL : {
            optimizeLoops(FPM);
        }
        case OA : {
          optimizeArrayAccess(FPM);
        }
        case ORL : {
          optimizeRecursions(FPM);
          optimizeLoops(FPM);
        }
        case OLA : {
          optimizeLoops(FPM);
          optimizeArrayAccess(FPM);
        }
        case ORA : {
          optimizeRecursions(FPM);
          optimizeArrayAccess(FPM);
        }
        case ORLA : {
          optimizeRecursions(FPM);
          optimizeLoops(FPM);
          optimizeArrayAccess(FPM);
        }
      }

      // if (Optimization > NoOptimizations) {
      //   std::cout << "Optimization: " << Optimization << std::endl;
      //   FPM->add(createInstructionCombiningPass());
      //   FPM->add(createReassociatePass());
      //   FPM->add(createGVNPass());
      //   FPM->add(createCFGSimplificationPass());
      // }

      FPM.doInitialization();

      for (auto &F : M) {
        FPM.run(F);
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
  std::cout << "opt" << std::endl;
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
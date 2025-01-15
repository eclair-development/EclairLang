#ifndef ECLAIR_DRIVER_DRIVER_H
#define ECLAIR_DRIVER_DRIVER_H

#include <memory>

#include "eclair/Basic/LLVM.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"


enum EclairOpts {
  NoOptimizations, OR, OL, OA, ORL, OLA, ORA, ORLA
};

namespace llvm {
namespace orc {

class EclairJIT {
private:
  std::unique_ptr<ExecutionSession> ES;

  DataLayout DL;
  MangleAndInterner Mangle;

  RTDyldObjectLinkingLayer ObjectLayer;
  IRCompileLayer CompileLayer;
  IRTransformLayer OptimizeLayer;

  JITDylib &MainJD;

public:
  EclairJIT(std::unique_ptr<ExecutionSession> ES,
                  JITTargetMachineBuilder JTMB, DataLayout DL)
      : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
        ObjectLayer(*this->ES,
                    []() { return std::make_unique<SectionMemoryManager>(); }),
        CompileLayer(*this->ES, ObjectLayer,
                     std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
        OptimizeLayer(*this->ES, CompileLayer, optimizeModule),
        MainJD(this->ES->createBareJITDylib("<main>")) {
    MainJD.addGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
            DL.getGlobalPrefix())));
  }

  ~EclairJIT() {
    if (auto Err = ES->endSession()) {
      ES->reportError(std::move(Err));
    }
  }

  static Expected<std::unique_ptr<EclairJIT>> Create() {
    auto EPC = SelfExecutorProcessControl::Create();
    
    if (!EPC) {
      return EPC.takeError();
    }

    auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

    JITTargetMachineBuilder JTMB(
        ES->getExecutorProcessControl().getTargetTriple());

    auto DL = JTMB.getDefaultDataLayoutForTarget();
    
    if (!DL) {
      return DL.takeError();
    }

    return std::make_unique<EclairJIT>(std::move(ES), std::move(JTMB),
                                             std::move(*DL));
  }

  const DataLayout &getDataLayout() const { return DL; }

  JITDylib &getMainJITDylib() { return MainJD; }

  Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
    if (!RT) {
      RT = MainJD.getDefaultResourceTracker();
    }

    return OptimizeLayer.add(RT, std::move(TSM));
  }

  Expected<JITEvaluatedSymbol> lookup(StringRef Name) {
    return ES->lookup({&MainJD}, Mangle(Name.str()));
  }

  Error addSymbol(StringRef Name, void* Ptr) {
    SymbolMap SMap;

    SMap[Mangle(Name)] = JITEvaluatedSymbol(
      pointerToJITTargetAddress(Ptr),
      JITSymbolFlags()
    );

    return MainJD.define(
      absoluteSymbols(std::move(SMap))
    );
  }

private:
  static Expected<ThreadSafeModule>
  optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R);

  static void optimizeRecursions(legacy::FunctionPassManager &FPM);
  
  static void optimizeLoops(legacy::FunctionPassManager &FPM);

  static void optimizeArrayAccess(legacy::FunctionPassManager &FPM);

};

} // namespace orc
} // namespace llvm

namespace eclair {

llvm::LLVMContext& getGlobalContext();

struct SLContext {
  llvm::Module* TheModule; ///< Current module
  llvm::Function* TheFunction; ///< Current function
  llvm::IRBuilder< >* TheBuilder; ///< Code builder
  // llvm::FunctionPassManager* ThePass; ///< Function's pass
  const llvm::DataLayout* TheTarget; ///< Info about target machine
};

SLContext& getSLContext();

llvm::orc::EclairJIT& getJIT();

void initJIT();

llvm::JITTargetAddress getJITMain();

} // namespace eclair

#endif
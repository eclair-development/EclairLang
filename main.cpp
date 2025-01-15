#include "eclair/AST/AST.h"
#include "eclair/Lexer/Lexer.h"
#include "eclair/Parser/Parser.h"
#include "eclair/Basic/Diagnostic.h"
#include "eclair/Driver/Driver.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/WithColor.h"
#include <iostream>


static llvm::cl::opt<std::string> InputFilename(
  llvm::cl::Positional,
  llvm::cl::Required,
  llvm::cl::desc("<input file>")
);

static const char *Head = "Eclair - best programming language";

void printVersion(llvm::raw_ostream &OS) {
  OS << "  Default target: "
     << llvm::sys::getDefaultTargetTriple() << "\n";
  std::string CPU(llvm::sys::getHostCPUName());
  OS << "  Host CPU: " << CPU << "\n";
  OS << "\n";
  OS.flush();
  exit(EXIT_SUCCESS);
}

int main(int Argc, const char **Argv) {
  llvm::InitLLVM X(Argc, Argv);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  llvm::cl::SetVersionPrinter(&printVersion);
  llvm::cl::ParseCommandLineOptions(Argc, Argv, Head);

  eclair::initJIT();

  llvm::SourceMgr SrcMgr;
  eclair::DiagnosticsEngine Diags(SrcMgr);

  std::unique_ptr<eclair::ModuleDeclAST> Mod(
    eclair::ModuleDeclAST::load(SrcMgr, Diags, InputFilename)
  );

  if (!Mod) {
    return -1;
  }

  Mod->semantic();
  Mod->generateCode();

  if (Mod->MainPtr) {
    llvm::outs() << "\n" << Mod->MainPtr() << "\n";
    llvm::outs().flush();
  }

  eclair::TypeAST::clearAllTypes();

  return 0;
}
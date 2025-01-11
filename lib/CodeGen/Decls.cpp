#include "eclair/AST/AST.h"

using namespace llvm;

cl::opt< std::string > OutputFilename("o", cl::desc("Specify output filename"), 
  cl::value_desc("filename"));

namespace eclair {

// # 1
Value* SymbolAST::getValue(SLContext& ) {
  assert(0 && "SymbolAST::getValue should never be reached");
  return nullptr;
}

Value* SymbolAST::generateCode(SLContext& Context) {
  assert(0 && "SymbolAST::generateCode should never be reached");
  return nullptr;
}

// # 2
Value* VarDeclAST::generateCode(SLContext& Context) {
  assert(SemaState >= 5);
  Value* val = getValue(Context);


  if (Val) {
    Value* init = Val->getRValue(Context);

    return Context.TheBuilder->CreateStore(init, val);
  }

  return val;
}

Value* VarDeclAST::getValue(SLContext& Context) {
  if (CodeValue) {
    return CodeValue;
  }

  CodeValue = Context.TheBuilder->CreateAlloca(ThisType->getType(),
    nullptr, StringRef(Id->Id, Id->Length));
  return CodeValue;
}

// # 3
llvm::Value* ParameterSymbolAST::getValue(SLContext& Context) {
  if (Param->CodeValue) {
    return Param->CodeValue;
  }

  Param->CodeValue = Context.TheBuilder->CreateAlloca(Param->Param->getType());
  return Param->CodeValue;
}

llvm::Value* ParameterSymbolAST::generateCode(SLContext& Context) {
  assert(SemaState >= 5);

  return getValue(Context);
}

// # 4
Value* FuncDeclAST::getValue(SLContext& Context) {
  if (CodeValue) {
    return CodeValue;
  }

  SmallString< 128 > str;
  raw_svector_ostream output(str);

  if (!(Id->Length == 4 && memcmp(Id->Id, "main", 4) == 0)) {
    output << "_P" << Id->Length << StringRef(Id->Id, Id->Length);
    ThisType->toMangleBuffer(output);
  } else {
    output << "main";
  }

  CodeValue = Function::Create((FunctionType*)ThisType->getType(), 
    Function::ExternalLinkage, output.str(), nullptr);
  Context.TheModule->getFunctionList().push_back(CodeValue);

  return CodeValue;
}
// # 5
Value* FuncDeclAST::generateCode(SLContext& Context) {
  if (Compiled) {
    return CodeValue;
  }

  assert(SemaState >= 5);
  getValue(Context);

  BasicBlock* oldBlock = Context.TheBuilder->GetInsertBlock();

  Function::arg_iterator AI = CodeValue->arg_begin();
  ParameterList::iterator PI = ((FuncTypeAST*)ThisType)->Params.begin();
  ParameterList::iterator PE = ((FuncTypeAST*)ThisType)->Params.end();

  BasicBlock* BB = BasicBlock::Create(getGlobalContext(), "entry", CodeValue);
  Context.TheBuilder->SetInsertPoint(BB);

  for (std::vector< SymbolAST* >::iterator it = FuncVars.begin(), end = FuncVars.end();
    it != end; ++it) {
    (*it)->getValue(Context);
  }

  for ( ; PI != PE; ++PI, ++AI) {
    ParameterAST* p = *PI;

    if (p->Id) {
      AI->setName(StringRef(p->Id->Id, p->Id->Length));
      if (!p->CodeValue) {
        p->CodeValue = Context.TheBuilder->CreateAlloca(p->Param->getType());
      }

      Context.TheBuilder->CreateStore(AI, p->CodeValue);
    }
  }

  if (!ReturnType->isVoid()) {
    LandingPad->ReturnValue = Context.TheBuilder->CreateAlloca(
      ReturnType->getType(),
      nullptr,
      "return.value"
    );
  }

  LandingPad->ReturnLoc = BasicBlock::Create(getGlobalContext(), "return.block");
  LandingPad->FallthroughLoc = LandingPad->ReturnLoc;

  Function* oldFunction = Context.TheFunction;
  Context.TheFunction = CodeValue;

  Body->generateCode(Context);
 
  Context.TheFunction = oldFunction;

  if (!Context.TheBuilder->GetInsertBlock()->getTerminator()) {
    Context.TheBuilder->CreateBr(LandingPad->ReturnLoc);
  }

  CodeValue->getBasicBlockList().push_back(LandingPad->ReturnLoc);
  Context.TheBuilder->SetInsertPoint(LandingPad->ReturnLoc);
  
  if (!ReturnType->isVoid()) {
    Value* ret = Context.TheBuilder->CreateLoad(
      ReturnType->getType(),
      LandingPad->ReturnValue
    );
    Context.TheBuilder->CreateRet(ret);
  } else {
    Context.TheBuilder->CreateRetVoid();
  }

  if (oldBlock) {
    Context.TheBuilder->SetInsertPoint(oldBlock);
  }

  verifyFunction(*CodeValue, &llvm::errs());
  
  Compiled = true;

  return CodeValue;
}

void ModuleDeclAST::generateCode() {
  SLContext& Context = getSLContext();
  
  for (SymbolList::iterator it = Members.begin(), end = Members.end();
    it != end; ++it) {
    (*it)->generateCode(Context);
  }

  Context.TheModule->dump();

  if (!OutputFilename.empty()) {
    std::error_code errorInfo;
    raw_fd_ostream fd(OutputFilename.c_str(), errorInfo);

    if (errorInfo) {
      llvm::errs() << "Can't write to \"" << OutputFilename.c_str() << "\" file\n";
    }

    Context.TheModule->print(fd, 0);
  }

  MainPtr = (double (*)())(intptr_t)getJITMain();
}

}
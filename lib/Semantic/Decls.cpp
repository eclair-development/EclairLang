#include "eclair/AST/AST.h"
#include "eclair/Lexer/Lexer.h"
#include "eclair/Parser/Parser.h"

using namespace llvm;

namespace eclair {

TypeAST* SymbolAST::getType() {
  assert(0 && "SymbolAST::getType should never be reached");
  return nullptr;
}

void SymbolAST::semantic(Scope* scope) {
  if (SemaState > 0) {
    return;
  }

  doSemantic(scope);

  ++SemaState;
}

void SymbolAST::semantic2(Scope* scope) {
  assert(SemaState >= 1);
  if (SemaState > 1) {
    return;
  }

  doSemantic2(scope);

  ++SemaState;
}

void SymbolAST::semantic3(Scope* scope) {
  assert(SemaState >= 2);
  if (SemaState > 2) {
    return;
  }

  doSemantic3(scope);

  ++SemaState;
}

void SymbolAST::semantic4(Scope* scope) {
  assert(SemaState >= 3);
  if (SemaState > 3) {
    return;
  }

  doSemantic4(scope);

  ++SemaState;
}

void SymbolAST::semantic5(Scope* scope) {
  assert(SemaState >= 4);
  if (SemaState > 4) {
    return;
  }

  doSemantic5(scope);

  ++SemaState;
}

void SymbolAST::doSemantic(Scope* ) {
  assert(0 && "SymbolAST::semantic should never be reached");
}

void SymbolAST::doSemantic2(Scope* ) {
  // By default do nothing
}

void SymbolAST::doSemantic3(Scope* ) {
  // By default do nothing
}

void SymbolAST::doSemantic4(Scope* ) {
  // By default do nothing
}

void SymbolAST::doSemantic5(Scope* ) {
  // By default do nothing
}

SymbolAST* SymbolAST::find(Name* id, int flags) {
  if (id == Id) {
    return this;
  }

  return nullptr;
}

TypeAST* VarDeclAST::getType() {
  return ThisType;
}

void VarDeclAST::doSemantic(Scope* scope) {
  if (scope->find(Id)) {
    scope->report(Loc, diag::ERR_SemaIdentifierRedefinition);
    return;
  }

  ((ScopeSymbol*)scope->CurScope)->Decls[Id] = this;
  Parent = scope->CurScope;

  if (scope->EnclosedFunc) {
    scope->EnclosedFunc->FuncVars.push_back(this);
  }

  ThisType = ThisType->semantic(scope);
}

void VarDeclAST::doSemantic3(Scope* scope) {
  if (Val) {
    Val = Val->semantic(scope);

    if (!Val->ExprType || Val->ExprType->isVoid()) {
      scope->report(Loc, diag::ERR_SemaVoidInitializer);
      return;
    }

    if (!Val->ExprType->equal(ThisType)) {
      Val = new CastExprAST(Loc, Val, ThisType);
      Val = Val->semantic(scope);
    }
  }
}

ScopeSymbol::~ScopeSymbol() {
}

SymbolAST* ScopeSymbol::find(Name* id, int /*flags*/) {
  SymbolMap::iterator it = Decls.find(id);

  if (it == Decls.end()) {
    return nullptr;
  }

  return it->second;
}

TypeAST* ParameterSymbolAST::getType() {
  return Param->Param;
}

void ParameterSymbolAST::doSemantic(Scope* ) {
}

SymbolAST* ParameterSymbolAST::find(Name* id, int ) {
  if (id == Param->Id) {
    return this;
  }

  return nullptr;
}

TypeAST* FuncDeclAST::getType() {
  return ThisType;
}

void FuncDeclAST::doSemantic(Scope* scope) {
  ThisType = ThisType->semantic(scope);
  Parent = scope->CurScope;
  ReturnType = ((FuncTypeAST*)ThisType)->ReturnType;
  

  if (Id->Length == 4 && memcmp(Id->Id, "main", 4) == 0) {
    FuncTypeAST* thisType = (FuncTypeAST*)ThisType;

    if (thisType->Params.size()) {
      scope->report(Loc, diag::ERR_SemaMainParameters);
      return;
    }

    if (ReturnType != BuiltinTypeAST::get(TypeAST::TI_Float)) {
      scope->report(Loc, diag::ERR_SemaMainReturnType);
      return;
    }
  }

  if (SymbolAST* fncOverload = scope->findMember(Id, 1)) {
    scope->report(Loc, diag::ERR_SemaFunctionRedefined, Id->Id);
    return;
  }

  ((ScopeSymbol*)Parent)->Decls[Id] = this;
}

void FuncDeclAST::doSemantic5(Scope* scope) {
  FuncTypeAST* func = (FuncTypeAST*)ThisType;
  if (Body) {
    Scope* s = scope->push(this);
    s->EnclosedFunc = this;

    for (ParameterList::iterator it = func->Params.begin(), end = func->Params.end();
      it != end; ++it) {
      ParameterAST* p = *it;
      if (p->Id) {
        // Disallow redefinition
        if (find(p->Id)) {
          scope->report(Loc, diag::ERR_SemaIdentifierRedefinition);
          return;
        }

        SymbolAST* newSym = new ParameterSymbolAST(p);
        Decls[p->Id] = newSym;
        FuncVars.push_back(newSym);
      }
    }

    LandingPadAST* oldLandingPad = s->LandingPad;
    LandingPad = new LandingPadAST();
    s->LandingPad = LandingPad;

    Body = Body->semantic(s);

    s->LandingPad = oldLandingPad;

    if (!ReturnType->isVoid() && !Body->hasReturn()) {
      scope->report(Loc, diag::ERR_SemaMissingReturnValueInFunction);
      return;
    }

    s->pop();
  }
}

extern "C" void lle_X_printDouble(double val) {
  outs() << val;
}

extern "C" void lle_X_printInt(int val) {
  outs() << val;
}

extern "C" void lle_X_printChar(char val) {
  outs() << val;
}

extern "C" void lle_X_printString(char* str) {
  outs() << str;
}

extern "C" void lle_X_printLine(char* str) {
  outs() << "\n";
}


FuncDeclAST* addDynamicFunc(const char* protoString, const char* newName, 
  ModuleDeclAST* modDecl, void* fncPtr) {
  FuncDeclAST* func = (FuncDeclAST*)parseFuncProto(protoString);
  func->CodeValue = Function::Create(
    (FunctionType*)func->ThisType->getType(),
    Function::ExternalLinkage,
    Twine(newName),
    getSLContext().TheModule
  );
  func->Compiled = true;
  modDecl->Members.insert(modDecl->Members.begin(), func);

  ExitOnError ExitOnErr;

  ExitOnErr(getJIT().addSymbol(newName, fncPtr));

  return func;
}

void initRuntimeFuncs(ModuleDeclAST* modDecl) {
  addDynamicFunc("fn eclay(_: char)", "lle_X_printChar", modDecl, (void*)lle_X_printChar);
  addDynamicFunc("fn eclay(_: int)", "lle_X_printInt", modDecl, (void*)lle_X_printInt);
  addDynamicFunc("fn eclay(_: float)", "lle_X_printDouble", modDecl, (void*)lle_X_printDouble);
  addDynamicFunc("fn eclay(_: string)", "lle_X_printString", modDecl, (void*)lle_X_printString);
  addDynamicFunc("fn eclamaticPause()", "lle_X_printLine", modDecl, (void*)lle_X_printLine);
}

ModuleDeclAST* ModuleDeclAST::load(SourceMgr &SrcMgr, DiagnosticsEngine &Diags, StringRef fileName) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
    FileOrErr = llvm::MemoryBuffer::getFile(fileName);

  if (std::error_code BufferError = FileOrErr.getError()) {
    llvm::WithColor::error(llvm::errs(), "eclair")
      << "Error reading " << fileName << ": "
      << BufferError.message() << "\n";
  }

  SrcMgr.AddNewSourceBuffer(std::move(*FileOrErr),
                            llvm::SMLoc());

  Lexer Lex(SrcMgr, Diags);
  Parser P(&Lex);

  return P.parseModule();
}

void ModuleDeclAST::semantic() {
  Scope s(this);
  
  initRuntimeFuncs(this);

  for (int i = TypeAST::TI_Void; i <= TypeAST::TI_Float; ++i) {
    BuiltinTypeAST::get(i)->semantic(&s);
  }

  for (SymbolList::iterator it = Members.begin(), end = Members.end();
    it != end; ++it) {
    if (!isa<FuncDeclAST>(*it))
      (*it)->semantic(&s);
  }

  for (SymbolList::iterator it = Members.begin(), end = Members.end();
    it != end; ++it) {
    if (isa<FuncDeclAST>(*it))
      (*it)->semantic(&s);
  }

  for (SymbolList::iterator it = Members.begin(), end = Members.end();
    it != end; ++it) {
    (*it)->semantic2(&s);
  }

  for (SymbolList::iterator it = Members.begin(), end = Members.end();
    it != end; ++it) {
    (*it)->semantic3(&s);
  }

  for (SymbolList::iterator it = Members.begin(), end = Members.end();
    it != end; ++it) {
    (*it)->semantic4(&s);
  }

  for (SymbolList::iterator it = Members.begin(), end = Members.end();
    it != end; ++it) {
    (*it)->semantic5(&s);
  }
}

SymbolAST* Scope::find(Name* id) {
  Scope* s = this;

  if (!id) {
    return ThisModule;
  }

  for ( ; s; s = s->Enclosed) {
    if (s->CurScope) {
      SymbolAST* sym = s->CurScope->find(id);

      if (sym) {
        return sym;
      }
    }
  }

  return nullptr;
}

SymbolAST* Scope::findMember(Name* id, int flags) {
  if (CurScope) {
    return CurScope->find(id, flags);
  }

  return nullptr;
}

Scope* Scope::push() {
  Scope* s = new Scope(this);
  return s;
}

Scope* Scope::push(ScopeSymbol* sym) {
  Scope* s = push();
  s->CurScope = sym;
  return s;
}

Scope* Scope::pop() {
  Scope* res = Enclosed;
  delete this;
  return res;
}

Scope* Scope::recreateScope(Scope* scope, SymbolAST* sym) {
  Scope* p = scope;

  while (p->Enclosed) {
    p = p->Enclosed;
  }


  SymbolList symList;
  SymbolAST* tmp = sym;

  while (tmp->Parent) {
    symList.push_back(tmp);
    tmp = tmp->Parent;
  }

  for (SymbolList::reverse_iterator it = symList.rbegin(), end = symList.rend();
    it != end; ++it) {
    p = p->push((ScopeSymbol*)*it);
  }

  return p;
}

void Scope::clearAllButModule(Scope* scope) {
  Scope* p = scope;

  while (p->Enclosed) {
    p = p->pop();
  }
}

}
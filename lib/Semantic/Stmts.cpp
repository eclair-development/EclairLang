#include "eclair/AST/AST.h"

using namespace llvm;

namespace eclair {

bool StmtAST::hasReturn() {
  return false;
}

bool StmtAST::hasJump() {
  return false;
}

StmtAST* StmtAST::semantic(Scope* scope) {
  if (SemaState > 0) {
    return this;
  }

  ++SemaState;
  return doSemantic(scope);
}

StmtAST* StmtAST::doSemantic(Scope* ) {
  assert(0 && "StmtAST::semantic should never be reached");
  return this;
}

StmtAST* ExprStmtAST::doSemantic(Scope* scope) {
  if (Expr) {
    Expr = Expr->semantic(scope);

    if (!Expr->ExprType) {
      scope->report(Loc, diag::ERR_SemaNoTypeForExpression);
      return nullptr;
    }
  }

  return this;
}

bool BlockStmtAST::hasReturn() {
  return HasReturn;
}

bool BlockStmtAST::hasJump() {
  return HasJump;
}

StmtAST* BlockStmtAST::doSemantic(Scope* scope) {
  ThisBlock = new ScopeSymbol(Loc, SymbolAST::SI_Block, nullptr);
  Scope* s = scope->push((ScopeSymbol*)ThisBlock);
  
  LandingPad = new LandingPadAST(s->LandingPad);
  LandingPad->OwnerBlock = this;
  s->LandingPad = LandingPad;

  ExprList args;

  for (StmtList::iterator it = Body.begin(), end = Body.end(); it != end; ++it) {
    if (HasJump) {
      scope->report(Loc, diag::ERR_SemaDeadCode);
      return nullptr;
    }

    *it = (*it)->semantic(s);

    if ((*it)->isJump()) {
      HasJump = true;

      if ((*it)->hasReturn()) {
        HasReturn = true;
      }
    } else {

      HasJump = (*it)->hasJump();
      HasReturn = (*it)->hasReturn();
    }
  }

  s->pop();

  return this;
}

StmtAST* DeclStmtAST::doSemantic(Scope* scope) {
  for (SymbolList::iterator it = Decls.begin(), end = Decls.end(); it != end; ++it) {
    (*it)->semantic(scope);
    (*it)->semantic2(scope);
    (*it)->semantic3(scope);
    (*it)->semantic4(scope);
    (*it)->semantic5(scope);
  }

  return this;
}

bool BreakStmtAST::hasJump() {
  return true;
}

StmtAST* BreakStmtAST::doSemantic(Scope* scope) {
  if (!scope->BreakLoc) {
    scope->report(Loc, diag::ERR_SemaInvalidJumpStatement);
    return nullptr;
  }

  BreakLoc = scope->LandingPad;
  ++BreakLoc->Breaks;
  return this;
}

bool ContinueStmtAST::hasJump() {
  return true;
}

StmtAST* ContinueStmtAST::doSemantic(Scope* scope) {
  if (!scope->ContinueLoc) {
    scope->report(Loc, diag::ERR_SemaInvalidJumpStatement);
    return nullptr;
  }

  ContinueLoc = scope->LandingPad;
  ++ContinueLoc->Continues;
  return this;
}

bool ReturnStmtAST::hasReturn() {
  return true;
}

bool ReturnStmtAST::hasJump() {
  return true;
}

StmtAST* ReturnStmtAST::doSemantic(Scope* scope) {
  assert(scope->LandingPad);
  ReturnLoc = scope->LandingPad;
  ++ReturnLoc->Returns;

  if (Expr) {
    if (!scope->EnclosedFunc->ReturnType || scope->EnclosedFunc->ReturnType->isVoid()) {
      scope->report(Loc, diag::ERR_SemaReturnValueInVoidFunction);
      return nullptr;
    }

    Expr = Expr->semantic(scope);

    if (!scope->EnclosedFunc->ReturnType->equal(Expr->ExprType)) {
      Expr = new CastExprAST(Loc, Expr, scope->EnclosedFunc->ReturnType);
      Expr = Expr->semantic(scope);
    }

    return this;
  }

  if (scope->EnclosedFunc->ReturnType && !scope->EnclosedFunc->ReturnType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaReturnVoidFromFunction);
    return nullptr;
  }

  return this;
}

bool WhileStmtAST::hasReturn() {
  return false;
}

StmtAST* WhileStmtAST::doSemantic(Scope* scope) {
  Cond = Cond->semantic(scope);

  if (!Cond->ExprType || Cond->ExprType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaConditionIsVoid);
    return nullptr;
  }

  if (!Cond->ExprType->implicitConvertTo(BuiltinTypeAST::get(TypeAST::TI_Bool))) {
    scope->report(Loc, diag::ERR_SemaCantConvertToBoolean);
    return nullptr;
  }

  StmtAST* oldBreak = scope->BreakLoc;
  StmtAST* oldContinue = scope->ContinueLoc;

  scope->BreakLoc = this;
  scope->ContinueLoc = this;
  LandingPad = new LandingPadAST(scope->LandingPad);
  LandingPad->IsLoop = true;
  scope->LandingPad = LandingPad;

  Body = Body->semantic(scope);

  scope->BreakLoc = oldBreak;
  scope->ContinueLoc = oldContinue;
  scope->LandingPad = LandingPad->Prev;

  if (PostExpr) {
    PostExpr = PostExpr->semantic(scope);
  }

  return this;
}

StmtAST* ForStmtAST::doSemantic(Scope* scope) {
  StmtAST* init = nullptr;


  if (InitExpr) {
    init = new ExprStmtAST(Loc, InitExpr);
  } else if (!InitDecls.empty()) {
    init = new DeclStmtAST(Loc, InitDecls);
  }

  if (Cond) {
    StmtList stmts;

    if (init) {
      stmts.push_back(init);
    }

    WhileStmtAST* newLoop = new WhileStmtAST(Loc, Cond, Body);
    newLoop->PostExpr = Post;
    stmts.push_back(newLoop);

    InitExpr = nullptr;
    InitDecls.clear();
    Cond = nullptr;
    Post = nullptr;
    Body = nullptr;
    delete this;

    StmtAST* res = new BlockStmtAST(Loc, stmts);
    return res->semantic(scope);
  } else {
    StmtList stmts;

    if (init) {
      stmts.push_back(init);
    }

    WhileStmtAST* newLoop = new WhileStmtAST(Loc, new IntExprAST(Loc, 1), Body);
    newLoop->PostExpr = Post;
    stmts.push_back(newLoop);

    InitExpr = nullptr;
    InitDecls.clear();
    Cond = nullptr;
    Post = nullptr;
    Body = nullptr;
    delete this;

    StmtAST* res = new BlockStmtAST(Loc, stmts);
    return res->semantic(scope);
  }
}

bool IfStmtAST::hasReturn() {
  if (!ElseBody) {
    return false;
  }

  return ThenBody->hasReturn() && ElseBody->hasReturn();
}

bool IfStmtAST::hasJump() {
  if (!ElseBody) {
    return false;
  }

  return ThenBody->hasJump() && ElseBody->hasJump();
}

StmtAST* IfStmtAST::doSemantic(Scope* scope) {
  Cond = Cond->semantic(scope);

  if (!Cond->ExprType || Cond->ExprType == BuiltinTypeAST::get(TypeAST::TI_Void)) {
    scope->report(Loc, diag::ERR_SemaConditionIsVoid);
    return nullptr;
  }

  if (!Cond->ExprType->implicitConvertTo(BuiltinTypeAST::get(TypeAST::TI_Bool))) {
    scope->report(Loc, diag::ERR_SemaCantConvertToBoolean);
    return nullptr;
  }

  LandingPad = new LandingPadAST(scope->LandingPad);
  scope->LandingPad = LandingPad;

  ThenBody = ThenBody->semantic(scope);

  if (ElseBody) {
    ElseBody = ElseBody->semantic(scope);
  }

  scope->LandingPad = LandingPad->Prev;

  return this;
}

}
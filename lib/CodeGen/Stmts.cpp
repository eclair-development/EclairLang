#include "eclair/AST/AST.h"

using namespace llvm;

namespace eclair {

Value* StmtAST::generateCode(SLContext& Context) {
  assert(0 && "StmtAST::generateCode should never be reached");
  return nullptr;
}

Value* ExprStmtAST::generateCode(SLContext& Context) {
  if (Expr) {
    return Expr->getRValue(Context);
  }
  
  return nullptr;
}

llvm::Value* BlockStmtAST::generatePartialCode(SLContext& Context, 
  StmtList::iterator it, StmtList::iterator end) {
  LandingPadAST* parent = LandingPad->Prev;
  LandingPad->BreakLoc = parent->BreakLoc;
  LandingPad->ContinueLoc = parent->ContinueLoc;
  LandingPad->ReturnLoc = parent->ReturnLoc;

  LandingPad->FallthroughLoc = BasicBlock::Create(getGlobalContext(), "block");

  for (; it != end; ++it) {
    BasicBlock* fallthroughBB = LandingPad->FallthroughLoc;

    (*it)->generateCode(Context);

    if (!LandingPad->FallthroughLoc) {
      Context.TheFunction->getBasicBlockList().push_back(fallthroughBB);
      Context.TheBuilder->SetInsertPoint(fallthroughBB);

      LandingPad->FallthroughLoc = BasicBlock::Create(getGlobalContext(), "block");
    }
  }

  parent->Breaks += LandingPad->Breaks;
  parent->Continues += LandingPad->Continues;
  parent->Returns += LandingPad->Returns;

  if (!Context.TheBuilder->GetInsertBlock()->getTerminator()) {  
    Context.TheBuilder->CreateBr(parent->FallthroughLoc);
    parent->FallthroughLoc = nullptr;
    
    if (!LandingPad->FallthroughLoc->hasNUsesOrMore(1)) {
      delete LandingPad->FallthroughLoc;
    }
  } else if (LandingPad->FallthroughLoc->hasNUsesOrMore(1)) {
    Context.TheFunction->getBasicBlockList().push_back(LandingPad->FallthroughLoc);
    Context.TheBuilder->SetInsertPoint(LandingPad->FallthroughLoc);
    Context.TheBuilder->CreateBr(parent->FallthroughLoc);
    parent->FallthroughLoc = nullptr;
  } else {
    delete LandingPad->FallthroughLoc;
  }

  return nullptr;
}

Value* BlockStmtAST::generateCode(SLContext& Context) {
  return generatePartialCode(Context, Body.begin(), Body.end());
}

Value* DeclStmtAST::generateCode(SLContext& Context) {
  for (SymbolList::iterator it = Decls.begin(), end = Decls.end(); it != end; ++it) {
    (*it)->generateCode(Context);
  }

  return nullptr;
}

Value* LandingPadAST::getReturnValue() {
  LandingPadAST* prev = this;

  while (prev->Prev) {
    prev = prev->Prev;
  }

  return prev->ReturnValue;
}

Value* BreakStmtAST::generateCode(SLContext& Context) {
  Context.TheBuilder->CreateBr(BreakLoc->BreakLoc);
  return nullptr;
}

Value* ContinueStmtAST::generateCode(SLContext& Context) {
  Context.TheBuilder->CreateBr(ContinueLoc->ContinueLoc);
  return nullptr;
}

Value* ReturnStmtAST::generateCode(SLContext& Context) {
  if (Expr) {
    Value* retVal = Expr->getRValue(Context);
    Context.TheBuilder->CreateStore(retVal, ReturnLoc->getReturnValue());
    Context.TheBuilder->CreateBr(ReturnLoc->ReturnLoc);
    return nullptr;
  }

  Context.TheBuilder->CreateBr(ReturnLoc->ReturnLoc);
  return nullptr;
}

Value* WhileStmtAST::generateCode(SLContext& Context) {
  LandingPadAST* prev = LandingPad->Prev;
  LandingPad->ReturnLoc = prev->ReturnLoc;

  if (Cond->isConst()) {
    if (!Cond->isTrue()) {
      return nullptr;
    }

    BasicBlock* bodyBB = LandingPad->ContinueLoc = BasicBlock::Create(getGlobalContext(), 
      "loopbody", Context.TheFunction);
    BasicBlock* endBB = LandingPad->BreakLoc = BasicBlock::Create(getGlobalContext(), 
      "loopend");

    if (PostExpr) {
      LandingPad->ContinueLoc = BasicBlock::Create(getGlobalContext(), "postbody");
    }

    Context.TheBuilder->CreateBr(bodyBB);
    Context.TheBuilder->SetInsertPoint(bodyBB);

    if (PostExpr) {
      LandingPad->FallthroughLoc = LandingPad->ContinueLoc;
    } else {
      LandingPad->FallthroughLoc = bodyBB;
    }

    Body->generateCode(Context);

    if (PostExpr) {
      Context.TheFunction->getBasicBlockList().push_back(LandingPad->ContinueLoc);
      Context.TheBuilder->SetInsertPoint(LandingPad->ContinueLoc);

      PostExpr->getRValue(Context);
      Context.TheBuilder->CreateBr(bodyBB);
    }

    Context.TheFunction->getBasicBlockList().push_back(endBB);
    Context.TheBuilder->SetInsertPoint(endBB);
    prev->Returns += LandingPad->Returns;

    Context.TheBuilder->CreateBr(prev->FallthroughLoc);
    prev->FallthroughLoc = nullptr;

    return nullptr;
  }
  
  BasicBlock* condBB = LandingPad->ContinueLoc = BasicBlock::Create(getGlobalContext(), 
    "loopcond", Context.TheFunction);
  BasicBlock* bodyBB = BasicBlock::Create(getGlobalContext(), "loopbody");
  BasicBlock* endBB = LandingPad->BreakLoc = BasicBlock::Create(getGlobalContext(), "loopend");

  if (PostExpr) {
    LandingPad->ContinueLoc = BasicBlock::Create(getGlobalContext(), "postbody");
  }

  Context.TheBuilder->CreateBr(condBB);
  Context.TheBuilder->SetInsertPoint(condBB);

  Value* cond = Cond->getRValue(Context);
  cond = promoteToBool(cond, Cond->ExprType, *Context.TheBuilder);
  Context.TheBuilder->CreateCondBr(cond, bodyBB, endBB);

  Context.TheFunction->getBasicBlockList().push_back(bodyBB);
  Context.TheBuilder->SetInsertPoint(bodyBB);

  if (PostExpr) {
    LandingPad->FallthroughLoc = LandingPad->ContinueLoc;
  } else {
    LandingPad->FallthroughLoc = condBB;
  }

  Body->generateCode(Context);

  if (PostExpr) {
    Context.TheFunction->getBasicBlockList().push_back(LandingPad->ContinueLoc);
    Context.TheBuilder->SetInsertPoint(LandingPad->ContinueLoc);

    PostExpr->getRValue(Context);
    Context.TheBuilder->CreateBr(condBB);
  }

  Context.TheFunction->getBasicBlockList().push_back(endBB);
  Context.TheBuilder->SetInsertPoint(endBB);
  prev->Returns += LandingPad->Returns;

  Context.TheBuilder->CreateBr(prev->FallthroughLoc);
  prev->FallthroughLoc = nullptr;

  return nullptr;
}

llvm::Value* ForStmtAST::generateCode(SLContext& Context) {
  assert(0 && "ForStmtAST::semantic should never be reached");
  return nullptr;
}


llvm::Value* IfStmtAST::generateCode(SLContext& Context) {
  LandingPadAST* prev = LandingPad->Prev;
  LandingPad->ReturnLoc = prev->ReturnLoc;
  LandingPad->FallthroughLoc = prev->FallthroughLoc;
  LandingPad->ContinueLoc = prev->ContinueLoc;
  LandingPad->BreakLoc = prev->BreakLoc;

  if (Cond->isConst()) {
    if (Cond->isTrue()) {
      ThenBody->generateCode(Context);

      prev->Returns += LandingPad->Returns;
      prev->Breaks += LandingPad->Breaks;
      prev->Continues += LandingPad->Continues;

      if (!Context.TheBuilder->GetInsertBlock()->getTerminator()) {
        Context.TheBuilder->CreateBr(prev->FallthroughLoc);
        prev->FallthroughLoc = nullptr;
      }

      return nullptr;
    }
      
    if (ElseBody) {
      ElseBody->generateCode(Context);

      prev->Returns += LandingPad->Returns;
      prev->Breaks += LandingPad->Breaks;
      prev->Continues += LandingPad->Continues;

      if (!Context.TheBuilder->GetInsertBlock()->getTerminator()) {
        Context.TheBuilder->CreateBr(prev->FallthroughLoc);
        prev->FallthroughLoc = nullptr;
      }
    }

    return nullptr;
  }
   
  Value* cond = Cond->getRValue(Context);
  cond = promoteToBool(cond, Cond->ExprType, *Context.TheBuilder);

  BasicBlock* thenBB = BasicBlock::Create(getGlobalContext(), "thenpart", 
    Context.TheFunction);
  BasicBlock* elseBB = nullptr;
  BasicBlock* endBB = BasicBlock::Create(getGlobalContext(), "ifcont");

  if (ElseBody) {
    elseBB = BasicBlock::Create(getGlobalContext(), "elsepart");
    Context.TheBuilder->CreateCondBr(cond, thenBB, elseBB);
  } else {
    Context.TheBuilder->CreateCondBr(cond, thenBB, endBB);
  }

  Context.TheBuilder->SetInsertPoint(thenBB);
  
  LandingPad->FallthroughLoc = endBB;

  ThenBody->generateCode(Context);

  LandingPad->FallthroughLoc = endBB;

  if (ElseBody) {
    Context.TheFunction->getBasicBlockList().push_back(elseBB);
    Context.TheBuilder->SetInsertPoint(elseBB);

    ElseBody->generateCode(Context);
  }

  if (endBB->hasNUsesOrMore(1)) {
    Context.TheFunction->getBasicBlockList().push_back(endBB);
    Context.TheBuilder->SetInsertPoint(endBB);

    Context.TheBuilder->CreateBr(prev->FallthroughLoc);
    prev->FallthroughLoc = nullptr;
  } else {
    delete endBB;
  }

  prev->Returns += LandingPad->Returns;
  prev->Breaks += LandingPad->Breaks;
  prev->Continues += LandingPad->Continues;

  return nullptr;
}

}
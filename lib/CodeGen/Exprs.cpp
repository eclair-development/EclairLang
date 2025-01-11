#include "eclair/AST/AST.h"

using namespace llvm;

namespace eclair {

// # 1
ConstantInt* getConstInt(uint64_t value) {
  return ConstantInt::get(Type::getInt32Ty(getGlobalContext()), value, true);
}

// # 2 
Value* promoteToBool(Value* val, TypeAST* type, IRBuilder< >& builder) {
  if (type == BuiltinTypeAST::get(TypeAST::TI_Bool)) {
    return val;
  }

  if (type->isInt()) {
    return builder.CreateICmpNE(val, ConstantInt::get(type->getType(), 0));
  } else {
    assert(type->isFloat());
    return builder.CreateFCmpUNE(val, ConstantFP::get(
      Type::getDoubleTy(getGlobalContext()), 0.0));
  }
}

// # 3
Value* ExprAST::getLValue(SLContext& ) {
  assert(0 && "");
  return nullptr;
}

// # 4
Value* IntExprAST::getRValue(SLContext& ) {
  return ConstantInt::get(ExprType->getType(), Val, true);
}

// # 5
Value* FloatExprAST::getRValue(SLContext& ) {
  return ConstantFP::get(ExprType->getType(), Val);
}

// # 6
Value* IdExprAST::getLValue(SLContext& Context) {
  return ThisSym->getValue(Context);
}

Value* IdExprAST::getRValue(SLContext& Context) {
  if (isa<FuncDeclAST>(ThisSym)) {
    return ThisSym->getValue(Context);
  }

  return Context.TheBuilder->CreateLoad(
    ExprType->getType(),
    ThisSym->getValue(Context), 
    StringRef(Val->Id, Val->Length)
  );
}

// # 7
Value* CastExprAST::getRValue(SLContext& Context) {
  if (ExprType->isInt()) {
    if (Val->ExprType->isBool()) {
      return Context.TheBuilder->CreateZExt(Val->getRValue(Context), 
        ExprType->getType());
    }

    assert(Val->ExprType->isFloat());
    return Context.TheBuilder->CreateFPToSI(Val->getRValue(Context), 
      ExprType->getType());
  }

  if (ExprType->isBool()) {
    return promoteToBool(Val->getRValue(Context), Val->ExprType, 
      *Context.TheBuilder);
  } else if (Val->ExprType->isInt()) {
    return Context.TheBuilder->CreateSIToFP(Val->getRValue(Context), 
      ExprType->getType());
  } else if (Val->ExprType->isBool()) {
    return Context.TheBuilder->CreateUIToFP(Val->getRValue(Context),
      ExprType->getType());
  }

  assert(0 && "should never be reached");
  return nullptr;
}

// # 8
Value* UnaryExprAST::getRValue(SLContext& Context) {
  assert(0 && "Should never happen");
  return nullptr;
}

Value* BinaryExprAST::getRValue(SLContext& Context) {

  if (Op == tok::Assign) {
    
    Value* right = RightExpr->getRValue(Context);
    Value* res = LeftExpr->getLValue(Context);

    Context.TheBuilder->CreateStore(right, res);
    return right;
  }

  if (Op == tok::Comma) {
    LeftExpr->getRValue(Context);
    Value* rhs = RightExpr->getRValue(Context);

    return rhs;
  }

  if (Op == tok::PlusPlus || Op == tok::MinusMinus) {
    Value* var = LeftExpr->getLValue(Context);
    Value* val = LeftExpr->getRValue(Context);

    if (Op == tok::PlusPlus) {
      Value* tmp = getConstInt(1);
      tmp = Context.TheBuilder->CreateAdd(val, tmp, "inctmp");
      Context.TheBuilder->CreateStore(tmp, var);
      return val;
    } else {
      Value* tmp = getConstInt(~0ULL);
      tmp = Context.TheBuilder->CreateAdd(val, tmp, "dectmp");
      Context.TheBuilder->CreateStore(tmp, var);
      return val;
    }
  }

  if (Op == tok::LogOr) {

    Value* lhs = LeftExpr->getRValue(Context);
    lhs = promoteToBool(lhs, LeftExpr->ExprType, *Context.TheBuilder);

    BasicBlock* condBB = Context.TheBuilder->GetInsertBlock();
    BasicBlock* resultBB = BasicBlock::Create(getGlobalContext(), "result");
    BasicBlock* falseBB = BasicBlock::Create(getGlobalContext(), "false");


    Context.TheBuilder->CreateCondBr(lhs, resultBB, falseBB);

    Context.TheFunction->getBasicBlockList().push_back(falseBB);

    Context.TheBuilder->SetInsertPoint(falseBB);
    Value* rhs = RightExpr->getRValue(Context);
    falseBB = Context.TheBuilder->GetInsertBlock();

    rhs = promoteToBool(rhs, RightExpr->ExprType, *Context.TheBuilder);

    Context.TheBuilder->CreateBr(resultBB);

    Context.TheFunction->getBasicBlockList().push_back(resultBB);
    Context.TheBuilder->SetInsertPoint(resultBB);

    PHINode* PN = Context.TheBuilder->CreatePHI(Type::getInt1Ty(getGlobalContext()), 2);

    PN->addIncoming(lhs, condBB);
    PN->addIncoming(rhs, falseBB);

    return PN;
  }

  if (Op == tok::LogAnd) {
    Value* lhs = LeftExpr->getRValue(Context);
    lhs = promoteToBool(lhs, LeftExpr->ExprType, *Context.TheBuilder);

    BasicBlock* condBB = Context.TheBuilder->GetInsertBlock();
    BasicBlock* resultBB = BasicBlock::Create(getGlobalContext(), "result");
    BasicBlock* trueBB = BasicBlock::Create(getGlobalContext(), "true");

    Context.TheBuilder->CreateCondBr(lhs, trueBB, resultBB);

    Context.TheFunction->getBasicBlockList().push_back(trueBB);
    Context.TheBuilder->SetInsertPoint(trueBB);

    Value* rhs = RightExpr->getRValue(Context);
    trueBB = Context.TheBuilder->GetInsertBlock();
    rhs = promoteToBool(rhs, RightExpr->ExprType, *Context.TheBuilder);
    Context.TheBuilder->CreateBr(resultBB);

    Context.TheFunction->getBasicBlockList().push_back(resultBB);
    Context.TheBuilder->SetInsertPoint(resultBB);

    PHINode* PN = Context.TheBuilder->CreatePHI(Type::getInt1Ty(getGlobalContext()), 2);

    PN->addIncoming(lhs, condBB);
    PN->addIncoming(rhs, trueBB);

    return PN;
  }

  Value* lhs = LeftExpr->getRValue(Context);
  Value* rhs = RightExpr->getRValue(Context);

  if (LeftExpr->ExprType == BuiltinTypeAST::get(TypeAST::TI_Int)) {
    switch (Op) {
      case tok::Plus: return Context.TheBuilder->CreateAdd(lhs, rhs, "addtmp");
      case tok::Minus: return Context.TheBuilder->CreateSub(lhs, rhs, "subtmp");
      case tok::Mul: return Context.TheBuilder->CreateMul(lhs, rhs, "multmp");
      case tok::Div: return Context.TheBuilder->CreateSDiv(lhs, rhs, "divtmp");
      case tok::Mod: return Context.TheBuilder->CreateSRem(lhs, rhs, "remtmp");
      case tok::BitOr: return Context.TheBuilder->CreateOr(lhs, rhs, "ortmp");
      case tok::BitAnd: return Context.TheBuilder->CreateAnd(lhs, rhs, "andtmp");
      case tok::BitXor: return Context.TheBuilder->CreateXor(lhs, rhs, "xortmp");
      case tok::LShift: return Context.TheBuilder->CreateShl(lhs, rhs, "shltmp");
      case tok::RShift: return Context.TheBuilder->CreateAShr(lhs, rhs, "shrtmp");
      case tok::Less: return Context.TheBuilder->CreateICmpSLT(lhs, rhs, "cmptmp");
      case tok::Greater: return Context.TheBuilder->CreateICmpSGT(lhs, rhs, "cmptmp");
      case tok::LessEqual: return Context.TheBuilder->CreateICmpSLE(lhs, rhs, "cmptmp");
      case tok::GreaterEqual: return Context.TheBuilder->CreateICmpSGE(lhs, rhs, "cmptmp");
      case tok::Equal: return Context.TheBuilder->CreateICmpEQ(lhs, rhs, "cmptmp");
      case tok::NotEqual: return Context.TheBuilder->CreateICmpNE(lhs, rhs, "cmptmp");
      default: assert(0 && "Invalid integral binary operator"); return nullptr;
    }
  }
  
  switch (Op) {
    case tok::Plus: return Context.TheBuilder->CreateFAdd(lhs, rhs, "addtmp");
    case tok::Minus: return Context.TheBuilder->CreateFSub(lhs, rhs, "subtmp");
    case tok::Mul: return Context.TheBuilder->CreateFMul(lhs, rhs, "multmp");
    case tok::Div: return Context.TheBuilder->CreateFDiv(lhs, rhs, "divtmp");
    case tok::Mod: return Context.TheBuilder->CreateFRem(lhs, rhs, "remtmp");
    case tok::Less: return Context.TheBuilder->CreateFCmpULT(lhs, rhs, "cmptmp");
    case tok::Greater: return Context.TheBuilder->CreateFCmpUGT(lhs, rhs, "cmptmp");
    case tok::LessEqual: return Context.TheBuilder->CreateFCmpULE(lhs, rhs, "cmptmp");
    case tok::GreaterEqual: return Context.TheBuilder->CreateFCmpUGE(lhs, rhs, "cmptmp");
    case tok::Equal: return Context.TheBuilder->CreateFCmpUEQ(lhs, rhs, "cmptmp");
    case tok::NotEqual: return Context.TheBuilder->CreateFCmpUNE(lhs, rhs, "cmptmp");
    default: assert(0 && "Invalid floating point binary operator"); return nullptr;
  }
}

// # 9
Value* generateCondExpr(SLContext& Context, ExprAST* Cond, ExprAST* IfExpr, 
  ExprAST* ElseExpr, bool isLValue) {
  Value* cond = Cond->getRValue(Context);
  cond = promoteToBool(cond, Cond->ExprType, *Context.TheBuilder);

  BasicBlock* thenBB = BasicBlock::Create(getGlobalContext(), "then", 
    Context.TheFunction);
  BasicBlock* elseBB = BasicBlock::Create(getGlobalContext(), "else");
  BasicBlock* mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

  Context.TheBuilder->CreateCondBr(cond, thenBB, elseBB);
  Context.TheBuilder->SetInsertPoint(thenBB);

  Value* thenValue;
  
  if (isLValue) {
    thenValue = IfExpr->getLValue(Context);
  } else {
    thenValue = IfExpr->getRValue(Context);
  }

  Context.TheBuilder->CreateBr(mergeBB);
  thenBB = Context.TheBuilder->GetInsertBlock();

  Context.TheFunction->getBasicBlockList().push_back(elseBB);
  Context.TheBuilder->SetInsertPoint(elseBB);

  Value* elseValue;
  
  if (isLValue) {
    elseValue = ElseExpr->getLValue(Context);
  } else {
    elseValue = ElseExpr->getRValue(Context);
  }

  Context.TheBuilder->CreateBr(mergeBB);
  elseBB = Context.TheBuilder->GetInsertBlock();

  Context.TheFunction->getBasicBlockList().push_back(mergeBB);
  Context.TheBuilder->SetInsertPoint(mergeBB);

  if (!IfExpr->ExprType || IfExpr->ExprType->isVoid()) {
    return nullptr;
  }

  PHINode* PN;
  
  if (isLValue) {
    PN = Context.TheBuilder->CreatePHI(
      PointerType::get(
        getGlobalContext(),
        Context.TheTarget->getProgramAddressSpace()
      ),
      2
    );
  } else {
    PN = Context.TheBuilder->CreatePHI(IfExpr->ExprType->getType(), 2);
  }

  PN->addIncoming(thenValue, thenBB);
  PN->addIncoming(elseValue, elseBB);
  
  return PN;
}

// # 10
Value* CondExprAST::getLValue(SLContext& Context) {
  return generateCondExpr(Context, Cond, IfExpr, ElseExpr, true);
}

Value* CondExprAST::getRValue(SLContext& Context) {
  return generateCondExpr(Context, Cond, IfExpr, ElseExpr, false);
}

// # 11
Value* CallExprAST::getRValue(SLContext& Context) {
  Value* callee = nullptr;
  std::vector< Value* > args;
  ExprList::iterator it = Args.begin();

  assert(isa<FuncDeclAST>(CallFunc));
  FuncDeclAST* funcDecl = (FuncDeclAST*)CallFunc;
  Type* funcRawType = CallFunc->getType()->getType();
  assert(isa<FunctionType>(funcRawType));
  FunctionType* funcType = static_cast<FunctionType*>(funcRawType);

  callee = CallFunc->getValue(Context);

  for (ExprList::iterator end = Args.end(); it != end; ++it) {
    Value* v = (*it)->getRValue(Context);
    args.push_back(v);
  }
  
  return Context.TheBuilder->CreateCall(funcType, callee, args);
}

}
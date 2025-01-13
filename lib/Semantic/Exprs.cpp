#include "eclair/AST/AST.h"

using namespace llvm;

namespace eclair {

bool ExprAST::isTrue() { 
  return false; 
}

bool ExprAST::isLValue() {
  return false;
}

bool IntExprAST::isTrue() {
  return Val != 0;
}

ExprAST* IntExprAST::semantic(Scope* ) {
  return this;
}

ExprAST* IntExprAST::clone() {
  return new IntExprAST(Loc, Val);
}

bool FloatExprAST::isTrue() {
  return Val != 0.0;
}

ExprAST* FloatExprAST::semantic(Scope* ) {
  return this;
}

ExprAST* FloatExprAST::clone() {
  return new FloatExprAST(Loc, Val);
}

bool IdExprAST::isLValue() {
  return true;
}

ExprAST* IdExprAST::semantic(Scope* scope) {
  if (!ThisSym) {
    ThisSym = scope->find(Val);

    if (!Val) {
      return this;
    }

    if (!ThisSym) {
      scope->report(Loc, diag::ERR_SemaUndefinedIdentifier, Val->Id);
      return nullptr;
    }

    ExprType = ThisSym->getType();
  }

  return this;
}

ExprAST* IdExprAST::clone() {
  return new IdExprAST(Loc, Val);
}

ExprAST* CastExprAST::semantic(Scope* scope) {
  if (SemaDone) {
    return this;
  }

  assert(ExprType != nullptr && "Type for cast not set"); 
  if (ExprType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaCastToVoid);
    return nullptr;
  }

  Val = Val->semantic(scope);

  if (!Val->ExprType || Val->ExprType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaCastToVoid);
    return nullptr;
  }

  if (isa<FuncTypeAST>(ExprType) || isa<FuncTypeAST>(Val->ExprType)) {
    scope->report(Loc, diag::ERR_SemaFunctionInCast);
    return nullptr;
  }

  if (!Val->ExprType->implicitConvertTo(ExprType)) {
    scope->report(Loc, diag::ERR_SemaInvalidCast);
    return nullptr;
  }

  SemaDone = true;
  return this;
}

ExprAST* CastExprAST::clone() {
  return new CastExprAST(Loc, Val->clone(), ExprType);
}

ExprAST* UnaryExprAST::semantic(Scope* scope) {
  if (!Val) {
    assert(0 && "Invalid expression value");
    return nullptr;
  }

  Val = Val->semantic(scope);

  if (!Val->ExprType || Val->ExprType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaOperandIsVoid);
    return nullptr;
  }


  if (Val->ExprType->isBool() && (Op == tok::Plus || Op == tok::Minus)) {
    scope->report(Loc, diag::ERR_SemaInvalidBoolForUnaryOperands);
    return nullptr;
  }

  ExprAST* result = this;

  switch (Op) {
    case tok::Plus: result = Val; break;

    case tok::Minus: 
      if (Val->ExprType->isFloat()) {
        result = new BinaryExprAST(Val->Loc, tok::Minus, new FloatExprAST(Val->Loc, 0), Val);
      } else {
        result = new BinaryExprAST(Val->Loc, tok::Minus, new IntExprAST(Val->Loc, 0), Val);
      }
      break;

    case tok::Tilda:
      if (!Val->ExprType->isInt()) {
        scope->report(Loc, diag::ERR_SemaInvalidOperandForComplemet);
        return nullptr;
      } else {
        result = new BinaryExprAST(Val->Loc, tok::BitXor, Val, new IntExprAST(Val->Loc, -1));
        break;
      }

    case tok::PlusPlus:
    case tok::MinusMinus: {
        if (!Val->ExprType->isInt() || !Val->isLValue()) {
          scope->report(Loc, diag::ERR_SemaInvalidPostfixPrefixOperand);
          return nullptr;
        }
        
        ExprAST* val = Val;
        ExprAST* valCopy = Val->clone();
        result = new BinaryExprAST(Val->Loc, tok::Assign, 
          val,
          new BinaryExprAST(Val->Loc, tok::Plus,
            valCopy, 
            new IntExprAST(Val->Loc, (Op == tok::PlusPlus) ? 1 : -1)));
      }
      break;

    case tok::Not:
      result = new BinaryExprAST(Val->Loc, tok::Equal, Val, new IntExprAST(Val->Loc, 
        0));
      break;

    default:
      assert(0 && "Invalid unary expression");
      return nullptr;
  }

  if (result != this) {
    Val = nullptr;
    delete this;
    return result->semantic(scope);
  }

  return result;
}

ExprAST* UnaryExprAST::clone() {
  return new UnaryExprAST(Loc, Op, Val->clone());
}

ExprAST* BinaryExprAST::semantic(Scope* scope) {
  if (ExprType) {
    return this;
  }

  LeftExpr = LeftExpr->semantic(scope);

  if (Op == tok::PlusPlus || Op == tok::MinusMinus) {
    if (!LeftExpr->isLValue() || !LeftExpr->ExprType->isInt()) {
      scope->report(Loc, diag::ERR_SemaInvalidPostfixPrefixOperand);
      return nullptr;
    }

    ExprType = LeftExpr->ExprType;
    return this;
  }

  RightExpr = RightExpr->semantic(scope);

  if (!LeftExpr->ExprType || !RightExpr->ExprType) {
    scope->report(Loc, diag::ERR_SemaUntypedBinaryExpressionOperands);
    return nullptr;
  }

  if (Op == tok::Comma) {
    ExprType = RightExpr->ExprType;
    return this;
  }

  if (LeftExpr->ExprType->isVoid() || RightExpr->ExprType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaOperandIsVoid);
    return nullptr;
  }

  switch (Op) {
    case tok::Less:
    case tok::Greater:
    case tok::LessEqual:
    case tok::GreaterEqual:
    case tok::Equal:
    case tok::NotEqual:
      if (LeftExpr->ExprType->isBool()) {
        LeftExpr = new CastExprAST(LeftExpr->Loc, LeftExpr, 
          BuiltinTypeAST::get(TypeAST::TI_Int));
        LeftExpr = LeftExpr->semantic(scope);
      }

      if (!LeftExpr->ExprType->equal(RightExpr->ExprType)) {
        RightExpr = new CastExprAST(RightExpr->Loc, RightExpr, LeftExpr->ExprType);
        RightExpr = RightExpr->semantic(scope);
      }
      
      ExprType = BuiltinTypeAST::get(TypeAST::TI_Bool);
      return this;

    case tok::LogOr:
    case tok::LogAnd:
      if (!LeftExpr->ExprType->implicitConvertTo(BuiltinTypeAST::get(TypeAST::TI_Bool)) ||
        !RightExpr->ExprType->implicitConvertTo(BuiltinTypeAST::get(TypeAST::TI_Bool))) {
        scope->report(Loc, diag::ERR_SemaCantConvertToBoolean);
        return nullptr;
      }

      ExprType = BuiltinTypeAST::get(TypeAST::TI_Bool);
      return this;

    default:
      break;
  }

  if (LeftExpr->ExprType == BuiltinTypeAST::get(TypeAST::TI_Bool)) {
    LeftExpr = new CastExprAST(LeftExpr->Loc, LeftExpr, 
      BuiltinTypeAST::get(TypeAST::TI_Int));
    LeftExpr = LeftExpr->semantic(scope);
  }

  ExprType = LeftExpr->ExprType;

  if (Op == tok::Assign) {
    if (!LeftExpr->ExprType->equal(RightExpr->ExprType)) {
      RightExpr = new CastExprAST(RightExpr->Loc, RightExpr, LeftExpr->ExprType);
      RightExpr = RightExpr->semantic(scope);
    }

    if (!LeftExpr->isLValue()) {
      scope->report(Loc, diag::ERR_SemaMissingLValueInAssignment);
      return nullptr;
    }

    return this;
  }

  if (!LeftExpr->ExprType->equal(RightExpr->ExprType)) {
    if (RightExpr->ExprType->isFloat()) {
      ExprType = RightExpr->ExprType;
      LeftExpr = new CastExprAST(LeftExpr->Loc, LeftExpr, RightExpr->ExprType);
      LeftExpr = LeftExpr->semantic(scope);
    } else {
      RightExpr = new CastExprAST(RightExpr->Loc, RightExpr, LeftExpr->ExprType);
      RightExpr = RightExpr->semantic(scope);
    }
  }

  if (ExprType == BuiltinTypeAST::get(TypeAST::TI_Int)) {
    switch (Op) {
      case tok::Plus:
      case tok::Minus:
      case tok::Mul:
      case tok::Div:
      case tok::Mod:
      case tok::BitOr:
      case tok::BitAnd:
      case tok::BitXor:
      case tok::LShift:
      case tok::RShift:
        return this;

      default:
        assert(0 && "Invalid integral binary operator"); 
        return nullptr;
    }
  } else {
    switch (Op) {
      case tok::Plus: 
      case tok::Minus: 
      case tok::Mul: 
      case tok::Div: 
      case tok::Mod: 
      case tok::Less: 
        return this;

      default:
        scope->report(Loc, diag::ERR_SemaInvalidBinaryExpressionForFloatingPoint);
        return nullptr;
    }
  }
}

ExprAST* BinaryExprAST::clone() {
  return new BinaryExprAST(Loc, Op, LeftExpr->clone(), 
    RightExpr ? RightExpr->clone() : nullptr);
}

bool CondExprAST::isLValue() {
  return IfExpr->isLValue() && ElseExpr->isLValue();
}

ExprAST* CondExprAST::semantic(Scope* scope) {
  if (SemaDone) {
    return this;
  }

  Cond = Cond->semantic(scope);
  IfExpr = IfExpr->semantic(scope);
  ElseExpr = ElseExpr->semantic(scope);

  if (Cond->ExprType == nullptr || Cond->ExprType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaConditionIsVoid);
    return nullptr;
  }

  if (!Cond->ExprType->implicitConvertTo(BuiltinTypeAST::get(TypeAST::TI_Bool))) {
    scope->report(Loc, diag::ERR_SemaCantConvertToBoolean);
    return nullptr;
  }

  ExprType = IfExpr->ExprType;

  if (IfExpr->ExprType->equal(ElseExpr->ExprType)) {
    SemaDone = true;
    return this;
  }

  if (!IfExpr->ExprType || !ElseExpr->ExprType ||
    IfExpr->ExprType->isVoid() || ElseExpr->ExprType->isVoid()) {
    scope->report(Loc, diag::ERR_SemaOperandIsVoid);
    return nullptr;
  }

  ElseExpr = new CastExprAST(ElseExpr->Loc, ElseExpr, ExprType);
  ElseExpr = ElseExpr->semantic(scope);
  SemaDone = true;

  return this;
}

ExprAST* CondExprAST::clone() {
  return new CondExprAST(Loc, Cond->clone(), IfExpr->clone(), ElseExpr->clone());
}

static SymbolAST* resolveFunctionCall(Scope *scope, SymbolAST* func, CallExprAST* args) {
  if (isa<FuncDeclAST>(func)) {
    FuncDeclAST* fnc = static_cast< FuncDeclAST* >(func);
    FuncTypeAST* type = static_cast< FuncTypeAST* >(fnc->ThisType);

    if (args->Args.size() != type->Params.size()) {
      scope->report(args->Loc, diag::ERR_SemaInvalidNumberOfArgumentsInCall);
      return nullptr;
    }

    ExprList::iterator arg = args->Args.begin();
    ParameterList::iterator it = type->Params.begin();

    for (ParameterList::iterator end = type->Params.end(); it != end; ++it, ++arg) {
      if (!(*arg)->ExprType->implicitConvertTo((*it)->Param)) {
        scope->report(args->Loc, diag::ERR_SemaInvalidTypesOfArgumentsInCall);
        return nullptr;
      }
    }

    return func;
  }

  return nullptr;
}

ExprAST* CallExprAST::semantic(Scope* scope) {
  if (ExprType) {
    return this;
  }

  Callee = Callee->semantic(scope);

  if (isa<IdExprAST>(Callee)) {
    SymbolAST* sym = ((IdExprAST*)Callee)->ThisSym;

    if (isa<FuncDeclAST>(sym)) {
      TypeAST* returnType = nullptr;

      for (ExprList::iterator arg = Args.begin(), end = Args.end(); arg != end; 
        ++arg) {
        *arg = (*arg)->semantic(scope);
      }
      
      if (SymbolAST* newSym = resolveFunctionCall(scope, sym, this)) {
        FuncDeclAST* fnc = static_cast< FuncDeclAST* >(newSym);
        FuncTypeAST* type = static_cast< FuncTypeAST* >(fnc->ThisType);

        ExprList::iterator arg = Args.begin();
        ParameterList::iterator it = type->Params.begin();

        for (ParameterList::iterator end = type->Params.end(); it != end; 
          ++it, ++arg) {
          if (!(*arg)->ExprType->equal((*it)->Param)) {
            ExprAST* oldArg = (*arg);
            *arg = new CastExprAST(oldArg->Loc, oldArg->clone(), (*it)->Param);
            *arg = (*arg)->semantic(scope);
            delete oldArg;
          }
        }

        if (!returnType) {
          ExprType = ((FuncDeclAST*)newSym)->ReturnType;
        } else {
          ExprType = returnType;
        }

        CallFunc = newSym;
        return this;
      }
    }
  }

  scope->report(Loc, diag::ERR_SemaInvalidArgumentsForCall);
  return nullptr;
}

ExprAST* CallExprAST::clone() {
  ExprList exprs;
  ExprList::iterator it = Args.begin();
  ExprList::iterator end = Args.end();

  for ( ; it != end; ++it) {
    exprs.push_back((*it)->clone());
  }

  return new CallExprAST(Loc, Callee->clone(), exprs);
}

}
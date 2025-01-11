#include "eclair/AST/AST.h"

using namespace llvm;

namespace eclair {

Type* BuiltinTypeAST::getType() {
  static Type* builtinTypes[] = {
    Type::getVoidTy(getGlobalContext()),
    Type::getInt1Ty(getGlobalContext()),
    Type::getInt32Ty(getGlobalContext()),
    Type::getDoubleTy(getGlobalContext()),
    Type::getInt8Ty(getGlobalContext()),
    Type::getInt8PtrTy(getGlobalContext())
  };

  if (ThisType) {
    return ThisType;
  }

  return ThisType = builtinTypes[TypeKind];
}

Type* FuncTypeAST::getType() {
  if (ThisType) {
    return ThisType;
  }

  Type* returnType = (ReturnType ? ReturnType->getType() : 
    Type::getVoidTy(getGlobalContext()));

  if (Params.empty()) {
    return ThisType = FunctionType::get(returnType, false);
  }
  
  std::vector< Type* > params;

  for (ParameterList::iterator it = Params.begin(), end = Params.end();
    it != end; ++it) {
    params.push_back((*it)->Param->getType());
  }

  return ThisType = FunctionType::get(returnType, params, false);
}

} 
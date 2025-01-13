#include "eclair/AST/AST.h"

using namespace llvm;

namespace eclair {

StringSet< > TypeAST::TypesTable;
static std::vector< TypeAST* > TypesToDelete;

void TypeAST::registerTypeForDeletion(TypeAST* thisType) {
  TypesToDelete.push_back(thisType);
}

void TypeAST::clearAllTypes() {
  for (std::vector< TypeAST* >::iterator it = TypesToDelete.begin(),
    end = TypesToDelete.end(); it != end; ++it) {
    delete *it;
  }

  TypesToDelete.clear();
}

void TypeAST::calcMangle() {
  if (!MangleName.empty()) {
    return;
  }

  llvm::SmallString< 128 > s;
  llvm::raw_svector_ostream output(s);

  toMangleBuffer(output);

  TypesTable.insert(output.str());

  StringSet< >::iterator pos = TypesTable.find(s);
  assert(pos != TypesTable.end());
  MangleName = pos->getKeyData();
}

bool TypeAST::equal(TypeAST* otherType) {
  assert(!MangleName.empty() && !otherType->MangleName.empty());
  return MangleName == otherType->MangleName;
}

TypeAST* BuiltinTypeAST::semantic(Scope* scope) {
  calcMangle();
  return this;
}

TypeAST *BuiltinTypeAST::get(int type) {
  static TypeAST *builtinTypes[] = {
    new BuiltinTypeAST(TI_Void),
    new BuiltinTypeAST(TI_Bool),
    new BuiltinTypeAST(TI_Int),
    new BuiltinTypeAST(TI_Float),
  };

  assert(type >= TI_Void && type <= TI_Float);
  return builtinTypes[type];
}

bool BuiltinTypeAST::implicitConvertTo(TypeAST* newType) {
  static bool convertResults[TI_Float + 1][TI_Float + 1] = {
    // void  bool   int    float
    { false, false, false, false }, // void
    { false, true,  true,  true  }, // bool
    { false, true,  true,  true  }, // int
    { false, true,  true,  true  }, // float
  };

  if (newType->TypeKind > TI_Float) {
    return false;
  }

  return convertResults[TypeKind][newType->TypeKind];
}

void BuiltinTypeAST::toMangleBuffer(llvm::raw_ostream& output) {
  switch (TypeKind) {
    case TI_Void : output << "v"; break;
    case TI_Bool : output << "b"; break;
    case TI_Int : output << "i"; break;
    case TI_Float : output << "f"; break;
    default: assert(0 && "Should never happen"); break;
  }
}

TypeAST* FuncTypeAST::semantic(Scope* scope) {
  if (!ReturnType) {
    ReturnType = BuiltinTypeAST::get(TypeAST::TI_Void);
  }
  
  ReturnType = ReturnType->semantic(scope);
  
  for (ParameterList::iterator it = Params.begin(), end = Params.end();
    it != end; ++it) {
    (*it)->Param = (*it)->Param->semantic(scope);
  }

  calcMangle();
  return this;
}

bool FuncTypeAST::implicitConvertTo(TypeAST* newType) {
  return false;
}

void FuncTypeAST::toMangleBuffer(llvm::raw_ostream& output) {
  if (Params.empty()) {
    output << "v";
    return;
  }
  
  ParameterList::iterator it = Params.begin(), end = Params.end();

  for ( ; it != end; ++it) {
    (*it)->Param->toMangleBuffer(output);
  }
}

}
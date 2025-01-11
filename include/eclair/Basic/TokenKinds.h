#ifndef ECLAIR_BASIC_TOKENKINDS_H
#define ECLAIR_BASIC_TOKENKINDS_H

#include "llvm/Support/Compiler.h"

namespace eclair {

namespace tok {
enum TokenKind : unsigned short {
#define TOK(ID, TEXT) ID,
#include "eclair/Basic/TokenKinds.def"
};

const char *getKeywordSpelling(TokenKind Kind) LLVM_READONLY;

const char *toString(TokenKind tok);
} // namespace tok

} // namespace eclair

#endif
#include "eclair/Basic/TokenKinds.h"

namespace eclair {

const char *tok::getKeywordSpelling(TokenKind Kind) {
  switch (Kind) {
#define KEYWORD(ID, TEXT)                                                      \
    case ID:                                                                   \
      return TEXT;
#include "eclair/Basic/TokenKinds.def"
    default:
      break;
  }

  return nullptr;
}

const char* tok::toString(TokenKind Kind) {
  static const char *TokenStrings[] = {
#define TOK(ID, TEXT) TEXT,
#include "eclair/Basic/TokenKinds.def"
  };

  return TokenStrings[Kind];
}

}
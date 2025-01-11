#ifndef ECLAIR_BASIC_NAME_H
#define ECLAIR_BASIC_NAME_H

#include "eclair/Basic/LLVM.h"

namespace eclair {

struct Name {
  const char *Id; ///< Name's text
  int Kind;       ///< Name's kind (one of TokenKind)
  size_t Length;  ///< Length of the text
};

} // namespace eclair

#endif
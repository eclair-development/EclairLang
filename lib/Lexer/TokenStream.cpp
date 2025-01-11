#include "eclair/Lexer/TokenStream.h"

namespace eclair {

TokenStream::TokenStream(Lexer *lex)
  : Lex(lex),
    Head(nullptr),
    Tail(nullptr),
    ScanDone(false) {
}

TokenStream::~TokenStream() {
  StreamNode *p = Head;

  while (p) {
    StreamNode *tmp = p;
    p = p->Next;
    delete tmp;
  }
}

TokenStream::TokenStreamIterator TokenStream::begin() {
  if (!Head) {
    StreamNode *p = new StreamNode(nullptr, nullptr);

    Lex->next(p->Tok);
    ScanDone = p->Tok.getKind() == tok::EndOfFile;
    Tail = Head = p;
  }

  return TokenStreamIterator(this, Head);
}

TokenStream::StreamNode *TokenStream::next(StreamNode *curPos) {
  if (!curPos->Next) {
    if (ScanDone) {
      return curPos;
    }

    StreamNode *p = new StreamNode(nullptr, Tail);

    Lex->next(p->Tok);
    ScanDone = p->Tok.getKind() == tok::EndOfFile;
    Tail->Next = p;
    Tail = p;
    
    return Tail;
  } else {
    return curPos->Next;
  }
}

}
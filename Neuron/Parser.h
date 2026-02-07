#ifndef PARSER_H
#define PARSER_H

#include "term.h"

class Reader;
class Scanner;

class Parser
{
  public:
    Parser(Reader* r = nullptr);
    ~Parser();

    Term* ParseTerm();
    Term* ParseTermBase();
    Term* ParseTermRest(Term* _base);
    TermList* ParseTermList(int for_struct);
    TermArray* ParseArray();
    TermStruct* ParseStruct();

  private:
    Reader* reader;
    Scanner* lexer;
};

#endif

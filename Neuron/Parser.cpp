#include "pch.h"
#include "parser.h"
#include "reader.h"
#include "token.h"

enum Keys : uint8_t
{
  KEY_TRUE,
  KEY_FALSE,
  KEY_DEF,
  KEY_MINUS
};

static int dump_tokens = 0;

namespace
{
  [[noreturn]] void Error(std::string_view _msg, const Token& _token)
  {
    auto msg = std::string(_msg) + std::format(" near '{}' in line {}.", _token.symbol(), _token.line());
    Fatal("{}", msg);
  }
}

Parser::Parser(Reader* r)
{
  DEBUG_ASSERT(r != nullptr);
  reader = r;
  lexer = NEW Scanner(reader);

  Token::addKey("true", KEY_TRUE);
  Token::addKey("false", KEY_FALSE);
  Token::addKey(":", KEY_DEF);
  Token::addKey("-", KEY_MINUS);
}

Parser::~Parser()
{
  delete lexer;
  delete reader;
  //Token::close();
}

Term* Parser::ParseTerm()
{
  Term* t = ParseTermBase();
  if (t == nullptr)
    return t;

  Term* t2 = ParseTermRest(t);

  return t2;
}

Term* Parser::ParseTermRest(Term* _base)
{
  Token t = lexer->Get();

  switch (t.type())
  {
    default:
      lexer->PutBack();
      return _base;

    case Token::StringLiteral:
    {
      // concatenate adjacent string literal tokens:
      if (TermText* text = _base->isText())
      {
        auto base2 = NEW TermText(text->value() + t.symbol().substr(1, t.symbol().length() - 2));
        delete _base;
        return ParseTermRest(base2);
      }
      lexer->PutBack();
    }
    break;

    case Token::Keyword:
      switch (t.key())
      {
        case KEY_DEF:
        {
          if (_base->isText())
            return NEW TermDef(_base->isText(), ParseTerm());
          Error("(Parse) illegal lhs in def", t);
        }

        default:
          lexer->PutBack();
          return _base;
      }
  }

  return _base;
}

static int xtol(const char* p)
{
  int n = 0;

  while (*p)
  {
    char digit = *p++;
    n *= 16;

    if (digit >= '0' && digit <= '9')
      n += digit - '0';

    else if (digit >= 'a' && digit <= 'f')
      n += digit - 'a' + 10;

    else if (digit >= 'A' && digit <= 'F')
      n += digit - 'A' + 10;
  }

  return n;
}

Term* Parser::ParseTermBase()
{
  Token t = lexer->Get();
  int n = 0;
  double d = 0.0;

  switch (t.type())
  {
    case Token::IntLiteral:
    {
      if (dump_tokens)
        DebugTrace("%s", t.symbol().data());

      char nstr[256], *p = nstr;
      for (int i = 0; i < t.symbol().length(); i++)
      {
        if (t.symbol()[i] != '_')
          *p++ = t.symbol()[i];
      }
      *p++ = '\0';

      // handle hex notation:
      if (nstr[1] == 'x')
        n = xtol(nstr + 2);

      else
        n = atol(nstr);

      return NEW TermNumber(n);
    }

    case Token::FloatLiteral:
    {
      if (dump_tokens)
        DebugTrace("%s", t.symbol().data());

      char nstr[256], *p = nstr;
      for (int i = 0; i < t.symbol().length(); i++)
      {
        if (t.symbol()[i] != '_')
          *p++ = t.symbol()[i];
      }
      *p++ = '\0';

      d = atof(nstr);
      return NEW TermNumber(d);
    }

    case Token::StringLiteral:
      if (dump_tokens)
        DebugTrace("%s", t.symbol().data());

      return NEW TermText(t.symbol().substr(1, t.symbol().length() - 2));

    case Token::AlphaIdent:
      if (dump_tokens)
        DebugTrace("%s", t.symbol().data());

      return NEW TermText(t.symbol());

    case Token::Keyword:
      if (dump_tokens)
        DebugTrace("%s", t.symbol().data());

      switch (t.key())
      {
        case KEY_FALSE:
          return NEW TermBool(false);
        case KEY_TRUE:
          return NEW TermBool(true);

        case KEY_MINUS:
        {
          Token next = lexer->Get();
          if (next.type() == Token::IntLiteral)
          {
            if (dump_tokens)
              DebugTrace("%s", next.symbol().data());

            char nstr[256], *p = nstr;
            for (int i = 0; i < next.symbol().length(); i++)
            {
              if (next.symbol()[i] != '_')
                *p++ = next.symbol()[i];
            }
            *p++ = '\0';

            n = -1 * atol(nstr);
            return NEW TermNumber(n);
          }
          if (next.type() == Token::FloatLiteral)
          {
            if (dump_tokens)
              DebugTrace("%s", next.symbol().data());

            char nstr[256], *p = nstr;
            for (int i = 0; i < next.symbol().length(); i++)
            {
              if (next.symbol()[i] != '_')
                *p++ = next.symbol()[i];
            }
            *p++ = '\0';

            d = -1.0 * atof(nstr);
            return NEW TermNumber(d);
          }
          lexer->PutBack();
          Error("(Parse) illegal token '-': number expected", next);
        }
        break;

        default:
          lexer->PutBack();
          return nullptr;
      }

    case Token::LParen:
      return ParseArray();

    case Token::LBrace:
      return ParseStruct();

    case Token::CharLiteral:
      Error("(Parse) illegal token ", t);

    default:
      lexer->PutBack();
      return nullptr;
  }
}

TermArray* Parser::ParseArray()
{
  TermList* elems = ParseTermList(0);
  Token end = lexer->Get();

  if (end.type() != Token::RParen)
    Error("(Parse) ')' missing in array-decl", end);

  return NEW TermArray(elems);
}

TermStruct* Parser::ParseStruct()
{
  TermList* elems = ParseTermList(1);
  Token end = lexer->Get();

  if (end.type() != Token::RBrace)
    Error("(Parse) '}' missing in struct", end);

  return NEW TermStruct(elems);
}

TermList* Parser::ParseTermList(int for_struct)
{
  auto tlist = NEW TermList;

  Term* term = ParseTerm();
  while (term)
  {
    if (for_struct && !term->isDef())
      Error("(Parse) non-definition term in struct");
    if (!for_struct && term->isDef())
      Error("(Parse) illegal definition in array");

    tlist->append(term);
    Token t = lexer->Get();

    // NEW WAY: COMMA SEPARATORS OPTIONAL:
    if (t.type() != Token::Comma)
      lexer->PutBack();

    term = ParseTerm();
  }

  return tlist;
}

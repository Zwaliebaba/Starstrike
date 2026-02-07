#include "pch.h"
#include "Token.h"
#include "Reader.h"

Token::Token()
  : m_type(Undefined),
    m_key(0),
    m_line(0),
    m_column(0)
{
  m_length = 0;
  m_symbol[0] = '\0';
}

Token::Token(const Token& rhs)
  : m_type(rhs.m_type),
    m_key(rhs.m_key),
    m_line(rhs.m_line),
    m_column(rhs.m_column)
{
  m_length = rhs.m_length;
  if (m_length < 8)
    strcpy_s(m_symbol, rhs.m_symbol);
  else
  {
    m_fullSymbol = NEW char[m_length + 1];
    strcpy(m_fullSymbol, rhs.m_fullSymbol);
  }
}

Token::Token(int t)
  : m_type(t),
    m_key(0),
    m_line(0),
    m_column(0)
{
  m_length = 0;
  m_symbol[0] = '\0';
}

Token::Token(const char* s, int t, int k, int l, int c)
  : m_type(t),
    m_key(k),
    m_line(l),
    m_column(c)
{
  m_length = strlen(s);
  if (m_length < 8)
    strcpy_s(m_symbol, s);
  else
  {
    m_fullSymbol = NEW char[m_length + 1];
    strcpy(m_fullSymbol, s);
  }
}

Token::Token(const std::string& s, int t, int k, int l, int c)
  : m_type(t),
    m_key(k),
    m_line(l),
    m_column(c)
{
  m_length = s.length();
  if (m_length < 8)
    strcpy_s(m_symbol, s.data());
  else
  {
    m_fullSymbol = NEW char[m_length + 1];
    strcpy(m_fullSymbol, s.data());
  }
}

Token::~Token()
{
  if (m_length >= 8)
    delete [] m_fullSymbol;
}

void Token::close() { sm_keymap.clear(); }

Token& Token::operator =(const Token& rhs)
{
  if (m_length >= 8)
    delete [] m_fullSymbol;

  m_length = rhs.m_length;
  if (m_length < 8)
    strcpy_s(m_symbol, rhs.m_symbol);
  else
  {
    m_fullSymbol = NEW char[m_length + 1];
    strcpy(m_fullSymbol, rhs.m_fullSymbol);
  }

  m_type = rhs.m_type;
  m_key = rhs.m_key;
  m_line = rhs.m_line;
  m_column = rhs.m_column;

  return *this;
}

bool Token::match(const Token& ref) const
{
  if (m_type == ref.m_type)
  {                    // if types match 
    if (ref.m_length == 0)                     // if no symbol to match
      return true;                           // match!
    if (m_length == ref.m_length)
    {        // else if symbols match
      if (m_length < 8)
      {
        if (!strcmp(m_symbol, ref.m_symbol))
          return true;                     // match!
      }
      else
      {
        if (!strcmp(m_fullSymbol, ref.m_fullSymbol))
          return true;                     // match!
      }
    }
  }

  return false;
}

std::string Token::symbol() const
{
  if (m_length < 8)
    return std::string(m_symbol);
  return std::string(m_fullSymbol);
}

void Token::addKey(const std::string& k, int v) { sm_keymap.insert(k, v); }

void Token::addKeys(Dictionary<int>& keys)
{
  DictionaryIter<int> iter = keys;
  while (++iter)
    sm_keymap.insert(iter.key(), iter.value());
}

bool Token::findKey(const std::string& k, int& v)
{
  if (sm_keymap.contains(k))
  {
    v = sm_keymap.find(k, 0);
    return true;
  }
  return false;
}

void Token::comments(const std::string& begin, const std::string& end)
{
  sm_combeg[0] = begin[0];
  if (begin.length() > 1)
    sm_combeg[1] = begin[1];
  else
    sm_combeg[1] = '\0';

  sm_comend[0] = end[0];
  if (end.length() > 1)
    sm_comend[1] = end[1];
  else
    sm_comend[1] = '\0';
}

void Token::altComments(const std::string& begin, const std::string& end)
{
  sm_altbeg[0] = begin[0];
  if (begin.length() > 1)
    sm_altbeg[1] = begin[1];
  else
    sm_altbeg[1] = '\0';

  sm_altend[0] = end[0];
  if (end.length() > 1)
    sm_altend[1] = end[1];
  else
    sm_altend[1] = '\0';
}

std::string Token::typestr() const
{
  std::string t = "Unknown";
  switch (type())
  {
    case Undefined:
      t = "Undefined";
      break;
    case Keyword:
      t = "Keyword";
      break;
    case AlphaIdent:
      t = "AlphaIdent";
      break;
    case SymbolicIdent:
      t = "SymbolicIdent";
      break;
    case Comment:
      t = "Comment";
      break;
    case IntLiteral:
      t = "IntLiteral";
      break;
    case FloatLiteral:
      t = "FloatLiteral";
      break;
    case StringLiteral:
      t = "StringLiteral";
      break;
    case CharLiteral:
      t = "CharLiteral";
      break;
    case Dot:
      t = "Dot";
      break;
    case Comma:
      t = "Comma";
      break;
    case Colon:
      t = "Colon";
      break;
    case Semicolon:
      t = "Semicolon";
      break;
    case LParen:
      t = "LParen";
      break;
    case RParen:
      t = "RParen";
      break;
    case LBracket:
      t = "LBracket";
      break;
    case RBracket:
      t = "RBracket";
      break;
    case LBrace:
      t = "LBrace";
      break;
    case RBrace:
      t = "RBrace";
      break;
    case EOT:
      t = "EOT";
      break;
    case LastTokenType:
      t = "LastTokenType";
      break;
  }

  return t;
}

std::string Token::describe(const std::string& tok)
{
  std::string d;

  switch (tok[0])
  {
    case '.':
      d = "Token::Dot";
      break;
    case ',':
      d = "Token::Comma";
      break;
    case ';':
      d = "Token::Semicolon";
      break;
    case '(':
      d = "Token::LParen";
      break;
    case ')':
      d = "Token::RParen";
      break;
    case '[':
      d = "Token::LBracket";
      break;
    case ']':
      d = "Token::RBracket";
      break;
    case '{':
      d = "Token::LBrace";
      break;
    case '}':
      d = "Token::RBrace";
      break;
    default:
      break;
  }

  if (d.empty())
  {
    if (isalpha(tok[0]))
      d = "\"" + tok + "\", Token::AlphaIdent";
    else if (isdigit(tok[0]))
    {
      if (tok.contains("."))
        d = "\"" + tok + "\", Token::FloatLiteral";
      else
        d = "\"" + tok + "\", Token::IntLiteral";
    }
    else
      d = "\"" + tok + "\", Token::SymbolicIdent";
  }

  return d;
}

Scanner::Scanner(Reader* r)
  : m_reader(r),
    m_str(nullptr),
    m_index(0),
    m_oldIndex(0),
    m_length(0),
    m_line(0),
    m_oldLine(0),
    m_lineStart(0) {}

Scanner::Scanner(const Scanner& rhs)
  : m_reader(rhs.m_reader),
    m_index(rhs.m_index),
    m_oldIndex(rhs.m_oldIndex),
    m_length(rhs.m_length),
    m_line(rhs.m_line),
    m_oldLine(0),
    m_lineStart(rhs.m_lineStart)
{
  m_str = NEW char [strlen(rhs.m_str) + 1];
  strcpy(m_str, rhs.m_str);
}

Scanner::Scanner(const std::string& s)
  : m_reader(nullptr),
    m_index(0),
    m_oldIndex(0),
    m_length(s.length()),
    m_line(0),
    m_oldLine(0),
    m_lineStart(0)
{
  m_str = NEW char [s.length() + 1];
  strcpy(m_str, s.data());
}

Scanner::~Scanner() { delete [] m_str; }

Scanner& Scanner::operator =(const Scanner& rhs)
{
  delete [] m_str;
  m_str = NEW char [strlen(rhs.m_str) + 1];
  strcpy(m_str, rhs.m_str);

  m_index = rhs.m_index;
  m_oldIndex = rhs.m_oldIndex;
  m_length = rhs.m_length;
  m_line = rhs.m_line;
  m_oldLine = rhs.m_oldLine;
  m_lineStart = rhs.m_lineStart;

  return *this;
}

void Scanner::Load(const std::string& s)
{
  delete [] m_str;
  m_str = NEW char [s.length() + 1];
  strcpy(m_str, s.data());

  m_index = 0;
  m_oldIndex = 0;
  m_best = Token();
  m_length = s.length();
  m_line = 0;
  m_oldLine = 0;
  m_lineStart = 0;
}

Token Scanner::Get(Need need)
{
  int type = Token::EOT;
  m_oldIndex = m_index;
  m_oldLine = m_line;

  m_eos = m_str + m_length;
  p = m_str + m_index;

  if (p >= m_eos)
  {
    if (need == Demand && m_reader)
    {
      Load(m_reader->more());
      if (m_length > 0)
        return Get(need);
    }
    return Token("", type, 0, m_line, 0);
  }

  while (isspace(*p) && p < m_eos)
  { // skip initial white space
    if (*p == '\n')
    {
      m_line++;
      m_lineStart = p - m_str;
    }
    p++;
  }

  if (p >= m_eos)
  {
    if (need == Demand && m_reader)
    {
      Load(m_reader->more());
      if (m_length > 0)
        return Get(need);
    }
    return Token("", type, 0, m_line, 0);
  }

  Token result;
  size_t start = p - m_str;

  if (*p == '"' || *p == '\'')
  {   // special case for quoted tokens

    if (*p == '"')
      type = Token::StringLiteral;
    else
      type = Token::CharLiteral;

    char match = *p;
    while (++p < m_eos)
    {
      if (*p == match)
      {         // find matching quote
        if (*(p - 1) != '\\')
        {   // if not escaped
          p++;                 // token includes matching quote
          break;
        }
      }
    }
  }

  // generic delimited comments
  else if (*p == Token::comBeg(0) && (!Token::comBeg(1) || *(p + 1) == Token::comBeg(1)))
  {
    type = Token::Comment;
    while (++p < m_eos)
    {
      if (*p == Token::comEnd(0) && (!Token::comEnd(1) || *(p + 1) == Token::comEnd(1)))
      {
        p++;
        if (Token::comEnd(1))
          p++;
        break;
      }
    }
  }

  // alternate form delimited comments
  else if (*p == Token::altBeg(0) && (!Token::altBeg(1) || *(p + 1) == Token::altBeg(1)))
  {
    type = Token::Comment;
    while (++p < m_eos)
    {
      if (*p == Token::altEnd(0) && (!Token::altEnd(1) || *(p + 1) == Token::altEnd(1)))
      {
        p++;
        if (Token::altEnd(1))
          p++;
        break;
      }
    }
  }

  else if (*p == '.')
    type = Token::Dot;
  else if (*p == ',')
    type = Token::Comma;
  else if (*p == ';')
    type = Token::Semicolon;
  else if (*p == '(')
    type = Token::LParen;
  else if (*p == ')')
    type = Token::RParen;
  else if (*p == '[')
    type = Token::LBracket;
  else if (*p == ']')
    type = Token::RBracket;
  else if (*p == '{')
    type = Token::LBrace;
  else if (*p == '}')
    type = Token::RBrace;

    // use lexical sub-parser for ints and floats
  else if (isdigit(*p))
    type = GetNumeric();

  else if (IsSymbolic(*p))
  {
    type = Token::SymbolicIdent;
    while (IsSymbolic(*p))
      p++;
  }

  else
  {
    type = Token::AlphaIdent;
    while (IsAlpha(*p))
      p++;
  }

  size_t extent = (p - m_str) - start;

  if (extent < 1)
    extent = 1;      // always get at least one character

  m_index = start + extent;         // advance the cursor
  int col = start - m_lineStart;
  if (m_line == 0)
    col++;

  auto buf = NEW char [extent + 1];
  strncpy(buf, m_str + start, extent);
  buf[extent] = '\0';

  if (type == Token::Comment && Token::sm_hidecom)
  {
    delete [] buf;
    if (Token::comEnd(0) == '\n')
    {
      m_line++;
      m_lineStart = p - m_str;
    }
    return Get(need);
  }

  if (type == Token::AlphaIdent || // check for keyword
    type == Token::SymbolicIdent)
  {
    int val;
    if (Token::findKey(std::string(buf), val))
      result = Token(buf, Token::Keyword, val, m_line + 1, col);
  }

  if (result.m_type != Token::Keyword)
    result = Token(buf, type, 0, m_line + 1, col);

  if (m_line + 1 > static_cast<size_t>(m_best.m_line) || (m_line + 1 == static_cast<size_t>(m_best.m_line) && col >
    m_best.m_column))
    m_best = result;

  delete [] buf;
  return result;
}

int Scanner::GetNumeric()
{
  int type = Token::IntLiteral;             // assume int

  if (*p == '0' && *(p + 1) == 'x')
  {         // check for hex:
    p += 2;
    while (isxdigit(*p))
      p++;
    return type;
  }

  while (isdigit(*p) || *p == '_')
    p++;     // whole number part

  if (*p == '.')
  {
    p++;                     // optional fract part
    type = Token::FloatLiteral;            // implies float

    while (isdigit(*p) || *p == '_')
      p++;  // fractional part
  }

  if (*p == 'E' || *p == 'e')
  {
    p++;       // optional exponent
    if (*p == '+' || *p == '-')
      p++;       // which may be signed
    while (isdigit(*p))
      p++;

    type = Token::FloatLiteral;            // implies float
  }

  return type;
}

bool Scanner::IsAlpha(char c) { return (isalpha(*p) || isdigit(*p) || (*p == '_')); }

bool Scanner::IsSymbolic(char c)
{
  auto s = "+-*/\\<=>~!@#$%^&|:";
  return strchr(s, c) ? true : false;
}

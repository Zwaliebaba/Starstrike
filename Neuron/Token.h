#pragma once

#include "Dictionary.h"

class Reader;
class Token;
class Scanner;

class Token
{
  friend class Scanner;

  public:
    // keywords must be alphanumeric identifiers or symbolic identifiers
    enum Types
    {
      Undefined,
      Keyword,
      AlphaIdent,
      SymbolicIdent,
      Comment,
      IntLiteral,
      FloatLiteral,
      StringLiteral,
      CharLiteral,
      Dot,
      Comma,
      Colon,
      Semicolon,
      LParen,
      RParen,
      LBracket,
      RBracket,
      LBrace,
      RBrace,
      EOT,
      LastTokenType
    };

    enum Alias
    {
      CompoundSeparator   = Dot,
      ItemSeparator       = Comma,
      StatementTerminator = Semicolon,
      TypeIndicator       = Colon,
      Lambda              = LastTokenType + 1
    };

    Token();
    Token(const Token& rhs);
    Token(int t);
    Token(const char* s, int t, int k = 0, int l = 0, int c = 0);
    Token(const std::string& s, int t, int k = 0, int l = 0, int c = 0);
    ~Token();

    Token& operator =(const Token& rhs);

    bool match(const Token& ref) const;

    std::string symbol() const;
    int type() const { return m_type; }
    int key() const { return m_key; }
    int line() const { return m_line; }
    int column() const { return m_column; }

    std::string typestr() const;

    static std::string describe(const std::string& tok);
    static void addKey(const std::string& k, int v);
    static void addKeys(Dictionary<int>& keys);
    static bool findKey(const std::string& k, int& v);
    static void comments(const std::string& begin, const std::string& end);
    static void altComments(const std::string& begin, const std::string& end);
    static void hideComments(bool hide = true) { sm_hidecom = hide; }

    static char comBeg(unsigned int i) { return sm_combeg[i]; }
    static char comEnd(unsigned int i) { return sm_comend[i]; }
    static char altBeg(unsigned int i) { return sm_altbeg[i]; }
    static char altEnd(unsigned int i) { return sm_altend[i]; }

    static void close();

  protected:
    int m_length;

    union
    {
      char m_symbol[8];
      char* m_fullSymbol;
    };

    int m_type;
    int m_key;
    int m_line;
    int m_column;

    inline static bool sm_hidecom = true;
    inline static char sm_combeg[3] = "//";
    inline static char sm_comend[3] = "\n";
    inline static char sm_altbeg[3] = "/*";
    inline static char sm_altend[3] = "*/";

    inline static Dictionary<int> sm_keymap;
};

class Scanner
{
  public:
    Scanner(Reader* r = nullptr);
    Scanner(const std::string& s);
    Scanner(const Scanner& rhs);
    virtual ~Scanner();

    Scanner& operator =(const Scanner& rhs);

    void Load(const std::string& s);

    enum Need
    {
      Demand,
      Request
    };

    virtual Token Get(Need n = Demand);

    void PutBack()
    {
      m_index = m_oldIndex;
      m_line = m_oldLine;
    }

    size_t GetCursor() { return m_index; }
    size_t GetLine() { return m_line; }

    void Reset(size_t c, size_t l)
    {
      m_index = m_oldIndex = c;
      m_line = m_oldLine = l;
    }

    Token Best() const { return m_best; }

  protected:
    virtual int GetNumeric();
    virtual bool IsSymbolic(char c);
    virtual bool IsAlpha(char c);

    Reader* m_reader;
    char* m_str;

    const char* p;
    const char* m_eos;

    size_t m_index;
    size_t m_oldIndex;
    Token m_best;
    size_t m_length;
    size_t m_line;
    size_t m_oldLine;
    size_t m_lineStart;
};

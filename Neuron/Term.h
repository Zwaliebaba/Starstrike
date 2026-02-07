#ifndef TERM_H
#define TERM_H

#include "List.h"

class Term;
class TermBool;
class TermNumber;
class TermText;
class TermArray;
class TermDef;
class TermStruct;

class Term
{
public:

  Term() {}
  virtual ~Term() {}

  virtual int operator==([[maybe_unused]] const Term& rhs) const { return 0; }

  virtual void print(int level = 10) {}

  // conversion tests
  virtual Term* touch() { return this; }
  virtual TermBool* isBool() { return nullptr; }
  virtual TermNumber* isNumber() { return nullptr; }
  virtual TermText* isText() { return nullptr; }
  virtual TermArray* isArray() { return nullptr; }
  virtual TermDef* isDef() { return nullptr; }
  virtual TermStruct* isStruct() { return nullptr; }
};

[[noreturn]] void Error(std::string_view, std::string_view = {});

using TermList = List<Term>;
using TermListIter = ListIter<Term>;

class TermBool : public Term
{
public:

  TermBool(bool v)
    : val(v) {
  }

  void print(int level = 10) override;
  TermBool* isBool() override { return this; }
  bool value() const { return val; }

private:
  bool val;
};

class TermNumber : public Term
{
public:

  TermNumber(double v)
    : val(v) {
  }

  void print(int level = 10) override;
  TermNumber* isNumber() override { return this; }
  double value() const { return val; }

private:
  double val;
};

class TermText : public Term
{
public:

  TermText(std::string v)
    : val(std::move(v)) {
  }

  void print(int level = 10) override;
  TermText* isText() override { return this; }
  std::string value() const { return val; }
  operator const char*() const { return  val.c_str(); }

private:
  std::string val;
};

class TermArray : public Term
{
public:

  TermArray(TermList* elist);
  ~TermArray() override;

  void print(int level = 10) override;
  TermArray* isArray() override { return this; }
  TermList* elements() { return elems; }

private:
  TermList* elems;
};

class TermStruct : public Term
{
public:

  TermStruct(TermList* elist);
  ~TermStruct() override;

  void print(int level = 10) override;

  TermStruct* isStruct() override { return this; }
  TermList* elements() { return elems; }

private:
  TermList* elems;
};

class TermDef : public Term
{
public:

  TermDef(TermText* n, Term* v)
    : mname(n),
    mval(v) {
  }

  ~TermDef() override;

  void print(int level = 10) override;
  TermDef* isDef() override { return this; }

  virtual TermText* name() { return mname; }
  virtual Term* term() { return mval; }

private:
  TermText* mname;
  Term* mval;
};

#endif

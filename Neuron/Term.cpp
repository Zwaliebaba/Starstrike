#include "pch.h"
#include "Term.h"

[[noreturn]] void Error(std::string_view s1, std::string_view s2)
{
  DebugTrace("ERROR: ");
  if (!s1.empty())
    DebugTrace(s1.data());
  if (!s2.empty())
    DebugTrace(s2.data());
  DebugTrace("\n\n");
}

void TermBool::print(int level)
{
  if (level > 0)
    DebugTrace(val ? "true" : "false");
  else
    DebugTrace("...");
}

void TermNumber::print(int level)
{
  if (level > 0)
    DebugTrace("{}", val);
  else
    DebugTrace("...");
}

void TermText::print(int level)
{
  if (level > 0)
    DebugTrace("\"{}\"", val.data());
  else
    DebugTrace("...");
}

TermArray::TermArray(TermList* elist) { elems = elist; }

TermArray::~TermArray()
{
  if (elems)
    elems->destroy();
  delete elems;
}

void TermArray::print(int level)
{
  if (level > 1)
  {
    DebugTrace("(");

    if (elems)
    {
      for (int i = 0; i < elems->size(); i++)
      {
        elems->at(i)->print(level - 1);
        if (i < elems->size() - 1)
          DebugTrace(", ");
      }
    }

    DebugTrace(") ");
  }
  else
    DebugTrace("(...) ");
}

TermStruct::TermStruct(TermList* elist) { elems = elist; }

TermStruct::~TermStruct()
{
  if (elems)
    elems->destroy();
  delete elems;
}

void TermStruct::print(int level)
{
  if (level > 1)
  {
    DebugTrace("{");

    if (elems)
    {
      for (int i = 0; i < elems->size(); i++)
      {
        elems->at(i)->print(level - 1);
        if (i < elems->size() - 1)
          DebugTrace(", ");
      }
    }

    DebugTrace("} ");
  }
  else
    DebugTrace("{...} ");
}

TermDef::~TermDef()
{
  delete mname;
  delete mval;
}

void TermDef::print(int level)
{
  if (level >= 0)
  {
    mname->print(level);
    DebugTrace(": ");
    mval->print(level - 1);
  }
  else
    DebugTrace("...");
}

#include "pch.h"
#include <fstream>
#include "reader.h"

std::string ConsoleReader::more()
{
  // loop until the user types something
  do
  {
    printPrimaryPrompt();
    fillInputBuffer();
  }
  while (!*p);

  return p;
}

void ConsoleReader::printPrimaryPrompt() { printf("- "); }

void ConsoleReader::fillInputBuffer()
{
  fgets(buffer, 980, stdin);
  p = buffer;
  while (isspace(*p))
    p++;
}

FileReader::FileReader(const char* fname)
  : filename(fname),
    done(0) {}

std::string FileReader::more()
{
  if (done)
    return {};

  std::fstream fin(filename.data(), std::fstream::in);

  if (!fin)
  {
    DebugTrace("ERROR(Parse): Could not open file '{}'\n", filename.data());
    return {};
  }

  std::string result;
  char buf[1000], newline;

  while (fin.get(buf, 1000))
  {
    result.append(buf);
    fin.get(newline);
    result += newline;
  }

  done = 1;
  return result;
}

BlockReader::BlockReader(std::string_view block)
  : data(block),
    done(0),
    length(0) {}

std::string BlockReader::more()
{
  if (done)
    return {};

  if (length)
  {
    std::string result(data);
    done = 1;
    return result;
  }
  if (!data.empty())
  {
    std::string result(data);
    done = 1;
    return result;
  }

  done = 1;
  return {};
}

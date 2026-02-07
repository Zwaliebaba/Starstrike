#include "pch.h"
#include "ContentBundle.h"
#include "DataLoader.h"

ContentBundle::ContentBundle(std::string_view bundle, Locale* locale)
{
  std::string file = FindFile(bundle, locale);
  if (!file.empty())
    LoadBundle(file);
}

ContentBundle::~ContentBundle() {}

std::string ContentBundle::GetText(std::string_view key) const { return values.find(std::string(key), std::string(key)); }

std::string ContentBundle::FindFile(std::string_view bundle, Locale* locale)
{
  auto basename = std::string(bundle);
  DataLoader* loader = DataLoader::GetLoader();

  if (loader && !bundle.empty())
  {
    std::string result;
    if (locale)
    {
      result = basename + locale->GetFullCode() + ".txt";

      if (loader->FindFile(result))
        return result;

      result = basename + "_" + locale->GetLanguage() + ".txt";

      if (loader->FindFile(result))
        return result;
    }

    result = basename + ".txt";

    if (loader->FindFile(result))
      return result;
  }

  return std::string();
}

void ContentBundle::LoadBundle(std::string_view filename)
{
  DataLoader* loader = DataLoader::GetLoader();
  if (loader && !filename.empty())
  {
    BYTE* buffer = nullptr;
    loader->LoadBuffer(filename, buffer, true, true);
    if (buffer && *buffer)
    {
      char key[1024];
      char val[2048];
      auto p = (char*)buffer;
      int s = 0, ik = 0, iv = 0;

      key[0] = 0;
      val[0] = 0;

      while (*p)
      {
        if (*p == '=')
          s = 1;
        else if (*p == '\n' || *p == '\r')
        {
          if (key[0] && val[0])
            values.insert(Trim(key), Trim(val));

          ZeroMemory(key, 1024);
          ZeroMemory(val, 2048);
          s = 0;
          ik = 0;
          iv = 0;
        }
        else if (s == 0)
        {
          if (!key[0])
          {
            if (*p == '#')
              s = -1; // comment
            else if (!isspace(*p))
              key[ik++] = *p;
          }
          else
            key[ik++] = *p;
        }
        else if (s == 1)
        {
          if (!isspace(*p))
          {
            s = 2;
            val[iv++] = *p;
          }
        }
        else if (s == 2)
          val[iv++] = *p;

        p++;
      }

      loader->ReleaseBuffer(buffer);
    }
  }
}

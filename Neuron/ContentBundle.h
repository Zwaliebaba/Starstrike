#pragma once

#include "Dictionary.h"
#include "Locale_ss.h"

class ContentBundle
{
  public:
    ContentBundle(std::string_view bundle, Locale* locale);
    virtual ~ContentBundle();

    int operator ==(const ContentBundle& that) const { return this == &that; }

    std::string GetName() const { return name; }
    std::string GetText(std::string_view key) const;
    bool IsLoaded() const { return !values.isEmpty(); }

  protected:
    void LoadBundle(std::string_view filename);
    std::string FindFile(std::string_view bundle, Locale* locale);

    std::string name;
    Dictionary<std::string> values;
};

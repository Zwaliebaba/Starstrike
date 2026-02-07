#ifndef FontMgr_h
#define FontMgr_h

#include "List.h"

class Font;

struct FontItem
{
  std::string name;
  int size;
  Font* font;
};

class FontMgr
{
  public:
    static void Close();
    static void Register(std::string_view name, Font* font);
    static Font* Find(std::string_view name);

  private:
    static List<FontItem> fonts;
};

#endif FontMgr_h

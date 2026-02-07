#include "pch.h"
#include "FontMgr.h"

List<FontItem> FontMgr::fonts;

void FontMgr::Close() { fonts.destroy(); }

void FontMgr::Register(std::string_view name, Font* font)
{
  auto item = NEW FontItem;

  if (item)
  {
    item->name = name;
    item->size = 0;
    item->font = font;

    fonts.append(item);
  }
}

Font* FontMgr::Find(std::string_view name)
{
  ListIter<FontItem> item = fonts;
  while (++item)
  {
    if (item->name == name)
      return item->font;
  }

  return nullptr;
}

#include "pch.h"
#include "Skin.h"
#include "Solid.h"

Skin::Skin(std::string_view n)
{
  name = n;
}

Skin::~Skin() { cells.destroy(); }

void Skin::SetName(std::string_view n)
{
    name = n;
}

void Skin::SetPath(std::string_view n)
{
  path = n;
}

void Skin::AddMaterial(const Material* mtl)
{
  if (!mtl)
    return;

  bool found = false;

  ListIter<SkinCell> iter = cells;
  while (++iter && !found)
  {
    SkinCell* s = iter.value();

    if (s->skin && s->skin->name == mtl->name)
    {
      s->skin = mtl;
      found = true;
    }
  }

  if (!found)
  {
    auto s = NEW SkinCell(mtl);
    cells.append(s);
  }
}

void Skin::ApplyTo(Model* model) const
{
  if (model)
  {
    for (int i = 0; i < cells.size(); i++)
    {
      SkinCell* s = cells[i];

      if (s->skin)
        s->orig = model->ReplaceMaterial(s->skin);
    }
  }
}

void Skin::Restore(Model* model) const
{
  if (model)
  {
    for (int i = 0; i < cells.size(); i++)
    {
      SkinCell* s = cells[i];

      if (s->orig)
      {
        model->ReplaceMaterial(s->orig);
        s->orig = nullptr;
      }
    }
  }
}

SkinCell::SkinCell(const Material* mtl)
  : skin(mtl),
    orig(nullptr) {}

SkinCell::~SkinCell() { delete skin; }

int SkinCell::operator ==(const SkinCell& other) const
{
  if (skin == other.skin)
    return true;

  if (skin && other.skin)
    return skin->name != other.skin->name;

  return false;
}

std::string SkinCell::Name() const
{
  if (skin)
    return skin->name;

  return "Invalid Skin Cell";
}

void SkinCell::SetSkin(const Material* mtl) { skin = mtl; }

void SkinCell::SetOrig(const Material* mtl) { orig = mtl; }

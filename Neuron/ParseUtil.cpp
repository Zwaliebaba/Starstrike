#include "pch.h"
#include "ParseUtil.h"
#include "DataLoader.h"

bool GetDefBool(bool& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing BOOL TermDef in '{}'\n", file);
    return false;
  }

  TermBool* tn = def->term()->isBool();
  if (tn)
  {
    dst = tn->value();
    return true;
  }
  DebugTrace("WARNING: invalid bool {} in '{}'.  value = ", def->name()->value().data(), file);
  def->term()->print(10);
  DebugTrace("\n");

  return false;
}

bool GetDefText(std::string& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing TEXT TermDef in '{}'\n", file);
    return false;
  }

  TermText* tn = def->term()->isText();
  if (tn)
  {
    dst = tn->value().data();
    return true;
  }
  DebugTrace("WARNING: invalid TEXT {} in '{}'\n", def->name()->value().data(), file);

  return false;
}

bool GetDefNumber(int& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing NUMBER TermDef in '{}'\n", file);
    return false;
  }

  TermNumber* tr = def->term()->isNumber();
  if (tr)
  {
    dst = static_cast<int>(tr->value());
    return true;
  }
  DebugTrace("WARNING: invalid NUMBER {} in '{}'\n", def->name()->value().data(), file);

  return false;
}

bool GetDefNumber(DWORD& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing NUMBER TermDef in '{}'\n", file);
    return false;
  }

  TermNumber* tr = def->term()->isNumber();
  if (tr)
  {
    dst = static_cast<DWORD>(tr->value());
    return true;
  }
  DebugTrace("WARNING: invalid NUMBER {} in '{}'\n", def->name()->value().data(), file);

  return false;
}

bool GetDefNumber(float& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing NUMBER TermDef in '{}'\n", file);
    return false;
  }

  TermNumber* tr = def->term()->isNumber();
  if (tr)
  {
    dst = static_cast<float>(tr->value());
    return true;
  }
  DebugTrace("WARNING: invalid NUMBER {} in '{}'\n", def->name()->value().data(), file);

  return false;
}

bool GetDefNumber(double& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing NUMBER TermDef in '{}'\n", file);
    return false;
  }

  TermNumber* tr = def->term()->isNumber();
  if (tr)
  {
    dst = tr->value();
    return true;
  }
  DebugTrace("WARNING: invalid NUMBER {} in '{}'\n", def->name()->value().data(), file);

  return false;
}

bool GetDefVec(Vec3& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing VEC3 TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    if (val->elements()->size() != 3)
      DebugTrace("WARNING: malformed vector in '{}'\n", file);
    else
    {
      dst.x = static_cast<float>(val->elements()->at(0)->isNumber()->value());
      dst.y = static_cast<float>(val->elements()->at(1)->isNumber()->value());
      dst.z = static_cast<float>(val->elements()->at(2)->isNumber()->value());

      return true;
    }
  }
  else
    DebugTrace("WARNING: vector expected in '{}'\n", file);

  return false;
}

bool GetDefRect(Rect& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing RECT TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    if (val->elements()->size() != 4)
      DebugTrace("WARNING: malformed rect in '{}'\n", file);
    else
    {
      dst.x = static_cast<int>(val->elements()->at(0)->isNumber()->value());
      dst.y = static_cast<int>(val->elements()->at(1)->isNumber()->value());
      dst.w = static_cast<int>(val->elements()->at(2)->isNumber()->value());
      dst.h = static_cast<int>(val->elements()->at(3)->isNumber()->value());

      return true;
    }
  }
  else
    DebugTrace("WARNING: rect expected in '{}'\n", file);

  return false;
}

bool GetDefInsets(Insets& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing Insets TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    if (val->elements()->size() != 4)
      DebugTrace("WARNING: malformed Insets in '{}'\n", file);
    else
    {
      dst.left = static_cast<WORD>(val->elements()->at(0)->isNumber()->value());
      dst.right = static_cast<WORD>(val->elements()->at(1)->isNumber()->value());
      dst.top = static_cast<WORD>(val->elements()->at(2)->isNumber()->value());
      dst.bottom = static_cast<WORD>(val->elements()->at(3)->isNumber()->value());

      return true;
    }
  }
  else
    DebugTrace("WARNING: Insets expected in '{}'\n", file);

  return false;
}

bool GetDefColor(Color& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing COLOR TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    if (val->elements()->size() != 3)
      DebugTrace("WARNING: malformed color in '{}'\n", file);
    else
    {
      BYTE r, g, b;
      double v0 = (val->elements()->at(0)->isNumber()->value());
      double v1 = (val->elements()->at(1)->isNumber()->value());
      double v2 = (val->elements()->at(2)->isNumber()->value());

      if (v0 >= 0 && v0 <= 1 && v1 >= 0 && v1 <= 1 && v2 >= 0 && v2 <= 1)
      {
        r = static_cast<BYTE>(v0 * 255);
        g = static_cast<BYTE>(v1 * 255);
        b = static_cast<BYTE>(v2 * 255);
      }
      else
      {
        r = static_cast<BYTE>(v0);
        g = static_cast<BYTE>(v1);
        b = static_cast<BYTE>(v2);
      }

      dst = Color(r, g, b);
      return true;
    }
  }
  else
    DebugTrace("WARNING: color expected in '{}'\n", file);

  return false;
}

bool GetDefColor(ColorValue& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing COLOR TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    if (val->elements()->size() < 3 || val->elements()->size() > 4)
      DebugTrace("WARNING: malformed color in '{}'\n", file);
    else
    {
      double r = (val->elements()->at(0)->isNumber()->value());
      double g = (val->elements()->at(1)->isNumber()->value());
      double b = (val->elements()->at(2)->isNumber()->value());
      double a = 1;

      if (val->elements()->size() == 4)
        a = (val->elements()->at(3)->isNumber()->value());

      dst.Set(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
      return true;
    }
  }
  else
    DebugTrace("WARNING: color expected in '{}'\n", file);

  return false;
}

bool GetDefArray(int* dst, int size, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing ARRAY TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    int nelem = val->elements()->size();

    if (nelem > size)
      nelem = size;

    for (int i = 0; i < nelem; i++)
      *dst++ = static_cast<int>(val->elements()->at(i)->isNumber()->value());

    return true;
  }
  DebugTrace("WARNING: array expected in '{}'\n", file);

  return false;
}

bool GetDefArray(float* dst, int size, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing ARRAY TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    int nelem = val->elements()->size();

    if (nelem > size)
      nelem = size;

    for (int i = 0; i < nelem; i++)
      *dst++ = static_cast<float>(val->elements()->at(i)->isNumber()->value());

    return true;
  }
  DebugTrace("WARNING: array expected in '{}'\n", file);

  return false;
}

bool GetDefArray(double* dst, int size, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing ARRAY TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    int nelem = val->elements()->size();

    if (nelem > size)
      nelem = size;

    for (int i = 0; i < nelem; i++)
      *dst++ = val->elements()->at(i)->isNumber()->value();

    return true;
  }
  DebugTrace("WARNING: array expected in '{}'\n", file);

  return false;
}

bool GetDefArray(std::vector<DWORD>& array, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing ARRAY TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    int nelem = val->elements()->size();

    array.clear();

    for (int i = 0; i < nelem; i++)
      array.push_back(static_cast<DWORD>(val->elements()->at(i)->isNumber()->value()));

    return true;
  }
  DebugTrace("WARNING: integer array expected in '{}'\n", file);

  return false;
}

bool GetDefArray(std::vector<float>& array, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing ARRAY TermDef in '{}'\n", file);
    return false;
  }

  TermArray* val = def->term()->isArray();
  if (val)
  {
    int nelem = val->elements()->size();

    array.clear();

    for (int i = 0; i < nelem; i++)
      array.push_back(static_cast<float>(val->elements()->at(i)->isNumber()->value()));

    return true;
  }
  DebugTrace("WARNING: float array expected in '{}'\n", file);

  return false;
}

bool GetDefTime(int& dst, TermDef* def, std::string_view file)
{
  if (!def || !def->term())
  {
    DebugTrace("WARNING: missing TIME TermDef in '{}'\n", file);
    return false;
  }

  TermText* tn = def->term()->isText();

  if (tn)
  {
    int d = 0;
    int h = 0;
    int m = 0;
    int s = 0;

    char buf[64];
    strcpy_s(buf, tn->value().c_str());

    if (strchr(buf, '/'))
      sscanf_s(buf, "%d/%d:%d:%d", &d, &h, &m, &s);
    else
      sscanf_s(buf, "%d:%d:%d", &h, &m, &s);

    dst = d * 24 * 60 * 60 + h * 60 * 60 + m * 60 + s;

    return true;
  }
  DebugTrace("WARNING: invalid TIME {} in '{}'\n", def->name()->value().data(), file);

  return false;
}

#include "pch.h"
#include "Font.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Polygon.h"
#include "Reader.h"
#include "Video.h"

DWORD GetRealTime();

Font::Font()
  : m_flags(0),
    m_height(0),
    m_baseline(0),
    interspace(0),
    spacewidth(0),
    expansion(0),
    alpha(1),
    blend(Video::BLEND_ALPHA),
    scale(1),
    caret_index(-1),
    caret_x(0),
    caret_y(0),
    imagewidth(0),
    image(nullptr),
    tgt_bitmap(nullptr),
    material(nullptr),
    vset(nullptr),
    polys(nullptr),
    npolys(0)
{
  ZeroMemory(m_name, sizeof(m_name));
  ZeroMemory(m_glyph, sizeof(m_glyph));
  ZeroMemory(m_kern, sizeof(m_kern));
}

Font::Font(const char* n)
  : m_flags(0),
    m_height(0),
    m_baseline(0),
    interspace(0),
    spacewidth(4),
    expansion(0),
    alpha(1),
    blend(Video::BLEND_ALPHA),
    scale(1),
    caret_index(-1),
    caret_x(0),
    caret_y(0),
    imagewidth(0),
    image(nullptr),
    tgt_bitmap(nullptr),
    material(nullptr),
    vset(nullptr),
    polys(nullptr),
    npolys(0)
{
  ZeroMemory(m_glyph, sizeof(m_glyph));
  ZeroMemory(m_kern, sizeof(m_kern));
  CopyMemory(m_name, n, sizeof(m_name));

  if (!Load(m_name))
  {
    m_flags = 0;
    m_height = 0;
    m_baseline = 0;
    interspace = 0;
    spacewidth = 0;
    imagewidth = 0;
    image = nullptr;

    ZeroMemory(m_glyph, sizeof(m_glyph));
    ZeroMemory(m_kern, sizeof(m_kern));
  }
}

Font::~Font()
{
  if (image)
    delete [] image;
  if (vset)
    delete vset;
  if (polys)
    delete [] polys;
  if (material)
    delete material;
}

static char kern_tweak[256][256];

bool Font::Load(const char* name)
{
  if (!name || !name[0])
    return false;

  std::string imgname = std::string(name) + ".pcx";
  std::string defname = std::string(name) + ".def";

  DataLoader* loader = DataLoader::GetLoader();
  if (!loader)
    return false;

  LoadDef(defname, imgname);

  for (int i = 0; i < 256; i++)
  {
    m_glyph[i].offset = GlyphOffset(i);
    m_glyph[i].width = 0;
  }

  if (loader->LoadBitmap(imgname, bitmap))
  {
    if (!bitmap.Pixels() && !bitmap.HiPixels())
      return false;

    scale = bitmap.Width() / 256;
    imagewidth = bitmap.Width();
    if (m_height > bitmap.Height())
      m_height = bitmap.Height();

    int imgsize = bitmap.Width() * bitmap.Height();
    image = NEW BYTE[imgsize];

    if (image)
    {
      if (bitmap.Pixels())
        CopyMemory(image, bitmap.Pixels(), imgsize);

      else
      {
        for (int i = 0; i < imgsize; i++)
          image[i] = static_cast<BYTE>(bitmap.HiPixels()[i].Alpha());
      }
    }

    material = NEW Material;
    material->tex_diffuse = &bitmap;
  }
  else
    return false;

  for (int i = 0; i < 256; i++)
    m_glyph[i].width = CalcWidth(i);

  color = Color::White;

  if (!(m_flags & (FONT_FIXED_PITCH | FONT_NO_KERN)))
    AutoKern();

  for (int i = 0; i < 256; i++)
  {
    for (int j = 0; j < 256; j++)
    {
      if (kern_tweak[i][j] < 100)
        m_kern[i][j] = kern_tweak[i][j];
    }
  }

  return true;
}

void Font::LoadDef(std::string_view defname, std::string& _imgname)
{
  for (int i = 0; i < 256; i++)
  {
    for (int j = 0; j < 256; j++)
      kern_tweak[i][j] = 111;
  }

  DataLoader* loader = DataLoader::GetLoader();
  if (!loader)
    return;

  BYTE* block;
  int blocklen = loader->LoadBuffer(defname, block, true);

  if (!block || blocklen < 4)
    return;

  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("WARNING: could not parse '{}'\n", defname);
    return;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "FONT")
  {
    DebugTrace("WARNING: invalid font def file '{}'\n", defname);
    return;
  }

  do
  {
    delete term;

    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
      {
        if (def->name()->value().starts_with("image"))
          GetDefText(_imgname, def, defname);

        else if (def->name()->value() == "height")
        {
          int h = 0;
          GetDefNumber(h, def, defname);

          if (h >= 0 && h <= 32)
            m_height = static_cast<BYTE>(h);
        }

        else if (def->name()->value() == "baseline")
        {
          int b = 0;
          GetDefNumber(b, def, defname);

          if (b >= 0 && b <= 32)
            m_baseline = static_cast<BYTE>(b);
        }

        else if (def->name()->value() == "flags")
        {
          if (def->term()->isText())
          {
            std::string buf;
            GetDefText(buf, def, defname);

            m_flags = 0;

            if (buf.contains("ALL_CAPS"))
              m_flags = m_flags | FONT_ALL_CAPS;

            if (buf.contains("KERN"))
              m_flags = m_flags | FONT_NO_KERN;

            if (buf.contains("FIXED"))
              m_flags = m_flags | FONT_FIXED_PITCH;
          }

          else
          {
            int f = 0;
            GetDefNumber(f, def, defname);
            m_flags = static_cast<WORD>(f);
          }
        }

        else if (def->name()->value() == "interspace")
        {
          int n = 0;
          GetDefNumber(n, def, defname);

          if (n >= 0 && n <= 100)
            interspace = static_cast<BYTE>(n);
        }

        else if (def->name()->value() == "spacewidth")
        {
          int n = 0;
          GetDefNumber(n, def, defname);

          if (n >= 0 && n <= 100)
            spacewidth = static_cast<BYTE>(n);
        }

        else if (def->name()->value() == "expansion")
          GetDefNumber(expansion, def, defname);

        else if (def->name()->value() == "kern")
        {
          TermStruct* val = def->term()->isStruct();

          std::string a, b;
          int k = 111;

          for (int i = 0; i < val->elements()->size(); i++)
          {
            TermDef* pdef = val->elements()->at(i)->isDef();
            if (pdef)
            {
              if (pdef->name()->value() == "left" || pdef->name()->value() == "a")
                GetDefText(a, pdef, defname);

              else if (pdef->name()->value() == "right" || pdef->name()->value() == "b")
                GetDefText(b, pdef, defname);

              else if (pdef->name()->value() == "kern" || pdef->name()->value() == "k")
                GetDefNumber(k, pdef, defname);
            }
          }

          if (k < 100)
            kern_tweak[a[0]][b[0]] = k;
        }

        else { DebugTrace("WARNING: unknown object '{}' in '{}'\n", def->name()->value().data(), defname); }
      }
      else
      {
        DebugTrace("WARNING: term ignored in '{}'\n", defname);
        term->print();
      }
    }
  }
  while (term);

  loader->ReleaseBuffer(block);
}

static constexpr int pipe_width = 16;
static constexpr int char_width = 16;
static constexpr int char_height = 16;
static constexpr int row_items = 16;
static constexpr int row_width = row_items * char_width;
static constexpr int row_size = char_height * row_width;

int Font::GlyphOffset(BYTE c) const
{
  if (m_flags & FONT_ALL_CAPS)
  {
    if (islower(c))
      c = toupper(c);
  }

  return (c / row_items * row_size * scale * scale + c % row_items * char_width * scale);
}

int Font::GlyphLocationX(BYTE c) const
{
  if (m_flags & FONT_ALL_CAPS)
  {
    if (islower(c))
      c = toupper(c);
  }

  return c % row_items * char_width;
}

int Font::GlyphLocationY(BYTE c) const
{
  if (m_flags & FONT_ALL_CAPS)
  {
    if (islower(c))
      c = toupper(c);
  }

  return c / row_items * char_height;
}

int Font::CalcWidth(BYTE c) const
{
  if (c >= PIPE_NBSP && c <= ARROW_RIGHT)
    return pipe_width;

  if (c >= 128 || !image)
    return 0;

  // all digits should be same size:
  if (isdigit(c))
    c = '0';

  int result = 0;
  int w = 16 * scale;
  int h = 16 * scale;

  BYTE* src = image + GlyphOffset(c);

  for (int y = 0; y < h; y++)
  {
    BYTE* pleft = src;

    for (int x = 0; x < w; x++)
    {
      if (*pleft++ > 0 && x > result)
        result = x;
    }

    src += imagewidth;
  }

  return result + 2;
}

struct FontKernData
{
  double l[32];
  double r[32];
};

void Font::FindEdges(BYTE c, double* l, double* r)
{
  if (!image)
    return;

  int w = m_glyph[c].width;
  int h = m_height;

  h = std::min(h, 32);

  BYTE* src = image + GlyphOffset(c);

  for (int y = 0; y < h; y++)
  {
    BYTE* pleft = src;
    BYTE* pright = src + w - 1;

    *l = -1;
    *r = -1;

    for (int x = 0; x < w; x++)
    {
      if (*l == -1 && *pleft != 0)
        *l = x + 1 - static_cast<double>(*pleft) / 255.0;
      if (*r == -1 && *pright != 0)
        *r = x + 1 - static_cast<double>(*pright) / 255.0;

      pleft++;
      pright--;
    }

    src += imagewidth;
    l++;
    r++;
  }
}

static bool nokern(char c)
{
  if (c <= Font::ARROW_RIGHT)
    return true;

  auto nokernchars = "0123456789+=<>-.,:;?'\"";

  if (strchr(nokernchars, c))
    return true;

  return false;
}

void Font::AutoKern()
{
  auto data = NEW FontKernData[256];

  if (!data)
    return;

  int h = m_height;
  h = std::min(h, 32);

  int i;

  // first, compute row edges for each glyph:

  for (i = 0; i < 256; i++)
  {
    ZeroMemory(&data[i], sizeof(FontKernData));

    int c = i;

    if ((m_flags & FONT_ALL_CAPS) && islower(c))
      c = toupper(c);

    if (m_glyph[static_cast<BYTE>(c)].width > 0)
      FindEdges(static_cast<BYTE>(c), data[i].l, data[i].r);
  }

  // then, compute the appropriate kern for each pair.
  // use a desired average distance of one pixel,
  // with a desired minimum distance of more than half a pixel:

  double desired_avg = 2.5 + expansion;
  double desired_min = 1;

  for (i = 0; i < 256; i++)
  {
    for (int j = 0; j < 256; j++)
    {
      // no kerning between digits or dashes:
      if (nokern(i) || nokern(j))
        m_kern[i][j] = static_cast<char>(0);

      else
      {
        double delta;
        double avg = 0;
        double min = 2500;
        int n = 0;

        for (int y = 0; y < h; y++)
        {
          if (data[i].r[y] >= 0 && data[j].l[y] >= 0)
          {
            delta = data[i].r[y] + data[j].l[y];
            avg += delta;
            min = std::min(delta, min);

            n++;
          }
        }

        if (n > 0)
        {
          avg /= n;

          delta = desired_avg - avg;

          if (delta < desired_min - min)
          {
            delta = ceil(desired_min - min);

            if (i == 'T' && islower(j) && !(m_flags & FONT_ALL_CAPS))
              delta += 1;
          }
        }
        else
          delta = 0;

        m_kern[i][j] = static_cast<char>(delta);
      }
    }
  }

  delete [] data;
}

int Font::CharWidth(char c) const
{
  if (m_flags & FONT_ALL_CAPS)
  {
    if (islower(c))
      c = toupper(c);
  }

  int result;

  if (c >= PIPE_NBSP && c <= ARROW_RIGHT)
    result = pipe_width;

  else if (c < 0 || isspace(c))
    result = spacewidth;

  else
    result = m_glyph[c].width + interspace;

  return result;
}

int Font::SpaceWidth() const { return spacewidth; }

int Font::KernWidth(int a, int b) const
{
  if (m_flags & FONT_ALL_CAPS)
  {
    if (islower(a))
      a = toupper(a);
    if (islower(b))
      b = toupper(b);
  }

  return m_kern[a][b];
}

void Font::SetKern(char a, char b, int k)
{
  if (k < -100 || k > 100)
    return;

  if (m_flags & FONT_ALL_CAPS)
  {
    if (islower(a))
      a = toupper(a);
    if (islower(b))
      b = toupper(b);
  }

  m_kern[a][b] = static_cast<char>(k);
}

int Font::StringWidth(std::string_view _str) const
{
  int result = 0;

  if (_str.empty())
    return result;

  for (auto it = _str.begin(); it != _str.end(); ++it)
  {
    if (int c = (int)(*it); isspace(c) && (c < PIPE_NBSP || c > ARROW_RIGHT))
      result += spacewidth;
    else
    {
      if (m_flags & FONT_ALL_CAPS)
      {
        if (islower(c))
          c = toupper(c);
      }

      int k = 0;
      if (it != _str.end()-1)
        k = m_kern[c][*std::next(it)];

      result += m_glyph[c].width + interspace + k;
    }
  }

  return result;
}

void Font::DrawText(const std::string_view _text, size_t _count, Rect& text_rect, DWORD flags, Bitmap* tgt)
{
  Rect clip_rect = text_rect;

  if (clip_rect.w < 1 || clip_rect.h < 1)
    return;

  tgt_bitmap = tgt;

  if (!_text.empty())
  {
    if (_count < 1)
      _count = _text.size();

    // single line:
    if (flags & DT_SINGLELINE)
      DrawTextSingle(_text, _count, text_rect, clip_rect, flags);

      // multi-line with word wrap:
    else if (flags & DT_WORDBREAK)
      DrawTextWrap(_text, _count, text_rect, clip_rect, flags);

      // multi-line with clip:
    else
      DrawTextMulti(_text, _count, text_rect, clip_rect, flags);
  }
  else
  {
    caret_x = text_rect.x + 2;
    caret_y = text_rect.y + 2;
  }

  // if calc only, update the rectangle:
  if (flags & DT_CALCRECT)
  {
    text_rect.h = clip_rect.h;
    text_rect.w = clip_rect.w;
  }

  // otherwise, draw caret if requested:
  else if (caret_index >= 0 && caret_y >= text_rect.y && caret_y <= text_rect.y + text_rect.h)
  {//caret_y + height < text_rect.y + text_rect.h) {
    Video* video = Video::GetInstance();

    if (video && (GetRealTime() / 500) & 1)
    {
      float v[4];
      v[0] = static_cast<float>(caret_x + 1);
      v[1] = static_cast<float>(caret_y);
      v[2] = static_cast<float>(caret_x + 1);
      v[3] = static_cast<float>(caret_y + m_height);

      video->DrawScreenLines(1, v, color, blend);
    }

    caret_index = -1;
  }

  tgt_bitmap = nullptr;
}

static size_t find_next_word_start(std::string_view text, ptrdiff_t index)
{
  // step through intra-word space:
  auto it = text.begin() + index;
  while (it != text.end() && isspace(*it) && *it != '\n')
  {
    ++it;
  }

  return it - text.begin();
}

static size_t find_next_word_end(std::string_view text, ptrdiff_t index)
{
  // check for leading newline:
  if (text[index] == '\n')
    return index;

  // step through intra-word space:
  while (index < text.size() && isspace(text[index]))
    index++;

  // step through word:
  while (index < text.size() && !isspace(text[index]))
    index++;

  return index - 1;
}

void Font::DrawTextSingle(std::string_view text, int count, const Rect& text_rect, Rect& clip_rect, DWORD flags)
{
  // parse the format flags:
  bool nodraw = (flags & DT_CALCRECT) ? true : false;

  int align = DT_LEFT;
  if (flags & DT_RIGHT)
    align = DT_RIGHT;
  else if (flags & DT_CENTER)
    align = DT_CENTER;

  int valign = DT_TOP;
  if (flags & DT_BOTTOM)
    valign = DT_BOTTOM;
  else
    if (flags & DT_VCENTER)
      valign = DT_VCENTER;

  int xoffset = 0;
  int yoffset = 0;

  int length = StringWidth(text);
  if (length < text_rect.w)
  {
    switch (align)
    {
      default: case DT_LEFT:
        break;
      case DT_RIGHT:
        xoffset = text_rect.w - length;
        break;
      case DT_CENTER:
        xoffset = (text_rect.w - length) / 2;
        break;
    }
  }

  if (Height() < text_rect.h)
  {
    switch (valign)
    {
      default: case DT_TOP:
        break;
      case DT_BOTTOM:
        yoffset = text_rect.h - Height();
        break;
      case DT_VCENTER:
        yoffset = (text_rect.h - Height()) / 2;
        break;
    }
  }

  int max_width = length;

  // if calc only, update the rectangle:
  if (nodraw)
  {
    clip_rect.h = Height();
    clip_rect.w = max_width;
  }

  // otherwise, draw the string now:
  else
  {
    int x1 = text_rect.x + xoffset;
    int y1 = text_rect.y + yoffset;

    DrawString(text, count, x1, y1, text_rect);
  }

  if (caret_index >= 0 && caret_index <= count)
  {
    caret_x = text_rect.x + xoffset;
    caret_y = text_rect.y + yoffset;

    if (caret_index > 0)
      caret_x += StringWidth(text.substr(0, caret_index));
  }

  else
  {
    caret_x = text_rect.x + 0;
    caret_y = text_rect.y + 0;
  }
}

void Font::DrawTextWrap(std::string_view _text, int _count, const Rect& text_rect, Rect& clip_rect, DWORD flags)
{
  // parse the format flags:
  bool nodraw = (flags & DT_CALCRECT) ? true : false;

  int align = DT_LEFT;
  if (flags & DT_RIGHT)
    align = DT_RIGHT;
  else if (flags & DT_CENTER)
    align = DT_CENTER;

  int nlines = 0;
  int max_width = 0;

  int line_start = 0;
  int count_remaining = _count;
  size_t curr_word_end = -1;
  size_t next_word_end;
  size_t eol_index = 0;

  int yoffset = 0;

  caret_x = -1;
  caret_y = -1;

  // repeat for each line of text:
  while (count_remaining > 0)
  {
    int length = 0;

    // find the end of the last whole word that fits on the line:
    for (;;)
    {
      next_word_end = find_next_word_end(_text, curr_word_end + 1);

      if (next_word_end < 0 || next_word_end == curr_word_end)
        break;

      if (_text[next_word_end] == '\n')
      {
        eol_index = curr_word_end = next_word_end;
        break;
      }

      int word_len = next_word_end - line_start + 1;

      length = StringWidth(_text.substr(line_start, word_len));

      if (length < text_rect.w)
      {
        curr_word_end = next_word_end;

        // check for a newline in the next block of white space:
        eol_index = 0;
        auto eol =  _text.begin() + curr_word_end + 1;
        while (eol != _text.end() && isspace(*eol) && *eol != '\n')
          ++eol;

        if (eol == _text.end())
          break;

        if (*eol == '\n')
        {
          eol_index = eol - _text.begin();
          break;
        }
      }
      else
        break;
    }

    int line_count = curr_word_end - line_start + 1;

    if (line_count > 0)
      length = StringWidth(_text.substr(line_start, line_count));

    // there was a single word longer than the entire line:
    else
    {
      line_count = next_word_end - line_start + 1;
      length = StringWidth(_text.substr(line_start, line_count));
      curr_word_end = next_word_end;
    }

    int xoffset = 0;
    if (length < text_rect.w)
    {
      switch (align)
      {
        default: case DT_LEFT:
          break;
        case DT_RIGHT:
          xoffset = text_rect.w - length;
          break;
        case DT_CENTER:
          xoffset = (text_rect.w - length) / 2;
          break;
      }
    }

    max_width = std::max(length, max_width);

    if (eol_index > 0)
      curr_word_end = eol_index;

    size_t next_line_start = find_next_word_start(_text, curr_word_end + 1);

    if (length > 0 && !nodraw)
    {
      int x1 = text_rect.x + xoffset;
      int y1 = text_rect.y + yoffset;

      DrawString(_text.substr(line_start), line_count, x1, y1, text_rect);

      if (caret_index == line_start)
      {
        caret_x = x1 - 2;
        caret_y = y1;
      }
      else if (caret_index > line_start && caret_index < next_line_start)
      {
        caret_x = text_rect.x + xoffset + StringWidth(_text.substr(line_start, caret_index - line_start)) - 2;
        caret_y = text_rect.y + yoffset;
      }
      else if (caret_index == _count)
      {
        if (_text[_count - 1] == '\n')
        {
          caret_x = x1 - 2;
          caret_y = y1 + m_height;
        }
        else
        {
          caret_x = text_rect.x + xoffset + StringWidth(_text.substr(line_start,  caret_index - line_start)) - 2;
          caret_y = text_rect.y + yoffset;
        }
      }
    }

    nlines++;
    yoffset += Height();
    if (eol_index > 0)
      curr_word_end = eol_index;
    line_start = find_next_word_start(_text, curr_word_end + 1);
    count_remaining = _count - line_start;
  }

  // if calc only, update the rectangle:
  if (nodraw)
  {
    clip_rect.h = nlines * Height();
    clip_rect.w = max_width;
  }
}

void Font::DrawTextMulti(std::string_view text, int count, const Rect& text_rect, Rect& clip_rect, DWORD flags)
{
  // parse the format flags:
  bool nodraw = (flags & DT_CALCRECT) ? true : false;

  int align = DT_LEFT;
  if (flags & DT_RIGHT)
    align = DT_RIGHT;
  else if (flags & DT_CENTER)
    align = DT_CENTER;

  int max_width = 0;
  int line_start = 0;
  int count_remaining = count;

  int yoffset = 0;
  int nlines = 0;

  // repeat for each line of text:
  while (count_remaining > 0)
  {
    int length = 0;
    int line_count = 0;

    // find the end of line:
    while (line_count < count_remaining)
    {
      char c = text[line_start + line_count];
      if (!c || c == '\n')
        break;

      line_count++;
    }

    if (line_count > 0)
      length = StringWidth(text.substr(line_start, line_count));

    int xoffset = 0;
    if (length < text_rect.w)
    {
      switch (align)
      {
        default: case DT_LEFT:
          break;
        case DT_RIGHT:
          xoffset = text_rect.w - length;
          break;
        case DT_CENTER:
          xoffset = (text_rect.w - length) / 2;
          break;
      }
    }

    if (length > max_width)
      max_width = length;

    if (length && !nodraw)
    {
      int x1 = text_rect.x + xoffset;
      int y1 = text_rect.y + yoffset;

      DrawString(text.substr(line_start), line_count, x1, y1, text_rect);
    }

    nlines++;
    yoffset += Height();

    if (line_start + line_count + 1 < count)
    {
      line_start = find_next_word_start(text, line_start + line_count + 1);
      count_remaining = count - line_start;
    }
    else
      count_remaining = 0;
  }

  // if calc only, update the rectangle:
  if (nodraw)
  {
    clip_rect.h = nlines * Height();
    clip_rect.w = max_width;
  }
}

int Font::DrawString(std::string_view str, int len, int x1, int y1, const Rect& clip, Bitmap* tgt)
{
  Video* video = Video::GetInstance();
  int count = 0;
  int maxw = clip.w;
  int maxh = clip.h;

  if (len < 1 || !video)
    return count;

  // vertical clip
  if ((y1 < clip.y) || (y1 > clip.y + clip.h))
    return count;

  // RENDER TO BITMAP

  if (!tgt)
    tgt = tgt_bitmap;

  if (tgt)
  {
    for (int i = 0; i < len; i++)
    {
      char c = str[i];

      if ((m_flags & FONT_ALL_CAPS) && islower(c))
        c = toupper(c);

      int cw = m_glyph[c].width + interspace;
      int ch = m_height;
      int k = 0;

      if (i < len - 1)
        k = m_kern[c][str[i + 1]];

      // horizontal clip:
      if (x1 < clip.x)
      {
        if (isspace(c) && (c < PIPE_NBSP || c > ARROW_RIGHT))
        {
          x1 += spacewidth;
          maxw -= spacewidth;
        }
        else
        {
          x1 += cw + k;
          maxw -= cw + k;
        }
      }
      else if (x1 + cw > clip.x + clip.w)
        return count;
      else
      {
        if (isspace(c) && (c < PIPE_NBSP || c > ARROW_RIGHT))
        {
          x1 += spacewidth;
          maxw -= spacewidth;
        }
        else
        {
          int sx = GlyphLocationX(c);
          int sy = GlyphLocationY(c);

          Color* srcpix = bitmap.HiPixels();
          Color* dstpix = tgt->HiPixels();
          if (srcpix && dstpix)
          {
            int spitch = bitmap.Width();
            int dpitch = tgt->Width();

            Color* dst = dstpix + (y1 * dpitch) + x1;
            Color* src = srcpix + (sy * spitch) + sx;

            for (int i = 0; i < ch; i++)
            {
              Color* ps = src;
              Color* pd = dst;

              for (int n = 0; n < cw; n++)
              {
                DWORD alpha = ps->Alpha();
                if (alpha)
                  *pd = color.dim(alpha / 240.0);
                ps++;
                pd++;
              }

              dst += dpitch;
              src += spitch;
            }
          }
          else
          {
            // this probably won't work...
            tgt->BitBlt(x1, y1, bitmap, sx, sy, cw, ch, true);
          }

          x1 += cw + k;
          maxw -= cw + k;
        }

        count++;
      }
    }
    return count;
  }

  // RENDER TO VIDEO

  // allocate verts, if necessary
  int nverts = 4 * len;
  if (!vset)
  {
    vset = NEW VertexSet(nverts);

    if (!vset)
      return false;

    vset->space = VertexSet::SCREEN_SPACE;

    for (int v = 0; v < vset->nverts; v++)
    {
      vset->s_loc[v].z = 0.0f;
      vset->rw[v] = 1.0f;
    }
  }
  else if (vset->nverts < nverts)
  {
    vset->Resize(nverts);

    for (int v = 0; v < vset->nverts; v++)
    {
      vset->s_loc[v].z = 0.0f;
      vset->rw[v] = 1.0f;
    }
  }

  if (vset->nverts < nverts)
    return count;

  if (alpha < 1)
    color.SetAlpha(static_cast<BYTE>(alpha * 255.0f));
  else
    color.SetAlpha(255);

  for (int i = 0; i < len; i++)
  {
    char c = str[i];

    if ((m_flags & FONT_ALL_CAPS) && islower(c))
      c = toupper(c);

    int cw = m_glyph[c].width + interspace;
    int k = 0;

    if (i < len - 1)
      k = m_kern[c][str[i + 1]];

    // horizontal clip:
    if (x1 < clip.x)
    {
      if (isspace(c) && (c < PIPE_NBSP || c > ARROW_RIGHT))
      {
        x1 += spacewidth;
        maxw -= spacewidth;
      }
      else
      {
        x1 += cw + k;
        maxw -= cw + k;
      }
    }
    else if (x1 + cw > clip.x + clip.w)
      break;
    else
    {
      if (isspace(c) && (c < PIPE_NBSP || c > ARROW_RIGHT))
      {
        x1 += spacewidth;
        maxw -= spacewidth;
      }
      else
      {
        // create four verts for this character:
        int v = count * 4;
        double char_x = GlyphLocationX(c);
        double char_y = GlyphLocationY(c);
        double char_w = m_glyph[c].width;
        double char_h = m_height;

        if (y1 + char_h > clip.y + clip.h)
          char_h = clip.y + clip.h - y1;

        vset->s_loc[v + 0].x = static_cast<float>(x1 - 0.5);
        vset->s_loc[v + 0].y = static_cast<float>(y1 - 0.5);
        vset->tu[v + 0] = static_cast<float>(char_x / 256);
        vset->tv[v + 0] = static_cast<float>(char_y / 256);
        vset->diffuse[v + 0] = color.Value();

        vset->s_loc[v + 1].x = static_cast<float>(x1 + char_w - 0.5);
        vset->s_loc[v + 1].y = static_cast<float>(y1 - 0.5);
        vset->tu[v + 1] = static_cast<float>(char_x / 256 + char_w / 256);
        vset->tv[v + 1] = static_cast<float>(char_y / 256);
        vset->diffuse[v + 1] = color.Value();

        vset->s_loc[v + 2].x = static_cast<float>(x1 + char_w - 0.5);
        vset->s_loc[v + 2].y = static_cast<float>(y1 + char_h - 0.5);
        vset->tu[v + 2] = static_cast<float>(char_x / 256 + char_w / 256);
        vset->tv[v + 2] = static_cast<float>(char_y / 256 + char_h / 256);
        vset->diffuse[v + 2] = color.Value();

        vset->s_loc[v + 3].x = static_cast<float>(x1 - 0.5);
        vset->s_loc[v + 3].y = static_cast<float>(y1 + char_h - 0.5);
        vset->tu[v + 3] = static_cast<float>(char_x / 256);
        vset->tv[v + 3] = static_cast<float>(char_y / 256 + char_h / 256);
        vset->diffuse[v + 3] = color.Value();

        x1 += cw + k;
        maxw -= cw + k;

        count++;
      }
    }
  }

  if (count)
  {
    // this small hack is an optimization to reduce the 
    // size of vertex buffer needed for font rendering:

    int old_nverts = vset->nverts;
    vset->nverts = 4 * count;

    // create a larger poly array, if necessary:
    if (count > npolys)
    {
      if (polys)
        delete [] polys;

      npolys = count;
      polys = NEW Poly[npolys];
      Poly* p = polys;
      int index = 0;

      for (int i = 0; i < npolys; i++)
      {
        p->nverts = 4;
        p->vertex_set = vset;
        p->material = material;
        p->verts[0] = index++;
        p->verts[1] = index++;
        p->verts[2] = index++;
        p->verts[3] = index++;

        p++;
      }
    }

    video->DrawScreenPolys(count, polys, blend);

    // remember to restore the proper size of the vertex set:
    vset->nverts = old_nverts;
  }

  return count;
}

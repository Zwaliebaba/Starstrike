#ifndef Font_h
#define Font_h

#include "Bitmap.h"
#include "Color.h"
#include "Geometry.h"

#undef DrawText

struct Poly;
struct Material;
struct VertexSet;
class Video;

struct FontChar
{
  short offset;
  short width;
};

class Font
{
  public:
    
    enum FLAGS
    {
      FONT_FIXED_PITCH = 1,
      FONT_ALL_CAPS    = 2,
      FONT_NO_KERN     = 4
    };

    enum CHARS
    {
      PIPE_NBSP   = 16,
      PIPE_VERT   = 17,
      PIPE_LT     = 18,
      PIPE_TEE    = 19,
      PIPE_UL     = 20,
      PIPE_LL     = 21,
      PIPE_HORZ   = 22,
      PIPE_PLUS   = 23,
      PIPE_MINUS  = 24,
      ARROW_UP    = 25,
      ARROW_DOWN  = 26,
      ARROW_LEFT  = 27,
      ARROW_RIGHT = 28
    };

    // default constructor:
    Font();
    Font(const char* name);
    ~Font();

    bool Load(const char* name);

    int CharWidth(char c) const;
    int SpaceWidth() const;
    int KernWidth(int left, int right) const;
    int StringWidth(std::string_view _str) const;

    void DrawText(std::string_view txt, size_t _count, Rect& txt_rect, DWORD flags, Bitmap* tgt_bitmap = nullptr);
    int DrawString(std::string_view txt, int len, int x1, int y1, const Rect& clip, Bitmap* tgt_bitmap = nullptr);

    int Height() const { return m_height; }
    int Baseline() const { return m_baseline; }
    WORD GetFlags() const { return m_flags; }
    void SetFlags(WORD s) { m_flags = s; }
    Color GetColor() const { return color; }
    void SetColor(const Color& c) { color = c; }
    double GetExpansion() const { return expansion; }
    void SetExpansion(double e) { expansion = static_cast<float>(e); }
    double GetAlpha() const { return alpha; }
    void SetAlpha(double a) { alpha = static_cast<float>(a); }
    int GetBlend() const { return blend; }
    void SetBlend(int b) { blend = b; }

    void SetKern(char left, char right, int k = 0);

    int GetCaretIndex() const { return caret_index; }
    void SetCaretIndex(int n) { caret_index = n; }

  private:
    void AutoKern();
    void FindEdges(BYTE c, double* l, double* r);
    int CalcWidth(BYTE c) const;
    int GlyphOffset(BYTE c) const;
    int GlyphLocationX(BYTE c) const;
    int GlyphLocationY(BYTE c) const;

    void DrawTextSingle(std::string_view txt, int count, const Rect& txt_rect, Rect& clip_rect, DWORD flags);
    void DrawTextWrap(std::string_view txt, int _count, const Rect& txt_rect, Rect& clip_rect, DWORD flags);
    void DrawTextMulti(std::string_view txt, int count, const Rect& txt_rect, Rect& clip_rect, DWORD flags);

    void LoadDef(std::string_view defname, std::string& _imgname);

    char m_name[64];
    WORD m_flags;
    BYTE m_height;
    BYTE m_baseline;
    BYTE interspace;
    BYTE spacewidth;
    float expansion;
    float alpha;
    int blend;
    int scale;

    int caret_index;
    int caret_x;
    int caret_y;

    int imagewidth;
    BYTE* image;
    Bitmap bitmap;
    Bitmap* tgt_bitmap;
    Material* material;
    VertexSet* vset;
    Poly* polys;
    int npolys;

    FontChar m_glyph[256];
    Color color;

    char m_kern[256][256];
};

#endif Font_h

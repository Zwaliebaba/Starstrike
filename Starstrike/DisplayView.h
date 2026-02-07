#ifndef DisplayView_h
#define DisplayView_h

#include "View.h"
#include "SimObject.h"
#include "Color.h"

class Bitmap;
class DisplayElement;
class Font;

class DisplayView : public View
{
  public:
    DisplayView(Window* c);
    virtual ~DisplayView();

    // Operations:
    virtual void Refresh();
    virtual void OnWindowMove();
    virtual void ExecFrame();
    virtual void ClearDisplay();

    virtual void AddText(std::string_view txt, Font* font, Color _color, const Rect& rect, double hold = 1e9,
                         double fade_in = 0, double fade_out = 0);

    virtual void AddImage(Bitmap* bmp, Color color, int blend, const Rect& rect, double hold = 1e9, double fade_in = 0,
                          double fade_out = 0);

    static DisplayView* GetInstance();

  protected:
    int width, height;
    double xcenter, ycenter;

    List<DisplayElement> elements;
};

#endif DisplayView_h

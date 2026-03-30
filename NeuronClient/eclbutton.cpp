#include "pch.h"
#include "eclipse.h"
#include "eclbutton.h"

EclButton::EclButton()
  : m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_caption(nullptr),
    m_tooltip(nullptr),
    m_parent(nullptr)
{
  strncpy(m_name, "New Button", SIZE_ECLBUTTON_NAME);
  m_name[SIZE_ECLBUTTON_NAME - 1] = '\0';
  SetTooltip(" ");
}

EclButton::~EclButton()
{
  if (m_caption)
    delete[] m_caption;
  if (m_tooltip)
    delete[] m_tooltip;
}

void EclButton::SetProperties(const char* _name, int _x, int _y, int _w, int _h, const char* _caption, const char* _tooltip)
{
  if (!_caption)
    _caption = _name;

  if (strlen(_name) > SIZE_ECLBUTTON_NAME) {}
  else
    strncpy(m_name, _name, SIZE_ECLBUTTON_NAME);
    m_name[SIZE_ECLBUTTON_NAME - 1] = '\0';

  m_x = _x;
  m_y = _y;
  m_w = _w;
  m_h = _h;
  SetCaption(_caption);
  SetTooltip(_tooltip);
}

void EclButton::SetCaption(const char* _caption)
{
  if (m_caption)
    delete [] m_caption;
  if (_caption)
  {
    size_t len = strlen(_caption) + 1;
    m_caption = new char [len];
    memcpy(m_caption, _caption, len);
  }
  else
  {
    m_caption = new char[1];
    *m_caption = '\0';
  }
}

void EclButton::SetTooltip(const char* _tooltip)
{
  if (!_tooltip)
    _tooltip = "";
  if (m_tooltip)
    delete [] m_tooltip;
  size_t len = strlen(_tooltip) + 1;
  m_tooltip = new char [len];
  memcpy(m_tooltip, _tooltip, len);
}

void EclButton::SetParent(EclWindow* _parent) { m_parent = _parent; }

void EclButton::Render([[maybe_unused]] int realX, [[maybe_unused]] int realY, [[maybe_unused]] bool highlighted, [[maybe_unused]] bool clicked) {}

void EclButton::MouseUp() {}

void EclButton::MouseDown() {}

void EclButton::MouseMove() {}

void EclButton::Keypress([[maybe_unused]] int keyCode, [[maybe_unused]] bool shift, [[maybe_unused]] bool ctrl, [[maybe_unused]] bool alt) {}

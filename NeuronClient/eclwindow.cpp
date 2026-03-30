#include "pch.h"
#include "eclipse.h"
#include "eclwindow.h"

EclWindow::EclWindow(const char* _name)
  : m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_resizable(true),
    m_dirty(false)
{
  SetName(_name);
  SetTitle("New Window");
  SetMovable(true);
  strncpy(m_currentTextEdit, "None", SIZE_ECLWINDOW_NAME);
  m_currentTextEdit[SIZE_ECLWINDOW_NAME - 1] = '\0';
}

EclWindow::~EclWindow()
{
  while (m_buttons.GetData(0))
  {
    EclButton* button = m_buttons.GetData(0);
    delete button;
    m_buttons.RemoveData(0);
  }
  m_buttons.Empty();
}

void EclWindow::SetName(const char* _name)
{
  if (strlen(_name) > SIZE_ECLWINDOW_NAME)
    return;

  strncpy(m_name, _name, SIZE_ECLWINDOW_NAME);
  m_name[SIZE_ECLWINDOW_NAME - 1] = '\0';
}

void EclWindow::SetTitle(const char* _title)
{
  if (strlen(_title) > SIZE_ECLWINDOW_TITLE)
    return;

  strncpy(m_title, _title, SIZE_ECLWINDOW_TITLE);
  m_title[SIZE_ECLWINDOW_TITLE - 1] = '\0';
  EclDirtyWindow(this);
}

void EclWindow::SetPosition(int _x, int _y)
{
  m_x = _x;
  m_y = _y;
  EclDirtyWindow(this);
}

void EclWindow::SetSize(int _w, int _h)
{
  m_w = _w;
  m_h = _h;
  EclDirtyWindow(this);
}

void EclWindow::SetMovable(bool _movable) { m_movable = _movable; }

void EclWindow::MakeAllOnScreen()
{
  int screenW = EclGetScreenW();
  int screenH = EclGetScreenH();
  if (m_x < 10)
    m_x = 10;
  if (m_y < 10)
    m_y = 10;
  if (m_x + m_w > screenW - 10)
    m_x = screenW - m_w - 10;
  if (m_y + m_h > screenH - 10)
    m_y = screenH - m_h - 10;
}

void EclWindow::BeginTextEdit(const char* _name) { strncpy(m_currentTextEdit, _name, SIZE_ECLWINDOW_NAME); m_currentTextEdit[SIZE_ECLWINDOW_NAME - 1] = '\0'; }

void EclWindow::EndTextEdit() { strncpy(m_currentTextEdit, "None", SIZE_ECLWINDOW_NAME); m_currentTextEdit[SIZE_ECLWINDOW_NAME - 1] = '\0'; }

void EclWindow::RegisterButton(EclButton* button)
{
  button->SetParent(this);

  m_buttons.PutDataAtStart(button);

  if (button->m_y + button->m_h + 10 > m_h)
  {
    SetSize(m_w, button->m_y + button->m_h + 10);
    MakeAllOnScreen();
  }

  EclDirtyWindow(this);
}

void EclWindow::RemoveButton(const char* _name)
{
  for (int i = 0; i < m_buttons.Size(); ++i)
  {
    EclButton* button = m_buttons.GetData(i);
    if (strcmp(button->m_name, _name) == 0)
    {
      m_buttons.RemoveData(i);
      delete button;
    }
  }
}

EclButton* EclWindow::GetButton(const char* _name)
{
  for (int i = 0; i < m_buttons.Size(); ++i)
  {
    EclButton* button = m_buttons.GetData(i);
    if (strcmp(button->m_name, _name) == 0)
      return button;
  }

  return nullptr;
}

EclButton* EclWindow::GetButton(int _x, int _y)
{
  for (int i = 0; i < m_buttons.Size(); ++i)
  {
    EclButton* button = m_buttons.GetData(i);

    if (_x >= button->m_x && _x <= button->m_x + button->m_w && _y >= button->m_y && _y <= button->m_y + button->m_h)
      return button;
  }

  return nullptr;
}

void EclWindow::Create() {}

void EclWindow::Remove() {}

void EclWindow::Update() {}

void EclWindow::Keypress(int keyCode, bool shift, bool ctrl, bool alt)
{
  EclButton* currentTextEdit = GetButton(m_currentTextEdit);
  if (currentTextEdit)
    currentTextEdit->Keypress(keyCode, shift, ctrl, alt);
}

void EclWindow::MouseEvent([[maybe_unused]] bool lmb, [[maybe_unused]] bool rmb, [[maybe_unused]] bool up, [[maybe_unused]] bool down) {}

void EclWindow::Render(bool hasFocus)
{
  for (int i = m_buttons.Size() - 1; i >= 0; --i)
  {
    EclButton* button = m_buttons.GetData(i);
    bool highlighted = EclMouseInButton(this, button) || strcmp(m_currentTextEdit, button->m_name) == 0;
    bool clicked = (hasFocus && strcmp(EclGetCurrentClickedButton(), button->m_name) == 0);
    button->Render(m_x + button->m_x, m_y + button->m_y, highlighted, clicked);
  }
}

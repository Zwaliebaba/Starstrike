#include "pch.h"
#include "prefs_graphics_window.h"
#include "GameApp.h"
#include "drop_down_menu.h"
#include "language_table.h"
#include "level_file.h"
#include "location.h"
#include "preferences.h"
#include "profiler.h"
#include "renderer.h"
#include "resource.h"
#include "text_renderer.h"
#include "water.h"

#define GRAPHICS_LANDDETAIL         "RenderLandscapeDetail"
#define GRAPHICS_WATERDETAIL        "RenderWaterDetail"
#define GRAPHICS_PIXELRANGE         "RenderPixelShader"
#define GRAPHICS_BUILDINGDETAIL     "RenderBuildingDetail"
#define GRAPHICS_ENTITYDETAIL       "RenderEntityDetail"
#define GRAPHICS_CLOUDDETAIL        "RenderCloudDetail"

class ApplyGraphicsButton : public DarwiniaButton
{
  public:
    void MouseUp() override
    {
      if (g_context->m_location)
      {
        auto parent = static_cast<PrefsGraphicsWindow*>(m_parent);

        g_prefsManager->SetInt(GRAPHICS_LANDDETAIL, parent->m_landscapeDetail);
        g_prefsManager->SetInt(GRAPHICS_BUILDINGDETAIL, parent->m_buildingDetail);
        g_prefsManager->SetInt(GRAPHICS_ENTITYDETAIL, parent->m_entityDetail);
        g_prefsManager->SetInt(GRAPHICS_CLOUDDETAIL, parent->m_cloudDetail);

        LandscapeDef* def = &g_context->m_location->m_levelFile->m_landscape;
        g_context->m_location->m_landscape.Init(def);

        delete g_context->m_location->m_water;
        g_context->m_location->m_water = new Water();

        for (int i = 0; i < g_context->m_location->m_buildings.Size(); ++i)
        {
          if (g_context->m_location->m_buildings.ValidIndex(i))
          {
            Building* building = g_context->m_location->m_buildings[i];
            building->SetDetail(parent->m_buildingDetail);
          }
        }

        g_prefsManager->Save();
      }
    }
};

PrefsGraphicsWindow::PrefsGraphicsWindow()
  : DarwiniaWindow(LANGUAGEPHRASE("dialog_graphicsoptions"))
{
  SetMenuSize(360, 300);
  SetPosition(g_context->m_renderer->ScreenW() / 2 - m_w / 2, g_context->m_renderer->ScreenH() / 2 - m_h / 2);

  m_landscapeDetail = g_prefsManager->GetInt(GRAPHICS_LANDDETAIL, 1);
  m_cloudDetail = g_prefsManager->GetInt(GRAPHICS_CLOUDDETAIL, 1);
  m_buildingDetail = g_prefsManager->GetInt(GRAPHICS_BUILDINGDETAIL, 1);
  m_entityDetail = g_prefsManager->GetInt(GRAPHICS_ENTITYDETAIL, 1);
}

PrefsGraphicsWindow::~PrefsGraphicsWindow() {}

void PrefsGraphicsWindow::Create()
{
  DarwiniaWindow::Create();

  int fontSize = GetMenuSize(11);
  int y = GetClientRectY1();
  int border = GetClientRectX1() + 10;
  int x = m_w * 0.6;
  int buttonH = GetMenuSize(20);
  int buttonW = m_w - x - border * 2;
  int buttonW2 = m_w / 2 - border * 2;
  int h = buttonH + border;

  auto box = new InvertedBox();
  box->SetShortProperties("invert", 10, y + border / 2, m_w - 20, GetMenuSize(190));
  RegisterButton(box);

  auto landDetail = new DropDownMenu();
  landDetail->SetShortProperties(LANGUAGEPHRASE("dialog_landscapedetail"), x, y += border, buttonW, buttonH);
  landDetail->AddOption(LANGUAGEPHRASE("dialog_high"), 1);
  landDetail->AddOption(LANGUAGEPHRASE("dialog_medium"), 2);
  landDetail->AddOption(LANGUAGEPHRASE("dialog_low"), 3);
  landDetail->AddOption(LANGUAGEPHRASE("dialog_upgrade"), 4);
  landDetail->RegisterInt(&m_landscapeDetail);
  landDetail->m_fontSize = fontSize;
  RegisterButton(landDetail);
  m_buttonOrder.PutData(landDetail);

  auto cloudDetail = new DropDownMenu();
  cloudDetail->SetShortProperties(LANGUAGEPHRASE("dialog_skydetail"), x, y += h, buttonW, buttonH);
  cloudDetail->AddOption(LANGUAGEPHRASE("dialog_high"), 1);
  cloudDetail->AddOption(LANGUAGEPHRASE("dialog_medium"), 2);
  cloudDetail->AddOption(LANGUAGEPHRASE("dialog_low"), 3);
  cloudDetail->RegisterInt(&m_cloudDetail);
  cloudDetail->m_fontSize = fontSize;
  RegisterButton(cloudDetail);
  m_buttonOrder.PutData(cloudDetail);

  auto buildingDetail = new DropDownMenu();
  buildingDetail->SetShortProperties(LANGUAGEPHRASE("dialog_buildingdetail"), x, y += h, buttonW, buttonH);
  buildingDetail->AddOption(LANGUAGEPHRASE("dialog_high"), 1);
  buildingDetail->AddOption(LANGUAGEPHRASE("dialog_medium"), 2);
  buildingDetail->AddOption(LANGUAGEPHRASE("dialog_low"), 3);
  buildingDetail->RegisterInt(&m_buildingDetail);
  buildingDetail->m_fontSize = fontSize;
  RegisterButton(buildingDetail);
  m_buttonOrder.PutData(buildingDetail);

  auto entityDetail = new DropDownMenu();
  entityDetail->SetShortProperties(LANGUAGEPHRASE("dialog_entitydetail"), x, y += h, buttonW, buttonH);
  entityDetail->AddOption(LANGUAGEPHRASE("dialog_high"), 1);
  entityDetail->AddOption(LANGUAGEPHRASE("dialog_medium"), 2);
  entityDetail->AddOption(LANGUAGEPHRASE("dialog_low"), 3);
  entityDetail->RegisterInt(&m_entityDetail);
  entityDetail->m_fontSize = fontSize;
  RegisterButton(entityDetail);
  m_buttonOrder.PutData(entityDetail);

  auto cancel = new CloseButton();
  cancel->SetShortProperties(LANGUAGEPHRASE("dialog_close"), border, m_h - h, buttonW2, buttonH);
  cancel->m_fontSize = GetMenuSize(13);
  cancel->m_centered = true;
  RegisterButton(cancel);
  m_buttonOrder.PutData(cancel);

  auto apply = new ApplyGraphicsButton();
  apply->SetShortProperties(LANGUAGEPHRASE("dialog_apply"), m_w - buttonW2 - border, m_h - h, buttonW2, buttonH);
  apply->m_fontSize = GetMenuSize(13);
  apply->m_centered = true;
  RegisterButton(apply);
  m_buttonOrder.PutData(apply);
}

void RenderCPUUsage(LList<const char*>* elements, int x, int y)
{
#ifdef PROFILER_ENABLED
  float totalOccup = 0.0f;
  for (int i = 0; i < elements->Size(); ++i)
  {
    ProfiledElement* element = g_context->m_profiler->m_rootElement->m_children.GetData(elements->GetData(i));
    if (element && element->m_lastNumCalls > 0)
    {
      float occup = element->m_lastTotalTime * 100;
      totalOccup += occup;
    }
  }

  if (totalOccup > 0.0f)
  {
    //if( totalOccup > 25 ) glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
    g_editorFont.DrawText2DCenter(x, y, 15, "%d%%", static_cast<int>(totalOccup));
  }
  else
  {
    glColor4f(1.0f, 0.3f, 0.3f, 1.0f);
    g_editorFont.DrawText2DCenter(x, y, 15, "-");
  }
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
#endif
}

void PrefsGraphicsWindow::Render(bool _hasFocus)
{
  DarwiniaWindow::Render(_hasFocus);

  int border = GetClientRectX1() + 10;
  int h = GetMenuSize(20) + border;
  int x = m_x + 20;
  int y = m_y + GetClientRectY1() + border;
  int size = GetMenuSize(13);
  LList<const char*> elements;

  g_editorFont.DrawText2D(x, y += border, size, LANGUAGEPHRASE("dialog_landscapedetail"));
  g_editorFont.DrawText2D(x, y += h, size, LANGUAGEPHRASE("dialog_skydetail"));
  g_editorFont.DrawText2D(x, y += h, size, LANGUAGEPHRASE("dialog_buildingdetail"));
  g_editorFont.DrawText2D(x, y += h, size, LANGUAGEPHRASE("dialog_entitydetail"));

  auto fpsCaption = std::format("{} FPS", g_context->m_renderer->m_fps);
  g_editorFont.DrawText2DCenter(m_x + m_w / 2, m_y + m_h - GetMenuSize(60), GetMenuSize(20), fpsCaption.c_str());
}

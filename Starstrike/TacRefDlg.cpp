#include "pch.h"
#include "TacRefDlg.h"
#include "Button.h"
#include "EventDispatch.h"
#include "FormatUtil.h"
#include "Game.h"
#include "Light.h"
#include "ListBox.h"
#include "MenuScreen.h"
#include "Mouse.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Solid.h"
#include "WeaponDesign.h"
#include "WeaponGroup.h"

DEF_MAP_CLIENT(TacRefDlg, OnClose);
DEF_MAP_CLIENT(TacRefDlg, OnSelect);
DEF_MAP_CLIENT(TacRefDlg, OnMode);
DEF_MAP_CLIENT(TacRefDlg, OnCamRButtonDown);
DEF_MAP_CLIENT(TacRefDlg, OnCamRButtonUp);
DEF_MAP_CLIENT(TacRefDlg, OnCamMove);
DEF_MAP_CLIENT(TacRefDlg, OnCamZoom);

TacRefDlg::TacRefDlg(Screen* s, FormDef& def, MenuScreen* mgr)
  : FormWindow(s, 0, 0, s->Width(), s->Height()),
    manager(mgr),
    beauty(nullptr),
    lst_designs(nullptr),
    txt_caption(nullptr),
    txt_stats(nullptr),
    txt_description(nullptr),
    btn_ships(nullptr),
    btn_weaps(nullptr),
    btn_close(nullptr),
    imgview(nullptr),
    camview(nullptr),
    mode(MODE_NONE),
    radius(100),
    cam_zoom(2.5),
    cam_az(-PI / 6),
    cam_el(PI / 7),
    mouse_x(0),
    mouse_y(0),
    captured(false),
    ship_index(0),
    weap_index(0) { Init(def); }

TacRefDlg::~TacRefDlg()
{
  if (beauty)
  {
    beauty->DelView(camview);
    beauty->DelView(imgview);
  }

  delete camview;
  delete imgview;
}

void TacRefDlg::RegisterControls()
{
  btn_close = dynamic_cast<Button*>(FindControl(1));
  btn_ships = dynamic_cast<Button*>(FindControl(101));
  btn_weaps = dynamic_cast<Button*>(FindControl(102));
  lst_designs = dynamic_cast<ListBox*>(FindControl(200));
  txt_caption = FindControl(301);
  beauty = FindControl(401);
  txt_stats = dynamic_cast<RichTextBox*>(FindControl(402));
  txt_description = dynamic_cast<RichTextBox*>(FindControl(403));

  if (btn_close) { REGISTER_CLIENT(EID_CLICK, btn_close, TacRefDlg, OnClose); }

  if (btn_ships)
  {
    btn_ships->SetButtonState(mode == MODE_SHIPS);
    REGISTER_CLIENT(EID_CLICK, btn_ships, TacRefDlg, OnMode);
  }

  if (btn_weaps)
  {
    btn_weaps->SetButtonState(mode == MODE_WEAPONS);
    REGISTER_CLIENT(EID_CLICK, btn_weaps, TacRefDlg, OnMode);
  }

  if (lst_designs) { REGISTER_CLIENT(EID_SELECT, lst_designs, TacRefDlg, OnSelect); }

  if (beauty)
  {
    REGISTER_CLIENT(EID_RBUTTON_DOWN, beauty, TacRefDlg, OnCamRButtonDown);
    REGISTER_CLIENT(EID_RBUTTON_UP, beauty, TacRefDlg, OnCamRButtonUp);
    REGISTER_CLIENT(EID_MOUSE_MOVE, beauty, TacRefDlg, OnCamMove);
    REGISTER_CLIENT(EID_MOUSE_WHEEL, beauty, TacRefDlg, OnCamZoom);

    scene.SetAmbient(Color(60, 60, 60));

    Point light_pos(3e6, 5e6, 4e6);

    auto main_light = new Light(1.0f); //1.25f);
    main_light->MoveTo(light_pos);
    main_light->SetType(Light::LIGHT_DIRECTIONAL);
    main_light->SetColor(Color::White);
    main_light->SetShadow(true);

    scene.AddLight(main_light);

    auto back_light = new Light(0.5f);
    back_light->MoveTo(light_pos * -1.0f);
    back_light->SetType(Light::LIGHT_DIRECTIONAL);
    back_light->SetColor(Color::White);
    back_light->SetShadow(false);

    scene.AddLight(back_light);

    camview = NEW CameraView(beauty, &cam, &scene);
    camview->SetProjectionType(Video::PROJECTION_PERSPECTIVE);
    camview->SetFieldOfView(2);

    beauty->AddView(camview);

    imgview = NEW ImgView(beauty, nullptr);
    imgview->SetBlend(Video::BLEND_ALPHA);
  }
}

void TacRefDlg::Show()
{
  update_scene = !shown;

  FormWindow::Show();

  if (update_scene)
  {
    AWEvent event(btn_ships, EID_CLICK);
    OnMode(&event);
  }
}

struct WepGroup
{
  std::string name;
  int count;

  WepGroup()
    : count(0) {}
};

WepGroup* FindWepGroup(WepGroup* weapons, std::string_view name);

void TacRefDlg::SelectShip(const ShipDesign* design)
{
  if (beauty && camview)
  {
    scene.Graphics().clear();

    if (design)
    {
      radius = design->radius;

      UpdateCamera();

      int level = design->lod_levels - 1;
      int n = design->models[level].size();

      for (int i = 0; i < n; i++)
      {
        Model* model = design->models[level].at(i);

        auto s = NEW Solid;
        s->UseModel(model);
        s->CreateShadows(1);
        s->MoveTo(*design->offsets[level].at(i));

        scene.Graphics().append(s);
      }
    }
  }

  if (txt_caption)
  {
    txt_caption->SetText("");

    if (design)
    {
      char txt[256];
      sprintf_s(txt, "%s %s", design->m_abrv.c_str(), design->DisplayName().c_str());
      txt_caption->SetText(txt);
    }
  }

  if (txt_stats)
  {
    txt_stats->SetText("");

    if (design)
    {
      std::string desc;
      char txt[256];

      sprintf_s(txt, "%s\t\t\t%s\n", Game::GetText("tacref.type").data(), Ship::ClassName(design->type).data());
      desc += txt;

      sprintf_s(txt, "%s\t\t\t%s\n", Game::GetText("tacref.class").data(), design->DisplayName().c_str());
      desc += txt;
      desc += Game::GetText("tacref.length");
      desc += "\t\t";

      if (design->type < Ship::STATION)
        FormatNumber(txt, design->radius / 2);
      else
        FormatNumber(txt, design->radius * 2);

      strcat_s(txt, " m\n");
      desc += txt;
      desc += Game::GetText("tacref.mass");
      desc += "\t\t\t";

      FormatNumber(txt, design->mass);
      strcat_s(txt, " T\n");
      desc += txt;
      desc += Game::GetText("tacref.hull");
      desc += "\t\t\t";

      FormatNumber(txt, design->integrity);
      strcat_s(txt, "\n");
      desc += txt;

      if (design->weapons.size())
      {
        desc += Game::GetText("tacref.weapons");

        WepGroup groups[8];
        for (int w = 0; w < design->weapons.size(); w++)
        {
          Weapon* gun = design->weapons[w];
          WepGroup* group = FindWepGroup(groups, gun->Group());

          if (group)
            group->count++;
        }

        for (int g = 0; g < 8; g++)
        {
          WepGroup* group = &groups[g];
          if (group && group->count)
          {
            sprintf_s(txt, "\t\t%s (%d)\n\t\t", group->name.c_str(), group->count);
            desc += txt;

            for (int w = 0; w < design->weapons.size(); w++)
            {
              Weapon* gun = design->weapons[w];

              if (group->name == gun->Group())
              {
                sprintf_s(txt, "\t\t\t%s\n\t\t", gun->Design()->name.c_str());
                desc += txt;
              }
            }
          }
        }

        desc += "\n";
      }

      txt_stats->SetText(desc);
    }
  }

  if (txt_description)
  {
    if (design && !design->description.empty())
      txt_description->SetText(design->description);
    else
      txt_description->SetText(Game::GetText("tacref.mass"));
  }
}

void TacRefDlg::SelectWeapon(const WeaponDesign* design)
{
  if (beauty && imgview)
  {
    imgview->SetPicture(nullptr);

    if (design)
      imgview->SetPicture(design->beauty_img);
  }

  if (txt_caption)
  {
    txt_caption->SetText("");

    if (design)
      txt_caption->SetText(design->name);
  }

  if (txt_stats)
  {
    txt_stats->SetText("");

    if (design)
    {
      std::string desc;
      char txt[256];

      desc = Game::GetText("tacref.name");
      desc += "\t";
      desc += design->name;
      desc += "\n";
      desc += Game::GetText("tacref.type");
      desc += "\t\t";

      if (design->damage < 1)
        desc += Game::GetText("tacref.wep.other");
      else if (design->beam)
        desc += Game::GetText("tacref.wep.beam");
      else if (design->primary)
        desc += Game::GetText("tacref.wep.bolt");
      else if (design->drone)
        desc += Game::GetText("tacref.wep.drone");
      else if (design->guided)
        desc += Game::GetText("tacref.wep.guided");
      else
        desc += Game::GetText("tacref.wep.missile");

      if (design->turret_model && design->damage >= 1)
      {
        desc += " ";
        desc += Game::GetText("tacref.wep.turret");
        desc += "\n";
      }
      else
        desc += "\n";

      desc += Game::GetText("tacref.targets");
      desc += "\t";

      if ((design->target_type & Ship::DROPSHIPS) != 0)
      {
        if ((design->target_type & Ship::STARSHIPS) != 0)
        {
          if ((design->target_type & Ship::GROUND_UNITS) != 0)
            desc += Game::GetText("tacref.targets.fsg");
          else
            desc += Game::GetText("tacref.targets.fs");
        }
        else
        {
          if ((design->target_type & Ship::GROUND_UNITS) != 0)
            desc += Game::GetText("tacref.targets.fg");
          else
            desc += Game::GetText("tacref.targets.f");
        }
      }

      else if ((design->target_type & Ship::STARSHIPS) != 0)
      {
        if ((design->target_type & Ship::GROUND_UNITS) != 0)
          desc += Game::GetText("tacref.targets.sg");
        else
          desc += Game::GetText("tacref.targets.s");
      }

      else if ((design->target_type & Ship::GROUND_UNITS) != 0)
        desc += Game::GetText("tacref.targets.g");

      desc += "\n";
      desc += Game::GetText("tacref.speed");
      desc += "\t";

      FormatNumber(txt, design->speed);
      desc += txt;
      desc += "m/s\n";
      desc += Game::GetText("tacref.range");
      desc += "\t";

      FormatNumber(txt, design->max_range);
      desc += txt;
      desc += "m\n";
      desc += Game::GetText("tacref.damage");
      desc += "\t";

      if (design->damage > 0)
      {
        FormatNumber(txt, design->damage * design->charge);
        desc += txt;
        if (design->beam)
          desc += "/s";
      }
      else
        desc += Game::GetText("tacref.none");

      desc += "\n";

      if (!design->primary && design->damage > 0)
      {
        desc += Game::GetText("tacref.kill-radius");
        desc += "\t";
        FormatNumber(txt, design->lethal_radius);
        desc += txt;
        desc += " m\n";
      }

      txt_stats->SetText(desc);
    }
  }

  if (txt_description)
  {
    if (design && design->description.length())
      txt_description->SetText(design->description);
    else
      txt_description->SetText(Game::GetText("tacref.no-info"));
  }
}

void TacRefDlg::ExecFrame() {}

bool TacRefDlg::SetCaptureBeauty()
{
  EventDispatch* dispatch = EventDispatch::GetInstance();
  if (dispatch && beauty)
    return dispatch->CaptureMouse(beauty) ? true : false;

  return false;
}

bool TacRefDlg::ReleaseCaptureBeauty()
{
  EventDispatch* dispatch = EventDispatch::GetInstance();
  if (dispatch && beauty)
    return dispatch->ReleaseMouse(beauty) ? true : false;

  return false;
}

void TacRefDlg::UpdateAzimuth(double a)
{
  cam_az += a;

  if (cam_az > PI)
    cam_az = -2 * PI + cam_az;

  else if (cam_az < -PI)
    cam_az = 2 * PI + cam_az;
}

void TacRefDlg::UpdateElevation(double e)
{
  cam_el += e;

  constexpr double limit = (0.43 * PI);

  if (cam_el > limit)
    cam_el = limit;
  else if (cam_el < -limit)
    cam_el = -limit;
}

void TacRefDlg::UpdateZoom(double delta)
{
  cam_zoom *= delta;

  if (cam_zoom < 1.2)
    cam_zoom = 1.2;

  else if (cam_zoom > 10)
    cam_zoom = 10;
}

void TacRefDlg::UpdateCamera()
{
  double x = cam_zoom * radius * sin(cam_az) * cos(cam_el);
  double y = cam_zoom * radius * cos(cam_az) * cos(cam_el);
  double z = cam_zoom * radius * sin(cam_el);

  cam.LookAt(Point(0, 0, 0), Point(x, z, y), Point(0, 1, 0));
}

void TacRefDlg::OnSelect(AWEvent* event)
{
  if (lst_designs)
  {
    int seln = lst_designs->GetSelection();
    uintptr_t dsn = lst_designs->GetItemData(seln);

    if (mode == MODE_SHIPS)
    {
      ship_index = seln;

      if (dsn)
        SelectShip((ShipDesign*)dsn);
    }

    else if (mode == MODE_WEAPONS)
    {
      weap_index = seln;

      if (dsn)
        SelectWeapon((WeaponDesign*)dsn);
    }
  }
}

void TacRefDlg::OnCamRButtonDown(AWEvent* event)
{
  captured = SetCaptureBeauty();
  mouse_x = event->x;
  mouse_y = event->y;
}

void TacRefDlg::OnCamRButtonUp(AWEvent* event)
{
  if (captured)
    ReleaseCaptureBeauty();

  captured = false;
  mouse_x = 0;
  mouse_y = 0;
}

void TacRefDlg::OnCamMove(AWEvent* event)
{
  if (captured)
  {
    int mouse_dx = event->x - mouse_x;
    int mouse_dy = event->y - mouse_y;

    UpdateAzimuth(mouse_dx * 0.3 * DEGREES);
    UpdateElevation(mouse_dy * 0.3 * DEGREES);
    UpdateCamera();

    mouse_x = event->x;
    mouse_y = event->y;
  }
}

void TacRefDlg::OnCamZoom(AWEvent* event)
{
  int w = Mouse::Wheel();

  if (w < 0)
  {
    while (w < 0)
    {
      UpdateZoom(1.25);
      w += 120;
    }
  }
  else
  {
    while (w > 0)
    {
      UpdateZoom(0.75f);
      w -= 120;
    }
  }

  UpdateCamera();
}

void TacRefDlg::OnMode(AWEvent* event)
{
  if (event->window == btn_ships && mode != MODE_SHIPS)
  {
    mode = MODE_SHIPS;

    if (lst_designs)
    {
      lst_designs->ClearItems();

      std::vector<std::string> designs;

      for (int n = 0; n < 16; n++)
      {
        int type = 1 << n;
        ShipDesign::GetDesignList(type, designs);

        for (auto& val : designs)
        {
          if (const ShipDesign* dsn = ShipDesign::Get(val))
          {
            char txt[256];
            sprintf_s(txt, "%s %s", dsn->m_abrv.c_str(), dsn->DisplayName().c_str());

            lst_designs->AddItemWithData(txt, reinterpret_cast<uint64_t>(dsn));
          }
          else
            lst_designs->AddItemWithData(val, 0);
        }
      }

      lst_designs->SetSelected(ship_index);
    }

    if (beauty)
    {
      beauty->AddView(camview);
      beauty->DelView(imgview);
    }

    if (auto dsn = lst_designs->GetItemData(ship_index))
      SelectShip(reinterpret_cast<ShipDesign*>(dsn));
  }

  else if (event->window == btn_weaps && mode != MODE_WEAPONS)
  {
    mode = MODE_WEAPONS;

    if (lst_designs)
    {
      const WeaponDesign* design = nullptr;

      lst_designs->ClearItems();
      std::vector<std::string> designs;

      WeaponDesign::GetDesignList(designs);

      for (auto& val : designs)
      {
        if (val.contains("Zolon") || val.contains("Inverted"))
          continue;

        const WeaponDesign* dsn = WeaponDesign::Find(val);

        if (dsn && !dsn->secret)
        {
          lst_designs->AddItemWithData(val, reinterpret_cast<intptr_t>(dsn));

          if (!design)
            design = dsn;
        }
      }

      lst_designs->SetSelected(weap_index);
    }

    if (beauty)
    {
      beauty->DelView(camview);
      beauty->AddView(imgview);
    }

    if (uintptr_t dsn = lst_designs->GetItemData(weap_index))
      SelectWeapon(reinterpret_cast<WeaponDesign*>(dsn));
  }

  btn_ships->SetButtonState(mode == MODE_SHIPS);
  btn_weaps->SetButtonState(mode == MODE_WEAPONS);
}

void TacRefDlg::OnClose(AWEvent* event) { manager->ShowMenuDlg(); }

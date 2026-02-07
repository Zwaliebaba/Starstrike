#include "pch.h"
#include "Galaxy.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Reader.h"
#include "StarSystem.h"

static Galaxy* g_galaxy = nullptr;

Galaxy::Galaxy(std::string_view n)
  : m_filename{},
    name(n),
    radius(10) {}

Galaxy::~Galaxy()
{
  DebugTrace("  Destroying Galaxy {}\n", name);
  systems.destroy();
  stars.destroy();
}

void Galaxy::Initialize()
{
  delete g_galaxy;
  g_galaxy = NEW Galaxy("Galaxy");
  g_galaxy->Load();
}

void Galaxy::Close()
{
  delete g_galaxy;
  g_galaxy = nullptr;
}

Galaxy* Galaxy::GetInstance() { return g_galaxy; }

void Galaxy::Load()
{
  DataLoader* loader = DataLoader::GetLoader();
  loader->SetDataPath("Galaxy\\");
  m_filename = name.data() + std::string(".def");
  Load(m_filename);
}

void Galaxy::Load(std::string_view filename)
{
  DebugTrace("\nLoading Galaxy: {}\n", filename);

  BYTE* block = nullptr;
  DataLoader* loader = DataLoader::GetLoader();
  loader->LoadBuffer(filename, block, true);

  Parser parser(NEW BlockReader((const char*)block));

  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("WARNING: could not parse '{}'\n", filename);
    return;
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "GALAXY")
  {
    DebugTrace("WARNING: invalid galaxy file '{}'\n", filename);
    return;
  }

  // parse the galaxy:
  do
  {
    delete term;
    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
      {
        if (def->name()->value() == "radius")
          GetDefNumber(radius, def, filename);

        else if (def->name()->value() == "system")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: system struct missing in '{}'\n", filename);
          else
          {
            TermStruct* val = def->term()->isStruct();

            std::string sys_name;
            std::string classname;
            Vec3 sys_loc;
            int sys_iff = 0;
            int star_class = Star::G;

            for (int i = 0; i < val->elements()->size(); i++)
            {
              TermDef* pdef = val->elements()->at(i)->isDef();
              if (pdef)
              {
                if (pdef->name()->value() == "name")
                  GetDefText(sys_name, pdef, filename);

                else if (pdef->name()->value() == "loc")
                  GetDefVec(sys_loc, pdef, filename);

                else if (pdef->name()->value() == "iff")
                  GetDefNumber(sys_iff, pdef, filename);

                else if (pdef->name()->value() == "class")
                {
                  GetDefText(classname, pdef, filename);

                  switch (classname[0])
                  {
                    case 'O':
                      star_class = Star::O;
                      break;
                    case 'B':
                      star_class = Star::B;
                      break;
                    case 'A':
                      star_class = Star::A;
                      break;
                    case 'F':
                      star_class = Star::F;
                      break;
                    case 'G':
                      star_class = Star::G;
                      break;
                    case 'K':
                      star_class = Star::K;
                      break;
                    case 'M':
                      star_class = Star::M;
                      break;
                    case 'R':
                      star_class = Star::RED_GIANT;
                      break;
                    case 'W':
                      star_class = Star::WHITE_DWARF;
                      break;
                    case 'Z':
                      star_class = Star::BLACK_HOLE;
                      break;
                  }
                }
              }
            }

            if (sys_name[0])
            {
              auto star_system = NEW StarSystem(sys_name, sys_loc, sys_iff, star_class);
              star_system->Load();
              systems.append(star_system);

              auto star = NEW Star(sys_name, sys_loc, star_class);
              stars.append(star);
            }
          }
        }

        else if (def->name()->value() == "star")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: star struct missing in '{}'\n", filename);
          else
          {
            TermStruct* val = def->term()->isStruct();

            std::string star_name;
            std::string classname;
            Vec3 star_loc;
            int star_class = Star::G;

            for (int i = 0; i < val->elements()->size(); i++)
            {
              TermDef* pdef = val->elements()->at(i)->isDef();
              if (pdef)
              {
                if (pdef->name()->value() == "name")
                  GetDefText(star_name, pdef, filename);

                else if (pdef->name()->value() == "loc")
                  GetDefVec(star_loc, pdef, filename);

                else if (pdef->name()->value() == "class")
                {
                  GetDefText(classname, pdef, filename);

                  switch (classname[0])
                  {
                    case 'O':
                      star_class = Star::O;
                      break;
                    case 'B':
                      star_class = Star::B;
                      break;
                    case 'A':
                      star_class = Star::A;
                      break;
                    case 'F':
                      star_class = Star::F;
                      break;
                    case 'G':
                      star_class = Star::G;
                      break;
                    case 'K':
                      star_class = Star::K;
                      break;
                    case 'M':
                      star_class = Star::M;
                      break;
                    case 'R':
                      star_class = Star::RED_GIANT;
                      break;
                    case 'W':
                      star_class = Star::WHITE_DWARF;
                      break;
                    case 'Z':
                      star_class = Star::BLACK_HOLE;
                      break;
                  }
                }
              }
            }

            if (star_name[0])
            {
              auto star = NEW Star(star_name, star_loc, star_class);
              stars.append(star);
            }
          }
        }
      }
    }
  }
  while (term);

  loader->ReleaseBuffer(block);
  loader->SetDataPath();
}

void Galaxy::ExecFrame()
{
  ListIter<StarSystem> sys = systems;
  while (++sys)
    sys->ExecFrame();
}

StarSystem* Galaxy::GetSystem(std::string_view _name)
{
  ListIter<StarSystem> sys = systems;
  while (++sys)
  {
    if (sys->Name() == _name)
      return sys.value();
  }

  return nullptr;
}

StarSystem* Galaxy::FindSystemByRegion(std::string_view _rgnName)
{
  ListIter<StarSystem> iter = systems;
  while (++iter)
  {
    StarSystem* sys = iter.value();
    if (sys->FindRegion(_rgnName))
      return sys;
  }

  return nullptr;
}

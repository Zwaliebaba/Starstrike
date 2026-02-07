#include "pch.h"
#include "SystemDesign.h"
#include "Component.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Reader.h"

List<SystemDesign> SystemDesign::catalog;

#define GET_DEF_TEXT(p,d,x) if(p->name()->value()==(#x))GetDefText(d->x,p,filename)
#define GET_DEF_NUM(p,d,x)  if(p->name()->value()==(#x))GetDefNumber(d->x,p,filename)

SystemDesign::SystemDesign() {}

SystemDesign::~SystemDesign() { components.destroy(); }

void SystemDesign::Initialize(const char* filename)
{
  DebugTrace("Loading System Designs '{}'\n", filename);

  // Load Design File:
  DataLoader* loader = DataLoader::GetLoader();
  BYTE* block;

  int blocklen = loader->LoadBuffer(filename, block, true);
  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse '{}'\n", filename);
    exit(-3);
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "SYSTEM")
  {
    DebugTrace("ERROR: invalid system design file '{}'\n", filename);
    exit(-4);
  }

  int type = 1;

  do
  {
    delete term;

    term = parser.ParseTerm();

    if (term)
    {
      TermDef* def = term->isDef();
      if (def)
      {
        if (def->name()->value() == "system")
        {
          if (!def->term() || !def->term()->isStruct())
            DebugTrace("WARNING: system structure missing in '{}'\n", filename);
          else
          {
            TermStruct* val = def->term()->isStruct();
            auto design = NEW SystemDesign;

            for (int i = 0; i < val->elements()->size(); i++)
            {
              TermDef* pdef = val->elements()->at(i)->isDef();
              if (pdef)
              {
                GET_DEF_TEXT(pdef, design, name);

                else if (pdef->name()->value() == ("component"))
                {
                  if (!pdef->term() || !pdef->term()->isStruct())
                    DebugTrace("WARNING: component structure missing in system '{}' in '{}'\n",
                               design->name, filename);
                  else
                  {
                    TermStruct* val2 = pdef->term()->isStruct();
                    auto comp_design = NEW ComponentDesign;

                    for (int i = 0; i < val2->elements()->size(); i++)
                    {
                      TermDef* pdef2 = val2->elements()->at(i)->isDef();
                      if (pdef2)
                      {
                        GET_DEF_TEXT(pdef2, comp_design, name);
                        else
                          GET_DEF_TEXT(pdef2, comp_design, abrv);
                          else
                            GET_DEF_NUM(pdef2, comp_design, repair_time);
                            else
                              GET_DEF_NUM(pdef2, comp_design, replace_time);
                              else
                                GET_DEF_NUM(pdef2, comp_design, spares);
                                else
                                  GET_DEF_NUM(pdef2, comp_design, affects);

                                  else
                                  {
                                    DebugTrace("WARNING: parameter '{}' ignored in '{}'\n",
                                               pdef2->name()->value().data(), filename);
                                  }
                      }
                    }

                    design->components.append(comp_design);
                  }
                }

                else
                {
                  DebugTrace("WARNING: parameter '{}' ignored in '{}'\n", pdef->name()->value().data(), filename);
                }
              }
              else
              {
                DebugTrace("WARNING: term ignored in '{}'\n", filename);
                val->elements()->at(i)->print();
              }
            }

            catalog.append(design);
          }
        }

        else { DebugTrace("WARNING: unknown definition '{}' in '{}'\n", def->name()->value().data(), filename); }
      }
      else
      {
        DebugTrace("WARNING: term ignored in '{}'\n", filename);
        term->print();
      }
    }
  }
  while (term);

  loader->ReleaseBuffer(block);
}

void SystemDesign::Close() { catalog.destroy(); }

SystemDesign* SystemDesign::Find(std::string_view name)
{
  SystemDesign test;
  test.name = name;
  return catalog.find(&test);
}

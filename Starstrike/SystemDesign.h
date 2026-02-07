#ifndef SystemDesign_h
#define SystemDesign_h

#include "List.h"

class ComponentDesign;

class SystemDesign
{
  public:
    
    SystemDesign();
    ~SystemDesign();
    int operator ==(const SystemDesign& rhs) const { return name == rhs.name; }

    static void Initialize(const char* filename);
    static void Close();
    static SystemDesign* Find(std::string_view name);

    // Unique ID:
    std::string name;

    // Sub-components:
    List<ComponentDesign> components;

    static List<SystemDesign> catalog;
};

#endif SystemDesign_h

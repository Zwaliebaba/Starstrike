#ifndef CombatEvent_h
#define CombatEvent_h

#include "Bitmap.h"
#include "Geometry.h"

class Campaign;
class CombatGroup;
class CombatUnit;

class CombatEvent
{
  public:
    enum EVENT_TYPE
    {
      ATTACK,
      DEFEND,
      MOVE_TO,
      CAPTURE,
      STRATEGY,
      CAMPAIGN_START,
      STORY,
      CAMPAIGN_END,
      CAMPAIGN_FAIL
    };

    enum EVENT_SOURCE
    {
      FORCOM,
      TACNET,
      INTEL,
      MAIL,
      NEWS
    };

    CombatEvent(Campaign* c, int type, int time, int team, int source, std::string rgn);

    int operator ==(const CombatEvent& u) const { return this == &u; }

    // accessors/mutators:
    int Type() const { return type; }
    int Time() const { return time; }
    int GetIFF() const { return team; }
    int Points() const { return points; }
    int Source() const { return source; }
    Point Location() const { return loc; }
    std::string Region() const { return region; }
    std::string Title() const { return title; }
    std::string Information() const { return info; }
    std::string Filename() const { return file; }
    std::string ImageFile() const { return image_file; }
    std::string SceneFile() const { return scene_file; }
    Bitmap& Image() { return image; }
    std::string SourceName() const;
    std::string TypeName() const;
    bool Visited() const { return visited; }

    void SetType(int t) { type = t; }
    void SetTime(int t) { time = t; }
    void SetIFF(int t) { team = t; }
    void SetPoints(int p) { points = p; }
    void SetSource(int s) { source = s; }
    void SetLocation(const Point& p) { loc = p; }
    void SetRegion(std::string rgn) { region = rgn; }
    void SetTitle(std::string t) { title = t; }
    void SetInformation(std::string t) { info = t; }
    void SetFilename(std::string f) { file = f; }
    void SetImageFile(std::string f) { image_file = f; }
    void SetSceneFile(std::string f) { scene_file = f; }
    void SetVisited(bool v) { visited = v; }

    // operations:
    void Load();

    // utilities:
    static int TypeFromName(std::string n);
    static int SourceFromName(std::string n);
    static std::string TypeName(int n);
    static std::string SourceName(int n);

  private:
    Campaign* campaign;
    int type;
    int time;
    int team;
    int points;
    int source;
    bool visited;
    Point loc;
    std::string region;
    std::string title;
    std::string info;
    std::string file;
    std::string image_file;
    std::string scene_file;
    Bitmap image;
};

#endif CombatEvent_h

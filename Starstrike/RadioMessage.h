#ifndef RadioMessage_h
#define RadioMessage_h

#include "Geometry.h"
#include "Instruction.h"
#include "List.h"
#include "SimObject.h"

class Element;
class Ship;
class SimObject;

class RadioMessage
{
  public:
    enum ACTION
    {
      NONE       = 0,
      DOCK_WITH  = Instruction::DOCK,
      RTB        = Instruction::RTB,
      QUANTUM_TO = Instruction::NUM_ACTIONS,
      FARCAST_TO,

      // protocol:
      ACK,
      NACK,

      // target mgt:
      ATTACK,
      ESCORT,
      BRACKET,
      IDENTIFY,

      // combat mgt:
      COVER_ME,
      WEP_FREE,
      WEP_HOLD,
      FORM_UP,       // alias for wep_hold
      SAY_POSITION,

      // sensor mgt:
      LAUNCH_PROBE,
      GO_EMCON1,
      GO_EMCON2,
      GO_EMCON3,

      // formation mgt:
      GO_DIAMOND,
      GO_SPREAD,
      GO_BOX,
      GO_TRAIL,

      // mission mgt:
      MOVE_PATROL,
      SKIP_NAVPOINT,
      RESUME_MISSION,

      // misc announcements:
      CALL_ENGAGING,
      FOX_1,
      FOX_2,
      FOX_3,
      SPLASH_1,
      SPLASH_2,
      SPLASH_3,
      SPLASH_4,
      SPLASH_5,   // target destroyed
      SPLASH_6,   // enemy destroyed
      SPLASH_7,   // confirmed kill
      DISTRESS,
      BREAK_ORBIT,
      MAKE_ORBIT,
      QUANTUM_JUMP,

      // friendly fire:
      WARN_ACCIDENT,
      WARN_TARGETED,
      DECLARE_ROGUE,

      // support:
      PICTURE,
      REQUEST_PICTURE,
      REQUEST_SUPPORT,

      // traffic control:
      CALL_INBOUND,
      CALL_APPROACH,
      CALL_CLEARANCE,
      CALL_FINALS,
      CALL_WAVE_OFF,
      NUM_ACTIONS
    };

    RadioMessage(Ship* dst, const Ship* sender, int action);
    RadioMessage(Element* dst, const Ship* sender, int action);
    RadioMessage(const RadioMessage& rm);
    virtual ~RadioMessage();

    // accessors:
    static const char* ActionName(int a);

    const Ship* Sender() const { return sender; }
    Ship* DestinationShip() const { return dst_ship; }
    Element* DestinationElem() const { return dst_elem; }
    int Action() const { return action; }
    List<SimObject>& TargetList() { return target_list; }
    const Point& Location() const { return location; }
    const std::string& Info() const { return info; }
    int Channel() const { return channel; }

    // mutators:
    void SetDestinationShip(Ship* s) { dst_ship = s; }
    void SetDestinationElem(Element* e) { dst_elem = e; }
    void AddTarget(SimObject* s);
    void SetLocation(const Point& l) { location = l; }
    void SetInfo(const std::string& msg) { info = msg; }
    void SetChannel(int c) { channel = c; }

  protected:
    const Ship* sender;
    Ship* dst_ship;
    Element* dst_elem;
    int action;
    List<SimObject> target_list;
    Point location;
    std::string info;
    int channel;
};

#endif RadioMessage_h

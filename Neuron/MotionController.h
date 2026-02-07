#ifndef MoCon_h
#define MoCon_h

struct KeyMapEntry
{
  
  KeyMapEntry()
    : act(0),
      key(0),
      alt(0) {}

  KeyMapEntry(int a, int k, int s = 0)
    : act(a),
      key(k),
      alt(s) {}

  int operator==(const KeyMapEntry& k) const { return act == k.act && key == k.key && alt == k.alt; }
  int operator!=(const KeyMapEntry& k) const { return !(*this == k); }

  int act;
  int key;
  int alt;
};

constexpr int KEY_MAP_SIZE = 256;
constexpr int KEY_BASE_SIZE = 64;
constexpr int KEY_USER_SIZE = KEY_MAP_SIZE - KEY_BASE_SIZE;

constexpr int KEY_MAP_BASE = 0;
constexpr int KEY_MAP_END = KEY_MAP_BASE + KEY_BASE_SIZE - 1;

constexpr int KEY_USER_BASE = KEY_MAP_END + 1;
constexpr int KEY_USER_END = KEY_USER_BASE + KEY_USER_SIZE - 1;

constexpr int KEY_MAP_FIRST = KEY_MAP_BASE;
constexpr int KEY_MAP_LAST = KEY_MAP_BASE + KEY_MAP_SIZE - 1;

// MAP NAMES:

constexpr int KEY_PLUS_X = 1;
constexpr int KEY_MINUS_X = 2;
constexpr int KEY_PLUS_Y = 3;
constexpr int KEY_MINUS_Y = 4;
constexpr int KEY_PLUS_Z = 5;
constexpr int KEY_MINUS_Z = 6;

constexpr int KEY_PITCH_UP = 7;
constexpr int KEY_PITCH_DOWN = 8;
constexpr int KEY_YAW_LEFT = 9;
constexpr int KEY_YAW_RIGHT = 10;
constexpr int KEY_ROLL_LEFT = 11;
constexpr int KEY_ROLL_RIGHT = 12;
constexpr int KEY_CENTER = 13;
constexpr int KEY_ROLL_ENABLE = 14;

constexpr int KEY_ACTION_0 = 15;
constexpr int KEY_ACTION_1 = 16;
constexpr int KEY_ACTION_2 = 17;
constexpr int KEY_ACTION_3 = 18;

constexpr int KEY_CONTROL_MODEL = 19;
constexpr int KEY_MOUSE_SELECT = 20;
constexpr int KEY_MOUSE_SENSE = 21;
constexpr int KEY_MOUSE_SWAP = 22;
constexpr int KEY_MOUSE_INVERT = 23;
constexpr int KEY_MOUSE_ACTIVE = 24;

constexpr int KEY_AXIS_YAW = 32;
constexpr int KEY_AXIS_PITCH = 33;
constexpr int KEY_AXIS_ROLL = 34;
constexpr int KEY_AXIS_THROTTLE = 35;

constexpr int KEY_AXIS_YAW_INVERT = 38;
constexpr int KEY_AXIS_PITCH_INVERT = 39;
constexpr int KEY_AXIS_ROLL_INVERT = 40;
constexpr int KEY_AXIS_THROTTLE_INVERT = 41;

// CONTROL VALUES:

// joystick buttons and switches must use
// ids greater than 255 so they don't interfere
// with extended ascii numbers for keyboard keys

constexpr int KEY_POV_0_UP = 0x1F0;
constexpr int KEY_POV_0_DOWN = 0x1F1;
constexpr int KEY_POV_0_LEFT = 0x1F2;
constexpr int KEY_POV_0_RIGHT = 0x1F3;

constexpr int KEY_POV_1_UP = 0x1F4;
constexpr int KEY_POV_1_DOWN = 0x1F5;
constexpr int KEY_POV_1_LEFT = 0x1F6;
constexpr int KEY_POV_1_RIGHT = 0x1F7;

constexpr int KEY_POV_2_UP = 0x1F8;
constexpr int KEY_POV_2_DOWN = 0x1F9;
constexpr int KEY_POV_2_LEFT = 0x1FA;
constexpr int KEY_POV_2_RIGHT = 0x1FB;

constexpr int KEY_POV_3_UP = 0x1FC;
constexpr int KEY_POV_3_DOWN = 0x1FD;
constexpr int KEY_POV_3_LEFT = 0x1FE;
constexpr int KEY_POV_3_RIGHT = 0x1FF;

class MotionController
{
  public:
    
    MotionController()
      : status(StatusOK),
        sensitivity(1),
        dead_zone(0),
        swapped(0),
        inverted(0),
        rudder(0),
        m_throttle(0),
        select(0) {}

    virtual ~MotionController() {}

    enum StatusValue
    {
      StatusOK,
      StatusErr,
      StatusBadParm
    };

    enum ActionValue { MaxActions = 32 };

    StatusValue Status() const { return status; }
    int Sensitivity() const { return sensitivity; }
    int DeadZone() const { return dead_zone; }
    int Swapped() const { return swapped; }
    int Inverted() const { return inverted; }
    int RudderEnabled() const { return rudder; }
    int ThrottleEnabled() const { return m_throttle; }
    int Selector() const { return select; }

    // setup:
    virtual void SetSensitivity(int sense, int dead)
    {
      if (sense > 0)
        sensitivity = sense;
      if (dead > 0)
        dead_zone = dead;
    }

    virtual void SetSelector(int sel) { select = sel; }
    virtual void SetRudderEnabled(int rud) { rudder = rud; }
    virtual void SetThrottleEnabled(int t) { m_throttle = t; }

    virtual void SwapYawRoll(int swap) { swapped = swap; }
    virtual int GetSwapYawRoll() { return swapped; }
    virtual void InvertPitch(int inv) { inverted = inv; }
    virtual int GetInverted() { return inverted; }

    virtual void MapKeys(KeyMapEntry* mapping, int nkeys) {}

    // sample the physical device
    virtual void Acquire() {}

    // translations
    virtual double X() { return 0; }
    virtual double Y() { return 0; }
    virtual double Z() { return 0; }

    // rotations
    virtual double Pitch() { return 0; }
    virtual double Roll() { return 0; }
    virtual double Yaw() { return 0; }
    virtual int Center() { return 0; }

    // throttle
    virtual double Throttle() { return 0; }
    virtual void SetThrottle(double t) {}

    // actions
    virtual int Action(int n) { return 0; }
    virtual int ActionMap(int n) { return 0; }

  protected:
    StatusValue status;
    int sensitivity;
    int dead_zone;
    int swapped;
    int inverted;
    int rudder;
    int m_throttle;
    int select;
};

#endif MoCon_h

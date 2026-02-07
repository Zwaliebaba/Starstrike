#ifndef RadioTraffic_h
#define RadioTraffic_h

#include "List.h"


#undef SendMessage

class Element;
class RadioMessage;
class Ship;
class SimObject;

class RadioTraffic
{
  public:
    RadioTraffic();
    ~RadioTraffic();

    // accessors:
    static void Initialize();
    static void Close();

    static RadioTraffic* GetInstance() { return radio_traffic; }

    static void SendQuickMessage(const Ship* ship, int msg);
    static void Transmit(RadioMessage* msg);
    static void DiscardMessages();
    static std::string TranslateVox(const char* phrase);

    void SendMessage(RadioMessage* msg);
    void DisplayMessage(RadioMessage* msg);

  protected:
    List<RadioMessage> traffic;

    static RadioTraffic* radio_traffic;
};

#endif RadioTraffic_h

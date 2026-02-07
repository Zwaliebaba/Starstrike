#include "pch.h"
#include "RadioTraffic.h"
#include "Contact.h"
#include "Element.h"
#include "Game.h"
#include "NetData.h"
#include "NetGame.h"
#include "RadioMessage.h"
#include "RadioView.h"
#include "RadioVox.h"
#include "Ship.h"
#include "Sim.h"


RadioTraffic* RadioTraffic::radio_traffic = nullptr;

RadioTraffic::RadioTraffic() { radio_traffic = this; }

RadioTraffic::~RadioTraffic() { traffic.destroy(); }

void RadioTraffic::Initialize()
{
  if (!radio_traffic)
    radio_traffic = NEW RadioTraffic;
}

void RadioTraffic::Close()
{
  delete radio_traffic;
  radio_traffic = nullptr;
}

void RadioTraffic::SendQuickMessage(const Ship* ship, int action)
{
  if (!ship)
    return;

  Element* elem = ship->GetElement();

  if (elem)
  {
    if (action >= RadioMessage::REQUEST_PICTURE)
    {
      Ship* controller = ship->GetController();

      if (controller && !ship->IsStarship())
      {
        auto msg = NEW RadioMessage(controller, ship, action);
        Transmit(msg);
      }
    }
    else if (action >= RadioMessage::SPLASH_1 && action <= RadioMessage::DISTRESS)
    {
      auto msg = NEW RadioMessage(static_cast<Element*>(nullptr), ship, action);
      Transmit(msg);
    }
    else
    {
      auto msg = NEW RadioMessage(elem, ship, action);

      if (action == RadioMessage::ATTACK || action == RadioMessage::ESCORT)
        msg->AddTarget(ship->GetTarget());

      Transmit(msg);
    }
  }
}

void RadioTraffic::Transmit(RadioMessage* msg)
{
  if (msg && radio_traffic)
  {
    NetGame* net_game = NetGame::GetInstance();
    if (net_game)
    {
      NetCommMsg net_msg;
      net_msg.SetRadioMessage(msg);
      net_game->SendData(&net_msg);
    }

    radio_traffic->SendMessage(msg);
  }
}

void RadioTraffic::SendMessage(RadioMessage* msg)
{
  if (!msg)
    return;

  Sim* sim = Sim::GetSim();
  Ship* player = sim->GetPlayerShip();
  int iff = 0;

  if (player)
    iff = player->GetIFF();

  if (msg->DestinationShip())
  {
    traffic.append(msg);

    if (msg->Channel() == 0 || msg->Channel() == iff)
      DisplayMessage(msg);

    if (!NetGame::IsNetGameClient())
      msg->DestinationShip()->HandleRadioMessage(msg);
  }

  else if (msg->DestinationElem())
  {
    traffic.append(msg);

    if (msg->Channel() == 0 || msg->Channel() == iff)
      DisplayMessage(msg);

    if (!NetGame::IsNetGameClient())
      msg->DestinationElem()->HandleRadioMessage(msg);
  }

  else
  {
    if (msg->Channel() == 0 || msg->Channel() == iff)
      DisplayMessage(msg);

    delete msg;
  }
}

std::string RadioTraffic::TranslateVox(const char* phrase)
{
  std::string vox = "vox.";
  vox += phrase;
  std::ranges::transform(vox, vox.begin(), ::tolower);
  vox = Game::GetText(vox);

  if (vox.contains("vox."))  // failed to translate
    return std::string(phrase);    // return the original text

  return vox;
}

void RadioTraffic::DisplayMessage(RadioMessage* msg)
{
  if (!msg)
    return;

  char txt_buf[256];
  txt_buf[0] = 0;
  char msg_buf[128];
  msg_buf[0] = 0;
  char src_buf[64];
  src_buf[0] = 0;
  char dst_buf[64];
  dst_buf[0] = 0;
  char act_buf[64];
  act_buf[0] = 0;
  int vox_channel = 0;

  Ship* dst_ship = msg->DestinationShip();
  Element* dst_elem = msg->DestinationElem();

  // BUILD SRC AND DST BUFFERS -------------------

  if (msg->Sender())
  {
    const Ship* sender = msg->Sender();

    // orders to self?
    if (dst_elem && dst_elem->NumShips() == 1 && dst_elem->GetShip(1) == sender)
    {
      if (msg->Action() >= RadioMessage::CALL_ENGAGING)
      {
        sprintf_s(src_buf, "%s", sender->Name().c_str());

        if (sender->IsStarship())
          vox_channel = (sender->Identity() % 3) + 5;
      }
    }

    // orders to other ships:
    else
    {
      if (sender->IsStarship())
        vox_channel = (sender->Identity() % 3) + 5;
      else
        vox_channel = sender->GetElementIndex();

      if (msg->Action() >= RadioMessage::CALL_ENGAGING)
        sprintf_s(src_buf, "%s", sender->Name().c_str());
      else
      {
        sprintf_s(src_buf, "This is %s", sender->Name().c_str());

        if (dst_ship)
        {
          // internal announcement
          if (dst_ship->GetElement() == sender->GetElement())
          {
            dst_elem = sender->GetElement();
            int index = sender->GetElementIndex();

            if (index > 1 && dst_elem)
            {
              sprintf_s(dst_buf, "%s Leader", dst_elem->Name().data());
              sprintf_s(src_buf, "this is %s %d", dst_elem->Name().data(), index);
            }
            else
              sprintf_s(src_buf, "this is %s leader", dst_elem->Name().data());
          }

          else
          {
            strcpy_s(dst_buf, dst_ship->Name().c_str());
            src_buf[0] = tolower(src_buf[0]);
          }
        }

        else if (dst_elem)
        {
          // flight
          if (dst_elem->NumShips() > 1)
          {
            sprintf_s(dst_buf, "%s Flight", dst_elem->Name().data());

            // internal announcement
            if (sender->GetElement() == dst_elem)
            {
              int index = sender->GetElementIndex();

              if (index > 1)
              {
                sprintf_s(dst_buf, "%s Leader", dst_elem->Name().data());
                sprintf_s(src_buf, "this is %s %d", dst_elem->Name().data(), index);
              }
              else
                sprintf_s(src_buf, "this is %s leader", dst_elem->Name().data());
            }
          }

          // solo
          else
          {
            strcpy_s(dst_buf, dst_elem->Name().c_str());
            src_buf[0] = tolower(src_buf[0]);
          }
        }
      }
    }
  }

  // BUILD ACTION AND TARGET BUFFERS -------------------

  SimObject* target = nullptr;

  strcpy_s(act_buf, RadioMessage::ActionName(msg->Action()));

  if (msg->TargetList().size() > 0)
    target = msg->TargetList()[0];

  if (msg->Action() == RadioMessage::ACK || msg->Action() == RadioMessage::NACK)
  {
    if (dst_ship == msg->Sender())
    {
      src_buf[0] = 0;
      dst_buf[0] = 0;

      if (msg->Action() == RadioMessage::ACK)
        sprintf_s(msg_buf, "%s.", TranslateVox("Acknowledged").data());
      else
        sprintf_s(msg_buf, "%s.", TranslateVox("Unable").data());
    }
    else if (msg->Sender())
    {
      dst_buf[0] = 0;

      if (msg->Info().length())
        sprintf_s(msg_buf, "%s. %s", TranslateVox(act_buf).data(), msg->Info().data());
      else
        sprintf_s(msg_buf, "%s.", TranslateVox(act_buf).data());
    }
    else
    {
      if (msg->Info().length())
        sprintf_s(msg_buf, "%s. %s", TranslateVox(act_buf).data(), msg->Info().data());
      else
        sprintf_s(msg_buf, "%s.", TranslateVox(act_buf).data());
    }
  }

  else if (msg->Action() == RadioMessage::MOVE_PATROL)
    sprintf_s(msg_buf, "%s.", TranslateVox("Move patrol.").data());

  else if (target && dst_ship && msg->Sender())
  {
    Contact* c = msg->Sender()->FindContact(target);

    if (c && c->GetIFF(msg->Sender()) > 10)
      sprintf_s(msg_buf, "%s %s.", TranslateVox(act_buf).c_str(), TranslateVox("unknown contact").c_str());

    else
      sprintf_s(msg_buf, "%s %s.", TranslateVox(act_buf).c_str(), target->Name().c_str());
  }

  else if (target)
    sprintf_s(msg_buf, "%s %s.", TranslateVox(act_buf).c_str(), target->Name().c_str());

  else if (!msg->Info().empty())
    sprintf_s(msg_buf, "%s %s", TranslateVox(act_buf).c_str(), msg->Info().c_str());

  else
    strcpy_s(msg_buf, TranslateVox(act_buf).data());

  char last_char = msg_buf[strlen(msg_buf) - 1];
  if (last_char != '!' && last_char != '.' && last_char != '?')
    strcat_s(msg_buf, ".");

  // final format:
  if (dst_buf[0] && src_buf[0])
  {
    sprintf_s(txt_buf, "%s %s. %s", TranslateVox(dst_buf).data(), TranslateVox(src_buf).data(), msg_buf);
    txt_buf[0] = toupper(txt_buf[0]);
  }

  else if (src_buf[0])
  {
    sprintf_s(txt_buf, "%s. %s", TranslateVox(src_buf).data(), msg_buf);
    txt_buf[0] = toupper(txt_buf[0]);
  }

  else if (dst_buf[0])
  {
    sprintf_s(txt_buf, "%s %s", TranslateVox(dst_buf).data(), msg_buf);
    txt_buf[0] = toupper(txt_buf[0]);
  }

  else
    strcpy_s(txt_buf, msg_buf);

  // vox:
  const char* path[8] = {"1", "1", "2", "3", "4", "5", "6", "7"};

  auto vox = NEW RadioVox(vox_channel, path[vox_channel], txt_buf);

  if (vox)
  {
    vox->AddPhrase(dst_buf);
    vox->AddPhrase(src_buf);
    vox->AddPhrase(act_buf);

    if (!vox->Start())
    {
      RadioView::Message(txt_buf);
      delete vox;
    }
  }
}

void RadioTraffic::DiscardMessages()
{
  if (radio_traffic)
    radio_traffic->traffic.destroy();
}

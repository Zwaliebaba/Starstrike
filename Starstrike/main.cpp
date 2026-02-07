#include "pch.h"
#include "HUDView.h"
#include "NetBrokerClient.h"
#include "NetLayer.h"
#include "StarServer.h"
#include "Starshatter.h"
#include "Token.h"

extern int VD3D_describe_things;
int dump_missions = 0;

auto g_versionInfo = "5.1.87 EX";

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  int result = 0;
  int test_mode = 0;
  int do_server = 0;

  if (strstr(lpCmdLine, "-test"))
  {
    DebugTrace("  Request TEST mode\n");
    test_mode = 1;
  }

  if (strstr(lpCmdLine, "-fps"))
    HUDView::ShowFPS(true);

  if (strstr(lpCmdLine, "-dump"))
  {
    DebugTrace("  Request dump dynamic missions\n");
    dump_missions = 1;
  }

  if (strstr(lpCmdLine, "-lan"))
  {
    DebugTrace("  Request LAN ONLY mode\n");
    NetBrokerClient::Disable();
  }

  if (strstr(lpCmdLine, "-server"))
  {
    do_server = 1;
    DebugTrace("  Request Standalone Server Mode\n");
  }

  char* d3dinfo = strstr(lpCmdLine, "-d3d");
  if (d3dinfo)
  {
    int n = d3dinfo[4] - '0';

    if (n >= 0 && n <= 5)
      VD3D_describe_things = n;

    DebugTrace("  D3D Info Level: %d\n", VD3D_describe_things);
  }
  else
    VD3D_describe_things = 0;

  try
  {
    NetLayer net;

    if (do_server)
    {
      auto server = NEW StarServer();

      if (server->Init(hInstance, hPrevInstance, lpCmdLine, nCmdShow))
        result = server->Run();

      DebugTrace("\n+====================================================================+\n");
      DebugTrace("  Begin Shutdown...\n");

      delete server;
    }

    else
    {
      Starshatter* stars = NEW Starshatter;
      stars->SetTestMode(test_mode);

      if (stars->Init(hInstance, hPrevInstance, lpCmdLine, nCmdShow))
        result = stars->Run();

      DebugTrace("\n+====================================================================+\n");
      DebugTrace("  Begin Shutdown...\n");

      delete stars;
    }

    Token::close();

    if (*Game::GetPanicMessage())
      MessageBoxA(nullptr, Game::GetPanicMessage(), "Starshatter - Error", MB_OK);
  }

  catch (const char* msg) { DebugTrace("  FATAL EXCEPTION: '%s'\n", msg); }

  DebugTrace("+====================================================================+\n");
  DebugTrace(" END OF LINE.\n");

  return result;
}

#include "pch.h"
#include "network_window.h"
#include "GameApp.h"
#include "AuthoritativeServer.h"
#include "PredictiveClient.h"
#include "main.h"
#include "DX9TextRenderer.h"

NetworkWindow::NetworkWindow(const char* name)
  : GuiWindow(name) {}

void NetworkWindow::Render(bool hasFocus)
{
  GuiWindow::Render(hasFocus);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  //
  // Render some Networking stats

  if (g_app->m_server)
  {
#ifdef PROFILER_ENABLED
    //        g_editorFont.DrawText2D( m_x + 10, m_y + 120, DEF_FONT_SIZE,
    //			"Server SEND  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Server Send") );
    //        g_editorFont.DrawText2D( m_x + 10, m_y + 135, DEF_FONT_SIZE,
    //			"Server RECV  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Server Receive") );
#endif // PROFILER_ENABLED
    // Server tick info not directly exposed in new system
    g_editorFont.DrawText2D(m_x + 10, m_y + 30, DEF_FONT_SIZE, "SERVER (Authoritative)");
  }

#ifdef PROFILER_ENABLED
  //    g_editorFont.DrawText2D( m_x + 10, m_y + 160, DEF_FONT_SIZE,
  //		"Client SEND  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Client Send") );
  //    g_editorFont.DrawText2D( m_x + 10, m_y + 175, DEF_FONT_SIZE,
  //		"Client RECV  : %4.0f bytes", g_app->m_profiler->GetTotalTime("Client Receive") );
#endif // PROFILER_ENABLED

  if (g_app->m_client)
  {
    g_editorFont.DrawText2D(m_x + 10, m_y + 45, DEF_FONT_SIZE, "CLIENT ServerTick : %d", g_app->m_client->GetServerTick());
    g_editorFont.DrawText2D(m_x + 10, m_y + 60, DEF_FONT_SIZE, "CLIENT LocalTick  : %d", g_app->m_client->GetLocalTick());
    g_editorFont.DrawText2D(m_x + 10, m_y + 75, DEF_FONT_SIZE, "Interpolation Delay: %.0fms", g_app->m_client->GetInterpolationDelay() * 1000.0f);
    g_editorFont.DrawText2D(m_x + 10, m_y + 90, DEF_FONT_SIZE, "Connected: %s", g_app->m_client->IsConnected() ? "Yes" : "No");
  }
}

#include "pch.h"
#include "NetFileServlet.h"
#include "DataLoader.h"
#include "NetAdminServer.h"
#include "NetLayer.h"

bool NetFileServlet::DoGet(HttpRequest& request, HttpResponse& response)
{
  if (!CheckUser(request, response))
    return true;

  std::string content;
  std::string path = request.GetParam("path");
  std::string name = request.GetParam("name");

  if (name.length())
  {
    BYTE* buffer = nullptr;
    DataLoader* loader = DataLoader::GetLoader();

    if (loader)
    {
      loader->SetDataPath(path);
      int len = loader->LoadBuffer(name, buffer);

      if (len)
        content = std::string((const char*)buffer, len);

      loader->ReleaseBuffer(buffer);
      loader->SetDataPath();
    }
  }

  response.SetStatus(HttpResponse::SC_OK);
  response.AddHeader("MIME-Version", "1.0");
  response.AddHeader("Cache-Control", "no-cache");
  response.AddHeader("Expires", "-1");
  response.AddHeader("Content-Type", "text/plain");
  response.SetContent(content);

  return true;
}

bool NetWebServlet::DoGet(HttpRequest& request, HttpResponse& response)
{
  std::string content;
  std::string name = request.URI();
  bool found = false;

  if (name.length() > 4)
  {
    char filename[256];
    strcpy_s(filename, name.data() + 1);  // skip leading '/'

    FILE* f;
    fopen_s(&f, filename, "rb");

    if (f)
    {
      fseek(f, 0, SEEK_END);
      int len = ftell(f);
      fseek(f, 0, SEEK_SET);

      auto buf = NEW BYTE[len];

      fread(buf, len, 1, f);
      fclose(f);

      content = std::string((const char*)buf, len);
      delete [] buf;

      found = true;
      DebugTrace("weblog: 200 OK %s %d bytes\n", name.data(), len);
    }
    else
      DebugTrace("weblog: 404 Not Found %s\n", name.data());
  }

  if (found)
  {
    std::string content_type = "text/plain";

    if (name.contains(".gif"))
      content_type = "image/gif";
    else if (name.contains(".jpg"))
      content_type = "image/jpeg";
    else if (name.contains(".htm"))
      content_type = "text/html";

    response.SetStatus(HttpResponse::SC_OK);
    response.AddHeader("MIME-Version", "1.0");
    response.AddHeader("Content-Type", content_type);
    response.SetContent(content);
  }
  else
    response.SetStatus(HttpResponse::SC_NOT_FOUND);

  return true;
}

#include "pch.h"
#include "NetAuth.h"
#include "NetUser.h"
#include "Random.h"
#include "sha1.h"

static int auth_level = NetAuth::NET_AUTH_MINIMAL;

int NetAuth::AuthLevel() { return auth_level; }

void NetAuth::SetAuthLevel(int n)
{
  if (n >= NET_AUTH_MINIMAL && n <= NET_AUTH_SECURE)
    auth_level = n;
}

std::string NetAuth::CreateAuthRequest(NetUser* u)
{
  std::string request;

  if (u)
  {
    u->SetAuthLevel(auth_level);

    if (auth_level == NET_AUTH_MINIMAL)
    {
      u->SetAuthState(NET_AUTH_OK);
      u->SetSalt("Very Low Sodium");
    }

    else if (auth_level == NET_AUTH_STANDARD)
    {
      u->SetAuthState(NET_AUTH_INITIAL);
      u->SetSalt("Very Low Sodium");

      request = "level 1";
    }

    else
    {
      char salt[33];

      for (int i = 0; i < 32; i++)
        salt[i] = static_cast<char>('0' + (int)Random(0, 9.4));

      salt[32] = 0;
      u->SetSalt(salt);
      u->SetAuthState(NET_AUTH_INITIAL);

      request = "level 2 salt ";
      request += salt;
    }
  }

  return request;
}

static std::string Digest(std::string_view salt, std::string_view file)
{
  int length = 0;
  int offset = 0;
  char block[4096];
  char digest[64];

  ZeroMemory(digest, sizeof(digest));

  if (!file.empty())
  {
    FILE* f;
    fopen_s(&f, file.data(), "rb");

    if (f)
    {
      SHA1 sha1;

      if (!salt.empty())
        sha1.Input(salt.data(), salt.length());

      fseek(f, 0, SEEK_END);
      length = ftell(f);
      fseek(f, 0, SEEK_SET);

      while (offset < length)
      {
        int n = fread(block, sizeof(char), 4096, f);
        sha1.Input(block, n);
        offset += n;
      }

      fclose(f);

      unsigned result[5];
      if (sha1.Result(result))
      {
        sprintf_s(digest, "SHA1_%08X_%08X_%08X_%08X_%08X", result[0], result[1], result[2], result[3], result[4]);
      }
    }
  }

  return digest;
}

std::string NetAuth::CreateAuthResponse(int level, std::string_view salt)
{
  std::string response;

  if (level == NET_AUTH_SECURE)
  {
    response += "exe ";
    response += Digest(salt, "stars.exe");    // XXX should look up name of this exe
    response += " ";

    response += "dat ";
    response += Digest(salt, "shatter.dat");
    response += " ";

    response += "etc ";
    response += Digest(salt, "start.dat");
    response += " ";
  }

  // We used to have the mod list here, but I removed it
  if (level >= NET_AUTH_STANDARD)
  {
    char buffer[32];
    sprintf_s(buffer, "num 0 ");
    response += buffer;
  }

  return response;
}

bool NetAuth::AuthUser(NetUser* u, const std::string& response)
{
  bool authentic = false;

  if (auth_level == NET_AUTH_MINIMAL)
  {  // (this case should not occur)
    if (u)
    {
      u->SetAuthLevel(auth_level);
      u->SetAuthState(NET_AUTH_OK);
    }

    authentic = (u != nullptr);
  }

  else if (u)
  {
    std::string expected_response = CreateAuthResponse(auth_level, u->Salt());
    if (expected_response == response)
      authentic = true;

    u->SetAuthState(authentic ? NET_AUTH_OK : NET_AUTH_FAILED);
  }

  return authentic;
}

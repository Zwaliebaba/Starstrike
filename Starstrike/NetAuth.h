#ifndef NetAuth_h
#define NetAuth_h

#include "NetUser.h"

class NetAuth
{
  public:
    enum AUTH_STATE
    {
      NET_AUTH_INITIAL = 0,
      NET_AUTH_FAILED  = 1,
      NET_AUTH_OK      = 2
    };

    enum AUTH_LEVEL
    {
      NET_AUTH_MINIMAL  = 0,
      NET_AUTH_STANDARD = 1,
      NET_AUTH_SECURE   = 2
    };

    static int AuthLevel();
    static void SetAuthLevel(int n);

    static std::string CreateAuthRequest(NetUser* u);
    static std::string CreateAuthResponse(int level, std::string_view salt);
    static bool AuthUser(NetUser* u, const std::string& response);
};

#endif NetAuth_h

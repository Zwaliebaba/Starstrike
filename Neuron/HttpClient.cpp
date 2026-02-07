#include "pch.h"

#include "HttpClient.h"
#include "NetHost.h"
#include "NetLayer.h"
#include <mmsystem.h>



HttpClient::HttpClient(const NetAddr& server_addr)
    : NetClient(server_addr)
{
}

HttpClient::~HttpClient()
{
    cookies.destroy();
}

HttpResponse*
HttpClient::DoRequest(HttpRequest& request)
{
    // add existing cookies to request before sending:
    CombineCookies(request.GetCookies(), cookies);

    std::string req = request.operator std::string();
    std::string msg = SendRecv(req);
    HttpResponse*  response = NEW HttpResponse(msg);

    if (response) {
        // save cookies returned in response:
        CombineCookies(cookies, response->GetCookies());
    }

    return response;
}

void
HttpClient::CombineCookies(List<HttpParam>& dst, List<HttpParam>& src)
{
    for (int i = 0; i < src.size(); i++) {
        HttpParam* s = src[i];
        HttpParam* d = dst.find(s);

        if (d) {
            d->value = s->value;
        }
        else {
            HttpParam* cookie = NEW HttpParam(s->name, s->value);
            if (cookie)
                dst.append(cookie);
        }
    }
}
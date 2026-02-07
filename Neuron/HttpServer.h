#ifndef HttpServer_h
#define HttpServer_h

#include "List.h"
#include "NetServer.h"

class HttpParam;
class HttpRequest;
class HttpResponse;

class HttpServer : public NetServer
{
  public:
    HttpServer(WORD port, int poolsize = 1);
    ~HttpServer() override = default;

    int operator ==(const HttpServer& l) const { return addr == l.addr; }

    std::string ProcessRequest(std::string request, const NetAddr& addr) override;
    std::string DefaultResponse() override;
    std::string ErrorResponse() override;

    virtual bool DoGet(HttpRequest& request, HttpResponse& response);
    virtual bool DoPost(HttpRequest& request, HttpResponse& response);
    virtual bool DoHead(HttpRequest& request, HttpResponse& response);

    virtual std::string GetServerName();
    virtual void SetServerName(std::string_view name);

  protected:
    std::string http_server_name;
};

class HttpParam
{
  public:
    HttpParam(std::string_view n, std::string_view v)
      : name(n),
        value(v) {}

    int operator ==(const HttpParam& p) const { return name == p.name; }

    std::string name;
    std::string value;
};

class HttpRequest
{
  public:
    enum METHOD
    {
      HTTP_OPTIONS,
      HTTP_GET,
      HTTP_HEAD,
      HTTP_POST,
      HTTP_PUT,
      HTTP_DELETE,
      HTTP_TRACE,
      HTTP_CONNECT
    };

    HttpRequest(std::string_view request = {});
    ~HttpRequest();

    operator std::string();

    void ParseRequest(std::string_view request);
    void ParseCookie(std::string_view param);

    int Method() const { return method; }
    std::string URI() const { return uri; }
    std::string Content() const { return content; }
    std::string RequestLine() const { return request_line; }

    List<HttpParam>& GetQuery() { return query; }
    List<HttpParam>& GetHeaders() { return headers; }
    List<HttpParam>& GetCookies() { return cookies; }

    NetAddr GetClientAddr() const { return client_addr; }
    void SetClientAddr(const NetAddr& a) { client_addr = a; }

    std::string GetParam(std::string_view name);

    std::string GetHeader(std::string_view name);
    void SetHeader(std::string_view name, std::string_view value);
    void AddHeader(std::string_view name, std::string_view value);
    std::string GetCookie(std::string_view name);
    void SetCookie(std::string_view name, std::string_view value);
    void AddCookie(std::string_view name, std::string_view value);

    std::string DecodeParam(std::string_view value);
    static std::string EncodeParam(std::string_view value);

  private:
    int method;
    std::string uri;
    std::string content;
    std::string request_line;
    NetAddr client_addr;

    List<HttpParam> query;
    List<HttpParam> headers;
    List<HttpParam> cookies;
};

class HttpResponse
{
  public:
    enum STATUS
    {
      SC_CONTINUE              = 100,
      SC_SWITCHING_PROTOCOLS   = 101,
      SC_OK                    = 200,
      SC_CREATED               = 201,
      SC_ACCEPTED              = 202,
      SC_NON_AUTHORITATIVE     = 203,
      SC_NO_CONTENT            = 204,
      SC_RESET_CONTENT         = 205,
      SC_PARTIAL_CONTENT       = 206,
      SC_MULTIPLE_CHOICES      = 300,
      SC_MOVED_PERMANENTLY     = 301,
      SC_FOUND                 = 302,
      SC_SEE_OTHER             = 303,
      SC_NOT_MODIFIED          = 304,
      SC_USE_PROXY             = 305,
      SC_TEMPORARY_REDIRECT    = 307,
      SC_BAD_REQUEST           = 400,
      SC_UNAUTHORIZED          = 401,
      SC_PAYMENT_REQUIRED      = 402,
      SC_FORBIDDEN             = 403,
      SC_NOT_FOUND             = 404,
      SC_METHOD_NOT_ALLOWED    = 405,
      SC_NOT_ACCEPTABLE        = 406,
      SC_PROXY_AUTH_REQ        = 407,
      SC_REQUEST_TIME_OUT      = 408,
      SC_CONFLICT              = 409,
      SC_GONE                  = 410,
      SC_LENGTH_REQUIRED       = 411,
      SC_SERVER_ERROR          = 500,
      SC_NOT_IMPLEMENTED       = 501,
      SC_BAD_GATEWAY           = 502,
      SC_SERVICE_UNAVAILABLE   = 503,
      SC_GATEWAY_TIMEOUT       = 504,
      SC_VERSION_NOT_SUPPORTED = 505
    };

    HttpResponse(int status = 500, std::string_view content = {});
    HttpResponse(std::string_view response);
    ~HttpResponse();

    operator std::string();

    void ParseResponse(std::string_view request);
    void ParseCookie(std::string_view param);

    int Status() const { return status; }
    void SetStatus(int s) { status = s; }

    std::string Content() const { return content; }
    void SetContent(std::string t) { content = t; }
    void AddContent(std::string t) { content += t; }

    List<HttpParam>& GetHeaders() { return headers; }
    List<HttpParam>& GetCookies() { return cookies; }

    std::string GetHeader(std::string_view name);
    void SetHeader(std::string_view name, std::string_view value);
    void AddHeader(std::string_view name, std::string_view value);
    std::string GetCookie(std::string_view name);
    void SetCookie(std::string_view name, std::string_view value);
    void AddCookie(std::string_view name, std::string_view value);

    void SendRedirect(std::string_view url);

  private:
    int status;
    std::string content;

    List<HttpParam> headers;
    List<HttpParam> cookies;
};

#endif HttpServer_h

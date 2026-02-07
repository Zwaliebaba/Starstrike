#ifndef NetAddr_h
#define NetAddr_h

#undef SetPort

class NetAddr
{
  public:
    NetAddr(std::string_view a, WORD p = 0);
    NetAddr(DWORD a = 0, WORD p = 0);
    NetAddr(const NetAddr& n);

    int operator ==(const NetAddr& a) const { return addr == a.addr && port == a.port; }

    DWORD IPAddr() const { return addr; }
    BYTE B4() const { return (BYTE)((addr & 0xff000000) >> 24); }
    BYTE B3() const { return (BYTE)((addr & 0x00ff0000) >> 16); }
    BYTE B2() const { return (BYTE)((addr & 0x0000ff00) >> 8); }
    BYTE B1() const { return (BYTE)((addr & 0x000000ff)); }

    WORD Port() const { return port; }
    void SetPort(WORD p) { port = p; }

    sockaddr* GetSockAddr() const;
    size_t GetSockAddrLength() const;

    void SetSockAddr(sockaddr* s, int size);
    void InitFromSockAddr();

  private:
    void Init();

    DWORD addr; // IP addr in host byte order
    WORD port; // IP port in host byte order
    sockaddr_in sadr;
};

#endif NetAddr_h

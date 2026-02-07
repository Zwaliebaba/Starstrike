#ifndef WebBrowser_h
#define WebBrowser_h

class WebBrowser
{
  public:
    
    WebBrowser();
    virtual ~WebBrowser();

    virtual void OpenURL(const char* url);

  protected:
    void FindDefaultBrowser();
    void FindOpenCommand();

    std::wstring browser;
    std::wstring command;
};

#endif WebBrowser_h
